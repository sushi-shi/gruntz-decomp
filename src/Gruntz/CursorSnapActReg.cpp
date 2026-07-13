// CursorSnapActReg.cpp - CCursorSnapSprite's per-class logic activation registrar
// (C:\Proj\Gruntz).
//
// Split out of the LogicActReg.cpp holding TU (REHOME D1). Owner CRACKED: the
// dispatch table @0x62bfa0 is CCursorSnapSprite's activation-dispatch table
// (g_logicActReg_62bfa0; CursorSnapSprite.cpp / CursorSnapSprite.h resolve it via
// ResolveEntry). The Construct+Register pair @0x3a530/0x3a710 is text-contained in
// CCursorSnapSprite's obj (interleaved between CursorSnapSprite.cpp's 0x3a340 and
// 0x3a5b0 methods) - a deeper pass may FOLD it into CursorSnapSprite.cpp; kept
// standalone here (owner-TU completeness + parallel-worker scope).
//
// Binds ONE game-object-logic class's per-frame activation handler into that class's
// per-class dispatch table, keyed by the shared action name "A" the global bute-tree
// assigns an id to (the CKitchenSlime::RegisterType archetype). The body is
// owner-independent (every global is a reloc-masked DATA extern, every callee
// external/no-body), so the byte match holds regardless.
#include <Gruntz/ActNameRegistry.h> // the shared action-name registry archetype
#include <Gruntz/ActReg.h>          // the shared activation-registrar archetype
#include <Wap32/ZDArrayDerived.h>   // CZDArrayDerived::Construct (the [lo,hi] range static-init)

// CCursorSnapSprite's per-logic-class activation dispatch table (.data, DATA-pinned).
DATA(0x0022bfa0)
CLogicActTable g_logicActReg_62bfa0; // 0x62bfa0

// The class activation handler (ILT thunk 0x401717 -> 0x39910).
extern "C" void LogicHandler_039910();

// The shared name-registry build (action key "A"), CKitchenSlime::RegisterType
// ordering: register the action name on first use, resolve its name-table slot, free
// the slot's old CString nodes, assign the key, bump the global counter; returns the
// (possibly newly-allocated) action id.
static inline i32 RegisterActionName() {
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        g_buteTree.Insert(s_codeA, (void*)g_typeCounter);
        i32 key = g_typeCounter;
        id = key;
        char* slot = ActNameLookup(key);
        i32 cnt = g_typeColl.m_grown;
        void** nodes = (void**)g_typeColl.m_alloc;
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    ((CString*)nodes)->CString::~CString();
                }
                nodes++;
            } while (--cnt);
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    return id;
}

// ConstructLogicActRange_62bfa0 @0x03a530 - the static initializer that builds
// dispatch table 0x62bfa0's fast [0x7d0, 0x7da] id range (CZDArrayDerived::Construct).
RVA(0x0003a530, 0x15)
void ConstructLogicActRange_62bfa0() {
    ((CZDArrayDerived*)&g_logicActReg_62bfa0)->Construct(0x7d0, 0x7da);
}

// RegisterXLogic @0x03a710 - bind CCursorSnapSprite's logic to its activation handler
// under the shared action name "A".
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md, topic:wall
// topic:regalloc) - the SAME residual CKitchenSlime::RegisterType / CAniCycle::
// RegisterActs carry: logic byte-faithful (every call/immediate/branch/offset + the
// per-class handler store match retail); residual is the action-id callee-saved
// register choice (retail esi vs edi) cascading into the node-count count-down
// induction-variable materialization. Not source-steerable; deferred.
RVA(0x0003a710, 0x18d)
void RegisterXLogic_62bfa0() {
    i32 id = RegisterActionName();
    *(void**)g_logicActReg_62bfa0.ResolveEntry(id) = (void*)&LogicHandler_039910;
}
