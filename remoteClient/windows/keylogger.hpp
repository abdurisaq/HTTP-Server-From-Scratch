#ifndef keylogger
#define keylogger

#include <unordered_map>
//#include <windows.h>
//#include <winuser.h>
#include <cstdint>
#include <vector>
#include <array>

#define ARRAY_SIZE 32
#define DEBOUNCE_DURATION 5

//used to be 50
void updateKeyState(uint8_t vkCode, bool pressed, std::array<uint8_t, ARRAY_SIZE>& keyStates);


bool isBitSet(const std::array<uint8_t, ARRAY_SIZE>& keyStates, uint8_t vkCode);


std::vector<uint32_t> findChanges(const std::vector<BYTE>& currentKeys,
                                     const std::vector<BYTE>& pastKeys,
                                      std::array<uint8_t, ARRAY_SIZE>& bitmask,
                                     std::unordered_map<BYTE, std::chrono::steady_clock::time_point>& keyPressTimes);

std::vector<uint32_t> packetize(std::vector<uint32_t> keystrokes);

void logMousePos(SOCKET clientUDPFD, std::atomic<bool>& running, ULONG serverAddr);


double calculateDistance(POINT a, POINT b);

void startLogging(SOCKET clientFD,std::atomic<bool>& running);

char parsePacket(char * buffer, int numBits);

std::vector<std::pair<BYTE,bool>> decodePacket(char * packet, int numBits);



void simulateKeystrokes(const std::vector<std::pair<BYTE, bool>>& keyEvents);

#endif // MY_HEADER_H

