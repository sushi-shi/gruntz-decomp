// GruntSteps.cpp - the SECOND original grunt TU (retail text 0x50ca0-0x55160):
// the grunt movement-step / move-sound / tile-claim / serialize family, carved
// out of the conflated Grunt.cpp (wave3-I grunt-region partition).
//
// original TU: filename unknown (@identity-TODO; named for the dominant
// movement-step family). Evidence this is its OWN obj:
//   * its private .data extent (0x20dbf8, StepCompassMove's statics) sits
//     between the 0x4dd50 userlogic extent and the 0x56f80 grunt-combat extent
//     in the 98%-monotone .data contribution order.
//   * the TU_MIGRATION extent-overlap claim "0x50ca0+0x56f80 = one obj" is
//     REFUTED: it was driven by the .bss act-registry singleton band (0x2445xx),
//     which is provably NOT obj-ordered (wormholeacts/warlord/secrettrigs
//     interleave there); the initialized-.data band has NO overlap.
//   * gamestaterecordload (0x555e0) + gruntdatarecord (0x56da0) sit between
//     this interval and 0x56f80 in text - one obj spanning both would have to
//     contain them (no evidence does).
// In-interval fold: LoadVehicleGruntSprites @0x50ce0 (ex VehicleGruntSprites.cpp)
// is text-contained (between 0x50ca0 and 0x511b0 - contiguity-forced).
// GruntTubeAnim.cpp (0x50a50, gap 139 before 0x50ca0) is a PROBABLE head of this
// TU but stays split (no privates/frags to prove it; noted there).
#include <Bute/ButeTree.h> // CButeTree::Find - g_buteTree @0x6bf620 (was the CEntranceAnimSrc view)
#include <Io/FileMem.h>    // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/Grunt.h>
#include <Gruntz/TypeKeyColl.h> // g_typeColl (folded CAnimNameResolver anim registry)
extern CTypeKeyColl g_typeColl; // 0x6bf650 - its m_alloc (+0x1c) / m_grown (+0x20)
                                // WERE the fake g_animScratch / g_animScratchCount
                                // globals (defined in 5 TUs each; LNK2005)
#include <Gruntz/ActReg.h>      // CLookupColl/CActReg::ResolveEntry
#include <Gruntz/AniElement.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/GameStateRecord.h> // CSerialObjRef::Chain (0x8c00) + CGameStateRecord::Load (0x555e0)
#include <Gruntz/TileWireLogic.h> // CTileWireLogic::WireTileSwitchLogic (0x6c130)
#include <Gruntz/BoundaryLowerMethodsViews.h>
#include <Gruntz/Effect6b.h>
#include <Gruntz/SoundCueMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
extern "C" WwdGameReg* g_gameReg; // 0x64556c (the WwdGameReg view, as in Grunt.cpp)
#include <rva.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/AssetNamespaceLoader.h> // CNamespaceLoader (vehicle sprite-set registration)
#include <Gruntz/TileGrid.h>             // the registry +0x70 tile occupancy grid
#include <Gruntz/GruntzMgr.h>            // the MFC-side registry view (vehicle path)
#include <Gruntz/PickupType.h>           // the toy/vehicle grunt-kind id band
#include <Gruntz/TriggerMgr.h>           // CTriggerMgr::ApplySwitch

// Entrance-animation globals (reloc-masked; see Grunt.h).
extern CButeMgr g_buteMgr;
static char s_TimePerTile[] = "TimePerTile";
static char s_Grunt[] = "Grunt";                               // s_Grunt_0060a9ec
static char s_EntranceSafeTime[] = "EntranceSafeTime";         // s_EntranceSafeTime_0060df98
static char s_IdleDelay[] = "IdleDelay";                       // s_IdleDelay_0060e1a0
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius"; // s_PlayerDefenderRadius_0060e1ac
static char s_CombatTimeout[] = "CombatTimeout";               // s_CombatTimeout_0060df84

// ---------------------------------------------------------------------------
// CGrunt::StepCompassMove()   @0x51c00   (ret 0, /GX)
// The per-tick compass-move driver. The vehicle/move-command field on the current
// tile (or the entrance-cell direction) selects one of 8 compass moves + a grunt-
// voice record; the target tile is validated against board occupancy + corner-cut
// walls; on success the move is committed (release old tile, claim new tile, fire
// the voice cue, re-face) and the move counter advances.
//
// The two grunt-vehicle name -> "ToyTiles" config + the random toy-tile bag drive
// the multi-step toy path. The destructible CString temp + the CToyTileBag local
// force the /GX EH frame.
static char s_ToyTiles[] = "ToyTiles";                     // s_ToyTiles_0060dbf8
static const char s_BABYWALKERGRUNT[] = "BABYWALKERGRUNT"; // s_..._0060da6c
static const char s_BIGWHEELGRUNT[] = "BIGWHEELGRUNT";     // s_..._0060da48
static const char s_GOKARTGRUNT[] = "GOKARTGRUNT";         // s_..._0060da38
static const char s_POGOSTICKGRUNT[] = "POGOSTICKGRUNT";   // s_..._0060d9fc

// Read the tile-flag word at board cell (tx, ty); out-of-bounds -> 1 (blocking).
static __inline i32 s_TileFlags(GruntBoard* b, i32 tx, i32 ty) {
    if ((u32)tx >= (u32)b->m_c || (u32)ty >= (u32)b->m_10) {
        return 1;
    }
    return ((i32*)b->m_8[ty])[tx * 7];
}

