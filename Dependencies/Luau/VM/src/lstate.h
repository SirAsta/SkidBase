// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#pragma once

#include "lobject.h"
#include "ltm.h"
#include "ludata.h"

#include <cstddef> // offsetof, for the client layout guards below

// registry
#define registry(L) (&L->global->registry)

// extra stack space to handle TM calls and some other extras
#define EXTRA_STACK 5

#define BASIC_CI_SIZE 8

#define BASIC_STACK_SIZE (2 * LUA_MINSTACK)

// clang-format off
typedef struct stringtable
{
    TString** hash;
    uint32_t nuse; // number of elements
    int size;
} stringtable;
// clang-format on

/*
** information about a call
**
** the general Lua stack frame structure is as follows:
** - each function gets a stack frame, with function "registers" being stack slots on the frame
** - function arguments are associated with registers 0+
** - function locals and temporaries follow after; usually locals are a consecutive block per scope, and temporaries are allocated after this, but
*this is up to the compiler
**
** when function doesn't have varargs, the stack layout is as follows:
** ^ (func) ^^ [fixed args] [locals + temporaries]
** where ^ is the 'func' pointer in CallInfo struct, and ^^ is the 'base' pointer (which is what registers are relative to)
**
** when function *does* have varargs, the stack layout is more complex - the runtime has to copy the fixed arguments so that the 0+ addressing still
*works as follows:
** ^ (func) [fixed args] [varargs] ^^ [fixed args] [locals + temporaries]
**
** computing the sizes of these individual blocks works as follows:
** - the number of fixed args is always matching the `numparams` in a function's Proto object; runtime adds `nil` during the call execution as
*necessary
** - the number of variadic args can be computed by evaluating (ci->base - ci->func - 1 - numparams)
**
** the CallInfo structures are allocated as an array, with each subsequent call being *appended* to this array (so if f calls g, CallInfo for g
*immediately follows CallInfo for f)
** the `nresults` field in CallInfo is set by the caller to tell the function how many arguments the caller is expecting on the stack after the
*function returns
** the `flags` field in CallInfo contains internal execution flags that are important for pcall/etc, see LUA_CALLINFO_*
*/
// clang-format off
struct CallInfo {
    TValue* top; /* offset (0) (0x0) */
    TValue* func; /* offset (8) (0x8) */
    Proto* p; /* offset (16) (0x10) */
    TValue* base; /* offset (24) (0x18) */
    union {
        const Instruction* savedpc;
        int errfunc;
    }; /* offset (32) (0x20) */
    int nresults; /* offset (40) (0x28) */
    unsigned int flags; /* offset (44) (0x2C) */
};
static_assert(sizeof(CallInfo) == 48, "sizeof(CallInfo) == 48");
// clang-format on

#define LUA_CALLINFO_RETURN (1 << 0) // should the interpreter return after returning from this callinfo? first frame must have this set
#define LUA_CALLINFO_HANDLE (1 << 1) // should the error thrown during execution get handled by continuation from this callinfo? func must be C
#define LUA_CALLINFO_NATIVE (1 << 2) // should this function be executed using execution callback for native code
#define LUA_CALLINFO_OPYIELD (1 << 3) // call frame has yielded on a non-call opcode and requires luau_finishop
#define LUA_CALLINFO_PCALL (1 << 4) // call frame was setup by a synthetic protected call and requires luau_pospcallsuccess

#define curr_func(L) (clvalue(L->ci->func))
#define ci_func(ci) (clvalue((ci)->func))
#define f_isLua(ci) (!ci_func(ci)->isC)
#define isLua(ci) (ttisfunction((ci)->func) && f_isLua(ci))

struct GCStats
{
    // data for proportional-integral controller of heap trigger value
    int32_t triggerterms[32] = {0};
    uint32_t triggertermpos = 0;
    int32_t triggerintegral = 0;

    size_t atomicstarttotalsizebytes = 0;
    size_t endtotalsizebytes = 0;
    size_t heapgoalsizebytes = 0;

    double starttimestamp = 0;
    double atomicstarttimestamp = 0;
    double endtimestamp = 0;
};

