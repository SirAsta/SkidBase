// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#pragma once

#include "lua.h"
#include "lcommon.h"

#include <cstddef> // offsetof, for the client layout guards below

/*
** Union of all collectible objects
*/
typedef union GCObject GCObject;

/*
** Common Header for all collectible objects (in macro form, to be included in other objects)
*/
// clang-format off
#define CommonHeader \
    uint8_t tt; \
    uint8_t memcat; \
    uint8_t marked;
// clang-format on

/*
** Common header in struct form
*/
typedef struct GCheader
{
    CommonHeader;
} GCheader;

/*
** Union of all Lua values
*/
typedef union
{
    GCObject* gc;
    void* p;
    double n;
    int b;
    int64_t l;
    float v[2]; // v[0], v[1] live here; v[2] lives in TValue::extra
} Value;

/*
** Tagged Values
*/

typedef struct lua_TValue
{
    Value value;
    int extra[LUA_EXTRA_SIZE];
    int tt;
} TValue;

#if LUA_VECTOR_SIZE == 4
#define condvector4(vec4expr, vec3expr) vec4expr
#else
#define condvector4(vec4expr, vec3expr) vec3expr
#endif

#if LUA_VECTOR_DOUBLE == 1
#define condvectordouble(vecdoubleexpr, vecfloatexpr) vecdoubleexpr
#else
#define condvectordouble(vecdoubleexpr, vecfloatexpr) vecfloatexpr
#endif

// Macros to test type
#define ttisnil(o) (ttype(o) == LUA_TNIL)
#define ttisnumber(o) (ttype(o) == LUA_TNUMBER)
#define ttisinteger(o) (ttype(o) == LUA_TINTEGER)
#define ttisstring(o) (ttype(o) == LUA_TSTRING)
#define ttistable(o) (ttype(o) == LUA_TTABLE)
#define ttisfunction(o) (ttype(o) == LUA_TFUNCTION)
#define ttisboolean(o) (ttype(o) == LUA_TBOOLEAN)
#define ttisuserdata(o) (ttype(o) == LUA_TUSERDATA)
#define ttisthread(o) (ttype(o) == LUA_TTHREAD)
#define ttisbuffer(o) (ttype(o) == LUA_TBUFFER)
#define ttislightuserdata(o) (ttype(o) == LUA_TLIGHTUSERDATA)
#define ttisvector(o) (ttype(o) == LUA_TVECTOR)
#define ttisupval(o) (ttype(o) == LUA_TUPVAL)
#define ttisclass(o) (ttype(o) == LUA_TCLASS)
#define ttisobject(o) (ttype(o) == LUA_TOBJECT)

// Macros to access values
#define ttype(o) ((o)->tt)
#define gcvalue(o) check_exp(iscollectable(o), (o)->value.gc)
#define pvalue(o) check_exp(ttislightuserdata(o), (o)->value.p)
#define nvalue(o) check_exp(ttisnumber(o), (o)->value.n)
#define lvalue(o) check_exp(ttisinteger(o), (o)->value.l)
#define vvalue(o) check_exp(ttisvector(o), condvectordouble((o)->value.gc->vec.v, (o)->value.v))
#define tsvalue(o) check_exp(ttisstring(o), &(o)->value.gc->ts)
#define uvalue(o) check_exp(ttisuserdata(o), &(o)->value.gc->u)
#define clvalue(o) check_exp(ttisfunction(o), &(o)->value.gc->cl)
#define hvalue(o) check_exp(ttistable(o), &(o)->value.gc->h)
#define bvalue(o) check_exp(ttisboolean(o), (o)->value.b)
#define thvalue(o) check_exp(ttisthread(o), &(o)->value.gc->th)
#define bufvalue(o) check_exp(ttisbuffer(o), &(o)->value.gc->buf)
#define upvalue(o) check_exp(ttisupval(o), &(o)->value.gc->uv)
#define classvalue(o) check_exp(ttisclass(o), &(o)->value.gc->lclass)
#define objectvalue(o) check_exp(ttisobject(o), &(o)->value.gc->lobject)

