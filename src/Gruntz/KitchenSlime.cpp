#include <rva.h>

#include <Gruntz/KitchenSlime.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CardinalDirectionOffset.h>
#include <Gruntz/CardinalDir.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <MakeRect.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

#include <math.h>
#include <string.h>

DATA(0x001ea3e0)
const double g_slimeSpeedNum = 32.0;

RVA_DYNINIT(0x000b28a0, 0xa, CActRegPool<CKitchenSlime>::s_table)
RVA_DYNINIT(0x000b28c0, 0x15, CActRegPool<CKitchenSlime>::s_table)
RVA_DYNINIT(0x000b28f0, 0xe, CActRegPool<CKitchenSlime>::s_table)
RVA_DYNINIT(0x000b2910, 0x1f, CActRegPool<CKitchenSlime>::s_table)
template<> DATA(0x00246228)
CActReg CActRegPool<CKitchenSlime>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

DATA(0x0021aea8)
i32 g_typeCounter = ACT_ID_FIRST;

static inline CActHandler* KSlimeLookup(i32 coord) {
    return (CActRegPool<CKitchenSlime>::s_table.ResolveEntry(coord));
}

RVA_COMPGEN(0x000130d0, 0x1e, ??_GCKitchenSlime@@UAEPAXI@Z)
RVA_COMPGEN(0x00013100, 0x44, ??1CKitchenSlime@@UAE@XZ)

// @early-stop
RVA(0x000b23a0, 0x3f8)
CKitchenSlime::CKitchenSlime(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);

    Coord snappedPosition = m_object->ScreenPos();
    SnapTileCenter(&snappedPosition);
    m_object->SetScreenPos(snappedPosition);
    m_position.Init(snappedPosition);
    CWwdSpriteObject* o = m_object;
    o->SetSortKey(SORTKEY_KITCHEN_SLIME);
    m_tilePosition = snappedPosition;

    TileCenter(&m_object->m_speed);
    Coord target = m_object->m_speed;
    if (m_object->ScreenPos() == target) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        return;
    }
    Coord extentLo = snappedPosition;
    extentLo.Min(target);
    Coord extentHi = snappedPosition;
    extentHi.Max(target);
    m_object->m_extent = MakeRect(extentLo.m_x, extentLo.m_y, extentHi.m_x, extentHi.m_y);

    CDDrawWorker* frameSet = Anim()->m_imageSet;
    if (frameSet != NULL) {
        CString name;
        name = frameSet->m_name;
        const char* s = static_cast<LPCTSTR>(name);
        if (strcmp(s, "LEVEL_KITCHENSLIME_NORTH") == 0) {
            m_object->m_smarts = IDX(CARDINAL_NORTH);
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_EAST") == 0) {
            m_object->m_smarts = IDX(CARDINAL_EAST);
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_SOUTH") == 0) {
            m_object->m_smarts = IDX(CARDINAL_SOUTH);
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_WEST") == 0) {
            m_object->m_smarts = IDX(CARDINAL_WEST);
        }
    }

    m_stepMag = 0.0;
    if (LoadSprites() == 0) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    }
    SET_ANIMATION_ACT("A");
    SwitchAnimationByName("GAME_CYCLE100", 0);
    ClearObjectArea();
}