// True if a move from the grunt's current tile to (moveX, moveY) is committable:
// the target is in-bounds + unoccupied (the arrival/owner flag-word test) AND, for a
// diagonal step, neither cardinal corner tile carries the wall bit (0x2000).
static __inline i32 s_CanCommitMove(CGrunt* g, i32 moveX, i32 moveY) {
    GruntBoard* board = g_gameReg->m_tileGrid;
    i32 tx = g->m_lastTilePxX >> 5;
    i32 ty = g->m_lastTilePxY >> 5;
    i32 mtx = moveX >> 5;
    i32 mty = moveY >> 5;
    i32 arr = g->m_arrivalFlags | 0x20000000;
    if (tx == mtx && ty == mty) {
        return 1;
    }
    if ((u32)mtx >= (u32)board->m_c || (u32)mty >= (u32)board->m_10) {
        return 0;
    }
    i32* tgt = &((i32*)board->m_8[mty])[mtx * 7];
    i32 tflags = *tgt;
    i32 hit = arr & tflags;
    if (hit & 0x20000000) {
        return 0;
    }
    if (hit != 0) {
        i32 mask = g->m_24c | 0x18000482;
        if ((tflags & mask) == 0) {
            return 0;
        }
    }
    i32 dx = mtx - tx;
    i32 dy = mty - ty;
    if (dx == 0 || dy == 0) {
        return 1;
    }
    char* cur = board->m_8[ty] + tx * 7 * 4;
    char* tg = (char*)tgt;
    i32 stride = board->m_c * 7 * 4; // bytes per board row
    if (dx > 0) {
        if (dy > 0) {
            if ((cur[0x1d] & 0x20) || (cur[stride + 1] & 0x20) || (*(i32*)(tg - 0x1c) & 0x2000)
                || (*(i32*)(tg - stride) & 0x2000)) {
                return 0;
            }
        } else {
            if ((cur[0x1d] & 0x20) || (*(i32*)(cur - stride) & 0x2000)
                || (*(i32*)(tg - 0x1c) & 0x2000) || (*(i32*)(tg + stride) & 0x2000)) {
                return 0;
            }
        }
    } else {
        if (dy > 0) {
            if ((cur[-0x1b] & 0x20) || (cur[stride + 1] & 0x20) || (*(i32*)(tg + 0x1c) & 0x2000)
                || (*(i32*)(tg - stride) & 0x2000)) {
                return 0;
            }
        } else {
            if ((cur[-0x1b] & 0x20) || (*(i32*)(cur - stride) & 0x2000)
                || (*(i32*)(tg + 0x1c) & 0x2000) || (*(i32*)(tg + stride) & 0x2000)) {
                return 0;
            }
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::SerializeMove(ar, mode, a3, a4)   @0x53b80   (ret 0x10)
// The grunt move/idle-timer record (de)serializer. mode 4 = save (ar->Write,
// vtable slot +0x30), mode 7 = load (ar->Read, slot +0x2c). It chains the head
// anim-state serializer (0x16e7f0, external) and the +0x150/+0x43c sub-records,
// runs a mode-specific pre-step, then streams the eight 16-byte (double-pair)
// timer records at +0x810..+0x880 directly, and finishes through the six
// tail sub-record serializers (+0x890/+0x8a0/+0x8b0/+0x8c0/+0x308/+0x278). The
// per-record `if (mode==4) Write,Write; else if (mode==7) Read,Read;` inlines
// once per record (each half is 8 bytes: p, then p+8).
static __inline void SerRecord(CGruntArchive* ar, i32 mode, char* p) {
    switch (mode) {
        case 4:
            ar->Write(p, 8);
            ar->Write(p + 8, 8);
            break;
        case 7:
            ar->Read(p, 8);
            ar->Read(p + 8, 8);
            break;
    }
}

// ===========================================================================
// The 5 grunt movement / anim-name dispatch state machines (formerly the
// CUserLogic_* stubs @0x4b370 / 0x4c170 / 0x52fb0 / 0x5f310 / 0x6a6d0). Each
// resolves the grunt's current anim-set node name
// (g_typeColl.GetNameRecord(m_14->m_1c), or the scratch-teardown
// GetNameRecords form) and dispatches on its single-letter type code
// (A/D/I/G/L/P/O/Q/J/N/M/K), driving the grunt's movement/arrival state, recycling
// its occupied-coord nodes onto the shared freelist, and re-latching m_14->m_1c to
// a new anim set via g_entranceAnimSrc.LookupAnimSet. The inline-strcmp `== bool` setcc
// reject form is per docs/patterns/strcmp-eq-bool-local-setcc.md.
//
// These are the CGrunt analogues of CBattlezMapConfig::Method_025d90 /
// Method_02f620 (the documented large-state-machine + grid-regalloc walls). Each is
// reconstructed complete in shape/order; all carry @early-stop on those walls.
// Raw-offset member access (the campaign style used by the cluster above) keeps the
// giant ~0x46c layout tractable.

// A grunt board-tile flag fetch (g_gameReg->m_tileGrid board, tile = row[y][x*7]); the
// out-of-bounds path returns 1 (so any flag test passes). Shared by all five.
static __inline i32 GruntTileFlags(i32 tx, i32 ty) {
    GruntBoard* b = g_gameReg->m_tileGrid;
    if ((u32)tx >= (u32)b->m_c || (u32)ty >= (u32)b->m_10) {
        return 1;
    }
    return ((i32*)b->m_8[ty])[tx * 7];
}

// CGrunt::LoadTypeTableClearMove(typeId) @0x50ca0 - reload the grunt type table for
// `typeId` (the inherited CUserLogic driver, thunk 0x3bd9 -> 0x4dd50) then reset the
// move-mode pair at +0x1a0/+0x1a4. Called at RunEntranceMove's tail. __thiscall.
// Re-homed from src/Stub/BoundaryLowerMethods.cpp (was C50ca0::M).
// NOTE: the named CGrunt::m_moveMode (+0x1a0 per Grunt.h) currently compiles to +0x2c0
// (the modeled base chain is ~0x120 oversized before it) - so this reset uses explicit
// this+offset access to hit retail's real +0x1a0/+0x1a4 (documented naming-independent
// codegen; see the layout-gap TODO on CGrunt::m_moveMode).
RVA(0x00050ca0, 0x2b)
void CGrunt::LoadTypeTableClearMove(i32 typeId) {
    // the real callee is the inherited CUserLogic::LoadGruntTypeTable (0x4dd50), not
    // CGrunt's i32-returning shadow (which InGameIcon still needs for its result read)
    CUserLogic::LoadGruntTypeTable(typeId, 0, 0, 0);
    *(i32*)((char*)this + 0x1a0) = -1;
    *(i32*)((char*)this + 0x1a4) = 0;
}

// ===========================================================================
// Migrated CGrunt cluster (formerly the CUserLogic_* stubs in
// src/Stub/Discovered.cpp). A prior matcher proved this whole block is CGrunt:
// the dtor @0xf2f0 stamps vtable 0x5e8754 over CUserLogic/CUserBase bases, and
// the anim loader @0x49c60 builds "GRUNTZ_<type>_<POSE>" keys. Reconstructed in
// ascending retail-RVA order. Raw-offset member access (the campaign style used
// by ReadConfigFromButeMgr above) keeps the giant ~0x46c layout tractable.
// ===========================================================================

// The global free-list pool the name caches recycle into (head @0x645544, base
// subtrahend @0x64554c). Defined TU-local (reloc-masked); shared in retail.
extern i32 g_serialCounter; // DEFINED in src/Gruntz/Grunt.cpp (owner TU)

// The grunt movement / anim-name dispatch state machines' reloc-masked data.
// All TU-local definitions (reloc-masked against the retail symbols); the grunt
// freelist aliases the same g_coordPool.m_freeHead/Base pool (0x645544 / 0x64554c).
extern "C" WwdGameReg* g_gameReg; // ?g_gameReg@@3PAUWwdGameReg@@A @0x64556c
extern FreeNodePool g_coordPool;  // DAT_00645540 - DEFINED once, in
                                  // src/Gruntz/GameText.cpp (the pool's owner TU).
                                  // It used to be DEFINED here too: six .cpp files each
                                  // defined it, i.e. six .bss objects for one global
                                  // (LNK2005). Only the owner defines; everyone externs.

// The single-letter anim type-code literals live ONCE in retail .rdata and are shared by
// every TU that compares against them (s_codeA..s_codeQ, declared in <Gruntz/Grunt.h>,
// DATA-bound in src/Globals.cpp). They used to be re-DEFINED here - 14 external symbols
// duplicated across 5 objs = a duplicate-symbol link defect.

// ==== LoadVehicleGruntSprites @0x50ce0 (ex VehicleGruntSprites.cpp; text-contained) ====
// CTileWireLogic::WireTileSwitchLogic (0x6c130) now comes from the shared header.

// The game registry singleton (*0x24556c): this TU declares it as the
// WwdGameReg view (Grunt.cpp style); the vehicle path reads it through the
// MFC-side CGruntzMgr view with a per-use cast (same load bytes).

// The grunt-command object's follow-up registrar (this->m_260).
struct CGruntCmdObj;
struct CGruntRegistrar {
    // RegisterA @0x26df IS CTriggerMgr::ApplySwitch (recv-this dropped); cast at the call.
    // RegisterB @0x3dfa IS CTileWireLogic::WireTileSwitchLogic; cast at the call.
};
// The grunt's current-tile anchor (this->m_10): m_5c/m_60 are its committed coords.
struct CGruntAnchor {
    char m_pad0[0x5c];
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
};

struct CGruntCmdObj {
    i32 LoadVehicleGruntSprites(i32 kind);

    char m_pad0[0x10];
    CGruntAnchor* m_10; // +0x10
    char m_pad14[0x17c - 0x14];
    i32 m_17c; // +0x17c  tile X (pixels)
    i32 m_180; // +0x180  tile Y (pixels)
    char m_pad184[0x198 - 0x184];
    i32 m_198; // +0x198  requested kind
    char m_pad19c[0x1a0 - 0x19c];
    i32 m_1a0; // +0x1a0
    char m_pad1a4[0x260 - 0x1a4];
    CGruntRegistrar* m_260; // +0x260
    char m_pad264[0x2b0 - 0x264];
    i32 m_region0[4]; // +0x2b0
    i32 m_region1[4]; // +0x2c0
};

// The toy/vehicle-grunt kind LoadVehicleGruntSprites dispatches on (kind, 0x17..0x20)
// is the toy band of PickupType, the shared object/pickup/grunt-kind id space in
// <Gruntz/PickupType.h>. Each name is confirmed by its case's "<NAME>GRUNT" sprite key;
// the ids are byte-verified to equal PickupType's toys (BABYWALKER=0x17..YOYO=0x20).
// Same immediates as the bare labels -> naming is matching-neutral.

// @early-stop
// ~37%: COMPLETE + correct (prologue/dispatch/jump-table/common CString-tail/
// tile-A-B gate + the two registrations all model retail). Residual is a GLOBAL
// register-COLORING wall, re-proven 2026-07-05 with llvm-objdump -dr base vs target:
//   * The A/B tile-code tail computes `code = grid->m_8[m_180>>5][(m_17c>>5)*7+4]`.
//     RETAIL colors its temps into callee-saved ebp ((x>>5)*7 via shl3/sub) and ebx
//     (grid->m_8 row table); THIS cl colors the same temps into volatile edi/edx and
//     leaves ebp/ebx free.
//   * Because ebp/ebx are free across the switch here, this cl HOISTS the shared
//     {-1,-1,1,1}/{0,0,0,0} region constants into ebx(-1)/ebp(1)/edi(0) -> each of the
//     10 arms is 10 esi-relative stores. Retail, denied ebp/ebx by the tail, instead
//     re-materializes per-arm (lea ebx,[esi+0x2b0]; or eax,-1; or ecx,-1; mov edx,1;
//     store; xor-recycle to 0; store region1) -> each arm ~20 insns. The 2x-longer
//     arm x10 + the prologue reg diff is the whole 63% residual.
// Not a source-shape bug: paired-column vs sequential region writes + pointer-vs-index
// spellings were tried (identical bytes / hoist unchanged). The hoist is downstream of
// the tail's callee-saved coloring, which cl build-8034 assigns opposite to retail and
// which no source spelling of the tail expression reorders. Deferred to the final sweep.
RVA(0x00050ce0, 0x399)
i32 CGruntCmdObj::LoadVehicleGruntSprites(i32 kind) {
    m_198 = kind;
    m_1a0 = -1;

    CString name;
    // Region init is copy-pasted into every arm (retail repeats it 10x, byte-identical):
    // region0 = {-1,-1,1,1}, region1 = {0,0,0,0}, in address order.
#define REGION_INIT()                                                                              \
    do {                                                                                           \
        i32* r0 = m_region0;                                                                       \
        r0[0] = -1;                                                                                \
        r0[1] = -1;                                                                                \
        r0[2] = 1;                                                                                 \
        r0[3] = 1;                                                                                 \
        r0 = m_region1;                                                                            \
        r0[0] = 0;                                                                                 \
        r0[1] = 0;                                                                                 \
        r0[2] = 0;                                                                                 \
        r0[3] = 0;                                                                                 \
    } while (0)
    switch (kind) {
        case PICKUP_BABYWALKER:
            REGION_INIT();
            name = "BABYWALKERGRUNT";
            break;
        case PICKUP_BEACHBALL:
            REGION_INIT();
            name = "BEACHBALLGRUNT";
            break;
        case PICKUP_BIGWHEEL:
            REGION_INIT();
            name = "BIGWHEELGRUNT";
            break;
        case PICKUP_GOKART:
            REGION_INIT();
            name = "GOKARTGRUNT";
            break;
        case PICKUP_JACKINTHEBOX:
            REGION_INIT();
            name = "JACKINTHEBOXGRUNT";
            break;
        case PICKUP_JUMPROPE:
            REGION_INIT();
            name = "JUMPROPEGRUNT";
            break;
        case PICKUP_POGOSTICK:
            REGION_INIT();
            name = "POGOSTICKGRUNT";
            break;
        case PICKUP_SCROLL:
            REGION_INIT();
            name = "SCROLLGRUNT";
            break;
        case PICKUP_SQUEAKTOY:
            REGION_INIT();
            name = "SQUEAKTOYGRUNT";
            break;
        case PICKUP_YOYO:
            REGION_INIT();
            name = "YOYOGRUNT";
            break;
        default:
            break;
    }
#undef REGION_INIT

    ((CNamespaceLoader*)((CGruntzMgr*)(void*)g_gameReg)->m_curState)
        ->BuildAssetNamespacePrefixes(name, 1, 1, 0);

    i32 code =
        ((i32*)((CGruntzMgr*)(void*)g_gameReg)->m_tileGrid->m_8[m_180 >> 5])[(m_17c >> 5) * 7 + 4];
    if (code == 0x41 || code == 0x42) {
        if (m_10->m_5c == m_17c && m_10->m_60 == m_180) {
            // retail pushes (this, x, y) - ret 0xc; the old 2-arg spelling had dropped
            // the receiver arg ("recv-this dropped" note above).
            ((CTriggerMgr*)m_260)->ApplySwitch((CGrunt*)this, m_17c, m_180);
            ((CTileWireLogic*)m_260)->WireTileSwitchLogic((void*)this, m_17c, m_180);
        }
    }
    return 1;
}
// The 8 compass grunt-voice records (3 DWORDs each, runtime-filled .bss) +
// PlaySound (the @0x4ac10 entrance handler, external/reloc-masked). TU-local
// definitions bound to their retail .bss RVAs so each `mov ds:addr` reloc-checks
// against the real target (array mangling -> @data-symbol names the exact cl sym).
// @data-symbol: ?g_voiceN@@3PAHA 0x002448e8
i32 g_voiceN[3];
// @data-symbol: ?g_voiceS@@3PAHA 0x002448d8
i32 g_voiceS[3];
// @data-symbol: ?g_voiceE@@3PAHA 0x002448c8
i32 g_voiceE[3];
// @data-symbol: ?g_voiceW@@3PAHA 0x002448f8
i32 g_voiceW[3];
// @data-symbol: ?g_voiceSE@@3PAHA 0x00244928
i32 g_voiceSE[3];
// @data-symbol: ?g_voiceNW@@3PAHA 0x00244918
i32 g_voiceNW[3];
// @data-symbol: ?g_voiceNE@@3PAHA 0x00244908
i32 g_voiceNE[3];
// @data-symbol: ?g_voiceSW@@3PAHA 0x00244948
i32 g_voiceSW[3];

// CGrunt::PlayMoveSound(x, y) @0x511b0 - directional grunt-voice dispatcher.
// Bucketize the screen vector (x,y) - (m_10->m_5c, m_10->m_60) into one of 8
// compass directions by the slope dy/dx vs {+-2.0f, +-0.5} and fire the matching
// 3-DWORD voice record through PlaySound(1000, rec). __thiscall, ret 8, frameless.
RVA(0x000511b0, 0x246)
void CGrunt::PlayMoveSound(i32 x, i32 y) {
    CGruntHud* h = m_10;
    i32 dy = y - h->m_60;
    i32 dx = x - h->m_5c;
    i32 cx = h->m_5c;

    if (dx == 0) {
        if (y > h->m_60) {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceN);
        } else if (y < h->m_60) {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceS);
        }
        return;
    }

    float ratio = (float)dy / dx;
    if (ratio > 2.0f || ratio < -2.0f) {
        if (y > h->m_60) {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceN);
        } else {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceS);
        }
        return;
    }
    if (ratio <= 0.5 && ratio >= -0.5) {
        if (x > cx) {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceE);
        } else {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceW);
        }
        return;
    }
    if (ratio > 0.5) {
        if (x > cx) {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceSE);
        } else {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceNW);
        }
        return;
    }
    if (ratio < -0.5) {
        if (x > cx) {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceNE);
        } else {
            PlaySound(1000, *(CGruntVoiceRec*)g_voiceSW);
        }
    }
}

