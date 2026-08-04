#include <Gruntz/CursorSnapActReg.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/CursorSnapSprite.h>
#include <Rez/FrameClock.h>

#include <stddef.h>

template<> DATA(0x0022bfa0)
CActReg CActRegPool<CCursorSnapSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

static inline i32 RegisterActionName() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 cnt = g_typeColl.m_grown;
        CString* nodes = g_typeColl.Slots();
        while (cnt-- != 0) {
            if (nodes != NULL) {
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
void RegisterCursorSnapActions() {
    i32 id = RegisterActionName();

    *CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id) =
        static_cast<CActHandler>(&CCursorSnapSprite::AdvanceAnim);
}

RVA(0x0003a910, 0x17)
i32 CCursorSnapSprite::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}