#ifdef LUAI_GCMETRICS
struct GCCycleMetrics
{
    size_t starttotalsizebytes = 0;
    size_t heaptriggersizebytes = 0;

    double pausetime = 0.0; // time from end of the last cycle to the start of a new one

    double starttimestamp = 0.0;
    double endtimestamp = 0.0;

    double marktime = 0.0;
    double markassisttime = 0.0;
    double markmaxexplicittime = 0.0;
    size_t markexplicitsteps = 0;
    size_t markwork = 0;

    double atomicstarttimestamp = 0.0;
    size_t atomicstarttotalsizebytes = 0;
    double atomictime = 0.0;

    // specific atomic stage parts
    double atomictimeupval = 0.0;
    double atomictimeweak = 0.0;
    double atomictimegray = 0.0;
    double atomictimeembedder = 0.0;
    double atomictimeclear = 0.0;

    double sweeptime = 0.0;
    double sweepassisttime = 0.0;
    double sweepmaxexplicittime = 0.0;
    size_t sweepexplicitsteps = 0;
    size_t sweepwork = 0;

    size_t assistwork = 0;
    size_t explicitwork = 0;

    size_t propagatework = 0;
    size_t propagateagainwork = 0;

    size_t endtotalsizebytes = 0;
};

struct GCMetrics
{
    double stepexplicittimeacc = 0.0;
    double stepassisttimeacc = 0.0;

    // when cycle is completed, last cycle values are updated
    uint64_t completedcycles = 0;

    GCCycleMetrics lastcycle;
    GCCycleMetrics currcycle;
};
#endif

// Callbacks that can be used to to redirect code execution from Luau bytecode VM to a custom implementation (AoT/JiT/sandboxing/...)
struct lua_ExecutionCallbacks
{
    void* context;
    void (*close)(lua_State* L);                 // called when global VM state is closed
    void (*destroy)(lua_State* L, Proto* proto); // called when function is destroyed
    int (*enter)(lua_State* L, Proto* proto);    // called when function is about to start/resume (when execdata is present), return 0 to exit VM
    void (*disable)(lua_State* L, Proto* proto); // called when function has to be switched from native to bytecode in the debugger
    size_t (*getmemorysize)(lua_State* L, Proto* proto); // called to request the size of memory associated with native part of the Proto
    uint8_t (*gettypemapping)(lua_State* L, const char* str, size_t len); // called to get the userdata type index
    char* (*getcounterdata)(
        lua_State* L,
        Proto* proto,
        size_t* count
    ); // called to get the execution counter data and count {uint32_t, uint32_t, uint64_t}
    Proto* (*inlinefunction)(lua_State* L, Closure* caller, Closure* target, uint32_t pc); // called when inlining threshold is reached
};

struct lua_UdataDirectAccessData
{
    TValue indextm;
    TValue newindextm;
    TValue namecalltm;
    lua_UserdataDirectAccess index;
    lua_UserdataDirectAccess newindex;
    lua_UserdataDirectNamecall namecall;
};

/*
** `global state', shared by all threads of this state
*/
struct registryfree_value
{
private:
    int _value;

public:
    registryfree_value() : _value(0) {}
    registryfree_value(int val) : _value(val) {}
    registryfree_value(const registryfree_value& other) : _value(other._value) {}

    void operator=(const registryfree_value& value)
    {
        _value = value._value;
    }

    void operator=(const int& value)
    {
        _value = (_value & 0xF0000000) | (value & 0xFFFFFFF);
    }

    operator const int() const
    {
        return _value & 0xFFFFFFF;
    }

    bool operator==(const int& value) const
    {
        return (_value & 0xFFFFFFF) == value;
    }

    bool operator!=(const int& value) const
    {
        return (_value & 0xFFFFFFF) != value;
    }
};

typedef registryfree_value registryfree_t;

