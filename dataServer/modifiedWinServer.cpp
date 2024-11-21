#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <thread>
#include <bitset>
#include <vector>
#include <condition_variable>
#include <atomic>
#include <deque>
#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#include <winuser.h>
#include <ws2tcpip.h>
#endif

#include "../remoteClient/windows/keylogger.hpp"
#define PORT 50000 
#define PORT1 52000
#define BROADCAST_IP "255.255.255.255"


//global variables now to handle multithreading, will change later

std::condition_variable cv;
std::deque<std::string> queue;
std::mutex queueMutex;
std::mutex countMutex;
std::mutex coutMutex;
std::vector<int>countVec;
std::atomic<int> removedCount(0); 
std::atomic<int> numReceivers(0);
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
char parsePacket(char * buffer, int numBits){
    if(numBits < 1)return ' ';
    uint8_t header = buffer[0];
    int type = (header & (0b11<<6)) >>6;
    char ascii = ' ';
    if(type ==0){
        std::cout<<"keylogging packet"<<std::endl;
        int numKeystrokes = (header & (0b1111<<2))>>2;

        std::cout<<"num keystrokes"<<numKeystrokes<<std::endl;
        int operatingSystem = (header &(0b10)>>1);
        std::cout<<"operating system"<<operatingSystem<<std::endl;
        int currentbitIndex = 0;
        
        std::vector<uint16_t> keystrokes(numKeystrokes);
        for(int i =1; i <= numKeystrokes; i++){

            char currentByte =  buffer[i];
            
            uint16_t keystroke = (((0b1 <<(8-currentbitIndex))-1) & currentByte)<<(1+currentbitIndex);
            keystroke = keystroke | ((buffer[i+1] & (((0b1 <<(1+currentbitIndex))-1)<<(7-currentbitIndex))) >>(7-currentbitIndex));
            keystrokes[i-1] = keystroke;
            currentbitIndex++;

        }
        BYTE keyboardState[256];

        if (!GetKeyboardState(keyboardState)) {
            return ' ';
        }
        WORD asciiValue;

        for(int j = 0; j < numKeystrokes; j++){
            uint16_t keystroke = keystrokes[j];
            BYTE keyCode = keystroke & 0xFF;
            int result = ToAscii(keyCode, MapVirtualKey(keyCode, MAPVK_VK_TO_VSC), keyboardState, &asciiValue, 0);
            ascii = static_cast<char>(asciiValue);
            if(((keystroke & (0b1<<8))>>8) != 0){
                
                std::cout<<"key pressed "<<ascii<<std::endl;
            }else{
                std::cout<<"key released "<<ascii<<std::endl;
            }
        }

    }
    
return ascii;
    
}

void handleRequestNew(SOCKET clientFD, sockaddr_in clientAddr,int clientAccessPosition) {
    // Handle the client request

    char buffer[1024];

    int n = recv(clientFD, buffer, sizeof(buffer) - 1,0);
    if (n <= 0) {
        if (n == 0) {
            std::cout << "Client disconnected\n";
        } else {
            std::cerr <<"Failed to read from socket\n";
        }
        return;
    }

    buffer[n] = '\0';
    std::string clientMessage(buffer);
    if (clientMessage == "DISCOVER_SERVER") {
        char ipBuffer[INET_ADDRSTRLEN];
        std::cout << "Received discovery message from ip address: ";
        if (inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuffer, sizeof(ipBuffer)) != NULL) {
            std::cout << ipBuffer << "\n";
        } else {
            std::cout << "Error converting IP address\n";
        }
        if (checkLocalAddress(clientAddr)) {

            std::string serverResponse = "SERVER_RESPONSE: ";//  + std::to_string(PORT) + std::to_string(clientAccessPosition);
            if(clientAccessPosition < 2){
                std::cout<<"first connection found, setting them to be broadcaster"<<std::endl;
                serverResponse += "broadcaster";

                int messageSize = static_cast<int>(serverResponse.size());    
                send(clientFD, serverResponse.c_str(),messageSize, 0);
                while(true){
                    int n = recv(clientFD, buffer, sizeof(buffer) - 1,0);
                    if (n <= 0) {
                        if (n == 0) {
                            std::cout << "Client disconnected\n";
                        } else {
                            std::cerr <<"Failed to read from socket\n";
                        }
                        return;
                    }
                    buffer[n] = '\0';
                    {
                    std::unique_lock<std::mutex>queueLock(queueMutex);
                    queue.push_back(std::string(buffer));
                    countVec.push_back(0);
                    }
                    cv.notify_all();

                }

            }else{
                serverResponse += "receiver";
                std::cout<<"setting connection to be receiver"<<std::endl;

                int messageSize = static_cast<int>(serverResponse.size());    
                send(clientFD, serverResponse.c_str(),messageSize, 0);
                numReceivers++;
                int i =0;
                while(true){
                    std::unique_lock<std::mutex> lock(queueMutex);
                    if(queue.empty()||(i-removedCount)>=queue.size()){
                            {
                                std::lock_guard<std::mutex> lock(coutMutex);
                                std::cout<<"worker id: "<<clientAccessPosition<<" no items in queue, sleeping.. "<<std::endl;
                            }
                        cv.wait(lock,[i]{return (!queue.empty()&&((i-removedCount)<queue.size()));});  
                        continue;
                    }
                    std::string packet = queue[i-removedCount];
                    lock.unlock();
                    int messageSize = static_cast<int>(packet.size());

                    {
                        std::lock_guard<std::mutex> lock(coutMutex);
                        std::cout<<"sending packet to client "<<std::endl;
                        std::cout<<packet<<std::endl;
                    }
                    
                    send(clientFD,packet.c_str(),messageSize,0);
                    {
                        std::unique_lock<std::mutex> count_lock(countMutex);
                        countVec[i-removedCount]++;
                        if(countVec.front() == numReceivers){
                            std::unique_lock<std::mutex> queue_lock(queueMutex);
                            // {
                            //     std::lock_guard<std::mutex> cout_lock(coutMutex);
                            //     std::cout<<"worker id: "<<clientAccessPosition<<" is resetting count"<<std::endl;
                            // }
                            queue.pop_front();
                            countVec.erase(countVec.begin());  // Reset the counter for the next 
                            removedCount++;
                            queue_lock.unlock();
                        }
                    }
                    i++;
                }
            }
            // write(clientFD, serverResponse.c_str(), serverResponse.size());
        } else {

            char ipBuffer[INET_ADDRSTRLEN];
            std::cout << "Ignoring request from ";
            if (inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuffer, sizeof(ipBuffer)) != NULL) {
                std::cout << ipBuffer << "\n";
            } else {
                std::cout << "Error converting IP address\n";
            }
        }
    } else if (clientMessage == "TERMINATE") {
        std::cout << "Terminating server\n";
        closesocket(clientFD);
        return;
    }

}


