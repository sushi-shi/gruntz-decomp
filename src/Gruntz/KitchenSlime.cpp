#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/KitchenSlime.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CardinalDir.h>
#include <Gruntz/CardinalDirectionOffset.h>
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
#include <MakeRect.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

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

RVA_COMPGEN(0x000130d0, 0x1e, ??_GCKitchenSlime@@UAEPAXI@Z)
RVA_COMPGEN(0x00013100, 0x44, ??1CKitchenSlime@@UAE@XZ)

// @early-stop
RVA(0x000b23a0, 0x3f8)
CKitchenSlime::CKitchenSlime(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);

    SNAP_OBJECT_TO_TILE_CENTER_DOUBLE_POS(m_object, snapX, snapY, m_position.m_x, m_position.m_y)
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_KITCHEN_SLIME)
    m_tilePosition.m_y = snapY;
    m_tilePosition.m_x = snapX;

    m_object->m_speed.m_x = (m_object->m_speed.m_x << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_object->m_speed.m_y = (m_object->m_speed.m_y << TILE_SHIFT_PX) + TILE_HALF_PX;
    if (m_object->m_screenPosition.m_x == m_object->m_speed.m_x
        && m_object->m_screenPosition.m_y == m_object->m_speed.m_y) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        return;
    }
    m_object->m_extent.left = (m_object->m_screenPosition.m_x < m_object->m_speed.m_x)
                                  ? m_object->m_screenPosition.m_x
                                  : m_object->m_speed.m_x;

    i32 exRight = m_object->m_speed.m_x;
    if (m_object->m_screenPosition.m_x > exRight) {
        exRight = m_object->m_screenPosition.m_x;
    }
    m_object->m_extent.right = exRight;
    i32 exTop = m_object->m_speed.m_y;
    if (m_object->m_screenPosition.m_y < exTop) {
        exTop = m_object->m_screenPosition.m_y;
    }
    m_object->m_extent.top = exTop;
    i32 exBottom = m_object->m_speed.m_y;
    if (m_object->m_screenPosition.m_y > exBottom) {
        exBottom = m_object->m_screenPosition.m_y;
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

RVA(0x000b2940, 0x102)
void CKitchenSlime::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

RVA(0x000b2aa0, 0x18d)
void CKitchenSlime::RegisterType() {
    ACT_NAME_ID(id, "A")

    CActRegPool<CKitchenSlime>::s_table[id] = static_cast<CActHandler>(&CKitchenSlime::Tick);
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
    if (lvl->m_screenPosition.m_x == m_tilePosition.m_x
        && lvl->m_screenPosition.m_y == m_tilePosition.m_y && LoadSprites() == 0) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        return 0;
    }

    double step = static_cast<double>(g_frameDelta) * m_speed;

    i32 newX;
    if (m_direction.m_x > 0.0) {
        double t = (m_position.m_x = m_position.m_x + step);
        newX = static_cast<i32>(floor(t));
        i32 tx = m_tilePosition.m_x;
        m_stepMag = fabs(m_position.m_x - static_cast<double>(tx));

        CLAMP_UPPER_INPLACE(newX, tx);
    } else if (m_direction.m_x < 0.0) {
        double t = (m_position.m_x = m_position.m_x - step);
        newX = static_cast<i32>(ceil(t));
        i32 tx = m_tilePosition.m_x;
        m_stepMag = fabs(m_position.m_x - static_cast<double>(tx));
        if (newX < tx) {
            newX = tx;
        }
    } else {
        newX = static_cast<i32>(floor(m_position.m_x));
    }

    i32 newY;
    if (m_direction.m_y > 0.0) {
        double t = (m_position.m_y = m_position.m_y + step);
        newY = static_cast<i32>(floor(t));
        i32 ty = m_tilePosition.m_y;
        m_stepMag = fabs(m_position.m_y - static_cast<double>(ty));
        CLAMP_UPPER_INPLACE(newY, ty);
    } else if (m_direction.m_y < 0.0) {
        double t = (m_position.m_y = m_position.m_y - step);
        newY = static_cast<i32>(ceil(t));
        i32 ty = m_tilePosition.m_y;
        m_stepMag = fabs(m_position.m_y - static_cast<double>(ty));
        if (newY < ty) {
            newY = ty;
        }
    } else {
        newY = static_cast<i32>(floor(m_position.m_y));
    }

    SET_SCREEN_POS(Level(), newX, newY);
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
            s->Read(&m_position.m_x, sizeof(m_position.m_x));
            s->Read(&m_position.m_y, sizeof(m_position.m_y));
            s->Read(&m_direction.m_x, sizeof(m_direction.m_x));
            s->Read(&m_direction.m_y, sizeof(m_direction.m_y));
            s->Read(&m_tilePosition, sizeof(m_tilePosition));
            s->Read(&m_stepMag, sizeof(m_stepMag));
        }
    } else {
        s->Write(&m_speed, sizeof(m_speed));
        s->Write(&m_position.m_x, sizeof(m_position.m_x));
        s->Write(&m_position.m_y, sizeof(m_position.m_y));
        s->Write(&m_direction.m_x, sizeof(m_direction.m_x));
        s->Write(&m_direction.m_y, sizeof(m_direction.m_y));
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
        i32 sw = lvl->m_smarts;
        switch (static_cast<CardinalDir>(sw)) {
            case CARDINAL_NORTH: {
                Coord step;
                step.Set(m_tilePosition.m_x, m_tilePosition.m_y - 0x20);
                tile = step;
                break;
            }
            case CARDINAL_EAST: {
                Coord step;
                step.Set(m_tilePosition.m_x + 0x20, m_tilePosition.m_y);
                tile = step;
                break;
            }
            case CARDINAL_SOUTH: {
                Coord step;
                step.Set(m_tilePosition.m_x, m_tilePosition.m_y + 0x20);
                tile = step;
                break;
            }
            case CARDINAL_WEST: {
                Coord step;
                step.Set(m_tilePosition.m_x - 0x20, m_tilePosition.m_y);
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

    m_position.m_x = 0;
    m_position.m_y = 0;
    b32 changed = (Level()->m_smarts != savedDir);
    switch (static_cast<CardinalDir>(Level()->m_smarts)) {
        case CARDINAL_NORTH:
            m_direction.m_x = 0.0;
            m_direction.m_y = -1.0;
            m_position.m_y = -m_stepMag;
            if (changed) {
                Anim()->SetImageSetByName("LEVEL_KITCHENSLIME_NORTH");
            }
            break;
        case CARDINAL_EAST:
            m_direction.m_x = 1.0;
            m_direction.m_y = 0.0;
            m_position.m_x = m_stepMag;
            if (changed) {
                Anim()->SetImageSetByName("LEVEL_KITCHENSLIME_EAST");
            }
            break;
        case CARDINAL_SOUTH:
            m_direction.m_x = 0.0;
            m_direction.m_y = 1.0;
            m_position.m_y = m_stepMag;
            if (changed) {
                Anim()->SetImageSetByName("LEVEL_KITCHENSLIME_SOUTH");
            }
            break;
        case CARDINAL_WEST:
            m_direction.m_x = -1.0;
            m_direction.m_y = 0.0;
            m_position.m_x = -m_stepMag;
            if (changed) {
                Anim()->SetImageSetByName("LEVEL_KITCHENSLIME_WEST");
            }
            break;
    }

    m_position.m_x = static_cast<double>(Level()->m_screenPosition.m_x) + m_position.m_x;
    m_position.m_y = static_cast<double>(Level()->m_screenPosition.m_y) + m_position.m_y;

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
            CImage* img = spr->GetAt(1);
            player->m_frameIndex = 1;
            player->m_frameImage = img;
        }
    }
    m_stepMag = 0.0;
    return 1;
}
