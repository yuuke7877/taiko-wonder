#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {
struct Room {
  std::string id, name, host, mode;
  bool isPublic = false;
  int players = 1;
  int batonMeasures = 4;
  std::chrono::steady_clock::time_point created = std::chrono::steady_clock::now();
};
std::map<std::string, Room> rooms;
std::mutex roomsMutex;

bool validWord(const std::string& value, size_t maxLength = 24) {
  return !value.empty() && value.size() <= maxLength &&
      std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return c >= 0x21 && c <= 0x7e && c != '<' && c != '>';
      });
}
std::string randomCode() {
  static std::mutex rngMutex;
  static std::mt19937 rng(std::random_device{}());
  std::lock_guard<std::mutex> rngLock(rngMutex);
  std::lock_guard<std::mutex> roomLock(roomsMutex);
  std::uniform_int_distribution<int> distribution(1000, 9999);
  for (int attempt = 0; attempt < 9000; ++attempt) {
    auto code = std::to_string(distribution(rng));
    if (!rooms.count(code)) return code;
  }
  return {};
}
bool sendAll(int fd, const std::string& text) {
  size_t sent = 0;
  while (sent < text.size()) {
    ssize_t n = ::send(fd, text.data() + sent, text.size() - sent, MSG_NOSIGNAL);
    if (n <= 0) return false;
    sent += static_cast<size_t>(n);
  }
  return true;
}
void sendLine(int fd, const std::string& line) { sendAll(fd, line + "\n"); }
void handleCommand(int fd, const std::string& line) {
  std::istringstream input(line);
  std::string command;
  input >> command;
  if (command == "HELP") {
    sendLine(fd, "OK HELP LIST | CREATE <public|private> <room> <host> <mode> <baton-measures> | JOIN <code> | LEAVE | PING | QUIT");
  } else if (command == "PING") {
    sendLine(fd, "PONG");
  } else if (command == "LIST") {
    std::lock_guard<std::mutex> lock(roomsMutex);
    for (const auto& entry : rooms) {
      const Room& room = entry.second;
      if (room.isPublic)
        sendLine(fd, "ROOM " + room.id + " " + room.name + " " + std::to_string(room.players) + " " + room.host + " " + room.mode + " " + std::to_string(room.batonMeasures));
    }
    sendLine(fd, "END");
  } else if (command == "CREATE") {
    std::string visibility, name, host, mode;
    int measures = 4;
    input >> visibility >> name >> host >> mode >> measures;
    if ((visibility != "public" && visibility != "private") || !validWord(name) || !validWord(host) ||
        !validWord(mode) || measures < 1 || measures > 32) {
      sendLine(fd, "ERROR usage: CREATE <public|private> <room> <host> <mode> <baton-measures-1-to-32>");
      return;
    }
    std::string id = randomCode();
    if (id.empty()) { sendLine(fd, "ERROR room-capacity"); return; }
    Room room;
    room.id = id; room.name = name; room.host = host; room.mode = mode;
    room.isPublic = visibility == "public"; room.batonMeasures = measures;
    {
      std::lock_guard<std::mutex> lock(roomsMutex);
      if (rooms.size() >= 500) { sendLine(fd, "ERROR room-capacity"); return; }
      rooms.emplace(id, room);
    }
    sendLine(fd, "CREATED " + id + " " + (room.isPublic ? "public" : "private"));
  } else if (command == "JOIN") {
    std::string code;
    input >> code;
    std::lock_guard<std::mutex> lock(roomsMutex);
    auto it = rooms.find(code);
    if (it == rooms.end()) sendLine(fd, "ERROR room-not-found");
    else if (it->second.players >= 8) sendLine(fd, "ERROR room-full");
    else { ++it->second.players; sendLine(fd, "JOINED " + it->second.id + " " + std::to_string(it->second.players)); }
  } else if (command == "LEAVE") {
    // This directory-only MVP does not retain client membership across short HTTP-like TCP requests.
    sendLine(fd, "OK");
  } else if (command == "QUIT") {
    sendLine(fd, "BYE");
  } else if (!command.empty()) {
    sendLine(fd, "ERROR unknown-command");
  }
}
void client(int fd) {
  timeval timeout{5, 0};
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  sendLine(fd, "taiko-wonder-room-server 0.2");
  std::string buffer;
  char chunk[1024];
  bool keepGoing = true;
  while (keepGoing) {
    ssize_t n = ::recv(fd, chunk, sizeof(chunk), 0);
    if (n <= 0) break;
    buffer.append(chunk, static_cast<size_t>(n));
    if (buffer.size() > 4096) { sendLine(fd, "ERROR command-too-long"); break; }
    size_t pos;
    while ((pos = buffer.find('\n')) != std::string::npos) {
      std::string line = buffer.substr(0, pos);
      buffer.erase(0, pos + 1);
      if (!line.empty() && line.back() == '\r') line.pop_back();
      if (line == "QUIT") { handleCommand(fd, line); keepGoing = false; break; }
      handleCommand(fd, line);
      // Clients issue one request per connection. This also keeps stale connections bounded.
      if (buffer.empty()) { keepGoing = false; break; }
    }
  }
  ::close(fd);
}
int parsePort(int argc, char** argv) {
  int port = argc > 1 ? std::atoi(argv[1]) : 24816;
  return port >= 1 && port <= 65535 ? port : 0;
}
}
int main(int argc, char** argv) {
  int port = parsePort(argc, argv);
  if (!port) { std::cerr << "Usage: taiko-wonder-server [port] [--public]\n"; return 1; }
  // Safe local default. Bind publicly only when explicitly requested; deploy behind TLS/auth/rate limits.
  const bool publicBind = argc > 2 && std::string(argv[2]) == "--public";
  int server = ::socket(AF_INET, SOCK_STREAM, 0);
  if (server < 0) { perror("socket"); return 1; }
  int yes = 1;
  ::setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(publicBind ? INADDR_ANY : INADDR_LOOPBACK);
  address.sin_port = htons(static_cast<uint16_t>(port));
  if (::bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 || ::listen(server, 32) < 0) {
    perror("bind/listen"); ::close(server); return 1;
  }
  std::cout << "taiko wonder room directory listening on " << (publicBind ? "all interfaces" : "127.0.0.1") << ':' << port << '\n';
  while (true) {
    int fd = ::accept(server, nullptr, nullptr);
    if (fd < 0) { if (errno == EINTR) continue; perror("accept"); continue; }
    std::thread(client, fd).detach();
  }
}
