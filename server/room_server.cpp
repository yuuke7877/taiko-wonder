#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
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
struct Room { std::string id, name, host; bool isPublic=false; int players=1; };
std::map<std::string,Room> rooms;
std::mutex roomsMutex;
std::string randomCode() {
  static std::mutex rngMutex; static std::mt19937 rng(std::random_device{}());
  std::lock_guard<std::mutex> lock(rngMutex);
  std::uniform_int_distribution<int> d(1000,9999);
  for(int i=0;i<10000;++i) { auto code=std::to_string(d(rng)); std::lock_guard<std::mutex> roomsLock(roomsMutex); if(!rooms.count(code)) return code; }
  return "0000";
}
void sendLine(int fd,const std::string& line) { std::string msg=line+"\n"; ::send(fd,msg.data(),msg.size(),MSG_NOSIGNAL); }
void client(int fd) {
  sendLine(fd,"taiko-wonder-room-server 0.1 | HELP for commands");
  std::string buffer; char chunk[1024];
  while(true) {
    ssize_t n=::recv(fd,chunk,sizeof(chunk),0); if(n<=0) break; buffer.append(chunk,(size_t)n);
    size_t pos;
    while((pos=buffer.find('\n'))!=std::string::npos) {
      std::string line=buffer.substr(0,pos); buffer.erase(0,pos+1); if(!line.empty()&&line.back()=='\r')line.pop_back();
      std::istringstream in(line); std::string cmd; in>>cmd;
      if(cmd=="HELP") sendLine(fd,"LIST | CREATE <public|private> <name> <host> | JOIN <4-digit-code> | LEAVE | QUIT");
      else if(cmd=="LIST") {
        std::lock_guard<std::mutex> lock(roomsMutex); bool any=false;
        for(const auto& pair:rooms) if(pair.second.isPublic) { any=true; sendLine(fd,"ROOM "+pair.second.id+" "+pair.second.name+" "+std::to_string(pair.second.players)+" "+pair.second.host); }
        if(!any) sendLine(fd,"EMPTY"); sendLine(fd,"END");
      } else if(cmd=="CREATE") {
        std::string visibility,name,host; in>>visibility>>name>>host;
        if((visibility!="public"&&visibility!="private")||name.empty()||host.empty()) {sendLine(fd,"ERROR usage: CREATE <public|private> <single-word-name> <host>");continue;}
        Room r; r.id=randomCode();r.name=name;r.host=host;r.isPublic=visibility=="public";
        {std::lock_guard<std::mutex> lock(roomsMutex);rooms[r.id]=r;}
        sendLine(fd,"CREATED "+r.id+" "+(r.isPublic?"public":"private"));
      } else if(cmd=="JOIN") {
        std::string code;in>>code;std::lock_guard<std::mutex> lock(roomsMutex);auto it=rooms.find(code);
        if(it==rooms.end())sendLine(fd,"ERROR room-not-found");else if(it->second.players>=8)sendLine(fd,"ERROR room-full");else {++it->second.players;sendLine(fd,"JOINED "+it->second.id+" "+std::to_string(it->second.players));}
      } else if(cmd=="LEAVE") sendLine(fd,"OK");
      else if(cmd=="QUIT") {sendLine(fd,"BYE");::close(fd);return;}
      else if(!cmd.empty()) sendLine(fd,"ERROR unknown-command");
    }
  }
  ::close(fd);
}
}
int main(int argc,char** argv) {
  int port=argc>1?std::atoi(argv[1]):24816;
  if(port<1||port>65535) {std::cerr<<"Invalid port\n";return 1;}
  int server=::socket(AF_INET,SOCK_STREAM,0); if(server<0){perror("socket");return 1;}
  int yes=1;setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
  sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_ANY);address.sin_port=htons((uint16_t)port);
  if(::bind(server,(sockaddr*)&address,sizeof(address))<0||::listen(server,32)<0){perror("bind/listen");::close(server);return 1;}
  std::cout<<"taiko wonder room server listening on port "<<port<<"\n";
  while(true) { int fd=::accept(server,nullptr,nullptr); if(fd<0)continue; std::thread(client,fd).detach(); }
}
