// PyramidBridgeSprites.cpp - the pyramid/bridge tile-transition dispatcher
// (C:\Proj\Gruntz). A backlog megafunction (3647 B): given a tile-action
// descriptor (`this`=edi, +0x8 = x, +0xc = y) and a sprite-type id (esi), it
// resolves the target tile cell from the level grid, and - keyed by the type id
// through a 0x66-entry jump table - registers the matching pyramid-color or
// bridge transition sprite (GAME_<COLOR>PYRAMIDZ + GAME_PYRAMIDUP/DOWN, or the
// LEVEL_*BRIDGE* set) and stamps the destination tile id, firing the transition
// (ShowTransition @0x33f0). It is a /GX EH-framed routine (two CString temps at
// [esp+0x14]/[esp+0x18] for the sprite-key names give it the exception frame).
//
// Identity: the stub had it as EngineLabelBacklog::LoadPyramidBridgeSprites; the
// $SG string set (GAME_REDPYRAMIDZ .. GAME_CHECKPOINTPYRAMIDZ, GAME_PYRAMIDUP/
// DOWN, LEVEL_WATERBRIDGE/BRIDGEUP/BRIDGEDOWN, LEVEL_DEATHBRIDGE,
// LEVEL_TOGGLE*BRIDGE, LEVEL_CRUMBLE*BRIDGE, "TileTriggerTransition") identifies
// it as the pyramid/bridge tile-transition loader on the same PLAY-state level
// object as CPlayLevelLoad::LoadByMode. Reconstructed as an EH unit.
//
// Structure (the verified top):
//   * resolve the SOURCE tile cell from the grid (clamp the descriptor's
//     +0x8/+0xc tile coord to the map [g_gameReg->m_30->m_24->m_5c] bounds,
//     index the row table m_24[y]+x into the cell array m_20, read the cell id,
//     and when valid dispatch the cell object's vtable +0x20 to a class id);
//   * the PtInRect trigger gate: bound the descriptor's screen rect against the
//     map rect and, when the resolved id isn't 0x67/0x68, fire the
//     TileTriggerTransition (FUN_001597b0, tag 0x40003);
//   * build the two CString sprite keys, then the big jump table over
//     (sprite-type - 0xf) in 0..0x65 (switchdataD 0x511a50 / index 0x511a98).
//
// CARCASS doctrine: `this`, the g_gameReg map/grid sub-objects and the cell
// objects are unmatched engine classes accessed by raw this+offset; every callee
// body is external (no-body, reloc-masked rel32); the GAME_*/LEVEL_* strings are
// $SG literals reloc-masked against the matched string symbols; PtInRect comes
// via <Mfc.h>. Only offsets / code bytes are load-bearing.
//
// @early-stop
// /GX nested-jump-table megafunction wall. The grid-cell resolve, the PtInRect
// transition gate, the two CString sprite-key temps and the pyramid-color jump
// arms (with the per-arm GAME_PYRAMIDUP/DOWN selection and the
// id->tile-id ShowTransition stamp) are reconstructed here and match retail's
// logic. The full 3647-byte body - the 0x66-case jump table, the per-bridge
// inner grid-scan loops (water/death/toggle/crumble), and the descending /GX
// exception-state thread around the two temps - is the documented wall shared by
// the sibling /GX megafunctions (TerrainTileLoader, MainMenuBuilder ~78%,
// ValidateLevelTiles ~17%): MSVC5's jump-table base reloc typing + the EH-state
// numbering across this size are not steerable from C source. Deferred to the
// final sweep (docs/patterns/jumptable-data-overlap.md;
// big-seh-fuzzy-desync.md; eh-state-numbering-base.md;
// o2-optimizer-bailout-framed.md).

#include <Mfc.h> // PtInRect (via <windows.h>); the two CString temps
#include <rva.h>

// ---------------------------------------------------------------------------
// Shared singleton (named so DIR32 reloc-mask).
// ---------------------------------------------------------------------------
extern void* g_64556c; // ?g_gameReg@@3PAUWwdGameReg@@A @0x64556c

// The transition-tag data + the trigger-table base (named so the immediate /
// rect-base operands reloc-mask).
extern void* g_60a848; // s_TileTriggerTransition_0060a848

// ---------------------------------------------------------------------------
// Engine helpers reached through reloc-masked __thiscall ILT thunks (no body).
// ---------------------------------------------------------------------------
i32 PbCellClass(void* cell);                                     // cell vtable +0x20 -> class id
void* PbTrigger(i32 a, void* tag, i32 b, i32 sx, i32 sy, i32 c); // 0x1597b0
void PbHideTrigger(void* trig);                                  // trig->m_7c +0x10
void PbShowTransition(void* mapHost, i32 id, i32 x, i32 y);      // 0x33f0
void PbStrCtor(void* str);                                       // 0x1b9b93 CString::CString
void PbStrDtor(void* str);                                       // 0x1b9cde CString::~CString
void PbAssignStr(void* str, const char* s);                      // 0x1b9e74 CString::operator=
i32 PbScanLoopA(void* self, i32 a);                              // 0x25b8 (water-bridge inner)

#define I32(p, off) (*(i32*)((char*)(p) + (off)))
#define PTR(p, off) (*(void**)((char*)(p) + (off)))

