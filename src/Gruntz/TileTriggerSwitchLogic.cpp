// TileTriggerSwitchLogic.cpp - the tile-trigger logic TU (C:\Proj\Gruntz): the
// WOVEN original obj at retail .text [0x110430 .. 0x1140e2] (TU_MIGRATION
// interval 0x110430, weave 0.30). The four former units tileswitchlogic /
// tiletriggerderivedctors / tilegridcommand / tileactionevent interleave
// function-by-function throughout the interval - impossible across objs at
// first link => ONE original TU (wave2-F merge; + the tiletriggerlogic /
// tiletriggerload singletons and Gate113860, whose RVAs sit inside this
// interval). Classes: CTileTriggerSwitchLogic + the 8 derived *Logic /
// *SwitchLogic leaves, base CTileTriggerLogic, CTileTriggerLogic,
// CTileActionEvent, the CTileTriggerData record and the Gate113860 mode gate.
//
// NOT one TU with the 0x115b60 container interval: the between-space
// [0x1140e2 .. 0x115b60] is solid foreign code (the ToobSpikez TU, Fonts,
// EngStr, SaveScreenshot), so first-link contiguity forces TWO adjacent objs.
// The CTileTriggerSwitchLogic methods at [0x115f00 .. 0x117ec0] are therefore
// defined in TileTriggerContainer.cpp (the devs split the class across the two
// files). Definitions below are in strict ascending retail-RVA order.
//
// Flags: eh (/GX) - the interval carries EH-registration evidence
// (TU_MIGRATION hard error: 0x110430-0x1140e2, 1 EH site).
#include <string.h>     // memcpy -> the /Oi `rep movsd` in BuildSmall
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Mfc.h>
#include <rva.h>

#include <Gruntz/GruntzMgr.h> // the REAL singleton class
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileTriggerContainer.h> // the owner container (m_owner; TtcNode/TtcHead)
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileGridCommand.h>
#include <Gruntz/TileActionEvent.h>
#include <Wwd/WwdFile.h> // CPlaneRender - the canonical plane
#include <Gruntz/SpriteFactory.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/Grunt.h>

// CTileTriggerSwitchLogic is now a REAL polymorphic class (4 virtuals in the
// header): cl emits the ??_7 vftable + the implicit ctor vptr-stamp - the manual
// `*(void**)this = &g_...Vtbl` struct stamp is gone. The target-side vtable name
// ??_7CTileTriggerSwitchLogic@@6B@ is derived AUTOMATICALLY by the build: the EXE
// has no debug symbols, so labels.py applies config/vtable_names.csv (generated
// from RTTI by gruntz.analysis.vtable_scan) whenever the base obj emits the
// ??_7. No source annotation is needed - naming is a byproduct of the inventory.

// (The former `__stdcall TileSwitchCheckType4/7(void*)` free-function externs are gone:
// they are CTileTriggerLogic::Serialize / ::Deserialize, __thiscall on `this` - see
// ValidateByType below.)

// (The EngineLabelBacklog host is DISSOLVED: its two "loaders" are CTriggerMgr
// methods on the +0x68 registry m_cmdGrid - LoadPowerupIconSprites IS FireCommand
// (0x7c620), LoadExplosionSprites (0x7b330) is its sibling. Both declared on the real
// CTriggerMgr (<Gruntz/TriggerMgr.h>), so the calls run cast-free on m_cmdGrid.)

// TileActionEvent.cpp - the per-tile game-action event record (trace placeholder
// tomalla-108). Methods in ascending retail-RVA order. The record shape comes from
// <Gruntz/TileActionEvent.h>; the serializer is the shared CSerialArchive; the game
// registry singleton (g_gameReg) is modeled here with only the offsets these paths
// touch. All engine callees are reloc-masked (no body).
//
// BANKED (code byte-exact, 100% fuzzy): ResetFlag (0x112d80), SetActionCode
//   (0x112da0), MorphByTool (0x113420), Serialize (0x113f10), SerializeFields
//   (0x113f60). The big Process (0x112ee0) is a complete logical reconstruction
//   parked at the two-jump-table wall (@early-stop) for the final sweep.
// <Mfc.h> (not <Win32.h>): UserLogic.h pulls afx via ButeMgr.h/String.h, so the
// umbrella must be the MFC superset kept first (mfc-wall-is-breakable doctrine).
// ---------------------------------------------------------------------------
// The game registry singleton (?g_gameReg@@3PAUWwdGameRegZ@@A at VA 0x64556c).
// Only the offsets this cluster reaches are modeled; reloc-masked DIR32.
// ---------------------------------------------------------------------------

// (The WwdGrViewport / CSpriteMakerSub / WwdGrSprHolder chain views are DISSOLVED:
// g_gameReg->m_world is the canonical CSpriteFactoryHolder (<Gruntz/GameRegistry.h>),
// whose m_8 is the CSpriteFactory, m_24 the CGameLevel and m_28 the CSndHost. The one
// live read - the impact-sound gate - is g_gameReg->m_world->m_28->m_emitGate (CSndHost
// +0x30). The CTileEventSink placeholder was dead - deleted.)

// The 0x64556c singleton IS CGruntzMgr (RTTI-confirmed). Declared at the REAL class: its
// ReportError (@0x8dc60) then emits a DEFINED symbol instead of the unlinkable phantom
// ?ReportError@CGameRegistry@@QAEXHH@Z. The sub-object chain reads through the real
// classes too - CWorldZ::m_24 is the CGameLevel whose +0x5c IS m_mainPlane (the field the
// fake view called `m_5c`; it was there all along, under its real name).
extern "C" CGruntzMgr* g_gameReg;

// The shared global at DAT_00644c54 (VA 0x644c54): used here as the per-player /
// active-slot index into m_playerFlags[]. Already named g_tileKindMagic by the
// leveltilevalidation unit; reuse that name so the DIR32 reloc pairs by symbol.
extern i32 g_tileKindMagic;

// The impact-sound sink param (DAT_0061ab24): Process plays the impact one-shot
// through it (Play(sink, 0,0,0)). Already named g_sndCueTag by the chatbox unit;
// reuse that name so the DIR32 reloc pairs by symbol (no competing DATA - chatbox
// owns the address; Process is deferred so its pairing is non-critical anyway).
extern i32 g_sndCueTag;

// (The CActionGridMgr::RefreshTile view is DISSOLVED: the method the action-set path
// runs after stamping a tile is CMapMgr::ComputeCellFlags(x, y, code) @0x77790 - a
// 3-arg terrain cell-flag recompute on g_gameReg->m_tileGrid (CGruntzMapMgr : CMapMgr),
// NOT a 0-arg "RefreshTile". The RVA neighbour ComputeCellFlags calls confirm it.)

// (The CBrickTile view is DISSOLVED: the tile-object Process acts on is the placed
// grid CELL, and CTmCell IS CGrunt (proven, <Gruntz/TriggerMgr.h>). It reaches beyond
// +0x1f0 (m_1e4/m_1ec), so it can only be the large CGrunt (0x8d8); the touched offsets
// ARE CGrunt members - LoadGruntTypeTable is the inherited CUserLogic method, m_1e4 is
// m_entranceActive, m_1ec is m_tileOwnerHi (the owning player id, indexing m_playerFlags).)

// The spawned brick-break sprite (CreateSprite result) is the shared CGameObject:
// ApplyLookupGeometry (0x1505b0) selects the break animation by name, ApplyName
// (0x150540) caches its first frame, m_flags the retire bit, m_layer the
// cache-state gate. (The former CBreakSprite view also mislaid m_8/m_198 at
// offsets 0/4 - the canonical layout fixes that.)

// The impact-sound lookup by name is the canonical name->cue lookup on the world
// CSndHost (g_gameReg->m_world->m_28): CDDrawSubMgrLeafScan::Lookup_05b7e0(name)
// returns the CObject cache value (a LeafCue), then LeafCue::PlayIfElapsed
// plays it. (The ex CImpactSound view + the fake `Eng_FindSound` free-function extern
// are dissolved onto the real classes - same thiscall thunk 0x2cca as GruntCombat.)

// TileGridCommand.cpp - Gruntz CTileTriggerLogic (C:\Proj\Gruntz).
//
// A tile-grid command object (type tag @ +0x04, coords @ +0x08/+0x0c, container
// back-pointer @ +0x20, game-clock snapshot @ +0x24).  RecordMove captures the
// game clock and hands the command back to its container; Serialize streams the
// command's fields through a CSerialStream; Classify drives the on/off duty cycle
// off the game clock; BumpCell / ApplyMove edit the active tile layer's cell.
//
// The dynamic this-tracer originally lumped these RVAs under
// CTileTriggerSwitchLogic; they are a DIFFERENT shape (verified by the +0x20
// container back-pointer and the +0x04 type tag, not the switch-logic layout).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).

// TileTriggerLoad.cpp - the version-4 deserialize handler (0x1138b0) for a tile
// trigger-data record. Reached through the per-version Load dispatcher (0x513860,
// sub-selector 4) under the outer Serialize switch (0x517636). It pulls a fixed
// run of dword fields off the archive (virtual Read @ vtable byte 0x30) once the
// game registry's m_30 sub-manager is live.
// DE-VIEW (2026-07-13, Fable lane): the `CTileTriggerData` record view is GONE -
// it was CTileTriggerSwitchLogic itself. 0x1138b0 streams the EXACT field list
// LoadState (0x1139a0) reads back - m_08, m_key0c, m_key1, m_linkGate, m_18,
// m_1c, m_20, m_28, then m_block[24] from +0x2c, skipping the +0x24 owner - so
// it is the WRITE mirror, now CTileTriggerSwitchLogic::SaveState (the mode-4 arm
// its own ValidateByType @0x113860 dispatches).

