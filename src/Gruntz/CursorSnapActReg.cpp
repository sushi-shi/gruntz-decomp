#include <Gruntz/CursorSnapActReg.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/CursorSnapSprite.h>
#include <Rez/FrameClock.h>

#include <stddef.h>

RVA_DYNINIT(0x0003a430, 0xa, CActRegPool<CCursorSnapSprite>::s_table)
RVA_DYNINIT(0x0003a450, 0x15, CActRegPool<CCursorSnapSprite>::s_table)
RVA_DYNINIT(0x0003a480, 0xe, CActRegPool<CCursorSnapSprite>::s_table)
RVA_DYNINIT(0x0003a4a0, 0x1f, CActRegPool<CCursorSnapSprite>::s_table)
template<> DATA(0x0022cef8)
CActReg CActRegPool<CCursorSnapSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x0003a630, 0x18d)
void RegisterCursorSnapActions() {
    ACT_NAME_ID(id, "A")

    *CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id) =
        static_cast<CActHandler>(&CCursorSnapSprite::AdvanceAnim);
}

RVA(0x0003a830, 0x17)
i32 CCursorSnapSprite::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
