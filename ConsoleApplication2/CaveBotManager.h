#pragma once

#include <vector>
#include <string>
#include <math.h>
#include <filesystem>
#include <Windows.h>
#include "PointerMap.h"

class CaveBotManager
{
public:
	std::vector<vec3>* waypoints;
    unsigned int currentWaypoint = 0;
    vec3 myPos = { 0 };
    HWND window;
    HANDLE process;
    bool active = false;
    bool integrated = false;
    bool autoTracking = false;
    bool canStepWaypoint = false;
    int autoTrackSteps = 2;
    int killingTimeStart = 8;
    int killingTimeEnd = 4;
    int actionDelay = 1000;
    int waypointWaitTime = 10000;
    int killingWaitTime = 10000;
    CaveBotManager* helpingAlly = nullptr;
    long long nextCaveBotStep = 0L, maxKillingTime = 0L, maxWaypointTime = 0L;
    PlayerData* playerData;
    PlayerData* leader;
    std::vector<std::unique_ptr<CaveBotManager>> integratedCaveBots;
    PointerMap* pointerMap;
    unsigned int maximumReachableWaypoint = 0;
    bool autoFreeMode = false;

    CaveBotManager(std::vector<vec3>* waypointsPointer, PlayerData* playerData, PointerMap* pointerMap, HANDLE process, HWND window, PlayerData* leader = NULL)
    {
        this->waypoints = waypointsPointer;
        this->process = process;
        this->window = window;
        this->playerData = playerData;
        this->pointerMap = pointerMap;
        this->leader = leader;
    }

    void think(long long clock);

    bool shouldStepWaypoint()
    {
        vec3 wp = getWaypoint(currentWaypoint);
        vec3 nextwp = getWaypoint(currentWaypoint+1);

        if (myPos.z != wp.z)
            return true;

        if (myPos == wp)
            return true;

        if((abs(myPos.x - wp.x) + abs(myPos.y - wp.y)) <= 4 && nextwp.z == wp.z)
            return true;

        if ((abs(myPos.x - wp.x) + abs(myPos.y - wp.y)) > 100)
            return true;

        return false;
    }

    void resetData()
    {
        currentWaypoint = 0;
        maximumReachableWaypoint = 0;
        for (auto& cbm : integratedCaveBots)
        {
            cbm->currentWaypoint = 0;
            cbm->maximumReachableWaypoint = 0;
        }
        waypoints->clear();
    }

    void shareDataToIntegrateds()
    {
        for (auto& cbm : integratedCaveBots)
        {
            shareDataToIntegrated(cbm.get());
        }
    }

    void shareDataToIntegrated(CaveBotManager* integrated)
    {
        integrated->waypoints = this->waypoints;
    }

    void savePointsToFile(std::string path)
    {
        FILE* f;
        fopen_s(&f, path.c_str(), "w");
        fprintf_s(f, "%d %d %d\n", actionDelay, waypointWaitTime, killingWaitTime);
        fprintf_s(f, "%d %d\n", killingTimeStart, killingTimeEnd);
        fprintf_s(f, "%d\n", waypoints->size());
        for (auto& v : *this->waypoints)
        {
            fprintf_s(f, "%d %d %d\n", v.x, v.y, v.z);
        }
        fclose(f);
    }

    bool loadPointsFromFile(std::string path)
    {
        if (!std::filesystem::exists(path))
            return false;

        resetData();
        int size;
        FILE* f;
        fopen_s(&f, path.c_str(), "r");
        fscanf_s(f, "%d %d %d", &actionDelay, &waypointWaitTime, &killingWaitTime);
        fscanf_s(f, "%d %d", &killingTimeStart, &killingTimeEnd);
        fscanf_s(f, "%d", &size);
        waypoints->reserve(size);
        vec3 v;
        while(fscanf_s(f, "%d %d %d", &v.x, &v.y, &v.z) > 0)
        {
            waypoints->push_back(v);
            printf("Loaded [%d %d %d] as waypoint %d.\n", v.x, v.y, v.z, waypoints->size());
        }
        fclose(f);
        printf("Killing time start %d and end %d.\n", killingTimeStart, killingTimeEnd);
        shareDataToIntegrateds();
        return true;
    }
    
    void walkTo(vec3 pos, PlayerData* playerData)
    {
        if (myPos.distance(pos) >= 100 || myPos.z != pos.z)
            return;

        vec3 difference = { pos.x - myPos.x, pos.y - myPos.y, 0 };
        double normX = (float)difference.x / (53.0 / playerData->zoom);
        double normY = (float)difference.y / (53.0 / playerData->zoom);
        if (abs(normX) <= 1 && abs(normY) <= 1)
        {
            POINT cursor = { 0 };

            GetCursorPos(&cursor);

            double normToScreenX = normX * 53;
            double normToScreenY = normY * 53;

            HWND focus = GetForegroundWindow();
            if(focus != window)
                SetActiveWindow(window);

            long width = (playerData->screen.right - playerData->screen.left);
            long inputX = width - 114 + (long)(normToScreenX);
            long inputY = 59 + (long)(normToScreenY);

            SendMessage(window, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(width-28, 28));
            SendMessage(window, WM_LBUTTONUP, 0, MAKELPARAM(width - 28, 28));

            SendMessage(window, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(inputX, inputY));
            SendMessage(window, WM_LBUTTONUP, 0, MAKELPARAM(inputX, inputY));

            if (focus != window)
                SetActiveWindow(focus);
        }
    }

    void setAutoTracking(bool toggle, int step = 5)
    {
        this->autoTracking = toggle;
        this->autoTrackSteps = step;
    }

    vec3 getWaypoint(int index)
    {
        return (*this->waypoints)[index % this->waypoints->size()];
    }
};