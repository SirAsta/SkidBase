#include "Execution.h"
#include "Luau/BytecodeUtils.h"
#include "lstate.h"
#include "lobject.h"
#include "lapi.h"
#include <cstring>

namespace Execution
{
    template<typename T>
    static T read(uintptr_t address, uintptr_t offset = 0) {
        return *reinterpret_cast<T*>(address + offset);
    }

    lua_State* rboloxstate = nullptr;
    lua_State* skidsstate = nullptr;
    std::queue<std::string> queue;
    std::mutex mutexex;

    uintptr_t caps = 0xFFFFFFFFFFFFFFFF;

    class BytecodeEncoder : public Luau::BytecodeEncoder
    {
        inline void encode(uint32_t* data, size_t count) override
        {
            for (auto i = 0; i < count;)
            {
                uint8_t opcode = LUAU_INSN_OP(data[i]);
                const auto lookuptable = reinterpret_cast<uint8_t*>(Main::Functions::OpcodeLookupTable);
                uint8_t opdiamondsword = opcode * 227;
                opdiamondsword = lookuptable[opdiamondsword];

                data[i] = (opdiamondsword) | (data[i] & ~0xFF);
                i += Luau::getOpLength(static_cast<LuauOpcode>(opcode));
            }
        }
    };

    std::string aexecute(std::string source)
    {
        auto bytecide = BytecodeEncoder();
        static const char* globalz[] = { "Game", "Workspace", "game", "plugin", "script", "shared", "workspace", "_G", "_ENV", nullptr };

        Luau::CompileOptions options;
        options.debugLevel = 1;
        options.optimizationLevel = 1;
        options.mutableGlobals = globalz;
        options.vectorLib = "Vector3";
        options.vectorCtor = "new";
        options.vectorType = "Vector3";

        return Luau::compile(source, options, {}, &bytecide);
    }

    void setprotocapabilities(Proto* root, uintptr_t* capabilities)
    {
        if (!root)
            return;

        std::vector<Proto*> pending{ root };
        while (!pending.empty())
        {
            Proto* current = pending.back();
            pending.pop_back();
            if (!current)
                continue;

            current->userdata = capabilities;
            if (current->sizep < 0 || current->sizep > 100000 || (!current->p && current->sizep != 0))
                continue;

            for (int i = 0; i < current->sizep; ++i)
                if (current->p[i])
                    pending.push_back(current->p[i]);
        }
    }

    void setthreadcapabilities(lua_State* L, int level, uintptr_t capabilities, bool AddExecutorMark)
    {
        if (!L || !L->userdata) return;
        
        L->userdata->identity = level;
        L->userdata->capabilities = capabilities | (AddExecutorMark ? 0b1000000 : 0);
    }

    void execute(lua_State* L, const std::string& script)
    {
		std::lock_guard<std::mutex> lock(Execution::mutexex);
        if (!L || script.empty())
            return;

        int originalTop = lua_gettop(L);
        lua_State* threadex = lua_newthread(L);
        // checks please
		if (!threadex)
		{
			lua_getglobal(L, "print");
			lua_pushstring(L, "failed to create a thread");
			lua_pcall(L, 1, 0, 0);
			return;
		}
        lua_pop(L, 1);

        luaL_sandboxthread(threadex);
        
        setthreadcapabilities(threadex, 8, caps, false);

        std::string bytecode = aexecute(script);
        if (luau_load(threadex, "", bytecode.c_str(), bytecode.length(), NULL) != LUA_OK)
        {
            std::string error = lua_tostring(threadex, -1);
            
            lua_getglobal(L, "print");
            lua_pushstring(L, ("script error:" + error).c_str()); // it cool
            lua_pcall(L, 1, 0, 0);

            lua_pop(threadex, 1);
			lua_settop(L, originalTop);
            return;
        }

        Closure* closure = clvalue(const_cast<TValue*>(luaA_toobject(threadex, -1)));
        setprotocapabilities(closure->l.p, const_cast<uintptr_t*>(&caps));

        lua_getglobal(threadex, "task");
        lua_getfield(threadex, -1, "defer");
        lua_remove(threadex, -2);
        lua_insert(threadex, -2);

        if (lua_pcall(threadex, 1, NULL, NULL) != LUA_OK)
        {
            std::string error = lua_tostring(threadex, -1);
            
            lua_getglobal(L, "print");
            lua_pushstring(L, ("(your shitsploit name) runtime error: " + error).c_str());
            lua_pcall(L, 1, 0, 0);

            lua_pop(threadex, 1);
			lua_settop(L, originalTop);
            return;
        }

        lua_settop(L, originalTop);
    }

    void extexecute(const std::string& script) {
        if (script.empty())
            return;

        std::lock_guard<std::mutex> Lock(Execution::mutexex);
        while (Execution::queue.size() >= 512)
            Execution::queue.pop();

        Execution::queue.push(script);
    }

    static bool CheckMemory(uintptr_t address) {
        if (address < 0x10000 || address > 0x7FFFFFFFFFFF)
            return false;

        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi)) == 0)
            return false;

        if (mbi.Protect & PAGE_NOACCESS || mbi.State != MEM_COMMIT)
            return false;

        return true;
    }

    uintptr_t GetJobByTypeName(const std::string& TypeName) {
        uintptr_t taskScheduler = read<uintptr_t>(Main::Scheduler::TaskScheduler);
        if (!taskScheduler)
            return 0;

        uintptr_t jobsStart = read<uintptr_t>(taskScheduler + Main::Scheduler::JobStart);
        uintptr_t jobsEnd = read<uintptr_t>(taskScheduler + Main::Scheduler::JobEnd);
        if (!jobsStart || !jobsEnd || jobsStart >= jobsEnd)
            return 0;

        const size_t nameLen = TypeName.size();
        if (nameLen == 0 || nameLen >= 0x100)
            return 0;

        // Job type name pointer lives at 0xF8 per the dump; 0x18/0x118 kept as fallback
        static constexpr uintptr_t kNameOffsets[] = { 0x18, 0xF8, 0x118 };

        for (uintptr_t current = jobsStart; current < jobsEnd; current += 0x10)
        {
            uintptr_t job = read<uintptr_t>(current);
            if (!job)
                continue;

            for (uintptr_t off : kNameOffsets)
            {
                if (!CheckMemory(job + off))
                    continue;

                const char* candidate = *reinterpret_cast<const char**>(job + off);
                if (!candidate)
                    continue;

                uintptr_t candAddr = reinterpret_cast<uintptr_t>(candidate);
                if (candAddr < 0x10000 || candAddr > 0x7FFFFFFFFFFF)
                    continue;

                if (!CheckMemory(candAddr))
                    continue;

                if (memcmp(candidate, TypeName.c_str(), nameLen) == 0)
                    return job;

                if (strstr(candidate, TypeName.c_str()) != nullptr)
                    return job;
            }
        }

        return 0;
    }
}
