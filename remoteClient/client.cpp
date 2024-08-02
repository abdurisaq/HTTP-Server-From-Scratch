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

#define PORT 10000

#define BROADCAST_IP "255.255.255.255"
int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  struct sockaddr_in broadcastAddr, recvAddr;
  socklen_t addrLen = sizeof(recvAddr);

  int clientFD = socket(AF_INET, SOCK_STREAM, 0);
  if (clientFD < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int broadcast = 1;
  if (setsockopt(clientFD, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  broadcastAddr.sin_family = AF_INET;
  broadcastAddr.sin_addr.s_addr = inet_addr("TARGET-IP-ADDRESS");
  broadcastAddr.sin_port = htons(PORT);


  std::cout<<"Sending discovery message to ip address "<<broadcastAddr.sin_addr.s_addr<<"\n";

  if(connect(clientFD, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) < 0){
    std::cerr << "Failed to connect to server\n";
    close(clientFD);
    return 1;
  } 

  std::string discoveryMessage = "DISCOVER_SERVER";
  send(clientFD, discoveryMessage.c_str(), discoveryMessage.size(), 0);
  char buffer[1024];
  int n = recvfrom(clientFD, buffer, sizeof(buffer), 0, (struct sockaddr *)&recvAddr, &addrLen);
  buffer[n] = '\0';
  std::cout << "Received: " << buffer << "\n";
  std::cout << "Discovered server: " << inet_ntoa(recvAddr.sin_addr) << std::endl;
  close(clientFD);
  return 0;
}
