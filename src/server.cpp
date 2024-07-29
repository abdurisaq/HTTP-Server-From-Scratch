#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <thread>
std::string makeLowerCase(std::string input){
  int length = input.length();
  for(int i = 0; i<length; i++){
    input[i] = tolower(input[i]);
  }
  return input;
}
std::string parseRequest(std::string request){
  std::string response;
  std::string substring = request.substr(request.find_first_of("/"));
  substring = substring.substr(0,substring.find_first_of(" "));
  if(substring.substr(1,4)=="echo"){
 
    std::string responseBody = request.substr(request.find_last_of("/")+1);

    std::string headers = request.substr(request.find_first_of("\r\n")+1,request.find_last_of("\r\n"));

    response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: " + std::to_string(substring.substr(6).length()) + "\r\n";
    response += "\r\n" + substring.substr(6);
    std::cout<<"response: "<<response<<"\n";
  }else{
    std::cout<<"lowercase substring: "<<makeLowerCase(substring)<<"\n";
    if(makeLowerCase(substring) == "/user-agent"){
      std::string headers = request.substr(request.find_first_of("\r\n")+1,request.find_last_of("\r\n"));
      std::string userAgent = headers.substr(headers.find("User-Agent: "));
      std::string userAgentHeader = userAgent.substr(0,userAgent.find_first_of  ("\r\n"));
      userAgent = userAgentHeader.substr(userAgentHeader.find_first_of(":")+2);
      response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length:"+std::to_string(userAgent.length())+"\r\n\r\n"+userAgent;
    }
    else if(substring == "/abcdefg"){
      response = "HTTP/1.1 404 Not Found\r\n\r\n";
    }else if (substring == "/"){
      response = "HTTP/1.1 200 OK\r\n\r\n";
    }else{
      response = "HTTP/1.1 404 Not Found\r\n\r\n";
    }
  }  
  return response;
}


int handle_request(int client_fd, sockaddr_in client_address){
  //read request
  std::string request;
  char buffer[1024] = {0};
  int bytesRead = read(client_fd,buffer,1024);
  if(bytesRead ==-1){
    std::cerr<<"Failed to read from client\n";
    return 1;
  }
  request += buffer;
  std::cout << "Request: " << request << "\n";

  std::string response = parseRequest(request);
  std::cout << "final Response: " << response << "\n";
  send(client_fd,response.c_str(),response.length(),0);
}
int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage
  //
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
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
  int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  if (client_fd < 0) {
   std::cerr << "Failed to connect to client\n";
   return 1;
  }
  handle_request(client_fd, client_addr);

    while (1)

    {

        client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);

        std::cout << "Client connected\n";

        std::thread th(handle_request, client_fd, client_addr);

        th.detach();

    }

  close(client_fd);
  
  return 0;
}




  // std::string substring = request.substr(request.find_first_of("/"));
  // substring = substring.substr(0,substring.find_first_of(" "));
  // std::cout << "Client connected\n"<<"request target: "<<substring<<"\n";
  // std::string response;
  
  
  // if(substring.substr(1,4)=="echo"){
 
  //   std::string responseBody = request.substr(request.find_last_of("/")+1);

  //   std::string headers = request.substr(request.find_first_of("\r\n")+1,request.find_last_of("\r\n"));

  //   response = "HTTP/1.1 200 OK\r\n";
  //   response += "Content-Type: text/plain\r\n";
  //   response += "Content-Length: " + std::to_string(substring.substr(6).length()) + "\r\n";
  //   response += "\r\n" + substring.substr(6);
  //   std::cout<<"response: "<<response<<"\n";
  // }else{
  //   std::cout<<"lowercase substring: "<<makeLowerCase(substring)<<"\n";
  //   if(makeLowerCase(substring) == "/user-agent"){
  //     std::string headers = request.substr(request.find_first_of("\r\n")+1,request.find_last_of("\r\n"));
  //     std::string userAgent = headers.substr(headers.find("User-Agent: "));
  //     std::string userAgentHeader = userAgent.substr(0,userAgent.find_first_of  ("\r\n"));
  //     userAgent = userAgentHeader.substr(userAgentHeader.find_first_of(":")+2);
  //     response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length:"+std::to_string(userAgent.length())+"\r\n\r\n"+userAgent;
  //   }
  //   else if(substring == "/abcdefg"){
  //     response = "HTTP/1.1 404 Not Found\r\n\r\n";
  //   }else if (substring == "/"){
  //     response = "HTTP/1.1 200 OK\r\n\r\n";
  //   }else{
  //     response = "HTTP/1.1 404 Not Found\r\n\r\n";
  //   }
  // }  