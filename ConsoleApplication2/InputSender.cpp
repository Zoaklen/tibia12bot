#include "InputSender.h"
#include "LuaManager.h"

extern PlayerData playerData;

void sendKeyPress(BYTE key)
{
    SendMessage(playerData.window, WM_KEYDOWN, key, 0);
    SendMessage(playerData.window, WM_KEYUP, key, 0);
}