// clang-format off
struct global_State {
    stringtable strt; /* offset (0) (0x0) */
    lua_Alloc frealloc; /* offset (16) (0x10) */
    void* ud; /* offset (24) (0x18) */
    GCObject* weak; /* offset (32) (0x20) */
    GCObject* grayagain; /* offset (40) (0x28) */
    GCObject* gray; /* offset (48) (0x30) */
    size_t GCthreshold; /* offset (56) (0x38) */
    size_t totalbytes; /* offset (64) (0x40) */
    unsigned char currentwhite; /* offset (72) (0x48) */
    unsigned char gcstate; /* offset (73) (0x49) */
    char gap_0[0x2];
    int gcstepsize; /* offset (76) (0x4C) */
    int gcstepmul; /* offset (80) (0x50) */
    int gcgoal; /* offset (84) (0x54) */
    struct lua_Page* allpages; /* offset (88) (0x58) */
    UpVal uvhead; /* offset (96) (0x60) */
    lua_Page* sweepgcopage; /* offset (136) (0x88) */
    lua_State* mainthread; /* offset (144) (0x90) */
    lua_Page* freepages[40]; /* offset (152) (0x98) */
    lua_Page* allgcopages; /* offset (472) (0x1D8) */
    lua_Page* freegcopages[40]; /* offset (480) (0x1E0) */
    TString* tmname[TM_N]; /* offset (800) (0x320) */
    TString* ttname[LUA_T_COUNT]; /* offset (968) (0x3C8) */
    LuaTable* mt[LUA_T_COUNT]; /* offset (1080) (0x438) */
    lua_Page* freegcopages_cage[40]; /* offset (1192) (0x4A8) */
    struct lua_jmpbuf* errorjmp; /* offset (1512) (0x5E8) */
    lua_Page* sweepgcopage_cage; /* offset (1520) (0x5F0) */
    lua_Page* allgcopages_cage; /* offset (1528) (0x5F8) */
    lua_CageAlloc cagealloc; /* offset (1536) (0x600) */
    TValue pseudotemp; /* offset (1544) (0x608) */
    TValue registry; /* offset (1560) (0x618) */
    registryfree_t registryfree; /* offset (1576) (0x628) */
    char gap_1[0x4];
    void* cageud; /* offset (1584) (0x630) */
    unsigned __int64 ptrenckey[4]; /* offset (1592) (0x638) */
    unsigned __int64 rngstate; /* offset (1624) (0x658) */
    lua_Callbacks cb; /* offset (1632) (0x660) */
    lua_ExecutionCallbacks ecb; /* offset (1744) (0x6D0) */
    char gap_2[0x8];
    alignas(16) uint8_t ecbdata[LUA_EXECUTION_CALLBACK_STORAGE]; /* offset (1824) (0x720) */
    lua_UdataDirectAccessData udatadirect[UTAG_INTERNAL_LIMIT]; /* offset (2336) (0x920) */
    size_t memcatbytes[LUA_MEMORY_CATEGORIES]; /* offset (11696) (0x2DB0) */
    void (*udatagc[LUA_UTAG_LIMIT])(struct lua_State*, void*); /* offset (13744) (0x35B0) */
    lua_UserdataMark udatamark[LUA_UTAG_LIMIT]; /* offset (14768) (0x39B0) */
    LuaTable* udatamt[LUA_UTAG_LIMIT]; /* offset (15792) (0x3DB0) */
    TValue weakregistry; /* offset (16816) (0x41B0) */
    int weakregistryfree; /* offset (16832) (0x41C0) */
    int weakregistrytop; /* offset (16836) (0x41C4) */
    lua_EmbedderGc embeddergc; /* offset (16840) (0x41C8) */
    TString* lightuserdataname[LUA_LUTAG_LIMIT]; /* offset (16848) (0x41D0) */
    struct LuaTable* udatadirectfields[UTAG_INTERNAL_LIMIT]; /* offset (17872) (0x45D0) */
    Closure* builtinPcall; /* offset (18912) (0x49E0) */
    Closure* builtinXpcall; /* offset (18920) (0x49E8) */
    unsigned __int64 ptrenckeynew[8]; /* offset (18928) (0x49F0) */
    bool ptrencactive; /* offset (18992) (0x4A30) */
    char gap_3[0x7];
    struct GCStats gcstats; /* offset (19000) (0x4A38) */
    unsigned int lastprotoid; /* offset (19184) (0x4AF0) */
    char gap_4[0x4];
#ifdef LUAI_GCMETRICS
    GCMetrics gcmetrics;
#endif
};
static_assert(sizeof(global_State) == 19200, "sizeof(global_State) == 19200");
// clang-format on

