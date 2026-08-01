#include <Gruntz/SerialArchive.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/LightFxMgr.h>
#include <Wap32/ZVec.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/TriggerMgr.h>
#include <rva.h>

VTBL(CGruntPowerupSprite, 0x001e76c4);

template<> DATA(0x00244d30)
CActReg CActRegPool<CGruntPowerupSprite>::s_table(2000, 2010);
RVA_COMPGEN(0x00012340, 0x1e, ??_GCGruntPowerupSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00012370, 0x44, ??1CGruntPowerupSprite@@UAE@XZ)

RVA(0x0007fdb0, 0x166)
CGruntPowerupSprite::CGruntPowerupSprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyName("GAME_LIGHTING_POWERUP");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    if (m_object->m_sortKey != 0x15) {
        m_object->m_sortKey = 0x15;
        m_object->m_flags |= 0x20000;
    }
    m_38->m_stateFlags |= 1;
}

RVA(0x00080020, 0x102)
void CGruntPowerupSprite::FireActivation(i32 id) {
    if ((*((CActRegPool<CGruntPowerupSprite>::s_table.ResolveEntry(id)))) != 0) {
        (this->*(*((CActRegPool<CGruntPowerupSprite>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x00080180, 0x18d)
void CGruntPowerupSprite::RegisterActs() {
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
    (*((CActRegPool<CGruntPowerupSprite>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntPowerupSprite::Update);
}

RVA(0x00080380, 0x6c)
i32 CGruntPowerupSprite::SetCell(i32 x, i32 y, i32 powerup) {
    m_cellX = x;
    m_cellY = y;
    m_powerupId = powerup;
    CShadeTable* rec = g_gameReg->m_logicPump->m_tables[powerup];
    CWwdGameObjectA* r = m_object;
    r->m_drawActive = 1;
    r->m_drawFillCmd = 7;
    r->m_drawFillArg = rec;
    m_38->m_stateFlags &= ~1;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    return 1;
}

RVA(0x00080410, 0x51)
i32 CGruntPowerupSprite::Update() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CGrunt* e = g_gameReg->m_cmdGrid->m_grid[m_cellX * 15 + m_cellY];
    if (e != 0) {
        m_object->m_screenX = e->m_object->m_screenX;
        m_object->m_screenY = e->m_object->m_screenY;
    }
    return 0;
}

RVA(0x00080490, 0xbe)
i32 CGruntPowerupSprite::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    if (Chain(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            ar->Write(&m_cellX, 8);
            ar->Write(&m_powerupId, 4);
            break;
        case 7: {
            ar->Read(&m_cellX, 8);
            ar->Read(&m_powerupId, 4);
            i32 id = m_powerupId;
            CWwdGameObjectA* r = m_object;
            CShadeTable* v = g_gameReg->m_logicPump->m_tables[id];
            r->m_drawActive = 1;
            r->m_drawFillArg = v;
            r->m_drawFillCmd = 7;
            break;
        }
    }
    return 1;
}
