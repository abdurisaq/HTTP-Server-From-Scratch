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
#include <zlib.h>
#include <vector>
std::string makeLowerCase(std::string input){
  int length = input.length();
  for(int i = 0; i<length; i++){
    input[i] = tolower(input[i]);
  }
  return input;
}
std::string parseRequest(std::string request,std::string directory){
  std::string response;
  std::string requestType = request.substr(0,request.find_first_of(" "));
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
  }else if(substring.substr(1,5)=="files"){
    FILE *file;
    std::string filename = directory + substring.substr(7);
    
    std::cout<<"request type \'"<<requestType<<"\'\n";
    if(requestType == "GET"){
      file = fopen(filename.c_str(),"r");
    }else if (requestType =="POST"){
      std::cout<<"creating file\n";
      file = fopen(filename.c_str(),"w+");
    }
    
    char buffer[1024];
    std::string output;
    std::cout<<"filename: "<<filename<<"\n";
    if(file == NULL){
      
      std::cout<<"file doesn't exist\n";
      response = "HTTP/1.1 404 Not Found\r\n\r\n";

    }else{
      std::cout<<"file exists\n";
      if(requestType =="GET"){
        while(fgets(buffer,1024,file) !=NULL){
        output += buffer;
      }
      
      response = "HTTP/1.1 200 OK\r\n";
      response += "Content-Type: application/octet-stream\r\n";
      response += "Content-Length: " + std::to_string(output.length()) + "\r\n";
      response += "\r\n" + output;
      std::cout<<"response: "<<response<<"\n";
      }else if(requestType =="POST"){
        std::string body = request.substr(request.find_last_of("\r\n")+1);
        std::cout<<"body: "<<body<<"\n";
        fwrite(body.c_str(),sizeof(body[0]),body.length(),file);
        response = "HTTP/1.1 201 Created\r\n\r\n";
      }
      
      fclose(file);
    }
    
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

void compressBody(std::string & response){

  int contentLengthIndex = response.find("Content-Length: ");
  int headerEndIndex = response.find("\r\n\r\n",contentLengthIndex)+4;
  std::string body = response.substr(headerEndIndex);
  std::cout<<"body: "<<body<<"\n";
  z_stream stream;
  memset(&stream, 0, sizeof(stream));

  // Initialize compression
  if (deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
      throw std::runtime_error("Failed to initialize zlib compression");
  }

  // Set input data
  stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(body.data()));
  stream.avail_in = static_cast<uInt>(body.size());

  // Allocate output buffer
  std::vector<Bytef> buffer(1024);
  std::string compressedData;

  // Compression loop
  while (stream.avail_in > 0 || stream.avail_out == 0) {
      stream.next_out = buffer.data();
      stream.avail_out = static_cast<uInt>(buffer.size());

      int ret = deflate(&stream, Z_FINISH);
      if (ret == Z_STREAM_ERROR) {
          deflateEnd(&stream);
          throw std::runtime_error("zlib compression error");
      }

      compressedData.append(reinterpret_cast<char*>(buffer.data()), buffer.size() - stream.avail_out);

      // Check if more data is needed
      if (stream.avail_out == 0 && stream.avail_in > 0) {
          // Buffer was full, need to process more data
          buffer.resize(buffer.size() * 2); // Double buffer size if needed
      }
  }

  // Clean up
  deflateEnd(&stream);

  //compressedData
  response.erase(headerEndIndex);
  std::cout<<"compressed data: "<<compressedData<<"\n";
  std::cout<<"response without uncompressed body: "<<response<<"\n";
  response += "\r\n"+compressedData;
  std::cout<<"response with compressed body: "<<response<<"\n";
  
}


void encodeRequest( std::string  request, std::string& response){
  std::cout<<"request in encoding function: "<<request<<"\n";
  int encodingLine = request.find("Accept-Encoding: ");
  std::cout<<encodingLine<<"\n";
  if(encodingLine ==-1){
    //just return, no compression needed
    return;
  }
  std::string encoding = request.substr(encodingLine,request.find("\r\n",encodingLine)-encodingLine)+"\r\n";
  std::string target ="Content-Type";
  std::string encodings = encoding.substr(encoding.find_first_of(" ")+1,encoding.find_last_not_of("\r\n") - encoding.find_first_of(" "));
  int chosenEncodingIndex = encodings.find("gzip");
  if(chosenEncodingIndex == -1){
    return;
  }
  int endOfEncoding = encodings.find_first_of(",",chosenEncodingIndex);
   std::string chosenEncoding;
  if(endOfEncoding != -1){
    chosenEncoding = encodings.substr(chosenEncodingIndex,endOfEncoding-chosenEncodingIndex);
  }else{
    chosenEncoding = encodings.substr(chosenEncodingIndex,encoding.find_last_not_of("\r\n")-chosenEncodingIndex);
  }
  std::cout<<"chosen encoding: "<<chosenEncoding<<"\n";
  std::string contentEncoding = "Content-Encoding: "+chosenEncoding+"\r\n";

  int inclusionPoint = response.find(target);
  std::cout<<"inclusion point: "<<inclusionPoint<<"\n";
  response.insert(inclusionPoint,contentEncoding);
  compressBody(response);
  
}

int handleRequest(int client_fd,std::string directory){
  //read request
  std::string request;
  char buffer[1024] = {0};
  int bytesRead = read(client_fd,buffer,1024);
  if(bytesRead ==-1){
    std::cerr<<"Failed to read from client\n";
    return 0;
  }
  request += buffer;
  std::cout << "Request: " << request << "\n";

  std::string response = parseRequest(request,directory);
  encodeRequest(request,response);
  std::cout << "final Response: \n" << response << "\n";
  int bytesSent = send(client_fd,response.c_str(),response.length(),0);
  if(bytesSent == -1){
    std::cerr<<"Failed to send response to client\n";
    return 0;
  }
  close(client_fd);
  return bytesSent;
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

    while (1)

    {
        std::cout << "Waiting for a client to connect...\n";
        int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
        if (client_fd < 0) {
          std::cerr << "Failed to connect to client\n";
          return 1;
        }
        std::cout << "Client connected\n";

        std::thread th(handleRequest, client_fd,directory);

        th.detach();

    }

  close(server_fd);
  
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