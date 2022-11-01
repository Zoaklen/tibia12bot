#include <iostream>
#include <thread>
#include <chrono>
#include <array>
#include <algorithm>
#include <filesystem>

#include "InputSender.h"
#include "MemoryManager.h"
#include "PointerMap.h"
#include "LuaManager.h"
#include "CaveBotManager.h"

#define INTERNAL_COOLDOWN 100

using namespace std;

bool keepAlive = true;
bool debug = false;
bool disabled = false;
LuaManager luaManager;
PlayerData playerData = { 0 };
std::unique_ptr<CaveBotManager> caveBotManager;
HWND consoleHandle;
PointerMap pointerMap;
std::vector<vec3> caveBotWaypoints;

void updateLastScript(char* scriptName, size_t n)
{
    FILE* f;
    fopen_s(&f, CONFIG_FILENAME, "w");
    fprintf_s(f, "%s", scriptName);
    fclose(f);
}

void inputThread(HANDLE processHandle, HWND window)
{
    std::string input;
    HWND foreground;
    do
    {
        foreground = GetForegroundWindow();
        if (foreground != consoleHandle && foreground != window)
        {
            Sleep(100);
            continue;
        }

        if (GetAsyncKeyState(VK_INSERT) & 0x1)
        {
            sendKeyPress(VK_F6);
            printf("Send unfocused up arrow.\n");
        }
        if (GetAsyncKeyState(VK_SUBTRACT) & 0x1)
        {
            disabled = !disabled;
            printf("Bot %s.\n", disabled ? "disabled" : "enabled");
        }
        if (GetAsyncKeyState(VK_ADD) & 0x1)
        {
            debug = !debug;
            printf("Bot debug %s.\n", debug ? "enabled" : "disabled");
        }
        if (GetAsyncKeyState(VK_DIVIDE) & 0x1)
        {
            vec3 pos;
            ReadProcessMemory(processHandle, (LPVOID)pointerMap.positionX, &pos, sizeof(pos), NULL);
            caveBotWaypoints[caveBotManager->currentWaypoint] = pos;
            printf("Cave bot waypoint %d changed to [%d %d %d]\n", caveBotManager->currentWaypoint, pos.x, pos.y, pos.z);
        }
        if (GetAsyncKeyState(VK_MULTIPLY) & 0x1)
        {
            caveBotManager->active = !caveBotManager->active;
            if (caveBotManager->active && caveBotWaypoints.size() < 1 && !caveBotManager->autoFreeMode)
            {
                printf("No cavebot script loaded.\n");
                caveBotManager->active = false;
            }
            else
                printf("Cave bot toggled %s.\n", caveBotManager->active ? "on" : "off");
        }
        if (GetAsyncKeyState(VK_DECIMAL) & 0x1)
        {
            vec3 pos;
            ReadProcessMemory(processHandle, (LPVOID)pointerMap.positionX, &pos, sizeof(pos), NULL);
            caveBotWaypoints.push_back(pos);
            if (caveBotManager->autoFreeMode)
                caveBotWaypoints.push_back(playerData.pos);
            printf("Added position [%d %d %d] to cave bot waypoint %d\n", pos.x, pos.y, pos.z, caveBotWaypoints.size());
        }
        if (GetAsyncKeyState(VK_DELETE) & 0x1)
        {
            vec3 pos;
            ReadProcessMemory(processHandle, (LPVOID)pointerMap.positionX, &pos, sizeof(pos), NULL);
            pos.x--;
            caveBotWaypoints.push_back(pos);
            if (caveBotManager->autoFreeMode)
                caveBotWaypoints.push_back(playerData.pos);
            printf("Added position [%d %d %d] to cave bot waypoint %d\n", pos.x, pos.y, pos.z, caveBotWaypoints.size());
        }
        if (GetAsyncKeyState(VK_END) & 0x1)
        {
            vec3 pos;
            ReadProcessMemory(processHandle, (LPVOID)pointerMap.positionX, &pos, sizeof(pos), NULL);
            pos.y++;
            caveBotWaypoints.push_back(pos);
            if (caveBotManager->autoFreeMode)
                caveBotWaypoints.push_back(playerData.pos);
            printf("Added position [%d %d %d] to cave bot waypoint %d\n", pos.x, pos.y, pos.z, caveBotWaypoints.size());
        }
        if (GetAsyncKeyState(VK_NEXT) & 0x1)
        {
            vec3 pos;
            ReadProcessMemory(processHandle, (LPVOID)pointerMap.positionX, &pos, sizeof(pos), NULL);
            pos.x++;
            caveBotWaypoints.push_back(pos);
            if (caveBotManager->autoFreeMode)
                caveBotWaypoints.push_back(playerData.pos);
            printf("Added position [%d %d %d] to cave bot waypoint %d\n", pos.x, pos.y, pos.z, caveBotWaypoints.size());
        }
        if (GetAsyncKeyState(VK_HOME) & 0x1)
        {
            vec3 pos;
            ReadProcessMemory(processHandle, (LPVOID)pointerMap.positionX, &pos, sizeof(pos), NULL);
            pos.y--;
            caveBotWaypoints.push_back(pos);
            if (caveBotManager->autoFreeMode)
                caveBotWaypoints.push_back(playerData.pos);
            printf("Added position [%d %d %d] to cave bot waypoint %d\n", pos.x, pos.y, pos.z, caveBotWaypoints.size());
        }

        if (GetForegroundWindow() == consoleHandle && GetAsyncKeyState(VK_TAB) & 0x1)
        {
            printf("\n>> ");
            std::cin >> input;
            std::for_each(input.begin(), input.end(), ::tolower);
            if (input.compare("reset") == 0)
            {
                printf("Resetting pointers...\n");
                DWORD pID;
                GetWindowThreadProcessId(window, &pID);
                pointerMap.initializePointers(processHandle, pID);
                printf("Pointer reset.\n");
            }
            else if (input.compare("cavereset") == 0)
            {
                caveBotManager->resetData();
                printf("Cavebot waypoints were reset.\n");
            }
            else if (input.compare("caveshare") == 0)
            {
                caveBotManager->shareDataToIntegrateds();
                printf("Cavebot data was shared to integrateds.\n");
            }
            else if (input.compare("cavesave") == 0)
            {
                printf_s("Cavebot file name:\n>> ");
                std::cin >> input;
                caveBotManager->savePointsToFile("scripts/cavebot/" + input + ".cav");
                printf_s("Waypoints saved to %s.\n", ("scripts/cavebot/" + input + ".cav").c_str());
            }
            else if (input.compare("caveload") == 0)
            {
                printf_s("Cavebot file name:\n>> ");
                std::cin >> input;
                if (!caveBotManager->loadPointsFromFile("scripts/cavebot/" + input + ".cav"))
                {
                    printf_s("Could not load waypoints from file %s.\n", ("scripts/cavebot/" + input + ".cav").c_str());
                    continue;
                }
                printf_s("Loaded %d waypoints from %s.\n", caveBotWaypoints.size(), ("scripts/cavebot/" + input + ".cav").c_str());
            }
            else if (input.compare("restartscript") == 0)
            {
                printf("Restarting script...\n");
                luaManager.prepareInstance();
                printf("Script restarted.\n");
            }
            else if (input.compare("setscript") == 0)
            {
                printf("Enter script name:\n>> ");
                std::cin >> input;
                if (!luaManager.setScriptFile("scripts/" + input + ".lua"))
                {
                    printf("File scripts/%s.lua could not be loaded.", input.c_str());
                    continue;
                }

                updateLastScript(&input[0], input.length());

                luaManager.prepareInstance();
            }
            else if (input.compare("walk") == 0)
            {
                vec3 pos = playerData.pos;
                printf("How many X (right) and Y (down) to walk?\n>> ");
                int down, right;
                std::cin >> right >> down;
                pos.y += down;
                pos.x += right;
                caveBotManager->walkTo(pos, &playerData);
            }
            else if (input.compare("cave") == 0)
            {
                caveBotManager->active = !caveBotManager->active;
                if (caveBotManager->active && caveBotWaypoints.size() < 1 && !caveBotManager->autoFreeMode)
                {
                    printf("No cavebot script loaded.\n");
                    caveBotManager->active = false;
                    continue;
                }
                printf("Cave bot toggled %s.\n", caveBotManager->active ? "on" : "off");
            }
            else if (input.compare("caveauto") == 0)
            {
                if (caveBotManager->autoTracking)
                {
                    caveBotManager->autoTracking = false;
                    printf("Cave bot auto tracking toggled off.\n");
                }
                else
                {
                    printf("Auto tracking step size:\n>> ");
                    int step;
                    std::cin >> step;
                    caveBotManager->setAutoTracking(true, step);
                    printf("Cave bot auto tracking toggled on. Tracking every %d steps.\n", step);
                }
            }
            else if (input.compare("caveautofree") == 0)
            {
                caveBotManager->autoFreeMode = !caveBotManager->autoFreeMode;
                printf("Cave bot auto free toggled %s.\n", caveBotManager->autoFreeMode ? "on" : "off");
            }
            else if (input.compare("cavekill") == 0)
            {
                printf("Killing time START and END:\n>> ");
                std::cin >> caveBotManager->killingTimeStart >> caveBotManager->killingTimeEnd;
                printf("Killing time START/END set to %d/%d.\n", caveBotManager->killingTimeStart, caveBotManager->killingTimeEnd);
            }
            else if (input.compare("cavedelay") == 0)
            {
                printf("Cavebot action delay time (ms):\n>> ");
                std::cin >> caveBotManager->actionDelay;
                printf("Cavebot now steps every %dms.\n", caveBotManager->actionDelay);
            }
            else if (input.compare("cavewptime") == 0)
            {
                printf("Cavebot waypoint wait time (ms):\n>> ");
                std::cin >> caveBotManager->waypointWaitTime;
                printf("Cavebot waits up to %dms to reach waypoint.\n", caveBotManager->waypointWaitTime);
            }
            else if (input.compare("cavekilltime") == 0)
            {
                printf("Cavebot killing wait time (ms):\n>> ");
                std::cin >> caveBotManager->killingWaitTime;
                printf("Cavebot waits up to %dms to slay enemies.\n", caveBotManager->killingWaitTime);
            }
            else if(input.compare("caveintegrate") == 0)
            {
                DWORD pID;
                HANDLE handle = getProcessByExe((WCHAR*)L"FuriaOT_client.exe", pID);
                HWND hwnd = getHWNDByPid(pID);
                if (processHandle == handle)
                {
                    printf("You cannot integrate the same client.\n");
                    continue;
                }

                PlayerData* pdata = (PlayerData*)malloc(sizeof(PlayerData));
                ZeroMemory(pdata, sizeof(pdata));
                PointerMap* pmap = new PointerMap;
                pmap->initializePointers(handle, pID);

                caveBotManager->integratedCaveBots.push_back(std::make_unique<CaveBotManager>(CaveBotManager(caveBotManager->waypoints, pdata, pmap, handle, hwnd, &playerData)));
                caveBotManager->shareDataToIntegrated(caveBotManager->integratedCaveBots[caveBotManager->integratedCaveBots.size()-1].get());
                printf("Added process with PID %lu to cavebot integration.\n", pID);
            }
            else if (input.compare("cavedesintegrate") == 0)
            {
                for (auto& cbm : caveBotManager->integratedCaveBots)
                {
                    free(cbm->playerData);
                    delete cbm->pointerMap;
                }
                caveBotManager->integratedCaveBots.clear();
                printf("Cleared integrated cavebots.\n");
            }
        }
    } while (true);
}

