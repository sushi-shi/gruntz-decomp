

#include <Bute/ButeTree.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>
#include <Gruntz/Grunt.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/GameStateRecord.h>
#include <Gruntz/BoundaryLowerMethodsViews.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <rva.h>
#include <Pix16.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/TriggerMgr.h>

DATA(0x002448d8)
GruntDirectionCell g_gruntMoveDirNorth = GruntDirectionCell(0, 1, 1);
DATA(0x00244908)
GruntDirectionCell g_gruntMoveDirNorthEast = GruntDirectionCell(0, 2, 2);
DATA(0x002448c8)
GruntDirectionCell g_gruntMoveDirEast = GruntDirectionCell(1, 2, 3);
DATA(0x00244928)
GruntDirectionCell g_gruntMoveDirSouthEast = GruntDirectionCell(2, 2, 4);
DATA(0x00244938)
GruntDirectionCell g_gruntMoveDirCenter = GruntDirectionCell(1, 1, 0);
DATA(0x002448e8)
GruntDirectionCell g_gruntMoveDirSouth = GruntDirectionCell(2, 1, 5);
DATA(0x00244948)
GruntDirectionCell g_gruntMoveDirSouthWest = GruntDirectionCell(2, 0, 6);
DATA(0x002448f8)
GruntDirectionCell g_gruntMoveDirWest = GruntDirectionCell(1, 0, 7);
DATA(0x00244918)
GruntDirectionCell g_gruntMoveDirNorthWest = GruntDirectionCell(0, 0, 8);

static char s_TimePerTile[] = "TimePerTile";
static char s_Grunt[] = "Grunt";
static char s_EntranceSafeTime[] = "EntranceSafeTime";
static char s_IdleDelay[] = "IdleDelay";
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius";
static char s_CombatTimeout[] = "CombatTimeout";

DATA(0x0020dbf8)
static char s_ToyTiles[] = "ToyTiles";
DATA(0x0020da6c)
static const char s_BABYWALKERGRUNT[] = "BABYWALKERGRUNT";
DATA(0x0020da48)
static const char s_BIGWHEELGRUNT[] = "BIGWHEELGRUNT";
DATA(0x0020da38)
static const char s_GOKARTGRUNT[] = "GOKARTGRUNT";
DATA(0x0020d9fc)
static const char s_POGOSTICKGRUNT[] = "POGOSTICKGRUNT";

static inline i32 TileFlags(const char* rec) {

    Pix16CPtr r;
    r.m_chars = rec;
    return *r.m_dwords;
}

