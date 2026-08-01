#include <Gruntz/SpriteRefTable.h>
#include <Rez/FrameClock.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GruntCreationPoint.h>
#include <Gruntz/AnimSink.h>
#include <Wap32/ZVec.h>
#include <rva.h>
#include <AddrWord.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Play.h>

VTBL(CGruntCreationPoint, 0x001e81d4);

RVA_COMPGEN(0x00010700, 0x1e, ??_GCGruntCreationPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x00010730, 0x44, ??1CGruntCreationPoint@@UAE@XZ)

// @early-stop
RVA(0x0003e520, 0x1fd)
CGruntCreationPoint::CGruntCreationPoint(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 2;
    if (m_object->m_sortKey != 5) {
        m_object->m_sortKey = 5;
        m_object->m_flags |= 0x20000;
    }
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);

    i32 key = m_object->m_124;
    i32 idx;
    if (g_gameReg->m_134 == 1) {
        idx = key;
    } else if (g_gameReg->m_options[key].m_liveGate != 0) {
        idx = g_gameReg->m_options[key].m_008;
    } else {
        m_38->m_flags |= 0x10000;

        AddrWord sel;
        sel.m_addr = obj;
        idx = sel.m_word;
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);

    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 0xa;
    m_object->m_drawFillArg = sel;
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
}

template<> DATA(0x00244700)
CActReg CActRegPool<CGruntCreationPoint>::s_table(2000, 2010);

RVA(0x0003e7a0, 0xd7)
i32 CGruntCreationPoint::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }
    if (tag != 4 && tag == 8) {
        i32 idx;
        if (g_gameReg->m_134 != 1) {
            if (g_gameReg->m_options[m_object->m_124].m_liveGate != 0) {
                idx = g_gameReg->m_options[m_object->m_124].m_008;
            } else {
                idx = ChannelSlots_FindFree();
            }
        } else {
            idx = m_object->m_124;
        }
        CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
        if (sel == 0) {
            sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
        }
        CWwdGameObjectA* obj = m_object;
        obj->m_drawActive = 1;
        obj->m_drawFillCmd = 0xa;
        obj->m_drawFillArg = sel;
    }
    return 1;
}

RVA(0x0003e960, 0x102)
void CGruntCreationPoint::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CGruntCreationPoint>::s_table.ResolveEntry(coord));
    if (*e != 0) {
        CActHandler* e2 = (CActRegPool<CGruntCreationPoint>::s_table.ResolveEntry(coord));
        (this->*(*e2))();
    }
}

RVA(0x0003eac0, 0x18d)
void CGruntCreationPoint::RegisterActs() {
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
    *(CActRegPool<CGruntCreationPoint>::s_table.ResolveEntry(id)) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntCreationPoint::AdvanceAnim);
}

RVA(0x0003ecc0, 0x17)
i32 CGruntCreationPoint::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}
