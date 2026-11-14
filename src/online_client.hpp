#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <cstdlib>
#include <sstream>
#include <string>

namespace tw_net {
inline std::string request(const std::string& command) {
  const char* configured = std::getenv("TAIKO_WONDER_SERVER");
  std::string endpoint = configured && *configured ? configured : "127.0.0.1:24816";
  const auto colon = endpoint.rfind(':');
  if (colon == std::string::npos) return "ERROR expected HOST:PORT in TAIKO_WONDER_SERVER";
  const std::string host = endpoint.substr(0, colon);
  const std::string port = endpoint.substr(colon + 1);
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* addresses = nullptr;
  if (::getaddrinfo(host.c_str(), port.c_str(), &hints, &addresses) != 0) return "ERROR cannot-resolve-server";
  int fd = -1;
  for (addrinfo* a = addresses; a; a = a->ai_next) {
    fd = ::socket(a->ai_family, a->ai_socktype, a->ai_protocol);
    if (fd >= 0 && ::connect(fd, a->ai_addr, a->ai_addrlen) == 0) break;
    if (fd >= 0) ::close(fd);
    fd = -1;
  }
  ::freeaddrinfo(addresses);
  if (fd < 0) return "ERROR cannot-connect";
  timeval timeout{3, 0};
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  const std::string payload = command + "\n";
  size_t sent = 0;
  while (sent < payload.size()) {
    ssize_t n = ::send(fd, payload.data() + sent, payload.size() - sent, MSG_NOSIGNAL);
    if (n <= 0) { ::close(fd); return "ERROR send-failed"; }
    sent += static_cast<size_t>(n);
  }
  std::string response;
  char buffer[1024];
  while (response.size() < 8192) {
    ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
    if (n <= 0) break;
    response.append(buffer, static_cast<size_t>(n));
    if (response.find("\nEND\n") != std::string::npos || response.rfind("END\n") == response.size() - 4 ||
        response.find("CREATED ") != std::string::npos || response.find("JOINED ") != std::string::npos ||
        response.find("ERROR ") != std::string::npos || response.find("PONG") != std::string::npos) break;
  }
  ::close(fd);
  return response.empty() ? "ERROR no-response" : response;
}
}
