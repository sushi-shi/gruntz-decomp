

#include <Bute/ButeTree.h>
#include <Rez/FrameClock.h>
#include <Gruntz/Random.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/Grunt.h>
#include <DDrawMgr/AniAdvance.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <Gruntz/Enums.h>
#include <Gruntz/State.h>
#include <Wap32/Wap32.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/BoundaryLowerMethodsViews.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GameRegistry.h>
#include <rva.h>
#include <Pix16.h>
#include <AddrWord.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <Bute/ButeMgr.h>

#include <Gruntz/InGameIcon.h>
#include <Gruntz/GruntEntranceArrival.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GameRegistry.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/UserLogic.h>
#include <Utils/MapTyped.h>

DATA(0x001e9a68)
double s_fpZero = 0.0;

static char s_TimePerTile[] = "TimePerTile";
static char s_Grunt[] = "Grunt";
static char s_EntranceSafeTime[] = "EntranceSafeTime";
static char s_IdleDelay[] = "IdleDelay";
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius";
static char s_CombatTimeout[] = "CombatTimeout";

static char s_BOMBGRUNT[] = "BOMBGRUNT";
static char s_RunningTimePerTile[] = "RunningTimePerTile";

static const char s_animKeyA[] = "A";
static const char s_animKeyK[] = "K";

DATA(0x0020df94)
char k_60df94[] = "S";

DATA(0x0020dc0c)
char s_codeO[] = "O";

DATA(0x0020dc04)
char s_codeN[] = "N";

DATA(0x0020dc08)
char s_codeQ[] = "Q";

DATA(0x0020e194)
static char s_ToyTime[] = "ToyTime";
static const char s_GRUNTZ_ENTRANCEZ[] = "GRUNTZ_ENTRANCEZ";
static const char s_GRUNTZ_ENTRANCEZ_ONE[] = "GRUNTZ_ENTRANCEZ_ONE";
static const char s_GRUNTZ_ENTRANCEZ_TWO[] = "GRUNTZ_ENTRANCEZ_TWO";
static const char s_GRUNTZ_ENTRANCEZ_THREE[] = "GRUNTZ_ENTRANCEZ_THREE";
static const char s_GRUNTZ_ENTRANCEZ_DROP[] = "GRUNTZ_ENTRANCEZ_DROP";
static const char s_GRUNTZ_ENTRANCEZ_RESSURECT[] = "GRUNTZ_ENTRANCEZ_RESSURECT";
static const char s_GRUNTZ_DEATHZ_MELT[] = "GRUNTZ_DEATHZ_MELT";

static const char s_exitKeyB[] = "B";
static const char s_GRUNTZ_EXITZ[] = "GRUNTZ_EXITZ";
DATA(0x0020e250)
static const char s_GRUNTZ_EXITZ_ONE[] = "GRUNTZ_EXITZ_ONE";
DATA(0x0020e23c)
static const char s_GRUNTZ_EXITZ_TWO[] = "GRUNTZ_EXITZ_TWO";
DATA(0x0020e224)
static const char s_GRUNTZ_EXITZ_THREE[] = "GRUNTZ_EXITZ_THREE";

DATA(0x0020e1f8)
static const char s_GRUNTZ_GOKARTGRUNT[] = "GRUNTZ_GOKARTGRUNT_GOKARTGRUNTLOOP";
DATA(0x0020e1c8)
static const char s_GRUNTZ_BIGWHEELGRUNT[] = "GRUNTZ_BIGWHEELGRUNT_BIGWHEELGRUNTLOOP";

static i32 s_entrancePreset0[3];
static i32 s_entrancePreset1[3];
static i32 s_entrancePreset2[3];

static void GruntPosScratchTeardown() {
    CString* slot = (g_typeColl.Slots());
    i32 cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
}

static __inline i32 s_TileFlags(CMapMgr* b, i32 tx, i32 ty) {
    if (static_cast<u32>(tx) >= static_cast<u32>(b->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(b->m_height)) {
        return 1;
    }
    return ((b->m_rowInts[ty]))[tx * 7];
}

static void GruntScratchTeardown() {
    CString* slot = (g_typeColl.Slots());
    i32 cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
}

// @early-stop
RVA(0x0005ecd0, 0x4f3)
void CGrunt::FinalizeStep(char* name) {
    CUserLogic::FinalizeStep(name);
    AdvanceMotion();
    if (m_struckSlotSound != 0) {
        bool neL = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "L") != 0);
        if (neL && strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "G") != 0) {
            StopStruckSlotSound();
        }
    }
    if (m_struckVoiceSound != 0) {
        if (m_gruntKind == 0) {
            StopStruckVoiceSound();
        } else {
            CGruntzMgr* g = g_gameReg;
            i32 y = m_object->m_screenY;
            i32 x = m_object->m_screenX;
            if (!(x < g->m_viewBounds.right && x >= g->m_viewBounds.left
                  && y < g->m_viewBounds.bottom && y >= g->m_viewBounds.top)) {
                StopStruckVoiceSound();
            }
        }
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeO) == 0) {

        if (m_object->m_screenX == m_lastTilePxX && m_object->m_screenY == m_lastTilePxY) {
            return;
        }
        GruntDirectionCell c = m_entranceCell;
        i32 row = (c.row == 0) ? 2 : (c.row == 2 ? 0 : c.row);
        i32 column = (c.column == 0) ? 2 : (c.column == 2 ? 0 : c.column);
        i32 base = 3 * row + column;
        CGruntCellRec* cell = &m_cells[base];
        double d48 = cell->m_dirX;
        double d50 = cell->m_dirY;
        m_408 = static_cast<double>(static_cast<i64>(g_frameDelta)) * d48 * m_moveSpeed + m_408;
        m_410 = static_cast<double>(static_cast<i64>(g_frameDelta)) * d50 * m_moveSpeed + m_410;
        i32 nx = static_cast<i32>((cell->m_stepX + m_408));
        i32 ny = static_cast<i32>((cell->m_stepY + m_410));
        if ((d48 > s_fpZero && nx > m_lastTilePxX) || (d48 < s_fpZero && nx < m_lastTilePxX)) {
            nx = m_lastTilePxX;
        }
        if ((d50 > s_fpZero && ny > m_lastTilePxY) || (d50 < s_fpZero && ny < m_lastTilePxY)) {
            ny = m_lastTilePxY;
        }
        m_object->m_screenX = nx;
        m_object->m_screenY = ny;
        CWwdGameObjectA* h = m_object;
        i32 v = h->m_screenY + 0x186a0;
        if (h->m_sortKey != v) {
            h->m_sortKey = v;
            h->m_flags |= 0x20000;
        }
        return;
    }

    CString* rec = g_typeColl.ScratchResolve(m_objAux->m_1c);
    GruntPosScratchTeardown();
    if (strcmp(*rec, k_60df94) == 0) {
        if (m_object->m_screenX == m_lastTilePxX && m_object->m_screenY == m_lastTilePxY) {
            return;
        }
        GruntDirectionCell c = m_entranceCell;
        i32 base = 3 * c.row + c.column;
        CGruntCellRec* cell = &m_cells[base];
        double d48 = cell->m_dirX;
        double d50 = cell->m_dirY;
        m_408 = static_cast<double>(static_cast<i64>(g_frameDelta)) * d48 * m_moveSpeed + m_408;
        m_410 = static_cast<double>(static_cast<i64>(g_frameDelta)) * d50 * m_moveSpeed + m_410;
        i32 nx = static_cast<i32>((cell->m_stepX + m_408));
        i32 ny = static_cast<i32>((cell->m_stepY + m_410));
        if ((d48 > s_fpZero && nx > m_lastTilePxX) || (d48 < s_fpZero && nx < m_lastTilePxX)) {
            nx = m_lastTilePxX;
        }
        if ((d50 > s_fpZero && ny > m_lastTilePxY) || (d50 < s_fpZero && ny < m_lastTilePxY)) {
            ny = m_lastTilePxY;
        }
        m_object->m_screenX = nx;
        m_object->m_screenY = ny;
        CWwdGameObjectA* h = m_object;
        i32 v = h->m_screenY + 0x186a0;
        if (h->m_sortKey != v) {
            h->m_sortKey = v;
            h->m_flags |= 0x20000;
        }
    }
    return;
}

