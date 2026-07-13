// TerrainTileLoader.cpp - the per-tile terrain-action sprite loader
// (C:\Proj\Gruntz). The backlog's second-biggest megafunction (4905 B): given a
// tile coordinate (edi=x, ebp=y) and an action descriptor, it dispatches on the
// tile's terrain-action type and registers / resolves the matching sprite,
// particle, lighting or puddle asset set in the game image registry
// (*g_gameReg). It is a /GX EH-framed routine (a CString diagnostic temp at
// [esp+0x74] gives it the exception frame) with three nested jump-table switches
// over the action-type id.
//
// Identity: the stub had it as EngineLabelBacklog::LoadTerrainTileSprites; the
// $SG string set ("GAME_DIRT"/"LEVEL_DIRT", "GAME_GAUNTLETBRICK1"/
// "LEVEL_GAUNTLETROCK1", "LEVEL_ROCKBREAK", "GAME_HIDDENITEM"/
// "GAME_LIGHTING_HIDDENITEM", "GAME_WATER"/"GAME_WATERSPLASH",
// "GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE2", "Particlez", "LightFx", "ToyPeek", and the
// "No giant rock logic found at: x=" diagnostic) identifies it as the terrain
// tile-action loader. Reconstructed as an EH unit (the CString temp).
//
// Structure: it first resolves the tile cell from the grid (the [esi+0x22c]
// level / [g_gameReg+0x2c] map chain), reads the cell's object id, then runs the
// outer action switch ([esp+0x84]-3, range 0..0xf -> jump table 0x4771bc) whose
// arms (DIRT, GIANTROCK, ROCKBREAK, WATER, HIDDENITEM-lighting, TOY, PUDDLE)
// each look up a GAME_*/LEVEL_* namespace pair in the registry hashtable
// (Lookup @0x1b8438) and register / free / resolve the resolved set. The giant
// case 0x22/0x23 reads the cell occupancy; the lighting cases 0x96..0x99 and the
// toy cases 0x1e/0x1f/0x21 each have their own inner switch.
//
// CARCASS doctrine: the level/map/registry sub-objects are unmatched engine
// classes accessed by raw this+offset; every callee is an external reloc-masked
// __thiscall thunk; the GAME_*/LEVEL_* strings are $SG literals reloc-masked
// against the matched string symbols. PtInRect (via the import thunk at
// 0x6c456c) bounds-checks the action rect. Only offsets / code bytes are
// load-bearing.
//
// @early-stop  (1.1% -> 7.9%: return type corrected to int [retail materialises
// eax=1 before each ret]; reconstructed the always-run prologue - the action
// descriptor, the grid-cell type resolve [level->m_24->m_5c CViewport clamp +
// cells[rowBase[y]+x] + tile-class GetTypeId], the pixel snap - the 0x4771bc
// byte-indexed outer switch mapped to actionTypes {3,5,7,0xd,0xf,0x12}, and the
// DIRT arm [actionType 0xd] with its a5 {-1,2,0x63} sub-dispatch + Particlez
// CreateSprite + tag-0x1a clear.) Residual is the documented /GX nested-jump-table
// megafunction wall: the retail frame is sub esp,0x54 - that size is fixed by the
// UNION of all 6 arms' locals + the descending EH-state scopes, so a partial body
// (this reconstructs 1 of 6 arms) frames at 0xc and shifts every [esp+X] slot;
// the descriptor + the 5 unreconstructed arms (each its own nested cellType jump
// table + CString diagnostic path) are needed to pin the frame and stop the DCE
// of the prologue's descriptor/cell reads. A leaf-first full-body redo is the fix
// (docs/patterns/jumptable-data-overlap.md; big-seh-fuzzy-desync.md;
// eh-state-numbering-base.md; o2-optimizer-bailout-framed.md).

#include <Mfc.h> // PtInRect (via <windows.h>), the CString diagnostic temp
#include <Gruntz/GruntzMgr.h>

#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // CGameObject (the created Particlez sprite)
#include <Gruntz/Viewport.h>      // the shared tile-grid geometry (cell lookup)
#include <Gruntz/TileGridCommand.h> // real CTileTriggerContainer (map+0x2e4) + CTileTriggerLogic (the found set)
#include <rva.h>

// The *0x24556c singleton. Declared here: <Gruntz/TileGridCommand.h>'s header-level decl was
// removed so each TU picks the view/real class it needs (see the note in Play.h). Type unchanged.
extern "C" CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------
// Shared singletons + the recycled-node free sink (named so DIR32 reloc-mask).
// ---------------------------------------------------------------------------
// The game image registry singleton (the object at *0x64556c; reloc-masked DIR32).
// ---------------------------------------------------------------------------

#define PTR(p, off) (*(void**)((char*)(p) + (off)))

// The trigger registrar reached through the level map's (g_gameReg->m_2c) +0x2e4 is the
// real CTileTriggerContainer (FindInLists12 @0x116f20 resolves a set by (key,kind);
// DelFromList1 @0x116e60 releases it); the looked-up set is a CTileTriggerLogic
// (ApplyMove @0x112590 tags it 0x22). All __thiscall, reloc-masked.

