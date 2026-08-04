#include <rva.h>

#include <Gruntz/GruntHealthSprite.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

VTBL(CGruntHealthSprite, 0x001e7ba4);

RVA(0x00011ef0, 0x4b)
CGruntHealthSprite::CGruntHealthSprite() {}

template<> DATA(0x00244d80)
CActReg CActRegPool<CGruntHealthSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_COMPGEN(0x00011f80, 0x1e, ??_GCGruntHealthSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00011fb0, 0x44, ??1CGruntHealthSprite@@UAE@XZ)

RVA(0x0007eb00, 0x170)
CGruntHealthSprite::CGruntHealthSprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->ApplyLookupSprite("GAME_GRUNTHEALTHSPRITE", 1);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_health = 0x64;
    if (m_object->m_sortKey != SORTKEY_GRUNT_HUD) {
        m_object->m_sortKey = SORTKEY_GRUNT_HUD;
        m_object->m_flags |= 0x20000;
    }
    m_yOffset = -0x19;
}

RVA(0x0007ed70, 0x102)
void CGruntHealthSprite::FireActivation(i32 id) {
    if ((*((CActRegPool<CGruntHealthSprite>::s_table.ResolveEntry(id)))) != 0) {
        (this->*(*((CActRegPool<CGruntHealthSprite>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x0007eed0, 0x18d)
void CGruntHealthSprite::RegisterActs() {
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
    (*((CActRegPool<CGruntHealthSprite>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntHealthSprite::HealthUpdate);
}

RVA(0x0007f0d0, 0x6e)
i32 CGruntHealthSprite::SetHealthGlyph(i32 x, i32 y, i32 health) {
    m_cell.m_x = x;
    m_cell.m_y = y;
    i32 slot = 0x15 - static_cast<i32>((static_cast<double>(health) * 0.2 + 0.5));
    CWwdGameObjectA* obj = m_object;
    CDDrawWorker* map = obj->m_frameSet;
    if (map) {
        CImage* glyph;
        if (slot >= map->m_minIndex && slot <= map->m_maxIndex) {
            glyph = static_cast<CImage*>(map->m_items.GetAt(slot));
        } else {
            glyph = NULL;
        }
        obj->m_layer = glyph;
        obj->m_frameIndex = slot;
    }
    m_health = health;
    return 1;
}

RVA(0x0007f160, 0xd)
i32 CGruntHealthSprite::GetDisplayedValue(CGrunt* g) {
    return g->m_health;
}

RVA(0x0007f180, 0xb4)
i32 CGruntHealthSprite::HealthUpdate() {

    CGrunt* e = g_gameReg->m_cmdGrid->m_grid[m_cell.m_x * 15 + m_cell.m_y];
    if (e == NULL) {
        return 0;
    }
    i32 result = GetDisplayedValue(e);
    if (m_health != result) {
        i32 slot = 0x15 - static_cast<i32>((static_cast<double>(result) * 0.2 + 0.5));
        CWwdGameObjectA* obj = m_object;
        CDDrawWorker* holder = obj->m_frameSet;
        if (holder != NULL) {
            CImage* glyph;
            if (slot >= holder->m_minIndex && slot <= holder->m_maxIndex) {
                glyph = static_cast<CImage*>(holder->m_items.GetAt(slot));
            } else {
                glyph = NULL;
            }
            obj->m_layer = glyph;
            obj->m_frameIndex = slot;
        }
        m_health = result;
    }
    m_object->m_screenX = e->m_object->m_screenX;
    m_object->m_screenY = m_yOffset + e->m_object->m_screenY;
    return 0;
}

RVA(0x0007f270, 0xa3)
i32 CGruntHealthSprite::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_cell, sizeof(m_cell));
            ar->Write(&m_health, sizeof(m_health));
            ar->Write(&m_yOffset, sizeof(m_yOffset));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_cell, sizeof(m_cell));
            ar->Read(&m_health, sizeof(m_health));
            ar->Read(&m_yOffset, sizeof(m_yOffset));
            break;
    }
    if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}
