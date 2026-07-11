// TileTriggerSwitchLogic.cpp - the tile-trigger logic TU (C:\Proj\Gruntz): the
// WOVEN original obj at retail .text [0x110430 .. 0x1140e2] (TU_MIGRATION
// interval 0x110430, weave 0.30). The four former units tileswitchlogic /
// tiletriggerderivedctors / tilegridcommand / tileactionevent interleave
// function-by-function throughout the interval - impossible across objs at
// first link => ONE original TU (wave2-F merge; + the tiletriggerlogic /
// tiletriggerload singletons and Gate113860, whose RVAs sit inside this
// interval). Classes: CTileTriggerSwitchLogic + the 8 derived *Logic /
// *SwitchLogic leaves, base CTileTriggerLogic, CTileGridCommand,
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
#include <Mfc.h>
#include <rva.h>

#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileGridCommand.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/Viewport.h>
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

// Per-type validators used by ValidateByType (reloc-masked rel32 callees; both
// callee-cleanup taking the object pointer).
i32 __stdcall TileSwitchCheckType4(void* obj);
i32 __stdcall TileSwitchCheckType7(void* obj);

// The engine-label backlog host reached through the command grid: the
// powerup-icon and explosion sprite loaders (the two donor TUs' local hosts,
// merged - both dispatch reloc-masked, declared-only).
class EngineLabelBacklog {
public:
    i32 LoadPowerupIconSprites(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);
    i32 LoadExplosionSprites(i32 a, i32 b, i32 c, i32 d);
};

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

// The action-occupancy tile grid reached as g->m_30->m_24->m_5c is the shared
// CViewport (<Gruntz/Viewport.h>): cell = m_cells[m_rowBase[y] + x].
struct WwdGrViewport {
    char m_pad0[0x5c];
    CViewport* m_5c; // +0x5c
};
SIZE_UNKNOWN(WwdGrViewport);

// The sprite factory the brick-break path spawns through (g->m_30->m_8) is the
// canonical CSpriteFactory (<Gruntz/SpriteFactory.h>). Process also reads
// g->m_30->m_28->m_30 (a one-shot impact-sound gate). External slots.
struct CSpriteMakerSub {
    char m_pad0[0x30];
    i32 m_30; // +0x30  impact-sound already-played gate
};
SIZE_UNKNOWN(CSpriteMakerSub);
struct WwdGrSprHolder {
    char m_pad0[0x8];
    CSpriteFactory* m_8; // +0x08  sprite factory (CreateSprite @0x1597b0)
    char m_pad0c[0x24 - 0xc];
    WwdGrViewport* m_24;   // +0x24  the SetActionCode grid viewport
    CSpriteMakerSub* m_28; // +0x28  impact-gate holder
};
SIZE_UNKNOWN(WwdGrSprHolder);

// The message/effect sink (g->m_68): PostTileEvent posts coordinate-tagged events
// to the game; external/no-body so the dispatch reloc-masks.
struct CTileEventSink {
    // PostA @0x400c IS CGruntTileMgr::CombatCue; cast at the call.
    // PostB @0x2fb3 IS EngineLabelBacklog::LoadExplosionSprites; cast at the call.
};
SIZE_UNKNOWN(CTileEventSink);

DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// The shared global at DAT_00644c54 (VA 0x644c54): used here as the per-player /
// active-slot index into m_playerFlags[]. Already named g_tileKindMagic by the
// leveltilevalidation unit; reuse that name so the DIR32 reloc pairs by symbol.
extern i32 g_tileKindMagic;

// The impact-sound sink param (DAT_0061ab24): Process plays the impact one-shot
// through it (Play(sink, 0,0,0)). Already named g_scrollDelta by the chatbox unit;
// reuse that name so the DIR32 reloc pairs by symbol (no competing DATA - chatbox
// owns the address; Process is deferred so its pairing is non-critical anyway).
extern i32 g_scrollDelta;

// The grid manager method the action-set path runs after stamping a tile
// (thunk_FUN_00477790, __thiscall ret 0). External/no-body -> reloc-masked.
struct CActionGridMgr {
    void RefreshTile();
};
SIZE_UNKNOWN(CActionGridMgr);

// The brick / tile-object passed as Process's arg (ebx). External methods modeled
// no-body so their __thiscall dispatch reloc-masks.
struct CBrickTile {
    // Detonate @0x3bd9 IS CUserLogic::LoadGruntTypeTable; cast at the call.
    i32 m_8;   // +0x08  flag word (|= 0x10000 on uncached default break)
    i32 m_198; // +0x198 cache-state gate
    i32 m_1e4; // +0x1e4 cleared on edi==0x132
    i32 m_1ec; // +0x1ec brick-color / slot discriminator (==5 -> all-slots)
};
SIZE_UNKNOWN(CBrickTile);

