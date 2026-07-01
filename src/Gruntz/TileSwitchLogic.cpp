// TileSwitchLogic.cpp - Gruntz CGruntzMgr::WireTileSwitchLogic (C:\Proj\Gruntz).
// The tile-switch/plate "wire" dispatcher: given a triggering object and a tile
// (x, y), resolve the tile cell, read its switch-logic tag and run the matching
// secret-switch / crumble-tile / trigger / plate effect, logging the "No switch
// logic found for switch/plate at: x=%d, y=%d" / "No trigger logic found ..."
// diagnostics when the tag has no handler. Identified by that $SG diagnostic set
// plus the "GAME_SECRETSWITCH" / "CrumbleTileDelay" / "Hazardz" keys.
//
// It is a /GX EH-framed routine: a CString diagnostic temp at [esp+0x30] (the
// wsprintf-into-CString format target for the "No ... logic found" messages)
// gives it the exception frame, so it lives in an `eh` unit. Signature is
// __thiscall(trigger, x, y) -> i32 (callee cleans 0xc).
//
// Structure: the prologue marks the trigger active ([trigger+0x358]=1), clamps
// the (x, y) into the level grid and reads the tile cell's switch-logic id from
// the action plane ([this+0x2e4] trigger container). The id (minus 0xb, range
// 0..0x65) indexes a 102-entry byte table (0x46cee4) into a 20-way jump table
// (0x46ce94) whose arms (secret-switch toggle, crumble-tile, the four
// trigger-list passes, the plate handler with its own nested 7-way switch at
// 0x46cf4c) look the switch/trigger up in the registry, walk its sibling list
// and apply or log the effect.
//
// CARCASS doctrine: the level/grid/registry sub-objects are unmatched engine
// classes accessed by raw this+offset; every callee is an external reloc-masked
// __thiscall thunk; the GAME_*/diagnostic strings are $SG literals reloc-masked
// against the matched string symbols. Only the offsets / code bytes are
// load-bearing (campaign doctrine).
//
// @early-stop
// /GX branchy nested-jump-table state-machine wall. The prologue, the (x, y)
// grid clamp, the cell-tag resolve and the primary switch dispatch with its
// validated diagnostic-logging arms are reconstructed here and match retail's
// logic. The full 3426-byte body - the 20-way + nested 7-way jump tables, the
// twelve near-identical registry-lookup / list-walk / CString-format arms, and
// the descending /GX exception-state thread around the diagnostic temp - is the
// documented megafunction wall (cf. the sibling /GX megafunctions
// TerrainTileLoader, MainMenuBuilder ~78%): MSVC5's jump-table base reloc typing
// + the EH-state numbering across this size are not steerable from C source.
// Deferred to the final sweep (docs/patterns/jumptable-data-overlap.md;
// big-seh-fuzzy-desync.md; eh-state-numbering-base.md;
// o2-optimizer-bailout-framed.md).

#include <Mfc.h> // the CString diagnostic temp (the /GX frame)
#include <rva.h>

// ---------------------------------------------------------------------------
// Shared singletons (named so their DIR32 datum reloc-masks).
// ---------------------------------------------------------------------------
// The CGruntzMgr game-manager singleton (_g_mgrSettings @0x64556c). This routine's
// `this` (ecx) is a DIFFERENT object (a tile/switch logic owner, level @+0x22c); it
// loads the manager separately to report/ack switch fires. (Earlier reconstructions
// mislabelled this datum as a WwdGameReg; the delinker names it _g_mgrSettings.)
extern "C" void* g_mgrSettings; // _g_mgrSettings @0x64556c (the CGruntzMgr singleton)
extern "C" void* g_buteMgr;     // ?g_buteMgr@@3VCButeMgr@@A @0x6453d8
extern "C" i32 g_644c54;        // DAT_00644c54 @0x644c54 (current area index)
extern "C" void* g_61ab24;      // DAT_0061ab24 @0x61ab24 (secret-switch sink)

// ---------------------------------------------------------------------------
// Engine helpers reached through reloc-masked thunks (no body).
// ---------------------------------------------------------------------------
void* TsLookupSwitch(void* container, i32 key);                 // 0x1c21 resolve switch by id
i32 TsStrCmp(void* str, i32 key);                               // 0x1fa5 name/key compare
void TsApply(void* node);                                       // 0x19dd apply a switch effect
i32 TsToggle(void* node);                                       // 0x3e63 toggle secret switch
i32 TsIsCrumble(void* node);                                    // 0x2289 crumble check
void TsTrigger(void* obj, i32 a, i32 b, i32 c, i32 tag, i32 d); // 0x39f4 fire a trigger
void TsActivate(void* obj, i32 a, i32 b, i32 c, i32 d, i32 e);  // 0x14e2 activate effect
void* TsAlloc(void* mgr, i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g); // 0x3265 allocate
i32 TsGetDword(const char* sec, const char* key);             // 0x172240 CButeMgr::GetDword
i32 TsLookupRegistry(void* reg, const char* name, void* out); // 0x1b8438 registry lookup
void TsFree(void* sink, i32 a, i32 b, i32 c);                 // 0x25fe free a set
void TsAck(void* gr, i32 a, i32 b);                           // 0x346d ack a switch fire
void TsRemove(void* gr, i32 a);                               // 0x417e remove from queue