static __inline i32 s_TileFlags(CGruntzMapMgr* b, i32 tx, i32 ty) {
    if (static_cast<u32>(tx) >= static_cast<u32>(b->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(b->m_height)) {
        return 1;
    }
    return b->m_rowInts[ty][tx * 7];
}

static __inline i32 s_CanCommitMove(CGrunt* g, i32 moveX, i32 moveY) {
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    i32 tx = g->m_lastTilePxX >> 5;
    i32 ty = g->m_lastTilePxY >> 5;
    i32 mtx = moveX >> 5;
    i32 mty = moveY >> 5;
    i32 arr = g->m_arrivalFlags | 0x20000000;
    if (tx == mtx && ty == mty) {
        return 1;
    }
    if (static_cast<u32>(mtx) >= static_cast<u32>(board->m_width)
        || static_cast<u32>(mty) >= static_cast<u32>(board->m_height)) {
        return 0;
    }
    i32* tgt = &board->m_rowInts[mty][mtx * 7];
    i32 tflags = *tgt;
    i32 hit = arr & tflags;
    if (hit & 0x20000000) {
        return 0;
    }
    if (hit != 0) {
        i32 mask = g->m_24c | 0x18000482;
        if ((tflags & mask) == 0) {
            return 0;
        }
    }
    i32 dx = mtx - tx;
    i32 dy = mty - ty;
    if (dx == 0 || dy == 0) {
        return 1;
    }
    char* cur = board->m_rowBytes[ty] + tx * 7 * 4;

    Pix16Ptr row;
    row.m_dwords = tgt;
    char* tg = row.m_chars;
    i32 stride = board->m_width * 7 * 4;
    if (dx > 0) {
        if (dy > 0) {
            if ((cur[0x1d] & 0x20) || (cur[stride + 1] & 0x20) || (TileFlags(tg - 0x1c) & 0x2000)
                || (TileFlags(tg - stride) & 0x2000)) {
                return 0;
            }
        } else {
            if ((cur[0x1d] & 0x20) || (TileFlags(cur - stride) & 0x2000)
                || (TileFlags(tg - 0x1c) & 0x2000) || (TileFlags(tg + stride) & 0x2000)) {
                return 0;
            }
        }
    } else {
        if (dy > 0) {
            if ((cur[-0x1b] & 0x20) || (cur[stride + 1] & 0x20) || (TileFlags(tg + 0x1c) & 0x2000)
                || (TileFlags(tg - stride) & 0x2000)) {
                return 0;
            }
        } else {
            if ((cur[-0x1b] & 0x20) || (TileFlags(cur - stride) & 0x2000)
                || (TileFlags(tg + 0x1c) & 0x2000) || (TileFlags(tg + stride) & 0x2000)) {
                return 0;
            }
        }
    }
    return 1;
}

static __inline void SerRecord(CFileMemBase* ar, i32 mode, void* p) {
    switch (mode) {
        case 4:
            ar->Write(p, 8);
            ar->Write(static_cast<char*>(p) + 8, 8);
            break;
        case 7:
            ar->Read(p, 8);
            ar->Read(static_cast<char*>(p) + 8, 8);
            break;
    }
}

static __inline i32 GruntTileFlags(i32 tx, i32 ty) {
    CGruntzMapMgr* b = g_gameReg->m_tileGrid;
    if (static_cast<u32>(tx) >= static_cast<u32>(b->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(b->m_height)) {
        return 1;
    }
    return b->m_rowInts[ty][tx * 7];
}

RVA(0x00050ca0, 0x2b)
i32 CGrunt::LoadTypeTableClearMove(i32 typeId) {

    i32 r = LoadGruntTypeTable(typeId, 0, 0, 0);
    m_moveMode = -1;
    m_1a4 = 0;
    return r;
}

// @early-stop
RVA(0x00050ce0, 0x3c4)
i32 CGrunt::LoadVehicleGruntSprites(i32 kind) {
    m_198 = kind;
    m_moveMode = -1;

    CString name;

#define REGION_INIT()                                                                              \
    do {                                                                                           \
        RECT a;                                                                                    \
        a.left = -1;                                                                               \
        a.top = -1;                                                                                \
        a.right = 1;                                                                               \
        a.bottom = 1;                                                                              \
        m_toyRectA = a;                                                                            \
        a.left = 0;                                                                                \
        a.top = 0;                                                                                 \
        a.right = 0;                                                                               \
        a.bottom = 0;                                                                              \
        m_toyRectB = a;                                                                            \
    } while (0)
    switch (kind) {
        case PICKUP_BABYWALKER:
            REGION_INIT();
            name = "BABYWALKERGRUNT";
            break;
        case PICKUP_BEACHBALL:
            REGION_INIT();
            name = "BEACHBALLGRUNT";
            break;
        case PICKUP_BIGWHEEL:
            REGION_INIT();
            name = "BIGWHEELGRUNT";
            break;
        case PICKUP_GOKART:
            REGION_INIT();
            name = "GOKARTGRUNT";
            break;
        case PICKUP_JACKINTHEBOX:
            REGION_INIT();
            name = "JACKINTHEBOXGRUNT";
            break;
        case PICKUP_JUMPROPE:
            REGION_INIT();
            name = "JUMPROPEGRUNT";
            break;
        case PICKUP_POGOSTICK:
            REGION_INIT();
            name = "POGOSTICKGRUNT";
            break;
        case PICKUP_SCROLL:
            REGION_INIT();
            name = "SCROLLGRUNT";
            break;
        case PICKUP_SQUEAKTOY:
            REGION_INIT();
            name = "SQUEAKTOYGRUNT";
            break;
        case PICKUP_YOYO:
            REGION_INIT();
            name = "YOYOGRUNT";
            break;
        default:
            break;
    }
#undef REGION_INIT

    g_gameReg->m_curState->BuildAssetNamespacePrefixes(name, 1, 1, 0);

    i32 code = g_gameReg->m_tileGrid->m_rowInts[m_lastTilePxY >> 5][(m_lastTilePxX >> 5) * 7 + 4];
    if (code == 0x41 || code == 0x42) {
        if (m_object->m_screenX == m_lastTilePxX && m_object->m_screenY == m_lastTilePxY) {

            m_tileMgr->ApplySwitch(this, m_lastTilePxX, m_lastTilePxY);
            m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
        }
    }
    return 1;
}
RVA(0x000511b0, 0x246)
void CGrunt::PlayMoveSound(i32 x, i32 y) {
    CWwdGameObjectA* h = m_object;
    i32 dy = y - h->m_screenY;
    i32 dx = x - h->m_screenX;
    i32 cx = h->m_screenX;

    if (dx == 0) {
        if (y > h->m_screenY) {
            PlaySound(1000, g_gruntMoveDirSouth);
        } else if (y < h->m_screenY) {
            PlaySound(1000, g_gruntMoveDirNorth);
        }
        return;
    }

    float ratio = static_cast<float>(dy) / dx;
    if (ratio > 2.0f || ratio < -2.0f) {
        if (y > h->m_screenY) {
            PlaySound(1000, g_gruntMoveDirSouth);
        } else {
            PlaySound(1000, g_gruntMoveDirNorth);
        }
        return;
    }
    if (ratio <= 0.5 && ratio >= -0.5) {
        if (x > cx) {
            PlaySound(1000, g_gruntMoveDirEast);
        } else {
            PlaySound(1000, g_gruntMoveDirWest);
        }
        return;
    }
    if (ratio > 0.5) {
        if (x > cx) {
            PlaySound(1000, g_gruntMoveDirSouthEast);
        } else {
            PlaySound(1000, g_gruntMoveDirNorthWest);
        }
        return;
    }
    if (ratio < -0.5) {
        if (x > cx) {
            PlaySound(1000, g_gruntMoveDirNorthEast);
        } else {
            PlaySound(1000, g_gruntMoveDirSouthWest);
        }
    }
}

RVA(0x000514a0, 0x26)
i32 CGrunt::CanShowStamina() {
    if (m_combatActive == 0 && m_stamina >= 0x64 && m_entranceActive == 0) {
        return 1;
    }
    return 0;
}

RVA(0x000514e0, 0x1e)
void CGrunt::PlayMoveSoundAtTile(i32 tx, i32 ty) {
    PlayMoveSound(tx * 0x20 + 0x10, ty * 0x20 + 0x10);
}

// @early-stop
RVA(0x00051510, 0x20f)
i32 CGrunt::IsDropReady(i32 a) {
    {
        CGruntzMapMgr* board = g_gameReg->m_tileGrid;
        i32 x = m_commitPxX >> 5;
        i32 y = m_commitPxY >> 5;
        i32 owner;
        if (static_cast<u32>(x) < static_cast<u32>(board->m_width)
            && static_cast<u32>(y) < static_cast<u32>(board->m_height)) {
            owner = board->m_rowInts[y][x * 7 + 1];
        } else {
            owner = -1;
        }
        if (owner != -1) {
            return 0;
        }
    }

    CWwdGameObjectA* object = m_object;
    i32 lastX = m_lastTilePxX;
    if (object->m_screenX == lastX) {
        i32 lastY = m_lastTilePxY;
        if (object->m_screenY == lastY) {
            return 0;
        }
    }

    if (m_31c.GetCount() != 0) {
        Coord* coord = 0;
        CoordPoolNode* node = g_coordPool.m_freeHead;
        i32 coordX = m_lastTilePxX >> 5;
        i32 coordY = m_lastTilePxY >> 5;
        if (node->m_next != 0) {
            coord = &node->m_coord;
            coord->m_x = coordX;
            coord->m_y = coordY;
            g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
        }
        m_31c.AddHead(coord);
    }

    m_object->m_screenX = m_commitPxX;
    m_object->m_screenY = m_commitPxY;
    object = m_object;
    if (object->m_sortKey != object->m_screenY + 0x186a0) {
        object->m_sortKey = object->m_screenY + 0x186a0;
        i32 flags = object->m_flags;
        object->m_flags = flags | 0x20000;
    }

    i32 oldY = m_lastTilePxY >> 5;
    i32 oldX = m_lastTilePxX >> 5;
    i32 newX = m_commitPxX >> 5;
    i32 newY = m_commitPxY >> 5;
    {
        CGruntzMapMgr* board = g_gameReg->m_tileGrid;
        board->m_rows[oldY][oldX].m_flagBytes[3] &= 0xdf;
        board->m_rows[oldY][oldX].m_4 = -1;
    }
    {
        CGruntzMapMgr* board = g_gameReg->m_tileGrid;
        i32 ownerLo = m_tileOwnerLo;
        i32 ownerHi = m_tileOwnerHi;
        board->m_rows[newY][newX].m_flagBytes[3] |= 0x20;
        board->m_rows[newY][newX].m_4 = (ownerHi << 8) | ownerLo;
    }

    m_lastTilePxX = m_commitPxX;
    m_lastTilePxY = m_commitPxY;
    m_commitPxX = m_entrancePxX;
    m_commitPxY = m_entrancePxY;
    m_35c = 1;

    SetEntrancePos(a, 1);
    if (m_arrivalPending != 0) {
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
        m_arrivalPending = 0;
    }
    return 1;
}

RVA(0x000517b0, 0x7d)
void CGrunt::SnapToLastTile(i32 a) {
    m_object->m_screenX = m_lastTilePxX;
    m_object->m_screenY = m_lastTilePxY;
    CWwdGameObjectA* h = m_object;
    if (h->m_sortKey != h->m_screenY + 0x186a0) {
        h->m_sortKey = h->m_screenY + 0x186a0;
        h->m_flags |= 0x20000;
    }
    SetEntrancePos(a, 1);
    if (m_arrivalPending != 0) {

        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
        m_arrivalPending = 0;
    }
}

// @early-stop
RVA(0x00051850, 0x165)
i32 CGrunt::RectContains(i32 x, i32 y) {
    i32 dx = m_lastTilePxX >> 5;
    i32 dy = m_lastTilePxY >> 5;
    i32 px = x >> 5;
    i32 py = y >> 5;

    i32* ra = ((&m_reachRectLeft));
    i32* rb = ((&m_2a0));

    RECT r1;
    r1.left = ra[0] + dx;
    r1.top = ra[1] + dy;
    r1.right = ra[2] + dx + 1;
    r1.bottom = ra[3] + dy + 1;

    RECT r2;
    r2.left = rb[0] + dx;
    r2.top = rb[1] + dy;
    r2.right = rb[2] + dx;
    r2.bottom = rb[3] + dy;

    if (IsRectEmpty(&r1) || IsRectEmpty(&r2)) {
        if (IsRectEmpty(&r2)) {

            if (px < r1.right && px >= r1.left && py < r1.bottom && py >= r1.top) {
                return 1;
            }
            return 0;
        }
        return 0;
    }

    if (px < r1.right && px >= r1.left && py < r1.bottom && py >= r1.top) {

        if (px >= r2.right || px < r2.left || py >= r2.bottom || py < r2.top) {
            return 1;
        }
    }
    return 0;
}

// @early-stop
RVA(0x00051a20, 0x17d)
i32 CGrunt::RectContainsGated(i32 x, i32 y) {
    i32 px = x >> 5;
    i32 py = y >> 5;
    i32 dx = m_lastTilePxX >> 5;
    i32 dy = m_lastTilePxY >> 5;

    RECT r1;
    r1.left = m_toyRectA.left + dx;
    r1.top = m_toyRectA.top + dy;
    r1.right = m_toyRectA.right + dx + 1;
    r1.bottom = m_toyRectA.bottom + dy + 1;

    RECT r2;
    r2.left = m_toyRectB.left + dx;
    r2.top = m_toyRectB.top + dy;
    r2.right = m_toyRectB.right + dx;
    r2.bottom = m_toyRectB.bottom + dy;

    if (m_198 == 0) {
        return 0;
    }

    if (IsRectEmpty(&r1) || IsRectEmpty(&r2)) {
        if (IsRectEmpty(&r2)) {
            if (px < r1.right && px >= r1.left && py < r1.bottom && py >= r1.top) {
                return 1;
            }
            return 0;
        }
        return 0;
    }
    if (px < r1.right && px >= r1.left && py < r1.bottom && py >= r1.top) {

        if (px >= r2.right || px < r2.left || py >= r2.bottom || py < r2.top) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00051c00, 0xd20)
i32 CGrunt::StepCompassMove() {
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    i32 x = m_lastTilePxX;
    i32 y = m_lastTilePxY;
    i32 tx = x >> 5;
    i32 ty = y >> 5;
    i32 result = 0;
    i32 moveX = x;
    i32 moveY = y;
    GruntDirectionCell voice;

    if (s_TileFlags(board, tx, ty) & 0x80) {

        i32 cmd = board->m_rowInts[ty][tx * 7 + 4];
        switch (cmd - 0xb) {
            case 8:
                switch (m_entranceCell.direction - 1) {
                    case 0:
                        moveY = y - 0x20;
                        voice = g_gruntMoveDirNorth;
                        break;
                    case 1:
                        moveX = x + 0x20;
                        moveY = y - 0x20;
                        voice = g_gruntMoveDirNorthEast;
                        break;
                    case 2:
                        moveX = x + 0x20;
                        voice = g_gruntMoveDirEast;
                        break;
                    case 3:
                        moveY = y + 0x20;
                        moveX = x + 0x20;
                        voice = g_gruntMoveDirSouthEast;
                        break;
                    case 4:
                        moveY = y + 0x20;
                        voice = g_gruntMoveDirSouth;
                        break;
                    case 5:
                        moveY = y + 0x20;
                        moveX = x - 0x20;
                        voice = g_gruntMoveDirSouthWest;
                        break;
                    case 6:
                        moveX = x - 0x20;
                        voice = g_gruntMoveDirWest;
                        break;
                    case 7:
                        moveX = x - 0x20;
                        moveY = y - 0x20;
                        voice = g_gruntMoveDirNorthWest;
                        break;
                }
                break;
            case 0:
            case 4:
                moveY = y - 0x20;
                voice = g_gruntMoveDirNorth;
                break;
            case 3:
            case 7:
                moveX = x + 0x20;
                voice = g_gruntMoveDirEast;
                break;
            case 1:
            case 5:
                moveY = y + 0x20;
                voice = g_gruntMoveDirSouth;
                break;
            case 2:
            case 6:
                moveX = x - 0x20;
                voice = g_gruntMoveDirWest;
                break;
        }
        i32 mtx = moveX >> 5;
        i32 mty = moveY >> 5;
        i32 tflags = s_TileFlags(board, mtx, mty);
        if ((tflags & 0x20000000) && !(tflags & 0x80)) {

            i32 owner;
            if (static_cast<u32>(mtx) >= static_cast<u32>(board->m_width)
                || static_cast<u32>(mty) >= static_cast<u32>(board->m_height)) {
                owner = -1;
            } else {
                owner = board->m_rowInts[mty][mtx * 7 + 1];
            }
            m_tileMgr->CellDispatch((owner >> 8) & 0xff, owner & 0xff, 2, m_tileOwnerHi);
        }
        goto commit;
    }

    if (m_toyTileIndex != 0) {
        CString str;
        switch (m_entranceReason - 0x17) {
            case 0:
                str = s_BABYWALKERGRUNT;
                break;
            case 2:
                str = s_BIGWHEELGRUNT;
                break;
            case 3:
                str = s_GOKARTGRUNT;
                break;
            case 6:
                str = s_POGOSTICKGRUNT;
                break;
            default:
                break;
        }
        i32 toyCount =
            g_buteMgr.GetIntDef(const_cast<char*>(static_cast<LPCTSTR>(str)), s_ToyTiles, 1);
        if (m_toyTileIndex < toyCount) {
            switch (m_entranceCell.direction - 1) {
                case 0:
                    moveY = y - 0x20;
                    voice = g_gruntMoveDirNorth;
                    break;
                case 1:
                    moveY = y - 0x20;
                    moveX = x + 0x20;
                    voice = g_gruntMoveDirNorthEast;
                    break;
                case 2:
                    moveX = x + 0x20;
                    voice = g_gruntMoveDirEast;
                    break;
                case 3:
                    moveY = y + 0x20;
                    moveX = x + 0x20;
                    voice = g_gruntMoveDirSouthEast;
                    break;
                case 4:
                    moveY = y + 0x20;
                    voice = g_gruntMoveDirSouth;
                    break;
                case 5:
                    moveY = y + 0x20;
                    moveX = x - 0x20;
                    voice = g_gruntMoveDirSouthWest;
                    break;
                case 6:
                    moveX = x - 0x20;
                    voice = g_gruntMoveDirWest;
                    break;
                case 7:
                    moveX = x - 0x20;
                    moveY = y - 0x20;
                    voice = g_gruntMoveDirNorthWest;
                    break;
            }
            result = s_CanCommitMove(this, moveX, moveY);
            if (result == 0) {
                m_toyTileIndex = 0;
            }
        } else {
            m_toyTileIndex = 0;
        }
    }
    if (result != 0) {
        goto commit;
    }

    {
        ::CByteArray bag;
        bag.SetAtGrow(bag.GetSize(), 1);
        bag.SetAtGrow(bag.GetSize(), 2);
        bag.SetAtGrow(bag.GetSize(), 3);
        bag.SetAtGrow(bag.GetSize(), 4);
        bag.SetAtGrow(bag.GetSize(), 5);
        bag.SetAtGrow(bag.GetSize(), 6);
        bag.SetAtGrow(bag.GetSize(), 7);
        bag.SetAtGrow(bag.GetSize(), 8);
        while (bag.GetSize() > 0) {
            i32 idx = GruntRand() % bag.GetSize();
            i32 dir = bag.GetAt(idx);
            moveX = x;
            moveY = y;
            switch (dir - 1) {
                case 0:
                    moveY = y - 0x20;
                    voice = g_gruntMoveDirNorth;
                    break;
                case 1:
                    moveX = x + 0x20;
                    moveY = y - 0x20;
                    voice = g_gruntMoveDirNorthEast;
                    break;
                case 2:
                    moveX = x + 0x20;
                    voice = g_gruntMoveDirEast;
                    break;
                case 3:
                    moveX = x + 0x20;
                    moveY = y + 0x20;
                    voice = g_gruntMoveDirSouthEast;
                    break;
                case 4:
                    moveY = y + 0x20;
                    voice = g_gruntMoveDirSouth;
                    break;
                case 5:
                    moveX = x - 0x20;
                    moveY = y + 0x20;
                    voice = g_gruntMoveDirSouthWest;
                    break;
                case 6:
                    moveX = x - 0x20;
                    voice = g_gruntMoveDirWest;
                    break;
                case 7:
                    moveY = y - 0x20;
                    moveX = x - 0x20;
                    voice = g_gruntMoveDirNorthWest;
                    break;
            }
            result = s_CanCommitMove(this, moveX, moveY);
            bag.RemoveAt(idx, 1);
            if (result != 0) {
                break;
            }
        }
    }
    if (result == 0) {
        return 0;
    }

commit:
    m_tileMgr->ApplySwitch(this, m_lastTilePxX, m_lastTilePxY);
    PlaySound(0x3e8, voice);
    m_commitPxX = m_lastTilePxX;
    m_commitPxY = m_lastTilePxY;
    {
        CGruntzMapMgr* b = g_gameReg->m_tileGrid;
        i32 ox = m_lastTilePxX >> 5;
        i32 oy = m_lastTilePxY >> 5;
        b->m_rowBytes[oy][ox * 7 * 4 + 3] &= 0xdf;
        b->m_rowInts[oy][ox * 7 + 1] = -1;
    }
    {
        CGruntzMapMgr* b = g_gameReg->m_tileGrid;
        i32 nx = moveX >> 5;
        i32 ny = moveY >> 5;
        i32 owner = (m_tileOwnerHi << 8) | m_tileOwnerLo;
        b->m_rowBytes[ny][nx * 7 * 4 + 3] |= 0x20;
        b->m_rowInts[ny][nx * 7 + 1] = owner;
    }
    m_lastTilePxX = moveX;
    m_lastTilePxY = moveY;
    ComputeFacing(1.0);
    m_arrivalPending = 1;
    m_toyTileIndex += 1;
    return 1;
}

// @early-stop
RVA(0x00052c70, 0x1e0)
i32 CGrunt::ClaimSwitchTile() {
    i32 x = m_lastTilePxX;
    i32 y = m_lastTilePxY;
    switch (m_entranceCell.direction - 1) {
        case 0:
            y -= 0x20;
            break;
        case 1:
            x += 0x20;
            y -= 0x20;
            break;
        case 2:
            x += 0x20;
            break;
        case 3:
            x += 0x20;
            y += 0x20;
            break;
        case 4:
            y += 0x20;
            break;
        case 5:
            x -= 0x20;
            y += 0x20;
            break;
        case 6:
            x -= 0x20;
            break;
        case 7:
            x -= 0x20;
            y -= 0x20;
            break;
        default:
            break;
    }

    CGruntzMapMgr* b = g_gameReg->m_tileGrid;
    i32 tx = x >> 5;
    i32 ty = y >> 5;
    i32 flags;
    if (static_cast<u32>(tx) >= static_cast<u32>(b->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(b->m_height)) {
        flags = 1;
    } else {
        flags = b->m_rowInts[ty][tx * 7];
    }
    if ((flags & 0x20000939) || (flags & 0x80)) {
        return 0;
    }

    m_tileMgr->ApplySwitch(this, m_lastTilePxX, m_lastTilePxY);

    m_commitPxX = m_lastTilePxX;
    m_commitPxY = m_lastTilePxY;
    CGruntzMapMgr* gb = g_gameReg->m_tileGrid;
    i32 oldTx = m_lastTilePxX >> 5;
    i32 oldTy = m_lastTilePxY >> 5;
    gb->m_rowInts[oldTy][oldTx * 7 * 4 + 3] &= 0xdf;
    *&gb->m_rowInts[oldTy][oldTx * 7 * 4 + 4] = -1;

    i32 owner = (m_tileOwnerHi << 8) | m_tileOwnerLo;
    gb->m_rowInts[ty][tx * 7 * 4 + 3] |= 0x20;
    *&gb->m_rowInts[ty][tx * 7 * 4 + 4] = owner;

    m_lastTilePxX = x;
    m_lastTilePxY = y;
    ComputeFacing(1.0);
    m_arrivalPending = 1;
    return 1;
}

// @early-stop
RVA(0x00052ed0, 0x42)
void CGrunt::SetArrivalTarget(i32 a, i32 b, i32 c, i32 d) {
    m_arrivalCol = a;
    m_arrivalRow = b;
    m_arrivalActive = 1;
    m_defenderX = (c & ~0x1f) + 0x10;
    m_defenderY = (d & ~0x1f) + 0x10;
}

// @early-stop
RVA(0x00052f40, 0x4b)
void CGrunt::ConsiderArrival(i32 a) {
    CWwdGameObjectA* h = m_object;
    i32 px = (h->m_screenX & ~0x1f) + 0x10;
    i32 py = (h->m_screenY & ~0x1f) + 0x10;
    if (px != m_lastTilePxX || py != m_lastTilePxY) {
        if (IsDropReady(a)) {
            return;
        }
    }
    SnapToLastTile(a);
}

// @early-stop
RVA(0x00052fb0, 0x96e)
i32 CGrunt::TryTeleportToCell(i32 tileX, i32 tileY, i32 useSecretColor, i32 spawnWormhole) {
    if (m_entranceCommitted == 0) {
        return 1;
    }
    i32 flags = GruntTileFlags(tileX, tileY);
    if ((flags & 0xd39) || (flags & 0x82)) {
        return 0;
    }

    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "A") == 0);
    if (eq) {
        goto applyTail;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeD) == 0);
    if (eq) {
        goto applyTail;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "I") == 0);
    if (eq) {

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
        if (m_entranceReason != 1) {
            goto applyTail;
        }
        m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 1, -1);
        goto applyTail;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "G") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "L") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "P") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeO) == 0);
    if (eq) {

        SnapToLastTile(1);
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxY, m_lastTilePxX);
        goto applyTail;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeQ) == 0);
    if (eq) {
        return 1;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "J") == 0);
    if (eq) {

        m_entranceActive = 0;
        if (m_poweredUp == 0 && m_neighborValid == 0) {
            m_entranceCommitted = 0;
            ResetEntranceAnimation(1, 0, 0);
        }
        m_35c = 0;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId(s_codeD);
        m_value = m_wwdObject->m_1a0.m_14;
        m_wwdObject->m_1a0.Setup(m_poseWalk);

        GruntDirectionCell cell = m_entranceCell;
        i32 col = cell.column + cell.row * 2;
        i32 base = cell.row + col;
        char* nm = m_cells[base].WalkName().GetBuffer(0);
        m_wwdObject->ApplyName(nm);
        goto modeDispatch;
    } else {
        SnapToLastTile(1);
        goto modeDispatch;
    }

