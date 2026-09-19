#pragma once
#include "lua.h"
#include "lualib.h"
#include "lobject.h"
#include "lstate.h"
#include "lapi.h"
#include "ltable.h"
#include "lgc.h"
#include "../../../../Core/Execution/Execution.h"
#include "../../../Offsets/Offsets.h"
#include <Windows.h>
#include <string>

namespace misc
{
    inline int getexecutorname(lua_State* L)
    {
        lua_pushstring(L, "SkidBase");
        return 1;
    }

    inline int getgenv(lua_State* L)
    {
        lua_State* mainState = Execution::skidsstate;

        if (mainState == L) {
            lua_pushvalue(L, LUA_GLOBALSINDEX);
            return 1;
        }

        luaC_threadbarrier(mainState);
        lua_pushvalue(mainState, LUA_GLOBALSINDEX);
        lua_xmove(mainState, L, 1);

        return 1;
    }

    inline int getreg(lua_State* L)
    {
        lua_pushvalue(L, LUA_REGISTRYINDEX);
        return 1;
    }

    inline int getrenv(lua_State* L)
    {
        lua_State* RobloxState = L->global->mainthread;
        LuaTable* clone = luaH_clone(L, RobloxState->gt);

        lua_rawcheckstack(L, 1);
        luaC_threadbarrier(L);
        luaC_threadbarrier(RobloxState);

        L->top->value.p = clone;
        L->top->tt = LUA_TTABLE;
        L->top++;

        lua_rawgeti(L, LUA_REGISTRYINDEX, 2);
        lua_setfield(L, -2, "_G");
        lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
        lua_setfield(L, -2, "shared");

        return 1;
    }

    inline int getthreadidentity(lua_State* L)
    {
        if (!L->userdata) {
            lua_pushinteger(L, 0);
            return 1;
        }
        lua_pushinteger(L, (int)L->userdata->identity);
        return 1;
    }

    inline int setthreadidentity(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TNUMBER);
        int identity = static_cast<int>(lua_tointeger(L, 1));
        if (L->userdata == nullptr)
            return 0;

        uint64_t caps = (identity == 8) ? 0xFFFFFFFFFFFFFFFFULL : (0xFFFFFFFFFFFFFF00ULL | SkidBase::GetCapabilities(&identity));

        L->userdata->identity = identity;
        L->userdata->capabilities = caps;

        uintptr_t thread_tls = *reinterpret_cast<uintptr_t*>(Main::Identity1::IdentityPointer);
        uintptr_t idpointer = SkidBase::GetIdentityStruct(thread_tls);
        if (idpointer != 0) {
            *reinterpret_cast<uint64_t*>(idpointer) = identity;
            *reinterpret_cast<uint64_t*>(idpointer + Main::Identity2::Capabilities) = caps;
        }

        return 0;
    }

    inline int gethui(lua_State* L)
    {
        lua_getfield(L, LUA_GLOBALSINDEX, "game");
        lua_getfield(L, -1, "CoreGui");
        return 1;
    }

    inline int compareinstances(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TUSERDATA);
        luaL_checktype(L, 2, LUA_TUSERDATA);

        uintptr_t skid = *reinterpret_cast<uintptr_t*>(lua_touserdata(L, 1));
        uintptr_t paster = *reinterpret_cast<uintptr_t*>(lua_touserdata(L, 2));

        lua_pushboolean(L, skid == paster);
        return 1;
    }

    inline int setclipboard(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        std::string content = lua_tostring(L, 1);

        HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, content.size() + 1);
        if (!hmem)
            return 0;

        void* lockedMem = GlobalLock(hmem);
        if (!lockedMem) {
            GlobalFree(hmem);
            return 0;
        }

        memcpy(lockedMem, content.data(), content.size());
        static_cast<char*>(lockedMem)[content.size()] = '\0';
        GlobalUnlock(hmem);

        if (!OpenClipboard(nullptr)) {
            GlobalFree(hmem);
            return 0;
        }

        EmptyClipboard();
        SetClipboardData(CF_TEXT, hmem);
        CloseClipboard();

        return 0;
    }

    inline int getfpscap(lua_State* L)
    {
        int target = *reinterpret_cast<int*>(Main::Miscellaneous::TargetFPS);
        lua_pushinteger(L, target);
        return 1;
    }

    inline int setfpscap(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TNUMBER);
        *reinterpret_cast<int*>(Main::Miscellaneous::TargetFPS) = static_cast<int>(lua_tonumber(L, 1));
        return 0;
    }
}