void handleRequest(SOCKET clientFD, sockaddr_in clientAddr,int clientAccessPosition) {
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
        char ascii = parsePacket(buffer,n);
        std::cout<<"ascii parsed: " << ascii<<std::endl;
        if(ascii =='s'){
            std::string serverResponse = "SERVER_RESPONSE: receiver";
            int messageSize = static_cast<int>(serverResponse.size());    
            send(clientFD, serverResponse.c_str(),messageSize, 0);
            continue;
        }
        std::cout << "Received: " << clientMessage << std::endl;
        std::cout << "Binary form of the message:\n";
        for (int i = 0; i <n; ++i) {
            // Convert each byte into its binary form and print
            std::bitset<8> binary(buffer[i]);
            std::cout << binary << " ";  
        }
        std::cout << std::endl;
        if (clientMessage == "DISCOVER_SERVER") {

            char ipBuffer[INET_ADDRSTRLEN];
            std::cout << "Received discovery message from ip address";
            if (inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuffer, sizeof(ipBuffer)) != NULL) {
                std::cout << ipBuffer << "\n";
            } else {
                std::cout << "Error converting IP address\n";
            }
            if (checkLocalAddress(clientAddr)) {

                std::string serverResponse = "SERVER_RESPONSE: ";//  + std::to_string(PORT) + std::to_string(clientAccessPosition);
                if(clientAccessPosition < 2){
                    std::cout<<"first connection found, setting them to be broadcaster"<<std::endl;
                    serverResponse += "broadcaster";
                }else{
                    serverResponse += "receiver";
                    std::cout<<"setting connection to be receiver"<<std::endl;
                }
                int messageSize = static_cast<int>(serverResponse.size());    
                send(clientFD, serverResponse.c_str(),messageSize, 0);
                // write(clientFD, serverResponse.c_str(), serverResponse.size());
            } else {

                char ipBuffer[INET_ADDRSTRLEN];
                std::cout << "Ignoring request from ";
                if (inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuffer, sizeof(ipBuffer)) != NULL) {
                    std::cout << ipBuffer << "\n";
                } else {
                    std::cout << "Error converting IP address\n";
                }
            }
        } else if (clientMessage == "TERMINATE") {
            std::cout << "Terminating server\n";
            closesocket(clientFD);
            return;
        }
    }
}

void handleUDPRequests(SOCKET udp_fd) {
    char buffer[1024];
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    while (true) {
        int n = recvfrom(udp_fd, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&clientAddr, &clientAddrLen);
        if (n <= 0) {
            if (n == 0) {
                std::cout << "Client disconnected\n";
            } else {
                std::cerr << "Failed to read from UDP socket\n";
            }
            continue;
        }

        buffer[n] = '\0';
        std::string clientMessage(buffer);
        std::cout << "Received UDP message: " << clientMessage << std::endl;

        // You can handle the received message here, for example:
        if (clientMessage == "DISCOVER_SERVER") {
            std::string response = "UDP_SERVER_RESPONSE: Here is the UDP server response";
            sendto(udp_fd, response.c_str(), response.size(), 0, (sockaddr*)&clientAddr, clientAddrLen);
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

    SOCKET udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd == INVALID_SOCKET) {
        std::cerr << "Failed to create UDP socket: " << WSAGetLastError() << "\n";
        return 1;
    }

    sockaddr_in udp_addr;
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
    udp_addr.sin_port = htons(PORT1);

    if (bind(udp_fd, (sockaddr*)&udp_addr, sizeof(udp_addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind UDP socket: " << WSAGetLastError() << "\n";
        closesocket(udp_fd);
        WSACleanup();
        return 1;
    }

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind to port: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::thread udpThread(handleUDPRequests, udp_fd);

    if (listen(server_fd, 5) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on socket: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "Listening on port " << PORT << std::endl;
    int pos = 0;
    while (true) {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET client_fd = accept(server_fd, (sockaddr*)&clientAddr, &clientAddrLen);
        if (client_fd == INVALID_SOCKET) {
            std::cerr << "Failed to accept connection: " << WSAGetLastError() << "\n";
            continue;
        }


        std::cout << "Got a connection" << std::endl;
        std::cout<<pos<<std::endl;
        std::thread th(handleRequestNew, client_fd, clientAddr,pos);
        th.detach();
        pos++;
    }

    closesocket(server_fd);
    WSACleanup();
    return 1;
}

