#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/CursorSnapActReg.h>
#include <Gruntz/CursorSnapSprite.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Rez/FrameClock.h>

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

RVA(0x0003a710, 0x18d)
void RegisterXLogic_62bfa0() {
    i32 id = RegisterActionName();

    *CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id) =
        static_cast<CActHandler>(&CCursorSnapSprite::AdvanceAnim);
}

RVA(0x0003a910, 0x17)
i32 CCursorSnapSprite::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}