// TileTriggerDerivedCtors.cpp - the 18-byte derived tile-trigger logic ctors
// (C:\Proj\Gruntz), homed from src/Stub/. Each is the canonical logic-ctor
// archetype: chain to the (engine, NOT matched) base ctor - an external no-body
// call so its rel32 reloc-masks - then re-stamp the derived vftable into [this].
//
// Each derived class is now modeled over its REAL RTTI base: the 1-virtual
// CTileTriggerLogic (vtable 0x5eaea4) for the "logic" family and the 4-virtual
// CTileTriggerSwitchLogic (vtable 0x5eae8c) for the "switch" family (both fully
// modeled in their own headers). cl emits the leaf ??_7 (1 slot / 4 slots, matching
// retail) + the implicit ctor vptr-stamp, and the build's RTTI auto-namer names the
// leaf vtable. The base ctor is defined in a DIFFERENT TU (TileTriggerLogic.cpp /
// TileTriggerSwitchLogic.cpp), so its call stays external and the rel32 masks. The
// fabricated per-class `*Base` stand-ins are gone. Definitions are in retail-RVA
// order.

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::CTileTriggerSwitchLogic()
// Constructor: stamps the vtable, zeroes the 24-dword m_block at +0x2c
// (rep stosd), then clears m_20 (+0x20).
// ---------------------------------------------------------------------------
RVA(0x00110430, 0x1c)
CTileTriggerSwitchLogic::CTileTriggerSwitchLogic() {
    // vptr stamp is now IMPLICIT (real polymorphic class) - cl prepends
    // `mov [this], offset ??_7CTileTriggerSwitchLogic@@6B@`, exactly the retail
    // ctor's first instruction, replacing the manual struct stamp.
    for (i32 i = 0; i < 24; i++) {
        m_block[i] = 0;
    }
    m_20 = 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::BuildSmall (slot 1, 0x110460) - the BASE builder that
// CCheckpointTriggerSwitchLogic::BuildSmall (0x112a50) overrides. Gate on the m_20
// one-shot, reject a type-4 build with an empty record, copy the caller's 24-dword
// record into m_block (+0x2c, `rep movsd` ecx=0x18), then chain the slot-0 build
// virtual with the remaining 8 args.
//
// RE-HOMED from src/Stub-era VtblForward.cpp, where it lived as "CVtEmitRecv::Accept" on a
// placeholder receiver. That name occurs nowhere in GRUNTZ.EXE, and the note there said the
// owner was "not recoverable from the xref graph" - true, because it is reached ONLY through
// the vtable, never by a rel32 call. The VTABLE is what recovers it:
// ??_7CTileTriggerSwitchLogic@@6B@ (0x1eae8c) slot 1 == 0x110460, and the checkpoint class's
// vtable overrides that same slot with 0x112a50. The placeholder's shape maps exactly onto
// this class: its "busy gate @+0x20" is m_20, its "0x60-byte record sink @+0x2c" is
// m_block[24], and its "virtual Dispatch slot 0" is Setup - which independently confirms the
// 9-arg slot-1 signature recovered from the checkpoint override.
// ---------------------------------------------------------------------------
RVA(0x00110460, 0x64)
i32 CTileTriggerSwitchLogic::BuildSmall(
    CTileTriggerContainer* owner,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    const i32* rect,
    i32 a7,
    i32 a8,
    i32 a9
) {
    if (m_20 != 0) {
        return 0;
    }
    if (a2 == 4 && rect[0] == 0) {
        return 0;
    }
    memcpy(m_block, rect, sizeof(m_block));
    return Setup(owner, a2, a3, a4, a5, a7, a8, a9);
}

// SwitchDown/SwitchUp stay DECLARED-ONLY on the base (bodies live in unmatched engine TUs); cl still
// emits the ??_7 vftable (the ctor references it) with those slots as external
// refs. Setup (slot 0, thunk 0x1749) IS reconstructed below.

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::Setup (slot 0) - the one-shot Setup virtual (0x1104f0,
// shared as slot 0 across the whole *TriggerSwitchLogic family). If already set up
// (m_20 non-zero) returns 0; else scatters the 8 args into the object fields, sets
// the init guard (m_20=1) + clears m_1c, returns 1. Re-homed from ReconBatch2
// (was the Init8_1104f0 placeholder view; xref: vtable slot +0x0 via thunk 0x1749).
// ---------------------------------------------------------------------------
RVA(0x001104f0, 0x56)
i32 CTileTriggerSwitchLogic::Setup(
    CTileTriggerContainer* owner,
    i32 a1,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7
) {
    if (m_20) {
        return 0;
    }
    m_04 = a1;
    m_08 = a2;
    m_key0c = a3;
    m_key1 = a4;
    m_owner = owner;
    m_18 = a6;
    m_28 = a7;
    m_1c = 0;
    m_linkGate = a5;
    m_20 = 1;
    return 1;
}

// TileTriggerLogic.cpp - Gruntz CTileTriggerLogic (C:\Proj\Gruntz).
// The constructor is matched byte-exact.

// ---------------------------------------------------------------------------
// CTileTriggerLogic::CTileTriggerLogic()
// Zeroes the 24-dword m_block array (rep stosl) then m_1c, reusing the zero
// register for the trailing +0x1c store.
RVA(0x001107f0, 0x1c)
CTileTriggerLogic::CTileTriggerLogic() {
    // m_block initialised before m_1c so the optimiser emits the rep stosl
    // first and reuses the zero register for the +0x1c store afterwards.
    for (i32 i = 0; i < 24; i++) {
        m_block[i] = 0;
    }
    m_1c = 0;
}

// The single slot-0 virtual (0x110c10, reached via ILT thunk 0x402072) is DECLARED
// ONLY: its body lives in an unmatched engine TU, so the ctor-emitted ??_7 vftable
// references it as an external reloc-masked slot (exactly retail's shared slot 0).
// The derived logic classes (TileTriggerDerivedCtors.cpp) inherit this one slot.
// (No virtual destructor: retail's derived vtables share this slot value, proving it
// is a normal inherited virtual, not a per-class ??_G.)

// ---------------------------------------------------------------------------
// CTileTriggerLogic::FindIndexByKey
// Linear scan of the 24-dword m_block; returns 1 on a hit, 0 otherwise.
//
// RE-HOMED from CTileTriggerSwitchLogic (which is 0x8c and cannot hold this): retail is
// `add ecx,0x3c` + 24 iterations = this+0x3c..+0x9b, exactly CTileTriggerLogic::m_block.
// On the old owner it had to be spelled m_block[i + 4] to reach +0x3c - that "+4" fudge WAS
// the misattribution. Same bytes, now with the base index and no fudge.
// ---------------------------------------------------------------------------
// @interleaver CTileTriggerLogic::FindIndexByKey emitted-in <boundary:
// StatusBarUpdaters.cpp LoadSwitchUpSprite @0x1106b0 (before) + BridgeMoveSprites.cpp
// LoadBridgeMove @0x110860 (after)>. A /Gy first-use COMDAT the linker scattered
// between two OTHER units inside this woven interval.
RVA(0x00110820, 0x23)
i32 CTileTriggerLogic::FindIndexByKey(i32 key) {
    for (i32 i = 0; i < 24; i++) {
        if (m_block[i] == key) {
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// CPlayLevelLoad::LoadPyramidBridge (0x0110c10; re-homed from the former
// pyramidbridgesprites unit, waveP - TU_MIGRATION MOVE row `0x110c10
// LoadPyramidBridge@CPlayLevelLoad pyramidbridgesprites -> 0x110430 tileswitchlogic`;
// this RVA is also the CTileTriggerLogic slot-0 vtable target referenced above). The
// pyramid/bridge tile-transition dispatcher; `this` is the PLAY-state level object
// (modeled as a standalone CPlayLevelLoad facet - byte-neutral, LoadPyramidBridge's
// mangling is base-independent and the body is all raw this+offset). g_gameReg is this
// TU's 0x64556c singleton. /GX (two CString sprite-key temps).
// ===========================================================================
// ("TileTriggerTransition" was NOT a global: it is the .rdata STRING LITERAL
//  "TileTriggerTransition" - the CreateSprite lookup key - which a previous pass
//  re-declared as an extern char[] that nothing defines. Written as the literal at
//  its use site now; cl emits the same reloc-masked $SG entry.)

// Engine helpers reached through reloc-masked __thiscall ILT thunks (no body).
i32 PbCellClass(void* cell);                                // cell vtable +0x20 -> class id
void PbShowTransition(void* mapHost, i32 id, i32 x, i32 y); // 0x33f0
void PbStrCtor(void* str);                                  // 0x1b9b93 CString::CString
void PbStrDtor(void* str);                                  // 0x1b9cde CString::~CString
void PbAssignStr(void* str, const char* s);                 // 0x1b9e74 CString::operator=
i32 PbScanLoopA(void* self, i32 a);                         // 0x25b8 (water-bridge inner)

#define PB_I32(p, off) (*(i32*)((char*)(p) + (off)))
#define PB_PTR(p, off) (*(void**)((char*)(p) + (off)))

// Resolve a tile-cell id at (x, y): clamp to the map grid, index the row table into
// the cell array, read the cell id, and when valid dispatch its class id.
static i32 PbResolveCell(void* map, i32 x, i32 y) {
    void* grid = PB_PTR(map, 0x5c);
    if (x < 0) {
        x = 0;
    } else if (x >= PB_I32(grid, 0x28)) {
        x = PB_I32(grid, 0x28) - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= PB_I32(grid, 0x2c)) {
        y = PB_I32(grid, 0x2c) - 1;
    }
    grid = PB_PTR(map, 0x5c);
    i32* rows = (i32*)PB_PTR(grid, 0x24);
    i32* cells = (i32*)PB_PTR(grid, 0x20);
    i32 cell = cells[rows[y] + x];
    if (cell == (i32)0xeeeeeeee || cell == -1) {
        return 0;
    }
    void* obj = (void*)((i32*)PB_PTR(map, 0x4c))[cell & 0xffff];
    return PbCellClass(obj);
}

enum PyramidSpriteType {
    kSpriteTypeBase = 0xf,   // first tile-action sprite type (jump-table base)
    kOrangePyramidUp = 0x62, // case 0x53
    kBlackPyramidUp = 0x64,  // case 0x55
    kGreenPyramidUp = 0x66,  // case 0x57
    kPurplePyramidUp = 0x6a, // case 0x5b
};

class CPlayLevelLoad {
public:
    void LoadPyramidBridge(i32 spriteType); // ?LoadPyramidBridgeSprites@@ placeholder
};

// @early-stop
// /GX nested-jump-table megafunction wall (~7%). The grid-cell resolve, the PtInRect
// transition gate, the two CString sprite-key temps and the pyramid-color jump arms are
// reconstructed and match retail's logic. The full 3647-byte body - the 0x66-case jump
// table, the per-bridge inner grid-scan loops, and the descending /GX exception thread -
// is the documented wall shared by the sibling /GX megafunctions. Deferred to the final
// sweep (docs/patterns/jumptable-data-overlap.md; big-seh-fuzzy-desync.md).
// @interleaver CPlayLevelLoad::LoadPyramidBridge emitted-in <boundary:
// BridgeMoveSprites.cpp LoadBridgeMove @0x110860 (before) + GruntzMgr2.cpp SetCellHeight
// @0x111ec0 (after)>. A foreign-class (CPlayLevelLoad; home Play.cpp, cross-lane skip)
// /Gy COMDAT the linker scattered between two OTHER units - not this TU's body run.
RVA(0x00110c10, 0xe3f)
void CPlayLevelLoad::LoadPyramidBridge(i32 spriteType) {
    void* desc = this;                   // edi
    void* map = PB_PTR(g_gameReg, 0x30); // g_gameReg->m_world
    void* grid = PB_PTR(map, 0x24);      // the level grid
    i32 srcId = 0;                       // [esp+0x1c]
    i32 transId = 0;                     // [esp+0x18] resolved id
    void* upTemp;                        // [esp+0x14] CString GAME_PYRAMIDUP/DOWN
    void* keyTemp;                       // [esp+0x18] CString GAME_<COLOR>PYRAMIDZ

    // ---- resolve the source cell at the descriptor's (x, y) ----
    {
        i32 x = PB_I32(desc, 0x8);
        i32 y = PB_I32(desc, 0xc);
        srcId = PbResolveCell((char*)grid + 0, x, y);
    }

    // ---- the PtInRect trigger gate (rect = grid screen rect at +0x13c) ----
    {
        i32 y = PB_I32(desc, 0xc);
        i32 x = PB_I32(desc, 0x8);
        i32 sy = (y << 5) + 0x10;
        i32 sx = (x << 5) + 0x10;
        POINT pt;
        pt.x = sx;
        pt.y = sy;
        if (!PtInRect((const RECT*)((char*)g_gameReg + 0x13c), pt) || srcId == 0x68
            || srcId == 0x67) {
            transId = 0;
        } else {
            CGameObject* trig = ((CSpriteFactory*)PB_PTR(map, 0x8))
                                    ->CreateSprite(0, sx, sy, 0, "TileTriggerTransition", 0x40003);
            if (trig == 0) {
                goto done;
            }
            trig->m_7c->m_notify(trig);
            transId = (i32)trig->m_7c->m_logic;
        }
    }

    // ---- build the two CString sprite-key temps ----
    PbStrCtor(&upTemp);
    PbStrCtor(&keyTemp);
    transId = PB_I32(&transId, 0);

    switch ((u32)(spriteType - kSpriteTypeBase)) {
        case kGreenPyramidUp - kSpriteTypeBase: // 0x57  GREENPYRAMIDZ
            PbAssignStr(&keyTemp, "GAME_GREENPYRAMIDZ");
            PbAssignStr(
                &upTemp,
                (spriteType == kGreenPyramidUp) ? "GAME_PYRAMIDUP" : "GAME_PYRAMIDDOWN"
            );
            break;
        case kPurplePyramidUp - kSpriteTypeBase: // 0x5b  PURPLEPYRAMIDZ
            PbAssignStr(&keyTemp, "GAME_PURPLEPYRAMIDZ");
            PbAssignStr(
                &upTemp,
                (spriteType == kPurplePyramidUp) ? "GAME_PYRAMIDUP" : "GAME_PYRAMIDDOWN"
            );
            break;
        case kOrangePyramidUp - kSpriteTypeBase: // 0x53  ORANGEPYRAMIDZ
            PbAssignStr(&keyTemp, "GAME_ORANGEPYRAMIDZ");
            PbAssignStr(
                &upTemp,
                (spriteType == kOrangePyramidUp) ? "GAME_PYRAMIDUP" : "GAME_PYRAMIDDOWN"
            );
            break;
        case kBlackPyramidUp - kSpriteTypeBase: // 0x55  BLACKPYRAMIDZ
            PbAssignStr(&keyTemp, "GAME_BLACKPYRAMIDZ");
            PbAssignStr(
                &upTemp,
                (spriteType == kBlackPyramidUp) ? "GAME_PYRAMIDUP" : "GAME_PYRAMIDDOWN"
            );
            break;
        default:
            break;
    }

    PbStrDtor(&keyTemp);
    PbStrDtor(&upTemp);
done:
    return;
}

#undef PB_I32
#undef PB_PTR

// --- CTileTriggerSwitchLogic family (base = 4 virtuals) --------------------
// The five derived switch logics now live in <Gruntz/TileTriggerSwitchLogic.h> (they were
// .cpp-local views; CCheckpointTriggerSwitchLogic has to be shared with CheckpointSwitchBuild.cpp,
// and all five are allocated at 0x8c = sizeof(base), so none adds a data member).
RVA(0x00111f10, 0x12)
CTileMultiTriggerSwitchLogic::CTileMultiTriggerSwitchLogic() {}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::VerifyBlockLinksB
// The FindChild(key, 3) variant of VerifyBlockLinks (byte-identical structure,
// different diagnostic codes 0x44d/0x44e and the slow-lookup kind arg 3 vs 8):
// gate on m_linkGate, find the owner's child that claims this object (FindIndexByKey),
// ack 0x44d on miss; then validate each nonzero block key resolves (FindChild
// (key, 3)) to a gated child, acking 0x44e on a lookup miss.
// ---------------------------------------------------------------------------
// @early-stop
// this-spill frame wall (~86%, same as VerifyBlockLinks 0x112c70): body
// byte-identical; retail reserves a `push ecx` stack local for `this` + reloads it
// to seed the `child` loop cursor, the recompile seeds it from a register. Dead seed
// value, non-steerable frame choice. See docs/patterns/this-spilled-to-local-for-loop-seed.md
// @interleaver CTileTriggerSwitchLogic::VerifyBlockLinksB emitted-in <boundary:
// GruntzMgr2.cpp SetCellHeight @0x111ec0 (before) + GroupOps.cpp Broadcast @0x112080
// (after)>. A /Gy first-use COMDAT the linker scattered between two OTHER units.
RVA(0x00111f40, 0xc4)
i32 CTileTriggerSwitchLogic::VerifyBlockLinksB() {
    if (m_linkGate == 0) {
        return 0;
    }
    // walk the owner CONTAINER's m_list1 (head @ container+0x20) - the 0x9c
    // CTileTriggerLogic children live there (the old "m_owner->m_20 child list"
    // reading was the same load through the switch-logic mis-typing).
    TtcNode* node = TtcHead(m_owner->m_list1);
    i32 found = 0;
    CTileTriggerLogic* child = 0;
    while (node != 0) {
        if (found != 0) {
            break;
        }
        TtcNode* cur = node;
        node = node->m_next;
        child = (CTileTriggerLogic*)cur->m_data;
        if (child != 0 && child->FindIndexByKey(m_key1) != 0) {
            found = 1;
        }
    }
    if (found == 0) {
        g_gameReg->ReportError(TRIGERR_LINK_BROKEN, TRIGSITE_LINKSB_NO_OWNER);
        return 0;
    }
    i32* p = &child->m_block[0]; // child+0x3c (the child is a 0x9c CTileTriggerLogic)
    for (i32 i = 0; i < 24; i++) {
        i32 key = *p;
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindChild(key, TRIGID_MULTI_SWITCH_3);
        if (c == 0) {
            g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_LINKSB_KEY_MISS);
            return 0;
        }
        if (c->m_linkGate == 0) {
            return 0;
        }
        p++;
    }
    return 0;
}

RVA(0x00112050, 0x12)
CTileExclusiveTriggerSwitchLogic::CTileExclusiveTriggerSwitchLogic() {}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::Broadcast (0x112080) - walk this switch's m_block key
// array; each key must resolve (owner->FindChild(key, 4), acking 0x44f on a miss)
// to a sibling switch; for a resolved sibling that is not THIS switch and is
// link-gated, run its slot-3 virtual, then Tick every m_list1 logic child that
// claims it (FindIndexByKey), acking 0x450 if none does.
// RE-HOMED from GroupOps.cpp (the whole `CGroupBroadcast`/`CFindNode`/
// `CBcastMember`/`CBcastListNode` view cluster is dissolved onto the real
// classes: same layout field-for-field, and this RVA sits inside THIS TU's
// interval 0x110430..0x1140e2 - first-link contiguity says it was defined here).
// ---------------------------------------------------------------------------
// @early-stop
// 84% - regalloc wall: the 0-terminated key-array walk, per-key map Find, the
// inner match/destroy list loop and both diagnostic exits are byte-faithful; the
// residual is loop-induction / counter register colouring.  No EH frame.
RVA(0x00112080, 0x138)
i32 CTileTriggerSwitchLogic::Broadcast() {
    // retail: a DIRECT `call 0x2e0f` (the slot-2 body's ILT thunk) - a qualified,
    // devirtualized call, so spell it qualified.
    CTileTriggerSwitchLogic::SwitchDown();
    i32 counter = 0;
    i32* p = &m_block[0];
    i32 i = 0;
    i32 done = 0;
    do {
        if (i >= 0x18) {
            return 1;
        }
        CTileTriggerSwitchLogic* node = m_owner->FindChild(*p, TRIGID_EXCLUSIVE_SWITCH_4);
        if (node == 0) {
            g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_BCAST_KEY_MISS);
            return 0;
        }
        if (node->m_key1 != m_key1 && node->m_linkGate != 0) {
            node->SwitchUp(); // virtual slot 3 (the old view's "Prepare")
            i32 any = 0;
            for (TtcNode* it = TtcHead(m_owner->m_list1); it != 0; it = it->m_next) {
                CTileTriggerLogic* o = (CTileTriggerLogic*)it->m_data;
                if (o != 0 && o->FindIndexByKey(node->m_key1)) {
                    o->Tick(); // slot 0 (the old view's "Destroy")
                    counter++;
                    any = 1;
                }
            }
            if (any == 0) {
                g_gameReg->ReportError(TRIGERR_LINK_BROKEN, TRIGSITE_BCAST_NO_CLAIM);
                return 0;
            }
        }
        i32 next = p[1];
        p++;
        i++;
        if (next == 0) {
            done = 1;
        }
    } while (!done);
    return 1;
}

// --- CTileTriggerLogic family (base = 1 virtual) ---------------------------
// Class shapes now live in <Gruntz/TileTriggerLogic.h> (shared with the AddLogic
// factory in TileTriggerContainer.cpp that news them); the ctor bodies stay here.
RVA(0x00112210, 0x12)
CGiantRockLogic::CGiantRockLogic() {}

RVA(0x00112240, 0x12)
CCoveredPowerupLogic::CCoveredPowerupLogic() {}

RVA(0x00112270, 0x12)
CTileTimeTriggerLogic::CTileTimeTriggerLogic() {}

// ---------------------------------------------------------------------------
// BuildRockBreakInGameText - the rock-break tile-effect loader (RVA 0x1122a0).
//
// OWNER SETTLED (2026-07-13, Fable lane): `this` is the 0xc8 CGiantRockLogic - the
// +0x8/+0xc tile (x, y) are its m_08/m_0c coords, the +0x9c 9-cell value block is
// its m_matrix[9], +0xc0/+0xc4 its m_c0/m_c4 - every touched offset is a
// CGiantRockLogic member, and the receiver TriggerMgr's rock-break driver passes is
// the ScanNeighborhood(tag 0x16 == giant rock) hit. The old CTileTriggerSwitchLogic
// filing was a Ghidra rtti-vptr guess (an 0x8c object cannot even hold +0x9c).
// It: (1) gates on whether the tile center sits inside the view rect;
// (2) walks the 3x3 neighborhood writing each saved cell value back into the level
// plane + notifying the tile grid (and, when in-rect, spawning a Particlez/
// LEVEL_ROCKBREAK sprite per cell); (3) fires the command-grid effect at the tile
// center; (4) when +0xc4 is set spawns an InGameText sprite carrying it; (5) if the
// tile is on-screen with no active override, plays the LEVEL_ROCKBREAK cue (rate-
// limited by the kill-cue clock).
// ---------------------------------------------------------------------------

// Kill-cue clock + sound flags (named so the DIR32 datum reloc-masks).
extern "C" i32 g_killCueClock; // _g_killCueClock @0x6bf3c0
extern i32 g_sndEnabled;       // ?g_sndEnabled@@3HA @0x61ab20
extern i32 g_sndCueTag;        // ?g_sndCueTag@@3HA  @0x61ab24

// The sound-cue registry (g->m_world->m_28) + its Lookup result (the LeafCue cue
// record whose m_14 last-play / m_18 cooldown rate-limit the DSoundCloneInst it plays) are
// the canonical CSndHost/CSndFinder/LeafCue/DSoundCloneInst from <Gruntz/SoundCue.h>
// (included above); the former per-TU RbSoundReg/RbLookupTable/RbCueRec/RbCueSound views
// are dissolved onto them (same offsets + RVAs, xref-confirmed: Lookup 0x1b8438,
// ConfigureItem 0x1360d0).

// `this` stays in esi; the tile coords are re-read from m_08/m_0c at each use
// (retail caches neither, so caching them in locals would spill the frame from
// 0x14 to 0x38) - direct member reads reproduce exactly that.

// @early-stop
// loop-body regalloc wall (~69%): complete + correct reconstruction - the frame
// (0x14, 5 locals), the PtInRect gate, and ALL of steps 3-5 (the m_cmdGrid Fire, the
// InGameText sprite + m_124 stamp, the view-rect bounds cascade, the sound-registry
// Lookup + kill-cue-clock cooldown Play) are byte-exact. The residual is confined to
// the 3x3 loop body: retail spills BOTH counters i (edi) and j (ebx) to the frame and
// reloads them, chaining the plane through a 2nd register so g_gameReg stays live in
// edi for the +0x70 tileGrid read; MSVC5 here keeps i in ebx and re-loads g_gameReg,
// which also duplicates the inner-tail (an extra jmp + a 2nd copy). Source nudges
// (read-order swap, an explicit g_gameReg loop local, px/py caching) all leave the
// i->edi / j->ebx spill-and-reload assignment unmoved - a non-steerable regalloc pick
// inside the hottest block. Deferred to the final sweep.
RVA(0x001122a0, 0x241)
void CGiantRockLogic::BuildRockBreakInGameText() {
    // The world holder: the ex-CWorldZ view IS CSpriteFactoryHolder (one object at +0x30;
    // now dissolved tree-wide). The two fields this fn reads - the sprite factory at +0x08
    // and the sound host at +0x28 - were declared identically on both.
    CSpriteFactoryHolder* gameMgr = g_gameReg->m_world; // cached only for the loop sprite

    // (1) in-rect gate: is the tile center inside the view rect (+0x13c)?
    i32 inRect = 0;
    POINT pt;
    pt.y = (m_0c << 5) + 0x10;
    pt.x = (m_08 << 5) + 0x10;
    if (PtInRect((const RECT*)&g_gameReg->m_viewOriginL, pt)) {
        inRect = 1;
    }

    // (2) 3x3 neighborhood: write each saved cell value into the level plane + notify
    // the tile grid; when in-rect, spawn a Particlez/LEVEL_ROCKBREAK sprite per cell.
    i32* cursor = &m_matrix[0];
    for (i32 j = 0; j <= 2; j++) {
        for (i32 i = 0; i <= 2; i++) {
            i32 value = *cursor;
            i32 px = i + m_08 - 1;
            i32 py = j + m_0c - 1;
            CPlaneRender* plane = (CPlaneRender*)g_gameReg->m_world->m_24->m_mainPlane;
            plane->m_tileGrid[plane->m_colOffsets[py] + px] = value;
            g_gameReg->m_tileGrid->Notify(px, py, value);
            if (inRect) {
                CGameObject* spr = gameMgr->m_8->CreateSprite(
                    0,
                    ((i + m_08) << 5) - 0x10,
                    ((j + m_0c) << 5) - 0x10,
                    0xcf84f,
                    "Particlez",
                    0x40003
                );
                if (spr != 0) {
                    spr->ApplyName("LEVEL_ROCKBREAK");
                    spr->ApplyLookupGeometry("LEVEL_ROCKBREAK", 0);
                }
            }
            cursor++;
        }
    }

    // (3) fire the command-grid effect at the tile center (cx/cy reused by step 4).
    i32 cx = (m_08 << 5) + 0x10;
    i32 cy = (m_0c << 5) + 0x10;
    g_gameReg->m_cmdGrid->FireCommand(m_c0, cx, cy, (i32)m_30, 1, 0);

    // (4) when +0xc4 is set, spawn an InGameText sprite carrying it.
    if (m_c4 != 0) {
        CGameObject* txt =
            g_gameReg->m_world->m_8->CreateSprite(0, cx, cy, 0x17318, "InGameText", 0x40003);
        if (txt == 0) {
            return;
        }
        txt->m_124 = m_c4;
    }

    // (5) on-screen + no active override -> play the LEVEL_ROCKBREAK cue.
    if ((m_08 << 5) + 0x10 >= g_gameReg->m_viewOriginR
        || (m_08 << 5) + 0x10 < g_gameReg->m_viewOriginL
        || (m_0c << 5) + 0x10 >= g_gameReg->m_viewOriginB
        || (m_0c << 5) + 0x10 < g_gameReg->m_viewOriginT) {
        return;
    }
    CSndHost* sreg = gameMgr->m_28; // m_28 typed CSndHost* on the canonical holder (GameRegistry.h)
    if (sreg->m_emitGate != 0) {
        return;
    }
    void* out_ob = 0;
    sreg->m_10.Lookup("LEVEL_ROCKBREAK", out_ob);
    LeafCue* out = (LeafCue*)out_ob;
    if (out == 0) {
        return;
    }
    if (g_sndEnabled == 0) {
        return;
    }
    i32 kc = g_killCueClock;
    if ((u32)(kc - out->m_14) < (u32)out->m_18) {
        return;
    }
    out->m_14 = kc;
    out->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::ApplyMove
// Edits the (m_08,m_0c) tile cell of the active layer: an explicit override m_34
// when set, else by the verb (0x1e->0x5a, 0x1f->0x5b, 0x21->cell+1), marks the
// cell dirty, flags the surrounding screen rect, and (when m_2c is set) posts an
// in-game-text record stamped with m_2c.  Returns 1.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc/addressing wall (~70%): logic + the subtract-chain switch + the
// shared px/py reuse match; the four duplicated grid-access blocks allocate
// registers differently from retail (same scale-4 vs pre-shift split as BumpCell).
RVA(0x00112590, 0x166)
i32 CTileTriggerLogic::ApplyMove(i32 verb) {
    i32 v;
    if (m_34 != 0) {
        CGruntzMgr* reg = g_gameReg;
        CPlaneRender* L = (CPlaneRender*)reg->m_world->m_24->m_mainPlane;
        L->m_tileGrid[L->m_colOffsets[m_0c] + m_08] = m_34;
        v = m_34;
        ((CBrickzGrid*)reg->m_tileGrid)->ComputeCellFlags(m_08, m_0c, v);
    } else {
        switch (verb) {
            case 0x22: {
                CGruntzMgr* reg = g_gameReg;
                CPlaneRender* L = (CPlaneRender*)reg->m_world->m_24->m_mainPlane;
                v = L->m_tileGrid[L->m_colOffsets[m_0c] + m_08] + 1;
                CPlaneRender* L2 = (CPlaneRender*)reg->m_world->m_24->m_mainPlane;
                L2->m_tileGrid[L2->m_colOffsets[m_0c] + m_08] = v;
                ((CBrickzGrid*)reg->m_tileGrid)->ComputeCellFlags(m_08, m_0c, v);
                break;
            }
            case 0x1f: {
                CGruntzMgr* reg = g_gameReg;
                CPlaneRender* L = (CPlaneRender*)reg->m_world->m_24->m_mainPlane;
                L->m_tileGrid[L->m_colOffsets[m_0c] + m_08] = 0x5b;
                ((CBrickzGrid*)reg->m_tileGrid)->ComputeCellFlags(m_08, m_0c, 0x5b);
                break;
            }
            case 0x1e: {
                CGruntzMgr* reg = g_gameReg;
                CPlaneRender* L = (CPlaneRender*)reg->m_world->m_24->m_mainPlane;
                L->m_tileGrid[L->m_colOffsets[m_0c] + m_08] = 0x5a;
                ((CBrickzGrid*)reg->m_tileGrid)->ComputeCellFlags(m_08, m_0c, 0x5a);
                break;
            }
            default:
                break;
        }
    }
    CGruntzMgr* reg = g_gameReg;
    i32 py = (m_0c << 5) + 0x10;
    i32 px = (m_08 << 5) + 0x10;
    reg->m_cmdGrid->FireCommand(m_28, px, py, m_30, 1, 0);
    if (m_2c != 0) {
        CGameObject* rec = reg->m_world->m_8->CreateSprite(0, px, py, 95000, "InGameText", 0x40003);
        if (rec != 0) {
            rec->m_124 = m_2c;
        }
    }
    return 1;
}

RVA(0x00112760, 0x12)
CTileSecretTriggerLogic::CTileSecretTriggerLogic() {}

// --- CTileTriggerSwitchLogic family (base = 4 virtuals), upper RVAs --------
RVA(0x00112790, 0x12)
CTileSecretTriggerSwitchLogic::CTileSecretTriggerSwitchLogic() {}

RVA(0x001127c0, 0x12)
CTileTimeTriggerSwitchLogic::CTileTimeTriggerSwitchLogic() {}

RVA(0x001127f0, 0x12)
CCheckpointTriggerSwitchLogic::CCheckpointTriggerSwitchLogic() {}

// The leaf slot overrides forward to the BASE slot-2/slot-3 virtuals (called
// non-virtually on `this`) and normalize the int result to a bool. thunk 0x2e0f ==
// CTileTriggerSwitchLogic::SwitchDown @0x110570; thunk 0x37e2 == SwitchUp @0x1106b0 (both defined
// in StatusBarUpdaters.cpp, reloc-masked to this TU).

// CTileSecretTriggerSwitchLogic::SwitchDown (slot 2 override, 0x112820) - `return base::SwitchDown() != 0`
// (int->bool neg/sbb/neg normalize).
RVA(0x00112820, 0xc)
i32 CTileSecretTriggerSwitchLogic::SwitchDown() {
    return CTileTriggerSwitchLogic::SwitchDown() != 0;
}

// ---------------------------------------------------------------------------
// CTileTimeTriggerSwitchLogic::SwitchDown (slot 2 override, 0x112840) - `return base::SwitchDown() != 0`
// (the int->bool neg/sbb/neg normalize). Re-homed from ReconBatch2 (was Probe_112840);
// xref: ??_7CTileTimeTriggerSwitchLogic@@6B@+0x8 via thunk 0x2464.
// ---------------------------------------------------------------------------
RVA(0x00112840, 0xc)
i32 CTileTimeTriggerSwitchLogic::SwitchDown() {
    return CTileTriggerSwitchLogic::SwitchDown() != 0;
}

// CTileTimeTriggerSwitchLogic::SwitchUp (slot 3 override, 0x112860) - `return base::SwitchUp() != 0`
// against the base slot-3 virtual.
RVA(0x00112860, 0xc)
i32 CTileTimeTriggerSwitchLogic::SwitchUp() {
    return CTileTriggerSwitchLogic::SwitchUp() != 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::RecordMove
// Captures the running game clock into +0x24, then hands this command to its
// owning container (m_20->MoveList1ToList2(this)).
// ---------------------------------------------------------------------------
RVA(0x00112880, 0x12)
void CTileTriggerLogic::RecordMove() {
    m_24 = g_frameTime;
    m_20->MoveList1ToList2(this);
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::Classify
// Drives the command's on/off duty cycle off the running game clock: while the
// elapsed time is within the lead-in (m_2c) it stays active (+1); past it, the
// remainder modulo the on+off period (m_28+m_30) selects the on or off phase,
// firing the slot-0 tick and latching m_dutyOn on each edge.  Returns +1 (active),
// 0 (just turned on, one-shot of type 0x18) or -1 (just turned off, not 0x17).
// ---------------------------------------------------------------------------
// @early-stop
// entropy-tail (~96%): logic + the single-ret1 convergence match; only the last
// type==0x17 case's ret1 is tail-duplicated instead of merged into the shared tail.
// @interleaver CTileTriggerLogic::Classify emitted-in <boundary: MgrSlotSwap.cpp DoSwap
// @0x1128b0 (before) + CheckpointSwitchBuild.cpp BuildSmall @0x112a50 (after)>. A /Gy
// first-use COMDAT the linker scattered between two OTHER units.
RVA(0x00112970, 0xad)
i32 CTileTriggerLogic::Classify(i32 arg) {
    u32 elapsed = g_frameTime - m_24;
    if (elapsed <= m_2c) {
        goto ret1;
    }
    elapsed -= m_2c;
    {
        u32 period = m_30 + m_28;
        if (elapsed > period) {
            if (m_typeTag == 0x18) {
                Tick();
                return 0;
            }
            if (m_typeTag != 0x17) {
                if (m_dutyOn == 1) {
                    Tick();
                }
                return -1;
            }
        }
        u32 rem = elapsed % period;
        if (rem < m_28) {
            if (m_dutyOn != 0) {
                goto ret1;
            }
            Tick();
            m_dutyOn = 1;
            if (m_typeTag == 0x18) {
                return 0;
            }
            goto ret1;
        }
        if (m_dutyOn != 1) {
            goto ret1;
        }
        Tick();
        m_dutyOn = 0;
        if (m_typeTag == 0x17) {
            goto ret1;
        }
        return -1;
    }
ret1:
    return 1;
}

// ===========================================================================
// The checkpoint switch-logic's slot-2 / slot-3 overrides (the "bump cell" /
// "decrement cell" pair). BOTH were misattributed; the retail VTABLE settles it.
// Reading ??_7CCheckpointTriggerSwitchLogic@@6B@ (0x1eaf54) straight out of the image:
//     slot 0 -> 0x1104f0  Setup        (inherited from CTileTriggerSwitchLogic)
//     slot 1 -> 0x112a50  BuildSmall (override)
//     slot 2 -> 0x112b70  <-- THIS   (override)
//     slot 3 -> 0x112bf0  <-- THIS   (override)
// So `this` is a CCheckpointTriggerSwitchLogic (0x8c, CTileTriggerSwitchLogic base) - NOT
// the 0x9c CTileTriggerLogic, and NOT an orphan "CDecrementCellHost" (a name that does not
// occur anywhere in GRUNTZ.EXE). The +0x08/+0x0c/+0x14 fields they touch exist in BOTH
// class families at the same offsets, which is exactly why the offsets alone never
// disambiguated them and the vtable had to.
//
// The fields are dual-role on this class: +0x08/+0x0c are the tile column/row here, while
// the switch-logic key paths (FindByField0C / FindChild) read +0x0c/+0x10 as lookup keys -
// consistent, since a cell key IS (x << 8) + y. Names stay as the base declares them; only
// the offsets are load-bearing.
// ===========================================================================

// Slot 2: reads the active tile layer's cell at (col,row), stores value+1 back, republishes
// it through the tile grid, and SETS the +0x14 flag.  Returns 1.
// @early-stop
// addressing-mode wall (~73%): logic identical; retail indexes the row table with
// a scale-4 address mode (`[rowtbl+y*4]`), the recompile pre-shifts y (shl 2) and
// uses scale-1, propagating through both cell accesses.
RVA(0x00112b70, 0x5a)
i32 CCheckpointTriggerSwitchLogic::SwitchDown() {
    CGruntzMgr* reg = g_gameReg;
    CPlaneRender* layer = (CPlaneRender*)reg->m_world->m_24->m_mainPlane;
    i32 v = layer->m_tileGrid[m_08 + layer->m_colOffsets[m_key0c]] + 1;
    CPlaneRender* layer2 = (CPlaneRender*)reg->m_world->m_24->m_mainPlane;
    layer2->m_tileGrid[m_08 + layer2->m_colOffsets[m_key0c]] = v;
    ((CBrickzGrid*)reg->m_tileGrid)->ComputeCellFlags(m_08, m_key0c, v);
    m_linkGate = 1;
    return 1;
}

// Slot 3: the decrement sibling - same cell read/write path, value-1, and CLEARS the +0x14
// flag.  Returns 1.
// @early-stop
// strength-reduction wall (~73%): cl materializes row<<2 (shl ecx,2) and reuses
// scale-1 addressing where retail keeps the row in a scale-4 address mode in both cell
// stores; the shift vs scaled-index pick is not steerable.
RVA(0x00112bf0, 0x5e)
i32 CCheckpointTriggerSwitchLogic::SwitchUp() {
    CGruntzMgr* reg = g_gameReg;
    CPlaneRender* layer = (CPlaneRender*)reg->m_world->m_24->m_mainPlane;
    i32 v = layer->m_tileGrid[m_08 + layer->m_colOffsets[m_key0c]] - 1;
    CPlaneRender* layer2 = (CPlaneRender*)reg->m_world->m_24->m_mainPlane;
    layer2->m_tileGrid[m_08 + layer2->m_colOffsets[m_key0c]] = v;
    ((CBrickzGrid*)reg->m_tileGrid)->ComputeCellFlags(m_08, m_key0c, v);
    m_linkGate = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::VerifyBlockLinks
// Linkage validator: if this->m_linkGate is clear, succeed (return 0 short-circuit on
// the null gate).  Otherwise walk the owner's child list (head @ owner->m_20),
// asking each child's FindIndexByKey(this->m_key1) until one claims this object.
// If none does, ack diagnostic 0x452 and fail.  Then, for the claiming child,
// scan its 24-dword key block (child->m_block[4..27]): an empty slot succeeds;
// each nonzero key must resolve via owner->FindChild(key, 8) to a child whose
// m_linkGate is set (else fail, acking 0x453 when the lookup itself misses).
// Returns 1 on the early empty-slot success, 0 otherwise.
// ---------------------------------------------------------------------------
// @early-stop
// this-spill frame wall (~86%): body byte-identical; retail reserves a `push ecx`
// stack local for `this` + reloads it (`mov edi,[esp+0x10]`) to seed the `child`
// loop cursor, the recompile seeds it from a register (`mov edi,ebp`) with no slot.
// Dead seed value, non-steerable frame choice. See
// docs/patterns/this-spilled-to-local-for-loop-seed.md
RVA(0x00112c70, 0xc4)
i32 CTileTriggerSwitchLogic::VerifyBlockLinks() {
    if (m_linkGate == 0) {
        return 0;
    }
    // walk the owner CONTAINER's m_list1 (head @ container+0x20) - the 0x9c
    // CTileTriggerLogic children live there (the old "m_owner->m_20 child list"
    // reading was the same load through the switch-logic mis-typing).
    TtcNode* node = TtcHead(m_owner->m_list1);
    i32 found = 0;
    CTileTriggerLogic* child = 0;
    while (node != 0) {
        if (found != 0) {
            break;
        }
        TtcNode* cur = node;
        node = node->m_next;
        child = (CTileTriggerLogic*)cur->m_data;
        if (child != 0 && child->FindIndexByKey(m_key1) != 0) {
            found = 1;
        }
    }
    if (found == 0) {
        g_gameReg->ReportError(TRIGERR_LINK_BROKEN, TRIGSITE_LINKS_NO_OWNER);
        return 0;
    }
    i32* p = &child->m_block[0]; // child+0x3c (the child is a 0x9c CTileTriggerLogic)
    for (i32 i = 0; i < 24; i++) {
        i32 key = *p;
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindChild(key, TRIGID_CHECKPOINT_SWITCH_8);
        if (c == 0) {
            g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_LINKS_KEY_MISS);
            return 0;
        }
        if (c->m_linkGate == 0) {
            return 0;
        }
        p++;
    }
    return 0;
}

// ??0CTileActionEvent (0x112d80): the constructor - zero the m_10 live flag.
// Was misread as a "ResetFlag" method: its only retail callers are the three
// new-sites in TileTriggerContainer.cpp (AddToList3 / AddToList3Switch /
// Serialize op-7), each with the compiler's guarded-ctor `alloc ? ctor : 0`
// shape, and it returns this in eax exactly as a __thiscall ctor does.
RVA(0x00112d80, 0xa)
CTileActionEvent::CTileActionEvent() {
    m_10 = 0;
}

// ===========================================================================
// CTileActionEvent::SetActionCode  (0x112da0) - __thiscall, ret 4
// ===========================================================================
// Stamp the action code into m_actionCode, fold it to a canonical kind (0x12f/0x130/0x131)
// via the dense byte-mapped remap switch unless this player's slot is already
// active, then write it into the action grid cell (g->m_30->m_24->m_5c flat cell
// = m_20[ m_24[y] + x ]); return 0 if the cell already held the code (no-op), else
// stamp it + run the grid-manager RefreshTile and return 1.
RVA(0x00112da0, 0x9e)
i32 CTileActionEvent::SetActionCode(i32 code) {
    m_actionCode = code;
    if (m_playerFlags[g_tileKindMagic] == 0 && (u32)(code - 0x12f) <= 0x1a) {
        switch (code) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                code = 0x12f;
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
                code = 0x130;
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
                code = 0x131;
                break;
        }
    }
    {
        CPlaneRender* grid = (CPlaneRender*)g_gameReg->m_world->m_24->m_mainPlane;
        i32* cell = &grid->m_tileGrid[grid->m_colOffsets[m_tileY] + m_tileX];
        if (*cell == code) {
            return 0;
        }
        *cell = code;
        g_gameReg->m_tileGrid->ComputeCellFlags(m_tileX, m_tileY, code);
        return 1;
    }
}

// ===========================================================================
// CTileActionEvent::Process  (0x112ee0) - __thiscall, ret 4
// ===========================================================================
// @early-stop
// 918-byte two-jump-table dispatch wall (outer remap switch on m_actionCode + inner
// brick-color switch on the derived effect code, four shl-5/add-0x10 coordinate
// scalings under heavy register pressure ebp=this/ebx=arg/esi=newCode/edi=effect).
// Logic complete + decoded; byte-match deferred to the final sweep.
//
// Outer switch(m_actionCode): derive `effect` (edi, 0=none) and the canonical re-fire code
// `newCode` (esi). First-half: fire the per-effect game action on the brick arg.
// Second-half (always): if the tile is on-screen, spawn the brick-break sprite and
// pick its colored break animation by `effect`. Finally re-fire SetActionCode with
// `newCode` if it changed; return (newCode == 0x12d).
RVA(0x00112ee0, 0x35e)
i32 CTileActionEvent::Process(i32 arg) {
    i32 newCode = m_actionCode;
    i32 effect = 0;
    switch (m_actionCode) {
        case 0x12f:
            newCode = 0x12d;
            break;
        case 0x130:
            newCode = 0x12f;
            break;
        case 0x131:
            newCode = 0x130;
            break;
        case 0x132:
            effect = 0x132;
            newCode = 0x12d;
            break;
        case 0x133:
            newCode = 0x132;
            break;
        case 0x134:
            effect = 0x132;
            newCode = 0x12f;
            break;
        case 0x135:
            newCode = 0x133;
            break;
        case 0x136:
            newCode = 0x134;
            break;
        case 0x137:
            effect = 0x132;
            newCode = 0x130;
            break;
        case 0x138:
            effect = 0x138;
            newCode = 0x12d;
            break;
        case 0x139:
            newCode = 0x138;
            break;
        case 0x13a:
            effect = 0x138;
            newCode = 0x12f;
            break;
        case 0x13b:
            newCode = 0x139;
            break;
        case 0x13c:
            newCode = 0x13a;
            break;
        case 0x13d:
            effect = 0x138;
            newCode = 0x130;
            break;
        case 0x13e:
            effect = 0x13e;
            if (arg != 0) {
                break;
            }
            newCode = 0x12d;
            break;
        case 0x13f:
            newCode = 0x13e;
            break;
        case 0x140:
            effect = 0x13e;
            if (arg != 0) {
                break;
            }
            newCode = 0x12f;
            break;
        case 0x141:
            newCode = 0x13f;
            break;
        case 0x142:
            newCode = 0x140;
            break;
        case 0x143:
            effect = 0x13e;
            if (arg != 0) {
                break;
            }
            newCode = 0x130;
            break;
        case 0x144:
            effect = 0x144;
            newCode = 0x12d;
            break;
        case 0x145:
            newCode = 0x144;
            break;
        case 0x146:
            effect = 0x144;
            newCode = 0x12f;
            break;
        case 0x147:
            newCode = 0x145;
            break;
        case 0x148:
            newCode = 0x146;
            break;
        case 0x149:
            effect = 0x144;
            newCode = 0x130;
            break;
    }

    CGrunt* brick = (CGrunt*)arg;
    if (effect != 0 && brick != 0) {
        if (effect == 0x132) {
            brick->LoadGruntTypeTable(0, 1, 0, 0);
            brick->m_entranceActive = 0;
        } else if (effect == 0x138) {
            g_gameReg->m_cmdGrid
                ->CombatCue((m_tileX << 5) + 0x10, (m_tileY << 5) + 0x10, 1, 2, -1);
        } else if (effect == 0x13e) {
            i32 px = (m_tileX << 5) + 0x10;
            i32 py = (m_tileY << 5) + 0x10;
            if (px < g_gameReg->m_viewOriginR && px >= g_gameReg->m_viewOriginL
                && py < g_gameReg->m_viewOriginB && py >= g_gameReg->m_viewOriginT
                && g_gameReg->m_world->m_28->m_emitGate == 0) {
                LeafCue* snd =
                    (LeafCue*)g_gameReg->m_world->m_28->Lookup_05b7e0("GRUNTZ_NORMALGRUNT_IMPACTMM3");
                if (snd != 0) {
                    snd->PlayIfElapsed((i32)g_sndCueTag, 0, 0, 0);
                }
            }
            if (brick->m_tileOwnerHi == 5) {
                m_playerFlags[0] = 1;
                m_playerFlags[1] = 1;
                m_playerFlags[2] = 1;
                m_playerFlags[3] = 1;
                SetActionCode(m_actionCode);
                return 0;
            }
            m_playerFlags[brick->m_tileOwnerHi] = 1;
            SetActionCode(m_actionCode);
            return 0;
        } else if (effect == 0x144) {
            g_gameReg->m_cmdGrid->LoadExplosionSprites(
                (m_tileX << 5) + 0x10, (m_tileY << 5) + 0x10, -1, 2
            );
        }
    }

    i32 px = (m_tileX << 5) + 0x10;
    i32 py = (m_tileY << 5) + 0x10;
    if (px < g_gameReg->m_viewOriginR && px >= g_gameReg->m_viewOriginL
        && py < g_gameReg->m_viewOriginB && py >= g_gameReg->m_viewOriginT) {
        CGameObject* spr =
            g_gameReg->m_world->m_8->CreateSprite(0, px, py, 0xcf84f, "Particlez", 0x40003);
        if (spr != 0) {
            spr->ApplyLookupGeometry("GAME_BRICKBREAK", 0);
            // Inner dense byte-mapped switch on (effect - 0x132) -> the colored break
            // animation; effect 0x138->RED, 0x13d->BLUE, 0x142->GOLD, the remaining
            // mapped slots->BLACK, anything off-table->default GAME_BRICKBREAK (which
            // also sets the +0x8 uncached flag). The exact slot-0 (effect 0x132) and
            // 0x144 color assignments are the deferred byte-match residual.
            switch (effect) {
                case 0x138:
                    spr->ApplyName("GAME_REDBRICKBREAK");
                    break;
                case 0x13d:
                    spr->ApplyName("GAME_BLUEBRICKBREAK");
                    break;
                case 0x142:
                    spr->ApplyName("GAME_GOLDBRICKBREAK");
                    break;
                default:
                    if (effect >= 0x133 && effect <= 0x144) {
                        spr->ApplyName("GAME_BLACKBRICKBREAK");
                        break;
                    }
                    spr->ApplyName("GAME_BRICKBREAK");
                    if (spr->m_layer == 0) {
                        spr->m_flags |= 0x10000;
                    }
                    break;
            }
        }
    }

    if (newCode != m_actionCode) {
        SetActionCode(newCode);
    }
    return newCode == 0x12d;
}

// ===========================================================================
// CTileActionEvent::MorphByTool  (0x113420) - __thiscall, ret 8
// ===========================================================================
// Apply a tool/key (toolId 0x22..0x26 - the five brick-painting tools) to the
// current action code m_actionCode: an `if/else-if` chain on toolId, each branch a dense
// jump-table switch(m_actionCode) over the 0x12f..0x149 brick-code range that advances m_actionCode
// to the tool's next code (table mappings recovered from the five byte-indexed
// switch tables at 0x513650/0x513694/0x5136d8/0x51371c/0x513760). A switch default
// (no transition for this tool+code) returns 0; any other unmatched toolId falls
// straight through to the shared tail. The tail zeroes the 4-slot per-player flag
// array (m_playerFlags[0..3]), marks this player's slot (or all four when playerSlot==5),
// then re-commits the new code via SetActionCode and returns 1.
//
// Code byte-exact (objdiff fuzzy == base, same as the banked sibling SetActionCode);
// the only residual is the inline .rdata jump-table scoring artifact (the five dense
// switch tables' DIR32 base relocs pair against $L labels vs the self-symbol, and the
// SetActionCode call goes through the ILT thunk) - docs/patterns/jumptable-data-overlap.md.
// No code divergence; the metric undercount is the documented tooling wall.
RVA(0x00113420, 0x1f2)
i32 CTileActionEvent::MorphByTool(i32 toolId, i32 playerSlot) {
    if (toolId == 0x22) {
        switch (m_actionCode) {
            case 0x12f:
                m_actionCode = 0x130;
                break;
            case 0x132:
                m_actionCode = 0x133;
                break;
            case 0x138:
                m_actionCode = 0x139;
                break;
            case 0x13e:
                m_actionCode = 0x13f;
                break;
            case 0x144:
                m_actionCode = 0x145;
                break;
            case 0x130:
                m_actionCode = 0x131;
                break;
            case 0x133:
                m_actionCode = 0x135;
                break;
            case 0x134:
                m_actionCode = 0x136;
                break;
            case 0x139:
                m_actionCode = 0x13b;
                break;
            case 0x13a:
                m_actionCode = 0x13c;
                break;
            case 0x13f:
                m_actionCode = 0x141;
                break;
            case 0x140:
                m_actionCode = 0x142;
                break;
            case 0x145:
                m_actionCode = 0x147;
                break;
            case 0x146:
                m_actionCode = 0x148;
                break;
            default:
                return 0;
        }
    } else if (toolId == 0x23) {
        switch (m_actionCode) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                m_actionCode = 0x134;
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
                m_actionCode = 0x137;
                break;
            default:
                return 0;
        }
    } else if (toolId == 0x24) {
        switch (m_actionCode) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                m_actionCode = 0x13a;
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
                m_actionCode = 0x13d;
                break;
            default:
                return 0;
        }
    } else if (toolId == 0x26) {
        switch (m_actionCode) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                m_actionCode = 0x146;
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
                m_actionCode = 0x149;
                break;
            default:
                return 0;
        }
    } else if (toolId == 0x25) {
        switch (m_actionCode) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                m_actionCode = 0x140;
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
                m_actionCode = 0x143;
                break;
            default:
                return 0;
        }
    }

    m_playerFlags[0] = 0;
    m_playerFlags[1] = 0;
    m_playerFlags[2] = 0;
    m_playerFlags[3] = 0;
    if (playerSlot == 5) {
        m_playerFlags[0] = 1;
        m_playerFlags[1] = 1;
        m_playerFlags[2] = 1;
        m_playerFlags[3] = 1;
    } else {
        m_playerFlags[playerSlot] = 1;
    }
    SetActionCode(m_actionCode);
    return 1;
}

// 0x113860 - CTileTriggerSwitchLogic::ValidateByType: the 0x8c family's save/load
// dispatcher (mode 4 -> SaveState @0x1138b0, 7 -> LoadState @0x1139a0), the exact
// twin of CTileTriggerLogic::ValidateByType below. __thiscall, ret 0x10.
// DE-VIEW (2026-07-13, Fable lane): was the "__stdcall Gate113860(obj,...)" free
// function - a mis-model. Retail's callers (SerializeApplyA 0x117630, LoadElement
// 0x117800) do `mov ecx,<element>` before `call 0x277f`, and the body passes ecx
// UNTOUCHED through to the two __thiscall state helpers (it loads the archive arg
// into eax, never ecx) - i.e. `this` is the element, the first stack arg the
// archive. The old model dropped the receiver and named the archive "obj".
RVA(0x00113860, 0x3b)
i32 CTileTriggerSwitchLogic::ValidateByType(CSerialArchive* s, i32 mode, i32 a3, i32 a4) {
    if (s == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (!SaveState(s)) {
                return 0;
            }
            break;
        case 7:
            if (!LoadState(s)) {
                return 0;
            }
            break;
    }
    return 1;
}

// ===========================================================================
// 0x1138b0 - CTileTriggerSwitchLogic::SaveState: the write mirror of LoadState
// (0x1139a0) - the SAME eight scalar fields (skipping the +0x24 owner) then the
// 24-dword m_block run, through the archive's +0x30 (write) slot. Bails if the
// archive is null or the registry sub-manager (m_30) is not yet live.
// (Was the "CTileTriggerData::LoadV4" view - a second model of this class.)
// ===========================================================================
RVA(0x001138b0, 0xb4)
i32 CTileTriggerSwitchLogic::SaveState(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    ar->Write(&m_08, 4);
    ar->Write(&m_key0c, 4);
    ar->Write(&m_key1, 4);
    ar->Write(&m_linkGate, 4);
    ar->Write(&m_18, 4);
    ar->Write(&m_1c, 4);
    ar->Write(&m_20, 4);
    ar->Write(&m_28, 4);
    i32* p = m_block;
    i32 n = 24;
    do {
        ar->Write(p, 4);
        p++;
    } while (--n);
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::LoadState - 0x1139a0. Network deserialize: gate on a
// non-null stream + the registry active-game flag, then read the eight scalar
// fields (skipping m_owner at +0x24) and the 24-dword m_block via the stream's
// read slot (+0x2c). Returns 1 (or 0 when gated off).
// ---------------------------------------------------------------------------
RVA(0x001139a0, 0xb4)
i32 CTileTriggerSwitchLogic::LoadState(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_08, 4);
    s->Read(&m_key0c, 4);
    s->Read(&m_key1, 4);
    s->Read(&m_linkGate, 4);
    s->Read(&m_18, 4);
    s->Read(&m_1c, 4);
    s->Read(&m_20, 4);
    s->Read(&m_28, 4);
    i32* p = m_block;
    for (i32 i = 0; i < 24; i++) {
        s->Read(p, 4);
        p++;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::ValidateByType - the 0x9c family's save/load dispatcher.
// Returns 0 if the archive is null; type 4 saves (Serialize), type 7 loads
// (Deserialize); any other type passes (returns 1).
//
// RE-HOMED from CTileTriggerSwitchLogic (0x8c): CTileTriggerFactory::Build calls it
// (ILT 0x1abe) at 0x117aa7 on a freshly-`new`ed 0x9c CTileTriggerLogic. Its two callees
// were modeled as `__stdcall TileSwitchCheckType4/7(void* obj)` free functions - but retail
// emits `push eax; call <rel32>` with ECX UNTOUCHED, i.e. they are __thiscall methods run on
// `this` (0x291e -> Serialize @0x113ae0, 0x3102 -> Deserialize @0x113c10). Same bytes; the
// free-function spelling only matched because `this` happened to still be live in ecx.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~93%): switch-case ordering + the two calls match; retail keeps arg1 in eax
// (returns it as the null-zero, push eax for the callees) vs our ecx + explicit xor in the
// null block. Entry-block register only.
RVA(0x00113a90, 0x3b)
i32 CTileTriggerLogic::ValidateByType(void* archive, i32 type, i32 a3, i32 a4) {
    if (archive == 0) {
        return 0;
    }
    switch (type) {
        case 4:
            if (Serialize((CSerialArchive*)archive) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Deserialize((CSerialArchive*)archive) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::Serialize  (the type-4 save ValidateByType dispatches to)
// Returns 0 if the stream is null or the active game-manager (g_gameReg+0x30) is
// null; otherwise transfers the scalar fields then the 24-dword m_block through
// the stream's Write slot and returns 1.
//
// RE-HOMED off the invented `CTileTriggerLogic` (see TileGridCommand.h @identity-TODO):
// ValidateByType reaches it with `this` in ecx, and that `this` is a 0x9c CTileTriggerLogic
// straight off the allocation site. Fields are the same offsets under this class's names
// (m_block -> m_block, m_dutyOn kept).
// ---------------------------------------------------------------------------
RVA(0x00113ae0, 0xe8)
i32 CTileTriggerLogic::Serialize(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_08, 4);
    s->Write(&m_0c, 4);
    s->Write(&m_10, 4);
    s->Write(&m_14, 4);
    s->Write(&m_18, 4);
    s->Write(&m_1c, 4);
    s->Write(&m_28, 4);
    s->Write(&m_2c, 4);
    s->Write(&m_30, 4);
    s->Write(&m_34, 4);
    s->Write(&m_dutyOn, 4);
    s->Write(&m_24, 4);
    i32* p = m_block;
    for (i32 i = 0; i < 24; i++) {
        s->Write(p, 4);
        p++;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::Deserialize  (the type-7 load ValidateByType dispatches to)
// The read counterpart of Serialize: same null/registry guard, same field list,
// but each transfer goes through the stream's read slot (+0x2c) instead of the
// write slot (+0x30). Reads the 12 scalar fields then the 24-dword m_block.
// ---------------------------------------------------------------------------
RVA(0x00113c10, 0xe8)
i32 CTileTriggerLogic::Deserialize(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_08, 4);
    s->Read(&m_0c, 4);
    s->Read(&m_10, 4);
    s->Read(&m_14, 4);
    s->Read(&m_18, 4);
    s->Read(&m_1c, 4);
    s->Read(&m_28, 4);
    s->Read(&m_2c, 4);
    s->Read(&m_30, 4);
    s->Read(&m_34, 4);
    s->Read(&m_dutyOn, 4);
    s->Read(&m_24, 4);
    i32* p = m_block;
    for (i32 i = 0; i < 24; i++) {
        s->Read(p, 4);
        p++;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGiantRockLogic::ApplyByType - the giant rock's save/load dispatcher: run the base
// (0x9c) family's ValidateByType first, then stream this leaf's own +0x9c..+0xc4 tail.
//
// RE-HOMED from CTileTriggerSwitchLogic (0x8c). CTileTriggerFactory::Build proves the owner:
//   117b49: push 0xc8 / call ??2 / mov ecx,eax / call ??0CGiantRockLogic / mov esi,eax
//   117b7b: mov ecx,esi / call 0x1d39    <- THIS function, on the fresh 0xc8 rock
// ---------------------------------------------------------------------------
RVA(0x00113d40, 0x6f)
i32 CGiantRockLogic::ApplyByType(void* archive, i32 type, i32 a3, i32 a4) {
    if (archive == 0) {
        return 0;
    }
    if (ValidateByType(archive, type, a3, a4) == 0) {
        return 0;
    }
    switch (type) {
        case 4:
            if (SerializeMatrix((CSerialArchive*)archive) == 0) {
                return 0;
            }
            break;
        case 7:
            if (DeserializeMatrix((CSerialArchive*)archive) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGiantRockLogic::SerializeMatrix
// Streams two header dwords (+0xc0, +0xc4) then the 3x3 dword matrix (+0x9c..) via
// the stream's Write slot.  Returns 0 if the stream or the active game-manager
// (g_gameReg+0x30) is null, else 1.
//
// RE-HOMED from CTileTriggerSwitchLogic. These +0xc0/+0xc4 writes are what made the old
// owner's array run to m_block[38] and overrun its 0x8c allocation - the contradiction that
// blocked the layout fix. They are CGiantRockLogic's own tail (0x9c base + 0x24 matrix + 8).
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~95%): whole body byte-identical; retail pins this->esi /
// stream->edi (pushes all 4 callee regs, then loads args) vs our this->edi /
// stream->esi (arg load interleaved with the pushes). Reg-pair swap only.
RVA(0x00113dd0, 0x7b)
i32 CGiantRockLogic::SerializeMatrix(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_c0, 4);
    s->Write(&m_c4, 4);
    i32* p = m_matrix;
    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Write(p, 4);
            p++;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGiantRockLogic::DeserializeMatrix (0x113e70) - the READ mirror of SerializeMatrix:
// streams two header dwords (+0xc0, +0xc4) then the 3x3 dword matrix (+0x9c..) via the
// stream's Read slot. Returns 0 if the stream or the active game-manager (g_gameReg+0x30)
// is null, else 1. This is the type-7 (load) apply ApplyByType dispatches to (thunk 0x3cd3).
// @early-stop
// esi/edi regalloc wall (~95%, same as SerializeMatrix): whole body byte-identical;
// retail pins this->esi / stream->edi vs our this->edi / stream->esi. Reg-pair swap.
RVA(0x00113e70, 0x7b)
i32 CGiantRockLogic::DeserializeMatrix(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_c0, 4);
    s->Read(&m_c4, 4);
    i32* p = m_matrix;
    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Read(p, 4);
            p++;
        }
    }
    return 1;
}

// ===========================================================================
// CTileActionEvent::Serialize  (0x113f10) - __thiscall, ret 0x10
// ===========================================================================
// The dispatcher: mode 4 writes the record fields, mode 7 reads them back; any
// other mode (or a null archive arg) is a no-op returning 1. The two field-stream
// helpers share the (edi=this record, esi=archive) shape.
RVA(0x00113f10, 0x3b)
i32 CTileActionEvent::Serialize(void* ar, i32 mode, i32 a3, i32 a4) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (SerializeFields(ar) == 0) {
                return 0;
            }
            break;
        case 7:
            if (DeserializeFields(ar) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// ===========================================================================
// CTileActionEvent::SerializeFields  (0x113f60) - __thiscall, ret 4
// ===========================================================================
// Stream the 9 record fields (m_actionCode,m_tileX,m_tileY,m_c,m_10,m_playerFlags[0..3] - m_14 is
// skipped) through the archive ar's vtable slot +0x30 (write, 4 bytes each).
// Returns 0 if ar or the registry's +0x30 sub-object is null, else 1.
RVA(0x00113f60, 0xa2)
i32 CTileActionEvent::SerializeFields(void* ar) {
    CSerialArchive* a = (CSerialArchive*)ar;
    if (a == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    a->Write(&m_actionCode, 4);
    a->Write(&m_tileX, 4);
    a->Write(&m_tileY, 4);
    a->Write(&m_c, 4);
    a->Write(&m_10, 4);
    a->Write(&m_playerFlags[0], 4);
    a->Write(&m_playerFlags[1], 4);
    a->Write(&m_playerFlags[2], 4);
    a->Write(&m_playerFlags[3], 4);
    return 1;
}

// ===========================================================================
// CTileActionEvent::DeserializeFields  (0x114040) - __thiscall, ret 4
// ===========================================================================
// The mode-7 read counterpart of SerializeFields: read the same 9 record fields
// (m_actionCode,m_tileX,m_tileY,m_c,m_10,m_playerFlags[0..3] - m_14 skipped) back through the archive
// ar's vtable slot +0x2c (read, 4 bytes each). Returns 0 if ar or the registry's
// +0x30 sub-object is null, else 1.
RVA(0x00114040, 0xa2)
i32 CTileActionEvent::DeserializeFields(void* ar) {
    CSerialArchive* a = (CSerialArchive*)ar;
    if (a == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    a->Read(&m_actionCode, 4);
    a->Read(&m_tileX, 4);
    a->Read(&m_tileY, 4);
    a->Read(&m_c, 4);
    a->Read(&m_10, 4);
    a->Read(&m_playerFlags[0], 4);
    a->Read(&m_playerFlags[1], 4);
    a->Read(&m_playerFlags[2], 4);
    a->Read(&m_playerFlags[3], 4);
    return 1;
}
