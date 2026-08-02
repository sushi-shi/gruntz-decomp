#include <Gruntz/SimpleAnimation.h>
#include <Rez/FrameClock.h>
#include <Image/CImage.h>
#include <Wap32/zBitVec.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/AniAdvanceCursor.h>

#include <Bute/ButeMgr.h>
#include <Mfc.h>
#include <Wap32/ZVec.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/SerialArchive.h>
#include <rva.h>
#include <rva.h>
#include <Gruntz/SerialArchive.h>

RVA(0x0000f930, 0x47)
i32 CSimpleAnimation::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x0000f9a0, 0x1e, ??_GCSimpleAnimation@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f9d0, 0x44, ??1CSimpleAnimation@@UAE@XZ)

VTBL(CSimpleAnimation, 0x001e8544);
template<> DATA(0x00246038)
CActReg CActRegPool<CSimpleAnimation>::s_table(2000, 2010);

static inline CString* ResolveNameSlot(CTypeCollRuntime* v, i32 idx) {
    CString* r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->Elem(idx);
    } else if (v->GrowTo(idx, 0)) {
        r = v->Elem(idx);
    } else {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetRetAddr();
        v->m_errSink->Set(v, msg, 0xc);
        r = v->Scratch();
    }
    CString* slot = v->Slots();
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

RVA(0x000ab940, 0x1b8)
CSimpleAnimation::CSimpleAnimation(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    CImage* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_width >= g_buteMgr.GetInt("World", "BigActHeight")
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

RVA(0x000abc10, 0x102)
void CSimpleAnimation::FireActivation(i32 idx) {
    if (*CActRegPool<CSimpleAnimation>::s_table.ResolveEntry(idx) != 0) {
        CActHandler fn = *CActRegPool<CSimpleAnimation>::s_table.ResolveEntry(idx);
        (this->*fn)();
    }
}

RVA(0x000abd70, 0x18d)
void RegisterSimpleAnimLogic() {
    i32 idx = ActFindId("A");
    if (idx == 0) {
        ActInsertId("A", g_typeCounter);
        idx = g_typeCounter;
        CString* slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *slot = "A";
        g_typeCounter++;
    }
    CActHandler* dslot = CActRegPool<CSimpleAnimation>::s_table.ResolveEntry(idx);
    *dslot = static_cast<CActHandler>(&CSimpleAnimation::AdvanceAnim);
}

RVA(0x000abf70, 0x17)
i32 CSimpleAnimation::AdvanceAnim() {
    m_wwdObject->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}
