#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <thread>
#include <vector>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#define PORT 50000 
#define BROADCAST_IP "255.255.255.255"

void startup() {
    
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        std::cerr << "Failed to initialize Winsock\n";
        exit(1);
    }
}

bool checkLocalAddress(sockaddr_in inAddr) {

    // Convert the in_addr structure to an integer
    uint32_t ipInt = ntohl(inAddr.sin_addr.s_addr);

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
void handleRequest(SOCKET clientFD, sockaddr_in clientAddr) {
    // Handle the client request

    char buffer[1024];
    while (true) {
        int n = recv(clientFD, buffer, sizeof(buffer) - 1,0);
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
            if (checkLocalAddress(clientAddr)) {
                std::string serverResponse = "SERVER_RESPONSE: " + std::to_string(PORT);
                int messageSize = serverResponse.size();    
                send(clientFD, serverResponse.c_str(),messageSize, 0);
                // write(clientFD, serverResponse.c_str(), serverResponse.size());
            } else {
                std::cout << "Ignoring request from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
            }
        } else if (clientMessage == "TERMINATE") {
            std::cout << "Terminating server\n";
            closesocket(clientFD);
            return;
        }
    }
}

int main(int argc, char **argv) {
    startup();

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Failed to create server socket: " << WSAGetLastError() << "\n";
        return 1;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == SOCKET_ERROR) {
        std::cerr << "setsockopt failed: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY to bind to all interfaces
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind to port: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, 5) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on socket: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "Listening on port " << PORT << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET client_fd = accept(server_fd, (sockaddr*)&clientAddr, &clientAddrLen);
        if (client_fd == INVALID_SOCKET) {
            std::cerr << "Failed to accept connection: " << WSAGetLastError() << "\n";
            continue;
        }

        std::cout << "Got a connection" << std::endl;
        std::thread th(handleRequest, client_fd, clientAddr);
        th.detach();
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}

