#include <StdAfx.h>

#include <Gruntz/CursorSnapActReg.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/CursorSnapSprite.h>
#include <Rez/FrameClock.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CCursorSnapSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

void RegisterCursorSnapActions() {
    ACT_NAME_ID(id, "A")

    CActRegPool<CCursorSnapSprite>::s_table[id] =
        static_cast<CActHandler>(&CCursorSnapSprite::AdvanceAnim);
}

i32 CCursorSnapSprite::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