// CGrunt::CanShowStamina() @0x514a0 - the stamina-bar visibility gate: shown
// only if not powered-up (m_combatActive==0), stamina below full (m_stamina < 0x64), and not
// mid-entrance (m_entranceActive==0).
RVA(0x000514a0, 0x26)
i32 CGrunt::CanShowStamina() {
    if (m_combatActive == 0 && m_stamina >= 0x64 && m_entranceActive == 0) {
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::PlayMoveSoundAtTile(tx, ty)   @0x514e0   (__thiscall, ret 8)
// Scale the tile coords to HUD pixel centers (tile*32 + 16) and forward to the
// directional grunt-voice dispatcher. `this` flows straight through to
// PlayMoveSound (both __thiscall).
RVA(0x000514e0, 0x1e)
void CGrunt::PlayMoveSoundAtTile(i32 tx, i32 ty) {
    PlayMoveSound(tx * 0x20 + 0x10, ty * 0x20 + 0x10);
}

// ---------------------------------------------------------------------------
// CGrunt::SnapToLastTile(a)   @0x517b0   (__thiscall, ret 4)
// Snap the grunt's HUD position to its last occupied tile (m_10->m_5c/m_60 =
// m_lastTilePxX/Y), bump the entrance latched-anim id (m_10->m_74 += 0x186a0 ->
// marks the HUD geometry dirty), commit the entrance position (SetEntrancePos(a,1)),
// and - if an arrival is pending (m_arrivalPending) - notify the tile mgr of the
// settled move and clear the pending latch.
RVA(0x000517b0, 0x7d)
void CGrunt::SnapToLastTile(i32 a) {
    m_10->m_5c = m_lastTilePxX;
    m_10->m_60 = m_lastTilePxY;
    CGruntHud* h = m_10;
    if (h->m_74 != h->m_60 + 0x186a0) {
        h->m_74 = h->m_60 + 0x186a0;
        h->m_8 |= 0x20000;
    }
    SetEntrancePos(a, 1);
    if (m_arrivalPending != 0) {
        // 0x6c130 is CTileWireLogic::WireTileSwitchLogic (the settled-move commit),
        // not a CGruntTileMgr method - cast at the call (as m_260's site above does).
        ((CTileWireLogic*)m_tileMgr)
            ->WireTileSwitchLogic((void*)this, m_lastTilePxX, m_lastTilePxY);
        m_arrivalPending = 0;
    }
}

// ---------------------------------------------------------------------------
// CGrunt::RectContains(x, y)   @0x51850   (__thiscall, ret 8)
// @early-stop
// register-relative rect-walk plateau: the logic is exact - builds two tile-space
// rects from the grunt's stored bounds (this+0x290 / this+0x2a0) shifted by the
// committed pixel position (m_17c/m_180)>>5 and inflated by +1 on the high edges,
// tests the query point (x>>5, y>>5) against the live rect(s) via IsRectEmpty, and
// returns whether it is contained. Residue: cl interleaves the two CRect builds
// and reuses the [esp+N] temp slots in an order the source can't pin (it folds the
// member loads + the >>5 shifts across both rects); the IAT-hoisted `IsRectEmpty`
// call shape matches. Pure stack-slot/regalloc scheduling. Deferred to final sweep.
RVA(0x00051850, 0x165)
i32 CGrunt::RectContains(i32 x, i32 y) {
    i32 dx = m_lastTilePxX >> 5;
    i32 dy = m_lastTilePxY >> 5;
    i32 px = x >> 5;
    i32 py = y >> 5;

    i32* ra = (i32*)(&m_reachRectLeft);
    i32* rb = (i32*)(&m_2a0);

    RECT r1;
    r1.left = ra[0] + dx;
    r1.top = ra[1] + dy;
    r1.right = ra[2] + dx + 1;
    r1.bottom = ra[3] + dy + 1;

    RECT r2;
    r2.left = rb[0] + dx;
    r2.top = rb[1] + dy;
    r2.right = rb[2] + dx;
    r2.bottom = rb[3] + dy;

    if (IsRectEmpty(&r1) || IsRectEmpty(&r2)) {
        if (IsRectEmpty(&r2)) {
            // rect2 degenerate: test the point against rect1 only.
            if (px < r1.right && px >= r1.left && py < r1.bottom && py >= r1.top) {
                return 1;
            }
            return 0;
        }
        return 0;
    }
    // both rects live: the point must sit inside rect1 and (left/top of) rect2.
    if (px < r1.right && px >= r1.left && py < r1.bottom && py >= r1.top) {
        if (px >= r2.right || px < r2.left || py >= r2.bottom || py >= r2.top) {
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::RectContainsGated(x, y)   @0x51a20   (__thiscall, ret 8)
// @early-stop
// register-relative rect-walk plateau (sibling of RectContains, same wall): gated
// on the m_198 enable flag, then builds two tile-space rects from this+0x2b0 /
// this+0x2c0 shifted by (m_17c/m_180)>>5 (rect1's high edges +1), and tests the
// query point (x>>5, y>>5) against them via IsRectEmpty + the 4-way bounds compare.
// Residue: identical to RectContains - cl interleaves the two CRect builds and the
// [esp+N] temp-slot reuse in an unpinnable order. Deferred to the final sweep.
// (wave5-R7: the SerializeMove/SnapToLastTile CALL-target rebindings in this TU
// nudged the shared temp-slot/regalloc schedule here ~62->59.5%; body unchanged.)
RVA(0x00051a20, 0x17d)
i32 CGrunt::RectContainsGated(i32 x, i32 y) {
    i32 px = x >> 5;
    i32 py = y >> 5;
    i32 dx = m_lastTilePxX >> 5;
    i32 dy = m_lastTilePxY >> 5;

    i32* ra = (i32*)(&m_2b0);
    i32* rb = (i32*)(&m_2c0);

    RECT r1;
    r1.left = ra[0] + dx;
    r1.top = ra[1] + dy;
    r1.right = ra[2] + dx + 1;
    r1.bottom = ra[3] + dy + 1;

    RECT r2;
    r2.left = rb[0] + dx;
    r2.top = rb[1] + dy;
    r2.right = rb[2] + dx;
    r2.bottom = rb[3] + dy;

    if (m_198 == 0) {
        return 0;
    }

    if (IsRectEmpty(&r1) || IsRectEmpty(&r2)) {
        if (IsRectEmpty(&r2)) {
            if (px < r1.right && px >= r1.left && py < r1.bottom && py >= r1.top) {
                return 1;
            }
            return 0;
        }
        return 0;
    }
    if (px < r1.right && px >= r1.left && py < r1.bottom && py >= r1.top) {
        if (px >= r2.right || px < r2.left || py >= r2.bottom || py >= r2.top) {
            return 1;
        }
    }
    return 0;
}

// @early-stop
// dual switch jump-table + grid-regalloc + /GX-trylevel wall (the same family as
// ClaimSwitchTile in this TU). Logic/CFG/offsets/flag bits + the compass voice
// records + the board release/claim + both engine calls are reconstructed in
// shape/order. Residue: (1) the move-command + direction switches tail-merge their
// overlapping +-0x20 compass arms in a .text layout no source case-order pins; (2)
// the x/y move coords held across the validity/wall test + the level-board double-
// deref land in a different callee-saved-reg/stack-spill assignment than retail;
// (3) the /GX trylevel transitions for the CString + CToyTileBag temps. Final sweep.
RVA(0x00051c00, 0xc7b)
i32 CGrunt::StepCompassMove() {
    GruntBoard* board = g_gameReg->m_tileGrid;
    i32 x = m_lastTilePxX;
    i32 y = m_lastTilePxY;
    i32 tx = x >> 5;
    i32 ty = y >> 5;
    i32 result = 0;
    i32 moveX = x;
    i32 moveY = y;
    CGruntVoiceRec voice;

    if (s_TileFlags(board, tx, ty) & 0x80) {
        // The current tile carries a move command at field +0x10 (4th dword).
        i32 cmd = ((i32*)board->m_8[ty])[tx * 7 + 4];
        switch (cmd - 0xb) {
            case 0:
            case 4:
                moveY = y - 0x20;
                voice = *(CGruntVoiceRec*)g_voiceS;
                break;
            case 1:
            case 5:
                moveY = y + 0x20;
                voice = *(CGruntVoiceRec*)g_voiceN;
                break;
            case 2:
            case 6:
                moveX = x - 0x20;
                voice = *(CGruntVoiceRec*)g_voiceW;
                break;
            case 3:
            case 7:
                moveX = x + 0x20;
                voice = *(CGruntVoiceRec*)g_voiceE;
                break;
            case 8:
                switch (m_entranceCell.reason - 1) {
                    case 0:
                        moveY = y - 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceS;
                        break;
                    case 1:
                        moveX = x + 0x20;
                        moveY = y - 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceNE;
                        break;
                    case 2:
                        moveX = x + 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceE;
                        break;
                    case 3:
                        moveY = y + 0x20;
                        moveX = x + 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceSE;
                        break;
                    case 4:
                        moveY = y + 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceN;
                        break;
                    case 5:
                        moveY = y + 0x20;
                        moveX = x - 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceSW;
                        break;
                    case 6:
                        moveX = x - 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceW;
                        break;
                    case 7:
                        moveX = x - 0x20;
                        moveY = y - 0x20;
                        voice = *(CGruntVoiceRec*)g_voiceNW;
                        break;
                }
                break;
        }
        i32 mtx = moveX >> 5;
        i32 mty = moveY >> 5;
        i32 tflags = s_TileFlags(board, mtx, mty);
        if ((tflags & 0x20000000) && !(tflags & 0x80)) {
            // The target is occupied by another owner: notify the tile mgr (the tile's
            // +0x4 owner id is split into its low two bytes).
            i32 owner;
            if ((u32)mtx >= (u32)board->m_c || (u32)mty >= (u32)board->m_10) {
                owner = -1;
            } else {
                owner = ((i32*)board->m_8[mty])[mtx * 7 + 1];
            }
            ((CTriggerMgr*)m_tileMgr)
                ->CellDispatch((owner >> 8) & 0xff, owner & 0xff, 2, m_tileOwnerHi);
        }
        goto commit;
    }

    // The current tile is a plain walkable tile.
    if (m_toyTileIndex != 0) {
        CString str;
        switch (m_entranceReason - 0x17) {
            case 0:
                str = s_BABYWALKERGRUNT;
                break;
            case 2:
                str = s_BIGWHEELGRUNT;
                break;
            case 3:
                str = s_GOKARTGRUNT;
                break;
            case 6:
                str = s_POGOSTICKGRUNT;
                break;
            default:
                break;
        }
        i32 toyCount = g_buteMgr.GetIntDef((char*)(LPCTSTR)str, s_ToyTiles, 1);
        if (m_toyTileIndex < toyCount) {
            switch (m_entranceCell.reason - 1) {
                case 0:
                    moveY = y - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceS;
                    break;
                case 1:
                    moveY = y - 0x20;
                    moveX = x + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceNE;
                    break;
                case 2:
                    moveX = x + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceE;
                    break;
                case 3:
                    moveY = y + 0x20;
                    moveX = x + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceSE;
                    break;
                case 4:
                    moveY = y + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceN;
                    break;
                case 5:
                    moveY = y + 0x20;
                    moveX = x - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceSW;
                    break;
                case 6:
                    moveX = x - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceW;
                    break;
                case 7:
                    moveX = x - 0x20;
                    moveY = y - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceNW;
                    break;
            }
            result = s_CanCommitMove(this, moveX, moveY);
            if (result == 0) {
                m_toyTileIndex = 0;
            }
        } else {
            m_toyTileIndex = 0;
        }
    }
    if (result != 0) {
        goto commit;
    }

    // The toy-tile bag: random-pick each of the 8 compass directions in turn and
    // commit the first that validates.
    {
        ::CByteArray bag;
        bag.SetAtGrow(bag.GetSize(), 1);
        bag.SetAtGrow(bag.GetSize(), 2);
        bag.SetAtGrow(bag.GetSize(), 3);
        bag.SetAtGrow(bag.GetSize(), 4);
        bag.SetAtGrow(bag.GetSize(), 5);
        bag.SetAtGrow(bag.GetSize(), 6);
        bag.SetAtGrow(bag.GetSize(), 7);
        bag.SetAtGrow(bag.GetSize(), 8);
        while (bag.GetSize() > 0) {
            i32 idx = GruntRand() % bag.GetSize();
            i32 dir = bag.GetAt(idx);
            moveX = x;
            moveY = y;
            switch (dir - 1) {
                case 0:
                    moveY = y - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceS;
                    break;
                case 1:
                    moveX = x + 0x20;
                    moveY = y - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceNE;
                    break;
                case 2:
                    moveX = x + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceE;
                    break;
                case 3:
                    moveX = x + 0x20;
                    moveY = y + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceSE;
                    break;
                case 4:
                    moveY = y + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceN;
                    break;
                case 5:
                    moveX = x - 0x20;
                    moveY = y + 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceSW;
                    break;
                case 6:
                    moveX = x - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceW;
                    break;
                case 7:
                    moveY = y - 0x20;
                    moveX = x - 0x20;
                    voice = *(CGruntVoiceRec*)g_voiceNW;
                    break;
            }
            result = s_CanCommitMove(this, moveX, moveY);
            bag.RemoveAt(idx, 1);
            if (result != 0) {
                break;
            }
        }
    }
    if (result == 0) {
        return 0;
    }

commit:
    ((CTriggerMgr*)m_tileMgr)->ApplySwitch(this, m_lastTilePxX, m_lastTilePxY); // real 0x6d300 (ex ApplyTileSwitch alias)
    PlaySound(0x3e8, voice);
    m_commitPxX = m_lastTilePxX;
    m_commitPxY = m_lastTilePxY;
    {
        GruntBoard* b = g_gameReg->m_tileGrid;
        i32 ox = m_lastTilePxX >> 5;
        i32 oy = m_lastTilePxY >> 5;
        b->m_8[oy][ox * 7 * 4 + 3] &= 0xdf;
        *(i32*)&b->m_8[oy][ox * 7 * 4 + 4] = -1;
    }
    {
        GruntBoard* b = g_gameReg->m_tileGrid;
        i32 nx = moveX >> 5;
        i32 ny = moveY >> 5;
        i32 owner = (m_tileOwnerHi << 8) | m_tileOwnerLo;
        b->m_8[ny][nx * 7 * 4 + 3] |= 0x20;
        *(i32*)&b->m_8[ny][nx * 7 * 4 + 4] = owner;
    }
    m_lastTilePxX = moveX;
    m_lastTilePxY = moveY;
    ComputeFacing(1.0);
    m_arrivalPending = 1;
    m_toyTileIndex += 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ClaimSwitchTile()   @0x52c70   (__thiscall, ret 0)
// Pick a neighbour tile by the entrance-cell direction code (m_entranceCell.reason,
// 1..8 -> the 8 compass deltas; anything else keeps the current tile), test the
// level board's occupancy flags there; if the tile is clear of the blocking bits
// (0x20000939 / 0x80), apply the tile switch (tile mgr), move the occupancy record
// from the old tile to the new one (clear bit 5 of the old tile's flag byte, stamp
// the new tile's owner = (ownerHi<<8)|ownerLo + set bit 5), re-anchor the grunt to
// the new pixel pos, recompute the facing (ComputeFacing(1.0)), latch
// m_arrivalPending=1, and return 1. On an obstructed tile return 0.
//
// @early-stop
// switch jump-table + grid-regalloc wall (docs/patterns: switch-cases-source-order,
// align-down-byte-and-encoding, the regalloc family): logic/CFG/offsets/flag bits +
// both engine calls byte-exact; residue = (1) the 8-way direction switch tail-merges
// the overlapping +-0x20 delta arms in a .text layout no source case-order pins, and
// (2) the x/y move-coords held across both calls + the level-board double-deref
// (g_gameReg->m_tileGrid->m_8[ty]) land in a different callee-saved-reg/stack-spill
// assignment than retail (retail spills the pre-switch x/y to [esp+0x18/0x1c] for the
// default arm and keeps x=ebx/y=edi). The memory-RMW byte twiddle is matched
// (55.8%->61.6%); the rest is the documented regalloc/scheduling plateau. Final sweep.
RVA(0x00052c70, 0x1b1)
i32 CGrunt::ClaimSwitchTile() {
    i32 x = m_lastTilePxX;
    i32 y = m_lastTilePxY;
    switch (m_entranceCell.reason - 1) {
        case 0:
            y -= 0x20;
            break;
        case 1:
            x += 0x20;
            y -= 0x20;
            break;
        case 2:
            x += 0x20;
            break;
        case 3:
            x += 0x20;
            y += 0x20;
            break;
        case 4:
            y += 0x20;
            break;
        case 5:
            x -= 0x20;
            y += 0x20;
            break;
        case 6:
            x -= 0x20;
            break;
        case 7:
            x -= 0x20;
            y -= 0x20;
            break;
        default:
            break;
    }

    GruntBoard* b = g_gameReg->m_tileGrid;
    i32 tx = x >> 5;
    i32 ty = y >> 5;
    i32 flags;
    if ((u32)tx >= (u32)b->m_c || (u32)ty >= (u32)b->m_10) {
        flags = 1;
    } else {
        flags = ((i32*)b->m_8[ty])[tx * 7];
    }
    if ((flags & 0x20000939) || (flags & 0x80)) {
        return 0;
    }

    ((CTriggerMgr*)m_tileMgr)->ApplySwitch(this, m_lastTilePxX, m_lastTilePxY); // real 0x6d300 (ex ApplyTileSwitch alias)

    // Release the grunt's old tile: clear bit 5 of the old tile's flag byte, set
    // its owner record to -1.
    m_commitPxX = m_lastTilePxX;
    m_commitPxY = m_lastTilePxY;
    GruntBoard* gb = g_gameReg->m_tileGrid;
    i32 oldTx = m_lastTilePxX >> 5;
    i32 oldTy = m_lastTilePxY >> 5;
    gb->m_8[oldTy][oldTx * 7 * 4 + 3] &= 0xdf;
    *(i32*)&gb->m_8[oldTy][oldTx * 7 * 4 + 4] = -1;

    // Claim the new tile: set bit 5 of its flag byte, stamp the owner id.
    i32 owner = (m_tileOwnerHi << 8) | m_tileOwnerLo;
    gb->m_8[ty][tx * 7 * 4 + 3] |= 0x20;
    *(i32*)&gb->m_8[ty][tx * 7 * 4 + 4] = owner;

    m_lastTilePxX = x;
    m_lastTilePxY = y;
    ComputeFacing(1.0);
    m_arrivalPending = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::SetArrivalTarget(a, b, c, d)   @0x52ed0   (__thiscall, ret 0x10)
// Seed the arrival/defender block: m_arrivalCol=a, m_arrivalRow=b, m_arrivalActive=1, and the two defender
// pixel coords m_defenderX/Y = (c/d aligned down to the tile grid) + 0x10.
//
// @early-stop
// leaf regalloc / align-down-encoding coin-flip (docs/patterns/align-down-byte-and-
// encoding + the regalloc family): all 5 member stores + values + `ret 0x10` exact.
// Residue = retail keeps `c` in edx (dword `and edx,~0x1f`) and materializes the
// constant 1 in eax (`mov eax,1; mov [m_arrivalActive],eax`) interleaved into the c-block;
// our cl loads c into eax (byte `and al,0xe0`) + stores m_arrivalActive as an immediate. No
// source spelling pins which value owns edx vs eax on a 66-byte leaf. Final sweep.
RVA(0x00052ed0, 0x42)
void CGrunt::SetArrivalTarget(i32 a, i32 b, i32 c, i32 d) {
    m_arrivalCol = a;
    m_arrivalRow = b;
    m_arrivalActive = 1;
    m_defenderX = (c & ~0x1f) + 0x10;
    m_defenderY = (d & ~0x1f) + 0x10;
}

// ---------------------------------------------------------------------------
// CGrunt::ConsiderArrival(a)   @0x52f40   (__thiscall, ret 4)
// If the grunt's HUD point (aligned to the tile grid + 0x10) does not already sit
// on its last occupied tile, ask the drop-ready predicate whether to defer; when it
// is NOT ready, snap to the last tile (SnapToLastTile(a)). When the HUD point IS on
// the last tile, snap unconditionally. Modeled void: retail never sets eax on the
// tail path (no `xor eax,eax`), so the slot is morally void.
//
// @early-stop
// leaf regalloc/schedule coin-flip (the same tiny-accessor family as GetTilePos):
// logic/CFG/offsets exact + the tail (no eax write) byte-matched. Residue = retail
// pins the arg `a` in ebx (3 callee-saved: ebx/esi/edi, arg spilled up-front) and
// loads m_5c->eax/m_60->ecx (m_5c aligns first); our cl uses 2 callee-saved and
// reverses the eax/ecx axis assignment. Source-invariant on a 75-byte leaf. ~84%.
RVA(0x00052f40, 0x4b)
void CGrunt::ConsiderArrival(i32 a) {
    CGruntHud* h = m_10;
    i32 px = (h->m_5c & ~0x1f) + 0x10;
    i32 py = (h->m_60 & ~0x1f) + 0x10;
    if (px != m_lastTilePxX || py != m_lastTilePxY) {
        if (IsDropReady(a)) {
            return;
        }
    }
    SnapToLastTile(a);
}

// ---------------------------------------------------------------------------
// CGrunt::StepAnimDispatchA(x, y, c, d)   @0x52fb0   (ret 0x10)
// @early-stop
// ebp-zero-pin regalloc wall (42.4%, was 39.9%). STRUCTURE now confirmed exact:
// GruntTileFlags is __inline (matches the inlined grid-walk), and the "J" arm's
// GruntEntranceCell by-value copy (dead-spilling `reason` to esp+0x1c) reproduces
// the `sub esp,0xc` frame + the GetBuffer(0)/CacheFirstFrame calls. Every remaining
// diff is register-COLORING: retail pins 0 in EBP (no 8-bit subreg in 32-bit x86 ->
// all 12 strcmp arms null-test `test cl,cl`, and a 4th callee-saved reg is pushed with
// `this` in esi); my body pins 0 in EBX (has bl -> `cmp cl,bl`, this in edi, only 3
// pushes). Same offsets throughout, no wrong field/dispatch. Which physical reg holds
// the zero is not source-steerable (permuter SKILL.md excludes register-coloring); the
// standalone-repro proof in StepArrivalCommit's note applies verbatim. Final sweep.
RVA(0x00052fb0, 0x96e)
i32 CGrunt::StepAnimDispatchA(i32 x, i32 y, i32 c, i32 d) {
    if (m_entranceCommitted == 0) {
        return 1;
    }
    i32 flags = GruntTileFlags(x, y);
    if ((flags & 0xd39) || (flags & 0x82)) {
        return 0;
    }

    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeA) == 0);
    if (eq) {
        goto applyTail;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeD) == 0);
    if (eq) {
        goto applyTail;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeI) == 0);
    if (eq) {
        // code "I": arrival cue (m_170==0x13) then re-notify the tile mgr.
        if (m_entranceReason == 0x13) {
            EmitMoveCueShort(m_10->m_188, 0, 0);
        }
        m_tileMgr->ArrivalNotify6(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            -1
        );
        if (m_entranceReason != 1) {
            goto applyTail;
        }
        m_tileMgr->SetTileState4(m_tileOwnerHi, m_tileOwnerLo, 1, -1);
        goto applyTail;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeG) == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeL) == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeP) == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeO) == 0);
    if (eq) {
        // code "O": commit the move directly.
        ApplySetState1(1);
        CommitMoveA(m_lastTilePxY, m_lastTilePxX, 0);
        goto applyTail;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeQ) == 0);
    if (eq) {
        return 1;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeJ) == 0);
    if (eq) {
        // code "J": clear the entrance gate, re-latch a fresh anim set, drive the
        // geometry sub-player.
        m_entranceActive = 0;
        if (m_poweredUp == 0 && m_neighborValid == 0) {
            m_entranceCommitted = 0;
            ReseedIdleReset(1, 0, 0);
        }
        m_35c = 0;
        m_prevAnimSetNode = m_14->m_1c;
        m_14->m_1c = (void*)EntranceLookupAnimSet(s_codeD);
        m_prevEntranceDesc = m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseWalk);
        // Stamp the first entrance-cell frame from m_cells[base].m_walk. The by-value
        // cell copy dead-spills `reason` (esp+0x1c) -> `sub esp,0xc`; base = 3*col+row.
        GruntEntranceCell cell = m_entranceCell;
        i32 col = cell.row + cell.col * 2;
        i32 base = cell.col + col;
        char* nm = m_cells[base].m_walk.GetBuffer(0);
        m_154->CacheFirstFrame(nm);
        goto modeDispatch;
    } else {
        ApplySetState1(1);
        goto modeDispatch;
    }