// @early-stop
RVA(0x000616e0, 0xa8)
i32 CGrunt::ResetGeometry() {
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(m_poseAttackIdle);

    CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
    CAniDesc* elem =
        desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
    i32 frame = elem->m_param;

    GruntDirectionCell cell = m_entranceCell;
    i32 row = cell.row;
    i32 column = cell.column;
    i32 index = 3 * row + column;

    const char* name = m_cells[index].AttackName().GetBuffer(0);
    m_wwdObject->ApplyLookupSprite(name, frame);

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_animKeyA);
    return 0;
}

// @early-stop
RVA(0x000617c0, 0x127)
i32 CGrunt::UpdateGruntStatus() {
    if (m_poweredUp == 0) {
        ResetEntranceAnimation(1, 0, 0);
        return 0;
    }

    m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));

    if (m_stamina >= 0x64) {
        if (m_neighborValid == 0) {
            return 0;
        }
        m_neighborValid = 0;
        CGrunt* n = m_tileMgr->m_grid[m_neighborCol * TM_GRID_COLS + m_neighborRow];
        if (n == 0 || n->m_entranceCommitted == 0) {
            return 0;
        }
        if (RectContains(n->m_object->m_screenX, n->m_object->m_screenY)) {
            CommitNeighbor(
                m_neighborCol,
                m_neighborRow,
                n->m_object->m_screenX,
                n->m_object->m_screenY
            );
        }
        return 0;
    }

    if (m_stamina <= 0x32) {
        return 0;
    }
    if (m_lowStaminaCued != 0) {
        return 0;
    }

    CGruntzMgr* g = g_gameReg;
    i32 y = m_object->m_screenY;
    i32 x = m_object->m_screenX;
    const RECT& vr = g->m_world->m_level->m_mainPlane->m_viewRect;
    if (x < vr.right && x >= vr.left && y < vr.bottom && y >= vr.top) {
        g->m_cueSink->LoadGruntSpawnConfig(this, 2, -1, -1, -1);
    }
    m_lowStaminaCued = 1;
    return 0;
}

// @early-stop
RVA(0x00061940, 0x200)
i32 CGrunt::RearmAttackAnim(i32 col, i32 row) {
    if (m_entranceReason >= 0x17) {
        return 0;
    }

    m_neighborCol = col;
    m_neighborRow = row;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_codeF);

    m_combatActive = 1;

    i32 idx;
    switch (m_entranceReason - 2) {
        case 0:
            if (m_arrivalState != 0) {
                m_entranceActive = 1;
            }
            idx = 1;
            break;
        case 7:
        case 8:
        case 9:
        case 15:
        case 18:
        case 19:
        case 20:
            idx = 1;
            break;
        default:
            idx = GruntRand() % 2;
            break;
    }

    CreateHealthSprite();

    m_combatTimeoutLo = static_cast<i32>(g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388));
    m_combatTimeoutHi = 0;
    m_combatClockLo = static_cast<i32>(g_frameTime);
    m_combatClockHi = 0;

    {
        CWwdGameObjectA* h = m_object;
        CGruntzMgr* g = g_gameReg;
        i32 yy = h->m_screenY;
        i32 xx = h->m_screenX;
        const RECT* rect = &g->m_world->m_level->m_mainPlane->m_viewRect;
        if (xx < rect->right && xx >= rect->left && yy < rect->bottom && yy >= rect->top) {
            g->m_cueSink->LoadGruntSpawnConfig(this, 1, -1, -1, -1);
        }
    }

    {
        CWwdGameObjectA* h = m_object;
        i32 z = h->m_screenY + 0x186c1;
        if (h->m_sortKey != z) {
            h->m_sortKey = z;
            h->m_flags |= 0x20000;
        }
    }

    CWwdGameObjectA* p = m_wwdObject;
    m_value = p->m_animCursor.m_animation;
    p->m_animCursor.Setup(m_poseAttack[idx]);

    CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
    CAniDesc* el =
        desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
    i32 frame = el->m_param;

    GruntDirectionCell cell = m_entranceCell;
    i32 cellRow = cell.row;
    i32 cellColumn = cell.column;
    i32 base = cellRow + (cellColumn + 2 * cellRow);
    char* buf = m_cells[base].AttackName().GetBuffer(0);
    m_wwdObject->ApplyLookupSprite(buf, frame);
    m_214 = 1;
    return 0;
}

// @early-stop
RVA(0x00061bc0, 0xb2)
i32 CGrunt::RearmAttackAnim2() {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_codeF);

    CWwdGameObjectA* p = m_wwdObject;
    m_value = p->m_animCursor.m_animation;
    p->m_animCursor.Setup(m_poseAttack[GRUNT_ATTACK2]);

    CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
    CAniDesc* el =
        desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
    i32 frame = el->m_param;

    GruntDirectionCell cell = m_entranceCell;
    i32 row = cell.row;
    i32 column = cell.column;
    i32 base = row + (column + 2 * row);
    char* buf = m_cells[base].AttackName().GetBuffer(0);
    m_wwdObject->ApplyLookupSprite(buf, frame);
    m_214 = 1;
    return 0;
}

RVA(0x00061cb0, 0x34a)
i32 CGrunt::StepAttackFire() {
    i32 flag = 0;
    if (m_wwdObject->m_animCursor.Advance(g_engineFrameDelta) == 2) {

        switch (m_entranceReason) {
            case GRUNT_GUNHAT:
            case GRUNT_NERFGUN:
            case GRUNT_ROCK: {
                CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    "Projectile",
                    0x40003
                );
                spr->m_animWorker->m_notify(spr);
                CProjectile* s = static_cast<CProjectile*>(spr->m_animWorker->m_logic);
                if (s->LoadProjectileSprites(
                        m_entranceReason,
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        m_208,
                        m_20c,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )
                    == 0) {
                    s->m_wwdObject->m_flags |= 0x10000;
                }
                break;
            }
            case GRUNT_BOOMERANG: {
                CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    "Boomerang",
                    0x40003
                );
                spr->m_animWorker->m_notify(spr);
                CProjectile* s = static_cast<CProjectile*>(spr->m_animWorker->m_logic);
                if (s->LoadProjectileSprites(
                        m_entranceReason,
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        m_208,
                        m_20c,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )
                    == 0) {
                    s->m_wwdObject->m_flags |= 0x10000;
                }
                break;
            }
            case GRUNT_TIMEBOMB: {
                i32 pos[2];
                EntranceTileOffset(pos);
                CGameObject* spr = g_gameReg->m_world->m_childGroup
                                       ->CreateSprite(0, pos[0], pos[1], 0xf, "TimeBomb", 0x40003);
                spr->m_damage = 0;
                spr->m_animWorker->m_notify(spr);
                spr->m_smarts = m_tileOwnerHi;
                break;
            }
            case GRUNT_WELDER:
            case GRUNT_WINGZ: {
                CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    "Projectile",
                    0x40003
                );
                spr->m_animWorker->m_notify(spr);
                CProjectile* s = static_cast<CProjectile*>(spr->m_animWorker->m_logic);
                if (s->LoadProjectileSprites(
                        m_entranceReason,
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        m_208,
                        m_20c,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )
                    == 0) {
                    s->m_wwdObject->m_flags |= 0x10000;
                }
                break;
            }
            default: {

                CGrunt* tgt = m_tileMgr->m_grid[m_neighborCol * TM_GRID_COLS + m_neighborRow];
                if (tgt == 0) {
                    flag = 1;
                    break;
                }
                tgt->StepCombatReaction(
                    m_entranceReason,
                    m_214,
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    m_gruntKind
                );
                i32 t = tgt->m_entranceReason;
                if (t > 0x16) {
                    t = tgt->m_toolId;
                }
                if (t == 1 && m_gruntKind != GRUNT_INVULNERABLE) {
                    m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 0xb, m_neighborCol);
                    return 0;
                }
                break;
            }
        }

        m_entranceActive = 1;
        i32 dt = g_buteMgr.GetDword(static_cast<const char*>(m_animSetName), "AttackDowntime");
        if (m_gruntKind == GRUNT_ROIDZ) {
            dt = 0;
        }
        m_attackDowntimeLo = dt;
        m_attackDowntimeHi = 0;
        m_attackClockLo = static_cast<i32>(g_frameTime);
        m_attackClockHi = 0;
        m_lowStaminaCued = 0;
        m_stamina = 0;
        if (m_healthSprite != 0) {
            CreateStaminaSprite();
        }
        m_combatActive = 0;
    }

    CWwdGameObjectA* r = m_wwdObject;
    if ((r->m_animCursor.m_finished == 0 || r->m_animCursor.m_frameTicksLeft != 0) && flag == 0) {
        return 0;
    }
    if (m_entranceReason == GRUNT_BOOMERANG) {
        LoadGruntTypeTable(0, 1, 0, 0);
    }
    CWwdGameObjectA* h = m_object;
    i32 zkey = h->m_screenY + 0x186a0;
    if (h->m_sortKey != zkey) {
        h->m_sortKey = zkey;
        h->m_flags |= 0x20000;
    }
    i32 v220 = m_poweredUp;
    m_entranceActive = 0;
    if (v220 != 0) {
        ResetGeometry();
        return 0;
    }
    ResetEntranceAnimation(1, 0, 0);
    return 0;
}

