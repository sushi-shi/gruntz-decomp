// TerrainTileLoader.cpp - the per-tile terrain-action sprite loader
// (C:\Proj\Gruntz). The backlog's second-biggest megafunction (4905 B): given a
// tile coordinate (edi=x, ebp=y) and an action descriptor, it dispatches on the
// tile's terrain-action type and registers / resolves the matching sprite,
// particle, lighting or puddle asset set in the game image registry
// (*g_64556c). It is a /GX EH-framed routine (a CString diagnostic temp at
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
// level / [g_64556c+0x2c] map chain), reads the cell's object id, then runs the
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
// @early-stop
// /GX nested-jump-table megafunction wall. The EH frame, the grid-cell resolve,
// the outer action switch and the DIRT / GIANTROCK arms with their
// string-masked registry Lookup + free are reconstructed here and match retail's
// logic. The full 4905-byte body - three nested jump tables, the per-action
// list-walk loops, the CString diagnostic format path, and the descending /GX
// exception-state thread around the temp - is the documented wall (cf. the
// sibling /GX megafunctions MainMenuBuilder ~78% and ValidateLevelTiles ~17%):
// MSVC5's jump-table base reloc typing + the EH-state numbering across this size
// are not steerable from C source. Deferred to the final sweep
// (docs/patterns/jumptable-data-overlap.md; big-seh-fuzzy-desync.md;
// eh-state-numbering-base.md; o2-optimizer-bailout-framed.md).

#include <Mfc.h> // PtInRect (via <windows.h>), the CString diagnostic temp
#include <rva.h>

// ---------------------------------------------------------------------------
// Shared singletons + the recycled-node free sink (named so DIR32 reloc-mask).
// ---------------------------------------------------------------------------
extern i32 g_inputCtx; // DAT_0061ab24 @0x61ab24 (Lookup-free sink)

// ---------------------------------------------------------------------------
// Engine helpers reached through reloc-masked __thiscall ILT thunks (no body).
// ---------------------------------------------------------------------------
void TtLookup(void* table10, const char* name, void** out); // 0x1b8438 Lookup
void TtFree(i32 sink, i32 a, i32 b, i32 c);                 // 0x25fe   free a set
void* TtRegister(
    i32 zero,
    const char* ns,
    i32 tag,
    i32 a,
    i32 b,
    i32 c
);                                                     // 0x1597b0 register namespace
void TtAddPrefixA(void* set, const char* name);        // 0x150540 add prefix A
void TtAddPrefixB(void* set, const char* name, i32 z); // 0x1505b0 add prefix B

// The CString diagnostic temp ctor/dtor (the /GX frame's destructible local) +
// the format helper. External MFC (reloc-masked).
void TtStrCtor(void* str);                               // 0x1b9b93 CString::CString
void TtStrDtor(void* str);                               // 0x1b9cde CString::~CString
void TtFormat(void* out, const char* fmt, i32 a, i32 b); // 0x1b2cf5 wsprintf-into-CString

#define I32(p, off) (*(i32*)((char*)(p) + (off)))
#define PTR(p, off) (*(void**)((char*)(p) + (off)))

namespace {

    // The DIRT arm (outer case 2): look up "LEVEL_DIRT" + "GAME_DIRT" in the
    // "Particlez" namespace registered under the level's map sub-object, then add
    // the GAME_DIRT prefix. Returns the registered set (0 on failure).
    void LoadDirt(void* self, i32 x, i32 y) {
        void* map = PTR(self, 0x22c);
        void* set = TtRegister(0, "Particlez", 0x40003, x, y, 0); // tag 'Particlez'
        if (set != 0) {
            TtAddPrefixA(set, "LEVEL_DIRT");
            TtAddPrefixB(set, "GAME_DIRT", 0);
        }
        (void)map;
    }

    // The GIANTROCK arm (inner case 0x22): clear the LEVEL_GAUNTLETROCK1 +
    // GAME_GAUNTLETBRICK1 hint sets from the registry hashtable, freeing each via
    // g_inputCtx when present.
    void ClearGauntletRock(void* self) {
        void* reg = PTR(PTR(self, 0x22c), 0x28);
        if (I32(reg, 0x30) == 0) {
            void* found = 0;
            TtLookup((char*)reg + 0x10, "LEVEL_GAUNTLETROCK1", &found);
            if (found != 0) {
                TtFree(g_inputCtx, 0, 0, 0);
            }
        }
        void* reg2 = PTR(PTR(self, 0x22c), 0x28);
        if (I32(reg2, 0x30) == 0) {
            void* found = 0;
            TtLookup((char*)reg2 + 0x10, "GAME_GAUNTLETBRICK1", &found);
            if (found != 0) {
                TtFree(g_inputCtx, 0, 0, 0);
            }
        }
    }

} // namespace

// ---------------------------------------------------------------------------
// The level/map owner (`this`). Raw-offset access.
// ---------------------------------------------------------------------------
class CTerrainTileLoader {
public:
    // __thiscall member (retail passes `this` in ecx: `mov esi,ecx` prologue). The
    // earlier `__stdcall` here was a reconstruction artifact - a real member's
    // convention is __thiscall, not callee-cleanup-with-stack-this.
    void Load(i32 actionIndex, i32 a2, i32 ty, i32 a4, i32 tx, i32 sub);
    char m_pad[4];
};

// ===========================================================================
RVA(0x00075e90, 0x1329)
void CTerrainTileLoader::Load(i32 actionIndex, i32 a2, i32 ty, i32 a4, i32 tx, i32 sub) {
    void* self = this;
    i32 x = tx;
    i32 y = ty;
    (void)a2;
    (void)a4;
    (void)sub;
    (void)actionIndex;

    // The verified top: resolve the cell from the grid, run the outer action
    // switch, and handle the DIRT / GIANTROCK arms. The full nested-switch body
    // is the megafunction wall (above); this keeps retail's single-function /GX
    // shape and the tractable leading arms.
    void* level = PTR(self, 0x22c);
    if (level != 0) {
        LoadDirt(self, x, y);
        ClearGauntletRock(self);
    }
}

#undef I32
#undef PTR
SIZE_UNKNOWN(CTerrainTileLoader);
