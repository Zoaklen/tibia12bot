#include "LuaManager.h"

#include <filesystem>
#include <iostream>
#include <Windows.h>
#include "InputSender.h"
#include <map>

const std::map<std::string, int> LUA_CONSTANTS = {
	{"VK_F1", 0x70},
	{"VK_F2", 0x71},
	{"VK_F3", 0x72},
	{"VK_F4", 0x73},
	{"VK_F5", 0x74},
	{"VK_F6", 0x75},
	{"VK_F7", 0x76},
	{"VK_F8", 0x77},
	{"VK_F9", 0x78},
	{"VK_F10", 0x79},
	{"VK_F11", 0x7A},
	{"VK_F12", 0x7B},
	{"VK_SPACE", VK_SPACE},
	{"INTERNAL_COOLDOWN", 100}
};

void setGlobalVariable(lua_State* L, const char* name, int value)
{
	lua_pushinteger(L, value);
	lua_setglobal(L, name);
}

void setGlobalVariableVector(lua_State* L, const char* name, std::vector<long long> arr)
{
	lua_getglobal(L, name);
	if (!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		lua_newtable(L);
	}
	for (size_t i = 0; i < arr.size(); i++)
	{
		lua_pushinteger(L, i);
		lua_pushinteger(L, arr[i]);
		lua_rawset(L, -3);
	}
	lua_pushliteral(L, "n");
	lua_pushnumber(L, arr.size());
	lua_rawset(L, -3);
	lua_setglobal(L, name);
}

bool CheckLua(lua_State* L, int r)
{
	if (r != LUA_OK)
	{
		std::string errormsg = lua_tostring(L, -1);
		std::cout << errormsg << std::endl;
		return false;
	}
	return true;
}

bool LuaManager::setScriptFile(std::string path)
{
	if (!std::filesystem::exists(path))
		return false;

	this->scriptFile = path;
	return true;
}

int lua_pressKey(lua_State* L)
{
	BYTE key = (BYTE)lua_tointeger(L, 1);
	if (key >= VK_F1 && key <= VK_F12)
	{
		int index = key - VK_F1;

		lua_getglobal(L, "internalCD");
		lua_pushinteger(L, index);
		lua_gettable(L, -2);
		int cd = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);

		if (cd > 0)
			return 0;

		lua_pushinteger(L, index);
		lua_pushinteger(L, 100);
		lua_rawset(L, -3);
	}
	sendKeyPress(key);
	return 0;
}

bool LuaManager::prepareInstance()
{
	if (this->scriptFile.length() < 1)
	{
		std::cout << "No script file loaded.\n";
		return false;
	}

	this->mutex.lock();
	if (this->L != NULL && this->initialized)
	{
		lua_close(this->L);
		this->initialized = false;
	}

	std::cout << "Initializing new LUA state...\n";
	this->L = luaL_newstate();
	std::cout << "Initializing Open LUA libs...\n";
	luaL_openlibs(this->L);
	lua_register(L, "pressKey", lua_pressKey);

	for (auto& entry : LUA_CONSTANTS)
	{
		setGlobalVariable(this->L, entry.first.c_str(), entry.second);
	}

	if (!CheckLua(this->L, luaL_dofile(this->L, this->scriptFile.c_str())))
	{
		lua_close(this->L);
		this->mutex.unlock();
		return false;
	}
	this->initialized = true;
	std::cout << "LUA state initialized.\n";
	this->mutex.unlock();
	return true;
}

bool LuaManager::runScript(PlayerData& data)
{
	this->mutex.lock();
	setGlobalVariable(this->L, "health", data.hp);
	setGlobalVariable(this->L, "maxHealth", data.maxhp);
	setGlobalVariable(this->L, "mana", data.mp);
	setGlobalVariable(this->L, "maxMana", data.maxmp);
	setGlobalVariable(this->L, "target", data.target);
	setGlobalVariable(this->L, "pz", data.protectionZone);
	setGlobalVariable(this->L, "buff", data.buff);
	setGlobalVariable(this->L, "cF1", data.cF1);
	setGlobalVariable(this->L, "cF2", data.cF2);
	setGlobalVariable(this->L, "cF3", data.cF3);
	setGlobalVariable(this->L, "cF4", data.cF4);
	setGlobalVariable(this->L, "cF5", data.cF5);
	setGlobalVariable(this->L, "cF6", data.cF6);
	setGlobalVariable(this->L, "cF7", data.cF7);
	setGlobalVariable(this->L, "cF8", data.cF8);
	setGlobalVariable(this->L, "cF9", data.cF9);
	setGlobalVariable(this->L, "cF10", data.cF10);
	setGlobalVariable(this->L, "cF11", data.cF11);
	setGlobalVariable(this->L, "cF12", data.cF12);
	setGlobalVariable(this->L, "cAttack", data.cAttack);
	setGlobalVariable(this->L, "speed", data.speed);
	setGlobalVariable(this->L, "baseSpeed", data.baseSpeed);
	setGlobalVariable(this->L, "battleCount", data.battleCount);
	setGlobalVariableVector(this->L, "internalCD", data.internalCD);

	lua_getglobal(this->L, "loop");

	bool ret = false;
	if (lua_isfunction(this->L, -1))
	{
		ret = CheckLua(this->L, lua_pcall(this->L, 0, 0, 0));
	}

	lua_getglobal(this->L, "internalCD");
	if (lua_istable(this->L, -1))
	{
		for (size_t i = 0; i < data.internalCD.size(); i++)
		{
			lua_pushinteger(this->L, i);
			lua_gettable(this->L, -2);
			data.internalCD[i] = lua_tointeger(this->L, -1);
			lua_pop(this->L, 1);
		}
	}
	this->mutex.unlock();
	return ret;
}