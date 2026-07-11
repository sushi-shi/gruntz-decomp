// LogicActReg646010.cpp - per-logic-class activation registrar for dispatch table
// 0x646010 (C:\Proj\Gruntz).
//
// Split out of the LogicActReg.cpp holding TU (REHOME D1). Owner NOT cracked: no
// reconstructed class references g_logicActReg_646010, and the pair @0xadde0/0xadfc0
// sits in a scattered leaf-init pool (interleaved with MenuSparkle @0xadbe0 /
// Projectile @0xade60 / MenuSparkleSerial @0xae1c0) rather than one owner obj - so
// this stays named by its dispatch-table address (the structural drain already
// landed). @identity-TODO: pin the owning leaf class (the .data key gives no string
// constant; the 0xadde0-0xadfc0 band is a COMDAT-pooled init run).
//
// Binds ONE game-object-logic class's per-frame activation handler into that class's
// per-class dispatch table, keyed by the shared action name "A" (the
// CKitchenSlime::RegisterType archetype). The body is owner-independent (every global
// is a reloc-masked DATA extern, every callee external/no-body).
#include <Gruntz/ActNameRegistry.h> // the shared action-name registry archetype
#include <Gruntz/ActReg.h>          // the shared activation-registrar archetype
#include <Wap32/ZDArrayDerived.h>   // CZDArrayDerived::Construct (the [lo,hi] range static-init)

// The per-logic-class activation dispatch table (.data, DATA-pinned).
DATA(0x00246010)
extern CLogicActTable g_logicActReg_646010; // 0x646010

// The class activation handler (ILT thunk 0x403c10 -> 0xad2a0).
extern "C" void LogicHandler_0ad2a0();

// The shared name-registry build (action key "A"), CKitchenSlime::RegisterType
// ordering: register the action name on first use, resolve its name-table slot, free
// the slot's old CString nodes, assign the key, bump the global counter; returns the
// (possibly newly-allocated) action id.
static inline i32 RegisterActionName() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        g_buteTree.Insert(s_actKeyA, (void*)g_nextActId);
        i32 key = g_nextActId;
        id = key;
        char* slot = ActNameLookup(key);
        i32 cnt = g_nameRegScratch;
        void** nodes = g_nameRegCurList;
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    ((CString*)nodes)->CString::~CString();
                }
                nodes++;
            } while (--cnt);
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    return id;
}

// ConstructLogicActRange_646010 @0x0adde0 - the static initializer that builds
// dispatch table 0x646010's fast [0x7d0, 0x7da] id range.
RVA(0x000adde0, 0x15)
void ConstructLogicActRange_646010() {
    ((CZDArrayDerived*)&g_logicActReg_646010)->Construct(0x7d0, 0x7da);
}

// RegisterXLogic @0x0adfc0 - bind the logic class behind dispatch table 0x646010 to
// its activation handler. Same archetype/wall as 0x03a710.
// @early-stop
// register-pinning wall (see CursorSnapActReg.cpp): logic byte-faithful, residual is
// the action-id register coloring + count-down induction. Deferred.
RVA(0x000adfc0, 0x18d)
void RegisterXLogic_646010() {
    i32 id = RegisterActionName();
    *(void**)g_logicActReg_646010.ResolveEntry(id) = (void*)&LogicHandler_0ad2a0;
}
