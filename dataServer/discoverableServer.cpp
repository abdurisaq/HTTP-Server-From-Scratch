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
bool isLocalAddress(const struct sockaddr_in &clientAddr) {
    return true;
}
void discoverServers() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return;
    }
        
    sockaddr_in listenAddr;
    std::memset(&listenAddr, 0, sizeof(listenAddr));
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_addr.s_addr = INADDR_ANY;
    listenAddr.sin_port = htons(PORT);

    if (bind(sock, (sockaddr*)&listenAddr, sizeof(listenAddr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return;
    }

    char buffer[1024];
    sockaddr_in senderAddr;
    socklen_t senderAddrLen = sizeof(senderAddr);
    ssize_t len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&senderAddr, &senderAddrLen);
    std::cout<<"received message\n";
    if (len > 0) {
        buffer[len] = '\0';
        std::cout << "Received broadcast: " << buffer << std::endl;
    }

    close(sock);
}
bool checkLocalAddress(in_addr inAddr) {

    // Convert the in_addr structure to an integer
    uint32_t ipInt = ntohl(inAddr.s_addr);

    // Check for 10.0.0.0/8
    if ((ipInt & 0xFF000000) == 0x0A000000) { // 0xFF000000 = 255.0.0.0, 0x0A000000 = 10.0.0.0
        return true;
    }

    // Check for 172.16.0.0/12
    if ((ipInt & 0xFFF00000) == 0xAC100000) { // 0xFFF00000 = 255.240.0.0, 0xAC100000 = 172.16.0.0
        return true;
    }

    // Check for 192.168.0.0/16
    if ((ipInt & 0xFFFF0000) == 0xC0A80000) { // 0xFFFF0000 = 255.255.0.0, 0xC0A80000 = 192.168.0.0
        return true;
    }

    // Not a private IP
    return false;
}

void handleRequest(int clientFD,sockaddr_in clientAddr ){

    char buffer[1024];
    while (true) {
        int n = read(clientFD, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            if (n == 0) {
                std::cout << "Client disconnected\n";
            } else {
                std::cerr <<"Failed to read from socket\n";
            }
            break;
        }

        buffer[n] = '\0';
        std::string clientMessage(buffer);
        std::cout << "Received: " << clientMessage << std::endl;

        if (clientMessage == "DISCOVER_SERVER") {
            std::cout << "Received discovery message from ip address"<<inet_ntoa(clientAddr.sin_addr)<<"\n";
            if (isLocalAddress(clientAddr)) {
                std::string serverResponse = "SERVER_RESPONSE: " + std::to_string(PORT);
                write(clientFD, serverResponse.c_str(), serverResponse.size());
            } else {
                std::cout << "Ignoring request from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
            }
        } else if (clientMessage == "TERMINATE") {
            std::cout << "Terminating server\n";
            close(clientFD);
            return;
        }
    }

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
        /*if(checkLocalAddress(clientAddr.sin_addr)){*/
        /*    std::cout<<"Local address\n";*/
        /*}*/
        /*char buffer[1024];*/
        /*while (true) {*/
        /*    int n = read(client_fd, buffer, sizeof(buffer) - 1);*/
        /*    if (n <= 0) {*/
        /*        if (n == 0) {*/
        /*            std::cout << "Client disconnected\n";*/
        /*        } else {*/
        /*            std::cerr << "Failed to read from socket\n";*/
        /*        }*/
        /*        break;*/
        /*    }*/
        /**/
        /*    buffer[n] = '\0';*/
        /*    std::string clientMessage(buffer);*/
        /*    std::cout << "Received: " << clientMessage << std::endl;*/
        /**/
        /*    if (clientMessage == "DISCOVER_SERVER") {*/
        /*        std::cout << "Received discovery message from ip address"<<inet_ntoa(clientAddr.sin_addr)<<"\n";*/
        /*        if (isLocalAddress(clientAddr)) {*/
        /*            std::string serverResponse = "SERVER_RESPONSE: " + std::to_string(PORT);*/
        /*            write(client_fd, serverResponse.c_str(), serverResponse.size());*/
        /*        } else {*/
        /*            std::cout << "Ignoring request from " << inet_ntoa(clientAddr.sin_addr) << std::endl;*/
        /*        }*/
        /*    } else if (clientMessage == "TERMINATE") {*/
        /*        std::cout << "Terminating server\n";*/
        /*        close(client_fd);*/
        /*        close(server_fd);*/
        /*        return 0;*/
        /*    }*/
        /*}*/
        /**/
        /*close(client_fd);*/
    }

    close(server_fd);
    return 0;
}