// @early-stop
RVA(0x00062110, 0x5bc)
i32 CGrunt::UpdateArrival(i32 walking, i32 commit) {
    if (commit != 0) {
        StopStruckSlotSound();
        if (m_arrivalPhase == 3 && m_arrivalActive != 0) {
            CGrunt* occ = m_tileMgr->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
            if (occ != 0) {
                CGameObject* inner = occ->m_object;
                i32 yMasked = (inner->m_screenY & ~0x1f) + 0x10;
                i32 xMasked = (inner->m_screenX & ~0x1f) + 0x10;
                if (RectContainsGated(xMasked, yMasked) != 0) {
                    m_tileMgr->ApplyTriggerB(
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        inner->m_screenX,
                        inner->m_screenY
                    );
                }
            }
        }

        if (m_poweredUp != 0 && m_neighborValid == 0) {
            m_entranceActive = 0;
            m_combatActive = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            ResetEntranceAnimation(1, 0, 0);
        }
        m_entranceActive = 1;
        SetEntrancePos(1, 1);

        if (CoordCount() != 0) {

            POSITION pos = m_31c.GetHeadPosition();
            while (pos != 0) {
                void* buf = m_31c.GetNext(pos);
                if (buf != 0) {
                    CoordPoolNode* sp = g_coordPool.NodeOf(buf);
                    sp->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = sp;
                }
            }
            m_31c.RemoveAll();
        }

        m_entranceStamped = 0;
        if (m_healthSprite != 0) {
            m_healthSprite->m_flags |= 0x10000;
            m_healthSprite = 0;
        }
        if (m_toySprite != 0) {
            m_toySprite->m_flags |= 0x10000;
            m_toySprite = 0;
        }

        if (m_entranceReason == 0x1e) {
            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = ActFindId("P");
            i32 toyIdx = rand() % 2;
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseToy[toyIdx], 0);

            CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
            CAniDesc* el = desc->m_records.GetSize() > 0
                               ? static_cast<CAniDesc*>(desc->m_records.GetAt(0))
                               : 0;
            i32 frame = el->m_param;
            char* buf = (&m_448)->GetBuffer(0);
            m_wwdObject->ApplyLookupSprite(buf, frame);

            i32 cueTier = ((toyIdx != 0) ? 0xa : 0) + 0x406;
            CGruntzMgr* g = g_gameReg;
            i32 m380 = m_moveVariant;
            if (m380 != 0) {
                i32 tier = cueTier + m380 - 1;
                const LevelCoordRect* bounds = &g->m_world->m_level->m_mainPlane->m_viewRect;
                if (CGameLevel::PointInBounds(bounds, m_object->m_screenX, m_object->m_screenY)
                    != 0) {
                    g->m_cueSink->SpawnVoiceDriver(this, tier, 0, -1, -1, -1);
                }
            } else {
                if (m_moveKind == 0) {
                    i32 md = (g->m_134 == 1) ? 3 : 6;
                    m_moveKind = rand() % md + 1;
                }
                i32 tier = cueTier + m_moveKind - 1;
                const LevelCoordRect* bounds = &g->m_world->m_level->m_mainPlane->m_viewRect;
                if (CGameLevel::PointInBounds(bounds, m_object->m_screenX, m_object->m_screenY)
                    != 0) {
                    g->m_cueSink->SpawnVoiceDriver(this, tier, 0, -1, -1, -1);
                }
            }
            return 0;
        } else {
            DWORD tt = g_buteMgr.GetDword(static_cast<const char*>(m_animSetName), s_ToyTime);
            m_toyDuration = static_cast<u32>(tt);
            m_toyClock = static_cast<u32>(g_frameTime);
            m_toyTime = 0x64;
            CreateToyTimeSprite();
        }
    }

    if (walking != 0) {

        m_toyTileIndex = 0;
        if (m_poweredUp != 0 && m_neighborValid == 0) {
            m_entranceActive = 0;
            m_combatActive = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            ResetEntranceAnimation(1, 0, 0);
        }
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("L");
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseWalk);
        GruntDirectionCell cell = m_entranceCell;
        i32 colv = cell.column + cell.row * 2;
        i32 basev = cell.row + colv;
        char* nm = m_cells[basev].WalkName().GetBuffer(0);
        m_wwdObject->ApplyName(nm);

        DWORD tt = g_buteMgr.GetDword(static_cast<const char*>(m_animSetName), s_ToyTime);
        m_idleDelay = static_cast<u32>(static_cast<i32>((tt >> 1)));
        m_idleAnchor = static_cast<u32>(static_cast<i32>(g_frameTime));
        return 0;
    }

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("G");

    CWwdGameObjectA* h = m_object;
    i32 z = h->m_screenY + 0xc3500;
    if (h->m_sortKey != z) {
        h->m_sortKey = z;
        h->m_flags |= 0x20000;
    }

    i32 t0 = m_poseToy[GRUNT_TOY1]->m_total;
    i32 t1 = m_poseToy[GRUNT_TOY2]->m_total;
    i64 elapsed = m_toyClock - static_cast<i64>(static_cast<u32>(g_frameTime));
    i32 cap = static_cast<i32>(elapsed);
    if (elapsed < 0) {
        cap = 0;
    }
    i32 d0 = (t0 > cap) ? (t0 - cap) : 0;
    i32 d1 = (t1 > cap) ? (t1 - cap) : 0;
    i32 sel;
    if (d0 != 0) {
        sel = (d1 != 0) ? ((d0 < d1) ? 0 : 1) : 0;
    } else if (d1 != 0) {
        sel = 1;
    } else {
        i32 r = rand() % 0x64 + 1;
        sel = (r >= m_toyBlendPct) ? 1 : 0;
    }

    CAniElement* cur = m_wwdObject->m_animCursor.m_animation;
    CAniElement* want = m_poseToy[sel];
    if (cur != want) {
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(want);
        CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
        CAniDesc* el =
            desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = el->m_param;
        char* buf = (&m_448)->GetBuffer(0);
        m_wwdObject->ApplyLookupSprite(buf, frame);
    }

    CWwdGameObjectA* hud = m_object;
    CGruntzMgr* g = g_gameReg;
    i32 yy = hud->m_screenY;
    i32 xx = hud->m_screenX;

    RecordBytes<CDDrawWorkerHost*> band;
    band.m_rec = &g->m_world->m_level->m_mainPlane;
    i32* rectBase = band.m_dwords;
    i32 lim = rectBase[0x48 / 4];
    i32* rect = rectBase + 0x40 / 4;
    if (sel != 0) {
        if (xx < lim && xx >= rect[0] && yy < rect[3] && yy >= rect[1]) {
            g->m_cueSink->LoadGruntSpawnConfig(this, 0xb, -1, -1, -1);
        }
    } else {
        if (xx < lim && xx >= rect[0] && yy < rect[3] && yy >= rect[1]) {
            g->m_cueSink->LoadGruntSpawnConfig(this, 0xa, -1, -1, -1);
        }
    }
    return 0;
}

