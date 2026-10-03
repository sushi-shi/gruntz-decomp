#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/MenuSparkle.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MenuSparkleSerial.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>

const i32 g_menuSparkleLo = 1000;

const i32 g_menuSparkleHi = 5000;

template<>
CActReg CActRegPool<CMenuSparkle>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

CMenuSparkle::CMenuSparkle(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("MENU_SPARKLE");
    SwitchAnimationByName("MENU_FORWARD100", 0);
    SET_ANIMATION_ACT("A");
    m_logicRecord->m_sparkleDelay = GetRandom(g_menuSparkleLo, g_menuSparkleHi);
}

void CMenuSparkle::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void RegisterMenuSparkleActions() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CMenuSparkle>::s_table[id] = static_cast<CActHandler>(&CMenuSparkle::AdvanceAnim);
}

i32 CMenuSparkle::SerializeDispatch(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (arc == NULL) {
        return 0;
    }

    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(
        static_cast<CFileMemBase*>(arc),
        mode,
        typeId,
        object
    )
    if (mode != SERIAL_SAVE) {
        if (mode != SERIAL_LOAD) {
            return 1;
        }
        arc->Read(const_cast<i32*>(&g_menuSparkleLo), sizeof(g_menuSparkleLo));
        arc->Read(const_cast<i32*>(&g_menuSparkleHi), sizeof(g_menuSparkleHi));
        return 1;
    }
    arc->Write(&g_menuSparkleLo, sizeof(g_menuSparkleLo));
    arc->Write(&g_menuSparkleHi, sizeof(g_menuSparkleHi));
    return 1;
}

i32 CMenuSparkle::AdvanceAnim() {
    u32 delta = g_frameDelta;
    if (delta >= m_logicRecord->m_sparkleDelay) {
        m_logicRecord->m_sparkleDelay = 0;
    } else {
        m_logicRecord->m_sparkleDelay -= delta;
    }
    if (m_logicRecord->m_sparkleDelay == 0) {
        m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    }
    if (m_wwdObject->m_animationCursor.IsComplete()) {
        CAniAdvanceCursor* anim = &m_wwdObject->m_animationCursor;
        if (anim != NULL) {
            anim->RestartAnimation(1);
        }
        m_ownerLogicRecord->m_timeDelay = GetRandom(g_menuSparkleLo, g_menuSparkleHi);
    }
    return 0;
}