void caveBotThread(HANDLE processHandle, HWND window)
{
    long long clock;
    vec3 lastTrackedPosition = { 0 };
    vec3 oldPosition = { 0 };
    char oldDirection = 0;
    while (true)
    {
        clock = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        if (disabled/* || !playerData.active*/)
        {
            Sleep(1000);
            continue;
        }
        
        if (caveBotManager->autoTracking || caveBotManager->autoFreeMode)
        {
            oldPosition = playerData.pos;
            oldDirection = playerData.direction;
            ReadProcessMemory(processHandle, (LPVOID)pointerMap.positionX, &playerData.pos, sizeof(playerData.pos), NULL);
            ReadProcessMemory(processHandle, (LPVOID)pointerMap.direction, &playerData.direction, sizeof(playerData.direction), NULL);

            if (playerData.pos.distance(oldPosition) >= 20)
            {
                vec3 stairPosition = oldPosition;

                vec3 offset = getDirectionOffset(oldDirection);
                stairPosition.x += offset.x;
                stairPosition.y += offset.y;

                lastTrackedPosition = playerData.pos;
                caveBotWaypoints.push_back(stairPosition);
                printf("Added teleport position [%d %d %d] to cave bot waypoint %d\n", stairPosition.x, stairPosition.y, stairPosition.z, caveBotWaypoints.size());
            }
            else if (playerData.pos.z != oldPosition.z)
            {
                vec3 stairPosition = playerData.pos;
                stairPosition.z = oldPosition.z;

                vec3 offset = getDirectionOffset(playerData.direction);
                offset.x *= -1;
                offset.y *= -1;
                stairPosition.x += offset.x;
                stairPosition.y += offset.y;

                lastTrackedPosition = playerData.pos;
                caveBotWaypoints.push_back(stairPosition);
                printf("Old: %d %d %d\nCur: %d %d %d\n", oldPosition.x, oldPosition.y, oldPosition.z, playerData.pos.x, playerData.pos.y, playerData.pos.z);
                printf("Added stair position [%d %d %d] to cave bot waypoint %d\n", stairPosition.x, stairPosition.y, stairPosition.z, caveBotWaypoints.size());
            }
            else if (playerData.pos.distance(lastTrackedPosition) >= caveBotManager->autoTrackSteps)
            {
                lastTrackedPosition = playerData.pos;
                caveBotWaypoints.push_back(playerData.pos);
                printf("Added position [%d %d %d] to cave bot waypoint %d\n", playerData.pos.x, playerData.pos.y, playerData.pos.z, caveBotWaypoints.size());
            }
        }
        
        if(!caveBotManager->autoTracking)
        {
            caveBotManager->think(clock);
            Sleep(caveBotManager->actionDelay);
        }
    }
}

