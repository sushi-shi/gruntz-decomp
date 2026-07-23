#include <Gruntz/ActNameRegistry.h>  // the shared action-name registry archetype
#include <Gruntz/ActReg.h>           // the shared activation-registrar archetype
#include <Gruntz/CursorSnapActReg.h> // CActRegPool<CCursorSnapSprite>::s_table decl
#include <Gruntz/CursorSnapSprite.h>

// CActRegPool<CCursorSnapSprite>::s_table (0x0022bfa0): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
template<> DATA(0x0022bfa0)
CActReg CActRegPool<CCursorSnapSprite>::s_table(2000, 2010);

static inline i32 RegisterActionName() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        i32 key = g_typeCounter;
        id = key;
        char* slot = ActNameLookup(key);
        i32 cnt = g_typeColl.m_grown;
        void** nodes = reinterpret_cast<void**>(g_typeColl.m_alloc);
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    (reinterpret_cast<CString*>(nodes))->CString::~CString();
                }
                nodes++;
            } while (--cnt);
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    return id;
}

RVA_COMPGEN(0x0003a510, 0xa, _$E238864)
RVA_COMPGEN(0x0003a530, 0x15, _$E238896)
RVA_COMPGEN(0x0003a560, 0xe, _$E238944)
RVA_COMPGEN(0x0003a580, 0x1f, _$E238976)

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
    *reinterpret_cast<void**>(CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id)) =
        static_cast<void*>(&CursorSnapAct);
}