idleReseed:

    if (m_entranceReason == 0x1e) {
        g_gameReg->m_cueSink->StopVoice(m_object->m_188);
    }
    LoadGruntTypeTable(m_toolId, 1, 0, 1);
    {
        i32 px = m_object->m_screenY + 0x186a0;
        if (m_object->m_sortKey != px) {
            m_object->m_sortKey = px;
            m_object->m_flags |= 0x20000;
        }
    }
    if (m_toyTimeSprite != 0) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    m_toyTime = 0;
    StopStruckSlotSound();

applyTail:

    if (m_wingzEnabled != 0) {
        LoadWingzGruntSprites(0);
    }
    if (m_poweredUp == 0 && m_neighborValid == 0) {
        m_entranceCommitted = 0;
        ResetEntranceAnimation(1, 0, 0);
    }

    return 1;

modeDispatch: {
    i32 mode = m_moveMode;
    if (mode >= 0x32) {
        LoadGruntTypeTable(mode, 1, 0, 1);
        m_moveMode = -1;
        m_1a4 = 0;
        return 1;
    }
    if (mode >= 0x22) {
        m_194 = mode;
        m_moveMode = -1;
        return 1;
    }
    if (mode >= 0x17) {
        LoadVehicleGruntSprites(mode);
        return 1;
    }
    LoadGruntTypeTable(mode, 1, 0, 1);
    m_moveMode = -1;
    return 1;
}
}