idleReseed:
    // codes G/L/P: drive the move state by m_19c and (m_170==0x1e) fire the cue.
    if (m_entranceReason == 0x1e) {
        EmitMoveCueShort(m_10->m_188, 0, 0);
    }
    SetMoveStateA(m_19c, 1, 0, 1);
    {
        i32 px = m_10->m_60 + 0x186a0;
        if (m_10->m_74 != px) {
            m_10->m_74 = px;
            m_10->m_8 |= 0x20000;
        }
    }
    if (m_toyTimeSprite != 0) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    m_toyTime = 0;
    StepCoordTick();

applyTail:
    // The shared movement-apply tail: re-set the geometry, recycle coords.
    if (m_wingzEnabled != 0) {
        OnMoveFinishA(0);
    }
    if (m_poweredUp == 0 && m_neighborValid == 0) {
        m_entranceCommitted = 0;
        ReseedIdleReset(1, 0, 0);
    }
    StepDropApply();
    return 1;

modeDispatch: {
    i32 mode = m_moveMode;
    if (mode >= 0x32) {
        SetMoveStateA(mode, 1, 0, 1);
        m_moveMode = -1;
        m_1a4 = 0;
        return 1;
    }
    if (mode >= 0x22) {
        m_194 = mode;
        m_moveMode = -1;
        return 1;
    }
    if (mode >= 0x17) {
        EmitMoveCueQ(mode);
        return 1;
    }
    SetMoveStateA(mode, 1, 0, 1);
    m_moveMode = -1;
    return 1;
}
}