RVA(0x000b2940, 0x102)
void CKitchenSlime::FireActivation(i32 coord) {
    CActHandler* e = KSlimeLookup(coord);
    if ((*e) != NULL) {
        CActHandler* e2 = KSlimeLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x000b2aa0, 0x18d)
void CKitchenSlime::RegisterType() {
    ACT_NAME_ID(id, "A")

    *KSlimeLookup(id) = static_cast<CActHandler>(&CKitchenSlime::Tick);
}

// @early-stop
RVA(0x000b2ca0, 0x29c)
i32 CKitchenSlime::Tick() {
    m_wwdObject->m_animationCursor.Advance(static_cast<i32>(g_engineFrameDelta));

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode == false || reg->m_gameMode != GAMEMODE_QUESTZ) {
        CGameObject* lvl = Level();
        i32 playerIndex, unitIndex;
        CGrunt* ent = static_cast<CGrunt*>(reg->m_triggerMgr->FindGruntAt(
            lvl->m_screenPosition.m_x,
            lvl->m_screenPosition.m_y,
            &lvl->m_area,
            &playerIndex,
            &unitIndex,
            static_cast<RECT*>(0)
        ));
        if (ent && ent->m_gruntKind != GRUNT_INVULNERABLE) {
            (static_cast<CTriggerMgr*>(g_gameReg->m_triggerMgr))
                ->StartUnitDeath(playerIndex, unitIndex, DEATH_MELT, -1);
        }
    }

    CGameObject* lvl = Level();
    if (lvl->ScreenPos() == m_tilePosition && LoadSprites() == 0) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        return 0;
    }

    double step = static_cast<double>(g_frameDelta) * m_speed;

    Coord next;
    if (m_direction.x > 0.0) {
        double t = (m_position.x = m_position.x + step);
        next.m_x = static_cast<i32>(floor(t));
        m_stepMag = fabs(m_position.x - static_cast<double>(m_tilePosition.m_x));

        if (next.m_x > m_tilePosition.m_x) {
            next.m_x = m_tilePosition.m_x;
        }
    } else if (m_direction.x < 0.0) {
        double t = (m_position.x = m_position.x - step);
        next.m_x = static_cast<i32>(ceil(t));
        m_stepMag = fabs(m_position.x - static_cast<double>(m_tilePosition.m_x));
        if (next.m_x < m_tilePosition.m_x) {
            next.m_x = m_tilePosition.m_x;
        }
    } else {
        next.m_x = static_cast<i32>(floor(m_position.x));
    }

    if (m_direction.y > 0.0) {
        double t = (m_position.y = m_position.y + step);
        next.m_y = static_cast<i32>(floor(t));
        m_stepMag = fabs(m_position.y - static_cast<double>(m_tilePosition.m_y));
        if (next.m_y > m_tilePosition.m_y) {
            next.m_y = m_tilePosition.m_y;
            Level()->SetScreenPos(next);
            return 0;
        }
    } else if (m_direction.y < 0.0) {
        double t = (m_position.y = m_position.y - step);
        next.m_y = static_cast<i32>(ceil(t));
        m_stepMag = fabs(m_position.y - static_cast<double>(m_tilePosition.m_y));
        if (next.m_y < m_tilePosition.m_y) {
            next.m_y = m_tilePosition.m_y;
            Level()->SetScreenPos(next);
            return 0;
        }
    } else {
        next.m_y = static_cast<i32>(floor(m_position.y));
    }

    Level()->SetScreenPos(next);
    return 0;
}

RVA(0x000b2ff0, 0x11b)
i32 CKitchenSlime::SerializeDispatch(
    CFileMemBase* stream,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    CFileMemBase* s = stream;

    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            s->Read(&m_speed, sizeof(m_speed));
            s->Read(&m_position.x, sizeof(m_position.x));
            s->Read(&m_position.y, sizeof(m_position.y));
            s->Read(&m_direction.x, sizeof(m_direction.x));
            s->Read(&m_direction.y, sizeof(m_direction.y));
            s->Read(&m_tilePosition, sizeof(m_tilePosition));
            s->Read(&m_stepMag, sizeof(m_stepMag));
        }
    } else {
        s->Write(&m_speed, sizeof(m_speed));
        s->Write(&m_position.x, sizeof(m_position.x));
        s->Write(&m_position.y, sizeof(m_position.y));
        s->Write(&m_direction.x, sizeof(m_direction.x));
        s->Write(&m_direction.y, sizeof(m_direction.y));
        s->Write(&m_tilePosition, sizeof(m_tilePosition));
        s->Write(&m_stepMag, sizeof(m_stepMag));
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(stream, mode, typeId, object)
}

