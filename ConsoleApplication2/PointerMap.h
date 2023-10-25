#pragma once

#include <vector>
#include <Windows.h>
#include <inttypes.h>

#define PROCESS_NAME L"client.exe"

#define DIRECTION_NORTH 0
#define DIRECTION_EAST 1
#define DIRECTION_SOUTH 2
#define DIRECTION_WEST 3

struct vec3
{
    int x;
    int y;
    int z;

    bool operator==(vec3 other)
    {
        return this->x == other.x && this->y == other.y && this->z == other.z;
    }

    vec3* operator=(vec3 other)
    {
        this->x = other.x;
        this->y = other.y;
        this->z = other.z;
        return this;
    }

    int distance(vec3 other)
    {
        return abs(x - other.x) + abs(y - other.y);
    }
};

struct PlayerData {
    bool active = false;
    int hp = 0, maxhp = 0, mp = 0, maxmp = 0;
    unsigned short target = 0;
    bool protectionZone = 0;
    bool buff;
    unsigned int cF1 = 0, cF2 = 0, cF3 = 0, cF4 = 0, cF5 = 0, cF6 = 0, cAttack = 0;
    unsigned int cF7 = 0, cF8 = 0, cF9 = 0, cF10 = 0, cF11 = 0, cF12 = 0;
    unsigned short speed, baseSpeed;
    vec3 pos;
    float zoom;
    int battleCount;
    bool caveBotKilling = false;
    HWND window;
    unsigned char direction;
    tagRECT screen;

    std::vector<long long> internalCD = std::vector<long long>(12);
};

vec3 getDirectionOffset(int direction);

class PointerMap {
public:
    DWORD health;
    DWORD maxHealth;
    DWORD mana;
    DWORD maxMana;
    DWORD target;
    DWORD protectionZone;
    DWORD speed;
    DWORD baseSpeed;
    DWORD cooldownF1;
    DWORD cooldownF2;
    DWORD cooldownF3;
    DWORD cooldownF4;
    DWORD cooldownF5;
    DWORD cooldownF6;
    DWORD cooldownF7;
    DWORD cooldownF8;
    DWORD cooldownF9;
    DWORD cooldownF10;
    DWORD cooldownF11;
    DWORD cooldownF12;
    DWORD cooldownAttack;
    DWORD buff;
    DWORD positionX;
    DWORD positionY;
    DWORD positionZ;
    DWORD zoom;
    DWORD battleCount;
    DWORD direction;

    void initializePointers(HANDLE, DWORD);
};