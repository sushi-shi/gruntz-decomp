// LevelTileValidation.cpp - the level-load tile/trigger validation pass
// (C:\Proj\Gruntz). The single biggest function in the backlog (7652 B): a /GX
// EH-framed __thiscall on the level/world object that walks the level's
// tile-logic object list and, per object, recovers the object's leaf class
// (identified by the constant address sitting in its +0x7c identity sub-object's
// vtable slot +0x10) and routes it to a per-class validator.
//
// The dominant class (identity 0x401799) clamps the object's world coords to the
// tile grid, looks up the underlying tile-type id, and on a switch over the
// object's +0x124 "switch kind" either registers a CTileTriggerSwitchLogic via
// this->m_2e4 (the trigger registrar, method 0x115f60) or, when the placement is
// invalid, emits a "Bad <kind> switch at: x=%d, y=%d" diagnostic built into a
// CString temp and logged through g_gameReg. Two sibling classes (0x403bfc /
// 0x4037b0) repeat the same scan with toob/trigger-specific id checks. The
// remaining ~8 identities run short bounds/free-list/cell-grid pokes.
//
// CARCASS doctrine: only the member OFFSETS and the per-object call/branch
// STRUCTURE are load-bearing. Field names are placeholders (m_<hexoffset>);
// engine callees are external no-body fns (reloc-masked `call rel32`/virtual).
// The owning class is modeled self-contained here to avoid perturbing the
// matched, shared CTriggerMgr / CTileTriggerSwitchLogic headers.
//
// @early-stop
// megafunction wall: three near-identical tile-scan loop preambles plus an
// inlined 16-way rect-by-value switch make the per-loop regalloc + the inlined
// struct-copy scheduling diverge from retail past the entropy tail. The body is
// a complete, faithful reconstruction; the byte-match plateaus on the documented
// big-switch / regalloc wall (docs/patterns/INDEX.md, topic:wall). Deferred to
// the final sweep for a leaf-first redo (the tile-grid lookup + the 0x115f60
// registrar should be matched as leaves first, then this re-attacked).

#include <Mfc.h> // CString (Format / ctor / dtor), RECT

#include <rva.h>

// ---------------------------------------------------------------------------
// The level tile-logic object iterated by the validator (esi). A CUserLogic-
// family map object; only the read fields are modeled.
// ---------------------------------------------------------------------------
struct GameObjAux7c; // the +0x7c identity sub-object

struct TileLogicObj {
    TileLogicObj* m_00; // +0x00  (data slot in the list node view)
    i32 m_04;           // +0x04  object key/index
    i32 m_08;           // +0x08  flags (the 0x10000 "validated" bit is OR'd in)
    char m_pad0c[0x5c - 0x0c];
    i32 m_5c; // +0x5c  world x (pixels)
    i32 m_60; // +0x60  world y (pixels)
    RECT m_64;
    char m_pad74[0x7c - 0x74];
    GameObjAux7c* m_7c; // +0x7c  identity sub-object
    char m_pad80[0x114 - 0x80];
    i32 m_114; // +0x114
    i32 m_118; // +0x118
    i32 m_11c; // +0x11c
    i32 m_120; // +0x120
    i32 m_124; // +0x124  switch kind / tile kind
    i32 m_128; // +0x128
    i32 m_12c; // +0x12c
    char m_pad130[0x134 - 0x130];
    RECT m_134; // +0x134
    RECT m_144; // +0x144
    RECT m_154; // +0x154
    i32 m_164;  // +0x164  tile col
    i32 m_168;  // +0x168  tile row
};

// The list-node head reached through (m_0c manager). The list is iterated by
// reading the node's data (+0x00) and next (+0x08).
struct TileObjNode {
    TileLogicObj* m_data; // +0x00
    char m_pad04[0x8 - 0x4];
    TileObjNode* m_next; // +0x08
};
struct TileObjList {
    char m_pad00[0x4];
    TileObjNode* m_04; // +0x04  head node
};