#define l_isfalse(o) (ttisnil(o) || (ttisboolean(o) && bvalue(o) == 0))

#define lightuserdatatag(o) check_exp(ttislightuserdata(o), (o)->extra[0])

// Internal tags used by the VM
#define LU_TAG_ITERATOR LUA_UTAG_LIMIT

/*
** for internal debug only
*/
#define checkconsistency(obj) LUAU_ASSERT(!iscollectable(obj) || (ttype(obj) == (obj)->value.gc->gch.tt))

#define checkliveness(g, obj) LUAU_ASSERT(!iscollectable(obj) || ((ttype(obj) == (obj)->value.gc->gch.tt) && !isdead(g, (obj)->value.gc)))

// Macros to set values
#define setnilvalue(obj) ((obj)->tt = LUA_TNIL)

#define setnvalue(obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.n = (x); \
        i_o->tt = LUA_TNUMBER; \
    }

#define setlvalue(obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.l = (x); \
        i_o->tt = LUA_TINTEGER; \
    }

#if LUA_VECTOR_DOUBLE == 1
#define setvvalue(L, obj, x, y, z, w) \
    { \
        TValue* i_o = (obj); \
        LuauVector* i_vec = luaVec_newvector((L), double(x), double(y), double(z), condvector4(double(w), 0)); \
        i_o->value.gc = cast_to(GCObject*, i_vec); \
        i_o->tt = LUA_TVECTOR; \
        checkliveness((L)->global, i_o); \
    }
#else
#define setvvalue(L, obj, x, y, z, w) \
    { \
        TValue* i_o = (obj); \
        float* i_v = i_o->value.v; \
        i_v[0] = float(x); \
        i_v[1] = float(y); \
        i_v[2] = float(z); \
        condvector4(i_v[3] = float(w), (void)(w)); \
        i_o->tt = LUA_TVECTOR; \
    }
#endif

#define setpvalue(obj, x, tag) \
    { \
        TValue* i_o = (obj); \
        i_o->value.p = (x); \
        i_o->extra[0] = (tag); \
        i_o->tt = LUA_TLIGHTUSERDATA; \
    }

#define setbvalue(obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.b = (x); \
        i_o->tt = LUA_TBOOLEAN; \
    }

#define setsvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TSTRING; \
        checkliveness(L->global, i_o); \
    }

#define setuvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TUSERDATA; \
        checkliveness(L->global, i_o); \
    }

#define setthvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TTHREAD; \
        checkliveness(L->global, i_o); \
    }

#define setbufvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TBUFFER; \
        checkliveness(L->global, i_o); \
    }

#define setclvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TFUNCTION; \
        checkliveness(L->global, i_o); \
    }

#define sethvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TTABLE; \
        checkliveness(L->global, i_o); \
    }

#define setptvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TPROTO; \
        checkliveness(L->global, i_o); \
    }

#define setupvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TUPVAL; \
        checkliveness(L->global, i_o); \
    }

#define setobj(L, obj1, obj2) \
    { \
        const TValue* o2 = (obj2); \
        TValue* o1 = (obj1); \
        *o1 = *o2; \
        checkliveness(L->global, o1); \
    }

#define setclassvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TCLASS; \
        checkliveness(L->global, i_o); \
    }


#define setobjectvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TOBJECT; \
        checkliveness(L->global, i_o); \
    }

/*
** different types of sets, according to destination
*/

// to stack
#define setobj2s setobj
// from table to same table (no barrier)
#define setobjt2t setobj
// to table (needs barrier)
#define setobj2t setobj
// to new object (no barrier)
#define setobj2n setobj
// to class instance or static member (needs barrier)
#define setobj2class setobj

#define setttype(obj, tt) (ttype(obj) = (tt))

#define iscollectable(o) (ttype(o) >= LUA_TSTRING)

// wrapper for returning userdata properties directly
struct DirectFieldResult
{
    lua_State* L;
    TValue* slot;
};

typedef TValue* StkId; // index to stack elements