// @early-stop
RVA(0x00062840, 0x25d)
i32 CGrunt::StepEntranceRelatchA() {
    i32 ready = m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (sub->m_finished != 0 && sub->m_frameTicksLeft == 0) {
        if (m_arrived != 0) {
            CreateHealthSprite();
            CreateStaminaSprite();
            CreateToySprite();
        }
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("A");
        LoadGruntTypeTable(m_toolId, 1, 0, 0);
        m_entranceActive = 0;
        CGruntzMgr* g = g_gameReg;
        CMapMgr* grid = g->m_tileGrid;
        i32 tx = m_lastTilePxX >> 5;
        i32 ty = m_lastTilePxY >> 5;
        i32 flags;
        if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
            || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
            flags = 1;
        } else {
            flags = ((grid->m_rowInts[ty]))[tx * 7];
        }
        if (flags & 0x80) {
            SetEntrancePos(1, 1);
            m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
            return 0;
        }
        CWwdGameObjectA* h = m_object;
        i32 v = h->m_screenY + 0x186a0;
        if (h->m_sortKey != v) {
            h->m_sortKey = v;
            h->m_flags |= 0x20000;
        }
        return 0;
    }

    i64 diff = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_toyClock;
    if (diff >= m_toyDuration && m_entranceStamped == 0 && ready == 1) {
        if (m_toyTimeSprite != 0) {
            m_toyTimeSprite->m_flags |= 0x10000;
            m_toyTimeSprite = 0;
        }
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseToy[GRUNT_TOY_BREAK]);
        CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
        CAniDesc* elem =
            desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = elem->m_param;
        char* nm = (&m_448)->GetBuffer(0);
        m_wwdObject->ApplyLookupSprite(nm, frame);
        m_entranceStamped = 1;
        CWwdGameObjectA* h = m_object;
        CGruntzMgr* g = g_gameReg;
        i32 y = h->m_screenY;
        i32 x = h->m_screenX;
        const RECT& r = g->m_world->m_level->m_mainPlane->m_viewRect;
        if (x < r.right && x >= r.left && y < r.bottom && y >= r.top) {
            g->m_cueSink->LoadGruntSpawnConfig(this, 0xc, -1, -1, -1);
        }
        return 0;
    }
    StopStruckSlotSound();
    if (ready == 1) {
        UpdateArrival(0, 0);
    }
    return 0;
}

RVA(0x00062b40, 0x11)
i32 CGrunt::RecordFrameTick() {
    m_438 = g_frameTicks;
    return 1;
}

RVA(0x00062b70, 0x205)
i32 CGrunt::RectSegProbe(RECT* p, POINT* e1, POINT* e2) {
    i32 e1y = e1->y;
    i32 e2y = e2->y;

    i32 py = p->top;
    if ((e1y < py) != (e2y < py)) {
        float t = static_cast<float>((py - e1y)) / static_cast<float>((e2y - e1y));
        float ix = static_cast<float>(e1->x) + t * static_cast<float>((e2->x - e1->x));

        if (ix >= static_cast<float>(p->left) && ix <= static_cast<float>(p->right)) {
            return 1;
        }
    }

    i32 pyc = p->bottom;
    if ((e1y < pyc) != (e2y < pyc)) {
        float t = static_cast<float>((pyc - e1y)) / static_cast<float>((e2y - e1y));
        float ix = static_cast<float>(e1->x) + t * static_cast<float>((e2->x - e1->x));

        if (ix >= static_cast<float>(p->left) && ix <= static_cast<float>(p->right)) {
            return 1;
        }
    }

    i32 e1x = e1->x;
    i32 e2x = e2->x;
    i32 px = p->left;
    if ((e1x > px) != (e2x > px)) {

        float t = static_cast<float>((px - e1x)) / static_cast<float>((e2x - e1x));
        float iy = static_cast<float>(e1y) + t * static_cast<float>((e2y - e1y));

        if (iy < static_cast<float>(p->bottom) && iy > static_cast<float>(p->top)) {
            return 1;
        }
    }

    i32 pxr = p->right;
    if ((e1x > pxr) != (e2x > pxr)) {

        float t = static_cast<float>((pxr - e1x)) / static_cast<float>((e2x - e1x));
        float iy = static_cast<float>(e1y) + t * static_cast<float>((e2y - e1y));

        if (iy < static_cast<float>(p->bottom) && iy > static_cast<float>(p->top)) {
            return 1;
        }
    }

    return 0;
}

RVA(0x00062e10, 0x47e)
void CGrunt::ResetEntranceAnimation(i32 apply, i32 cycle, i32 cue) {
    m_resetApplied = 0;

    i32 notIdle = strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_animKeyA) != 0;
    i32 applied = 0;

    if (notIdle && cycle == 0) {

        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseIdle[GRUNT_IDLE1]);
        m_idleWindow = static_cast<u32>(0x3a98);
        m_idleTimer = static_cast<u32>(static_cast<i32>(g_frameTime));
        i32 n = static_cast<i32>(g_buteMgr.GetDwordDef(s_Grunt, s_IdleDelay, 0x7530)) + 1;
        m_idleDelay = static_cast<u32>(GruntRand() % n + 0x7530);
        m_idleAnchor = static_cast<u32>(static_cast<i32>(g_frameTime));
        applied = 1;
    } else if (m_poseIdle[GRUNT_IDLE2] == 0) {

        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseIdle[GRUNT_IDLE1]);
    } else if (cycle == 0) {

        if (m_wwdObject->m_animCursor.m_animation == m_poseIdle[GRUNT_IDLE1]) {
            goto latch;
        }
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseIdle[GRUNT_IDLE1]);
        {
            i32 d = static_cast<i32>(g_buteMgr.GetDwordDef(s_Grunt, s_IdleDelay, 0x7530));
            applied = 1;
            m_idleDelay = static_cast<u32>(GruntRand() % (d - 0x4e1f) + 0x4e20);
            m_idleAnchor = static_cast<u32>(static_cast<i32>(g_frameTime));
        }
    } else {

        i32 count = (m_poseIdle[GRUNT_IDLE3] == 0) ? 1 : 2;
        i32 idx = GruntRand() % count + 1;
        if (cue != 0) {
            CGruntzMgr* g = g_gameReg;
            g->Rand();
            i32 focused = (m_tileOwnerHi == g_curPlayer);
            if (focused && idx > 0x5a) {
                if (CGameLevel::PointInBounds(
                        &g->m_world->m_level->m_mainPlane->m_viewRect,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )) {

                    AddrWord<CGrunt> src;
                    src.m_addr = this;
                    g->m_cueSink->SpawnVoiceDriver(src.m_word, 4, -1, -1, -1);
                }
            } else if (focused || m_entranceReason != 0) {
                if (idx == 1) {
                    if (CGameLevel::PointInBounds(
                            &g->m_world->m_level->m_mainPlane->m_viewRect,
                            m_object->m_screenX,
                            m_object->m_screenY
                        )) {

                        AddrWord<CGrunt> src;
                        src.m_addr = this;
                        g->m_cueSink->SpawnVoiceDriver(src.m_word, 5, -1, -1, -1);
                    }
                } else if (idx == 2) {
                    if (CGameLevel::PointInBounds(
                            &g->m_world->m_level->m_mainPlane->m_viewRect,
                            m_object->m_screenX,
                            m_object->m_screenY
                        )) {

                        AddrWord<CGrunt> src;
                        src.m_addr = this;
                        g->m_cueSink->SpawnVoiceDriver(src.m_word, 6, -1, -1, -1);
                    }
                }
            }
        }
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseIdle[idx]);
        m_resetApplied = 1;
        applied = 1;
    }

