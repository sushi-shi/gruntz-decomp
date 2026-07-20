#include <Mfc.h> // real ::CPtrArray (CGruntzMapMgr::m_arr) - MFC umbrella kept first
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <rva.h>
#include <Ints.h>
#include <Gruntz/GruntzMapMgr.h>      // CGruntzMapMgr : CMapMgr (the +0x70 grid container)
#include <Gruntz/Brickz.h>            // BrickzCell (the 0x1c-byte grid cell)
#include <Gruntz/GameRegistry.h>      // CGameRegistry (*0x64556c); m_world == CDDrawSurfaceMgr
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup (object mgr) + CDDrawGroupNode (live list)
#include <Gruntz/UserLogic.h>    // CGameObject (walked sprite) + AnimWorkerObj (m_7c) + g_buteMgr
#include <Gruntz/GameLevel.h>    // CGameLevel (m_world->m_level; m_mainPlane @+0x5c)
#include <Wwd/WwdFile.h>         // CPlaneRender - the raw tile-grid facet of the main plane
#include <Bute/ButeMgr.h>        // CButeMgr::GetInt (g_buteMgr @0x6453d8)
#include <Gruntz/FreeNodePool.h> // g_coordPool @0x645540 + CoordPoolNode (recycled coord node)

#include <stdlib.h> // rand (0x11fee0, the engine rng)

