#include <rva.h>

#include <Gruntz/MenuSparkle.h>

#include <Bute/ButeTree.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MenuSparkleSerial.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>

#include <stdlib.h>

RVA_DYNINIT(0x000addc0, 0xa, CActRegPool<CMenuSparkle>::s_table)
RVA_DYNINIT(0x000adde0, 0x15, CActRegPool<CMenuSparkle>::s_table)
RVA_DYNINIT(0x000ade10, 0xe, CActRegPool<CMenuSparkle>::s_table)
RVA_DYNINIT(0x000ade30, 0x1f, CActRegPool<CMenuSparkle>::s_table)
template<> DATA(0x00246010)
CActReg CActRegPool<CMenuSparkle>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x00010180, 0x1e, ??_GCMenuSparkle@@UAEPAXI@Z)
RVA_COMPGEN(0x000101b0, 0x44, ??1CMenuSparkle@@UAE@XZ)

RVA(0x000adbe0, 0x178)
CMenuSparkle::CMenuSparkle(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    ApplyName("MENU_SPARKLE");
    SwitchGeometry("MENU_FORWARD100", 0);
    SET_ANIMATION_ACT("A");
    m_objAux->m_sparkleDelay = rand() % 0xfa1 + 0x3e8;
}

typedef i32 (CUserLogic::*CActHandler)();

RVA(0x000ade60, 0x102)
void CMenuSparkle::FireActivation(i32 coord) {
    CActHandler* e = CActRegPool<CMenuSparkle>::s_table.ResolveEntry(coord);
    if (*e != 0) {
        CActHandler* e2 = CActRegPool<CMenuSparkle>::s_table.ResolveEntry(coord);
        CActHandler h = *e2;
        (this->*h)();
    }
}

RVA(0x000adfc0, 0x18d)
void RegisterMenuSparkleActions() {
    ACT_NAME_ID(id, "A")
    *CActRegPool<CMenuSparkle>::s_table.ResolveEntry(id) =
        static_cast<CActHandler>(&CMenuSparkle::AdvanceAnim);
}

RVA(0x000ae1c0, 0xae)
i32 CMenuSparkle::SerializeMove(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (arc == NULL) {
        return 0;
    }

    if (!CUserLogic::SerializeMove(static_cast<CFileMemBase*>(arc), mode, typeId, pObj)) {
        return 0;
    }
    if (!Chain(static_cast<CFileMemBase*>(arc), mode, typeId, pObj)) {
        return 0;
    }
    if (mode != SERIAL_SAVE) {
        if (mode != SERIAL_LOAD) {
            return 1;
        }
        // Retail's SERIAL_LOAD arm reads INTO storage it placed in read-only
        // `.rdata` (0x5ea3d4/0x5ea3d8, pushed verbatim at 0xae222/0xae230), so
        // the cast is retail's own: the section characteristics prove the const,
        // and this arm would fault if it ever ran.
        arc->Read(const_cast<i32*>(&g_menuSparkleLo), sizeof(g_menuSparkleLo));
        arc->Read(const_cast<i32*>(&g_menuSparkleHi), sizeof(g_menuSparkleHi));
        return 1;
    }
    arc->Write(&g_menuSparkleLo, sizeof(g_menuSparkleLo));
    arc->Write(&g_menuSparkleHi, sizeof(g_menuSparkleHi));
    return 1;
}

// @early-stop
RVA(0x000ae2a0, 0x8e)
i32 CMenuSparkle::AdvanceAnim() {
    u32 delta = g_frameDelta;
    if (delta >= m_objAux->m_sparkleDelay) {
        m_objAux->m_sparkleDelay = 0;
    } else {
        m_objAux->m_sparkleDelay -= delta;
    }
    if (m_objAux->m_sparkleDelay == 0) {
        m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    }
    CAniAdvanceCursor* anim = &m_wwdObject->m_animCursor;
    if (IsAniCursorComplete(anim)) {
        if (anim != NULL) {
            anim->Recompute(1);
        }
        m_animWorker->m_timeDelay = rand() % 0xfa1 + 0x3e8;
    }
    return 0;
}
// Const (retail .rdata): AdvanceAnim's rand() % 0xfa1 + 0x3e8 is the folded
// hi-lo+1 / lo pair, which only a known const value produces.
DATA(0x001ea3d4)
const i32 g_menuSparkleLo = 1000;

DATA(0x001ea3d8)
const i32 g_menuSparkleHi = 5000;
