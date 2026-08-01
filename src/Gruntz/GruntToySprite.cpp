#include <Gruntz/Sprite.h>
#include <Image/CImage.h>
#include <Gruntz/GruntToySprite.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>
#include <Gruntz/SerialArchive.h>
#include <Wap32/ZVec.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/TriggerMgr.h>
#include <rva.h>

VTBL(CGruntToySprite, 0x001e7b4c);

template<> DATA(0x00244d58)
CActReg CActRegPool<CGruntToySprite>::s_table(2000, 2010);
RVA_COMPGEN(0x00012280, 0x1e, ??_GCGruntToySprite@@UAEPAXI@Z)
RVA_COMPGEN(0x000122b0, 0x44, ??1CGruntToySprite@@UAE@XZ)

RVA(0x0007f350, 0x16a)
CGruntToySprite::CGruntToySprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALL", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_38->m_stateFlags |= 1;
    if (m_object->m_sortKey != 0xdbba0) {
        m_object->m_sortKey = 0xdbba0;
        m_object->m_flags |= 0x20000;
    }
    m_lastLayer = 0;
}

RVA(0x0007f5c0, 0x102)
void CGruntToySprite::FireActivation(i32 id) {
    if ((*((CActRegPool<CGruntToySprite>::s_table.ResolveEntry(id)))) != 0) {
        (this->*(*((CActRegPool<CGruntToySprite>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x0007f720, 0x18d)
void CGruntToySprite::RegisterActs() {
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
    (*((CActRegPool<CGruntToySprite>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntToySprite::Update);
}

RVA(0x0007f920, 0x21)
i32 CGruntToySprite::SetCell(i32 x, i32 y) {
    m_cellX = x;
    m_cellY = y;
    m_38->m_stateFlags &= ~1;
    return 1;
}

RVA(0x0007f960, 0x85)
i32 CGruntToySprite::Update() {
    CGrunt* e = g_gameReg->m_cmdGrid->m_grid[m_cellX * 15 + m_cellY];
    if (e == 0) {
        return 0;
    }
    i32 layer = e->m_198;
    if (m_lastLayer != layer) {
        CWwdGameObjectA* r = m_object;
        m_lastLayer = layer;
        CDDrawWorker* h = r->m_sprite;
        if (h != 0) {
            CImage* mapped;
            if (layer >= h->m_minIndex && layer <= h->m_maxIndex) {
                mapped = static_cast<CImage*>(h->m_items.GetAt(layer));
            } else {
                mapped = 0;
            }
            r->m_layer = mapped;
            r->m_190 = layer;
        }
    }
    m_object->m_screenX = e->m_object->m_screenX;
    m_object->m_screenY = e->m_object->m_screenY - 0x20;
    return 0;
}

RVA(0x0007fa20, 0x89)
i32 CGruntToySprite::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    switch (mode) {
        case 4:
            ar->Write(&m_cellX, 8);
            ar->Write(&m_lastLayer, 4);
            break;
        case 7:
            ar->Read(&m_cellX, 8);
            ar->Read(&m_lastLayer, 4);
            break;
    }
    if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}
