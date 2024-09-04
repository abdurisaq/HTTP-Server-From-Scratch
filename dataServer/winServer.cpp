#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#define PORT 50000 
#define BROADCAST_IP "255.255.255.255"

void startup() {
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        std::cerr << "Failed to initialize Winsock\n";
        exit(1);
    }
}

void handleRequest(SOCKET clientFD, sockaddr_in clientAddr) {
    // Handle the client request
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

