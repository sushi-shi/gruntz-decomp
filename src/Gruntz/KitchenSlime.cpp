#include <rva.h>

#include <Gruntz/KitchenSlime.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <Enums.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/CardinalDir.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Sprite.h>
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

VTBL(CKitchenSlime, 0x001e750c);

DATA(0x001ea3e0)
const double g_slimeSpeedNum = 32.0;

template<> DATA(0x00246228)
CActReg CActRegPool<CKitchenSlime>::s_table(2000, 2010);

DATA(0x0021aea8)
i32 g_typeCounter = 2000;

static inline CActHandler* KSlimeLookup(i32 coord) {
    return (CActRegPool<CKitchenSlime>::s_table.ResolveEntry(coord));
}

RVA_COMPGEN(0x000130d0, 0x1e, ??_GCKitchenSlime@@UAEPAXI@Z)
RVA_COMPGEN(0x00013100, 0x44, ??1CKitchenSlime@@UAE@XZ)

// @early-stop
RVA(0x000b23a0, 0x3f8)
CKitchenSlime::CKitchenSlime(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 0x2000002;

    i32 snapX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 snapY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_object->m_screenX = snapX;
    m_posX = static_cast<double>(snapX);
    m_object->m_screenY = snapY;
    m_posY = static_cast<double>(snapY);
    if (m_object->m_sortKey != 0x13) {
        m_object->m_sortKey = 0x13;
        m_object->m_flags |= 0x20000;
    }
    m_tilePosition.m_y = snapY;
    m_tilePosition.m_x = snapX;

    m_object->m_speedX = (m_object->m_speedX << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_object->m_speedY = (m_object->m_speedY << TILE_SHIFT_PX) + TILE_HALF_PX;
    if (m_object->m_screenX == m_object->m_speedX && m_object->m_screenY == m_object->m_speedY) {
        m_wwdObject->m_flags |= 0x10000;
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

    CDDrawWorker* frameSet = Anim()->m_frameSet;
    if (frameSet != NULL) {
        CString name;
        name = frameSet->m_name;
        const char* s = static_cast<LPCTSTR>(name);
        if (strcmp(s, "LEVEL_KITCHENSLIME_NORTH") == 0) {
            m_object->m_smarts = CARDINAL_NORTH;
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_EAST") == 0) {
            m_object->m_smarts = CARDINAL_EAST;
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_SOUTH") == 0) {
            m_object->m_smarts = CARDINAL_SOUTH;
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_WEST") == 0) {
            m_object->m_smarts = CARDINAL_WEST;
        }
    }

    m_stepMag = 0.0;
    if (LoadSprites() == 0) {
        m_wwdObject->m_flags |= 0x10000;
    }
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_object->m_area.left = 0;
    m_object->m_area.right = 0;
    m_object->m_area.top = 0;
    m_object->m_area.bottom = 0;
}

static inline CString* TypeLookup(i32 key) {
    g_typeColl.m_grown = 0;
    if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
        return g_typeColl.Elem(key);
    }
    if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(key, 0) != NULL) {
        return g_typeColl.Elem(key);
    }
    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
    return g_typeColl.Scratch();
}

RVA(0x000b2940, 0x102)
void CKitchenSlime::FireActivation(i32 coord) {
    CActHandler* e = KSlimeLookup(coord);
    if ((*e) != 0) {
        CActHandler* e2 = KSlimeLookup(coord);
        (this->*((*e2)))();
    }
}

// @early-stop
RVA(0x000b2aa0, 0x18d)
void CKitchenSlime::RegisterType() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = TypeLookup(g_typeCounter);
        i32 cnt = g_typeColl.m_grown;
        CString* nodes = g_typeColl.Slots();
        while (cnt-- != 0) {
            if (nodes != NULL) {
                nodes->~CString();
            }
            nodes++;
        }
        (*slot) = "A";
        g_typeCounter++;
    }

    *KSlimeLookup(id) = static_cast<CActHandler>(&CKitchenSlime::Tick);
}