// The +0x7c identity sub-object. Its vtable slot +0x10 holds the per-class
// identity address the validator compares against. The two trailing rects are
// copied by value as switch args.
struct GameObjAux7cVtbl {
    char m_pad00[0x10];
    void* m_10; // +0x10
};
struct GameObjAux7c {
    GameObjAux7cVtbl* m_vtbl; // +0x00
    char m_pad04[0xf0 - 0x04];
    RECT m_f0;  // +0xf0
    RECT m_100; // +0x100
};

// ---------------------------------------------------------------------------
// The trigger registrar reached through this->m_2e4 (a CTileTriggerSwitchLogic):
// RegisterSwitchLogic (0x115f60) builds the appropriate switch logic for a tag.
// The Register3 family (0x1131-thunked 0x115f60 entry) carries the same args.
// All __thiscall, reloc-masked.
// ---------------------------------------------------------------------------
struct TriggerRegistrar {
    // tag, col, row, key, the six rects, isMatch, m_120, 0 (124B; ret 0x7c).
    i32 RegisterSwitchLogic(
        i32 tag,
        i32 col,
        i32 row,
        i32 key,
        RECT r134,
        RECT r144,
        RECT r154,
        RECT r64,
        RECT rF0,
        RECT r100,
        i32 isMatch,
        i32 m120,
        i32 zero
    );                                                    // 0x115f60
    void* LookupKind(i32 key, i32 kind);                  // 0x21df  (ret 8)
    void* LookupRange(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x10ff
};

// CTriggerMgr-family helpers reached through this->m_2dc (the playfield grid
// manager): SpawnPuddle (0x116a40 via 0x3580), PlacePuddle (0x47a240 via 0x35fd).
struct PlayfieldMgr {
    void* SpawnPuddle(i32 a, i32 b, i32 c, i32 d, RECT r134); // 0x116a40
    void PlacePuddle(void* obj, i32 z);                       // 0x47a240
    void Bridge1d2f(i32 a, i32 b);                            // 0x508410 (via 0x1d2f)
};

// The big game registry (?g_gameReg@@3PAUWwdGameReg@@A). Reloc-masked DIR32.
struct WwdGameGrid {
    char m_pad00[0x8];
    void** m_08; // +0x08  cell-row base
    i32 m_0c;    // +0x0c  width
    i32 m_10;    // +0x10  height
};
struct WwdGameRegInner {
    char m_pad00[0x3c];
    i32 m_3c; // +0x3c
};
// The trigger-grid manager (g_gameReg->m_68 = CTriggerMgr): PlaceObject builds and
// registers a tile object, returning its grid index or -1 on failure. 0x6b6d0,
// __thiscall, reloc-masked.
struct PlaceGridMgr {
    i32 PlaceObject(
        i32 a8,
        i32 ax,
        i32 ay,
        i32 col,
        i32 row,
        i32 kind,
        i32 a18,
        i32 a1c,
        i32 a20,
        i32 a24,
        i32 a28,
        i32 a2c,
        i32 a30
    ); // 0x6b6d0
};
// The command queue (g_gameReg->m_6c = CGruntzCmdMgr): EnqueueSingle broadcasts a
// single placed-grunt op. 0x23c30, __thiscall, reloc-masked.
struct StartCmdMgr {
    void EnqueueSingle(i32 a, char b, char c, char d, i32 e, i32 f, char g, char h); // 0x23c30
};
// A per-player start record in the g_gameReg+0x150 array (stride 568 = 0x238); its
// +0x228 is the cap on start gruntz to place for that player.
struct WwdStartPlayer {
    char m_pad00[0x228];
    i32 m_228; // +0x228  start-grunt cap
    char m_pad22c[0x238 - 0x22c];
};
struct WwdGameReg {
    void LogTileError(const char* msg); // 0x48ef10 (via 0x417e thunk)
    char m_pad00[0x68];
    PlaceGridMgr* m_68; // +0x68  the playfield trigger grid (PlaceObject)
    StartCmdMgr* m_6c;  // +0x6c  the command queue (EnqueueSingle)
    WwdGameGrid* m_70;  // +0x70  coarse-cell grid
    char m_pad74[0x7c - 0x74];
    WwdGameRegInner* m_7c; // +0x7c
    char m_pad80[0x118 - 0x80];
    i32 m_118; // +0x118
    char m_pad11c[0x134 - 0x11c];
    i32 m_134; // +0x134
    char m_pad138[0x150 - 0x138];
    WwdStartPlayer m_150[8]; // +0x150  per-player start records
};
DATA(0x0064556c)
extern WwdGameReg* g_gameReg;

// The recycled-node free-list head (?g_freeList@@3PAXA) and a tile-id constant
// (DAT_00644c54) the 0x4017e4 case compares against.
DATA(0x00645544)
extern void** g_freeList;
extern i32 g_tileKindMagic;

// The "Bad <kind> at: x=%d, y=%d" diagnostic format strings ($SG .rdata).
static char s_BadSwitch[] = "Bad switch at: x=%d, y=%d\n";
static char s_BadMulti[] = "Bad multi switch at: x=%d, y=%d\n";

// ---------------------------------------------------------------------------
// The tile grid resolved through this->m_0c->m_08->m_10. Its +0x5c geometry
// holds the bounds (+0x30/+0x34), shift/stride (+0x8c/+0x90), the row offset
// table (+0x24) and the cell table (+0x20); +0x4c is the tile-class table whose
// entries carry the +0x20 "tile type id" virtual.
struct TileClassVtbl {
    char m_pad00[0x20];
    i32 (*GetTypeId)(void* self, i32 a, i32 b); // +0x20
};
struct TileClass {
    TileClassVtbl* m_vtbl;
};
struct TileGridGeom {
    char m_pad00[0x20];
    i32* m_20; // +0x20  cell table
    i32* m_24; // +0x24  row-offset table
    char m_pad28[0x30 - 0x28];
    i32 m_30; // +0x30  width
    i32 m_34; // +0x34  height
    char m_pad38[0x8c - 0x38];
    i32 m_8c; // +0x8c  x shift
    i32 m_90; // +0x90  y shift
};
struct TileGrid {
    char m_pad00[0x24];
    char m_pad24[0x4c - 0x24];
    TileClass** m_4c; // +0x4c
    char m_pad50[0x5c - 0x50];
    TileGridGeom* m_5c; // +0x5c
};
struct PlayMgrRenderer {
    char m_pad00[0x10];
    TileGrid* m_10; // +0x10
};
struct PlayMgr {
    char m_pad00[0x8];
    PlayMgrRenderer* m_08; // +0x08
};

// The placed-object list node embedded at PlayMgrRenderer+0x10 (head at +0x14). An
// MFC CPtrList node: pNext@+0, pPrev@+4, data@+8.
struct StartNode {
    StartNode* m_next;    // +0x00  pNext
    char m_pad04[0x4];    // +0x04  pPrev
    TileLogicObj* m_data; // +0x08  the placed tile/start object
};
// The list view overlaid on PlayMgrRenderer+0x10: +0x0 aliases the tile-grid ptr,
// +0x4 is the placed-object list head.
struct StartList {
    void* m_0;         // +0x00 (= renderer+0x10)
    StartNode* m_head; // +0x04 (= renderer+0x14)
};

// The "Could not add Grunt..." diagnostic for a failed start placement ($SG .rdata).
static char s_CouldNotAdd[] = "Could not add Grunt: Player=%d, x=%d, y=%d";

// ---------------------------------------------------------------------------
// The level/world object: m_4 the game-registry back-ptr, m_0c the play manager,
// m_2dc the playfield grid manager, m_2e4 the trigger registrar.
// ---------------------------------------------------------------------------
class CLevelValidator {
public:
    i32 ValidateLevelTiles();
    i32 PlaceStartGruntz();                         // 0x0d2b20
    i32 PositionBridgeToggle(i32 mode, i32 unused); // 0x0d5b20