latch:
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_animKeyA);

    if (!applied && apply == 0) {
        return;
    }

    i32 row = m_entranceCell.row;
    i32 column = m_entranceCell.column;
    i32 direction = m_entranceCell.direction;
    if (m_wwdObject->m_animCursor.m_animation != m_poseIdle[GRUNT_IDLE1]) {
        switch (direction) {
            case 2:
            case 3:
                row = s_entrancePreset0[0];
                column = s_entrancePreset0[1];
                break;
            case 4:
            case 5:
                row = s_entrancePreset1[0];
                column = s_entrancePreset1[1];
                break;
            case 6:
            case 7:
            case 8:
                row = s_entrancePreset2[0];
                column = s_entrancePreset2[1];
                break;
            default:
                break;
        }
    }

    CString key = static_cast<const char*>(m_cells[3 * row + column].IdleName());

    CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
    CAniDesc* elem =
        desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
    m_wwdObject->ApplyLookupSprite(key, elem->m_param);
}

// @early-stop
RVA(0x000633e0, 0x2ca)
i32 CGrunt::ResolveEntranceArrival() {
    if (m_entranceActive != 0 && m_object->m_screenX == m_lastTilePxX
        && m_object->m_screenY == m_lastTilePxY) {
        CGruntzMgr* g = g_gameReg;
        CMapMgr* grid = g->m_tileGrid;
        i32 tx = m_object->m_screenX >> 5;
        i32 ty = m_object->m_screenY >> 5;
        i32 flags;
        if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
            || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
            flags = 1;
        } else {
            flags = ((grid->m_rowInts[ty]))[tx * 7];
        }
        if (!(flags & 0x80)) {
            m_entranceActive = 0;
        }
    }

    i32 ready = m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));

    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_idleTimer >= m_idleWindow) {
        CGruntzMgr* g = g_gameReg;
        i32 mode = g->m_134;
        if (mode != 1) {
            GruntzPlayer* slot = &g->m_options[m_tileOwnerHi];
            if (slot != 0 && slot->m_014 != 0) {
                if (m_tileClaimed == 0 && m_arrivalNotified == 0 && mode == 2
                    && g_curPlayer == m_tileOwnerHi && m_arrived == 0) {
                    m_tileMgr->GridAction6(m_tileOwnerHi, m_tileOwnerLo);
                    m_arrivalNotified = 1;
                    goto tail;
                }
                if (mode != 2 && g_curPlayer == m_tileOwnerHi && m_arrived == 0
                    && m_tileClaimed != 1) {
                    m_arrivalRerollLo = 0;
                    m_arrivalRerollWindowLo = 0;
                    m_arrivalRerollHi = 0;
                    m_arrivalRerollWindowHi = 0;
                    m_defenderX = m_lastTilePxX;
                    m_defenderY = m_lastTilePxY;
                    m_tileClaimed = 1;
                    i32 kind = m_entranceReason;

                    switch (kind) {
                        case 2:
                        case 9:
                        case 10:
                        case 11:
                        case 21:
                        case 22:
                            m_defenderRadius = 1;
                            break;
                        default:
                            m_defenderRadius =
                                g_buteMgr.GetIntDef(s_Grunt, s_PlayerDefenderRadius, 3) + 1;
                            break;
                    }
                    m_arrivalCol = -1;
                    m_arrivalRow = -1;
                    m_arrivalState = 4;
                    m_defenderState = 0;
                    m_arrivalActive = 0;
                    m_arrivalFlags |= 0x18040402;
                    m_object->m_extent.left = 0;
                    m_object->m_extent.right = 0;
                    m_object->m_extent.top = 0;
                    m_object->m_extent.bottom = 0;
                    SetEntrancePos(0, 0);
                }
            }
        }
    }

tail:
    if (m_wwdObject->m_animCursor.m_animation != m_poseIdle[GRUNT_IDLE1]) {

        if (m_wwdObject->m_animCursor.m_finished != 0
            && m_wwdObject->m_animCursor.m_frameTicksLeft == 0) {
            ResetEntranceAnimation(0, 0, 0);
        }
        return 0;
    }
    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_idleAnchor >= m_idleDelay
        && ready == 1) {
        ResetEntranceAnimation(0, 1, 1);
    }
    return 0;
}

// @early-stop
RVA(0x000637a0, 0x2f8)
i32 CGrunt::StepEntranceReinit() {
    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeD) == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "L") == 0);
    if (eq) {
        return 0;
    }

    m_8c8 = 0x7530;
    m_8cc = 0;
    m_8c0 = static_cast<i32>(g_frameTime);
    m_8c4 = 0;
    m_358 = 0;

    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "I") == 0);
    if (eq) {

        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            -1
        );
    }
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
    }
    m_35c = 0;
    if (CoordCount() == 0) {
        return 0;
    }

    Coord* co = static_cast<Coord*>(m_31c.GetHead());
    CMapMgr* b = g_gameReg->m_tileGrid;
    i32 flag;
    if (static_cast<u32>(co->m_x) >= static_cast<u32>(b->m_width)
        || static_cast<u32>(co->m_y) >= static_cast<u32>(b->m_height)) {
        flag = 1;
    } else {
        flag = ((b->m_rowInts[co->m_y]))[co->m_x * 7];
    }
    if (!(flag & 0x20000000)) {
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId(s_codeD);
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseWalk);
    } else {

        i32 tx = m_object->m_screenX >> 5;
        i32 ty = m_object->m_screenY >> 5;
        i32 flag2;
        if (static_cast<u32>(tx) >= static_cast<u32>(b->m_width)
            || static_cast<u32>(ty) >= static_cast<u32>(b->m_height)) {
            flag2 = 1;
        } else {
            flag2 = ((b->m_rowInts[ty]))[tx * 7];
        }
        if (!(flag2 & 0x80)) {
            return 0;
        }
        m_entranceActive = 1;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId(s_codeD);
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseWalk);
    }
    GruntDirectionCell cell = m_entranceCell;
    i32 col = cell.column + cell.row * 2;
    i32 base = cell.row + col;

    char* nm = m_cells[base].WalkName().GetBuffer(0);
    m_wwdObject->ApplyName(nm);
    return 0;
}

// @early-stop
RVA(0x00063b60, 0x1cf)
i32 CGrunt::StepArrivalReroll() {
    m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));
    i64 diff = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_struckClock.m_v;

    u32 elapsed;
    if (diff < 0) {
        elapsed = 0;
    } else {
        elapsed = static_cast<u32>(diff);
    }
    if (elapsed <= 0x2710) {
        return 0;
    }
    if (elapsed % 1000 != 0) {
        return 0;
    }
    i32 v;
    i32 range = 0x7531 - elapsed;
    u32 x;
    if (range == 0) {
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            x = ::timeGetTime();
        } else {
            x = g_randSeed;
        }
        g_randSeed = x * 214013 + 2531011;
        if (g_randSeed & 0x10000) {
            v = elapsed;
        } else {
            v = 0x7530;
        }
    } else {
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            x = ::timeGetTime();
        } else {
            x = g_randSeed;
        }
        g_randSeed = x * 214013 + 2531011;
        v = ((static_cast<i32>(g_randSeed) >> 16) & 0x7fff) % range + elapsed;
    }
    if (v <= 0x7148) {
        return 0;
    }
    u32 x2;
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        x2 = ::timeGetTime();
    } else {
        x2 = g_randSeed;
    }
    g_randSeed = x2 * 214013 + 2531011;
    i32 pick = ((static_cast<i32>(g_randSeed) >> 16) & 0x7fff) % 0x65;
    CWwdGameObjectA* h = m_object;
    i32 y = h->m_screenY;
    i32 xp = h->m_screenX;
    CGruntzMgr* g = g_gameReg;
    const RECT& r = g->m_world->m_level->m_mainPlane->m_viewRect;
    if (pick > 0x19) {
        if (xp < r.right && xp >= r.left && y < r.bottom && y >= r.top) {
            g->m_cueSink->SpawnVoiceDriver(this, 0x15d, -1, 0, -1, -1);
        }
    } else {
        if (xp < r.right && xp >= r.left && y < r.bottom && y >= r.top) {
            g->m_cueSink->LoadGruntSpawnConfig(this, 9, -1, -1, -1);
        }
    }
    return 0;
}

