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
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

#include <math.h>
#include <string.h>

DATA(0x001ea3e0)
const double g_slimeSpeedNum = 32.0;

RVA_DYNINIT(0x000b2890, 0xa, CActRegPool<CKitchenSlime>::s_table)
RVA_DYNINIT(0x000b28b0, 0x15, CActRegPool<CKitchenSlime>::s_table)
RVA_DYNINIT(0x000b28e0, 0xe, CActRegPool<CKitchenSlime>::s_table)
RVA_DYNINIT(0x000b2900, 0x1f, CActRegPool<CKitchenSlime>::s_table)
template<> DATA(0x00247180)
CActReg CActRegPool<CKitchenSlime>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

DATA(0x0021be08)
i32 g_typeCounter = ACT_ID_FIRST;

static inline CActHandler* KSlimeLookup(i32 coord) {
    return (CActRegPool<CKitchenSlime>::s_table.ResolveEntry(coord));
}

RVA_COMPGEN(0x000130e0, 0x1e, ??_GCKitchenSlime@@UAEPAXI@Z)
RVA_COMPGEN(0x00013110, 0x44, ??1CKitchenSlime@@UAE@XZ)

// @early-stop
RVA(0x000b2390, 0x3f8)
CKitchenSlime::CKitchenSlime(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);

    SNAP_OBJECT_TO_TILE_CENTER_DOUBLE_POS(m_object, snapX, snapY, m_posX, m_posY)
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_KITCHEN_SLIME)
    m_tilePosition.m_y = snapY;
    m_tilePosition.m_x = snapX;

    m_object->m_speedX = (m_object->m_speedX << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_object->m_speedY = (m_object->m_speedY << TILE_SHIFT_PX) + TILE_HALF_PX;
    if (m_object->m_screenX == m_object->m_speedX && m_object->m_screenY == m_object->m_speedY) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        return;
    }
    m_object->m_extent.left =
        (m_object->m_screenX < m_object->m_speedX) ? m_object->m_screenX : m_object->m_speedX;

    i32 exRight = m_object->m_speedX;
    if (m_object->m_screenX > exRight) {
        exRight = m_object->m_screenX;
    }
    m_object->m_extent.right = exRight;
    i32 exTop = m_object->m_speedY;
    if (m_object->m_screenY < exTop) {
        exTop = m_object->m_screenY;
    }
    m_object->m_extent.top = exTop;
    i32 exBottom = m_object->m_speedY;
    if (m_object->m_screenY > exBottom) {
        exBottom = m_object->m_screenY;
    }
    m_object->m_extent.bottom = exBottom;

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
    CLEAR_OBJECT_AREA
}

RVA(0x000b2930, 0x102)
void CKitchenSlime::FireActivation(i32 coord) {
    CActHandler* e = KSlimeLookup(coord);
    if ((*e) != NULL) {
        CActHandler* e2 = KSlimeLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x000b2a90, 0x18d)
void CKitchenSlime::RegisterType() {
    ACT_NAME_ID(id, "A")

    *KSlimeLookup(id) = static_cast<CActHandler>(&CKitchenSlime::Tick);
}

// @early-stop
RVA(0x000b2c90, 0x29c)
i32 CKitchenSlime::Tick() {
    m_wwdObject->m_animationCursor.Advance(static_cast<i32>(g_engineFrameDelta));

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode == false || reg->m_gameMode != GAMEMODE_QUESTZ) {
        CGameObject* lvl = Level();
        i32 playerIndex, unitIndex;
        CGrunt* ent = static_cast<CGrunt*>(reg->m_triggerMgr->FindGruntAt(
            lvl->m_screenX,
            lvl->m_screenY,
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
    if (lvl->m_screenX == m_tilePosition.m_x && lvl->m_screenY == m_tilePosition.m_y
        && LoadSprites() == 0) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        return 0;
    }

    double step = static_cast<double>(g_frameDelta) * m_speed;

    i32 newX;
    if (m_dirX > 0.0) {
        double t = (m_posX = m_posX + step);
        newX = static_cast<i32>(floor(t));
        i32 tx = m_tilePosition.m_x;
        m_stepMag = fabs(m_posX - static_cast<double>(tx));

        if (newX > tx) {
            newX = tx;
        }
    } else if (m_dirX < 0.0) {
        double t = (m_posX = m_posX - step);
        newX = static_cast<i32>(ceil(t));
        i32 tx = m_tilePosition.m_x;
        m_stepMag = fabs(m_posX - static_cast<double>(tx));
        if (newX < tx) {
            newX = tx;
        }
    } else {
        newX = static_cast<i32>(floor(m_posX));
    }

    i32 newY;
    if (m_dirY > 0.0) {
        double t = (m_posY = m_posY + step);
        newY = static_cast<i32>(floor(t));
        i32 ty = m_tilePosition.m_y;
        m_stepMag = fabs(m_posY - static_cast<double>(ty));
        if (newY > ty) {
            Level()->m_screenX = newX;
            Level()->m_screenY = ty;
            return 0;
        }
    } else if (m_dirY < 0.0) {
        double t = (m_posY = m_posY - step);
        newY = static_cast<i32>(ceil(t));
        i32 ty = m_tilePosition.m_y;
        m_stepMag = fabs(m_posY - static_cast<double>(ty));
        if (newY < ty) {
            Level()->m_screenX = newX;
            Level()->m_screenY = ty;
            return 0;
        }
    } else {
        newY = static_cast<i32>(floor(m_posY));
    }

    Level()->m_screenX = newX;
    Level()->m_screenY = newY;
    return 0;
}

RVA(0x000b2fe0, 0x11b)
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
            s->Read(&m_posX, sizeof(m_posX));
            s->Read(&m_posY, sizeof(m_posY));
            s->Read(&m_dirX, sizeof(m_dirX));
            s->Read(&m_dirY, sizeof(m_dirY));
            s->Read(&m_tilePosition, sizeof(m_tilePosition));
            s->Read(&m_stepMag, sizeof(m_stepMag));
        }
    } else {
        s->Write(&m_speed, sizeof(m_speed));
        s->Write(&m_posX, sizeof(m_posX));
        s->Write(&m_posY, sizeof(m_posY));
        s->Write(&m_dirX, sizeof(m_dirX));
        s->Write(&m_dirY, sizeof(m_dirY));
        s->Write(&m_tilePosition, sizeof(m_tilePosition));
        s->Write(&m_stepMag, sizeof(m_stepMag));
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(stream, mode, typeId, object)
}