// ---------------------------------------------------------------------------
// The level/map owner (`this`). Raw-offset access.
// ---------------------------------------------------------------------------
class CTerrainTileLoader {
public:
    // __thiscall member (retail passes `this` in ecx: `mov esi,ecx` prologue), returns
    // 1 on every handled path (retail materialises eax=1 before each `ret`).
    i32 Load(i32 actionIndex, i32 a1, i32 tileX, i32 tileY, i32 actionType, i32 a5);
    char m_pad[4];
};

// ===========================================================================
// CTerrainTileLoader::Load (0x075e90) - the per-tile terrain-action loader.
// Prologue (always run): resolve the tile cell's type id from the level grid
// (level->m_24->m_5c CViewport: clamp tile coords, cells[rowBase[y]+x], tile-class
// GetTypeId), snap (tileX,tileY) to a pixel centre, then dispatch on
// (actionType - 3) through the 0x4771bc byte-indexed jump table. The DIRT arm
// (actionType 0xd) registers the "Particlez"/LEVEL_DIRT/GAME_DIRT eye-candy set
// when its sub-op (a5) is 2, and clears the tile's tag-0x1a set when a5 is 0x63.
// ===========================================================================
RVA(0x00075e90, 0x1329)
i32 CTerrainTileLoader::Load(
    i32 actionIndex,
    i32 a1,
    i32 tileX,
    i32 tileY,
    i32 actionType,
    i32 a5
) {
    void* self = this;
    (void)actionIndex;
    (void)a1;
    CString diag; // the "No giant rock logic found" temp - forces the /GX EH frame

    void* level = PTR(self, 0x22c);
    void* map = PTR(g_gameReg, 0x2c);
    void* grid = PTR(level, 0x24);
    CViewport* g = *(CViewport**)((char*)grid + 0x5c);

    // clamp the tile coords to the grid (tile-space bounds at +0x28/+0x2c)
    i32 cx = tileX;
    if (tileX < 0) {
        cx = 0;
    } else if (tileX >= g->m_tileWidth) {
        cx = g->m_tileWidth - 1;
    }
    i32 cy;
    if (tileY < 0) {
        cy = 0;
    } else if (tileY >= g->m_tileHeight) {
        cy = g->m_tileHeight - 1;
    } else {
        cy = tileY;
    }

    i32 cellType;
    i32 cell = g->m_cells[g->m_rowBase[cy] + cx];
    if (cell == (i32)0xeeeeeeee || cell == -1) {
        cellType = 0;
    } else {
        void* tc = *(void**)((char*)PTR(grid, 0x4c) + (cell & 0xffff) * 4);
        cellType = (*(i32(**)(void*, i32, i32))(*(void***)tc + 8))(tc, 0, 0);
    }

    i32 px = tileX * 32 + 0x10;
    i32 py = tileY * 32 + 0x10;

    switch (actionType - 3) {
        case 0xa: // actionType 0xd - the DIRT / eye-candy arm
            if (a5 == -1) {
                return 1;
            }
            if (a5 == 2) {
                POINT pt;
                pt.x = px;
                pt.y = py;
                if (PtInRect((const RECT*)((char*)g_gameReg + 0x13c), pt)) {
                    CGameObject* set = ((CSpriteFactory*)PTR(level, 0x8))
                                           ->CreateSprite(0, px, py, 0xcf84f, "Particlez", 0x40003);
                    if (set != 0) {
                        set->ApplyName("LEVEL_DIRT");
                        set->ApplyLookupGeometry("GAME_DIRT", 0);
                    }
                }
                return 1;
            }
            if (a5 != 0x63) {
                return 1;
            }
            // a5 == 0x63: clear the tile's registered tag-0x1a set, keyed by tile coord
            if (cellType == 0x22) {
                CTileTriggerContainer* reg = (CTileTriggerContainer*)PTR(map, 0x2e4);
                void* found = reg->FindInLists12((tileX << 8) + tileY, 0x1a);
                if (found != 0) {
                    ((CTileTriggerLogic*)found)->ApplyMove(0x22);
                    reg->DelFromList1(found);
                }
            }
            return 1;
        default:
            return 1;
    }
}

#undef PTR

// ---------------------------------------------------------------------------
// The window-validate poll (0x094bc0), re-homed from the ApiCaller stubs:
// CTerrainTileLoader::Load (0x075e90) drives it. If the +0x08->+0x08 sub-chain is
// live and its Ready() (0x2441) returns true, validate the +0x04 window (skip if
// null). A placeholder host whose concrete class is not yet recovered; offsets +
// code bytes load-bearing.
struct ValidateChain {
    char m_pad0[8];
    CGruntzMgr* m_8; // +0x08
};
struct ValidateHost {
    char m_pad0[4];
    HWND m_4;           // +0x04
    ValidateChain* m_8; // +0x08
    i32 Validate();
};
SIZE_UNKNOWN(ValidateChain);
SIZE_UNKNOWN(ValidateHost);
RVA(0x00094bc0, 0x31)
i32 ValidateHost::Validate() {
    CGruntzMgr* sub = m_8->m_8;
    if (sub && sub->IsLobbyHostReady()) {
        if (m_4) {
            ValidateRect(m_4, 0);
        }
        return 1;
    }
    return 0;
}

SIZE_UNKNOWN(CTerrainTileLoader);
SIZE_UNKNOWN(TtTrigReg);
SIZE_UNKNOWN(TtSet);
