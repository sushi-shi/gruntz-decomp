#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialArchive.h>
#include <Wap32/ZVec.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/TriggerMgr.h>
#include <rva.h>

VTBL(CGruntSelectedSprite, 0x001e7bfc);

template<> DATA(0x00244da8)
CActReg CActRegPool<CGruntSelectedSprite>::s_table(2000, 2010);
RVA_COMPGEN(0x00011e50, 0x1e, ??_GCGruntSelectedSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00011e80, 0x44, ??1CGruntSelectedSprite@@UAE@XZ)

RVA(0x0007e3e0, 0x178)
CGruntSelectedSprite::CGruntSelectedSprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyName("GAME_GRUNTSELECTEDSPRITE");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_GRUNTSELECTEDSPRITE", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    if (m_object->m_sortKey != 0x14) {
        m_object->m_sortKey = 0x14;
        m_object->m_flags |= 0x20000;
    }
}

RVA(0x0007e660, 0x102)
void CGruntSelectedSprite::FireActivation(i32 id) {
    if ((*((CActRegPool<CGruntSelectedSprite>::s_table.ResolveEntry(id)))) != 0) {
        (this->*(*((CActRegPool<CGruntSelectedSprite>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x0007e7c0, 0x18d)
void CGruntSelectedSprite::RegisterActs() {
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
    (*((CActRegPool<CGruntSelectedSprite>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntSelectedSprite::Update);
}

RVA(0x0007e9c0, 0x16)
i32 CGruntSelectedSprite::SetCell(i32 x, i32 y) {
    m_cellX = x;
    m_cellY = y;
    return 1;
}

// @early-stop
RVA(0x0007e9f0, 0x5f)
i32 CGruntSelectedSprite::Update() {
    CGruntzMgr* reg = g_gameReg;
    CGrunt* e = reg->m_cmdGrid->m_grid[m_cellX * 15 + m_cellY];
    if (e != 0 && e->m_arrived != 0) {
        m_38->m_1a0.Advance(g_engineFrameDelta);
        m_object->m_screenX = e->m_object->m_screenX;
        m_object->m_screenY = e->m_object->m_screenY;
    }
    return 0;
}

RVA(0x0007ea70, 0x6f)
i32 CGruntSelectedSprite::SerializeMove(
    CFileMemBase* arc,
    i32 mode,
    i32 typeId,
    CGameObject* pObj
) {
    CFileMemBase* sa = static_cast<CFileMemBase*>(arc);

    if (mode != 4) {
        if (mode == 7) {
            sa->Read(&m_cellX, 8);
        }
    } else {
        sa->Write(&m_cellX, 8);
    }
    if (!CUserLogic::SerializeMove(arc, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(sa, mode, typeId, pObj) ? 1 : 0;
}
