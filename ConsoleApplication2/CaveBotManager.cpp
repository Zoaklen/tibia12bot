#include "CaveBotManager.h"
#include <windows.h>

#pragma comment(lib, "winmm.lib")

int mini(int a, int b)
{
    return a < b ? a : b;
}

void CaveBotManager::think(long long clock)
{
    ReadProcessMemory(process, (LPVOID)pointerMap->positionX, &playerData->pos, sizeof(playerData->pos), NULL);
    ReadProcessMemory(process, (LPVOID)pointerMap->zoom, &playerData->zoom, sizeof(playerData->zoom), NULL);
    ReadProcessMemory(process, (LPVOID)pointerMap->battleCount, &playerData->battleCount, sizeof(playerData->battleCount), NULL);
    ReadProcessMemory(process, (LPVOID)pointerMap->protectionZone, &playerData->protectionZone, sizeof(playerData->protectionZone), NULL);
    if (!IsIconic(window))
        GetClientRect(window, &playerData->screen);

    this->myPos = playerData->pos;

    if ((!this->active && !this->integrated) || this->waypoints->size() < 1)
        return;

    //printf("Verifying cavebot killing...\n");
    if (!playerData->caveBotKilling)
    {
        if (playerData->battleCount >= this->killingTimeStart && !this->playerData->protectionZone)
        {
            playerData->caveBotKilling = true;
            maxKillingTime = clock + this->killingWaitTime;
        }
    }
    else
    {
        if (playerData->battleCount < this->killingTimeEnd || clock >= maxKillingTime)
        {
            playerData->caveBotKilling = false;
            maxWaypointTime = clock + this->waypointWaitTime;
        }
    }

    if (this->helpingAlly != nullptr && !this->helpingAlly->playerData->caveBotKilling)
    {
        this->helpingAlly = nullptr;
    }

    if (integratedCaveBots.size() > 0 && this->helpingAlly == nullptr)
    {
        if (this->playerData->caveBotKilling)
        {
            this->helpingAlly = this;
        }
        for (auto& cbm : integratedCaveBots)
        {
            if (cbm->playerData->caveBotKilling)
            {
                this->helpingAlly = cbm.get();
                break;
            }
        }
    }

    bool passCanStep = false;
    bool canDelete = true;
    if (!this->playerData->caveBotKilling)
    {
        if (this->helpingAlly != nullptr)
        {
            //printf("%s helping %p, verifying position...\n", (integrated?"Integrated":"Leader"), this->helpingAlly);
            vec3 allyPos;
            ReadProcessMemory(helpingAlly->process, (LPVOID)helpingAlly->pointerMap->positionX, &allyPos, sizeof(allyPos), NULL);

            //printf("Positions: %d %d %d | %d %d %d", myPos.x, myPos.y, myPos.z, allyPos.x, allyPos.y, allyPos.z);
            if (allyPos.z == myPos.z && allyPos.distance(myPos) <= 80)
            {
                //printf("Getting close to ally...\n");
                if(!autoFreeMode || integrated)
                    this->walkTo(allyPos, this->playerData);
            }
        }
        else
        {
            if (!this->integrated && this->integratedCaveBots.size() < 1)
            {
                //printf("Alone mode, verifying if can step...\n");
                this->canStepWaypoint = this->shouldStepWaypoint() || (clock >= maxWaypointTime && getWaypoint(currentWaypoint + 1).position.z == getWaypoint(currentWaypoint).position.z);
            }
            else if (this->integratedCaveBots.size() > 0)
            {
                //printf("Leader mode, verifying if can step...\n");
                if (!autoFreeMode)
                    this->canStepWaypoint = !this->helpingAlly && !(this->currentWaypoint < this->maximumReachableWaypoint&& clock <= maxWaypointTime) && (this->shouldStepWaypoint() || (clock >= maxWaypointTime && getWaypoint(currentWaypoint + 1).position.z == myPos.z));
                else
                    this->canStepWaypoint = true;
                //printf("Verifying for integrateds behind...\n");
                for (auto& cbm : this->integratedCaveBots)
                {
                    //printf("Integrated: %p...\n", cbm->helpingAlly);
                    //printf("Integrated: %d...\n", cbm->currentWaypoint);
                    //printf("Integrated: %d...\n", cbm->shouldStepWaypoint());
                    if (cbm->helpingAlly != nullptr || (cbm->currentWaypoint < cbm->maximumReachableWaypoint && clock < cbm->maxWaypointTime) || !(cbm->shouldStepWaypoint() || (clock >= cbm->maxWaypointTime && getWaypoint(currentWaypoint + 1).position.z == myPos.z)))
                    {
                        this->canStepWaypoint = false;
                        break;
                    }
                }
                //printf("Result: %d...\n", canStepWaypoint);
                passCanStep = this->canStepWaypoint;
                if (autoFreeMode && passCanStep)
                {
                    maximumReachableWaypoint++;
                }
            }
            int nextwp = this->currentWaypoint + 1;
            unsigned int startingWaypoint = currentWaypoint % waypoints->size();

            //printf("Looping through waypoint skipper...\n");
            if (autoFreeMode)
            {
                if (integrated)
                {
                    if (this->canStepWaypoint || this->shouldStepWaypoint() || (clock >= maxWaypointTime && getWaypoint(nextwp).position.z == myPos.z))
                    {
                        if(this->currentWaypoint < waypoints->size()-1)
                            this->currentWaypoint++;
                        maxWaypointTime = clock + this->waypointWaitTime;
                    }
                    Waypoint wp = getWaypoint(currentWaypoint);
                    this->walkTo(wp, playerData);
                }
                else
                {
                    if (waypoints->size() <= 1)
                    {
                        canDelete = false;
                    }
                    for (auto& cbm : integratedCaveBots)
                    {
                        if (cbm->currentWaypoint <= 0)
                        {
                            canDelete = false;
                            break;
                        }
                    }

                    if (canDelete)
                    {
                        //printf("Deleting first waypoint.\n");
                        waypoints->erase(waypoints->begin());
                        maximumReachableWaypoint--;
                    }
                }
            }
            else
            {
                while (this->canStepWaypoint || ((this->shouldStepWaypoint() || (clock >= maxWaypointTime && getWaypoint(nextwp).position.z == myPos.z)) && this->currentWaypoint < this->maximumReachableWaypoint))
                {
                    if (canStepWaypoint && integratedCaveBots.size() > 0)
                    {
                        //printf("Maximum reachable waypoint set to %d...\n", maximumReachableWaypoint);
                        this->maximumReachableWaypoint++;
                    }

                    this->currentWaypoint = nextwp;
                    //printf("%s waypoint set to %d.\n", (this->integrated ? "Integrated" : "Leader"), currentWaypoint);
                    nextwp = this->currentWaypoint + 1;
                    maxWaypointTime = clock + this->waypointWaitTime;
                    this->canStepWaypoint = (this->integratedCaveBots.size() > 0 || this->integrated) ? false : this->shouldStepWaypoint();
                    if (currentWaypoint % waypoints->size() == startingWaypoint)
                    {
                        printf("ANTI MACRO DETECTED. Cavebot toggled off.\n");
                        this->active = false;
                        SetForegroundWindow(this->window);
                        PlaySound(TEXT("beep.wav"), NULL, SND_FILENAME | SND_ASYNC);
                        return;
                    }
                }
                Waypoint wp = getWaypoint(currentWaypoint);
                this->walkTo(wp, playerData);
            }
            //printf("Walking to waypoint...\n");
        }
    }
    //printf("Updating integrated...\n");
    for (auto& cbm : this->integratedCaveBots)
    {
        cbm->waypoints = this->waypoints;
        if (autoFreeMode)
        {
            if (canDelete && cbm->currentWaypoint > 0)
            {
                cbm->currentWaypoint--;
                printf("Deleting, new waypoint: %d/%d\n", cbm->currentWaypoint, waypoints->size());
            }

            cbm->currentWaypoint = mini(waypoints->size()-1, cbm->currentWaypoint);
        }

        cbm->integrated = true;
        cbm->helpingAlly = this->helpingAlly;
        cbm->canStepWaypoint = passCanStep;
        cbm->killingTimeStart = this->killingTimeStart;
        cbm->killingTimeEnd = this->killingTimeEnd;
        cbm->waypointWaitTime = this->waypointWaitTime;
        cbm->killingWaitTime = this->killingWaitTime;
        cbm->maximumReachableWaypoint = this->maximumReachableWaypoint;
        cbm->autoFreeMode = this->autoFreeMode;
        cbm->think(clock);
    }
}