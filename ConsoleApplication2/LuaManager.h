#pragma once

#include "CaveBotManager.h"

#include <string>
#include <vector>
#include <mutex>
#include "PointerMap.h"

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#define CONFIG_FILENAME "config.ini"

class LuaManager
{
private:
	std::string scriptFile;
	lua_State* L;
    bool initialized = false;
    std::mutex mutex;

public:
	bool setScriptFile(std::string path);
	bool prepareInstance();
	bool runScript(PlayerData& data);
};