// @early-stop
RVA(0x000b3150, 0x35c)
i32 CKitchenSlime::LoadSprites() {
    i32 savedDir = Level()->m_smarts;

    Coord tile;
    b32 found = false;
    i32 i = 0;
    while (found == false) {
        CGameObject* lvl = Level();
        i32 sw = lvl->m_smarts;
        switch (static_cast<CardinalDir>(sw)) {
            case CARDINAL_NORTH: {
                Coord step;
                step.m_x = m_tilePosition.m_x;
                step.m_y = m_tilePosition.m_y - 0x20;
                tile = step;
                break;
            }
            case CARDINAL_EAST: {
                Coord step;
                step.m_x = m_tilePosition.m_x + 0x20;
                step.m_y = m_tilePosition.m_y;
                tile = step;
                break;
            }
            case CARDINAL_SOUTH: {
                Coord step;
                step.m_x = m_tilePosition.m_x;
                step.m_y = m_tilePosition.m_y + 0x20;
                tile = step;
                break;
            }
            case CARDINAL_WEST: {
                Coord step;
                step.m_x = m_tilePosition.m_x - 0x20;
                step.m_y = m_tilePosition.m_y;
                tile = step;
                break;
            }
        }

        i32 gx = tile.m_x >> TILE_SHIFT_PX;
        i32 gy = tile.m_y >> TILE_SHIFT_PX;
        CMapMgr* map = g_gameReg->m_tileGrid;
        i32 tileFlags = map->CellFlagsAt(gx, gy);

        if (tile.m_y >= lvl->m_extent.top && tile.m_x <= lvl->m_extent.right
            && tile.m_y <= lvl->m_extent.bottom && tile.m_x >= lvl->m_extent.left
            && !(tileFlags & BRICKZ_BLOCKED_MASK) && !(tileFlags & IDX(CELL_FLAG_SPECIAL))) {
            found = true;
        } else {
            if (++i > 4) {
                return 0;
            }

            if (lvl->m_direction == 1) {
                lvl->m_smarts = sw - 1;
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

    m_posX = 0;
    m_posY = 0;
    b32 changed = (Level()->m_smarts != savedDir);
    switch (static_cast<CardinalDir>(Level()->m_smarts)) {
        case CARDINAL_NORTH:
            m_dirX = 0.0;
            m_dirY = -1.0;
            m_posY = -m_stepMag;
            if (changed) {
                Anim()->SetImageSetByName("LEVEL_KITCHENSLIME_NORTH");
            }
            break;
        case CARDINAL_EAST:
            m_dirX = 1.0;
            m_dirY = 0.0;
            m_posX = m_stepMag;
            if (changed) {
                Anim()->SetImageSetByName("LEVEL_KITCHENSLIME_EAST");
            }
            break;
        case CARDINAL_SOUTH:
            m_dirX = 0.0;
            m_dirY = 1.0;
            m_posY = m_stepMag;
            if (changed) {
                Anim()->SetImageSetByName("LEVEL_KITCHENSLIME_SOUTH");
            }
            break;
        case CARDINAL_WEST:
            m_dirX = -1.0;
            m_dirY = 0.0;
            m_posX = -m_stepMag;
            if (changed) {
                Anim()->SetImageSetByName("LEVEL_KITCHENSLIME_WEST");
            }
            break;
    }

    m_posX = static_cast<double>(Level()->m_screenX) + m_posX;
    m_posY = static_cast<double>(Level()->m_screenY) + m_posY;

    u32 time;
    if (Level()->m_logicRecord->m_speed != 0) {
        time = Level()->m_logicRecord->m_speed;
    } else {
        time = g_buteMgr.GetDwordDef("Hazardz", "KitchenSlimeTimePerTile", 1000);
    }

    m_tilePosition = tile;
    m_speed = g_slimeSpeedNum / static_cast<double>(time);

    if (changed != false) {
        CWwdSpriteObject* player = Anim();
        CDDrawWorker* spr = player->m_imageSet;
        if (spr != NULL) {
            if (DDRAW_WORKER_CONTAINS_FRAME(spr, 1)) {
                CImage* img = DDRAW_WORKER_FRAME_AT_UNCHECKED(spr, 1);
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
