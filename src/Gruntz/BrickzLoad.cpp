#include <rva.h>
#include <Ints.h>
#include <Bute/ButeMgr.h>        // CButeMgr::GetInt (g_buteMgr)
#include <Gruntz/Viewport.h>     // shared world-plane grid (the terrain descriptor)
#include <Gruntz/GameRegistry.h> // canonical *0x24556c singleton (level mgr via m_world)

#include <stdlib.h> // rand (0x11fee0, the engine rng)
// BrickzLoad.cpp - CBrickz::LoadAttributes (0x0810f0), the level-load terrain
// parser for the self-contained pathfinding grid container (the placeholder
// "CBrickz" in <Gruntz/Brickz.h>; the SAME container AllocGrid/ComputeCellFlags
// run on, NOT the CUserLogic-derived game object).
//
// The function allocates the grid (AllocGrid), reads the five brick-colour counts
// (Brown/Red/Blue/Gold/Black) from the "Brickz" bute section into cumulative
// thresholds, then walks every cell (col x row):
//   1. read the source tile id; in editor mode remap 0x105->0x101 / 0x106->0x103;
//   2. when NOT in test mode, roll a random brick-colour id for the [0x12f,0x149]
//      tile range via three weighted selectors (dense jump table);
//   3. inline the per-cell ComputeCellFlags: LookupTile -> pack cell->m_0 via a
//      second dense jump table -> run the 8-neighbour diagonal-passability walk.
// Then a freelist-recycle pass seeds the moving-object footprints and finally
// stamps the dirty flag m_5c = 1.
//
// The two jump-table data regions score as a delinker artifact (REL32 vs cl's $L
// self-relocs, same as CBrickz::ComputeCellFlags), so the whole symbol reads ~0%;
// the logic is byte-faithful. Field names are placeholders; the OFFSETS + code
// bytes are the load-bearing fact.

// ---------------------------------------------------------------------------
// Modelled engine views (only the touched members/methods; every call is
// reloc-masked).

// A grid cell (0x1c bytes): packed terrain flags at +0, the resolved tile id at
// +0xc, the bute type code at +0x10.
struct BzCell {
    i32 m_0;      // +0x00  packed terrain flags
    i32 m_4;      // +0x04
    BzCell* m_8;  // +0x08
    i32 m_c;      // +0x0c  resolved tile id
    i32 m_10;     // +0x10  bute type code
    i32 m_14;     // +0x14
    BzCell* m_18; // +0x18
};

// The terrain grid descriptor (attr->m_5c): a flat id table (m_20) indexed by a
// per-row base-offset table (m_24).
// The terrain grid descriptor (m_78->m_24->m_5c) is the shared world-plane
// CViewport (<Gruntz/Viewport.h>): cell = m_cells[m_rowBase[col] + row].

// The attribute/bute-type manager (this->m_78->m_24): the grid descriptor at
// +0x5c and the per-cell tile-type lookup at 0x082600 (ILT thunk 0x4228).
struct BzAttr {
    // 0x082600 (via ILT 0x4228), __thiscall(row, col): returns the 1-based tile
    // type code for cell (row,col).
    i32 LookupTile(i32 row, i32 col); // 0x082600
    char m_pad0[0x5c];
    CViewport* m_5c; // +0x5c  the terrain grid descriptor (world-plane)
};

// The level manager reached as (BzLevelMgr*)g_gameReg->m_world: the attribute manager at
// +0x24 and the moving-object manager at +0x8.
struct BzMovingObj;
struct BzObjMgr {
    char m_pad0[0x14];
    BzMovingObj* m_14; // +0x14  moving-object list head-source
    char m_pad18[0x64 - 0x18];
    BzMovingObj* m_64; // +0x64  walk cursor
};
struct BzLevelMgr {
    char m_pad0[0x8];
    BzObjMgr* m_8; // +0x08  moving-object manager
    char m_pad0c[0x24 - 0xc];
    BzAttr* m_24; // +0x24  attribute/bute-type manager
};