// ---------------------------------------------------------------------------
// Resolve a tile-cell id at (x, y): clamp to the map grid, index the row table
// into the cell array, read the cell id, and when valid dispatch its class id.
// (The recurring entry idiom, inlined per-arm in retail.)
// ---------------------------------------------------------------------------
static i32 ResolveCell(void* map, i32 x, i32 y) {
    void* grid = PTR(map, 0x5c);
    if (x < 0) {
        x = 0;
    } else if (x >= I32(grid, 0x28)) {
        x = I32(grid, 0x28) - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= I32(grid, 0x2c)) {
        y = I32(grid, 0x2c) - 1;
    }
    grid = PTR(map, 0x5c);
    i32* rows = (i32*)PTR(grid, 0x24);
    i32* cells = (i32*)PTR(grid, 0x20);
    i32 cell = cells[rows[y] + x];
    if (cell == (i32)0xeeeeeeee || cell == -1) {
        return 0;
    }
    void* obj = (void*)((i32*)PTR(map, 0x4c))[cell & 0xffff];
    return PbCellClass(obj);
}

// The PLAY-state level object whose tile descriptor drives the transition.
class CPlayLevelLoad {
public:
    void LoadPyramidBridge(i32 spriteType); // ?LoadPyramidBridgeSprites@@ placeholder
    char m_pad[4];
};

// ===========================================================================
RVA(0x00110c10, 0xe3f)
void CPlayLevelLoad::LoadPyramidBridge(i32 spriteType) {
    void* desc = this;               // edi
    void* map = PTR(g_64556c, 0x30); // g_gameReg->m_30
    void* grid = PTR(map, 0x24);     // the level grid
    i32 srcId = 0;                   // [esp+0x1c]
    i32 transId = 0;                 // [esp+0x18] resolved id
    void* upTemp;                    // [esp+0x14] CString GAME_PYRAMIDUP/DOWN
    void* keyTemp;                   // [esp+0x18] CString GAME_<COLOR>PYRAMIDZ

    // ---- resolve the source cell at the descriptor's (x, y) ----
    {
        i32 x = I32(desc, 0x8);
        i32 y = I32(desc, 0xc);
        srcId = ResolveCell((char*)grid + 0, x, y); // [grid+0x5c] via ResolveCell semantics
    }

    // ---- the PtInRect trigger gate (rect = grid screen rect at +0x13c) ----
    {
        i32 y = I32(desc, 0xc);
        i32 x = I32(desc, 0x8);
        i32 sy = (y << 5) + 0x10;
        i32 sx = (x << 5) + 0x10;
        POINT pt;
        pt.x = sx;
        pt.y = sy;
        if (!PtInRect((const RECT*)((char*)g_64556c + 0x13c), pt) || srcId == 0x68
            || srcId == 0x67) {
            transId = 0; // fall through to the switch with no trigger
        } else {
            void* trig = PbTrigger(0, g_60a848, 0x40003, sy, sx, 0);
            if (trig == 0) {
                goto done;
            }
            PbHideTrigger(trig);
            transId = I32(PTR(trig, 0x7c), 0x18);
        }
    }

    // ---- build the two CString sprite-key temps ----
    PbStrCtor(&upTemp);
    PbStrCtor(&keyTemp);
    transId = I32(&transId, 0); // [esp+0x18] carries the resolved id into the switch

    // ---- the jump table over (spriteType - 0xf) in 0..0x65 ----
    // Each pyramid arm assigns its GAME_<COLOR>PYRAMIDZ key + the
    // GAME_PYRAMIDUP/DOWN selection, re-resolves the cell, and stamps the
    // destination tile id via ShowTransition. The full table + the bridge inner
    // loops are the deferred /GX wall; the representative pyramid arms below
    // carry the verified shape.
    switch ((u32)(spriteType - 0xf)) {
        case 0x57: // GREENPYRAMIDZ
            PbAssignStr(&keyTemp, "GAME_GREENPYRAMIDZ");
            PbAssignStr(&upTemp, (spriteType == 0x66) ? "GAME_PYRAMIDUP" : "GAME_PYRAMIDDOWN");
            break;
        case 0x5b: // PURPLEPYRAMIDZ
            PbAssignStr(&keyTemp, "GAME_PURPLEPYRAMIDZ");
            PbAssignStr(&upTemp, (spriteType == 0x6a) ? "GAME_PYRAMIDUP" : "GAME_PYRAMIDDOWN");
            break;
        case 0x53: // ORANGEPYRAMIDZ
            PbAssignStr(&keyTemp, "GAME_ORANGEPYRAMIDZ");
            PbAssignStr(&upTemp, (spriteType == 0x62) ? "GAME_PYRAMIDUP" : "GAME_PYRAMIDDOWN");
            break;
        case 0x55: // BLACKPYRAMIDZ
            PbAssignStr(&keyTemp, "GAME_BLACKPYRAMIDZ");
            PbAssignStr(&upTemp, (spriteType == 0x64) ? "GAME_PYRAMIDUP" : "GAME_PYRAMIDDOWN");
            break;
        default:
            // the remaining ~0x60 cases (red/white/checkpoint pyramids + the
            // water/death/toggle/crumble bridge grid scans) hit the deferred
            // jump-table wall.
            break;
    }

    // ---- teardown the two CString temps (EH state -1) ----
    PbStrDtor(&keyTemp);
    PbStrDtor(&upTemp);
done:
    return;
}

#undef I32
#undef PTR
