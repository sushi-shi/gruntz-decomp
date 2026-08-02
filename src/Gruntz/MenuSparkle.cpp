

#include <Gruntz/MenuSparkle.h>
#include <Rez/FrameClock.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Bute/ButeTree.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <stdlib.h>
#include <rva.h>
#include <rva.h>

template<> DATA(0x00246010)
CActReg CActRegPool<CMenuSparkle>::s_table(2000, 2010);

RVA_COMPGEN(0x00010180, 0x1e, ??_GCMenuSparkle@@UAEPAXI@Z)
RVA_COMPGEN(0x000101b0, 0x44, ??1CMenuSparkle@@UAE@XZ)

RVA(0x000adbe0, 0x178)
CMenuSparkle::CMenuSparkle(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->ApplyName("MENU_SPARKLE");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("MENU_FORWARD100", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_objAux->m_sparkleDelay = rand() % 0xfa1 + 0x3e8;
}

VTBL(CMenuSparkle, 0x001e82dc);

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
    i32 id = RegisterActionName();
    *CActRegPool<CMenuSparkle>::s_table.ResolveEntry(id) =
        static_cast<CActHandler>(&CMenuSparkle::AdvanceAnim);
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
    i32 active = m_wwdObject->m_animCursor.m_finished;
    if (active != 0 && anim->m_frameTicksLeft == 0) {
        if (anim != 0) {
            anim->Recompute(1);
        }
        m_animWorker->m_timeDelay = rand() % 0xfa1 + 0x3e8;
    }
    return 0;
}
