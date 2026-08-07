#include <rva.h>

#include <Gruntz/GruntToySprite.h>

#include <Enums.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

template<> DATA(0x00244d58)
CActReg CActRegPool<CGruntToySprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_COMPGEN(0x00012280, 0x1e, ??_GCGruntToySprite@@UAEPAXI@Z)
RVA_COMPGEN(0x000122b0, 0x44, ??1CGruntToySprite@@UAE@XZ)

// @early-stop
// The vptr stamp is transposed with the body's first m_wwdObject read; the rest is
// a scratch-register rotation.  docs/patterns/vptr-stamp-transposed-with-second-base-member-load.md
RVA(0x0007f350, 0x16a)
CGruntToySprite::CGruntToySprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALL", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_GRUNT_HUD) {
        o->m_sortKey = SORTKEY_GRUNT_HUD;
        o->m_flags |= 0x20000;
    }
    m_lastLayer = PICKUP_NONE;
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
            if (list != NULL) {
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
    m_cell.m_x = x;
    m_cell.m_y = y;
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    return 1;
}

RVA(0x0007f960, 0x85)
i32 CGruntToySprite::Update() {
    CGrunt* e = g_gameReg->m_cmdGrid->m_grid[m_cell.m_x * 15 + m_cell.m_y];
    if (e == NULL) {
        return 0;
    }
    PickupType layer = e->m_vehiclePickupType;
    if (m_lastLayer != layer) {
        CWwdGameObjectA* r = m_object;
        m_lastLayer = layer;
        CDDrawWorker* h = r->m_frameSet;
        if (h != NULL) {
            CImage* mapped;
            i32 layerIndex = IDX(layer);
            if (layerIndex >= h->m_minIndex && layerIndex <= h->m_maxIndex) {
                mapped = static_cast<CImage*>(h->m_items.GetAt(layerIndex));
            } else {
                mapped = NULL;
            }
            r->m_layer = mapped;
            r->m_frameIndex = layerIndex;
        }
    }
    m_object->m_screenX = e->m_object->m_screenX;
    m_object->m_screenY = e->m_object->m_screenY - 0x20;
    return 0;
}

RVA(0x0007fa20, 0x89)
i32 CGruntToySprite::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_cell, sizeof(m_cell));
            ar->Write(&m_lastLayer, sizeof(m_lastLayer));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_cell, sizeof(m_cell));
            ar->Read(&m_lastLayer, sizeof(m_lastLayer));
            break;
    }
    if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}