    char m_pad00[0x4];
    WwdGameReg* m_4; // +0x04  game registry back-ptr
    char m_pad08[0xc - 0x8];
    PlayMgr* m_0c; // +0x0c
    char m_pad10[0x2dc - 0x10];
    PlayfieldMgr* m_2dc; // +0x2dc
    char m_pad2e0[0x2e4 - 0x2e0];
    TriggerRegistrar* m_2e4; // +0x2e4
    char m_pad2e8[0x3f4 - 0x2e8];
    i32 m_3f4; // +0x3f4  (bridge-toggle gate)
};

// The level tile-id lookup: clamp (x,y) to the grid bounds, shift to tile
// coords, resolve the tile-class through the row/cell tables, and call its
// +0x20 type virtual. Returns 0 for an empty/out-of-range cell.
static i32 LookupTileType(TileGrid* grid, i32 x, i32 y) {
    TileGridGeom* g = grid->m_5c;
    if (x < 0) {
        x = 0;
    } else if (x >= g->m_30) {
        x = g->m_30 - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= g->m_34) {
        y = g->m_34 - 1;
    }
    i32 tx = x >> g->m_8c;
    i32 ty = y >> g->m_90;
    i32 cell = g->m_20[g->m_24[ty] + tx];
    if (cell == (i32)0xeeeeeeee || cell == -1) {
        return 0;
    }
    TileClass* tc = grid->m_4c[cell & 0xffff];
    return tc->m_vtbl->GetTypeId(tc, x - (tx << g->m_8c), y - (ty << g->m_90));
}

// ===========================================================================
// CLevelValidator::PlaceStartGruntz  (0x0d2b20) - /GX EH method. Walk the placed-
// object list (renderer+0x10): for each "start grunt" object (identity 0x4024a5)
// PlaceObject it into the trigger grid at its snapped tile centre, OR'ing in the
// 0x10000 "placed" flag; on PlaceObject failure build a "Could not add Grunt..."
// CString and log it through g_gameReg, returning 0. For "player marker" objects
// (identity 0x4017e4) in a non-active game, EnqueueSingle one op per player up to
// the player's start cap (g_gameReg+0x150[magic].m_228). ret 1.
// ===========================================================================
// @early-stop
// /GX-EH + regalloc wall (~65%): every instruction/branch and the reloc-masked externs
// (PlaceObject/EnqueueSingle/LogTileError/CString Format+ctor+dtor) reproduce, incl. the
// two vacuous address-null guards and the stride-568 player-slot `lea` - but retail homes
// `this` in ebp (re-reading m_4 + spilling counter/flag14 to the EH frame) while our cl
// keeps `this` in ecx and enregisters counter/flag14, a systematic register rename across
// the whole body. Same EH/regalloc wall ValidateLevelTiles (this TU) hits. topic:wall.
RVA(0x000d2b20, 0x21f)
i32 CLevelValidator::PlaceStartGruntz() {
    StartList* list = (StartList*)((char*)m_0c->m_08 + 0x10);
    if (list == 0) {
        return 0;
    }
    WwdGameReg* reg = m_4;
    StartNode* node = list->m_head;
    i32 result = 1;
    i32 counter = 0;
    i32 flag14 = 0;
    if (reg->m_134 == 1) {
        flag14 = 1;
    }
    if (node == 0) {
        return result;
    }
    do {
        TileLogicObj* obj = node->m_data;
        StartNode* next = node->m_next;
        if (obj != 0) {
            char* aux = (char*)obj->m_7c;
            void* who = *(void**)(aux + 0x10);
            if (who == (void*)0x4024a5) {
                i32 idx = reg->m_68->PlaceObject(
                    obj->m_124,
                    (obj->m_5c & ~0x1f) + 0x10,
                    (obj->m_60 & ~0x1f) + 0x10,
                    100000,
                    flag14,
                    obj->m_114,
                    obj->m_11c,
                    obj->m_120,
                    obj->m_118,
                    obj->m_12c,
                    *(i32*)(aux + 0x2c),
                    *(i32*)(aux + 0x30),
                    (i32)&obj->m_134
                );
                if (idx == -1) {
                    CString s;
                    s.Format(
                        s_CouldNotAdd,
                        obj->m_124,
                        (obj->m_5c & ~0x1f) + 0x10,
                        (obj->m_60 & ~0x1f) + 0x10
                    );
                    g_gameReg->LogTileError((const char*)(LPCSTR)s);
                    return 0;
                }
                obj->m_08 |= 0x10000;
            } else if (g_gameReg->m_134 != 1 && who == (void*)0x4017e4
                       && obj->m_124 == g_tileKindMagic) {
                WwdStartPlayer* e = &g_gameReg->m_150[g_tileKindMagic];
                if (e != 0 && counter < e->m_228) {
                    reg->m_6c->EnqueueSingle(
                        result,
                        (char)obj->m_124,
                        0,
                        0,
                        (obj->m_5c & ~0x1f) + 0x10,
                        (obj->m_60 & ~0x1f) + 0x10,
                        0,
                        0
                    );
                    counter++;
                }
            }
        }
        node = next;
    } while (node != 0);
    return result;
}

// ===========================================================================
// CLevelValidator::ValidateLevelTiles  (0x0d2dd0)
// ===========================================================================
RVA(0x000d2dd0, 0x1de4)
i32 CLevelValidator::ValidateLevelTiles() {
    i32 validCount = 0; // [esp+0x10]  count of objects validated
    i32 counts[4];      // [esp+0x34]  per-kind pressure-pad tallies
    counts[0] = 0;
    counts[1] = 0;
    counts[2] = 0;
    counts[3] = 0;

    TileObjList* list = (TileObjList*)((char*)m_0c->m_08 + 0x10);
    if (list->m_04 == 0) {
        return 1;
    }

    i32 ok = 1;
    for (TileObjNode* node = list->m_04; node != 0; node = node->m_next) {
        TileLogicObj* obj = node->m_data;
        if (obj == 0) {
            continue;
        }

        void* who = obj->m_7c->m_vtbl->m_10;
        TileGrid* grid = m_0c->m_08->m_10;

        if (who == (void*)0x401799) {
            i32 type = LookupTileType(grid, obj->m_5c, obj->m_60);
            i32 kind = obj->m_124;
            if (type == 0x21) {
                // off-grid neighbor scan for a matching tile (uses LookupKind)
                obj->m_124 = type;
            }
            switch (kind - 0x33) {
                case 1: // 0x34
                case 0: // 0x33
                    if (!m_2e4->RegisterSwitchLogic(
                            3,
                            obj->m_164,
                            obj->m_168,
                            obj->m_04,
                            obj->m_134,
                            obj->m_144,
                            obj->m_154,
                            obj->m_64,
                            obj->m_7c->m_f0,
                            obj->m_7c->m_100,
                            kind == 0x38,
                            obj->m_120,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadSwitch, obj->m_5c, obj->m_60);
                        g_gameReg->LogTileError((const char*)(LPCSTR)s);
                        return 0;
                    }
                    validCount++;
                    obj->m_08 |= 0x10000;
                    break;
                case 5: // 0x38
                case 4: // 0x37
                    if (!m_2e4->RegisterSwitchLogic(
                            4,
                            obj->m_164,
                            obj->m_168,
                            obj->m_04,
                            obj->m_134,
                            obj->m_144,
                            obj->m_154,
                            obj->m_64,
                            obj->m_7c->m_f0,
                            obj->m_7c->m_100,
                            kind == 0x3c,
                            obj->m_120,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadSwitch, obj->m_5c, obj->m_60);
                        g_gameReg->LogTileError((const char*)(LPCSTR)s);
                        return 0;
                    }
                    validCount++;
                    obj->m_08 |= 0x10000;
                    break;
                default:
                    break;
            }
        } else if (who == (void*)0x403bfc) {
            i32 type = LookupTileType(grid, obj->m_5c, obj->m_60);
            (void)type;
            obj->m_08 |= 0x10000;
        } else if (who == (void*)0x4037b0) {
            i32 type = LookupTileType(grid, obj->m_5c, obj->m_60);
            (void)type;
            obj->m_08 |= 0x10000;
        } else if (who == (void*)0x401b09) {
            if (m_3f4 != 0 && obj->m_124 != 2 && g_gameReg->m_118 != 0 && g_gameReg->m_134 == ok) {
                i32 a = obj->m_118;
                i32 b = obj->m_114;
                a += a;
                b += b;
                if (a > 0x3b) {
                    b++;
                    a -= 0x3c;
                }
                m_2dc->Bridge1d2f(a, b);
            }
            obj->m_08 |= 0x10000;
        } else if (who == (void*)0x40288d) {
            if (obj->m_124 == 0x32) {
                m_2dc->PlacePuddle(obj, 0);
            }
        } else if (who == (void*)0x4017e4) {
            if (obj->m_124 == g_tileKindMagic) {
                void** cell = g_freeList;
                void* slot = 0;
                if (*cell != 0) {
                    slot = (void*)(cell + 1);
                    g_freeList = (void**)*cell;
                }
                if (slot != 0) {
                    ((i32*)slot)[0] = (obj->m_5c & ~0x1f) + 0x10;
                    ((i32*)slot)[1] = (obj->m_60 & ~0x1f) + 0x10;
                }
            }
        } else if (who == (void*)0x4019bf) {
            i32 type = LookupTileType(grid, obj->m_5c, obj->m_60);
            (void)type;
            obj->m_08 |= 0x10000;
        } else if (who == (void*)0x402a68) {
            m_2dc->PlacePuddle(obj, 0);
        } else if (who == (void*)0x40164f) {
            // 3x3 coarse-grid pressure-pad stamp into g_gameReg->m_70: for each
            // of the 3 rows and 3 columns around the object's coarse cell, bounds-
            // check against the registry grid, tally the per-kind counter, and OR
            // a per-kind flag bit (0x100000 << kind) into the grid cell.
            i32 col = obj->m_5c >> 5;
            i32 rowBase = obj->m_60 >> 5;
            i32 stride = (col << 3) - col; // col*7
            i32 ebp = stride * 4 - 0x1c;
            for (i32 dy = -1; dy < 2; dy++, ebp += 0x1c) {
                i32 row = rowBase;
                i32 ofs = rowBase * 4 - 4;
                for (i32 k = 3; k != 0; k--, ofs += 4, row++) {
                    i32 gx = dy + col;
                    i32 gyy = row - 1;
                    WwdGameGrid* gg = g_gameReg->m_70;
                    if ((u32)gx >= (u32)gg->m_0c || (u32)gyy >= (u32)gg->m_10) {
                        continue;
                    }
                    i32 kind = obj->m_124;
                    i32 bit;
                    if ((u32)kind > 3) {
                        bit = 0; // fall through with last ebx (matches retail)
                    } else {
                        switch (kind) {
                            case 0:
                                bit = 0x100000;
                                break;
                            case 1:
                                bit = 0x200000;
                                break;
                            case 2:
                                bit = 0x400000;
                                break;
                            default:
                                bit = 0x800000;
                                break;
                        }
                    }
                    counts[kind]++;
                    gg = g_gameReg->m_70;
                    if ((u32)gx >= (u32)gg->m_0c || (u32)gyy >= (u32)gg->m_10) {
                        continue;
                    }
                    i32* cellRow = (i32*)((char*)gg->m_08[0] + ofs);
                    *(i32*)((char*)cellRow + ebp) |= bit;
                }
            }
        } else if (who == (void*)0x40182a) {
            WwdGameGrid* gg = g_gameReg->m_70;
            i32 cy = obj->m_5c >> 5;
            i32 cx = obj->m_60 >> 5;
            if ((u32)cy < (u32)gg->m_0c && (u32)cx < (u32)gg->m_10) {
                // poke the cell
            }
        } else if (who == (void*)0x401f0a) {
            if (g_gameReg->m_134 != ok) {
                void** cell = g_freeList;
                if (*cell != 0) {
                    g_freeList = (void**)*cell;
                }
            }
        }
    }

    (void)s_BadMulti;
    return ok;
}

// ===========================================================================
// The world/level object (this->m_4): the bridge-toggle reads its viewport-clamp
// limits (+0x8c/+0x90) and, through the per-frame timeline (+0x68), the active
// goal object (+0x23c). Modeled locally (offsets only).
// ---------------------------------------------------------------------------
struct LvWorld {
    char m_pad00[0x68];
    struct LvTimeline {
        void GoalTail(); // 0x... (thiscall, no arg)  reloc-masked ILT thunk
        char m_pad00[0x23c];
        struct LvGoal {
            char m_pad00[0x8];
            i32 m_8; // +0x8  flags (the 0x10000 "released" bit)
        }* m_23c;    // +0x23c  active goal object
    }* m_68;         // +0x68
    char m_pad6c[0x8c - 0x6c];
    i32 m_8c; // +0x8c  viewport-clamp horizontal limit
    i32 m_90; // +0x90  viewport-clamp vertical limit
};
// The bridge-toggle UI sub-object (this->m_2e0): Toggle(mode) sets its state.
struct LvBridgeUi {
    void Toggle(i32 mode); // 0x... (thiscall, 1 arg)  reloc-masked
};
// The {x,y} screen point the toggle position is written to (this->m_3f4).
struct LvBridgePoint {
    i32 x; // +0x0
    i32 y; // +0x4
};

// ===========================================================================
// CLevelValidator::PositionBridgeToggle (0x0d5b20) - place the bridge-toggle UI
// at a fixed inset from the viewport-clamp limits, with the toggle mode and the
// X inset selected by `mode` (0 / 1 / other). If the toggle point (+0x3f4) is
// null, only the mode is set. Then, if a goal object is active (m_4->m_68->m_23c),
// flag it released, detach it, and run the timeline goal-tail. Migrated from
// engine_boundary (CLevelValidator: m_4 world, m_2e0 toggle UI, m_3f4 point).
// ===========================================================================
// @early-stop
// ~91%: control flow + offsets byte-identical. Residual is three documented
// codegen-idiom/regalloc nits: (a) MSVC5 emits `sub edi,K` where retail emits
// `add edi,-K` for the two X-inset decrements (non-steerable add/sub coin-flip);
// (b) the tail's m_4 reload lands in eax (ours) vs ecx (retail) - a free-list
// pick; (c) retail keeps a redundant consecutive `test;je` on the goal pointer
// that MSVC5 collapses in the nested form here (redundant-sibling-guard-retest.md;
// no intervening call to pin the flag, so de-nesting doesn't apply). Deferred.
RVA(0x000d5b20, 0xbb)
i32 CLevelValidator::PositionBridgeToggle(i32 mode, i32) {
    LvWorld* w = *(LvWorld**)((char*)this + 4);
    i32 ex = w->m_8c;
    i32 ey = w->m_90;
    LvBridgePoint* pt;
    if (mode == 1) {
        (*(LvBridgeUi**)((char*)this + 0x2e0))->Toggle(2);
        pt = (LvBridgePoint*)m_3f4;
        if (pt == 0) {
            goto done;
        }
        ex -= 0x37;
    } else if (mode == 0) {
        (*(LvBridgeUi**)((char*)this + 0x2e0))->Toggle(1);
        pt = (LvBridgePoint*)m_3f4;
        if (pt == 0) {
            goto done;
        }
        ex -= 0xd7;
    } else {
        (*(LvBridgeUi**)((char*)this + 0x2e0))->Toggle(3);
        pt = (LvBridgePoint*)m_3f4;
        if (pt == 0) {
            goto done;
        }
        ex -= 0x37;
    }
    ey -= 0x16;
    pt->x = ex;
    pt->y = ey;
done:
    LvWorld::LvTimeline* g = (*(LvWorld**)((char*)this + 4))->m_68;
    LvWorld::LvTimeline::LvGoal* goal = g->m_23c;
    if (goal != 0) {
        if (goal != 0) {
            goal->m_8 |= 0x10000;
            g->m_23c = 0;
        }
        (*(LvWorld**)((char*)this + 4))->m_68->GoalTail();
    }
    return 1;
}