// The spawned brick-break sprite (CreateSprite result) is the shared CGameObject:
// ApplyLookupGeometry (0x1505b0) selects the break animation by name, ApplyName
// (0x150540) caches its first frame, m_flags the retire bit, m_layer the
// cache-state gate. (The former CBreakSprite view also mislaid m_8/m_198 at
// offsets 0/4 - the canonical layout fixes that.)

// The impact one-shot emitter vtable slot used on g->m_30->m_28->m_30==0.
// CImpactSound::Play @0x25fe IS LeafCue::PlayIfElapsed_01f940 (header-less); local decl.
struct CImpactSound {
    // Play @0x25fe IS LeafCue::PlayIfElapsed_01f940; cast at the call.
};
SIZE_UNKNOWN(CImpactSound);

// The sound-bank lookup by name (thunk_FUN_0045b7e0, __cdecl). External no-body.
extern CImpactSound* Eng_FindSound(const char* name);

// TileGridCommand.cpp - Gruntz CTileGridCommand (C:\Proj\Gruntz).
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
// game registry's m_30 sub-manager is live. Names are placeholders; the field
// offsets + the emitted Read sequence are load-bearing.
// (The former local 13-slot CSerialArchive view is folded onto the shared
// <Gruntz/SerialArchive.h> model: the +0x30 slot this record streams through is
// the shared WRITE slot - LoadV4 is the mode-4 SAVE-side check Gate113860
// dispatches (Func4499), the "Load" name is a historical misnomer.)
// The tile trigger-data record being loaded. Reads land at +0x08..+0x20, +0x28
// (NB: +0x24 is skipped), then a 24-dword run from +0x2c.
class CTileTriggerData {
public:
    virtual void Slot0();           // vptr @+0x00 (real polymorphic; declared-only)
    i32 LoadV4(CSerialArchive* ar); // 0x1138b0

    i32 m_04;     // +0x04
    i32 m_08;     // +0x08
    i32 m_0c;     // +0x0c
    i32 m_10;     // +0x10
    i32 m_14;     // +0x14
    i32 m_18;     // +0x18
    i32 m_1c;     // +0x1c
    i32 m_20;     // +0x20
    i32 m_24;     // +0x24 (not read here)
    i32 m_28;     // +0x28
    i32 m_2c[24]; // +0x2c..+0x88
};
SIZE_UNKNOWN(CTileTriggerData);

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

