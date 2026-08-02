#include <Mfc.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <rva.h>
#include <Ints.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegistry.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/GameLevel.h>
#include <Wwd/WwdFile.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameObjectFactory.h>

#include <stdlib.h>

static __inline i32 PickA(i32 total, i32 t1, i32 t2, i32 t3, i32 t4) {
    i32 roll = (total == 0) ? (rand() & 1) : (rand() % total + 1);
    if (roll <= t1) {
        return 0x12f;
    }
    if (roll <= t2) {
        return 0x132;
    }
    if (roll <= t3) {
        return 0x138;
    }
    return (roll > t4) ? 0x144 : 0x13e;
}
static __inline i32 PickB(i32 total, i32 t1, i32 t2, i32 t3, i32 t4) {
    i32 roll = (total == 0) ? (rand() & 1) : (rand() % total + 1);
    if (roll <= t1) {
        return 0x130;
    }
    if (roll <= t2) {
        return (rand() % 100 + 1 <= 0x32) ? 0x134 : 0x133;
    }
    if (roll <= t3) {
        return (rand() % 100 + 1 <= 0x32) ? 0x13a : 0x139;
    }
    if (roll <= t4) {
        return (rand() % 100 + 1 <= 0x32) ? 0x140 : 0x13f;
    }
    return (rand() % 100 + 1 <= 0x32) ? 0x146 : 0x145;
}
static __inline i32 PickC(i32 total, i32 t1, i32 t2, i32 t3, i32 t4) {
    i32 roll = (total == 0) ? (rand() & 1) : (rand() % total + 1);
    if (roll <= t1) {
        return 0x131;
    }
    if (roll <= t2) {
        i32 r = rand() % 0x258 + 1;
        if (r <= 0xc8) {
            return 0x135;
        }
        return (r > 0x190) ? 0x137 : 0x136;
    }
    if (roll <= t3) {
        i32 r = rand() % 0x258 + 1;
        if (r <= 0xc8) {
            return 0x13b;
        }
        return (r > 0x190) ? 0x13d : 0x13c;
    }
    if (roll <= t4) {
        i32 r = rand() % 0x258 + 1;
        if (r <= 0xc8) {
            return 0x141;
        }
        return (r > 0x190) ? 0x143 : 0x142;
    }
    i32 r = rand() % 0x258 + 1;
    if (r <= 0xc8) {
        return 0x147;
    }
    return (r > 0x190) ? 0x149 : 0x148;
}

