// oh no no no no
// wait i hate asta.
// i love asta
// easter egg

#pragma once
#include <cstdint>
#include <Windows.h>

namespace Main {
    inline uintptr_t krah() {
        return reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    }

    inline uintptr_t Rebase(uintptr_t offset) {
        return krah() + offset;
    }

    namespace Functions {
        inline const uintptr_t Print = Rebase(0x1cd9d90);
        inline const uintptr_t GetGlobalState = Rebase(0x421A7A0);
        inline const uintptr_t LuauExecute = Rebase(0x277a970);
        inline const uintptr_t LuaDThrow = Rebase(0x2750300);
        inline const uintptr_t OpcodeLookupTable = Rebase(0x6e964d0);
    }

    namespace Miscellaneous {
        inline const uintptr_t FakeDatamodelPOINTER = Rebase(0x8e42c98);
        inline const uintptr_t LuaNil = Rebase(0x6437a18);
        inline const uintptr_t LuaDummy = Rebase(0x6437098);
        inline const uintptr_t TargetFPS = Rebase(0x8227738);
    }

    namespace Offsets {
        inline const uintptr_t DataModel = 0x1f8;
        inline const uintptr_t ScriptContext = 0x440;
        inline const uintptr_t Children = 0x78;
        inline const uintptr_t GameLoaded = 0x638;
    }

    namespace Scheduler {
        inline const uintptr_t TaskScheduler = Rebase(0x8BDD8E8);
        inline const uintptr_t JobStart = 0xC8;
        inline const uintptr_t JobEnd = 0xD0;
        inline const uintptr_t JobTypeName = 0xF8;
    }

    namespace ExtraSpace {
        inline const uintptr_t RequireBypass = 0xA58;
    }

    namespace Identity1 {
        inline const uintptr_t IdentityPointer = Rebase(0x81DDD08);
        inline const uintptr_t GetTlsPointer = Rebase(0x4250);
    }

    namespace Identity2 {
        inline const uintptr_t GetCapabilities = Rebase(0x1D14D50);
        inline const uintptr_t Capabilities = 0x30;
    }
}