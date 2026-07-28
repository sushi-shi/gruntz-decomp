#include <Gruntz/ActNameRegistry.h>  // the shared action-name registry archetype
#include <Gruntz/ActReg.h>           // the shared activation-registrar archetype
#include <Gruntz/CursorSnapActReg.h> // CActRegPool<CCursorSnapSprite>::s_table decl
#include <Gruntz/CursorSnapSprite.h>
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor::Advance (the m_38 +0x1a0 cursor)
#include <Rez/FrameClock.h>          // g_engineFrameDelta (the draw-delta mirror)

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
    // ILT 0x401717 -> 0x03a910 == CCursorSnapSprite::AdvanceAnim.
    *CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id) =
        static_cast<CActHandler>(&CCursorSnapSprite::AdvanceAnim);
}

// CCursorSnapSprite::AdvanceAnim @0x03a910 - the act-"A" body: retail is
// `mov eax,[g_engineFrameDelta]; mov ecx,[ecx+0x38]; push eax; add ecx,0x1a0;
// call CAniAdvanceCursor::Advance; xor eax,eax; ret` - the advance result is
// dropped and 0 returned.
RVA(0x0003a910, 0x17)
i32 CCursorSnapSprite::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}