// The level manager is the Brickz-loader facet of the canonical registry's reused
// +0x30 world/resource slot ((BzLevelMgr*)g_gameReg->m_world; see CGameRegistry.h);
// the editor/test-mode gates are the registry's m_isEasyMode (+0x118) and m_134
// (+0x134, 1 => test) discriminators. Authentic per-mode downcast of the singleton.
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// A moving object off the level manager's list: a type record at +0x7c whose
// +0x10 identifies the class (0x40192e), and its tile position at +0x5c/+0x60.
struct BzMovingObjType {
    char m_pad0[0x10];
    i32 m_10; // +0x10  class id (== 0x40192e for the footprint kind)
};
struct BzMovingObj {
    BzMovingObj* m_0; // +0x00  free-list next link
    char m_pad4[0x8 - 0x4];
    BzMovingObj* m_8; // +0x08  object payload
    char m_pad0c[0x5c - 0xc];
    i32 m_5c; // +0x5c  world x
    i32 m_60; // +0x60  world y
    char m_pad64[0x7c - 0x64];
    BzMovingObjType* m_7c; // +0x7c  type record
};

// A recycled footprint node off the shared free-list (g_freeList): the (col,row)
// pair the recycle pass stamps into the grid.
struct BzFreeNode {
    BzFreeNode* m_0; // +0x00  next-free link / col write base
    i32 m_4;         // +0x04  col
    i32 m_8;         // +0x08  row
};
// ?g_freeList@@3PAXA (0x645544) / ?g_freeListNodeBias@@3HA (0x64554c).
DATA(0x00245544)
extern BzFreeNode* g_freeList;
DATA(0x0024554c)
extern i32 g_freeListNodeBias;

// CPtrArray helpers on the container's +0x7c footprint array (m_80 = data,
// m_84 = count). SetAtGrow (0x1b5144, __thiscall ret 8), SetSize (0x1b4f75,
// __thiscall ret 8). Modelled no-body so the calls reloc-mask.
struct BzPtrArray {
    void SetAtGrow(i32 index, void* elem); // 0x1b5144
    void SetSize(i32 newSize, i32 growBy); // 0x1b4f75
};

// The global bute store (?g_buteMgr@@3VCButeMgr@@A, 0x6453d8); GetInt @0x171af0.
extern CButeMgr g_buteMgr;

// ---------------------------------------------------------------------------
// The pathfinding grid container. Minimal per-file view (offsets are the
// load-bearing fact); the full container lives in <Gruntz/Brickz.h>.
class CBrickz {
public:
    i32 LoadAttributes(i32 width, i32 height);    // 0x0810f0
    i32 AllocGrid(i32 width, i32 height, i32 cb); // 0x09ea60 (via ILT 0x412e)

    char m_pad0[0x4];
    BzCell* m_4;  // +0x04  flat cell pool
    BzCell** m_8; // +0x08  column table
    i32 m_c;      // +0x0c  height
    i32 m_10;     // +0x10  width
    char m_pad14[0x5c - 0x14];
    i32 m_5c; // +0x5c  dirty flag (stamped 1 at the end)
    char m_pad60[0x78 - 0x60];
    BzLevelMgr* m_78;       // +0x78  level manager (= (BzLevelMgr*)g_gameReg->m_world)
    BzPtrArray m_footprint; // +0x7c  footprint CPtrArray (m_80 data, m_84 count)
    void** m_80;            // +0x80  footprint array data
    i32 m_84;               // +0x84  footprint array count
    char m_pad88[0x90 - 0x88];
    i32 m_90; // +0x90  cleared at start
};

