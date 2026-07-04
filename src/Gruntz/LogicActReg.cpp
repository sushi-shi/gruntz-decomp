// LogicActReg.cpp - per-logic-class activation registrars (C:\Proj\Gruntz).
//
// Three "RegisterXLogic" companions (each 0x18d) to the file-scope dispatch-table
// initializers in LogicDispatchInit.cpp: each binds ONE game-object-logic class's
// per-frame activation handler into that class's per-class dispatch table, keyed
// by the shared action-name "A" the global bute-tree assigns an id to. The SAME
// archetype as CKitchenSlime::RegisterType (0x0b2aa0) and CProjectile::RegisterType
// (0x0dfb00): ensure the action name is registered once (shared name registry R1
// @0x6bf650), then store the class handler into the per-class table at that id.
//
// The owning leaf class for each table is not string-pinned (the registration
// keys live in .data, not a .rdata constant), so - matching the LogicDispatchInit.cpp
// precedent - each registrar is named by its dispatch-table address. The body is
// owner-independent: every global is a reloc-masked DATA extern and every callee is
// external/no-body, so the byte match holds regardless of the recovered class name.
//
//   0x03a710 -> table 0x62bfa0, handler 0x39910 (thunk 0x401717)
//   0x0474b0 -> table 0x6447f8 (g_logicDispatch_6447f8), handler 0x466b0 (thunk 0x4041ec)
//   0x0adfc0 -> table 0x646010, handler 0xad2a0 (thunk 0x403c10)
#include <Gruntz/ActNameRegistry.h> // the shared action-name registry archetype
#include <Gruntz/ActReg.h>          // the shared activation-registrar archetype

// The per-logic-class activation dispatch table (a zDArray<handler> in .data) is
// the shared <Gruntz/ActReg.h> CLogicActTable alias: same fast-[lo,hi] / slow-Find /
// rebuild resolve as the shared name registry; the resolved entry's first dword
// receives the class's activation handler. Inlined on a fixed-address global so the
// member reads fold to absolute DIR32 loads (the SAME shape as CAniCycleActReg::
// ResolveEntry / KSlimeLookup).

// The shared name-registry build (action key "A"), CKitchenSlime::RegisterType
// ordering: register the action name on first use (g_buteTree maps name->id),
// resolve its name-table slot, free the slot's old CString nodes, assign the key,
// bump the global counter; returns the (possibly newly-allocated) action id.
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
                    ((CActName*)nodes)->Free();
                }
                nodes++;
            } while (--cnt);
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    return id;
}

// The three per-class dispatch tables (.data, DATA-pinned so the loads reloc-mask).
DATA(0x0022bfa0)
extern CLogicActTable g_logicActReg_62bfa0; // 0x62bfa0
DATA(0x002447f8)
extern CLogicActTable g_logicActReg_6447f8; // 0x6447f8 (g_logicDispatch_6447f8)
DATA(0x00246010)
extern CLogicActTable g_logicActReg_646010; // 0x646010

// The three class activation handlers (ILT thunks; referenced by address so the
// entry store emits a reloc-masked DIR32 to the named symbol).
extern "C" void LogicHandler_039910(); // thunk 0x401717 -> 0x39910
extern "C" void LogicHandler_0466b0(); // thunk 0x4041ec -> 0x466b0
extern "C" void LogicHandler_0ad2a0(); // thunk 0x403c10 -> 0xad2a0

// RegisterXLogic @0x03a710 - bind the logic class behind dispatch table 0x62bfa0
// to its activation handler under the shared action name "A".
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md, topic:wall
// topic:regalloc) - the SAME residual CKitchenSlime::RegisterType / CAniCycle::
// RegisterActs carry: logic byte-faithful (every call/immediate/branch/offset +
// the per-class handler store match retail); residual is the action-id callee-saved
// register choice (retail esi vs edi) cascading into the node-count count-down
// induction-variable materialization. Not source-steerable; deferred.
RVA(0x0003a710, 0x18d)
void RegisterXLogic_62bfa0() {
    i32 id = RegisterActionName();
    *(void**)g_logicActReg_62bfa0.ResolveEntry(id) = (void*)&LogicHandler_039910;
}

// RegisterXLogic @0x0474b0 - bind the logic class behind dispatch table 0x6447f8
// (g_logicDispatch_6447f8, constructed by InitLogicDispatch_6447f8 @0x472d0) to its
// activation handler. Same archetype/wall as 0x03a710.
// @early-stop
// register-pinning wall (see 0x03a710): logic byte-faithful, residual is the
// action-id register coloring + count-down induction. Deferred.
RVA(0x000474b0, 0x18d)
void RegisterXLogic_6447f8() {
    i32 id = RegisterActionName();
    *(void**)g_logicActReg_6447f8.ResolveEntry(id) = (void*)&LogicHandler_0466b0;
}

// RegisterXLogic @0x0adfc0 - bind the logic class behind dispatch table 0x646010 to
// its activation handler. Same archetype/wall as 0x03a710.
// @early-stop
// register-pinning wall (see 0x03a710): logic byte-faithful, residual is the
// action-id register coloring + count-down induction. Deferred.
RVA(0x000adfc0, 0x18d)
void RegisterXLogic_646010() {
    i32 id = RegisterActionName();
    *(void**)g_logicActReg_646010.ResolveEntry(id) = (void*)&LogicHandler_0ad2a0;
}
