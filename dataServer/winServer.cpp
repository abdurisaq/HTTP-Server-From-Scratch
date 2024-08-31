#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <vector>
#define PORT 10000
#include <Windows.h>
#include <winuser.h>
#include <winsock2.h>
#define BROADCAST_IP "255.255.255.255"
void startup(){
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2,2),&wsadata);

}

void handleRequest(int clientFD,sockaddr_in clientAddr ){

}
int main(int argc, char **argv) {
    startup();
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
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

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        perror("bind");
        std::cerr << "Failed to bind to port\n";
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        std::cerr << "Failed to listen on socket\n";
        return 1;
    }

    std::cout << "Listening at IP address: " << inet_ntoa(server_addr.sin_addr) << " and port: " << PORT << "\n";

    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int client_fd = accept(server_fd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (client_fd < 0) {
            std::cerr << "Failed to accept connection\n";
            continue;
        }
        std::string incommingIp = inet_ntoa(clientAddr.sin_addr);
        std::cout << "Got a connection from " << incommingIp << std::endl;
        /*handleRequest(client_fd, clientAddr);*/
        std::thread th(handleRequest, client_fd,clientAddr);


        th.detach();
    }

    close(server_fd);
    return 0;
}