RVA(0x00053b80, 0x340)
i32 CGrunt::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (ar == 0) {
        return 0;
    }
    // chain the base-class serialize on `this` (0x16e7f0 = CMovingLogicBase::Serialize)
    if (((CMovingLogicBase*)this)->Serialize((CSerialArchive*)ar, mode, a3, a4) == 0) {
        return 0;
    }
    // then the +0x150 CSerialObjRef's chain (0x8c00 via the 0x1aff thunk)
    if (((CSerialObjRef*)(&m_150))->Chain((CSerialArchive*)ar, mode, a3, (CSerialObj*)a4) == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            // mode-4 save path = CGrunt::Save (0x53f90)
            if (Save(ar) == 0) {
                return 0;
            }
            break;
        case 7:
            // mode-7 load path (0x555e0; labeled CGameStateRecord::Load in gamestaterecordload,
            // really CGrunt's load facet on the same `this` layout - attribution TODO there)
            if (((CGameStateRecord*)this)->Load((CSerialArchive*)ar) == 0) {
                return 0;
            }
            break;
        case 8:
            m_tileMgr = (CGruntTileMgr*)g_gameReg->m_68;
            break;
    }
    ((CTriRecord*)(&m_entranceCell))->Serialize((CSerialArchive*)ar, mode, a3, a4);
    SerRecord(ar, mode, (char*)&m_toyClockLo);
    SerRecord(ar, mode, (char*)&m_idleAnchorLo);
    SerRecord(ar, mode, (char*)&m_idleTimerLo);
    SerRecord(ar, mode, (char*)&m_entranceClockLo);
    SerRecord(ar, mode, (char*)&m_850);
    SerRecord(ar, mode, (char*)&m_860);
    SerRecord(ar, mode, (char*)&m_combatClockLo);
    SerRecord(ar, mode, (char*)&m_880);
    ((CPairRecord*)(&m_wingzClockLo))->Serialize((CSerialArchive*)ar, mode, a3, a4);
    ((CPairRecord*)(&m_8a0))->Serialize((CSerialArchive*)ar, mode, a3, a4);
    ((CPairRecord*)(&m_8b0))->Serialize((CSerialArchive*)ar, mode, a3, a4);
    ((CPairRecord*)(&m_8c0))->Serialize((CSerialArchive*)ar, mode, a3, a4);
    ((CPairRecord*)(&m_arrivalRerollLo))->Serialize((CSerialArchive*)ar, mode, a3, a4);
    ((CPairRecord*)(&m_278))->Serialize((CSerialArchive*)ar, mode, a3, a4);
    return 1;
}