static i32 PickA(i32 total, i32 t1, i32 t2, i32 t3, i32 t4) {
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
static i32 PickB(i32 total, i32 t1, i32 t2, i32 t3, i32 t4) {
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
static i32 PickC(i32 total, i32 t1, i32 t2, i32 t3, i32 t4) {
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

// @early-stop
// jump-table-data scoring artifact + regalloc-cascade wall (verified base-vs-target
// with llvm-objdump -dr): the head is byte-faithful (g_gameReg->m_world / m_attrMgr,
// the m_24->m_mainPlane grid gate + early return, the AllocGrid call, the five
// Brickz-section GetInt threshold sums with the correct pooled strings). Two
// divergences drag the whole symbol to ~0% and neither is source-steerable here:
// (1) the two dense switches (the [0x12f,0x149] colour selector + the 0x99-case
// cell-flag packer) emit byte-index LUT + jump-table data whose self-relocs the
// delinker types REL32 where cl emits $L (same wall as CMapMgr::ComputeCellFlags);
// (2) MSVC5 pins `this` in ebp + defers the esi save (frame 0x48) where retail pins
// `this` in esi + shrink-wraps only edi (frame 0x40), a whole-function regalloc
// cascade. Parked for the final sweep (a leaf-first redo).
RVA(0x000810f0, 0x8b4)
i32 CGruntzMapMgr::LoadAttributes(i32 width, i32 height) {
    m_attrMgr = g_gameReg->m_world;
    CPlaneRender* grid = m_attrMgr->m_level->m_mainPlane;
    if (grid == 0) {
        return 0;
    }
    AllocGrid(width, height, 0);
    m_90 = 0;

    // Cumulative brick-colour thresholds off the "Brickz" bute section.
    i32 t1 = g_buteMgr.GetInt("Brickz", "Brown");
    i32 t2 = t1 + g_buteMgr.GetInt("Brickz", "Red");
    i32 t3 = t2 + g_buteMgr.GetInt("Brickz", "Blue");
    i32 t4 = t3 + g_buteMgr.GetInt("Brickz", "Gold");
    i32 total = t4 + g_buteMgr.GetInt("Brickz", "Black");

    BrickzCell* cell = m_cellPool;
    for (i32 col = 0; col < static_cast<i32>(m_10); col++) {
        for (i32 row = 0; row < static_cast<i32>(m_c); row++, cell++) {
            i32 tileId = grid->m_tileGrid[grid->m_colOffsets[col] + row];
            if (tileId != -1) {
                tileId &= 0xffff;
            }
            cell->m_0 = 0;
            cell->m_4 = -1;
            cell->m_8 = 0;
            cell->m_c = -1;

            // Editor mode: remap the two placeholder tiles in-place.
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
                if (tileId == 0x105) {
                    tileId = 0x101;
                    grid->m_tileGrid[grid->m_colOffsets[col] + row] = 0x101;
                } else if (tileId == 0x106) {
                    tileId = 0x103;
                    grid->m_tileGrid[grid->m_colOffsets[col] + row] = 0x103;
                }
            }

            // When NOT in test mode, roll a brick-colour id for the tile range.
            if (g_gameReg->m_134 != 1) {
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

            // Inline ComputeCellFlags: look up the type code, pack the flags.
            i32 typeCode = m_attrMgr->m_level->LookupTile(row, col);
            i32 oldFlags = cell->m_0;
            i32 keep = oldFlags & 0x1bf40000;
            i32 edgeBit = oldFlags & 0x20000000;
            switch (typeCode - 1) {
                case 0:
                    cell->m_0 = 0x1;
                    break;
                case 9:
                    cell->m_0 = 0x100;
                    break;
                case 113:
                    cell->m_0 = 0x300;
                    break;
                case 35:
                    cell->m_0 = 0x800;
                    break;
                case 92:
                case 94:
                case 96:
                case 98:
                case 100:
                case 102:
                case 104:
                    cell->m_0 = 0x4002008;
                    break;
                case 29:
                case 30:
                case 32:
                    cell->m_0 = 0x2021;
                    break;
                case 150:
                case 151:
                case 152:
                    cell->m_0 = 0x6021;
                    break;
                case 149:
                    cell->m_0 = 0x8000;
                    break;
                case 153:
                    cell->m_0 = 0x2001;
                    break;
                case 107:
                    cell->m_0 = 0x108;
                    break;
                case 109:
                    cell->m_0 = 0xa;
                    break;
                case 3:
                    cell->m_0 = 0x2;
                    break;
                case 34:
                    cell->m_0 = 0x42;
                    break;
                case 33:
                    cell->m_0 = 0x10000;
                    break;
                case 115:
                    cell->m_0 = 0x202;
                    break;
                case 50:
                case 51:
                case 52:
                case 53:
                case 54:
                case 55:
                case 56:
                case 57:
                case 58:
                case 59:
                case 60:
                case 61:
                case 62:
                case 63:
                case 64:
                case 65:
                    cell->m_0 = 0x4;
                    break;
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                case 15:
                case 16:
                case 17:
                case 18:
                    cell->m_0 = 0x80;
                    break;
                case 31:
                    cell->m_0 = 0x400;
                    break;
                default:
                    cell->m_0 = (tileId == -1) ? 2 : 0;
                    break;
            }
            if (edgeBit != 0) {
                cell->m_0 |= 0x20000000;
            }
            cell->m_0 |= keep;
            cell->m_c = tileId;
            cell->m_10 = typeCode;

            // 8-neighbour diagonal-passability walk over the 3x3 block.
            if ((cell->m_0 & 0x100) != 0) {
                i32 colCount = m_c;
                for (i32 r = row - 1; r <= row + 1; r++) {
                    if (r < 0 || static_cast<u32>(r) >= static_cast<u32>(m_c)) {
                        continue;
                    }
                    for (i32 c = col - 1; c <= col + 1; c++) {
                        if (c < 0 || static_cast<u32>(c) >= static_cast<u32>(m_10)) {
                            continue;
                        }
                        BrickzCell* nc = &m_rows[c][r];
                        i32 nf = nc->m_0 & ~0x1000;
                        nc->m_0 = nf;
                        if ((nf & 0x100) == 0) {
                            continue;
                        }
                        BrickzCell* up = (r != 0) ? nc - 1 : 0;
                        BrickzCell* down = (r < colCount - 1) ? nc + 1 : 0;
                        BrickzCell* right = (c < static_cast<i32>(m_10) - 1) ? nc + colCount : 0;
                        BrickzCell* left = (c != 0) ? nc - colCount : 0;
                        BrickzCell* ur = (up && right) ? up + colCount : 0;
                        BrickzCell* dl = (down && left) ? down - colCount : 0;
                        BrickzCell* ul = (up && left) ? up - colCount : 0;
                        BrickzCell* dr = (down && right) ? down + colCount : 0;
                        bool set = false;
                        if (up && down && !(up->m_0 & 0x939) && !(down->m_0 & 0x939)) {
                            set = true;
                        } else if (right && left && !(right->m_0 & 0x939) && !(left->m_0 & 0x939)) {
                            set = true;
                        } else if (ur && dl && !(ur->m_0 & 0x939) && !(dl->m_0 & 0x939)) {
                            set = true;
                        } else if (ul && dr && !(ul->m_0 & 0x939) && !(dr->m_0 & 0x939)) {
                            set = true;
                        }
                        if (set) {
                            nc->m_0 = nf | 0x1000;
                        }
                    }
                }
            }
        }
    }

    // Freelist-recycle pass: for each moving object of the footprint kind, seed a
    // 3x3 footprint of recycled free nodes, then commit them into the grid.
    CDDrawChildGroup* mgr = g_gameReg->m_world->m_childGroup;
    mgr->m_walkCursor = reinterpret_cast<CDDrawGroupNode*>(mgr->m_list.GetHeadPosition());
    CGameObject* obj;
    if (mgr->m_walkCursor != 0) {
        CDDrawGroupNode* n = mgr->m_walkCursor;
        mgr->m_walkCursor = n->m_next;
        obj = n->m_gameObj;
    } else {
        obj = 0;
    }
    while (obj != 0) {
        // The footprint kind is the object whose worker's post-create notify hook is
        // &CreateExitTrigger (@0x40192e); m_notify is a raw fn-ptr, reinterpreted to int
        // exactly as retail's `cmp [worker+0x10], 0x40192e`.
        if (reinterpret_cast<i32>(obj->m_7c->m_notify) == 0x40192e) {
            i32 tileX = (obj->m_screenX + (obj->m_screenX >> 31 & 0x1f)) >> 5;
            i32 tileY = (obj->m_screenY + (obj->m_screenY >> 31 & 0x1f)) >> 5;
            for (i32 xo = -1; xo < 2; xo++) {
                for (i32 yo = -1; yo < 2; yo++) {
                    // Pop a CoordPoolNode off g_coordPool: the {x,y} payload (a Coord) sits
                    // one link-offset (4) past the head, the m_next link is at head+0. Same
                    // void* generic-free-list idiom as Grunt.cpp's recycle. Retail hardcodes
                    // the +4 in the pop (and reads m_linkOffset only in the push below).
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
                if (elem != 0 && static_cast<u32>(elem->m_x) < static_cast<u32>(m_c) && static_cast<u32>(elem->m_y) < static_cast<u32>(m_10)) {
                    m_cellPool[elem->m_y * m_c + elem->m_x].m_0 = 0x10;
                    m_cellPool[elem->m_y * m_c + elem->m_x].m_c = 0;
                    // Recycle: recover the raw node (payload - m_linkOffset) and relink onto
                    // the free-list head (the Grunt.cpp void** idiom).
                    CoordPoolNode* node = g_coordPool.NodeOf(elem);
                    node->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = node;
                }
            }
            m_arr.SetSize(0, -1);
        }
        if (mgr->m_walkCursor != 0) {
            CDDrawGroupNode* n = mgr->m_walkCursor;
            mgr->m_walkCursor = n->m_next;
            obj = n->m_gameObj;
        } else {
            obj = 0;
        }
    }

    m_dirty = 1;
    return 1;
}
