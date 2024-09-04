#include <iostream>
#include <winsock2.h>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sys/types.h>
// #include <thread>
#include <vector>
#define PORT 10000
#include <Windows.h>
#include <winuser.h>
// #elif __linux__
// #include <unistd.h>
// #include <X11/Xlib.h>  // Include Linux-specific headers
// #include <X11/keysym.h>
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <netdb.h>
#define BROADCAST_IP "255.255.255.255"
void startup(){
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2,2),&wsadata);

}


int main(int argc, char **argv) {
    startup();
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    struct sockaddr_in broadcastAddr, recvAddr;
    socklen_t addrLen = sizeof(recvAddr);

    int clientFD = socket(AF_INET, SOCK_STREAM, 0);winC
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
    broadcastAddr.sin_addr.s_addr = 0;
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
    close(clientFD);
    return 0;
}
