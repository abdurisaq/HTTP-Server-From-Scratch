#include <windows.h>
#include <winuser.h>
#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include <vector>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <chrono>


#define ARRAY_SIZE 32
#define DEBOUNCE_DURATION 50
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
                                      std::array<uint8_t, ARRAY_SIZE>& bitmask,
                                     std::unordered_map<BYTE, std::chrono::steady_clock::time_point>& keyPressTimes) {
    std::unordered_set<BYTE> currentKeySet(currentKeys.begin(), currentKeys.end());
    std::unordered_set<BYTE> pastKeySet(pastKeys.begin(), pastKeys.end());
    std::vector<std::string> result;

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    
    for (BYTE key : pastKeySet) {
        if (currentKeySet.find(key) == currentKeySet.end() && isBitSet(bitmask, key)) {
            // // Key was in past but not pressed now, meaning it was released
            // result.push_back("Key " + std::to_string(key) + " was released");
            // updateKeyState(key, false, bitmask);
            std::unordered_map<BYTE, std::chrono::steady_clock::time_point>::iterator lastPressTime = keyPressTimes.find(key);
            if (lastPressTime != keyPressTimes.end() &&
                std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPressTime->second).count() >= DEBOUNCE_DURATION) {
                // Key was in past but not pressed now, meaning it was released
                result.push_back("Key " + std::to_string(key) + " was released");
                updateKeyState(key, false, bitmask);
                keyPressTimes.erase(key); // Remove key from map
            }
        }
    }
    for (BYTE key : currentKeySet) {
        if (std::find(pastKeySet.begin(), pastKeySet.end(), key) == pastKeySet.end() && !isBitSet(bitmask, key)) {
            if (keyPressTimes.find(key) == keyPressTimes.end() ||
                std::chrono::duration_cast<std::chrono::milliseconds>(now - keyPressTimes[key]).count() >= DEBOUNCE_DURATION) {
                // Key is newly pressed
                result.push_back("Key " + std::to_string(key) + " was pressed");
                updateKeyState(key, true, bitmask);
                keyPressTimes[key] = now; // Update the time of the key press
            }
        }
    }

    return result;
}

void startLogging() {
    std::vector<BYTE> currentKeys; 
    std::vector<BYTE> pastKeys; 
    std::array<uint8_t, ARRAY_SIZE> keyStates = { 0 };
    std::vector<std::string> output;

    std::unordered_map<BYTE, std::chrono::steady_clock::time_point> keyPressTimes;
    std::chrono::milliseconds debounceDuration = std::chrono::milliseconds(DEBOUNCE_DURATION);

    while (true) {    
        currentKeys.clear();

        for (BYTE vkCode = 0; vkCode < 255; ++vkCode) {
            if (GetAsyncKeyState(vkCode) & 0x8000) { // Check if the key is pressed
                currentKeys.push_back(vkCode);
            }
        }

        std::vector<std::string> changes = findChanges(currentKeys, pastKeys, keyStates, keyPressTimes);
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

