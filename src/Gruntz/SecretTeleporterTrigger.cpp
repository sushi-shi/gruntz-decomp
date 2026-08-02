#include <Bute/ButeTree.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Wap32/ZVec.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/BattlezData.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/SecretTeleporterTrigger.h>
#include <Gruntz/SecretLevelTrigger.h>
#include <Gruntz/SerialArchive.h>
#include <rva.h>
#include <Gruntz/SerialArchive.h>

VTBL(CSecretTeleporterTrigger, 0x001e7564);
VTBL(CSecretLevelTrigger, 0x001e8804);
template<> DATA(0x00244688)
CActReg CActRegPool<CSecretTeleporterTrigger>::s_table(2000, 2010);
template<> DATA(0x00244598)
CActReg CActRegPool<CSecretLevelTrigger>::s_table(2000, 2010);

static inline CActHandler* ActLookup(i32 coord) {
    return (CActRegPool<CSecretTeleporterTrigger>::s_table.ResolveEntry(coord));
}

RVA(0x00010a10, 0x47)
i32 CSecretTeleporterTrigger::SerializeMove(CFileMemBase* a, i32 b, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, d) != 0;
}

RVA_COMPGEN(0x00010a80, 0x1e, ??_GCSecretTeleporterTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00010ab0, 0x44, ??1CSecretTeleporterTrigger@@UAE@XZ)

RVA(0x00010b20, 0x4b)
CSecretLevelTrigger::CSecretLevelTrigger() {}

RVA(0x00010bb0, 0x47)
i32 CSecretLevelTrigger::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x00010c20, 0x1e, ??_GCSecretLevelTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00010c50, 0x44, ??1CSecretLevelTrigger@@UAE@XZ)

RVA(0x00041e90, 0x1ac)
CSecretTeleporterTrigger::CSecretTeleporterTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {

    if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
        m_wwdObject->m_flags |= 0x10000;
    } else {
        m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
        m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
        if (m_object->m_sortKey != 0) {
            m_object->m_sortKey = 0;
            m_object->m_flags |= 0x20000;
        }
        m_wwdObject->m_flags |= 2;
        m_wwdObject->m_stateFlags |= 1;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("A");
        g_gameReg->m_scoreHud->m_secretsAvailable++;
    }
}

RVA(0x00042150, 0x102)
void CSecretTeleporterTrigger::FireActivation(i32 coord) {
    CActHandler* e = ActLookup(coord);
    if ((*e) != 0) {
        CActHandler* e2 = ActLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x000422b0, 0x18d)
void CSecretTeleporterTrigger::RegisterActs() {
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
    (*((ActLookup(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CSecretTeleporterTrigger::SpawnTeleporter);
}

RVA(0x000424b0, 0x1a0)
CSecretLevelTrigger::CSecretLevelTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    if (g_gameReg->m_134 == 1 && g_gameReg->m_130 == 0) {
        m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
        m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
        if (m_object->m_sortKey != 0) {
            m_object->m_sortKey = 0;
            m_object->m_flags |= 0x20000;
        }
        m_wwdObject->m_flags |= 2;
        m_wwdObject->m_stateFlags |= 1;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("A");
    } else {
        m_wwdObject->m_flags |= 0x10000;
    }
}

RVA(0x00042760, 0x102)
void CSecretLevelTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CSecretLevelTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CSecretLevelTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x000428c0, 0x18d)
void CSecretLevelTrigger::RegisterActs() {
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
    (*((CActRegPool<CSecretLevelTrigger>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CSecretLevelTrigger::Tick);
}

RVA(0x00042ac0, 0x90)
i32 CSecretLevelTrigger::Tick() {
    i32 outA, outB;
    CWwdGameObjectA* spr = m_object;
    CGrunt* hit =
        g_gameReg->m_cmdGrid->HitTestCell(spr->m_screenX, spr->m_screenY, &outB, &outA, 1);
    if (hit) {
        spr = m_object;
        i32 ok = 1;
        i32 lvl = spr->m_powerup;
        i32 lyr = spr->m_damage;

        if (lvl != 0 && hit->m_entranceReason != lvl) {
            ok = 0;
        }
        if (lyr != 0 && hit->m_198 != lyr) {
            ok = 0;
        }
        if (ok) {
            g_gameReg->m_cmdGrid->CellDispatch(outB, outA, 0xc, -1);
        }
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}

RVA(0x00042b80, 0x153)
i32 CSecretTeleporterTrigger::SpawnTeleporter() {
    i32 loc0, loc4;
    CWwdGameObjectA* o = m_object;
    CGrunt* hit = g_gameReg->m_cmdGrid->HitTestCell(o->m_screenX, o->m_screenY, &loc0, &loc4, 1);
    if (hit) {
        o = m_object;
        CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
            0,
            (o->m_score << 5) + 0x10,
            (o->m_points << 5) + 0x10,
            0,
            "Teleporter",
            0x40003
        );
        if (spr) {
            spr->m_smarts = 2;
            spr->m_animWorker->m_speed = m_object->m_animWorker->m_speed;
            spr->m_speedX = m_object->m_speedX;
            spr->m_speedY = m_object->m_speedY;
            spr->m_powerup = m_object->m_powerup;
            spr->m_damage = m_object->m_damage;
            spr->m_score = m_object->m_score;
            spr->m_points = m_object->m_points;
            spr->m_health = 0;
            CWwdGameObjectA* eo = hit->m_object;
            CGruntzMgr* g = g_gameReg;
            i32 ey = eo->m_screenY;
            i32 ex = eo->m_screenX;
            CDDrawWorkerHost* rc = g->m_world->m_level->m_mainPlane;
            if (ex < rc->m_viewRect.right && ex >= rc->m_viewRect.left && ey < rc->m_viewRect.bottom
                && ey >= rc->m_viewRect.top) {
                g->m_cueSink->SpawnVoiceDriver(hit, 0x3fc, -1, 0, -1, -1);
            }
        }
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}
