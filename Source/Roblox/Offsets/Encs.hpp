#pragma once
#include <Windows.h>

template<typename T>
struct VMValue0
{
private:
    T Storage;

public:
    operator const T() const { return Storage; }
    void operator=(const T& Value) { Storage = Value; }
    const T operator->() const { return Storage; }
    T Get() const { return Storage; }
    void Set(const T& Value) { Storage = Value; }
};

template<typename T>
struct VMValue1
{
private:
    T Storage;

public:
    operator const T() const { return (T)((uintptr_t)Storage - (uintptr_t)this); }
    void operator=(const T& Value) { Storage = (T)((uintptr_t)Value + (uintptr_t)this); }
    const T operator->() const { return operator const T(); }
    T Get() const { return operator const T(); }
    void Set(const T& Value) { operator=(Value); }
};

template<typename T>
struct VMValue2
{
private:
    T Storage;

public:
    operator const T() const { return (T)((uintptr_t)this - (uintptr_t)Storage); }
    void operator=(const T& Value) { Storage = (T)((uintptr_t)this - (uintptr_t)Value); }
    const T operator->() const { return operator const T(); }
    T Get() const { return operator const T(); }
    void Set(const T& Value) { operator=(Value); }
};

template<typename T>
struct VMValue3
{
private:
    T Storage;

public:
    operator const T() const { return (T)((uintptr_t)this ^ (uintptr_t)Storage); }
    void operator=(const T& Value) { Storage = (T)((uintptr_t)Value ^ (uintptr_t)this); }
    const T operator->() const { return operator const T(); }
    T Get() const { return operator const T(); }
    void Set(const T& Value) { operator=(Value); }
};

template<typename T>
struct VMValue4
{
private:
    T Storage;

public:
    operator const T() const { return (T)((uintptr_t)this + (uintptr_t)Storage); }
    void operator=(const T& Value) { Storage = (T)((uintptr_t)Value - (uintptr_t)this); }
    const T operator->() const { return operator const T(); }
    T Get() const { return operator const T(); }
    void Set(const T& Value) { operator=(Value); }
};

// 739 pointer-encoding slots: vmvalN maps 1:1 onto VMValueN
template<typename T>
using vmval1 = VMValue1<T>;
template<typename T>
using vmval2 = VMValue2<T>;
template<typename T>
using vmval3 = VMValue3<T>;
template<typename T>
using vmval4 = VMValue4<T>;

#define CLOSURE_CONT_ENC vmval2
#define CLOSURE_DEBUGNAME_DEPRECATED_ENC vmval2
#define LSTATE_STACKSIZE_ENC vmval1
#define PROTO_ABSLINEINFO_ENC vmval2
#define PROTO_DEBUGINSN_ENC vmval4
#define PROTO_DEBUGNAME_ENC vmval4
#define PROTO_LINEINFO_ENC vmval2
#define PROTO_LOCVARS_ENC vmval4
#define PROTO_SOURCE_ENC vmval3
#define PROTO_TYPEINFO_ENC vmval2
#define PROTO_UPVALUES_ENC vmval3
#define PROTO_USERDATA_ENC vmval2
#define TSTRING_HASH_ENC vmval3
#define UDATA_META_ENC vmval3

// dont scroll down































#define proto_lineinfo         PROTO_LINEINFO_ENC
#define proto_debuginsn        PROTO_DEBUGINSN_ENC
#define proto_typeinfo         PROTO_TYPEINFO_ENC
#define proto_abslineinfo      PROTO_ABSLINEINFO_ENC
#define proto_source           PROTO_SOURCE_ENC
#define proto_locvars          PROTO_LOCVARS_ENC
#define proto_upvalues         PROTO_UPVALUES_ENC
#define proto_debugname        PROTO_DEBUGNAME_ENC
#define proto_userdata         PROTO_USERDATA_ENC
#define udata_meta             UDATA_META_ENC
#define closure_debugname      CLOSURE_DEBUGNAME_ENC
#define closure_cont           CLOSURE_CONT_ENC
#define tstring_hash           TSTRING_HASH_ENC
#define lstate_stacksize       LSTATE_STACKSIZE_ENC
