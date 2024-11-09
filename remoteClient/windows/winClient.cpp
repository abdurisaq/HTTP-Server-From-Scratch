#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <thread>
#include <vector>
#include <atomic>

#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#include <winuser.h>
#include <ws2tcpip.h>
#endif
#include "keylogger.hpp"
#define PORT 50000
#define BROADCAST_IP "255.255.255.255"


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

void startup(){
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        std::cerr << "Failed to initialize Winsock\n";
        exit(1);
    }

}


void runScript(const std::string &scriptPath) {
    std::string command = scriptPath;

    // Execute the command
    int result = system(command.c_str());

    // Check the result of the command
    if (result != 0) {
        throw std::runtime_error("Command failed with exit status: " + std::to_string(result));
    }
}

ULONG scanForServer(){
    struct sockaddr_in broadcastAddr, recvAddr;
    socklen_t addrLen = sizeof(recvAddr);
    runScript("pwsh -ExecutionPolicy Bypass -File .\\scripts\\scanLAN.ps1");
    SOCKET clientFD = socket(AF_INET, SOCK_STREAM, 0);
    if (clientFD < 0) {
        std::cerr << "Failed to create server socket\n";
        exit(1);
    }

    char buffer[1024];
    std::vector<std::string> output;

    FILE*  file = fopen("./ipaddress.txt","r");
    if(file == NULL){
        std::cerr << "Failed to open file\n";
        exit(1);
    }
    int count = 0;
    for(count = 0; ;count++){
        if(fgets(buffer,1024,file) ==NULL){
            break;
        }
        output.push_back(buffer);
    }
    fclose(file);

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int broadcast = 1;
    if (setsockopt(clientFD, SOL_SOCKET, SO_REUSEADDR, (const char*)&broadcast, sizeof(broadcast)) < 0) {
        std::cerr << "setsockopt failed\n";
        exit(1);
    }
    for(int i = 0 ; i < count ; i ++){
        broadcastAddr.sin_family = AF_INET;
        broadcastAddr.sin_addr.s_addr = inet_addr(output[i].c_str());
        broadcastAddr.sin_port = htons(PORT);
        std::cout<<"Sending discovery message to ip address "<<inet_ntoa(broadcastAddr.sin_addr)<<"\n";
        if(connect(clientFD, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) >= 0){
            std::cout<<"Connected to server at ip address"<<inet_ntoa(broadcastAddr.sin_addr)<<"\n";
            break;

        } else{
            std::cerr << "Failed to connect to server\n";
        }
    }
    return broadcastAddr.sin_addr.s_addr;

}

std::atomic<bool> running(false);
int main(int argc, char **argv) {
    startup();
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    struct sockaddr_in broadcastAddr, recvAddr;
    int addrLen = sizeof(recvAddr);

    SOCKET clientFD = socket(AF_INET, SOCK_STREAM, 0);
    if (clientFD < 0) {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }
    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int broadcast = 1;

    ULONG serverAddr =  scanForServer();
    // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == SOCKET_ERROR) {
    if (setsockopt(clientFD, SOL_SOCKET, SO_REUSEADDR, (const char*)&broadcast, sizeof(broadcast)) == SOCKET_ERROR) {
        std::cerr << "setsockopt failed\n";
        closesocket(clientFD);
        WSACleanup();
        return 1;
    }

    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_addr.s_addr = serverAddr;
    broadcastAddr.sin_port = htons(PORT);


    std::cout<<"Sending discovery message to ip address "<<inet_ntoa(broadcastAddr.sin_addr)<<"\n";

    if(connect(clientFD, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) < 0){
        std::cerr << "Failed to connect to server\n";
        closesocket(clientFD);
        WSACleanup();
        return 1;
    } 
    
    std::string discoveryMessage = "DISCOVER_SERVER";
    int messageSize = discoveryMessage.size();
    send(clientFD, discoveryMessage.c_str(),messageSize, 0);
    char buffer[1024];
    int n = recvfrom(clientFD, buffer, sizeof(buffer), 0, (struct sockaddr *)&recvAddr, &addrLen);
    buffer[n] = '\0';
    std::cout << "Received: " << buffer << "\n";
    std::cout << "Discovered server: " << inet_ntoa(recvAddr.sin_addr) << std::endl;
    std::cout<<"connected, please type and then click enter to send messages to the server"<<std::endl;
    std::cout<<"type \"terminate\" to kill the connection"<<std::endl;
    const char* expectedPrefix = "SERVER_RESPONSE: ";
    size_t prefixLength = strlen(expectedPrefix);
    if (strncmp(buffer, expectedPrefix, prefixLength) != 0) {
        std::cout << "did not get the expected response, closing connection" << std::endl;
        std::cout << "response received: " << buffer << std::endl;
    } else{
        // std::atomic<bool> running(true);
        // 
        std::thread keylogger_thread;

        std::string afterPrefix(buffer + prefixLength);
        afterPrefix.erase(0, afterPrefix.find_first_not_of(" \n\r\t")); // Trim

        if (afterPrefix == "broadcaster") {
            if (!running) {
                running = true;  // Set running to true and start the keylogger
                keylogger_thread = std::thread(startLogging, clientFD, std::ref(running));
                std::cout << "Now in broadcaster mode. Keylogger started.\n";
            }

        } else if (afterPrefix == "receiver") {
            if (running) {
                running = false;  // Set running to false to stop the keylogger
                if (keylogger_thread.joinable()) {
                    keylogger_thread.join();  // Join the thread to ensure it stops
                }
                std::cout << "Now in receiver mode. Keylogger stopped.\n";
            }
        }
        // Simulate the client doing other work
        while(true){
            int n = recvfrom(clientFD, buffer, sizeof(buffer), 0, (struct sockaddr *)&recvAddr, &addrLen);
            if (n < 0) {
                std::cerr << "recvfrom failed, closing connection.\n";
                break;  // Exit the loop or handle the error
            }
            buffer[n] = '\0';
            // When ready to stop the keylogger
            const char* terminateMessage = "TERMINATE";
            if(strncmp(buffer,terminateMessage,strlen(terminateMessage)) ==0){
                running = false;

                if (keylogger_thread.joinable()) {
                    keylogger_thread.join();
                }
                break;
            }

            std::cout<<"received packet"<<std::endl;
            
            std::string message(buffer);
            std::cout<<message<<std::endl;
            if (strncmp(buffer, expectedPrefix, prefixLength) == 0) {

                std::cout<<"prefix is correct"<<std::endl;
                std::string afterPrefix(buffer + prefixLength);
                afterPrefix.erase(0, afterPrefix.find_first_not_of(" \n\r\t")); // Trim

                if (afterPrefix == "broadcaster") {
                    if (!running) {
                        running = true;  // Set running to true and start the keylogger
                        keylogger_thread = std::thread(startLogging, clientFD, std::ref(running));
                        std::cout << "Now in broadcaster mode. Keylogger started.\n";
                    }
                } else if (afterPrefix == "receiver") {
                    if (running) {
                        running = false;  // Set running to false to stop the keylogger
                        if (keylogger_thread.joinable()) {
                            keylogger_thread.join();  // Join the thread to ensure it stops
                        }

                        std::cout << "Now in receiver mode. Keylogger stopped.\n";
                    }
                }
            }else{
                
                if(!running){

                    char ascii = parsePacket(buffer,n);
                    std::cout<<"ascii parsed: " << ascii<<std::endl;
                }else{
                    std::cout<<"not a server message but still running"<<std::endl;
                }


            }



        }

    }
    closesocket(clientFD);
    WSACleanup();
    return 0;
}
