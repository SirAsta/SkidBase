#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <vector>
#include <cstdint>
#include "../../Roblox/Offsets/Offsets.h"
#include "../../Roblox/Offsets/Funcs.h"
#include "lua.h"
#include "lualib.h"
#include "lobject.h"
#include "Luau/Compiler.h"
#include "Luau/BytecodeBuilder.h"

namespace Execution {
    extern lua_State* rboloxstate;
    extern lua_State* skidsstate;
    extern std::queue<std::string> queue;
    extern std::mutex mutexex;

    extern const uintptr_t caps;

    std::string aexecute(std::string source);
    void setprotocapabilities(Proto* proto, uintptr_t* capabilities);
    void setthreadcapabilities(lua_State* L, int level, uintptr_t capabilities, bool AddExecutorMark);
    void execute(lua_State* L, const std::string& script);
    void extexecute(const std::string& script);
    uintptr_t GetJobByTypeName(const std::string& TypeName);
}

LUAU_FASTFLAG(LuauCIProto)
inline void flagfix() { FFlag::LuauCIProto.value = true; }