// A weighted colour selector: rolls rand()%total (+1) or rand()&1 when total==0,
// walks the four cumulative thresholds t1..t4 and returns the picked tile id.
// Block A: solid colours (0x12f/0x132/0x138/0x13e/0x144).
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
// Block B: a 50/50 secondary variant on colours red..black (%100 threshold 50).
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
// Block C: a 3-way variant (%600 thresholds 200/400) on colours.
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
// with llvm-objdump -dr): the head is byte-faithful (g_gameReg->m_world / m_78,
// the m_24->m_5c grid gate + early return, the AllocGrid call, the five
// Brickz-section GetInt threshold sums with the correct pooled strings). Two
// divergences drag the whole symbol to ~0% and neither is source-steerable here:
// (1) the two dense switches (the [0x12f,0x149] colour selector + the 0x99-case
// cell-flag packer) emit byte-index LUT + jump-table data whose self-relocs the
// delinker types REL32 where cl emits $L (same wall as CBrickz::ComputeCellFlags);
// (2) MSVC5 pins `this` in ebp + defers the esi save (frame 0x48) where retail pins
// `this` in esi + shrink-wraps only edi (frame 0x40), a whole-function regalloc
// cascade. The empty stub's fuzzy 14.9% was size-mismatch noise; this is the
// complete correct body. Parked for the final sweep (a leaf-first redo).
RVA(0x000810f0, 0x8b4)
i32 CBrickz::LoadAttributes(i32 width, i32 height) {
    m_78 = (BzLevelMgr*)g_gameReg->m_world;
    CViewport* grid = m_78->m_24->m_5c;
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

    BzCell* cell = m_4;
    for (i32 col = 0; col < m_10; col++) {
        for (i32 row = 0; row < m_c; row++, cell++) {
            i32 tileId = grid->m_cells[grid->m_rowBase[col] + row];
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
                    grid->m_cells[grid->m_rowBase[col] + row] = 0x101;
                } else if (tileId == 0x106) {
                    tileId = 0x103;
                    grid->m_cells[grid->m_rowBase[col] + row] = 0x103;
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
                grid->m_cells[grid->m_rowBase[col] + row] = tileId;
            }

            // Inline ComputeCellFlags: look up the type code, pack the flags.
            i32 typeCode = m_78->m_24->LookupTile(row, col);
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
                    if (r < 0 || (u32)r >= (u32)m_c) {
                        continue;
                    }
                    for (i32 c = col - 1; c <= col + 1; c++) {
                        if (c < 0 || (u32)c >= (u32)m_10) {
                            continue;
                        }
                        BzCell* nc = &m_8[c][r];
                        i32 nf = nc->m_0 & ~0x1000;
                        nc->m_0 = nf;
                        if ((nf & 0x100) == 0) {
                            continue;
                        }
                        BzCell* up = (r != 0) ? nc - 1 : 0;
                        BzCell* down = (r < colCount - 1) ? nc + 1 : 0;
                        BzCell* right = (c < m_10 - 1) ? nc + colCount : 0;
                        BzCell* left = (c != 0) ? nc - colCount : 0;
                        BzCell* ur = (up && right) ? up + colCount : 0;
                        BzCell* dl = (down && left) ? down - colCount : 0;
                        BzCell* ul = (up && left) ? up - colCount : 0;
                        BzCell* dr = (down && right) ? down + colCount : 0;
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
    BzObjMgr* mgr = ((BzLevelMgr*)g_gameReg->m_world)->m_8;
    mgr->m_64 = mgr->m_14;
    BzMovingObj* obj;
    if (mgr->m_64 != 0) {
        BzMovingObj* n = mgr->m_64;
        mgr->m_64 = n->m_0;
        obj = n->m_8;
    } else {
        obj = 0;
    }
    while (obj != 0) {
        if (obj->m_7c->m_10 == 0x40192e) {
            i32 tileX = (obj->m_5c + (obj->m_5c >> 31 & 0x1f)) >> 5;
            i32 tileY = (obj->m_60 + (obj->m_60 >> 31 & 0x1f)) >> 5;
            for (i32 xo = -1; xo < 2; xo++) {
                for (i32 yo = -1; yo < 2; yo++) {
                    BzFreeNode* node = 0;
                    if (*(void**)g_freeList != 0) {
                        node = g_freeList;
                        node->m_4 = tileX + xo;
                        node->m_8 = tileY + yo;
                        g_freeList = g_freeList->m_0;
                    }
                    m_footprint.SetAtGrow(m_84, node);
                }
            }
            for (i32 k = 0; k < m_84; k++) {
                BzFreeNode* node = (BzFreeNode*)m_80[k];
                if (node != 0 && (u32)node->m_4 < (u32)m_c && (u32)node->m_8 < (u32)m_10) {
                    m_4[node->m_8 * m_c + node->m_4].m_0 = 0x10;
                    m_4[node->m_8 * m_c + node->m_4].m_c = 0;
                    BzFreeNode* rec = (BzFreeNode*)((char*)node - g_freeListNodeBias);
                    rec->m_0 = g_freeList;
                    g_freeList = rec;
                }
            }
            m_footprint.SetSize(0, -1);
        }
        if (mgr->m_64 != 0) {
            BzMovingObj* n = mgr->m_64;
            mgr->m_64 = n->m_0;
            obj = n->m_8;
        } else {
            obj = 0;
        }
    }

    m_5c = 1;
    return 1;
}

SIZE_UNKNOWN(BzAttr);
SIZE_UNKNOWN(BzCell);
SIZE_UNKNOWN(BzFreeNode);
SIZE_UNKNOWN(BzLevelMgr);
SIZE_UNKNOWN(BzMovingObj);
SIZE_UNKNOWN(BzMovingObjType);
SIZE_UNKNOWN(BzObjMgr);
SIZE_UNKNOWN(BzPtrArray);
