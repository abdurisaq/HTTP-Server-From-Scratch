#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#define PORT 4221

bool isLocalAddress(const struct sockaddr_in &clientAddr) {
    return true;
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  // ./your_program.sh --directory /tmp/
  std::string directory;
  if(argc == 3){
    if(std::string(argv[1]) == "--directory"){
      directory = argv[2];
      std::cout<<"Directory: "<<directory<<"\n";
    }else{
      std::cerr<<"Invalid arguments\n";
      return 1;
    }
  }
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port \n";
    return 1;
  }
  
  
  struct sockaddr_in clientAddr;
  socklen_t ClientAddrLen = sizeof(clientAddr);
  std::cout<<"listening at ip address: "<<inet_ntoa(server_addr.sin_addr)<<" and port: "<<PORT<<"\n";
  char buffer[1024];
    while (1){
       int n = recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &ClientAddrLen);
      buffer[n] = '\0';

      std::string clientMessage(buffer);
      if (clientMessage == "DISCOVER_SERVER") {
          if (isLocalAddress(clientAddr)) {
              std::string serverResponse = "SERVER_RESPONSE: " + std::to_string(PORT);
              sendto(server_fd, serverResponse.c_str(), serverResponse.size(), 0, (const struct sockaddr *)&clientAddr, ClientAddrLen);
          } else {
              std::cout << "Ignoring request from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
          }
      }
    }

  close(server_fd);
  
  return 0;
}
