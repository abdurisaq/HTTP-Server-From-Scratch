#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sys/types.h>
// #include <thread>
#include <vector>
#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#include <winuser.h>
#include <ws2tcpip.h>
#endif
#define PORT 50000
#define BROADCAST_IP "255.255.255.255"
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
    const char* expectedPrefix = "SERVER_RESPONSE";
    if (strncmp(buffer, expectedPrefix, strlen(expectedPrefix)) != 0) {
        std::cout << "did not get the expected response, closing connection" << std::endl;
        std::cout << "response received: " << buffer << std::endl;
    } else{

        while(true){
            std::string input;
            std::getline(std::cin,input);
            if(input=="terminate")break;

            send(clientFD,input.c_str(),input.size(),0);
        }
    }
    closesocket(clientFD);
    WSACleanup();
    return 0;
}
