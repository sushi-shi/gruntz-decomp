#include <rva.h>

#include <Gruntz/BehindCandyAni.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

VTBL(CBehindCandyAni, 0x001e838c);
template<> DATA(0x00245f98)
CActReg CActRegPool<CBehindCandyAni>::s_table(2000, 2010);

RVA(0x00010050, 0x47)
i32 CBehindCandyAni::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x000100c0, 0x1e, ??_GCBehindCandyAni@@UAEPAXI@Z)
RVA_COMPGEN(0x000100f0, 0x44, ??1CBehindCandyAni@@UAE@XZ)

RVA(0x000ad540, 0x1f0)
CBehindCandyAni::CBehindCandyAni(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    if (m_wwdObject->m_animCursor.m_animation == 0) {
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    if (m_object->m_sortKey != 0) {
        m_object->m_sortKey = 0;
        m_object->m_flags |= 0x20000;
    }
    if (m_object->m_layer != 0) {
        if (m_object->m_layer->m_width >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_height >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_animWorker != 0) {
                m_object->m_animWorker->m_flags &= ~6;
                m_object->m_animWorker->m_flags |= 1;
                m_wwdObject->m_flags &= ~0x1000002;
                m_wwdObject->m_flags |= 0x800000;
            }
        }
    }
}

RVA(0x000ad850, 0x102)
void CBehindCandyAni::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CBehindCandyAni>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        (this->*(*((CActRegPool<CBehindCandyAni>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000ad9b0, 0x18d)
void CBehindCandyAni::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CBehindCandyAni>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CBehindCandyAni::AdvanceAnim);
}

RVA(0x000adbb0, 0x17)
i32 CBehindCandyAni::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}
