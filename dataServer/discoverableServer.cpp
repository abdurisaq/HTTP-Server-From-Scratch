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

        std::cout << "Got a connection from " << inet_ntoa(clientAddr.sin_addr) << std::endl;

        char buffer[1024];
        while (true) {
            int n = read(client_fd, buffer, sizeof(buffer) - 1);
            if (n <= 0) {
                if (n == 0) {
                    std::cout << "Client disconnected\n";
                } else {
                    std::cerr << "Failed to read from socket\n";
                }
                break;
            }

            buffer[n] = '\0';
            std::string clientMessage(buffer);
            std::cout << "Received: " << clientMessage << std::endl;

            if (clientMessage == "DISCOVER_SERVER") {
                if (isLocalAddress(clientAddr)) {
                    std::string serverResponse = "SERVER_RESPONSE: " + std::to_string(PORT);
                    write(client_fd, serverResponse.c_str(), serverResponse.size());
                } else {
                    std::cout << "Ignoring request from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
                }
            } else if (clientMessage == "TERMINATE") {
                std::cout << "Terminating server\n";
                close(client_fd);
                close(server_fd);
                return 0;
            }
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}