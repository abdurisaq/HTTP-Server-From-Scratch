#include <windows.h>
#include <winuser.h>
#include <iostream>
#include <string>


void testpowershell(std::string path){
    std::string command = "powershell -ExecutionPolicy Bypass -File \"" + path + "\"";
    system(command.c_str());
}
int main(){
  
  testpowershell(".\\scripts\\testOutput.ps1");
  printf("hello world changed");
}

