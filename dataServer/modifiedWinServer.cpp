#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <thread>
#include <bitset>
#include <vector>
#include <cmath>
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

struct BroadcastState{
std::condition_variable cv;
std::condition_variable cv2;
std::deque<std::string> queue;
std::mutex queueMutex;
std::mutex countMutex;
std::mutex coutMutex;
std::vector<int>countVec;
std::atomic<int> removedCount = 0; 
std::atomic<int> numReceivers = 0;

};
struct ClientInfo {
    SOCKET clientFD;
    sockaddr_in clientAddr;
};
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

bool checkValidReturn(int n){

    if (n <= 0) {
        if (n == 0) {
            std::cout << "Client disconnected\n";
        } else {
            std::cerr <<"Failed to read from socket\n";
        }
        return false;
    }
    return true;

}
void handleRequestNew(SOCKET clientFD, sockaddr_in clientAddr,int clientAccessPosition,BroadcastState * global) {
    // Handle the client request

    char buffer[1024];

    int n = recv(clientFD, buffer, sizeof(buffer) - 1,0);
    if(!checkValidReturn(n))return;

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
            if(clientAccessPosition <1){
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
                    std::unique_lock<std::mutex>queueLock(global->queueMutex);

                    if(global->numReceivers==0){
                        {
                                std::lock_guard<std::mutex> lock(global->coutMutex);
                                std::cout<<"no receivers, sleeping "<<std::endl;
                        }
                        global->cv2.wait(queueLock,[global]{return global->numReceivers>0;});  
                        continue;
                    }
                    std::cout<<"adding packet to queue"<<std::endl;
                    global->queue.push_back(std::string(buffer));
                    global->countVec.push_back(0);
                    }
                    global->cv.notify_all();

                }

            }else{
                serverResponse += "receiver";
                std::cout<<"setting connection to be receiver"<<std::endl;

                int messageSize = static_cast<int>(serverResponse.size());    
                send(clientFD, serverResponse.c_str(),messageSize, 0);
                global->numReceivers++;
                global->cv2.notify_all();
                int i =global->removedCount;
                while(true){
                    std::unique_lock<std::mutex> lock(global->queueMutex);
                    if(global->queue.empty()||(i-global->removedCount)>=global->queue.size()){
                            {
                                std::lock_guard<std::mutex> lock(global->coutMutex);
                                std::cout<<"worker id: "<<clientAccessPosition<<" no items in queue, sleeping.. "<<std::endl;
                                std::cout<<"i: "<<i<<"removedCount: "<<global->removedCount<<"queue size: "<<global->queue.size()<<std::endl;
                        }


                        global->cv.wait(lock,[i,global]{return (!global->queue.empty()&&((i-global->removedCount)<global->queue.size()));});  
                        continue;
                    }
                    std::string packet = global->queue[i-global->removedCount];
                    lock.unlock();
                    int messageSize = static_cast<int>(packet.size());

                    {
                        std::lock_guard<std::mutex> lock(global->coutMutex);
                        std::cout<<"sending packet to client "<<std::endl;
                        std::cout<<packet<<std::endl;
                    }

                    int result = send(clientFD,packet.c_str(),messageSize,0);
                    if(result <=0){
                        std::cout<<"disconnecting this thread"<<std::endl;
                        std::lock_guard<std::mutex> count_lock(global->countMutex);
                        global->numReceivers--;
                        for (size_t i = 0; i < global->countVec.size(); i++) {
                            if (global->countVec[i] > global->numReceivers) {
                                global->countVec[i] = global->numReceivers;
                            }
                        }
                        return;
                    }
                    {
                        std::unique_lock<std::mutex> count_lock(global->countMutex);
                        global->countVec[i-global->removedCount]++;
                        if(global->countVec.front() == global->numReceivers){
                            std::unique_lock<std::mutex> queue_lock(global->queueMutex);
                            // {
                            //     std::lock_guard<std::mutex> cout_lock(coutMutex);
                            //     std::cout<<"worker id: "<<clientAccessPosition<<" is resetting count"<<std::endl;
                            // }
                            global->queue.pop_front();
                            global->countVec.erase(global->countVec.begin());  // Reset the counter for the next 
                            global->removedCount++;
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
int findExistingClientID(const sockaddr_in& addr, std::unordered_map<int,ClientInfo> map) {
    for (const auto& [id, clientInfo] : map) {
        const sockaddr_in& storedAddr = clientInfo.address;
        if (addr.sin_family == storedAddr.sin_family &&
            addr.sin_port == storedAddr.sin_port &&
            addr.sin_addr.s_addr == storedAddr.sin_addr.s_addr) {
            return id;  
        }
    }
    return -1;  
}


void handleUDPRequests(SOCKET udp_fd, std::atomic<int> clientId, std::unordered_map<int,ClientInfo> map ) {
    char buffer[1024];
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    while (true) {
        int n = recvfrom(udp_fd, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&clientAddr, &clientAddrLen);
        
        if(!checkValidReturn(n))continue;

        buffer[n] = '\0';
        std::string clientMessage(buffer);
        if( clientMessage.size() == 0 ){
            std::cout<<"message is empty"<<std::endl;
            continue;
        }
        std::cout << "Received UDP message: " << clientMessage << std::endl;

         std::bitset<8> bits(clientMessage.back());
        std::cout << "8-bit representation: " << bits << "\n";
        if((clientMessage.back() >>6) == 1){
            std::cout<<"this is a mouse movement packet"<<std::endl;

        }
        int id = findExistingClientID(clientAddr,map);
        if(id == -1){
            id = clientId.fetch_add(1);
        }
        if (clientMessage == "DISCOVER_SERVER") {
            std::string response = "UDP_SERVER_RESPONSE:ID-" + std::to_string(id) + "- Here is the UDP server response";
            map[id] = {udp_fd,clientAddr};
            sendto(udp_fd, response.c_str(), response.size(), 0, (sockaddr*)&clientAddr, clientAddrLen);
        }else if(){//handle if first bits indicate its sending a mouse movement over udp, then broadcast it to all in the mappin gbut the sender
            //probably have dedicated thread to read through packets sent to a thread safe queue, and sleep afterwards. copy the same architect of tcp thread safe queue, but just one  thread for now instead of each connection having a thread because udp is connectionless. just goonna loop over map and send out packet.

        }
    }
}

int main(int argc, char **argv) {
    startup();


    BroadcastState* global = new BroadcastState;
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


    std::atomic<int> clientCounter = 0;
    std::unordered_map<int, ClientInfo> clientMap;
    std::thread udpThread(handleUDPRequests, udp_fd,clientCounter, clientMap);

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
        std::thread th(handleRequestNew, client_fd, clientAddr,pos,global);
        th.detach();
        pos++;
    }

    closesocket(server_fd);
    WSACleanup();
    return 1;
}

