#include "Instances.h"
#include "../Offsets/Funcs.h"

namespace
{
    inline bool CheckStateMemory(uintptr_t address) {
        if (address < 0x10000 || address > 0x7FFFFFFFFFFF)
            return false;

        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi)) == 0)
            return false;

        if (mbi.Protect & PAGE_NOACCESS || mbi.State != MEM_COMMIT)
            return false;

        return true;
    }

    // Brute-force fallback: the ScriptContext holds the main Lua state behind
    // one of the four pointer encodings. Probe every qword in the first page.
    lua_State* FindLuaStateForInstance(uintptr_t sc)
    {
        if (!sc)
            return nullptr;

        __try
        {
            *reinterpret_cast<BOOLEAN*>(sc + Main::ExtraSpace::RequireBypass) = TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }

        auto dec1 = [](uint32_t a, uint32_t b) -> uint32_t { return b - a; };
        auto dec2 = [](uint32_t a, uint32_t b) -> uint32_t { return a + b; };
        auto dec3 = [](uint32_t a, uint32_t b) -> uint32_t { return a ^ b; };
        auto dec4 = [](uint32_t a, uint32_t b) -> uint32_t { return a - b; };

        auto enc_test = [&](uintptr_t address, uint32_t e0, uint32_t e1, auto&& op) -> uintptr_t
            {
                uint32_t low = op(static_cast<uint32_t>(address), e0);
                uint32_t high = op(static_cast<uint32_t>(address), e1);

                uintptr_t ptr =
                    (static_cast<uint64_t>(high) << 32) | static_cast<uint64_t>(low);

                if (!ptr)
                    return 0;

                if (!CheckStateMemory(ptr))
                    return 0;

                __try
                {
                    uintptr_t test = *reinterpret_cast<uintptr_t*>(ptr);
                    if (!test)
                        return 0;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    return 0;
                }

                return ptr;
            };

        for (uintptr_t offset = 0; offset <= 0x1000; offset += 0x8)
        {
            uintptr_t address = sc + offset;

            if (!CheckStateMemory(address))
                continue;

            __try
            {
                uint32_t* encrypted = reinterpret_cast<uint32_t*>(address);
                uint32_t e0 = encrypted[0];
                uint32_t e1 = encrypted[1];

                if (e0 == 0 && e1 == 0)
                    continue;

                if (uintptr_t p = enc_test(address, e0, e1, dec1)) return reinterpret_cast<lua_State*>(p);
                if (uintptr_t p = enc_test(address, e0, e1, dec2)) return reinterpret_cast<lua_State*>(p);
                if (uintptr_t p = enc_test(address, e0, e1, dec3)) return reinterpret_cast<lua_State*>(p);
                if (uintptr_t p = enc_test(address, e0, e1, dec4)) return reinterpret_cast<lua_State*>(p);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                continue;
            }
        }

        return nullptr;
    }
}

namespace laustate
{
    lua_State* GetLuasState(uintptr_t scriptContext)
    {
        uint64_t v1 = 8;
        uint64_t v2 = 8;

        __try
        {
            if (lua_State* L = SkidBase::GetLuaState(scriptContext, &v1, &v2))
                return L;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }

        return FindLuaStateForInstance(scriptContext);
    }
}
