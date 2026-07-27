#include <Gruntz/ActNameRegistry.h>  // the shared action-name registry archetype
#include <Gruntz/ActReg.h>           // the shared activation-registrar archetype
#include <Gruntz/CursorSnapActReg.h> // CActRegPool<CCursorSnapSprite>::s_table decl
#include <Gruntz/CursorSnapSprite.h>

// CActRegPool<CCursorSnapSprite>::s_table (0x0022bfa0): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
template<> DATA(0x0022bfa0)
CActReg CActRegPool<CCursorSnapSprite>::s_table(2000, 2010);

static inline i32 RegisterActionName() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 cnt = g_typeColl.m_grown;
        CString* nodes = g_typeColl.Slots();
        while (cnt-- != 0) {
            if (nodes != 0) {
                nodes->CString::~CString();
            }
            nodes++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    return id;
}

// RegisterXLogic @0x03a710 - bind CCursorSnapSprite's logic to its activation handler
// under the shared action name "A".
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
RVA(0x0003a710, 0x18d)
void RegisterXLogic_62bfa0() {
    i32 id = RegisterActionName();
    // @identity-TODO a free `void()` registrant into a member-fn-ptr slot
    *reinterpret_cast<void**>(CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id)) = static_cast<void*>(&CursorSnapAct);
}