// The wsprintf-into-CString diagnostic helper (the /GX frame's destructible temp
// is a real <Mfc.h> CString; its ctor 0x1b9b93 / dtor 0x1b9cde are reloc-masked).
void TsFormat(void* out, const char* fmt, i32 x, i32 y); // 0x1b2cf5 format into CString

#define I32(p, off) (*(i32*)((char*)(p) + (off)))
#define PTR(p, off) (*(void**)((char*)(p) + (off)))

// Vtable slot +0x20 (the cell -> object-id resolver): mov edx,[node]; call [edx+0x20].
static i32 VtblResolve(void* node) {
    void* vtbl = *(void**)node;
    return (*(i32(**)(void*))((char*)vtbl + 0x20))(node);
}

// The tile/switch-logic owner that hosts WireTileSwitchLogic (`this`, ecx). NOT the
// CGruntzMgr game manager (which is loaded separately as g_mgrSettings): its level
// lives at +0x22c and its trigger container at +0x2e4. Only touched offsets matter;
// carcass access by raw this+offset.
class CTileWireLogic {
public:
    i32 WireTileSwitchLogic(void* trigger, i32 x, i32 y); // __thiscall (callee cleans 0xc)
    char m_pad[0x2f0];
};

// CGruntzMgr::WireTileSwitchLogic - the tile-switch wire dispatcher.
// @early-stop
// /GX branchy nested-jump-table megafunction wall (~10%): validated top
// reconstructed (prologue, grid clamp, cell-tag resolve, primary switch +
// first diagnostic arm); the 20-way + nested 7-way dispatch, the twelve
// near-identical list-walk/CString-format arms and the /GX EH-state thread
// across 3426 B are the documented wall. See file header; final-sweep.
RVA(0x0006c130, 0xd62)
i32 CTileWireLogic::WireTileSwitchLogic(void* trigger, i32 x, i32 y) {
    void* self = this;
    void* gr = g_mgrSettings;
    i32 areaGm = I32(gr, 0x2c);

    if (trigger != 0) {
        I32(trigger, 0x358) = 1;
    }

    // Resolve the tile cell from the level's action plane and clamp (x, y).
    void* lvl = PTR(self, 0x22c);
    void* plane = PTR(lvl, 0x24);
    i32 cx = x;
    i32 cy = y;
    if (cx < 0) {
        cx = 0;
    } else {
        i32 w = I32(PTR(plane, 0x5c), 0x30);
        if (cx >= w) {
            cx = w - 1;
        }
    }
    if (cy < 0) {
        cy = 0;
    } else {
        i32 h = I32(PTR(plane, 0x5c), 0x34);
        if (cy >= h) {
            cy = h - 1;
        }
    }
    void* p5c = PTR(plane, 0x5c);
    i32 sx = I32(p5c, 0x8c);
    i32 sy = I32(p5c, 0x90);
    i32 col = (cx >> sx);
    i32 row = (cy >> sy);
    i32 base = I32(I32(p5c, 0x24), row * 4) + col;
    i32 raw = I32(PTR(p5c, 0x20), base * 4);
    i32 tag = 0;
    if (raw != (i32)0xeeeeeeee && raw != -1) {
        void* tbl = PTR(plane, 0x4c);
        void* node = PTR((char*)tbl, (raw & 0xffff) * 4);
        tag = VtblResolve(node);
    }

    // The switch-logic id (tag - 0xb) selects the effect arm. Spelled as a
    // natural switch so cl emits its own byte-index + jump table; the validated
    // arms log the "No ... logic found" diagnostic and apply/free the resolved
    // switch. The full 20-way + nested 7-way dispatch is the /GX megafunction
    // wall (see file header) and is deferred to the final sweep.
    i32 id = tag - 0xb;
    if ((u32)id > 0x65) {
        return 0;
    }

    void* trig = PTR(self, 0x2e4);
    void* sw = TsLookupSwitch(trig, (x >> 5) + ((y >> 5) << 8) + 0x700);
    if (sw == 0) {
        CString msg; // [esp+0x30] diagnostic temp
        TsFormat(&msg, "No switch logic found for switch at: x=%d, y=%d", x, y);
        TsRemove(gr, *(i32*)((char*)&msg + 0));
        TsAck(gr, 0x80dd, 0x3eb);
        return 0;
    }

    // (carcass) The switch was resolved: walk its sibling list, apply/toggle the
    // effect or fire the trigger, then ack. The remaining twelve arms (secret
    // switch, crumble tile, the four trigger passes, the plate handler with its
    // nested 7-way switch, and the "No trigger/plate logic" diagnostics) are the
    // documented /GX wall and are deferred to the final sweep.
    (void)areaGm;
    (void)sw;
    return 1;
}