// @early-stop
// reloc-masked-extern tail (94%+): the 4560-byte instruction stream is
// byte-exact vs retail (no EH frame, same sprite/string/name-id/field/tail
// blocks, verified llvm-objdump) - the residual is the ~35 unnamed call operands
// in the 18 name-id + 3 string blocks (catalog LookupName + ~CString) pairing to
// differently named retail symbols. Naming the whole referent set -> exact is a
// final-sweep task.
// ---------------------------------------------------------------------------
// CGrunt::Save(ar) @0x53f90 - serializes the whole grunt state into a custom
// archive (each member -> ar->Write(&field, size) via vtable slot 0x30). Bails
// (return 0) if the archive is null or the type catalog (m_158->m_c) is unset.
// The 4560-byte body is, in order: 7 sprite-id blocks (each bumps the global
// serialize counter and writes the sprite's m_188, or 0 if the slot is empty);
// 3 name strings (a 0x80-byte buffer copy); 18 anim-name-id blocks (look the id
// up in the catalog's name map and copy the resolved name into the buffer); then
// ~100 plain field writes; finally a linked-list tail (m_33c) writing each
// node's +0x8 (size 0x2c). The serialize counter is the global DAT_00629ad0.
RVA(0x00053f90, 0x11d0)
i32 CGrunt::Save(CGruntArchive* ar) {
    if (!ar) {
        return 0;
    }
    CDDrawSubMgrLeaf* catalog = ((CGruntTypeCatalog*)*(void**)&m_158)->m_c;
    if (!catalog) {
        return 0;
    }
    i32 tmp;
    char buf[0x80];
    g_serialCounter++;
    tmp = 0;
    {
        CGameObject* sp = m_selectedSprite;
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    tmp = 0;
    {
        CGameObject* sp = m_toySprite;
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    tmp = 0;
    {
        CGameObject* sp = m_healthSprite;
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    tmp = 0;
    {
        CGameObject* sp = m_staminaSprite;
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    tmp = 0;
    {
        CGameObject* sp = m_toyTimeSprite;
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    tmp = 0;
    {
        CGameObject* sp = m_wingzTimeSprite;
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    tmp = 0;
    {
        CGameObject* sp = m_powerupSprite;
        if (sp) {
            tmp = *(i32*)((char*)sp + 0x188);
        }
    }
    ar->Write(&tmp, 4);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    strcpy(buf, (const char*)m_animSetName);
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    strcpy(buf, *(const char**)&m_448);
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    strcpy(buf, *(const char**)&m_44c);
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseWalk;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseAttack1;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseAttack2;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseAttackIdle;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseStruck1;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseStruck2;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseIdle[0];
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseIdle[1];
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseIdle[2];
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseIdle4;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseIdle5;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseDeath;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseToy1;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseToy2;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseToyBreak;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseItem;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_poseItem2;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    g_serialCounter++;
    memset(buf, 0, 0x80);
    {
        i32 id = m_pickupGeoSrc;
        if (id) {
            CString nm = catalog->KeyOfValue_152d30((CObject*)id);
            strcpy(buf, nm);
        }
    }
    ar->Write(buf, 0x80);
    ar->Write(&m_18c, 4);
    ar->Write(&m_toyBlendPct, 4);
    ar->Write(&m_194, 4);
    ar->Write(&m_entranceReason, 4);
    ar->Write(&m_198, 4);
    ar->Write(&m_19c, 4);
    ar->Write(&m_moveMode, 4);
    ar->Write(&m_1a4, 4);
    ar->Write(&m_1a8, 4);
    ar->Write(&m_1ac, 4);
    ar->Write(&m_1b0, 4);
    ar->Write(&m_1b4, 4);
    ar->Write(&m_arrived, 4);
    ar->Write(&m_entrancePxX, 8);
    ar->Write(&m_lastTilePxX, 8);
    ar->Write(&m_commitPxX, 8);
    ar->Write(&m_1dc, 8);
    ar->Write(&m_entranceActive, 4);
    ar->Write(&m_arrivalPending, 4);
    ar->Write(&m_tileOwnerHi, 4);
    ar->Write(&m_tileOwnerLo, 4);
    ar->Write(&m_1f4_moveIcon, 4);
    ar->Write(&m_1f8, 4);
    ar->Write(&m_entranceCommitted, 4);
    ar->Write(&m_neighborCol, 8);
    ar->Write(&m_208, 8);
    ar->Write(&m_210, 4);
    ar->Write(&m_214, 4);
    ar->Write(&m_combatActive, 4);
    ar->Write(&m_neighborValid, 4);
    ar->Write(&m_poweredUp, 4);
    ar->Write(&m_224, 4);
    ar->Write(&m_entranceStamped, 4);
    ar->Write(&m_22c, 4);
    ar->Write(&m_arrivalActive, 4);
    ar->Write(&m_reachRectLeft, 16);
    ar->Write(&m_2a0, 16);
    ar->Write(&m_2b0, 16);
    ar->Write(&m_2c0, 16);
    ar->Write(&m_health, 4);
    ar->Write(&m_stamina, 4);
    ar->Write(&m_toyTime, 4);
    ar->Write(&m_wingzTime, 4);
    ar->Write(
        (char*)this + 0x400,
        8
    ); // m_400 double (raw: converting shifts a neighbor's regalloc)
    ar->Write(&m_418, 4);
    ar->Write(&m_42c, 4);
    ar->Write(&m_430, 4);
    ar->Write(&m_434, 4);
    ar->Write(&m_438, 4);
    ar->Write(&m_arrivalState, 4);
    ar->Write(&m_defenderState, 4);
    ar->Write(&m_2d8, 4);
    ar->Write(&m_defenderRadius, 4);
    ar->Write(&m_2e0, 4);
    ar->Write(&m_2e4, 4);
    ar->Write(&m_dwell, 4);
    ar->Write(&m_arrivalCol, 8);
    ar->Write(&m_defenderX, 8);
    ar->Write(&m_354, 4);
    ar->Write(&m_358, 4);
    ar->Write(&m_35c, 4);
    ar->Write(&m_3dc, 8);
    ar->Write(&m_moveTileX, 8);
    ar->Write(&m_arrivalPhase, 4);
    ar->Write(&m_timePerTile, 4);
    ar->Write((char*)this + 0x408, 8); // m_408 double (raw: see m_400 note)
    ar->Write((char*)this + 0x410, 8); // m_410 double (raw: see m_400 note)
    ar->Write(&m_8d0, 4);
    ar->Write(&m_coordToggle, 4);
    ar->Write(&m_wingzEnabled, 4);
    ar->Write(&m_freezeDelayDone, 4);
    ar->Write(&m_freezeUnfrozen, 4);
    ar->Write(&m_resetApplied, 4);
    ar->Write(&m_arrivalFlags, 4);
    ar->Write(&m_24c, 4);
    ar->Write(&m_gruntKind, 4);
    ar->Write(&m_entranceArmed, 4);
    ar->Write(&m_deathType, 4);
    ar->Write(&m_entranceDropActive, 4);
    ar->Write(&m_318, 4);
    ar->Write(&m_2f8, 8);
    ar->Write(&m_36c, 4);
    ar->Write(&m_454, 4);
    ar->Write(&m_370, 4);
    ar->Write(&m_tileClaimed, 4);
    ar->Write(&m_deathAnimStarted, 4);
    ar->Write(&m_458, 8);
    ar->Write(&m_250, 4);
    ar->Write(&m_254, 4);
    ar->Write(&m_374, 4);
    ar->Write(&m_moveKind, 4);
    ar->Write(&m_moveVariant, 4);
    ar->Write(&m_coordRetryCount, 4);
    ar->Write(&m_toyTileIndex, 4);
    ar->Write(&m_390, 4);
    ar->Write(&m_378, 4);
    ar->Write(&m_38c, 4);
    ar->Write(&m_lowStaminaCued, 4);
    ar->Write(&m_2e8, 4);
    ar->Write(&m_288, 8);

    for (CGruntListNode* node = PayloadHead(); node; node = node->m_next) {
        ar->Write(node->m_data, 0x2c);
    }
    return 1;
}

SIZE_UNKNOWN(CGruntAnchor);
SIZE_UNKNOWN(CGruntCmdObj);
SIZE_UNKNOWN(CGruntRegistrar);
SIZE_UNKNOWN(CSpriteSetReg);
SIZE_UNKNOWN(CTileCell);