// @early-stop
RVA(0x000b3160, 0x35c)
i32 CKitchenSlime::LoadSprites() {
    i32 savedDir = Level()->m_smarts;

    Coord tile;
    b32 found = false;
    i32 i = 0;
    while (found == false) {
        CGameObject* lvl = Level();
        CardinalDir direction = static_cast<CardinalDir>(lvl->m_smarts);
        tile = m_tilePosition + CardinalDirectionOffset(direction, TILE_SIZE_PX);

        Coord tileCell = tile;
        ScreenTile(&tileCell);
        CMapMgr* map = g_gameReg->m_tileGrid;
        i32 tileFlags = map->CellFlagsAt(tileCell.m_x, tileCell.m_y);

        if (tile.m_y >= lvl->m_extent.top && tile.m_x <= lvl->m_extent.right
            && tile.m_y <= lvl->m_extent.bottom && tile.m_x >= lvl->m_extent.left
            && !(tileFlags & BRICKZ_BLOCKED_MASK) && !(tileFlags & IDX(CELL_FLAG_SPECIAL))) {
            found = true;
        } else {
            if (++i > 4) {
                return 0;
            }

            if (lvl->m_direction == 1) {
                lvl->m_smarts = IDX(direction) - 1;
                if (Level()->m_smarts <= 0) {
                    Level()->m_smarts = 4;
                }
            } else {
                lvl->m_smarts++;
                if (Level()->m_smarts > 4) {
                    Level()->m_smarts = 1;
                }
            }
        }
    }

    m_position.Init();
    b32 changed = (Level()->m_smarts != savedDir);
    CardinalDir direction = static_cast<CardinalDir>(Level()->m_smarts);
    if (direction >= CARDINAL_FIRST && direction <= CARDINAL_LAST) {
        Coord stepDirection = CardinalDirectionOffset(direction, 1);
        m_direction = DoubleVector2(stepDirection);
        m_position = m_direction * m_stepMag;
    }
    switch (direction) {
        case CARDINAL_NORTH:
            if (changed) {
                Anim()->SetImageSetByName("LEVEL_KITCHENSLIME_NORTH");
            }
            break;
        case CARDINAL_EAST:
            if (changed) {
                Anim()->SetImageSetByName("LEVEL_KITCHENSLIME_EAST");
            }
            break;
        case CARDINAL_SOUTH:
            if (changed) {
                Anim()->SetImageSetByName("LEVEL_KITCHENSLIME_SOUTH");
            }
            break;
        case CARDINAL_WEST:
            if (changed) {
                Anim()->SetImageSetByName("LEVEL_KITCHENSLIME_WEST");
            }
            break;
    }

    Coord screenPosition = Level()->ScreenPos();
    DoubleVector2 origin(screenPosition);
    m_position += origin;

    u32 time;
    if (Level()->m_logicRecord->m_speed != 0) {
        time = Level()->m_logicRecord->m_speed;
    } else {
        time = g_buteMgr.GetDword("Hazardz", "KitchenSlimeTimePerTile", 1000);
    }

    m_tilePosition = tile;
    m_speed = g_slimeSpeedNum / static_cast<double>(time);

    if (changed != false) {
        CWwdSpriteObject* player = Anim();
        CDDrawWorker* spr = player->m_imageSet;
        if (spr != NULL) {
            if (spr->ContainsFrame(1)) {
                CImage* img = spr->FrameAtUnchecked(1);
                player->m_frameIndex = 1;
                player->m_frameImage = img;
                m_stepMag = 0.0;
                return 1;
            }
            player->m_frameIndex = 1;
            player->m_frameImage = NULL;
            m_stepMag = 0.0;
            return 1;
        }
    }
    m_stepMag = 0.0;
    return 1;
}
