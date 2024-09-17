#include <windows.h>
#include <winuser.h>
#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include <vector>
#include <array>
#include <unordered_set>

#define ARRAY_SIZE 32

void updateKeyState(uint8_t vkCode, bool pressed, std::array<uint8_t, ARRAY_SIZE>& keyStates) {
    if (vkCode < 256) {    
        // std::cout<<"Key code in range"<<static_cast<int>(vkCode)<<std::endl;
        size_t byteIndex = vkCode / 8;        
        size_t bitIndex = vkCode % 8; 
        if (pressed) {
            keyStates[byteIndex] |= (1 << bitIndex);        
        } else {
            keyStates[byteIndex] &= ~(1 << bitIndex);
        }
    } else {
        std::cout << "Virtual key code out of range" << std::endl;
        
    }
}

bool isBitSet(const std::array<uint8_t, ARRAY_SIZE>& keyStates, uint8_t vkCode) {
    if (vkCode >= 256) {
        return false; // Out of range
    }

    size_t byteIndex = vkCode / 8;
    size_t bitIndex = vkCode % 8;

    return (keyStates[byteIndex] & (1 << bitIndex)) != 0;
}

std::vector<std::string> findChanges(const std::vector<BYTE>& currentKeys,
                                     const std::vector<BYTE>& pastKeys,
                                      std::array<uint8_t, ARRAY_SIZE>& bitmask) {
    std::unordered_set<BYTE> currentKeySet(currentKeys.begin(), currentKeys.end());
    std::unordered_set<BYTE> pastKeySet(pastKeys.begin(), pastKeys.end());

    std::vector<std::string> result;

    for (BYTE key : pastKeySet) {
        if (currentKeySet.find(key) == currentKeySet.end() && isBitSet(bitmask, key)) {
            // Key was in past but not pressed now, meaning it was released
            result.push_back("Key " + std::to_string(key) + " was released");
            updateKeyState(key, false, bitmask);
        }
    }

    return result;
}

void startLogging() {
    std::vector<BYTE> currentKeys; 
    std::vector<BYTE> pastKeys; 
    std::array<uint8_t, ARRAY_SIZE> keyStates = { 0 };
    std::vector<std::string> output;

    while (true) {    
        currentKeys.clear(); 
        for (BYTE vkCode = 0; vkCode < 255; ++vkCode) {

            if (GetAsyncKeyState(vkCode) & 0x1) { // Check if the key is pressed
                currentKeys.push_back(vkCode);
                if (!isBitSet(keyStates, vkCode)) {
                    updateKeyState(vkCode, true, keyStates);
                    output.push_back("Key " + std::to_string(vkCode) + " was pressed");
                    // std::cout<<output.size()<<std::endl;
                }
            }
        }
        // std::cout<<"keys being held right now"<<std::endl;
        
        // for (BYTE key : currentKeys) {
        //     std::cout<<key;
        //
        // }
        // std::cout<<std::endl;
        std::vector<std::string> changes = findChanges(currentKeys, pastKeys, keyStates);
        output.insert(output.end(), changes.begin(), changes.end()); 

        // Print all outputs
        for (const std::string& message : output) {
            std::cout << message << std::endl;
        }
        output.clear();
        pastKeys = currentKeys;  

        Sleep(100); // Sleep to reduce CPU usage
    }
}
int main(){
    // std::string scriptPath =".\\scripts\\testOutput.ps1"; 
    // testpowershell(scriptPath);
    // printf("hello world changed");
    printf("starting test");
    std::cout<<"test from std::cout"<<std::endl;
    startLogging();
}