// @early-stop
RVA(0x00063db0, 0x32f)
i32 CGrunt::LoadVehicleGruntAnimations() {
    m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));

    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (sub->m_finished != 0 && sub->m_frameTicksLeft == 0) {
        if (m_arrived) {
            CreateHealthSprite();
            CreateStaminaSprite();
            CreateToySprite();
        }
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId(s_animKeyA);
        LoadGruntTypeTable(m_toolId, 1, 0, 0);
        m_entranceActive = 0;

        CMapMgr* grid = g_gameReg->m_tileGrid;
        i32 tx = m_lastTilePxX >> 5;
        i32 ty = m_lastTilePxY >> 5;
        i32 flags;
        if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
            || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
            flags = 1;
        } else {
            flags = ((grid->m_rowInts[ty]))[tx * 7];
        }
        if (flags & 0x80) {
            SetEntrancePos(1, 1);
            m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
        }
        return 0;
    }

    i64 elapsed = static_cast<i64>(static_cast<u64>(g_frameTime)) - m_toyClock;
    if (elapsed >= m_toyDuration) {
        if (m_entranceStamped == 0 && m_object->m_screenX == m_lastTilePxX
            && m_object->m_screenY == m_lastTilePxY) {
            if (m_toyTimeSprite) {
                m_toyTimeSprite->m_flags |= 0x10000;
                m_toyTimeSprite = 0;
            }
            SetEntrancePos(1, 1);
            m_entranceStamped = 1;
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseToy[GRUNT_TOY_BREAK], 0);

            CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
            CAniDesc* elem = desc->m_records.GetSize() > 0
                                 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0))
                                 : 0;

            i32 frame = elem->m_param;
            char* buf = (&m_448)->GetBuffer(0);
            m_wwdObject->ApplyLookupSprite(buf, frame);

            CWwdGameObjectA* h = m_object;
            CGruntzMgr* g = g_gameReg;
            i32 y = h->m_screenY;
            i32 x = h->m_screenX;
            const RECT& rect = g->m_world->m_level->m_mainPlane->m_viewRect;
            if (x < rect.right && x >= rect.left && y < rect.bottom && y >= rect.top) {
                g->m_cueSink->LoadGruntSpawnConfig(this, 0xc, -1, -1, -1);
                StopStruckSlotSound();
                return 0;
            }
        }
        StopStruckSlotSound();
        return 0;
    }

    i64 elapsed2 = static_cast<i64>(static_cast<u64>(g_frameTime)) - m_idleAnchor;
    if (elapsed2 >= m_idleDelay) {
        CWwdGameObjectA* h = m_object;
        CGruntzMgr* g = g_gameReg;
        i32 y = h->m_screenY;
        i32 x = h->m_screenX;
        const RECT& rect = g->m_world->m_level->m_mainPlane->m_viewRect;
        if (x < rect.right && x >= rect.left && y < rect.bottom && y >= rect.top) {
            g->m_cueSink->LoadGruntSpawnConfig(this, 0xd, -1, -1, -1);
        }
    }

    CWwdGameObjectA* h2 = m_object;
    CGruntzMgr* g2 = g_gameReg;
    i32 hy = h2->m_screenY;
    i32 hx = h2->m_screenX;
    if (hx < g2->m_viewBounds.right && hx >= g2->m_viewBounds.left && hy < g2->m_viewBounds.bottom
        && hy >= g2->m_viewBounds.top) {
        if (m_entranceReason == 0x1a) {
            EnsureStruckSlot(s_GRUNTZ_GOKARTGRUNT);
            return 0;
        }
        if (m_entranceReason == 0x19) {
            EnsureStruckSlot(s_GRUNTZ_BIGWHEELGRUNT);
            return 0;
        }
        return 0;
    }
    StopStruckSlotSound();
    return 0;
}

RVA(0x000641b0, 0x2c1)
i32 CGrunt::BuildGruntExitAnimation() {
    if (m_deathAnimStarted != 0) {
        return 0;
    }

    FinishActiveAction();
    StopStruckSlotSound();
    StopStruckVoiceSound();

    m_object->m_stateFlags &= ~8;
    m_entranceCommitted = 0;
    m_deathAnimStarted = 1;

    if (m_healthSprite) {
        m_healthSprite->m_flags |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_staminaSprite) {
        m_staminaSprite->m_flags |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_toySprite) {
        m_toySprite->m_flags |= 0x10000;
        m_toySprite = 0;
    }
    if (m_toyTimeSprite) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    if (m_wingzTimeSprite) {
        m_wingzTimeSprite->m_flags |= 0x10000;
        m_wingzTimeSprite = 0;
    }
    if (m_powerupSprite) {
        m_powerupSprite->m_flags |= 0x10000;
        m_powerupSprite = 0;
    }
    if (m_selectedSprite) {
        m_selectedSprite->m_flags |= 0x10000;
        m_selectedSprite = 0;
    }

    m_gruntKind = 0;
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
    }

    m_entranceActive = 1;
    m_tileMgr->RemoveCellRecord(m_tileOwnerHi, m_tileOwnerLo, 1);

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_exitKeyB);

    CAniElement* found;
    i32 r = GruntRand() % 0x1e1;
    if (r > 0x140) {
        found = static_cast<CAniElement*>(
            m_wwdObject->OwnerMgr()->m_animRegistry->LookupValue(s_GRUNTZ_EXITZ_ONE)
        );
        CGruntzMgr* g = g_gameReg;
        if (CGameLevel::PointInBounds(
                &g->m_world->m_level->m_mainPlane->m_viewRect,
                m_object->m_screenX,
                m_object->m_screenY
            )) {
            g->m_cueSink->SpawnVoiceDriver(this, 0x384, -1, 0, -1, -1);
        }
    } else if (r > 0xa0) {
        found = static_cast<CAniElement*>(
            m_wwdObject->OwnerMgr()->m_animRegistry->LookupValue(s_GRUNTZ_EXITZ_TWO)
        );
        CGruntzMgr* g = g_gameReg;
        if (CGameLevel::PointInBounds(
                &g->m_world->m_level->m_mainPlane->m_viewRect,
                m_object->m_screenX,
                m_object->m_screenY
            )) {
            g->m_cueSink->SpawnVoiceDriver(this, 0x385, -1, 0, -1, -1);
        }
    } else {
        found = static_cast<CAniElement*>(
            m_wwdObject->OwnerMgr()->m_animRegistry->LookupValue(s_GRUNTZ_EXITZ_THREE)
        );
        CGruntzMgr* g = g_gameReg;
        if (CGameLevel::PointInBounds(
                &g->m_world->m_level->m_mainPlane->m_viewRect,
                m_object->m_screenX,
                m_object->m_screenY
            )) {
            g->m_cueSink->SpawnVoiceDriver(this, 0x386, -1, 0, -1, -1);
        }
    }

    CWapX::Apply(found, 0);
    i32 frame =
        static_cast<CAniDesc*>(m_wwdObject->m_animCursor.m_animation->AtChecked(0))->m_param;
    m_wwdObject->ApplyLookupSprite(s_GRUNTZ_EXITZ, frame);
    return 0;
}