RVA(0x000810f0, 0x8b4)
i32 CGruntzMapMgr::LoadAttributes(i32 width, i32 height) {
    m_attrMgr = g_gameReg->m_world;
    CDDrawWorkerHost* grid = m_attrMgr->m_level->m_mainPlane;
    if (grid == 0) {
        return 0;
    }
    AllocGrid(width, height, 0);
    m_90 = 0;

    i32 t1 = g_buteMgr.GetInt("Brickz", "Brown");
    i32 t2 = t1 + g_buteMgr.GetInt("Brickz", "Red");
    i32 t3 = t2 + g_buteMgr.GetInt("Brickz", "Blue");
    i32 t4 = t3 + g_buteMgr.GetInt("Brickz", "Gold");
    i32 total = t4 + g_buteMgr.GetInt("Brickz", "Black");

    BrickzCell* cell = m_cellPool;
    for (u32 col = 0; col < m_height; col++) {
        for (u32 row = 0; row < m_width; row++, cell++) {
            i32 tileId = grid->m_tileGrid[grid->m_colOffsets[col] + row];
            if (tileId != -1) {
                tileId &= 0xffff;
            }
            cell->m_flags = 0;
            cell->m_occupantId = -1;
            cell->m_objectId = 0;
            cell->m_tileId = -1;

            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == 1) {
                if (tileId == 0x105) {
                    tileId = 0x101;
                    grid->m_tileGrid[grid->m_colOffsets[col] + row] = 0x101;
                } else if (tileId == 0x106) {
                    tileId = 0x103;
                    grid->m_tileGrid[grid->m_colOffsets[col] + row] = 0x103;
                }
            }

            if (g_gameReg->m_gameMode != 1) {
                switch (tileId) {
                    case 0x12f:
                    case 0x132:
                    case 0x138:
                    case 0x13e:
                    case 0x144:
                        tileId = PickA(total, t1, t2, t3, t4);
                        break;
                    case 0x130:
                    case 0x133:
                    case 0x134:
                    case 0x139:
                    case 0x13a:
                    case 0x13f:
                    case 0x140:
                    case 0x145:
                    case 0x146:
                        tileId = PickB(total, t1, t2, t3, t4);
                        break;
                    case 0x131:
                    case 0x135:
                    case 0x136:
                    case 0x137:
                    case 0x13b:
                    case 0x13c:
                    case 0x13d:
                    case 0x141:
                    case 0x142:
                    case 0x143:
                    case 0x147:
                    case 0x148:
                    case 0x149:
                        tileId = PickC(total, t1, t2, t3, t4);
                        break;
                    default:
                        break;
                }
                grid->m_tileGrid[grid->m_colOffsets[col] + row] = tileId;
            }

            i32 typeCode = m_attrMgr->m_level->LookupTile(row, col);
            i32 oldFlags = cell->m_flags;
            i32 keep = oldFlags & 0x1bf40000;
            i32 edgeBit = oldFlags & 0x20000000;
            switch (typeCode - 1) {
                case 0:
                    cell->m_flags = 0x1;
                    break;
                case 3:
                    cell->m_flags = 0x2;
                    break;
                case 9:
                    cell->m_flags = 0x100;
                    break;
                case 10:
                    cell->m_flags = 0x80;
                    break;
                case 11:
                    cell->m_flags = 0x80;
                    break;
                case 12:
                    cell->m_flags = 0x80;
                    break;
                case 13:
                    cell->m_flags = 0x80;
                    break;
                case 14:
                    cell->m_flags = 0x80;
                    break;
                case 15:
                    cell->m_flags = 0x80;
                    break;
                case 16:
                    cell->m_flags = 0x80;
                    break;
                case 17:
                    cell->m_flags = 0x80;
                    break;
                case 18:
                    cell->m_flags = 0x80;
                    break;
                case 29:
                    cell->m_flags = 0x2021;
                    break;
                case 30:
                    cell->m_flags = 0x2021;
                    break;
                case 31:
                    cell->m_flags = 0x400;
                    break;
                case 32:
                    cell->m_flags = 0x2021;
                    break;
                case 33:
                    cell->m_flags = 0x10000;
                    break;
                case 34:
                    cell->m_flags = 0x42;
                    break;
                case 35:
                    cell->m_flags = 0x800;
                    break;
                case 50:
                    cell->m_flags = 0x4;
                    break;
                case 51:
                    cell->m_flags = 0x4;
                    break;
                case 52:
                    cell->m_flags = 0x4;
                    break;
                case 53:
                    cell->m_flags = 0x4;
                    break;
                case 54:
                    cell->m_flags = 0x4;
                    break;
                case 55:
                    cell->m_flags = 0x4;
                    break;
                case 56:
                    cell->m_flags = 0x4;
                    break;
                case 57:
                    cell->m_flags = 0x4;
                    break;
                case 58:
                    cell->m_flags = 0x4;
                    break;
                case 59:
                    cell->m_flags = 0x4;
                    break;
                case 60:
                    cell->m_flags = 0x4;
                    break;
                case 61:
                    cell->m_flags = 0x4;
                    break;
                case 62:
                    cell->m_flags = 0x4;
                    break;
                case 63:
                    cell->m_flags = 0x4;
                    break;
                case 64:
                    cell->m_flags = 0x4;
                    break;
                case 65:
                    cell->m_flags = 0x4;
                    break;
                case 92:
                    cell->m_flags = 0x4002008;
                    break;
                case 94:
                    cell->m_flags = 0x4002008;
                    break;
                case 96:
                    cell->m_flags = 0x4002008;
                    break;
                case 98:
                    cell->m_flags = 0x4002008;
                    break;
                case 100:
                    cell->m_flags = 0x4002008;
                    break;
                case 102:
                    cell->m_flags = 0x4002008;
                    break;
                case 104:
                    cell->m_flags = 0x4002008;
                    break;
                case 107:
                    cell->m_flags = 0x108;
                    break;
                case 109:
                    cell->m_flags = 0xa;
                    break;
                case 113:
                    cell->m_flags = 0x300;
                    break;
                case 115:
                    cell->m_flags = 0x202;
                    break;
                case 149:
                    cell->m_flags = 0x8000;
                    break;
                case 150:
                    cell->m_flags = 0x6021;
                    break;
                case 151:
                    cell->m_flags = 0x6021;
                    break;
                case 152:
                    cell->m_flags = 0x6021;
                    break;
                case 153:
                    cell->m_flags = 0x2001;
                    break;
                default:
                    cell->m_flags = (tileId == -1) ? 2 : 0;
                    break;
            }
            if (edgeBit != 0) {
                cell->m_flags |= 0x20000000;
            }
            cell->m_flags |= keep;
            cell->m_tileId = tileId;
            cell->m_typeCode = typeCode;

            if ((cell->m_flags & 0x100) != 0) {
                i32 colCount = m_width;
                for (i32 r = row - 1; r <= row + 1; r++) {
                    if (r < 0 || static_cast<u32>(r) >= static_cast<u32>(m_width)) {
                        continue;
                    }
                    for (i32 c = col - 1; c <= col + 1; c++) {
                        if (c < 0 || static_cast<u32>(c) >= static_cast<u32>(m_height)) {
                            continue;
                        }
                        BrickzCell* nc = &m_rows[c][r];
                        i32 nf = nc->m_flags & ~0x1000;
                        nc->m_flags = nf;
                        if ((nf & 0x100) == 0) {
                            continue;
                        }
                        BrickzCell* up = (r != 0) ? nc - 1 : 0;
                        BrickzCell* down = (r < colCount - 1) ? nc + 1 : 0;
                        BrickzCell* right =
                            (c < static_cast<i32>(m_height) - 1) ? nc + colCount : 0;
                        BrickzCell* left = (c != 0) ? nc - colCount : 0;
                        BrickzCell* ur = (up && right) ? up + colCount : 0;
                        BrickzCell* dl = (down && left) ? down - colCount : 0;
                        BrickzCell* ul = (up && left) ? up - colCount : 0;
                        BrickzCell* dr = (down && right) ? down + colCount : 0;
                        bool set = false;
                        if (up && down && !(up->m_flags & 0x939) && !(down->m_flags & 0x939)) {
                            set = true;
                        } else if (right && left && !(right->m_flags & 0x939)
                                   && !(left->m_flags & 0x939)) {
                            set = true;
                        } else if (ur && dl && !(ur->m_flags & 0x939) && !(dl->m_flags & 0x939)) {
                            set = true;
                        } else if (ul && dr && !(ul->m_flags & 0x939) && !(dr->m_flags & 0x939)) {
                            set = true;
                        }
                        if (set) {
                            nc->m_flags = nf | 0x1000;
                        }
                    }
                }
            }
        }
    }

    CDDrawChildGroup* mgr = g_gameReg->m_world->m_childGroup;
    mgr->m_walkCursor = mgr->m_list.GetHeadPosition();
    CGameObject* obj;
    if (mgr->m_walkCursor != 0) {
        obj = static_cast<CGameObject*>(mgr->m_list.GetNext(mgr->m_walkCursor));
    } else {
        obj = 0;
    }
    while (obj != 0) {

        if (obj->m_animWorker->m_notify == &CreateExitTrigger) {
            i32 tileX = (obj->m_screenX + (obj->m_screenX >> 31 & 0x1f)) >> 5;
            i32 tileY = (obj->m_screenY + (obj->m_screenY >> 31 & 0x1f)) >> 5;
            for (i32 xo = -1; xo < 2; xo++) {
                for (i32 yo = -1; yo < 2; yo++) {

                    Coord* elem = 0;
                    if (g_coordPool.m_freeHead->m_next != 0) {
                        elem = &g_coordPool.m_freeHead->m_coord;
                        elem->m_x = tileX + xo;
                        elem->m_y = tileY + yo;
                        g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
                    }
                    m_arr.SetAtGrow(m_arr.GetSize(), elem);
                }
            }
            for (i32 k = 0; k < m_arr.GetSize(); k++) {
                Coord* elem = static_cast<Coord*>(m_arr[k]);
                if (elem != 0 && static_cast<u32>(elem->m_x) < static_cast<u32>(m_width)
                    && static_cast<u32>(elem->m_y) < static_cast<u32>(m_height)) {
                    m_cellPool[elem->m_y * m_width + elem->m_x].m_flags = 0x10;
                    m_cellPool[elem->m_y * m_width + elem->m_x].m_tileId = 0;

                    CoordPoolNode* node = g_coordPool.NodeOf(elem);
                    node->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = node;
                }
            }
            m_arr.SetSize(0, -1);
        }
        if (mgr->m_walkCursor != 0) {
            obj = static_cast<CGameObject*>(mgr->m_list.GetNext(mgr->m_walkCursor));
        } else {
            obj = 0;
        }
    }

    m_dirty = 1;
    return 1;
}