RVA(0x00053b80, 0x340)
i32 CGrunt::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (ar == 0) {
        return 0;
    }

    if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
        return 0;
    }

    if (CWapX::Chain(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    switch (mode) {
        case 4:

            if (Save(ar) == 0) {
                return 0;
            }
            break;
        case 7:

            if (LoadStateRecord(ar) == 0) {
                return 0;
            }
            break;
        case 8:
            m_tileMgr = g_gameReg->m_cmdGrid;
            break;
    }
    SerTriRecord(&m_entranceCell, ar, mode, typeId, pObj);
    SerRecord(ar, mode, &m_toyClock);
    SerRecord(ar, mode, &m_idleAnchor);
    SerRecord(ar, mode, &m_idleTimer);
    SerRecord(ar, mode, &m_entranceClockLo);
    SerRecord(ar, mode, &m_flashClockLo);
    SerRecord(ar, mode, &m_attackClockLo);
    SerRecord(ar, mode, &m_combatClockLo);
    SerRecord(ar, mode, &m_hudRetireClockLo);
    SerPairRecord(&m_wingzClockLo, ar, mode, typeId, pObj);
    SerPairRecord(&m_convertClockLo, ar, mode, typeId, pObj);
    SerPairRecord(&m_shimmerClockLo, ar, mode, typeId, pObj);
    SerPairRecord(&m_8c0, ar, mode, typeId, pObj);
    SerPairRecord(&m_arrivalRerollLo, ar, mode, typeId, pObj);
    SerPairRecord(&m_278, ar, mode, typeId, pObj);
    return 1;
}

RVA(0x00053f90, 0x11d0)
i32 CGrunt::Save(CFileMemBase* ar) {
    if (!ar) {
        return 0;
    }

    CDDrawSurfaceMgr* mgr = m_animWorker->m_ownerCtx;
    if (!mgr) {
        return 0;
    }
    i32 n;
    char buf[0x80];
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_selectedSprite;
        if (sp) {
            tmp = sp->m_188;
        }
        ar->Write(&tmp, 4);
    }
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_toySprite;
        if (sp) {
            tmp = sp->m_188;
        }
        ar->Write(&tmp, 4);
    }
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_healthSprite;
        if (sp) {
            tmp = sp->m_188;
        }
        ar->Write(&tmp, 4);
    }
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_staminaSprite;
        if (sp) {
            tmp = sp->m_188;
        }
        ar->Write(&tmp, 4);
    }
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_toyTimeSprite;
        if (sp) {
            tmp = sp->m_188;
        }
        ar->Write(&tmp, 4);
    }
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_wingzTimeSprite;
        if (sp) {
            tmp = sp->m_188;
        }
        ar->Write(&tmp, 4);
    }
    g_serialCounter++;
    {
        i32 tmp = 0;
        CWwdGameObjectA* sp = m_powerupSprite;
        if (sp) {
            tmp = sp->m_188;
        }
        ar->Write(&tmp, 4);
    }
    g_serialCounter++;
    memset(buf, 0, 0x80);
    strcpy(buf, static_cast<const char*>(m_animSetName));
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    strcpy(buf, m_448);
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    strcpy(buf, m_44c);
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseWalk;
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseAttack[GRUNT_ATTACK1];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseAttack[GRUNT_ATTACK2];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseAttackIdle;
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseStruck[GRUNT_STRUCK1];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseStruck[GRUNT_STRUCK2];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseIdle[GRUNT_IDLE1];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseIdle[GRUNT_IDLE2];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseIdle[GRUNT_IDLE3];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseIdle[GRUNT_IDLE4];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseIdle[GRUNT_IDLE5];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseDeath;
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseToy[GRUNT_TOY1];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseToy[GRUNT_TOY2];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseToy[GRUNT_TOY_BREAK];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseItem[GRUNT_ITEM1];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_poseItem[GRUNT_ITEM2];
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(static_cast<CObject*>(id)));
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        CAniElement* id = m_pickupGeoSrc;
        if (id) {
            strcpy(buf, mgr->m_animRegistry->KeyOfValue(id));
        }
    }
    ar->Write(buf, 0x80);
    ar->Write(&m_18c, 4);
    ar->Write(&m_toyBlendPct, 4);
    ar->Write(&m_194, 4);
    ar->Write(&m_entranceReason, 4);
    ar->Write(&m_198, 4);
    ar->Write(&m_toolId, 4);
    ar->Write(&m_moveMode, 4);
    ar->Write(&m_1a4, 4);
    ar->Write(&m_1a8, 4);
    ar->Write(&m_1ac, 4);
    ar->Write(&m_1b0, 4);
    ar->Write(&m_1b4, 4);
    ar->Write(&m_arrived, 4);
    ar->Write(&m_entrancePxX, 8);
    ar->Write(&m_lastTilePxX, 8);
    ar->Write(&m_commitPxX, 8);
    ar->Write(&m_1dc, 8);
    ar->Write(&m_entranceActive, 4);
    ar->Write(&m_arrivalPending, 4);
    ar->Write(&m_tileOwnerHi, 4);
    ar->Write(&m_tileOwnerLo, 4);
    ar->Write(&m_1f4_moveIcon, 4);
    ar->Write(&m_1f8, 4);
    ar->Write(&m_entranceCommitted, 4);
    ar->Write(&m_neighborCol, 8);
    ar->Write(&m_208, 8);
    ar->Write(&m_210, 4);
    ar->Write(&m_214, 4);
    ar->Write(&m_combatActive, 4);
    ar->Write(&m_neighborValid, 4);
    ar->Write(&m_poweredUp, 4);
    ar->Write(&m_224, 4);
    ar->Write(&m_entranceStamped, 4);
    ar->Write(&m_22c, 4);
    ar->Write(&m_arrivalActive, 4);
    ar->Write(&m_reachRectLeft, 16);
    ar->Write(&m_2a0, 16);
    ar->Write(&m_toyRectA, 16);
    ar->Write(&m_toyRectB, 16);
    ar->Write(&m_health, 4);
    ar->Write(&m_stamina, 4);
    ar->Write(&m_toyTime, 4);
    ar->Write(&m_wingzTime, 4);
    ar->Write(&m_moveSpeed, 8);
    ar->Write(&m_418, 4);
    ar->Write(&m_42c, 4);
    ar->Write(&m_430, 4);
    ar->Write(&m_434, 4);
    ar->Write(&m_438, 4);
    ar->Write(&m_arrivalState, 4);
    ar->Write(&m_defenderState, 4);
    ar->Write(&m_2d8, 4);
    ar->Write(&m_defenderRadius, 4);
    ar->Write(&m_2e0, 4);
    ar->Write(&m_2e4, 4);
    ar->Write(&m_dwell, 4);
    ar->Write(&m_arrivalCol, 8);
    ar->Write(&m_defenderX, 8);
    ar->Write(&m_354, 4);
    ar->Write(&m_358, 4);
    ar->Write(&m_35c, 4);
    ar->Write(&m_3dc, 8);
    ar->Write(&m_moveTileX, 8);
    ar->Write(&m_arrivalPhase, 4);
    ar->Write(&m_timePerTile, 4);
    ar->Write(&m_408, 8);
    ar->Write(&m_410, 8);
    ar->Write(&m_8d0, 4);
    ar->Write(&m_coordToggle, 4);
    ar->Write(&m_wingzEnabled, 4);
    ar->Write(&m_freezeDelayDone, 4);
    ar->Write(&m_freezeUnfrozen, 4);
    ar->Write(&m_resetApplied, 4);
    ar->Write(&m_arrivalFlags, 4);
    ar->Write(&m_24c, 4);
    ar->Write(&m_gruntKind, 4);
    ar->Write(&m_entranceArmed, 4);
    ar->Write(&m_deathType, 4);
    ar->Write(&m_entranceDropActive, 4);
    ar->Write(&m_318, 4);
    ar->Write(&m_2f8, 8);
    ar->Write(&m_36c, 4);
    ar->Write(&m_454, 4);
    ar->Write(&m_370, 4);
    ar->Write(&m_tileClaimed, 4);
    ar->Write(&m_deathAnimStarted, 4);
    ar->Write(&m_458, 8);
    ar->Write(&m_250, 4);
    ar->Write(&m_254, 4);
    ar->Write(&m_374, 4);
    ar->Write(&m_moveKind, 4);
    ar->Write(&m_moveVariant, 4);
    ar->Write(&m_coordRetryCount, 4);
    ar->Write(&m_toyTileIndex, 4);
    ar->Write(&m_390, 4);
    ar->Write(&m_378, 4);
    ar->Write(&m_38c, 4);
    ar->Write(&m_lowStaminaCued, 4);
    ar->Write(&m_2e8, 4);
    ar->Write(&m_288, 8);

    {
        i32 row, col;
        for (row = 0; row < 3; row++) {
            for (col = 0; col < 3; col++) {
                if (m_cells[3 * row + col].SerializeStrings(ar) == 0) {
                    return 0;
                }
            }
        }
    }

    {
        n = m_31c.GetCount();
        ar->Write(&n, 4);
        POSITION cpos = m_31c.GetHeadPosition();
        while (cpos != 0) {
            ar->Write(m_31c.GetNext(cpos), 8);
        }
    }
    {
        n = m_338.GetCount();
        ar->Write(&n, 4);
        POSITION pos = m_338.GetHeadPosition();
        while (pos != 0) {
            ar->Write(m_338.GetNext(pos), 0x2c);
        }
    }
    return 1;
}