// @early-stop
RVA(0x00064540, 0x11c)
i32 CGrunt::StepWarpExit() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (sub->m_finished == 0) {
        return 0;
    }
    if (sub->m_frameTicksLeft != 0) {
        return 0;
    }
    if (m_deathType == GRUNT_DEATH_WARPOUT) {
        CState* st = g_gameReg->m_curState;
        i32 lvl = st->m_levelIndex + 0x64;
        CString s;
        s.Format("WORLDZ\\LEVEL%i", lvl);
        if (st->m_levelBank->ResolveQualified(static_cast<LPCTSTR>(s), REZ_TAG_WWD)) {
            PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, GOTOLEVEL, lvl);
        }
    }
    if (m_36c == 0) {
        m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 1);
    }
    m_wwdObject->m_flags |= 0x10000;
    return 0;
}

// @early-stop
RVA(0x000646b0, 0x9c8)
i32 CGrunt::StepCombatReaction(
    i32 attackKind,
    i32 struckPose,
    i32 srcRow,
    i32 srcCol,
    i32 srcPxX,
    i32 srcPxY,
    i32 fromProjectile,
    i32 attackerGruntKind
) {
    if (m_entranceCommitted == 0 || m_entranceDropActive != 0) {
        return 0;
    }
    {
        CWwdGameObjectA* h = m_object;
        i32 v = h->m_screenY + 0x186a0;
        if (h->m_sortKey != v) {
            h->m_sortKey = v;
            h->m_flags |= 0x20000;
        }
    }

    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "A") == 0) {
        goto tail;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeD) == 0) {
        goto tail;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "I") == 0) {
        if (m_entranceReason == 0x13) {
            g_gameReg->m_cueSink->StopVoice(m_object->m_188);
        }
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            -1
        );
        goto tail;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "G") == 0) {
        goto reject;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "L") == 0) {
        goto reject;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "P") == 0) {
        goto reject;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeO) == 0) {
        SnapToLastTile(1);
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
        goto tail;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeQ) == 0) {
        m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 6, srcRow);
        return 0;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "J") == 0) {
        m_entranceActive = 0;
        if (strcmp(*g_typeColl.GetNameRecord(m_prevAnimSetNode), s_codeD) == 0) {
            if (m_poweredUp != 0 && m_neighborValid == 0) {
                m_entranceActive = 0;
                m_combatActive = 0;
                m_neighborValid = 0;
                m_poweredUp = 0;
                ResetEntranceAnimation(1, 0, 0);
            }
            m_35c = 0;
            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = ActFindId(s_codeD);
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->m_animCursor.Setup(m_poseWalk);
            GruntDirectionCell cell = m_entranceCell;
            i32 col = cell.column + cell.row * 2;
            i32 base = cell.row + col;
            char* cn = m_cells[base].WalkName().GetBuffer(0);
            m_wwdObject->ApplyName(cn);
        } else {
            ResetEntranceAnimation(1, 0, 0);
        }
        i32 mode = m_moveMode;
        if (mode >= 0x32) {
            LoadGruntTypeTable(mode, 1, 0, 1);
            m_moveMode = -1;
            m_1a4 = 0;
        } else if (mode >= 0x22) {
            m_194 = mode;
            m_moveMode = -1;
        } else if (mode >= 0x17) {
            LoadVehicleGruntSprites(mode);
        } else {
            LoadGruntTypeTable(mode, 1, 0, 1);
            m_moveMode = -1;
        }
        goto tail;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeN) == 0) {
        CWwdGameObjectA* h = m_object;
        i32 hx = (h->m_screenX & ~0x1f) + 0x10;
        i32 hy = (h->m_screenY & ~0x1f) + 0x10;
        i32 flag = 1;
        if (hx != m_lastTilePxX || hy != m_lastTilePxY) {
            if (IsDropReady(1)) {
                m_coordToggle = (m_coordToggle == 0) ? 1 : 0;
                flag = 0;
            }
        }
        SnapToLastTile(1);
        if (flag != 0) {
            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = ActFindId(s_codeD);
        }
    }
    goto tail;

reject:
    if (m_entranceReason == 0x1e) {
        g_gameReg->m_cueSink->StopVoice(m_object->m_188);
    }
    LoadGruntTypeTable(m_toolId, 1, 0, 1);
    {
        CWwdGameObjectA* h = m_object;
        i32 v = h->m_screenY + 0x186a0;
        if (h->m_sortKey != v) {
            h->m_sortKey = v;
            h->m_flags |= 0x20000;
        }
    }
    if (m_toyTimeSprite != 0) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    m_toyTime = 0;
    StopStruckSlotSound();

tail:
    CreateHealthSprite();
    m_combatTimeoutLo = g_buteMgr.GetIntDef(s_Grunt, s_CombatTimeout, 0x1388);
    m_combatTimeoutHi = 0;
    m_combatClockLo = static_cast<i32>(g_frameTime);
    m_combatClockHi = 0;
    if (m_object->m_screenX != m_lastTilePxX || m_object->m_screenY != m_lastTilePxY) {
        ConsiderArrival(1);
    }
    if (LoadGruntCombatAnimations(
            attackKind,
            struckPose,
            srcRow,
            srcCol,
            srcPxX,
            srcPxY,
            fromProjectile,
            attackerGruntKind
        )
        == 0) {
        return 0;
    }

    {
        CString* rec = g_typeColl.ScratchResolve(m_objAux->m_1c);
        GruntScratchTeardown();
        if (strcmp(*rec, s_codeF) == 0) {
            if (m_entranceCommitted != 0) {
                return 0;
            }
        }
    }
    m_entranceActive = 1;
    {
        CString* rec = g_typeColl.ScratchResolve(m_objAux->m_1c);
        GruntScratchTeardown();
        if (strcmp(*rec, s_codeO) != 0) {
            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = ActFindId(s_codeH);
            void* cellObj = m_tileMgr->m_grid[srcRow * TM_GRID_COLS + srcCol];
            if (cellObj != 0) {
                CGameObject* oh = (static_cast<CGrunt*>(cellObj))->m_object;
                i32 cx = oh->m_screenX;
                i32 cy = oh->m_screenY;
                if (m_358 != 0 && m_entranceCommitted != 0 && RectContains(cx, cy)) {
                    if (!(s_TileFlags(g_gameReg->m_tileGrid, m_lastTilePxX >> 5, m_lastTilePxY >> 5)
                          & 0x80)) {
                        CommitNeighbor(srcRow, srcCol, cx, cy);
                    }
                }
            }
        }
    }

    m_combatActive = 0;
    CAniElement* pose = m_poseStruck[struckPose];
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(pose);
    i32 frame;
    {
        CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
        CAniDesc* elem;
        if (desc->m_records.GetSize() > 0) {
            elem = static_cast<CAniDesc*>(desc->m_records.GetAt(0));
        } else {
            elem = 0;
        }
        frame = elem->m_param;
    }
    {
        GruntDirectionCell cell = m_entranceCell;
        i32 col = cell.column + cell.row * 2;
        i32 base = cell.row + col;
        char* cn = m_cells[base].StruckName().GetBuffer(0);
        m_wwdObject->ApplyLookupSprite(cn, frame);
    }
    {
        CWwdGameObjectA* h = m_object;
        i32 vx = h->m_screenX;
        i32 vy = h->m_screenY;
        const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
        if (vx < rect->right && vx >= rect->left && vy < rect->bottom && vy >= rect->top) {
            g_gameReg->m_cueSink->LoadGruntSpawnConfig(this, 7, -1, -1, -1);
        }
    }
    return 0;
}

RVA(0x00065300, 0x148)
i32 CGrunt::StepArrivalCommitA() {
    m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (sub->m_finished == 0 || sub->m_frameTicksLeft != 0) {
        return 0;
    }
    if (m_health <= 0) {
        m_entranceCommitted = 0;
        m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 1, m_370);
        return 0;
    }
    m_entranceActive = 0;

    CMapMgr* grid = g_gameReg->m_tileGrid;
    i32 tx = m_lastTilePxX >> 5;
    i32 ty = m_lastTilePxY >> 5;
    i32 flags;
    if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
        flags = 1;
    } else {
        flags = ((grid->m_rowInts[ty]))[tx * 7];
    }
    if (flags & 0x80) {
        SetEntrancePos(1, 1);
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
        return 0;
    }
    if (m_358 == 0 && m_35c != 0) {
        StepArrivalDrop(m_commitPxX, m_commitPxY, 0, -1, 1, 0);
        return 0;
    }
    if (m_entranceReason == 0x14) {
        ResetEntranceAnimation(1, 0, 0);
        return 0;
    }
    ResetGeometry();
    return 0;
}