// Vf1/Vf2/Vf3 stay DECLARED-ONLY (bodies live in unmatched engine TUs); cl still
// emits the ??_7 vftable (the ctor references it) with those slots as external
// refs. Vf0 (slot 0, thunk 0x1749) IS reconstructed below.

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::Vf0 (slot 0) - the one-shot Setup virtual (0x1104f0,
// shared as slot 0 across the whole *TriggerSwitchLogic family). If already set up
// (m_20 non-zero) returns 0; else scatters the 8 args into the object fields, sets
// the init guard (m_20=1) + clears m_1c, returns 1. Re-homed from ReconBatch2
// (was the Init8_1104f0 placeholder view; xref: vtable slot +0x0 via thunk 0x1749).
// ---------------------------------------------------------------------------
RVA(0x001104f0, 0x56)
i32 CTileTriggerSwitchLogic::Vf0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7) {
    if (m_20) {
        return 0;
    }
    m_04 = a1;
    m_08 = a2;
    m_key0c = a3;
    m_key1 = a4;
    m_owner = (CTileTriggerSwitchLogic*)a0;
    m_18 = a6;
    m_28 = a7;
    m_1c = 0;
    m_linkGate = a5;
    m_20 = (ListNode*)1;
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
// CTileTriggerSwitchLogic::FindIndexByKey
// Linear scan of the 24-dword m_block; returns 1 on a hit, 0 otherwise.
// ---------------------------------------------------------------------------
RVA(0x00110820, 0x23)
i32 CTileTriggerSwitchLogic::FindIndexByKey(i32 key) {
    // Scans the 24-dword array that begins at +0x3c (== &m_block[4]).
    for (i32 i = 0; i < 24; i++) {
        if (m_block[i + 4] == key) {
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
// The "TileTriggerTransition" class-name string (the CreateSprite lookup key).
extern char g_60a848[]; // s_TileTriggerTransition_0060a848

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
            CGameObject* trig =
                ((CSpriteFactory*)PB_PTR(map, 0x8))->CreateSprite(0, sx, sy, 0, g_60a848, 0x40003);
            if (trig == 0) {
                goto done;
            }
            trig->m_7c->Init(trig);
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
class CTileMultiTriggerSwitchLogic : public CTileTriggerSwitchLogic {
public:
    CTileMultiTriggerSwitchLogic();
};
SIZE_UNKNOWN(CTileMultiTriggerSwitchLogic);
VTBL(CTileMultiTriggerSwitchLogic, 0x001eaeb4); // vtable_names -> code (RTTI game class)
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
RVA(0x00111f40, 0xc4)
i32 CTileTriggerSwitchLogic::VerifyBlockLinksB() {
    if (m_linkGate == 0) {
        return 0;
    }
    ListNode* node = m_owner->m_20;
    i32 found = 0;
    CTileTriggerSwitchLogic* child = this;
    while (node != 0) {
        if (found != 0) {
            break;
        }
        ListNode* cur = node;
        node = node->m_next;
        child = cur->m_data;
        if (child != 0 && child->FindIndexByKey(m_key1) != 0) {
            found = 1;
        }
    }
    if (found == 0) {
        g_gameReg->Ack(0x80de, 0x44d);
        return 0;
    }
    i32* p = &child->m_block[4]; // child+0x3c
    for (i32 i = 0; i < 24; i++) {
        i32 key = *p;
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindChild(key, 3);
        if (c == 0) {
            g_gameReg->Ack(0x80dd, 0x44e);
            return 0;
        }
        if (c->m_linkGate == 0) {
            return 0;
        }
        p++;
    }
    return 0;
}

class CTileExclusiveTriggerSwitchLogic : public CTileTriggerSwitchLogic {

    virtual i32 Vf2() OVERRIDE; // slot 2
public:
    CTileExclusiveTriggerSwitchLogic();
};
SIZE_UNKNOWN(CTileExclusiveTriggerSwitchLogic);
VTBL(CTileExclusiveTriggerSwitchLogic, 0x001eaecc); // vtable_names -> code (RTTI game class)
RVA(0x00112050, 0x12)
CTileExclusiveTriggerSwitchLogic::CTileExclusiveTriggerSwitchLogic() {}

// --- CTileTriggerLogic family (base = 1 virtual) ---------------------------
class CGiantRockLogic : public CTileTriggerLogic {
public:
    CGiantRockLogic();
};
SIZE_UNKNOWN(CGiantRockLogic);
VTBL(CGiantRockLogic, 0x001eaee4); // vtable_names -> code (RTTI game class)
RVA(0x00112210, 0x12)
CGiantRockLogic::CGiantRockLogic() {}

class CCoveredPowerupLogic : public CTileTriggerLogic {
public:
    CCoveredPowerupLogic();
};
SIZE_UNKNOWN(CCoveredPowerupLogic);
VTBL(CCoveredPowerupLogic, 0x001eaef4); // vtable_names -> code (RTTI game class)
RVA(0x00112240, 0x12)
CCoveredPowerupLogic::CCoveredPowerupLogic() {}

class CTileTimeTriggerLogic : public CTileTriggerLogic {
public:
    CTileTimeTriggerLogic();
};
SIZE_UNKNOWN(CTileTimeTriggerLogic);
VTBL(CTileTimeTriggerLogic, 0x001eaf04); // vtable_names -> code (RTTI game class)
RVA(0x00112270, 0x12)
CTileTimeTriggerLogic::CTileTimeTriggerLogic() {}

// ---------------------------------------------------------------------------
// BuildRockBreakInGameText - the rock-break tile-effect loader (RVA 0x1122a0).
//
// MISLABELED: the delinker filed this __thiscall(void) under CTileTriggerSwitchLogic
// (Ghidra RTTI-vptr guess), but xref shows its real callers are CTerrainTileLoader::
// Load (0x75e90) and CRockBreakMgr::BuildRockBreakParticles (0x7b440), and `this` is
// a tile-command object whose +0x8/+0xc are the tile (x, y) and +0x9c a 9-cell value
// block. The RVA is locked to this class name (moving it craters the delinker pack),
// so it is reconstructed here by raw this+offset - only the offsets/bytes are load-
// bearing. It: (1) gates on whether the tile center sits inside the view rect;
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
// record whose m_14 last-play / m_18 cooldown rate-limit the CSoundCueMgr it plays) are
// the canonical CSndHost/CSndFinder/LeafCue/CSoundCueMgr from <Gruntz/SoundCue.h>
// (included above); the former per-TU RbSoundReg/RbLookupTable/RbCueRec/RbCueSound views
// are dissolved onto them (same offsets + RVAs, xref-confirmed: Lookup 0x1b8438,
// ConfigureItem 0x1360d0).

// `this` stays in esi; tile (x, y) are re-read from +0x8/+0xc at each use (retail
// caches neither, so caching them here would spill the frame from 0x14 to 0x38).
#define TX (*(i32*)(self + 0x8))
#define TY (*(i32*)(self + 0xc))

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
void CTileTriggerSwitchLogic::BuildRockBreakInGameText() {
    char* self = (char*)this;
    CSpriteFactoryHolder* gameMgr = g_gameReg->m_world; // cached only for the loop sprite

    // (1) in-rect gate: is the tile center inside the view rect (+0x13c)?
    i32 inRect = 0;
    POINT pt;
    pt.y = (TY << 5) + 0x10;
    pt.x = (TX << 5) + 0x10;
    if (PtInRect((const RECT*)&g_gameReg->m_viewOriginL, pt)) {
        inRect = 1;
    }

    // (2) 3x3 neighborhood: write each saved cell value into the level plane + notify
    // the tile grid; when in-rect, spawn a Particlez/LEVEL_ROCKBREAK sprite per cell.
    i32* cursor = (i32*)(self + 0x9c);
    for (i32 j = 0; j <= 2; j++) {
        for (i32 i = 0; i <= 2; i++) {
            i32 value = *cursor;
            i32 px = i + TX - 1;
            i32 py = j + TY - 1;
            CViewport* plane = (CViewport*)g_gameReg->m_world->m_24->m_5c;
            plane->m_cells[plane->m_rowBase[py] + px] = value;
            g_gameReg->m_tileGrid->Notify(px, py, value);
            if (inRect) {
                CGameObject* spr = gameMgr->m_8->CreateSprite(
                    0,
                    ((i + TX) << 5) - 0x10,
                    ((j + TY) << 5) - 0x10,
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
    i32 cx = (TX << 5) + 0x10;
    i32 cy = (TY << 5) + 0x10;
    g_gameReg->m_cmdGrid->FireCommand(*(i32*)(self + 0xc0), cx, cy, *(i32*)(self + 0x30), 1, 0);

    // (4) when +0xc4 is set, spawn an InGameText sprite carrying it.
    if (*(i32*)(self + 0xc4) != 0) {
        CGameObject* txt =
            g_gameReg->m_world->m_8->CreateSprite(0, cx, cy, 0x17318, "InGameText", 0x40003);
        if (txt == 0) {
            return;
        }
        txt->m_124 = *(i32*)(self + 0xc4);
    }

    // (5) on-screen + no active override -> play the LEVEL_ROCKBREAK cue.
    if ((TX << 5) + 0x10 >= g_gameReg->m_viewOriginR || (TX << 5) + 0x10 < g_gameReg->m_viewOriginL
        || (TY << 5) + 0x10 >= g_gameReg->m_viewOriginB
        || (TY << 5) + 0x10 < g_gameReg->m_viewOriginT) {
        return;
    }
    CSndHost* sreg = gameMgr->m_28; // m_28 typed CSndHost* on the canonical holder (GameRegistry.h)
    if (sreg->m_emitGate != 0) {
        return;
    }
    LeafCue* out = 0;
    sreg->m_10.Lookup("LEVEL_ROCKBREAK", &out);
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

#undef TX
#undef TY

// ---------------------------------------------------------------------------
// CTileGridCommand::ApplyMove
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
i32 CTileGridCommand::ApplyMove(i32 verb) {
    i32 v;
    if (m_34 != 0) {
        CGameRegistry* reg = g_gameReg;
        CViewport* L = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
        L->m_cells[L->m_rowBase[m_0c] + m_08] = m_34;
        v = m_34;
        ((CBrickzGrid*)reg->m_tileGrid)->ComputeCellFlags(m_08, m_0c, v);
    } else {
        switch (verb) {
            case 0x22: {
                CGameRegistry* reg = g_gameReg;
                CViewport* L = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
                v = L->m_cells[L->m_rowBase[m_0c] + m_08] + 1;
                CViewport* L2 = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
                L2->m_cells[L2->m_rowBase[m_0c] + m_08] = v;
                ((CBrickzGrid*)reg->m_tileGrid)->ComputeCellFlags(m_08, m_0c, v);
                break;
            }
            case 0x1f: {
                CGameRegistry* reg = g_gameReg;
                CViewport* L = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
                L->m_cells[L->m_rowBase[m_0c] + m_08] = 0x5b;
                ((CBrickzGrid*)reg->m_tileGrid)->ComputeCellFlags(m_08, m_0c, 0x5b);
                break;
            }
            case 0x1e: {
                CGameRegistry* reg = g_gameReg;
                CViewport* L = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
                L->m_cells[L->m_rowBase[m_0c] + m_08] = 0x5a;
                ((CBrickzGrid*)reg->m_tileGrid)->ComputeCellFlags(m_08, m_0c, 0x5a);
                break;
            }
            default:
                break;
        }
    }
    CGameRegistry* reg = g_gameReg;
    i32 py = (m_0c << 5) + 0x10;
    i32 px = (m_08 << 5) + 0x10;
    ((EngineLabelBacklog*)reg->m_cmdGrid)->LoadPowerupIconSprites(m_28, px, py, m_30, 1, 0);
    if (m_2c != 0) {
        CGameObject* rec = reg->m_world->m_8->CreateSprite(0, px, py, 95000, "InGameText", 0x40003);
        if (rec != 0) {
            rec->m_124 = m_2c;
        }
    }
    return 1;
}

class CTileSecretTriggerLogic : public CTileTriggerLogic {
    virtual i32 TileLogicVfunc0() OVERRIDE; // slot 0
public:
    CTileSecretTriggerLogic();
};
SIZE_UNKNOWN(CTileSecretTriggerLogic);
VTBL(CTileSecretTriggerLogic, 0x001eaf14); // vtable_names -> code (RTTI game class)
RVA(0x00112760, 0x12)
CTileSecretTriggerLogic::CTileSecretTriggerLogic() {}

// --- CTileTriggerSwitchLogic family (base = 4 virtuals), upper RVAs --------
class CTileSecretTriggerSwitchLogic : public CTileTriggerSwitchLogic {

    virtual i32 Vf2() OVERRIDE; // slot 2
public:
    CTileSecretTriggerSwitchLogic();
};
SIZE_UNKNOWN(CTileSecretTriggerSwitchLogic);
VTBL(CTileSecretTriggerSwitchLogic, 0x001eaf24); // vtable_names -> code (RTTI game class)
RVA(0x00112790, 0x12)
CTileSecretTriggerSwitchLogic::CTileSecretTriggerSwitchLogic() {}

class CTileTimeTriggerSwitchLogic : public CTileTriggerSwitchLogic {

    virtual i32 Vf2() OVERRIDE; // slot 2
    virtual i32 Vf3() OVERRIDE; // slot 3
public:
    CTileTimeTriggerSwitchLogic();
};
SIZE_UNKNOWN(CTileTimeTriggerSwitchLogic);
VTBL(CTileTimeTriggerSwitchLogic, 0x001eaf3c); // vtable_names -> code (RTTI game class)
RVA(0x001127c0, 0x12)
CTileTimeTriggerSwitchLogic::CTileTimeTriggerSwitchLogic() {}

class CCheckpointTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual i32 Vf2() OVERRIDE; // slot 2
    virtual i32 Vf3() OVERRIDE; // slot 3
public:
    CCheckpointTriggerSwitchLogic();
};
SIZE_UNKNOWN(CCheckpointTriggerSwitchLogic);
VTBL(CCheckpointTriggerSwitchLogic, 0x001eaf54); // vtable_names -> code (RTTI game class)
RVA(0x001127f0, 0x12)
CCheckpointTriggerSwitchLogic::CCheckpointTriggerSwitchLogic() {}

// The base switch-logic slot probes (reloc-masked free callees): thunk 0x2e0f == base
// CTileTriggerSwitchLogic slot 2; thunk 0x37e2 == base slot 3 (LoadSwitchUpSprite
// @0x1106b0). The leaf slot overrides normalize the probe result to a bool.
extern "C" i32 TileSwitchProbe_2e0f();
extern "C" i32 TileSwitchProbe_37e2();

// CTileSecretTriggerSwitchLogic::Vf2 (slot 2 override, 0x112820) - `return probe() != 0`
// (int->bool neg/sbb/neg normalize); shares the base slot-2 probe (thunk 0x2e0f).
RVA(0x00112820, 0xc)
i32 CTileSecretTriggerSwitchLogic::Vf2() {
    return TileSwitchProbe_2e0f() != 0;
}

// ---------------------------------------------------------------------------
// CTileTimeTriggerSwitchLogic::Vf2 (slot 2 override, 0x112840) - `return probe() != 0`
// (the int->bool neg/sbb/neg normalize). Re-homed from ReconBatch2 (was Probe_112840);
// xref: ??_7CTileTimeTriggerSwitchLogic@@6B@+0x8 via thunk 0x2464.
// ---------------------------------------------------------------------------
RVA(0x00112840, 0xc)
i32 CTileTimeTriggerSwitchLogic::Vf2() {
    return TileSwitchProbe_2e0f() != 0;
}

// CTileTimeTriggerSwitchLogic::Vf3 (slot 3 override, 0x112860) - `return probe() != 0`
// against the base slot-3 probe (thunk 0x37e2).
RVA(0x00112860, 0xc)
i32 CTileTimeTriggerSwitchLogic::Vf3() {
    return TileSwitchProbe_37e2() != 0;
}

// ---------------------------------------------------------------------------
// CTileGridCommand::RecordMove
// Captures the running game clock into +0x24, then hands this command to its
// owning container (m_20->MoveList1ToList2(this)).
// ---------------------------------------------------------------------------
RVA(0x00112880, 0x12)
void CTileGridCommand::RecordMove() {
    m_24 = g_645588;
    m_20->MoveList1ToList2(this);
}

// ---------------------------------------------------------------------------
// CTileGridCommand::Classify
// Drives the command's on/off duty cycle off the running game clock: while the
// elapsed time is within the lead-in (m_2c) it stays active (+1); past it, the
// remainder modulo the on+off period (m_28+m_30) selects the on or off phase,
// firing the slot-0 tick and latching m_dutyOn on each edge.  Returns +1 (active),
// 0 (just turned on, one-shot of type 0x18) or -1 (just turned off, not 0x17).
// ---------------------------------------------------------------------------
// @early-stop
// entropy-tail (~96%): logic + the single-ret1 convergence match; only the last
// type==0x17 case's ret1 is tail-duplicated instead of merged into the shared tail.
RVA(0x00112970, 0xad)
i32 CTileGridCommand::Classify(i32 arg) {
    u32 elapsed = g_645588 - m_24;
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

// ---------------------------------------------------------------------------
// CTileGridCommand::BumpCell
// Reads the active tile layer's cell at (m_08,m_0c), stores value+1 back, marks
// the cell dirty for redraw, and latches m_14.  Returns 1.
// ---------------------------------------------------------------------------
// @early-stop
// addressing-mode wall (~73%): logic identical; retail indexes the row table with
// a scale-4 address mode (`[rowtbl+y*4]`), the recompile pre-shifts y (shl 2) and
// uses scale-1, propagating through both cell accesses.
RVA(0x00112b70, 0x5a)
i32 CTileGridCommand::BumpCell() {
    CGameRegistry* reg = g_gameReg;
    CViewport* layer = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
    i32 v = layer->m_cells[m_08 + layer->m_rowBase[m_0c]] + 1;
    CViewport* layer2 = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
    layer2->m_cells[m_08 + layer2->m_rowBase[m_0c]] = v;
    ((CBrickzGrid*)reg->m_tileGrid)->ComputeCellFlags(m_08, m_0c, v);
    m_14 = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// 0x112bf0 (spatially re-homed from src/Stub/BoundaryLowerMethods.cpp). The
// decrement sibling of BumpCell: reads the active tile layer's cell at
// (m_08,m_0c), stores value-1 back, re-publishes it through the tile grid, and
// clears m_14. Reuses this TU's real grid types (TgcGameMgr/CViewport/CBrickzGrid);
// only the receiver is an orphan view - its true class is CCheckpointTriggerSwitch-
// Logic (vtbl slot 3, thunk 0x36fc) per BoundaryLowerMethodsViews.h, class
// attribution deferred.
// @early-stop
// strength-reduction wall (~73%): cl materializes m_row<<2 (shl ecx,2) and reuses
// scale-1 addressing where retail keeps m_row in a scale-4 address mode in both cell
// stores; the shift vs scaled-index pick is not steerable.
struct CDecrementCellHost { // orphan receiver (m_08 col / m_0c row / m_14, same as BumpCell)
    char pad0[8];
    i32 m_08; // +0x08 column
    i32 m_0c; // +0x0c row
    char pad10[0x14 - 0x10];
    i32 m_14; // +0x14
    i32 DecrementCell();
};
RVA(0x00112bf0, 0x5e)
i32 CDecrementCellHost::DecrementCell() {
    CGameRegistry* reg = g_gameReg;
    CViewport* layer = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
    i32 v = layer->m_cells[m_08 + layer->m_rowBase[m_0c]] - 1;
    CViewport* layer2 = ((TgcGameMgr*)reg->m_world)->m_24->m_5c;
    layer2->m_cells[m_08 + layer2->m_rowBase[m_0c]] = v;
    ((CBrickzGrid*)reg->m_tileGrid)->ComputeCellFlags(m_08, m_0c, v);
    m_14 = 0;
    return 1;
}
SIZE_UNKNOWN(CDecrementCellHost);

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
    ListNode* node = m_owner->m_20;
    i32 found = 0;
    CTileTriggerSwitchLogic* child = this;
    while (node != 0) {
        if (found != 0) {
            break;
        }
        ListNode* cur = node;
        node = node->m_next;
        child = cur->m_data;
        if (child != 0 && child->FindIndexByKey(m_key1) != 0) {
            found = 1;
        }
    }
    if (found == 0) {
        g_gameReg->Ack(0x80de, 0x452);
        return 0;
    }
    i32* p = &child->m_block[4]; // child+0x3c
    for (i32 i = 0; i < 24; i++) {
        i32 key = *p;
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindChild(key, 8);
        if (c == 0) {
            g_gameReg->Ack(0x80dd, 0x453);
            return 0;
        }
        if (c->m_linkGate == 0) {
            return 0;
        }
        p++;
    }
    return 0;
}

// ResetFlag (0x112d80): zero the m_10 flag word, return this. Out-of-line (retail
// emits it standalone; the inline member folded into its callers and never emitted).
RVA(0x00112d80, 0xa)
CTileActionEvent* CTileActionEvent::ResetFlag() {
    m_10 = 0;
    return this;
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
        CViewport* grid = (CViewport*)g_gameReg->m_world->m_24->m_5c;
        i32* cell = &grid->m_cells[grid->m_rowBase[m_tileY] + m_tileX];
        if (*cell == code) {
            return 0;
        }
        *cell = code;
        ((CActionGridMgr*)g_gameReg->m_tileGrid)->RefreshTile();
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

    CBrickTile* brick = (CBrickTile*)arg;
    if (effect != 0 && brick != 0) {
        if (effect == 0x132) {
            ((CUserLogic*)brick)->LoadGruntTypeTable(0, 1, 0, 0);
            brick->m_1e4 = 0;
        } else if (effect == 0x138) {
            ((CGruntTileMgr*)g_gameReg->m_cmdGrid)
                ->CombatCue((m_tileX << 5) + 0x10, (m_tileY << 5) + 0x10, 1, 2, -1);
        } else if (effect == 0x13e) {
            i32 px = (m_tileX << 5) + 0x10;
            i32 py = (m_tileY << 5) + 0x10;
            if (px < g_gameReg->m_viewOriginR && px >= g_gameReg->m_viewOriginL
                && py < g_gameReg->m_viewOriginB && py >= g_gameReg->m_viewOriginT
                && ((WwdGrSprHolder*)g_gameReg->m_world)->m_28->m_30 == 0) {
                CImpactSound* snd = (CImpactSound*)Eng_FindSound("GRUNTZ_NORMALGRUNT_IMPACTMM3");
                if (snd != 0) {
                    ((LeafCue*)snd)->PlayIfElapsed_01f940((i32)g_scrollDelta, 0, 0, 0);
                }
            }
            if (brick->m_1ec == 5) {
                m_playerFlags[0] = 1;
                m_playerFlags[1] = 1;
                m_playerFlags[2] = 1;
                m_playerFlags[3] = 1;
                SetActionCode(m_actionCode);
                return 0;
            }
            m_playerFlags[brick->m_1ec] = 1;
            SetActionCode(m_actionCode);
            return 0;
        } else if (effect == 0x144) {
            ((EngineLabelBacklog*)g_gameReg->m_cmdGrid)
                ->LoadExplosionSprites((m_tileX << 5) + 0x10, (m_tileY << 5) + 0x10, -1, 2);
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

// 0x113860 - Gate113860: mode gate over a container element - validate `obj`
// against the mode (4 -> the write-check 0x4499, 7 -> the read-check 0x1893, both
// TileTriggerSwitchLogic-family helpers), passing through otherwise. __stdcall,
// ret 0x10. The __stdcall helper SerializeApplyA / CTileTriggerFactory::Build call.
// Re-homed from src/Stub/BoundaryLowerMethods.cpp (was the Gate113860 placeholder).
extern i32 __stdcall Func1893(void* p); // 0x1893 -> 0x1139a0
extern i32 __stdcall Func4499(void* p); // 0x4499 -> 0x1138b0
// @early-stop
// regalloc wall (~93%): retail keeps obj in eax (so the obj==0 return 0 is free); cl
// pins it in ecx and adds xor eax. switch(mode) recovers the case layout; the eax vs
// ecx pick is not source-steerable.
RVA(0x00113860, 0x3b)
i32 __stdcall Gate113860(void* obj, i32 mode, i32 a3, i32 a4) {
    if (obj == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (!Func4499(obj)) {
                return 0;
            }
            break;
        case 7:
            if (!Func1893(obj)) {
                return 0;
            }
            break;
    }
    return 1;
}

// ===========================================================================
// 0x1138b0 - stream the trigger-data block through the archive's +0x30 (write)
// slot. Bails if the archive is null or the registry sub-manager (m_30) is not
// yet live; otherwise streams the eight scalar fields then the 24-dword tail
// run, returning 1.
// ===========================================================================
RVA(0x001138b0, 0xb4)
i32 CTileTriggerData::LoadV4(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    ar->Write(&m_08, 4);
    ar->Write(&m_0c, 4);
    ar->Write(&m_10, 4);
    ar->Write(&m_14, 4);
    ar->Write(&m_18, 4);
    ar->Write(&m_1c, 4);
    ar->Write(&m_20, 4);
    ar->Write(&m_28, 4);
    i32* p = m_2c;
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
// CTileTriggerSwitchLogic::ValidateByType
// Returns 0 if obj is null; for type 4 / 7 defers to the matching validator
// (returns 0 on its failure); any other type passes (returns 1).
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~93%): switch-case ordering + validator calls match; retail
// keeps arg1 in eax (returns it as the null-zero, push eax for validators) vs
// our ecx + explicit xor in the null block. Entry-block register only.
RVA(0x00113a90, 0x3b)
i32 CTileTriggerSwitchLogic::ValidateByType(void* obj, i32 type, i32 a3, i32 a4) {
    if (obj == 0) {
        return 0;
    }
    switch (type) {
        case 4:
            if (TileSwitchCheckType4(obj) == 0) {
                return 0;
            }
            break;
        case 7:
            if (TileSwitchCheckType7(obj) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileGridCommand::Serialize
// Returns 0 if the stream is null or the active game-manager (g_gameReg+0x30) is
// null; otherwise transfers the command's scalar fields then a 24-dword grid
// block through the stream's Transfer (vtable slot 12) and returns 1.
// ---------------------------------------------------------------------------
RVA(0x00113ae0, 0xe8)
i32 CTileGridCommand::Serialize(CSerialArchive* s) {
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
    i32* p = m_grid;
    for (i32 i = 0; i < 24; i++) {
        s->Write(p, 4);
        p++;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileGridCommand::Deserialize
// The read counterpart of Serialize: same null/registry guard, same field list,
// but each transfer goes through the stream's read slot (+0x2c) instead of the
// write slot (+0x30). Reads the 12 scalar fields then the 24-dword grid block.
// ---------------------------------------------------------------------------
RVA(0x00113c10, 0xe8)
i32 CTileGridCommand::Deserialize(CSerialArchive* s) {
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
    i32* p = m_grid;
    for (i32 i = 0; i < 24; i++) {
        s->Read(p, 4);
        p++;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::ApplyByType
// Returns 0 if obj is null or the base apply fails; for type 4 / 7 defers to the
// matching per-type apply (returns 0 on its failure); otherwise returns 1.
// ---------------------------------------------------------------------------
RVA(0x00113d40, 0x6f)
i32 CTileTriggerSwitchLogic::ApplyByType(void* obj, i32 type, i32 a3, i32 a4) {
    if (obj == 0) {
        return 0;
    }
    if (ApplyBase(obj, type, a3, a4) == 0) {
        return 0;
    }
    switch (type) {
        case 4:
            if (ApplyType4(obj) == 0) {
                return 0;
            }
            break;
        case 7:
            if (ApplyType7(obj) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::SerializeMatrix
// Streams two header dwords (+0xc0, +0xc4) then a 3x3 dword matrix (+0x9c..) via
// the stream's Transfer.  Returns 0 if the stream or the active game-manager
// (g_gameReg+0x30) is null, else 1.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~95%): whole body byte-identical; retail pins this->esi /
// stream->edi (pushes all 4 callee regs, then loads args) vs our this->edi /
// stream->esi (arg load interleaved with the pushes). Reg-pair swap only.
RVA(0x00113dd0, 0x7b)
i32 CTileTriggerSwitchLogic::SerializeMatrix(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_block[37], 4); // +0xc0
    s->Write(&m_block[38], 4); // +0xc4
    i32* p = &m_block[28];     // +0x9c
    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Write(p, 4);
            p++;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::DeserializeMatrix (0x113e70) - the READ mirror of
// SerializeMatrix: streams two header dwords (+0xc0, +0xc4) then a 3x3 dword matrix
// (+0x9c..) via the stream's Read slot. Returns 0 if the stream or the active
// game-manager (g_gameReg+0x30) is null, else 1. This is the type-7 (load) apply
// ApplyByType dispatches to as ApplyType7 (thunk 0x3cd3). Re-homed from
// src/Stub/BoundaryLowerMethods.cpp (was the C113e70 placeholder view).
// @early-stop
// esi/edi regalloc wall (~95%, same as SerializeMatrix): whole body byte-identical;
// retail pins this->esi / stream->edi vs our this->edi / stream->esi. Reg-pair swap.
RVA(0x00113e70, 0x7b)
i32 CTileTriggerSwitchLogic::DeserializeMatrix(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_block[37], 4); // +0xc0
    s->Read(&m_block[38], 4); // +0xc4
    i32* p = &m_block[28];    // +0x9c
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