/*
** String headers for string table
*/
struct TString {
    CommonHeader; /* offset (0) (0x0) */
    uint16_t atomflag; /* offset (4) (0x4) */
    int16_t atom; /* offset (6) (0x6) */
    TString* next; /* offset (8) (0x8) */
    TSTRING_HASH_ENC<unsigned int> hash; /* offset (16) (0x10) */
    unsigned int len; /* offset (20) (0x14) */
    char data[1]; /* offset (24) (0x18) */
};
static_assert(sizeof(TString) == 32, "sizeof(TString) == 32");


#define getstr(ts) (ts)->data
#define svalue(o) getstr(tsvalue(o))

struct Udata {
    CommonHeader; /* offset (0) (0x0) */
    unsigned char tag; /* offset (3) (0x3) */
    int len; /* offset (4) (0x4) */
    UDATA_META_ENC<struct LuaTable*> metatable; /* offset (8) (0x8) */
    char data[1]; /* offset (16) (0x10) */
};
static_assert(sizeof(Udata) == 24, "sizeof(Udata) == 24");

typedef struct LuauBuffer
{
    CommonHeader;

    unsigned int len;

    alignas(8) char data[1];
} Buffer;

typedef struct LuauVector
{
    CommonHeader;
    // 1 byte padding

    LUA_VECTOR_TYPE v[LUA_VECTOR_SIZE];
} LuauVector;

enum FeedbackVectorSlotKind
{
    CALL_TARGET
};

struct FeedbackVectorSlot
{
    FeedbackVectorSlotKind kind;

    union
    {
        struct
        {
            uint32_t pc;
            uint32_t proto;
            uint32_t hits;
        } call_target;
    };
};

/*
** Function Prototypes
*/
// clang-format off
struct Proto {
    CommonHeader; /* offset (0) (0x0) */
    unsigned char maxstacksize; /* offset (3) (0x3) */
    unsigned char flags; /* offset (4) (0x4) */
    unsigned char is_vararg; /* offset (5) (0x5) */
    unsigned char nups; /* offset (6) (0x6) */
    unsigned char numparams; /* offset (7) (0x7) */
    PROTO_LOCVARS_ENC<struct LocVar*> locvars; /* offset (8) (0x8) */
    PROTO_LINEINFO_ENC<unsigned char*> lineinfo; /* offset (16) (0x10) */
    unsigned int* codeentry; /* offset (24) (0x18) */
    PROTO_DEBUGINSN_ENC<unsigned char*> debuginsn; /* offset (32) (0x20) */
    Proto** p; /* offset (40) (0x28) */
    GCObject* gclist; /* offset (48) (0x30) */
    PROTO_DEBUGNAME_ENC<TString*> debugname; /* offset (56) (0x38) */
    PROTO_ABSLINEINFO_ENC<int*> abslineinfo; /* offset (64) (0x40) */
    void* execdata; /* offset (72) (0x48) */
    uintptr_t exectarget; /* offset (80) (0x50) */
    PROTO_SOURCE_ENC<TString*> source; /* offset (88) (0x58) */
    TValue* k; /* offset (96) (0x60) */
    unsigned int* code; /* offset (104) (0x68) */
    PROTO_TYPEINFO_ENC<unsigned char*> typeinfo; /* offset (112) (0x70) */
    PROTO_UPVALUES_ENC<TString**> upvalues; /* offset (120) (0x78) */
    PROTO_USERDATA_ENC<uint64_t*> userdata; /* offset (128) (0x80) */
    int sizelineinfo; /* offset (136) (0x88) */
    int bytecodeid; /* offset (140) (0x8C) */
    int sizetypeinfo; /* offset (144) (0x90) */
    int linedefined; /* offset (148) (0x94) */
    int sizeupvalues; /* offset (152) (0x98) */
    int sizelocvars; /* offset (156) (0x9C) */
    int sizecode; /* offset (160) (0xA0) */
    int sizep; /* offset (164) (0xA4) */
    int sizek; /* offset (168) (0xA8) */
    int linegaplog2; /* offset (172) (0xAC) */
    struct FeedbackVectorSlot* feedbackvec; /* offset (176) (0xB0) */
    int feedbackvecsize; /* offset (184) (0xB8) */
    int funid; /* offset (188) (0xBC) */
    Proto* optimized; /* offset (192) (0xC0) */
    Proto* deoptimized; /* offset (200) (0xC8) */
    uint64_t cost; /* offset (208) (0xD0) */
};
static_assert(sizeof(Proto) == 216, "sizeof(Proto) == 216");
static_assert(offsetof(Proto, userdata) == 0x80, "Proto::userdata must match the client");
static_assert(offsetof(Proto, code) == 0x68, "Proto::code must match the client");
// clang-format on