RVA(0x000654b0, 0x130)
i32 CGrunt::StepArrivalCommitB() {

    m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (sub->m_finished == 0 || sub->m_frameTicksLeft != 0) {
        return 0;
    }
    m_entranceActive = 0;
    SnapToLastTile(1);
    SetEntrancePos(1, 1);

    m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
    if (m_health <= 0) {
        m_entranceCommitted = 0;
        m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 1, m_370);
        return 0;
    }
    CGruntzMgr* g = g_gameReg;
    CMapMgr* grid = g->m_tileGrid;
    i32 tx = m_lastTilePxX >> 5;
    i32 ty = m_lastTilePxY >> 5;
    i32 flags;
    if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
        flags = 1;
    } else {
        flags = ((grid->m_rowInts[ty]))[tx * 7];
    }
    if (flags & 0x80) {
        return 0;
    }
    if (m_358 == 0 && m_35c != 0) {
        StepArrivalDrop(m_commitPxX, m_commitPxY, 0, -1, 1, 0);
        return 0;
    }
    ResetGeometry();
    return 0;
}

// @early-stop
RVA(0x00065630, 0x34b)
void CGrunt::RunMoveConfig(i32 a, i32 b) {
    i32 poseIdx = 0;

    i32 eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "I") == 0);
    if (eq) {
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            -1
        );
    } else {
        CWwdGameObjectA* h = m_object;
        CGruntzMgr* g = g_gameReg;
        const LevelCoordRect* bounds = &g->m_world->m_level->m_mainPlane->m_viewRect;
        if (CGameLevel::PointInBounds(bounds, h->m_screenX, h->m_screenY)) {
            g->m_cueSink->LoadGruntSpawnConfig(this, 8, -1, -1, -1);
        }
    }

    PlayMoveSoundAtTile(a, b);
    m_moveTileX = a;
    m_moveTileY = b;
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
    }

    if (m_entranceReason == 1) {
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId(s_codeM);
        m_object->m_stateFlags &= ~8;
        m_timePerTile = g_buteMgr.GetDwordDef(s_BOMBGRUNT, s_RunningTimePerTile, 0x64);
        m_entranceActive = 1;
        m_22c = 1;
        SetEntrancePos(1, 1);
    } else if (m_entranceReason == 0x12) {
        m_entranceActive = 1;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId(s_codeN);
        m_coordToggle = (m_coordToggle == 0);
    } else if (m_entranceReason == 0x13) {
        i32 base;
        if (GruntRand() % 0x64 < 0x50) {
            poseIdx = 1;
            base = 0x41a;
        } else {
            poseIdx = 0;
            base = 0x424;
        }

        i32 variant = m_374;
        m_moveVariant = variant;
        if (variant == 0) {
            i32 n = (g_gameReg->m_134 == 1) ? 3 : 6;
            m_moveVariant = GruntRand() % n + 1;
        }

        i32 cueId = base + m_moveVariant - 1;
        CWwdGameObjectA* h = m_object;
        CGruntzMgr* g = g_gameReg;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        const RECT& rect = g->m_world->m_level->m_mainPlane->m_viewRect;
        if (x < rect.right && x >= rect.left && y < rect.bottom && y >= rect.top) {
            g->m_cueSink->SpawnVoiceDriver(this, cueId, -1, 0, -1, -1);
        }

        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("I");
        m_entranceActive = 1;
        SetEntrancePos(1, 1);
    } else {
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("I");
        SetEntrancePos(1, 1);
    }

    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(m_poseItem[poseIdx]);

    GruntDirectionCell cell = m_entranceCell;
    i32 col = cell.column + cell.row * 2;
    i32 base = cell.row + col;
    char* name = m_cells[base].ItemName().GetBuffer(0);
    m_wwdObject->ApplyName(name);
}

// @early-stop
RVA(0x00065a60, 0x159)
i32 CGrunt::LoadWandGruntItemConfig() {
    i32 phase = m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    if (phase > 0) {
        if (phase == 0x63) {
            m_entranceActive = 1;
            u32 downtime =
                g_buteMgr.GetDword(static_cast<const char*>(m_animSetName), "ItemDowntime");
            if (m_gruntKind == GRUNT_ROIDZ) {
                downtime = 0;
            }
            m_attackDowntimeLo = downtime;
            m_attackDowntimeHi = 0;
            m_attackClockLo = g_frameTime;
            m_attackClockHi = 0;
            m_lowStaminaCued = 0;
            m_stamina = 0;
            if (m_healthSprite != 0) {
                CreateStaminaSprite();
            }
            if (m_entranceReason == 0x13) {
                LoadGruntAbilityTuning(m_moveVariant);
                i32 hp = m_health - g_buteMgr.GetIntDef("WANDGRUNT", "HealthLoss", 0x19);
                m_health = hp < 0 ? 0 : hp;
                if (m_health <= 0) {
                    m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 1, -1);
                }
            }
        }
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            phase
        );
    }
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (sub->m_finished != 0 && sub->m_frameTicksLeft == 0) {
        m_entranceActive = 0;
        ResetEntranceAnimation(1, 0, 0);
    }
    return 0;
}

// @early-stop
RVA(0x00065c20, 0x1d5)
i32 CGrunt::StepEntranceRelatchB() {
    i32 ready = m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));
    if (ready > 0) {
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            ready
        );
    }
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (sub->m_finished == 0 || sub->m_frameTicksLeft != 0) {
        return 0;
    }
    m_entranceActive = 0;
    if (m_arrived != 0) {
        CreateHealthSprite();
        CreateStaminaSprite();
        CreateToySprite();
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_codeD);
    CGruntzMgr* g = g_gameReg;
    CMapMgr* grid = g->m_tileGrid;
    i32 tx = m_lastTilePxX >> 5;
    i32 ty = m_lastTilePxY >> 5;
    i32 f1;
    if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
        f1 = 1;
    } else {
        f1 = ((grid->m_rowInts[ty]))[tx * 7];
    }
    if (f1 & 0x2000000) {
        BuildGruntLoseItemAnimation();
        g = g_gameReg;
    }
    grid = g->m_tileGrid;
    void* cellObj;
    if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
        cellObj = 0;
    } else {

        AddrWord<char> slot;
        slot.m_word = ((grid->m_rowInts[ty]))[tx * 7 + 2];
        cellObj = slot.m_addr;
    }
    if (cellObj == 0) {
        return 0;
    }
    CGameObject* found = 0;
    CGameObject* result = 0;
    if (MapLookup(g->m_world->m_childGroup->m_map48, cellObj, found)) {
        result = found;
    }
    if (result != 0) {

        CInGameIcon* icon = static_cast<CInGameIcon*>(result->m_animWorker->m_logic);
        icon->PlaceAt(m_tileOwnerHi, m_tileOwnerLo);
        return 0;
    }
    grid = g_gameReg->m_tileGrid;
    if (static_cast<u32>(tx) < static_cast<u32>(grid->m_width)
        && static_cast<u32>(ty) < static_cast<u32>(grid->m_height)) {
        ((grid->m_rowInts[ty]))[tx * 7 + 2] = 0;
        ((grid->m_rowInts[ty]))[tx * 7] &= ~0x40000;
    }
    return 0;
}

RVA(0x0006b2e0, 0x39)
void CWapX::Apply(CAniElement* a, i32 b) {
    m_value = m_wwdObject->m_animCursor.m_animation;
    CAniAdvanceCursor* anim = &m_wwdObject->m_animCursor;
    anim->Setup(a);
    if (b != 0) {
        anim->Advance(static_cast<i32>(g_engineFrameDelta));
    }
}