/*
** `per thread' state
*/
// clang-format off
struct lua_State {
    CommonHeader; /* offset (0) (0x0) */
    uint8_t status; /* offset (3) (0x3) */
    bool singlestep; /* offset (4) (0x4) */
    bool isactive; /* offset (5) (0x5) */
    uint8_t activememcat; /* offset (6) (0x6) */
    char gap_0[0x1];
    UpVal* openupval; /* offset (8) (0x8) */
    LuaTable* finalizers; /* offset (16) (0x10) */
    LSTATE_STACKSIZE_ENC<int> stacksize; /* offset (24) (0x18) */
    int size_ci; /* offset (28) (0x1C) */
    LuaTable* gt; /* offset (32) (0x20) */
    struct RobloxExtraSpace* userdata; /* offset (40) (0x28) */
    unsigned short nCcalls; /* offset (48) (0x30) */
    unsigned short baseCcalls; /* offset (50) (0x32) */
    unsigned int cachedslot; /* offset (52) (0x34) */
    GCObject* gclist; /* offset (56) (0x38) */
    CallInfo* ci; /* offset (64) (0x40) */
    global_State* global; /* offset (72) (0x48) */
    TValue* stack; /* offset (80) (0x50) */
    TValue* top; /* offset (88) (0x58) */
    TValue* base; /* offset (96) (0x60) */
    TValue* stack_last; /* offset (104) (0x68) */
    CallInfo* end_ci; /* offset (112) (0x70) */
    CallInfo* base_ci; /* offset (120) (0x78) */
    TString* namecall; /* offset (128) (0x80) */
};
static_assert(sizeof(lua_State) == 136, "sizeof(lua_State) == 136");
static_assert(offsetof(lua_State, stacksize) == 0x18, "lua_State::stacksize must match the client");
static_assert(offsetof(lua_State, userdata) == 0x28, "lua_State::userdata must match the client");
// clang-format on

/*
** Union of all collectible objects
*/
union GCObject
{
    GCheader gch;
    struct TString ts;
    struct Udata u;
    struct Closure cl;
    struct LuaTable h;
    struct Proto p;
    struct UpVal uv;
    struct lua_State th; // thread
    struct LuauBuffer buf;
    struct LuauClass lclass;
    struct LuauObject lobject;
    struct LuauVector vec;
};

// macros to convert a GCObject into a specific value
#define gco2ts(o) check_exp((o)->gch.tt == LUA_TSTRING, &((o)->ts))
#define gco2u(o) check_exp((o)->gch.tt == LUA_TUSERDATA, &((o)->u))
#define gco2cl(o) check_exp((o)->gch.tt == LUA_TFUNCTION, &((o)->cl))
#define gco2h(o) check_exp((o)->gch.tt == LUA_TTABLE, &((o)->h))
#define gco2p(o) check_exp((o)->gch.tt == LUA_TPROTO, &((o)->p))
#define gco2uv(o) check_exp((o)->gch.tt == LUA_TUPVAL, &((o)->uv))
#define gco2th(o) check_exp((o)->gch.tt == LUA_TTHREAD, &((o)->th))
#define gco2buf(o) check_exp((o)->gch.tt == LUA_TBUFFER, &((o)->buf))
#define gco2class(o) check_exp((o)->gch.tt == LUA_TCLASS, &((o)->lclass))
#define gco2object(o) check_exp((o)->gch.tt == LUA_TOBJECT, &((o)->lobject))
#define gco2vec(o) check_exp((o)->gch.tt == LUA_TVECTOR, &((o)->vec))

// macro to convert any Lua object into a GCObject
#define obj2gco(v) check_exp(iscollectable(v), cast_to(GCObject*, (v) + 0))

LUAI_FUNC lua_State* luaE_newthread(lua_State* L);
LUAI_FUNC void luaE_freethread(lua_State* L, lua_State* L1, struct lua_Page* page);