typedef struct LocVar
{
    TString* varname;
    int startpc; // first point where variable is active
    int endpc;   // first point where variable is dead
    uint8_t reg; // register slot, relative to base, where variable is stored
} LocVar;

/*
** Upvalues
*/

typedef struct UpVal
{
    CommonHeader;
    uint8_t markedopen; // set if reachable from an alive thread (only valid during atomic)

    // 4 byte padding (x64)

    TValue* v; // points to stack or to its own value
    union
    {
        TValue value; // the value (when closed)
        struct
        {
            // global double linked list (when open)
            struct UpVal* prev;
            struct UpVal* next;

            // thread linked list (when open)
            struct UpVal* threadnext;
        } open;
    } u;
} UpVal;

#define upisopen(up) ((up)->v != &(up)->u.value)

/*
** Closures
*/

struct Closure {
    CommonHeader; /* offset (0) (0x0) */
    unsigned char preload; /* offset (3) (0x3) */
    unsigned char nupvalues; /* offset (4) (0x4) */
    unsigned char stacksize; /* offset (5) (0x5) */
    unsigned char isC; /* offset (6) (0x6) */
    GCObject* gclist; /* offset (8) (0x8) */
    LuaTable* env; /* offset (16) (0x10) */
    union {
        struct {
            lua_CFunction f; /* offset (0) (0x0) */
            CLOSURE_CONT_ENC<lua_Continuation> cont; /* offset (8) (0x8) */
            TString* debugname; /* offset (16) (0x10) */
            TValue upvals[1]; /* offset (24) (0x18) */
        } c;
        struct {
            Proto* p; /* offset (0) (0x0) */
            TValue uprefs[1]; /* offset (8) (0x8) */
        } l;
    };
};
static_assert(sizeof(Closure) == 64, "sizeof(Closure) == 64");

#define iscfunction(o) (ttype(o) == LUA_TFUNCTION && clvalue(o)->isC)
#define isLfunction(o) (ttype(o) == LUA_TFUNCTION && !clvalue(o)->isC)

/*
** Tables
*/

typedef struct TKey
{
    ::Value value;
    int extra[LUA_EXTRA_SIZE];
    unsigned tt : 4;
    int next : 28; // for chaining
} TKey;

typedef struct LuaNode
{
    TValue val;
    TKey key;
} LuaNode;

// copy a value into a key
#define setnodekey(L, node, obj) \
    { \
        LuaNode* n_ = (node); \
        const TValue* i_o = (obj); \
        n_->key.value = i_o->value; \
        memcpy(n_->key.extra, i_o->extra, sizeof(n_->key.extra)); \
        n_->key.tt = i_o->tt; \
        checkliveness(L->global, i_o); \
    }

// copy a value from a key
#define getnodekey(L, obj, node) \
    { \
        TValue* i_o = (obj); \
        const LuaNode* n_ = (node); \
        i_o->value = n_->key.value; \
        memcpy(i_o->extra, n_->key.extra, sizeof(i_o->extra)); \
        i_o->tt = n_->key.tt; \
        checkliveness(L->global, i_o); \
    }

