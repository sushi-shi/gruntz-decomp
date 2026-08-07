#include <rva.h>

#include <Gruntz/GruntSelectedSprite.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Io/FileMem.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

template<> DATA(0x00244da8)
CActReg CActRegPool<CGruntSelectedSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_COMPGEN(0x00011e50, 0x1e, ??_GCGruntSelectedSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00011e80, 0x44, ??1CGruntSelectedSprite@@UAE@XZ)

// @early-stop
// The vptr stamp is transposed with the body's first m_wwdObject read; the rest is
// a scratch-register rotation.  docs/patterns/vptr-stamp-transposed-with-second-base-member-load.md
RVA(0x0007e3e0, 0x178)
CGruntSelectedSprite::CGruntSelectedSprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->ApplyName("GAME_GRUNTSELECTEDSPRITE");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_GRUNTSELECTEDSPRITE", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_GRUNT_SELECTED) {
        o->m_sortKey = SORTKEY_GRUNT_SELECTED;
        o->m_flags |= 0x20000;
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
            if (list != NULL) {
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
    m_cell.m_x = x;
    m_cell.m_y = y;
    return 1;
}

// @early-stop
RVA(0x0007e9f0, 0x5f)
i32 CGruntSelectedSprite::Update() {
    CGruntzMgr* reg = g_gameReg;
    CGrunt* e = reg->m_cmdGrid->m_grid[m_cell.m_y + m_cell.m_x * 15];
    if (e != NULL && e->m_arrived != 0) {
        m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
        m_object->m_screenX = e->m_object->m_screenX;
        m_object->m_screenY = e->m_object->m_screenY;
    }
    return 0;
}

RVA(0x0007ea70, 0x6f)
i32 CGruntSelectedSprite::SerializeMove(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    CFileMemBase* sa = static_cast<CFileMemBase*>(arc);

    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            sa->Read(&m_cell, sizeof(m_cell));
        }
    } else {
        sa->Write(&m_cell, sizeof(m_cell));
    }
    if (!CUserLogic::SerializeMove(arc, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(sa, mode, typeId, pObj) ? 1 : 0;
}