void loopThread(HANDLE processHandle, HWND window)
{
    // Internal variables
    auto clock = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    auto oldClock = clock;
    float hpp = 0.0;

    printf("Creating LUA Manager...\n");
    printf("Loading last script file...\n");
    char scriptFile[100];

    if (std::filesystem::exists(CONFIG_FILENAME))
    {
        FILE* f;
        fopen_s(&f, CONFIG_FILENAME, "r");
        fscanf_s(f, "%[^\n]s", scriptFile, sizeof(scriptFile));
        fclose(f);
    }
    else
    {
        printf("No last file detected.\n");
        do
        {
            printf("Enter script name:\n>> ");
            scanf_s("%s", scriptFile, sizeof(scriptFile));
        } while (!std::filesystem::exists("scripts/" + std::string(scriptFile) + ".lua"));
        updateLastScript(scriptFile, sizeof(scriptFile));
    }

    printf("Setting script file to %s...\n", scriptFile);

    if (!luaManager.setScriptFile("scripts/" + std::string(scriptFile) + ".lua"))
    {
        printf("Error while loading script file %s.\n", scriptFile);
        return;
    }
    printf("Preparing LUA instance...\n");
    luaManager.prepareInstance();

    caveBotManager = std::make_unique<CaveBotManager>(CaveBotManager(&caveBotWaypoints, &playerData, &pointerMap, processHandle, window));

    std::thread inputThread(inputThread, processHandle, window);
    std::thread caveBotThread(caveBotThread, processHandle, window);
    while (keepAlive)
    {
        clock = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        for (auto &el : playerData.internalCD)
        {
            el -= clock - oldClock;
        }

        if (disabled)
        {
            Sleep(100);
            continue;
        }
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.health, &playerData.hp, sizeof(playerData.hp), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.maxHealth, &playerData.maxhp, sizeof(playerData.maxhp), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.mana, &playerData.mp, sizeof(playerData.mp), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.maxMana, &playerData.maxmp, sizeof(playerData.maxmp), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.target, &playerData.target, sizeof(playerData.target), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.protectionZone, &playerData.protectionZone, sizeof(playerData.protectionZone), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.speed, &playerData.speed, sizeof(playerData.speed), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.baseSpeed, &playerData.baseSpeed, sizeof(playerData.speed), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.buff, &playerData.buff, sizeof(playerData.buff), NULL);
        //ReadProcessMemory(processHandle, (LPVOID)pointerMap.positionX, &playerData.pos.x, sizeof(playerData.pos.x), NULL);
        //ReadProcessMemory(processHandle, (LPVOID)pointerMap.positionY, &playerData.pos.y, sizeof(playerData.pos.y), NULL);
        //ReadProcessMemory(processHandle, (LPVOID)pointerMap.positionZ, &playerData.pos.z, sizeof(playerData.pos.z), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.zoom, &playerData.zoom, sizeof(playerData.zoom), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.battleCount, &playerData.battleCount, sizeof(playerData.battleCount), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.direction, &playerData.direction, sizeof(playerData.direction), NULL);

        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownF1, &playerData.cF1, sizeof(playerData.cF1), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownF2, &playerData.cF2, sizeof(playerData.cF2), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownF3, &playerData.cF3, sizeof(playerData.cF3), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownF4, &playerData.cF4, sizeof(playerData.cF4), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownF5, &playerData.cF5, sizeof(playerData.cF5), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownF6, &playerData.cF6, sizeof(playerData.cF6), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownF7, &playerData.cF7, sizeof(playerData.cF7), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownF8, &playerData.cF8, sizeof(playerData.cF8), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownF9, &playerData.cF9, sizeof(playerData.cF9), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownF10, &playerData.cF10, sizeof(playerData.cF10), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownF11, &playerData.cF11, sizeof(playerData.cF11), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownF12, &playerData.cF12, sizeof(playerData.cF12), NULL);
        ReadProcessMemory(processHandle, (LPVOID)pointerMap.cooldownAttack, &playerData.cAttack, sizeof(playerData.cAttack), NULL);

        if (!IsIconic(window))
            GetClientRect(window, &playerData.screen);

        playerData.active = GetForegroundWindow() == window;
        if (debug)
        {
            printf("Position: %d %d %d | Zoom: %.1f\n", playerData.pos.x, playerData.pos.y, playerData.pos.z, playerData.zoom);
            printf("HP: %d/%d MP: %d/%d\n", playerData.hp, playerData.maxhp, playerData.mp, playerData.maxmp);
            printf("Target ID: %d\n", playerData.target);
            printf("Protection Zone: %d | Speed: %d | Buff: %d\n", playerData.protectionZone, playerData.speed, playerData.buff);
            printf("F1: %d | F3: %d | F4: %d | F5: %d | F6: %d\n", playerData.cF1, playerData.cF3, playerData.cF4, playerData.cF5, playerData.cF6);
            printf("F10: %d | F11: %d\n", playerData.cF10, playerData.cF11);
            printf("Internal F1: %lld | F3: %lld | F4: %lld | F5: %lld | F6: %lld\n", playerData.internalCD[0], playerData.internalCD[2], playerData.internalCD[3], playerData.internalCD[4], playerData.internalCD[5]);
            printf("Attack: %d\n", playerData.cAttack);
        }

        luaManager.runScript(playerData);

        oldClock = clock;
        Sleep(51);
    }
    inputThread.detach();
}

int main()
{
    DWORD pID;
    HANDLE processHandle = getProcessByExe((WCHAR*)PROCESS_NAME, pID);
    HWND tibia = getHWNDByPid(pID);
    WCHAR ctitle[256];
    GetConsoleTitle(ctitle, 256);
    consoleHandle = FindWindow(NULL, ctitle);
    if (consoleHandle == NULL)
    {
        std::cout << "Could not obtain self handle!" << std::endl;
        system("pause");
        return 0;
    }

    if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL)
    {
        std::cout << "Failed to open process!" << std::endl;
        system("pause");
        return 0;
    }


    TCHAR* title = const_cast<wchar_t*>(PROCESS_NAME);

    pointerMap.initializePointers(processHandle, pID);
    
    playerData.window = tibia;
    std::thread lThread(loopThread, processHandle, tibia);

    lThread.join();
    std::cout << "Program ending\n";

    CloseHandle(processHandle);
}