// clang-format off
struct LuaTable {
    CommonHeader; /* offset (0) (0x0) */
    uint8_t tmcache; /* offset (3) (0x3) */
    uint8_t nodemask8; /* offset (4) (0x4) */
    uint8_t safeenv; /* offset (5) (0x5) */
    uint8_t lsizenode; /* offset (6) (0x6) */
    uint8_t readonly; /* offset (7) (0x7) */
    int sizearray; /* offset (8) (0x8) */
    union {
        int lastfree;
        int aboundary;
    }; /* offset (12) (0xC) */
    LuaTable* metatable; /* offset (16) (0x10) */
    GCObject* gclist; /* offset (24) (0x18) */
    LuaNode* node; /* offset (32) (0x20) */
    TValue* array; /* offset (40) (0x28) */
};
static_assert(sizeof(LuaTable) == 48, "sizeof(LuaTable) == 48");
// clang-format on

typedef struct LuauClass
{
    CommonHeader;

    GCObject* gclist;

    TString* name;

    // The superclass of this class. NULL if this class doesn't inherit.
    LuauClass* super;

    // Mapping from offset to static members (only methods for now).
    TValue* staticmembers;

    // Mapping from member name to offset.
    // For static members, subtracting numberofinstancemembers from the offset gives the actual index into staticmembers.
    LuaTable* memberstooffset;

    // Mapping from offset to member name. Instance member offsets are stored before static member offsets.
    TString** offsettomember;

    // Metatable for instances of this class. NULL until the first metamethod
    // is added via luaR_addclassmember.
    LuaTable* instancemetatable;

    // Number of instance members that we expect instances of this class object
    // to have.
    uint32_t numberofinstancemembers;

    // Total number of members that we expect this class object to have between
    // instance and static members.
    //
    // We store this number as an optimization. It's pretty rare that we need
    // to reference the specific number of static members, but it's very common
    // to reference the total number of members (for validating hot paths in
    // the interpreter) and the number of instance members (branching on
    // instance or static members, creating class instances).
    uint32_t numberofallmembers;

    // Can this class be extended?
    bool isopen;

    // True if this class or any of its ancestors defines an __init method.
    // If a class's ancestors define an __init method, it must itself also define an __init method.
    // We cannot determine this statically, so we track it here to error at runtime if the invariant is violated.
    // The default constructor errors if this is true, which works because the default constructor is overridden if a class defines an __init method.
    bool hasuserinitinchain;
} LuauClass;

typedef struct LuauObject
{
    CommonHeader;

    GCObject* gclist;

    // The class object that this value is an instance of.
    LuauClass* lclass;

    // The number of members that this instance contains. We need this in order
    // to free ourselves if we got swept in the same GC cycle as our class
    // pointer.
    uint32_t numberofmembers;

    // The fields of this instance.
    TValue* members;

} LuauObject;

/*
** `module' operation for hashing (size is always a power of 2)
*/
#define lmod(s, size) (check_exp((size & (size - 1)) == 0, (cast_to(int, (s) & ((size) - 1)))))

#define twoto(x) ((int)(1 << (x)))
#define sizenode(t) (twoto((t)->lsizenode))
#define hasmetacache(t) (((t)->readonly & 2) != 0)
#define getmetacache(t, event) ((t)->array - (1 + (event)))

#define luaO_nilobject (&luaO_nilobject_)

LUAI_DATA const TValue luaO_nilobject_;

#define ceillog2(x) (luaO_log2((x) - 1) + 1)

LUAI_FUNC int luaO_log2(unsigned int x);
LUAI_FUNC int luaO_rawequalObj(const TValue* t1, const TValue* t2);
LUAI_FUNC int luaO_rawequalKey(const TKey* t1, const TValue* t2);
LUAI_FUNC int luaO_str2d(const char* s, double* result);
LUAI_FUNC int luaO_str2l(const char* s, int64_t* result, int base = 10);
LUAI_FUNC const char* luaO_pushvfstring(lua_State* L, const char* fmt, va_list argp);
LUAI_FUNC const char* luaO_pushfstring(lua_State* L, const char* fmt, ...);
LUAI_FUNC const char* luaO_chunkid(char* buf, size_t buflen, const char* source, size_t srclen);