RVA(0x000b2ca0, 0x29c)
i32 CKitchenSlime::Tick() {
    m_wwdObject->m_animCursor.Advance(static_cast<i32>(g_engineFrameDelta));

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode == 0 || reg->m_gameMode != GAMEMODE_SINGLE) {
        CGameObject* lvl = Level();
        i32 outX, outY;
        CGrunt* ent = static_cast<CGrunt*>(reg->m_cmdGrid->FindGruntAt(
            lvl->m_screenX,
            lvl->m_screenY,
            &lvl->m_area,
            &outY,
            &outX,
            static_cast<RECT*>(0)
        ));
        if (ent && ent->m_gruntKind != GRUNT_INVULNERABLE) {
            (static_cast<CTriggerMgr*>(g_gameReg->m_cmdGrid))
                ->CellDispatch(outY, outX, DEATH_MELT, -1);
        }
    }

    CGameObject* lvl = Level();
    if (lvl->m_screenX == m_tilePosition.m_x && lvl->m_screenY == m_tilePosition.m_y
        && LoadSprites() == 0) {
        m_wwdObject->m_flags |= 0x10000;
        return 0;
    }

    double step = static_cast<double>(static_cast<i64>(static_cast<u64>(g_frameDelta))) * m_speed;
    double* m88d = &m_stepMag;

    i32 newX;
    if (m_dirX > 0.0) {
        double t = (m_posX = m_posX + step);
        newX = static_cast<i32>(floor(t));
        i32 tx = m_tilePosition.m_x;
        *m88d = fabs(m_posX - static_cast<double>(tx));

        if (newX > tx) {
            newX = newX;
        }
    } else if (m_dirX < 0.0) {
        double t = (m_posX = m_posX - step);
        newX = static_cast<i32>(ceil(t));
        i32 tx = m_tilePosition.m_x;
        *m88d = fabs(m_posX - static_cast<double>(tx));
        if (newX < tx) {
            newX = newX;
        }
    } else {
        newX = static_cast<i32>(floor(m_posX));
    }

    i32 newY;
    if (m_dirY > 0.0) {
        double t = (m_posY = m_posY + step);
        newY = static_cast<i32>(floor(t));
        i32 ty = m_tilePosition.m_y;
        *m88d = fabs(m_posY - static_cast<double>(ty));
        if (newY > ty) {
            Level()->m_screenX = newX;
            Level()->m_screenY = ty;
            return 0;
        }
    } else if (m_dirY < 0.0) {
        double t = (m_posY = m_posY - step);
        newY = static_cast<i32>(ceil(t));
        i32 ty = m_tilePosition.m_y;
        *m88d = fabs(m_posY - static_cast<double>(ty));
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

RVA(0x000b2ff0, 0x11b)
i32 CKitchenSlime::SerializeMove(
    CFileMemBase* stream,
    SerialMode tag,
    LogicTypeId c,
    CGameObject* d
) {
    CFileMemBase* s = stream;

    if (tag != SERIAL_SAVE) {
        if (tag == SERIAL_LOAD) {
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
    if (CUserLogic::SerializeMove(stream, tag, c, d) == 0) {
        return 0;
    }
    return Chain(stream, tag, c, d) != 0;
}

// @early-stop
RVA(0x000b3160, 0x35c)
i32 CKitchenSlime::LoadSprites() {
    i32 savedDir = Level()->m_smarts;

    i32 tileX, tileY;
    i32 found = 0;
    for (i32 i = 0; i <= 4;) {
        CGameObject* lvl = Level();
        i32 sw = lvl->m_smarts;
        switch (sw) {
            case CARDINAL_NORTH:
                tileX = m_tilePosition.m_x;
                tileY = m_tilePosition.m_y - 0x20;
                break;
            case CARDINAL_EAST:
                tileX = m_tilePosition.m_x + 0x20;
                tileY = m_tilePosition.m_y;
                break;
            case CARDINAL_SOUTH:
                tileX = m_tilePosition.m_x;
                tileY = m_tilePosition.m_y + 0x20;
                break;
            case CARDINAL_WEST:
                tileX = m_tilePosition.m_x - 0x20;
                tileY = m_tilePosition.m_y;
                break;
        }

        i32 gx = tileX >> TILE_SHIFT_PX;
        i32 gy = tileY >> TILE_SHIFT_PX;
        i32 tileFlags;
        CMapMgr* map = g_gameReg->m_tileGrid;
        if (static_cast<u32>(gx) >= static_cast<u32>(map->m_width)
            || static_cast<u32>(gy) >= static_cast<u32>(map->m_height)) {
            tileFlags = 1;
        } else {
            tileFlags = ((map->m_rowInts[gy]))[gx * 7];
        }

        if (tileY >= lvl->m_extent.top && tileX <= lvl->m_extent.right
            && tileY <= lvl->m_extent.bottom && tileX >= lvl->m_extent.left && !(tileFlags & 0x939)
            && !(tileFlags & 2)) {
            found = 1;
            break;
        }

        if (++i > 4) {
            return 0;
        }

        if (lvl->m_direction == 1) {
            lvl->m_smarts = sw;
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
    if (!found) {
        return 0;
    }

    m_posX = 0;
    m_posY = 0;
    i32 changed = (Level()->m_smarts != savedDir);
    switch (Level()->m_smarts) {
        case CARDINAL_NORTH:
            m_posY = -m_stepMag;
            m_dirX = 0.0;
            m_dirY = -1.0;
            if (changed) {
                Anim()->ApplyName("LEVEL_KITCHENSLIME_NORTH");
            }
            break;
        case CARDINAL_EAST:
            m_posX = m_stepMag;
            m_dirX = 1.0;
            m_dirY = 0.0;
            if (changed) {
                Anim()->ApplyName("LEVEL_KITCHENSLIME_EAST");
            }
            break;
        case CARDINAL_SOUTH:
            m_posY = m_stepMag;
            m_dirY = 1.0;
            m_dirX = 0.0;
            if (changed) {
                Anim()->ApplyName("LEVEL_KITCHENSLIME_SOUTH");
            }
            break;
        case CARDINAL_WEST:
            m_posX = -m_stepMag;
            m_dirX = -1.0;
            m_dirY = 0.0;
            if (changed) {
                Anim()->ApplyName("LEVEL_KITCHENSLIME_WEST");
            }
            break;
    }

    m_posX = static_cast<double>(Level()->m_screenX) + m_posX;
    m_posY = static_cast<double>(Level()->m_screenY) + m_posY;

    u32 time;
    if (Level()->m_animWorker->m_speed != 0) {
        time = Level()->m_animWorker->m_speed;
    } else {
        time = g_buteMgr.GetDwordDef("Hazardz", "KitchenSlimeTimePerTile", 1000);
    }

    m_speed = g_slimeSpeedNum / static_cast<double>(static_cast<i64>(static_cast<u64>(time)));
    m_tilePosition.m_x = tileX;
    m_tilePosition.m_y = tileY;

    CWwdGameObjectA* player = Anim();
    CDDrawWorker* spr = player->m_frameSet;
    if (changed != 0 && spr != NULL) {
        if (spr->m_minIndex <= 1 && spr->m_maxIndex >= 1) {
            player->m_frameIndex = 1;
            player->m_layer = static_cast<CImage*>(spr->m_items.GetAt(1));
            m_stepMag = 0.0;
            return 1;
        }
        player->m_frameIndex = 1;
        player->m_layer = NULL;
        m_stepMag = 0.0;
        return 1;
    }
    m_stepMag = 0.0;
    return 1;
}
