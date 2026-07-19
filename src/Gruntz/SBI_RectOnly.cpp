// Emit the retail INLINE SBI base-dtor bodies (stamp vftable + DtorStatus/ClearFrame/...)
// so this /GX host TU's ??1/??_GCStatusBarItem match retail + the SBI leaf TUs instead of
// the declared-only empty form that diverged across objs. Must precede any SBI include.
#define SBI_DTOR_CHAIN
#include <Mfc.h>                 // afx-first umbrella (CByteArray/CPtrList consumers below)
#include <Gruntz/GameRegMfcPtr.h>
#include <Io/FileMem.h>          // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/StatusBarMgr.h> // canonical CStatusBarMgr (the 0x630 host) + referent views
#include <Gruntz/StatusBarTabWidgets.h> // the tab-widget leaves this TU's builders `new`
#include <Gruntz/LevelSync.h>           // CLevelSync + its referents
#include <DDrawMgr/DDrawChildGroup.h>   // real CDDrawChildGroup::CreateSprite (0x1597b0); 0x104dd0
#include <Gruntz/WarpStoneFly.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Rez/RezList.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/StreamFeeder.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_c->m_imageRegistry->m_10map (LoadMainStatusBarSprite)
#include <Gruntz/SBI_GruntMachine.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/LeafCue.h>
// The g_gameReg spine slots are the REAL classes: the current play-state (CPlay), the
// single-player trigger grid (CTriggerMgr) and the registry writer (RegistryHelper).
#include <Gruntz/Play.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Grunt.h>     // real CGrunt (the m_grid cells; ex CSbiTileEntry/CSbiTileSub views)
#include <Gruntz/GruntzMgr.h> // the REAL *0x24556c singleton class (ReportError @0x08dc60)
#include <Gruntz/GameRegistry.h> // CDDrawSurfaceMgr (m_world def; +0x28 CSndHost)
#include <Gruntz/StatusBarMgr.h> // CStatusBarMgr::LoadTabSprites @0x102250 (SetTab's real callee)
#include <Utils/RegistryHelper.h>
#include <Globals.h>
// One-TU merge (interval dossier 0x104d60-0x10bc14): this TU absorbed
// StatusBarUpdaters' five in-interval updaters, WarpStoneFly.cpp, SBI_SideTabBuild.cpp
// (CStatzTabBuilder::Build), LevelSync.cpp (CLevelSync::Sync), MgrSettings.cpp
// (CMgrSettings::Serialize) and SBI_TabzDialogEh.cpp (CTabzBuilder::BuildTabzDialog) -
// the updater family / warpstone fly / serialize cluster are one original /GX obj
// (single sbi_rectonly init-frag region; ??0CWarpStoneFly abuts UpdateWarpStoneStatusBar).
#include <Gruntz/StatusBarUpdatersViews.h> // EngineLabelBacklog host + updater referent views
#include <Gruntz/Sprite.h>                 // CSprite (frame-data value) + CSpriteHashTable
#include <Gruntz/SbiSideTabBuildViews.h>   // CSBI_SideTab (ctor view) + CStatzTabBuilder
#include <Gruntz/MgrSettings.h>            // CMgrSettings + the g_gameReg/g_serialCounter externs
#include <Rez/RezMgr.h>                    // RezFree (the per-frame warpstone overlay free)
#include <math.h>   // sqrt - intrinsified to inline fsqrt under VC5 /O2 (warpstone fly)
#include <string.h> // strlen / memset (inlined repne scas / rep stos; CMgrSettings::Serialize)
// SBI_RectOnly.cpp - Gruntz CStatusBarMgr (C:\Proj\Gruntz).
// The constructor is matched byte-exact.
//
// CStatusBarMgr derives from CStatusBarItem. The retail ctor inlines the base
// ctor (zeroing m_4/m_24/m_28; the base's m_8=0 store is dropped as dead because
// the derived ctor sets m_8=1), then stores its own vptr, then m_8=1. That fold
// is exactly why CStatusBarItem's ctor is inline in the shared header (MSVC 5.0
// will not fold an out-of-line base ctor).

// CStatusBarMgr + all its engine-referent views (CSbiSlot/CSbiRect/...) and the
// slot-state enum/consts now live in the canonical shared header <Gruntz/StatusBarMgr.h>
// (included above); the serialize stream is the shared CSerialArchive. Only the
// RVA-keyed method bodies and the DATA()-bound globals remain in this TU.

// The running game clock (RezMgr's g_accumMs) read by the HUD-rect group setters.
// This TU used to declare the SAME cell twice - here as a C++ `g_frameTime` and again
// below as the extern-"C" `g_frameTime` - i.e. two symbols for one address, neither with
// storage. One declaration now; it is DEFINED in Projectile.cpp.

// The current local-player / area index (PlaceCursorTarget's tile-grid column).
// DEFINED HERE - this TU already held the canonical binding and 11 TUs referenced it
// with none defining it. .bss, zero-init.
DATA(0x00244c54)
extern "C" {
    i32 g_curPlayer = 0;
}

// CMapStringToOb/CSbiCueRecord/DSoundCloneInst/CSbiMusicHost/CSbiGameMgr/CSbiSubMgr/
// CSbiTile*/CSbiActiveObj/CSbiLogger/CSbiWndHost/CGameReg moved to
// <Gruntz/StatusBarMgr.h>.
// 0x24556c: the game-mgr singleton, typed as the REAL class (CGruntzMgr, the RTTI-true
// owner of the object `new`'d at 0x080a20 with size 0xa30) rather than the Win32-safe
// CGameRegistry view. This TU is already MFC (<Mfc.h> above), so it can see the real
// class -- and it MUST: the three ReportError call sites below are CGruntzMgr::
// ReportError @0x08dc60, and going through the view emitted a call to
// ?ReportError@CGameRegistry@@QAEXHH@Z, a symbol NOTHING defines (an unbound reloc that
// would fail at link). Every member this TU reads off the singleton (m_curState/m_world/
// m_cmdGrid/m_settings/m_134/m_modeW/m_modeH/m_soundVolume + the CGameMgr base's
// m_gameWnd/m_frameGate/m_soundEnabled) exists on CGruntzMgr at the same offsets.
// Its canonical symbol is _g_mgrSettings (extern-C, bound via DATA in userlogic);
// referencing it by that name pairs the DIR32 relocs.

// The reentrancy gate + cue-item id pair the highlight handlers play through.
// The draw-clock mirror (g_killCueClock), unsigned for the wrap-safe gate compare.

// Global serialize-sequence counter (bumped once per Serialize).

// The engine free-list head + the node-pointer bias (raw subtrahend), shared with
// Projectile/TriggerMgr. The teardown returns each pooled element to this list.
#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// The pool's INTERIOR FIELDS - m_freeHead (+0x04) and m_linkOffset (+0x0c) are
// fields of g_coordPool (DEFINED in src/Gruntz/GameText.cpp), which is
// why the free-list push/pop code reads exactly [pool+4] and [pool+0xc].

// ---------------------------------------------------------------------------
// The /GX members collapsed from SBI_RectOnlyEh.cpp (the split companion TU was
// our invention; retail's one TU was compiled /GX - EH sites inside its interval).

// The MSVC 'eh vector destructor iterator' runtime (0x51f640): runs `dtor` over
// `count` elements of `stride` from `base`, descending. Reloc-masked rel32 callee.
void Tm_DestroyArray(void* base, i32 stride, i32 count, void* dtor); // 0x11f640

// The per-element list dtor (~CPtrList, aliased ~CInternetSession @0x5b48c6) passed to
// the vector-destroy iterator for the eight +0x2c notify lists.
void SbiList_Dtor(); // 0x5b48c6

// 0xc8980: CStatusBarMgr member teardown - the /GX dtor body that drains the pooled
// state (Teardown), destructs the +0x530 CByteArray, then runs the eh-vector-destroy
// iterator over the eight +0x2c notify lists (stride 0x1c, ~CPtrList per element). The
// three teardown stages each carry their own descending /GX trylevel.
// @early-stop
// ~49% /GX member-array dtor wall (same family as ~CTriggerMgr 0x85c50): the body is
// byte-correct - Teardown call, lea [this+0x530] + ~CByteArray,
// and the four-arg eh-vector-destroy push sequence (push &dtor / 8 / add esi,0x2c / 0x1c /
// push base) all match retail. But the whole /GX SEH frame (push -1 / push handler / mov
// fs:0,esp) and the descending [esp+0x10]=1/0/-1 trylevel stamps are MISSING: MSVC only
// emits them for a real `~Class()` whose VALUE members have non-trivial dtors. 0xc8980 is
// a standalone teardown HELPER. 0xc8980 IS the out-of-line COMDAT copy of the real
// ~CStatusBarMgr, which now exists in <Gruntz/StatusBarMgr.h> as `{ Teardown(); }` plus
// the compiler-generated member teardown - PROVEN by CPlay::LoadGameAssetNamespaces's fail path at 0xc82b6,
// where `delete` INLINES exactly this sequence (Teardown / ~CPtrArray on m_ptrPool /
// __ehvec_dtor over m_tabLists[8]) under /GX states 3/2. So the fix is known: spell this
// AS the destructor and the frame + trylevels fall out.
// IT WAS DELIBERATELY NOT TAKEN, because it is a TWO-SITE codegen trade, not a free win:
//   * CPlayDtorBody (0xc8700, ~99%) calls this teardown OUT-OF-LINE from a frameless
//     context (call 0x1438 -> here). Re-pointing that call at an in-class inline dtor
//     invites MSVC5 to inline this ~100-byte body INTO CPlayDtorBody - cratering a 99%
//     function to buy this one.
//   * Emitting the dtor out-of-line while `delete` ALSO inlines it at 0xc82b6 is exactly
//     what retail does, but which MSVC5 picks is driven by the inlining budget of
//     whichever TU instantiates it first - not steerable from this file alone.
// It is ONE decision across both sites, so it belongs to the recovery pass - with this
// note, not a re-derivation. Documented wall (docs/patterns/
// eh-dtor-model-members-as-destructible.md).
RVA(0x000c8980, 0x64)
void CStatusBarMgr::DtorMembers() {
    Teardown();
    reinterpret_cast<CByteArray*>(&m_ptrPool)
        ->CByteArray::~CByteArray(); // +0x530 real CByteArray teardown
    Tm_DestroyArray(m_tabLists, 0x1c, 8, static_cast<void*>(&SbiList_Dtor));
}

// ~CStatusBarItem is the SBI_DTOR_CHAIN inline body (stamp vftable + DtorStatus) now, so
// this TU's ??1/??_GCStatusBarItem match retail + the SBI leaf TUs.
// (The fabricated `CStatusBarItem::SbiVfunc0 { return 0; }` vftable anchor that stood
// here is GONE - slot 1 is the real 4-arg SerializeFields (0x10bfc0, SBI_MenuItem.cpp).
// It was a second definition of the same base slot; the inline dtor above already
// references ??_7, which is what emits the COMDAT.)

// CStatusBarItem base default slots 6..9 (0x100530/0x100550/0x100570/0x100590):
// each is `xor eax,eax; ret 0xc` - returns 0 for its 3-arg call (no SBI leaf
// overrides them).
RVA(0x00100530, 0x5)
i32 CStatusBarItem::SbiSlot6(i32, i32, i32) {
    return 0;
}
RVA(0x00100550, 0x5)
i32 CStatusBarItem::SbiSlot7(i32, i32, i32) {
    return 0;
}
RVA(0x00100570, 0x5)
i32 CStatusBarItem::SbiSlot8(i32, i32, i32) {
    return 0;
}
RVA(0x00100590, 0x5)
i32 CStatusBarItem::SbiSlot9(i32, i32, i32) {
    return 0;
}

// (0x101fa0 ??0CStatusBarMgr + its slot-1 SbiVfunc0 -> SBI_RectOnlyBase.cpp. They are
// the THIN polymorphic sub-widget's members, not this host's: the ctor stamps
// ??_7CStatusBarMgr@@6B@ and this class has no vtable at all. They lived here only
// because the two classes shared one name before the split.)
// (0xe86e0 Setup -> SBI_RectOnlyBase.cpp (the thin chain CStatusBarMgr's own obj);
// 0xe7400 ResetCounters -> SBI_ImageSet.cpp (vtbl 0x1eac4c slot [3]) - dossier #16.)

// Reset slots 0..4, then m_activeSlot = -1.
RVA(0x00105520, 0x21)
void CStatusBarMgr::ResetSlots() {
    for (i32 i = 0; i < 5; i++) {
        ArmSlot(i);
    }
    m_activeSlot = -1;
}

// Arm slot[idx]: state = armed, value = 1; notify the slot's pointer (vfunc 0x30).
RVA(0x00105560, 0x33)
void CStatusBarMgr::ArmSlot(i32 idx) {
    m_slots[idx].m_state = kSlotArmed;
    m_slots[idx].m_value = 1;
    if (m_slotNotify[idx]) {
        m_slotNotify[idx]->Notify(1);
    }
}

// Probe slots 0..4; return 1 on first hit, else 0.
RVA(0x00105710, 0x23)
i32 CStatusBarMgr::AnySlotActive() {
    for (i32 i = 0; i < 5; i++) {
        if (LoadGooCookingSprite(i)) {
            return 1;
        }
    }
    return 0;
}

// m_gaugeTarget = min(m_gauge + delta, 100).
RVA(0x00105750, 0x1f)
void CStatusBarMgr::AdvanceGauge(i32 delta) {
    i32 v = m_gauge + delta;
    if (v >= 100) {
        v = 100;
    }
    m_gaugeTarget = v;
}

// m_gaugeTarget = m_gauge = value.
RVA(0x001057d0, 0x13)
void CStatusBarMgr::SetGauge(i32 value) {
    m_gaugeTarget = value;
    m_gauge = value;
}

// Place the cursor on the resolved tile under highlight row `row`: probe the active
// object's tile at (g_curPlayer, row); bail (0) if the probe fails or the grid
// cell is empty. Forward the tile's origin pair to the sub-manager's ScrollTo, then
// (when `commit` is set and the active object accepts the scroll) latch the placed
// column/row and reload the camera sprite. Always returns 1 past the two probes.
// @early-stop
// ~71%: the code bytes are byte-exact vs retail (same regs/order/offsets; verified by
// llvm-objdump -dr base vs target). The residual is purely the reloc-symbol-naming
// scoring tail - this TU models the g_gameReg singleton as ?g_gameReg@@3PAUCGameReg@@A
// while the retail obj names it _g_mgrSettings, so the three DIR32 data relocs don't
// pair (weighted heavily on a short function). g_curPlayer + the ILT call thunks already
// pair. A TU-wide g_gameReg rename, not a per-function fix; matcher.md reloc artifact.
RVA(0x00105800, 0x9e)
i32 CStatusBarMgr::PlaceCursorTarget(i32 row, i32 commit) {
    i32 col = g_curPlayer;
    if (g_gameReg->m_cmdGrid->ResetCell(col, row, 0, 0) == 0) {
        return 0;
    }
    // the grid cell is the real CGrunt (CTmCell typedef); its m_10 HUD carries the
    // on-screen origin pair.
    CGrunt* entry = g_gameReg->m_cmdGrid->m_grid[row + col * TM_GRID_COLS];
    if (entry == 0) {
        return 0;
    }
    (static_cast<CPlay*>(g_gameReg->m_curState))->ResetGoals(entry->m_10->m_screenX, entry->m_10->m_screenY);
    if (commit != 0) {
        CTriggerMgr* obj = g_gameReg->m_cmdGrid;
        if (obj->RecordListHas(col, row)) {
            obj->m_recX = col;
            obj->m_recY = row;
            obj->m_armed = 1;
            obj->LoadCameraSprite();
        }
    }
    return 1;
}

// Run the seven per-stat refresh updaters in sequence.
RVA(0x001058d0, 0x34)
void CStatusBarMgr::RefreshAll() {
    UpdateGruntOvenStatusBar();
    TickGauge();
    UpdateRezConveyorStatusBar();
    LoadRezMachineConfig();
    LoadChipMachineConfig();
    UpdateChipGrinderStatusBar();
    UpdateDestructButtonStatusBar();
}

// Clear the gauge, zero m_gauge/m_gaugeTarget, run two updaters, set toggle flags.
RVA(0x00105920, 0x47)
void CStatusBarMgr::Reset() {
    ResetSlots();
    m_gaugeTarget = 0;
    m_gauge = 0;
    ResetGroupA();
    UpdateRezMachineSnoozeStatusBar();
    InitTabRects();
    m_modeState = 1;
    m_destructWarnActive = 0;
}

// Toggle stat[idx]: if already set, clear it; else set it on.
RVA(0x00107aa0, 0x23)
void CStatusBarMgr::ToggleStat(i32 idx) {
    if (m_statFlags[idx]) {
        ClearStat(idx);
    } else {
        LoadStatzTabToggleSprite(idx, 1);
    }
}

// Find the index of the first enabled hit-test rect containing (x,y); -1 none.
// @early-stop
// bool-materialization wall: retail keeps the redundant inner `p->m_enabled` test and
// materializes the point-in-rect predicate via `mov ecx,1 / xor ecx,ecx / test`;
// MSVC5 folds our `&&` chain into direct control flow (~80%). Logic byte-correct.
RVA(0x00105280, 0x61)
i32 CStatusBarMgr::HitTest(i32 x, i32 y) {
    if (m_hitTestDisabled == 0) {
        for (i32 i = 0; i < 15; i++) {
            CSbiRect* p = m_hitRects[i];
            if (p && p->m_enabled) {
                i32 hit =
                    p->m_enabled && x < p->m_xHi && x >= p->m_xLo && y < p->m_yHi && y >= p->m_yLo;
                if (hit) {
                    return i;
                }
            }
        }
    }
    return -1;
}

// Reset the three group-A slots (arm state=0/value=1) and notify each pointer.
RVA(0x00106610, 0x3b)
void CStatusBarMgr::ResetGroupA() {
    for (i32 i = 0; i < 3; i++) {
        m_groupSlots[i].m_state = kSlotArmed;
        m_groupSlots[i].m_value = 1;
        if (m_groupNotify[i]) {
            m_groupNotify[i]->Notify(-1);
        }
    }
}

// Latch HUD-rect group A from three args + a global dword.
// @early-stop
// scheduling wall: logic byte-correct, but MSVC defers the m_hudRectA_x store and
// reorders the m_hudRectA_clockHi/m_hudRectA_clock zero+global pair vs retail; ~72%, not steerable from
// C (see docs/patterns/u64-store-clock-hi-zero.md, statement-schedule-faithful).
RVA(0x001066f0, 0x3b)
void CStatusBarMgr::SetHudRectA(i32 y0, i32 x0, i32 z) {
    m_hudRectA_y = y0;
    m_hudRectA_x = x0;
    m_hudRectA_z = z;
    m_hudRectA_zHi = 0;
    m_hudRectA_clock = g_frameTime;
    m_hudRectA_clockHi = 0;
}

// Latch HUD-rect group B from three args + a global dword.
// @early-stop
// scheduling wall: same store-reorder as SetHudRectA; ~72%.
RVA(0x00106740, 0x3b)
void CStatusBarMgr::SetHudRectB(i32 y0, i32 x0, i32 z) {
    m_hudRectB_y = y0;
    m_hudRectB_x = x0;
    m_hudRectB_z = z;
    m_hudRectB_zHi = 0;
    m_hudRectB_clock = g_frameTime;
    m_hudRectB_clockHi = 0;
}

// Commit the active slot: either re-arm it, or push the cooked commit level and
// notify its pointer; then clear the active-slot index.
RVA(0x00106790, 0x62)
void CStatusBarMgr::CommitSlot(i32 active) {
    if (active) {
        ArmSlot(m_activeSlot);
        m_activeSlot = -1;
    } else {
        m_slots[m_activeSlot].m_value = kSlotCommitLevel;
        if (m_slotNotify[m_activeSlot]) {
            m_slotNotify[m_activeSlot]->Notify(m_slots[m_activeSlot].m_value);
        }
        m_activeSlot = -1;
    }
}

// Clear highlight-grid cell [row,group] (state + handle = 0), then re-notify.
RVA(0x001069c0, 0x2e)
void CStatusBarMgr::ClearHlCell(i32 row, i32 group) {
    i32 idx = group + row * 4;
    m_hlGrid[idx].m_state = 0;
    m_hlGrid[idx].m_handle = 0;
    NotifyAllSlots();
}

// Set highlight-grid cell [row,group] handle if its slot is free; 1 = set, 0 = busy.
// 0x106af0 - map a handle to its highlight-grid tier row (>=0x22 -> 2, >=0x17 -> 1,
// else 0), then arm that cell via SetHlCell.
RVA(0x00106af0, 0x37)
i32 CStatusBarMgr::SetHlCellByTier(i32 handle, i32 group) {
    i32 row;
    if (handle >= 0x22) {
        row = 2;
    } else {
        row = (handle >= 0x17);
    }
    return SetHlCell(row, handle, group);
}

RVA(0x00106b40, 0x44)
i32 CStatusBarMgr::SetHlCell(i32 row, i32 handle, i32 group) {
    i32 idx = group + row * 4;
    if (m_hlGrid[idx].m_state) {
        return 0;
    }
    m_hlGrid[idx].m_handle = handle;
    m_hlGrid[idx].m_state = 1;
    NotifyAllSlots();
    return 1;
}

// Fire every live slot's notifier: the four singletons, the 3x4 group grid
// (each pointer notified with its row's handle), then two trailing singletons.
RVA(0x00106a00, 0xbf)
void CStatusBarMgr::NotifyAllSlots() {
    if (m_notify0) {
        m_notify0->Refresh();
    }
    if (m_notify2) {
        m_notify2->Refresh();
    }
    if (m_notify3) {
        m_notify3->Refresh();
    }
    if (m_extraNotify0 && m_extraNotifyArg0) {
        m_extraNotify0->Notify(m_extraNotifyArg0);
    }

    CSbiSlotPtr** p = &m_hlNotify[4]; // group B base; ±4 elements reach groups A / C
    i32* h = &m_hlGrid[4].m_handle;   // anchor on the handle field (+0x3dc)
    for (i32 n = 0; n < 4; n++) {
        if (p[-4]) {
            p[-4]->Notify(h[-24]); // -4 rows = -0x60 bytes = -24 ints
        }
        if (p[0]) {
            p[0]->Notify(h[0]);
        }
        if (p[4]) {
            p[4]->Notify(h[24]);
        }
        p++;
        h += 6; // one row = 0x18 bytes = 6 ints
    }

    if (m_notify1) {
        m_notify1->Refresh();
    }
    if (m_extraNotify1) {
        m_extraNotify1->Notify(m_extraNotifyArg1);
    }
}

// Stream the full rect-only item state through the archive (stream slot 0x30).
// Returns 0 if the stream or the active game-manager is null; bumps the global
// serialize counter; ends with a variable-length loop over the m_ptrPool.GetData()[] pointer
// table (count m_ptrPool.GetSize()), each element streamed as 8 bytes. Field buffers are
// addressed by offset (the codegen is naming-independent here).
// @early-stop
// ~95.6%: the entire ~70-field transfer body is byte-exact; the residual is a
// regalloc/frame-size difference in the trailing 3x4 nested loop (retail pins
// the inner counter in ebp + reserves 1 stack dword via `push ecx`; the
// recompile spills the inner counter and reserves 3 via `sub esp,0xc`). Not
// steerable from C (docs/patterns regalloc/scheduling walls); deferred.

// CLevelSync::Sync (0x1084d0). THE BINARY SAYS IT BELONGS IN THIS OBJ. Carved into its
// own TU (the pre-merge
// LevelSync.cpp, byte-identical source, same flags="eh", same minimal include set) it
// scores 45.87%; compiled here it scores 73.53%. A 27.66-point swing on a 2412-byte
// function is a codegen-level statement about which TU retail compiled it in - the same
// class of evidence that proved BuildTabzDialog was NOT in this obj (its base-ctor
// spelling). So the wave1-E merge was RIGHT for LevelSync and WRONG for BuildTabzDialog;
// "the merge" was not uniformly one or the other. Header-weight was ruled out: with the
// pre-merge TU's exact minimal includes it still reads 45.87.
// Its referent views (CLevelSync/CLevelSyncChild/SyncSub) stay in <Gruntz/LevelSync.h> -
// a type defined in a .cpp is a fake per-TU view regardless of which TU wins.
RVA(0x001084d0, 0x96c)
i32 CLevelSync::Sync(CSerialArchive* s, i32 op, i32 p4, i32 p5) {
    if (s == 0) {
        return 0;
    }
    if (op == 4) {
        if (PreWriteValidate(s) == 0) {
            return 0;
        }
    } else if (op == 7) {
        if (PreReadValidate(s) == 0) {
            return 0;
        }
    } else if (op == 8) {
        (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
        if (m[0] == 0) {
            SubResetA();
            SubResetB();
        }
    }

    if (m[0x153] == 0) {
        i32 tmp = 0;
        if (op == 4) {
            s->Write(&tmp, 4);
        } else if (op == 7) {
            s->Read(&tmp, 4);
            if (tmp != 0) {
                CLevelSyncChild* c = new CLevelSyncChild();
                m[0x153] = reinterpret_cast<i32>(c);
                c->m_3c = this;
            }
        }
    } else {
        i32 tmp = 1;
        if (op == 4) {
            s->Write(&tmp, 4);
        }
    }

    if (m[0x153] != 0) {
        if ((reinterpret_cast<CLevelSync*>(m[0x153]))->ChildSync(s, op, p4, p5) == 0) {
            return 0;
        }
    }

    if (op == 4) {
        s->Write(&m[0x134], 8);
        s->Write(&m[0x136], 8);
    } else if (op == 7) {
        s->Read(&m[0x134], 8);
        s->Read(&m[0x136], 8);
    }
    if (op == 4) {
        s->Write(&m[0x13c], 8);
        s->Write(&m[0x13e], 8);
    } else if (op == 7) {
        s->Read(&m[0x13c], 8);
        s->Read(&m[0x13e], 8);
    }
    if (op == 4) {
        s->Write(&m[200], 8);
        s->Write(&m[0xca], 8);
    } else if (op == 7) {
        s->Read(&m[200], 8);
        s->Read(&m[0xca], 8);
    }
    if (op == 4) {
        s->Write(&m[0xce], 8);
        s->Write(&m[0xd0], 8);
    } else if (op == 7) {
        s->Read(&m[0xce], 8);
        s->Read(&m[0xd0], 8);
    }
    if (op == 4) {
        s->Write(&m[0x158], 8);
        s->Write(&m[0x15a], 8);
    } else if (op == 7) {
        s->Read(&m[0x158], 8);
        s->Read(&m[0x15a], 8);
    }

    i32* p = &m[0x8a];
    i32 n = 5;
    do {
        if (op == 4) {
            s->Write(p, 8);
            s->Write(p + 2, 8);
        } else if (op == 7) {
            s->Read(p, 8);
            s->Read(p + 2, 8);
        }
        p += 6;
        n--;
    } while (n != 0);

    n = 3;
    p = &m[0xb2];
    do {
        if (op == 4) {
            s->Write(p, 8);
            s->Write(p + 2, 8);
        } else if (op == 7) {
            s->Read(p, 8);
            s->Read(p + 2, 8);
        }
        p += 6;
        n--;
    } while (n != 0);

    i32 outer = 3;
    p = &m[0xe0];
    do {
        n = 4;
        do {
            if (op == 4) {
                s->Write(p, 8);
                s->Write(p + 2, 8);
            } else if (op == 7) {
                s->Read(p, 8);
                s->Read(p + 2, 8);
            }
            p += 6;
            n--;
        } while (n != 0);
        outer--;
    } while (outer != 0);

    if (op == 4) {
        s->Write(&m[0xa8], 8);
        s->Write(&m[0xaa], 8);
    } else if (op == 7) {
        s->Read(&m[0xa8], 8);
        s->Read(&m[0xaa], 8);
    }
    if (op == 7 && m[0] != 2) {
        PostBlockFixup();
    }

#define SER(idx)                                                                                   \
    if (SyncSub* _o = reinterpret_cast<SyncSub*>(m[idx])) {                                                          \
        if (_o->Serialize(s, op, p4, p5) == 0)                                                     \
            return 0;                                                                              \
    }

    {
        i32 i = 0;
        i32* q = &m[99];
        do {
            if (SyncSub* a = reinterpret_cast<SyncSub*>(q[-0xf])) {
                if (a->Serialize(s, op, p4, p5) == 0) {
                    return 0;
                }
            }
            if (SyncSub* b = reinterpret_cast<SyncSub*>(*q)) {
                if (b->Serialize(s, op, p4, p5) == 0) {
                    return 0;
                }
            }
            i++;
            q++;
        } while (i < 0xf);
    }
    {
        i32 i = 0;
        i32* q = &m[0x81];
        do {
            if (SyncSub* a = reinterpret_cast<SyncSub*>(*q)) {
                if (a->Serialize(s, op, p4, p5) == 0) {
                    return 0;
                }
            }
            i++;
            q++;
        } while (i < 5);
    }
    {
        i32 i = 0;
        i32* q = &m[0xc2];
        do {
            if (SyncSub* a = reinterpret_cast<SyncSub*>(*q)) {
                if (a->Serialize(s, op, p4, p5) == 0) {
                    return 0;
                }
            }
            i++;
            q++;
        } while (i < 3);
    }
    {
        i32 row = 0;
        i32* base = &m[0x126];
        do {
            i32 i = 0;
            i32* q = base;
            do {
                if (SyncSub* a = reinterpret_cast<SyncSub*>(*q)) {
                    if (a->Serialize(s, op, p4, p5) == 0) {
                        return 0;
                    }
                }
                i++;
                q++;
            } while (i < 4);
            row++;
            base += 4;
        } while (row < 3);
    }
    {
        i32 i = 0;
        i32* q = &m[0x187];
        do {
            if (SyncSub* a = reinterpret_cast<SyncSub*>(*q)) {
                if (a->Serialize(s, op, p4, p5) == 0) {
                    return 0;
                }
            }
            i++;
            q++;
        } while (i < 4);
    }

    SER(0x72)
    SER(0x73)
    SER(0x74)
    SER(0x75)
    SER(0x76)
    SER(0x77)
    SER(0x78)
    SER(0x79)
    SER(0x7a)
    SER(0x7b)
    SER(0x7c)
    SER(0x7c)
    SER(0x7d)
    SER(0x7e)
    SER(0x7f)
    SER(0x80)
    SER(0x86)
    SER(0x87)
    SER(0xd2)
    SER(0xd9)
    SER(0xda)
    SER(0xdb)
    SER(0xdc)
    SER(0x138)
    SER(0x140)
    SER(0x15c)
#undef SER

    Finalize();
    return 1;
}

RVA(0x001090a0, 0x38f)
i32 CStatusBarMgr::Serialize(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }

    char* B = reinterpret_cast<char*>(this);
    s->Write(B, 4); // offset 0 (vptr field)
    s->Write(B + 0x4, 4);

    g_serialCounter++;
    i32 seq = 0;
    // m_8 is the base CStatusBarItem `int` member, overlaid here as a pointer to
    // the sequence holder (retail reads it as a pointer and fetches +0x188).
    // Authentic int-as-pointer overlay; retyping it lives in the base class, not
    // here, so the cast stays (flagged in the report).
    if (m_8) {
        seq = (reinterpret_cast<CSbiSeqHolder*>(m_8))->m_188;
    }
    s->Write(&seq, 4);

    s->Write(B + 0x10, 0x10);
    s->Write(B + 0x20, 4);
    s->Write(B + 0x24, 4);
    s->Write(B + 0x28, 4);
    s->Write(B + 0x110, 4);
    s->Write(B + 0x62c, 4);

    char* p = B + 0x114;
    for (i32 i = 0; i < 15; i++) {
        s->Write(p, 4);
        p += 4;
    }

    s->Write(B + 0x34c, 4);
    s->Write(B + 0x350, 4);
    s->Write(B + 0x354, 4);
    s->Write(B + 0x35c, 4);
    s->Write(B + 0x360, 4);
    s->Write(B + 0x10c, 4);
    s->Write(B + 0x298, 4);
    s->Write(B + 0x29c, 4);
    s->Write(B + 0x524, 4);
    s->Write(B + 0x52c, 4);
    s->Write(B + 0x528, 4);
    s->Write(B + 0x544, 4);
    s->Write(B + 0x504, 0x10);
    s->Write(B + 0x514, 0x10);
    s->Write(B + 0x548, 4);
    s->Write(B + 0x550, 4);
    s->Write(B + 0x554, 4);
    s->Write(B + 0x4c8, 4);
    s->Write(B + 0x4cc, 4);
    s->Write(B + 0x4e8, 4);
    s->Write(B + 0x4ec, 4);
    s->Write(B + 0x318, 4);
    s->Write(B + 0x31c, 4);
    s->Write(B + 0x330, 4);
    s->Write(B + 0x334, 4);
    s->Write(B + 0x558, 4);
    s->Write(B + 0x55c, 4);
    s->Write(B + 0x574, 4);
    s->Write(B + 0x578, 4);

    char* q = B + 0x224;
    for (i32 j = 0; j < 5; j++) {
        s->Write(q - 4, 4);
        s->Write(q, 4);
        q += 0x18;
    }
    char* r = B + 0x2c4;
    for (i32 k = 0; k < 3; k++) {
        s->Write(r - 4, 4);
        s->Write(r, 4);
        r += 0x18;
    }
    char* nb = B + 0x378;
    i32 outer = 3;
    do {
        for (i32 m = 0; m < 4; m++) {
            s->Write(nb, 4);
            s->Write(nb + 4, 4);
            nb += 0x18;
        }
    } while (--outer);

    i32 count = m_ptrPool.GetSize();
    s->Write(&count, 4);
    for (u32 n = 0; n < static_cast<u32>(count); n++) {
        s->Write(m_ptrPool.GetData()[n], 8);
    }
    return 1;
}

// CSbiSeqMap/CSbiSeqObj moved to <Gruntz/StatusBarMgr.h>.

// CStatusBarMgr::Deserialize - the load/restore counterpart of Serialize. Pulls
// the full rect-only item state from the archive via stream slot 0x2c (Read);
// resolves the base m_8 sequence holder from the streamed seq id through the
// game-manager's object map (validated by the looked-up object's type tag == 5);
// reads every field back; returns each pooled m_ptrPool.GetData() element to the engine
// free-list, sizes the +0x530 collection, then reloads the pointer table from the
// free-list. Field buffers are offset-addressed (naming-independent), mirroring
// Serialize.
// @early-stop
// twin of Serialize (95.6%, regalloc/frame wall in the trailing nested loop): the
// whole ~70-field Read body + the seq resolution + the free-list return/reload are
// reconstructed byte-faithfully, but the same trailing 3x4 nested-loop induction /
// frame-size regalloc choice (plus the free-list induction wall shared with
// Teardown/InsertPtr) caps it below 100%. Not steerable from C; deferred.
RVA(0x00109520, 0x44c)
i32 CStatusBarMgr::Deserialize(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* gm = g_gameReg->m_world; // the REAL world mgr (ex CSbiGameMgr facet)
    if (gm == 0) {
        return 0;
    }
    char* B = reinterpret_cast<char*>(this);
    *reinterpret_cast<i32*>((B + 0x618)) = 0;
    ResetWidgets(0);

    s->Read(B, 4);
    s->Read(B + 0x4, 4);

    g_serialCounter++;
    i32 seq = 0;
    s->Read(&seq, 4);

    void* obj = 0;
    CMapPtrToPtr* map = reinterpret_cast<CMapPtrToPtr*>((*reinterpret_cast<char**>(reinterpret_cast<char*>(gm) + 8) + 0x48));
    i32 m8 = 0;
    if (map->Lookup(reinterpret_cast<void*>(seq), obj)) {
        if (obj != 0) {
            m8 = ((static_cast<CSbiSeqObj*>(obj))->TypeTag() == 5) ? reinterpret_cast<i32>(obj) : 0;
        }
    }
    m_8 = m8;
    if (m_8 == 0 && seq != 0) {
        return 0;
    }

    s->Read(B + 0x10, 0x10);
    s->Read(B + 0x20, 4);
    s->Read(B + 0x24, 4);
    s->Read(B + 0x28, 4);
    s->Read(B + 0x110, 4);
    s->Read(B + 0x62c, 4);

    char* p = B + 0x114;
    for (i32 i = 0; i < 15; i++) {
        s->Read(p, 4);
        p += 4;
    }

    s->Read(B + 0x34c, 4);
    s->Read(B + 0x350, 4);
    s->Read(B + 0x354, 4);
    s->Read(B + 0x35c, 4);
    s->Read(B + 0x360, 4);
    s->Read(B + 0x10c, 4);
    s->Read(B + 0x298, 4);
    s->Read(B + 0x29c, 4);
    s->Read(B + 0x524, 4);
    s->Read(B + 0x52c, 4);
    s->Read(B + 0x528, 4);
    s->Read(B + 0x544, 4);
    s->Read(B + 0x504, 0x10);
    s->Read(B + 0x514, 0x10);
    s->Read(B + 0x548, 4);
    s->Read(B + 0x550, 4);
    s->Read(B + 0x554, 4);
    s->Read(B + 0x4c8, 4);
    s->Read(B + 0x4cc, 4);
    s->Read(B + 0x4e8, 4);
    s->Read(B + 0x4ec, 4);
    s->Read(B + 0x318, 4);
    s->Read(B + 0x31c, 4);
    s->Read(B + 0x330, 4);
    s->Read(B + 0x334, 4);
    s->Read(B + 0x558, 4);
    s->Read(B + 0x55c, 4);
    s->Read(B + 0x574, 4);
    s->Read(B + 0x578, 4);

    char* q = B + 0x224;
    for (i32 j = 0; j < 5; j++) {
        s->Read(q - 4, 4);
        s->Read(q, 4);
        q += 0x18;
    }
    char* r = B + 0x2c4;
    for (i32 k = 0; k < 3; k++) {
        s->Read(r - 4, 4);
        s->Read(r, 4);
        r += 0x18;
    }
    char* nb = B + 0x378;
    i32 outer = 3;
    do {
        for (i32 m = 0; m < 4; m++) {
            s->Read(nb, 4);
            s->Read(nb + 4, 4);
            nb += 0x18;
        }
    } while (--outer);

    for (i32 t = 0; t < m_ptrPool.GetSize(); t++) {
        void* pp = m_ptrPool.GetData()[t];
        if (pp) {
            CoordPoolNode* node = g_coordPool.NodeOf(pp);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_ptrPool.SetSize(0, -1);

    i32 count = 0;
    s->Read(&count, 4);
    m_ptrPool.SetSize(count, -1);
    for (u32 n = 0; n < static_cast<u32>(count); n++) {
        CoordPoolNode* head = g_coordPool.m_freeHead;
        void* node = 0;
        if (head->m_next != 0) {
            node = &head->m_coord;
            g_coordPool.m_freeHead = head->m_next;
        }
        s->Read(node, 8);
        m_ptrPool.GetData()[n] = node;
    }
    return 1;
}

// Periodic highlight-cursor tick (RVA-ascending). Bail while the suppress flag
// (m_hlBusy) is set or the game is over (g_gameReg->m_134==1). If this is the
// subtype-2 cursor item, refresh its state first. When the active tab is not the
// gauge tab (4), latch it inactive (SetTabState(4,3)) and Deactivate; otherwise
// advance the 4-state cursor (forward wraps 4->0; the `reverse` path only guards
// the signed-overflow wrap), then refresh the widgets and re-arm.
RVA(0x0010b4f0, 0xaa)
void CStatusBarMgr::AdvanceTab(i32 reverse) {
    if (m_hlBusy != 0) {
        return;
    }
    if (g_gameReg->m_134 == 1) {
        return;
    }
    if (m_position == kSubtypeTag) {
        RefreshState();
    }
    if (m_activeTab != 4) {
        SetTabState(4, 3);
        Deactivate();
        return;
    }
    if (reverse != 0) {
        if (++m_tabCycle < 0) {
            m_tabCycle = 3;
        }
    } else {
        if (++m_tabCycle >= 4) {
            m_tabCycle = 0;
        }
    }
    ResetWidgets(0);
    TryActivate();
    Deactivate();
}

// Highlight-click handler for group 0 (rows m_hlGrid[0..3]): bail if the busy
// gate is set or the row is not armed (state==1); validate the row's handle, play
// the GAME_TABHIGHLIGHT1 cue on the draw-clock window if the music gate is free,
// then latch the pending row, clear the handle, and re-notify. Single-exit nesting
// (success-deepest, one trailing `return 0`) reproduces retail's shared FAIL tail
// (docs/patterns/nested-if-success-deepest-error-tail.md); the global pair must be
// read TOGETHER inside the `if (found)` so MSVC lands g_sndEnabled in ecx / g_sndCueTag in
// edx like retail.
// @early-stop
// ~94.2%: code bytes are byte-exact vs retail; the residual is purely the
// reloc-symbol-naming scoring tail (DIR32 to differently-named globals/the cue
// string vs retail's REL32). Not a wall - matcher.md reloc-typing artifact.
RVA(0x0010b5d0, 0xdd)
i32 CStatusBarMgr::HlClickGroup0(i32 row) {
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_4f0 == 0 && m_hlGrid[row].m_state == 1) {
        i32 handle = m_hlGrid[row].m_handle;
        i32* slot = &m_hlGrid[row].m_handle;
        if (ResolveHandle(handle)) {
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
            if (host->m_30 == 0) {
                void* found = 0;
                CMapStringToPtr* map = &host->m_10; // CMapStringToPtr per the mfc_class audit (the facet said Ob - the documented band inversion)
                map->Lookup("GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0) {
                        CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                        if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                            p->m_14 = g_killCueClock;
                            p->m_10->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            m_pendingHlRow = row;
            *slot = 0;
            NotifyAllSlots();
            return 1;
        }
    }
    return 0;
}

// Highlight-click handler for group 1 (rows m_hlGrid[4..7]); identical to group 0
// with the +4-row base folded into the index.
// @early-stop
// ~94.2%: code byte-exact; reloc-symbol-naming tail only (see HlClickGroup0).
RVA(0x0010b6f0, 0xdd)
i32 CStatusBarMgr::HlClickGroup1(i32 row) {
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_4f0 == 0 && m_hlGrid[row + 4].m_state == 1) {
        i32 handle = m_hlGrid[row + 4].m_handle;
        i32* slot = &m_hlGrid[row + 4].m_handle;
        if (ResolveHandle(handle)) {
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
            if (host->m_30 == 0) {
                void* found = 0;
                CMapStringToPtr* map = &host->m_10; // CMapStringToPtr per the mfc_class audit (the facet said Ob - the documented band inversion)
                map->Lookup("GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0) {
                        CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                        if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                            p->m_14 = g_killCueClock;
                            p->m_10->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            m_pendingHlRow = row;
            *slot = 0;
            NotifyAllSlots();
            return 1;
        }
    }
    return 0;
}

// Highlight-click handler for group 2 (rows m_hlGrid[8..11]); +8-row base.
// @early-stop
// ~94.2%: code byte-exact; reloc-symbol-naming tail only (see HlClickGroup0).
RVA(0x0010b810, 0xdd)
i32 CStatusBarMgr::HlClickGroup2(i32 row) {
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_4f0 == 0 && m_hlGrid[row + 8].m_state == 1) {
        i32 handle = m_hlGrid[row + 8].m_handle;
        i32* slot = &m_hlGrid[row + 8].m_handle;
        if (ResolveHandle(handle)) {
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
            if (host->m_30 == 0) {
                void* found = 0;
                CMapStringToPtr* map = &host->m_10; // CMapStringToPtr per the mfc_class audit (the facet said Ob - the documented band inversion)
                map->Lookup("GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0) {
                        CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                        if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                            p->m_14 = g_killCueClock;
                            p->m_10->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            m_pendingHlRow = row;
            *slot = 0;
            NotifyAllSlots();
            return 1;
        }
    }
    return 0;
}

// Step the gauge one unit toward its target; on reaching 100 fire the completion
// hook; if the gauge moved, refresh the two gauge notifiers (the sink latches the
// new reading into its m_44). The toward-target step is spelled as `<`/`<=goto`/`>`
// (no outer `!=`) so the equal case folds into the jge/jle pair like retail and the
// two inc/dec branches share one store, not an extra top `je`.
// @early-stop
// ~95.7%: byte-exact except a 1-instr regalloc coin-flip in the m_gaugeSink->m_44 store
// block (retail keeps the gauge value in eax + loads the vtable into edx; the
// recompile uses edx for the value). Not steerable from C; deferred.
RVA(0x00105480, 0x7d)
void CStatusBarMgr::TickGauge() {
    i32 changed = 0;
    i32 g = m_gauge;
    i32 t = m_gaugeTarget;
    if (g < t) {
        g++;
    } else if (g <= t) {
        goto noChange;
    } else {
        g--;
    }
    m_gauge = g;
    changed = 1;
noChange:;
    if (m_gauge == 100) {
        if (GaugeComplete()) {
            changed = 1;
            GaugeFinish(0);
        }
    }
    if (changed) {
        if (m_gaugeSink && m_gaugeNotify) {
            m_gaugeNotify->Refresh();
            m_gaugeSink->m_gaugeReading = m_gauge;
            m_gaugeSink->Refresh();
        }
    }
}

// Find the first slot whose state is ready; re-arm it and report found.
RVA(0x00109a90, 0x25)
i32 CStatusBarMgr::FindReadySlot() {
    for (i32 i = 0; i < 5; i++) {
        if (m_slots[i].m_state == kSlotReady) {
            ArmSlot(i);
            return 1;
        }
    }
    return 0;
}

// Lazily create the +0x54c sub-object on first use: bail (return 0) if it already
// exists; otherwise `new` a 0x40-byte CWarpStoneFly, default-construct it, store it,
// and forward the three args to its Init. /GX frames the new+ctor span (collapsed
// from SBI_RectOnlyEh.cpp; this fn is one of the interval's EH-site proofs).
RVA(0x00109ad0, 0xa9)
i32 CStatusBarMgr::EnsureSub(i32 a, i32 b, i32 c) {
    if (m_retabNotify) {
        return 0;
    }
    CWarpStoneFly* o = new CWarpStoneFly();
    m_retabNotify = o;
    if (o == 0) {
        return reinterpret_cast<i32>(o); // retail returns the null pointer already in eax (no re-xor)
    }
    return o->Init(this, a, b, c);
}

// Release the per-tab sprite widgets for the given tab group (idx, with -1 = all).
RVA(0x00101420, 0x110)
i32 CStatusBarMgr::ClearTabSprites(i32 idx) {
    if (idx == -1 || idx == 0) {
        if (m_tabSprite0) {
            m_tabSprite0->Blit();
        }
        if (m_tabSprite2) {
            m_tabSprite2->Blit();
        }
        if (m_tabSprite1) {
            m_tabSprite1->Blit();
        }
        if (m_tabSprite3) {
            m_tabSprite3->Blit();
        }
        if (m_tabSprite4) {
            m_tabSprite4->Blit();
        }
    }
    if (idx == 5 || idx == -1) {
        if (m_tabSprite5) {
            m_tabSprite5->Blit();
        }
        if (m_tabSprite6) {
            m_tabSprite6->Blit();
        }
        if (m_tabSprite7) {
            m_tabSprite7->Blit();
        }
        if (m_tabSprite8) {
            m_tabSprite8->Blit();
        }
        if (m_tabSprite9) {
            m_tabSprite9->Blit();
        }
        if (m_tabSprite10) {
            m_tabSprite10->Blit();
        }
    }
    if (idx == 6 || idx == -1) {
        if (m_tabSprite11) {
            m_tabSprite11->Blit();
        }
        if (m_tabSprite12) {
            m_tabSprite12->Blit();
        }
        if (m_tabSprite13) {
            m_tabSprite13->Blit();
        }
        if (m_tabSprite14) {
            m_tabSprite14->Blit();
        }
    }
    return 1;
}

// Deactivate the rect-only item: if it is the subtype-2 cursor item, latch the
// game-reg cursor rect; then fire the deactivate notifier across list[0] and the
// active-tab list, clear, and tag the state. The +0x30 list array (0x1c stride,
// head at +0) is offset-addressed: the typed array view would overlap the
// matched m_30/m_34 ints, so the raw access is the codegen-faithful model here.
// @early-stop
// ~97.2%: the two notify-list walks + teardown are byte-exact; the residual is a
// 3-instr regalloc choice in the cursor-rect prologue (retail loads g_gameReg via
// `mov eax,moffs` and computes the two `-0x45/-0x30` offsets with non-destructive
// `lea` into fresh regs; the recompile loads into ecx and uses `sub` in place).
// Not steerable from C - logic byte-correct; deferred to the final sweep.
RVA(0x00100cb0, 0x8b)
i32 CStatusBarMgr::Deactivate() {
    if (m_position == kSubtypeTag) {
        CGruntzMgr* g = g_gameReg;
        i32 a = g->m_modeW;
        i32 b = g->m_modeH;
        i32 x = a - 0x45;
        i32 y = b - 0x30;
        m_28 = y;
        m_24 = x;
        SetCursorRect(x, y);
    }

    POSITION n = m_tabLists[0].GetHeadPosition();
    while (n) {
        CSbiSlotPtr* cur = static_cast<CSbiSlotPtr*>(m_tabLists[0].GetNext(n));
        if (cur) {
            cur->Refresh();
        }
    }

    CPtrList& tab = m_tabLists[m_activeTab];
    POSITION m = tab.GetHeadPosition();
    while (m) {
        CSbiSlotPtr* cur = static_cast<CSbiSlotPtr*>(tab.GetNext(m));
        if (cur) {
            cur->Refresh();
        }
    }

    ClearTabSprites(-1);
    m_rect14.m_c = 2;
    return 1;
}

// Switch the active tab: short-circuit if already on it (and not forced);
// otherwise notify the per-tab list, clear it, drop the tab-5 sprites, latch the
// new tab, and re-probe. On probe failure report the error and bail.
RVA(0x001020a0, 0xae)
i32 CStatusBarMgr::SetTab(i32 tab, i32 flag) {
    if (tab == m_activeTab && flag == 0) {
        return 1;
    }
    POSITION n = m_tabLists[5].GetHeadPosition();
    while (n) {
        CSbiNotifyTarget* cur = static_cast<CSbiNotifyTarget*>(m_tabLists[5].GetNext(n));
        if (cur) {
            cur->Notify(1);
        }
    }
    m_tabLists[5].RemoveAll();
    m_tabSprite5 = 0;
    m_tabSprite6 = 0;
    m_tabSprite7 = 0;
    m_tabSprite8 = 0;
    m_tabSprite9 = 0;
    m_tabSprite10 = 0;
    m_activeTab = tab;
    // WRONG-CALLEE FIX (assert_relocs): this called RefreshState() @0xfe670 (43 B). Retail
    // calls `this->LoadTabSprites()` @0x102250 (7629 B) - `mov ecx,edi; call 0x1690` right
    // after the m_activeTab store. Both exist, so it linked and objdiff reloc-masked it: a
    // silently-wrong callee. LoadTabSprites is modeled on CStatusBarMgr (StatusBarMgr.cpp),
    // reached by the cast SBI_MenuItem.cpp:208 already uses. (The cast is itself a smell -
    // SetTab @0x1020a0 and LoadTabSprites @0x102250 are adjacent and run on the SAME `this`,
    // so CStatusBarMgr and CStatusBarMgr are very likely one class - an identity fold for a
    // later pass, not something to fabricate here.)
    if (!LoadTabSprites()) {
        g_gameReg->ReportError(kActivateErrId, kSetTabErrTag);
        return 0;
    }
    Deactivate();
    return 1;
}

// Tear down the rect-only item: log its position tag, fire the pre-teardown
// notify, return every pooled m_ptrPool.GetData() element to the engine free-list, then
// RemoveAll the +0x530 collection.
// @early-stop
// ~84.7%: the log/notify head + the free-list return + the +0x530 RemoveAll are
// byte-correct; the residual is the free-loop's induction-variable choice (retail
// indexes m_ptrPool.GetData()[ecx] reloading the base each iter and keeps the running
// free-list head in edx; the recompile strength-reduces to a walking pointer +
// pins the head in a callee-saved edi). Regalloc/induction wall; deferred.
RVA(0x000fe350, 0x6d)
void CStatusBarMgr::Teardown() {
    (static_cast<Utils::RegistryHelper*>(g_gameReg->m_settings))
        ->SetValueDword("StatusBar Position", m_position);
    ResetWidgets(0);
    for (i32 i = 0; i < m_ptrPool.GetSize(); i++) {
        void* p = m_ptrPool.GetData()[i];
        if (p) {
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    // The +0x530 head is an MFC CPtrArray (m_ptrPool.GetData()/m_ptrPool.GetSize() are its m_pData/m_nSize);
    // its teardown is CPtrArray::SetSize(0,-1) inlined from RemoveAll (0x1b4f75, library).
    m_ptrPool.SetSize(0, -1);
}

// Activate the rect-only item; gate on the offset-0 subtype tag and a probe.
RVA(0x00104d60, 0x48)
i32 CStatusBarMgr::TryActivate() {
    // Offset-0 read: in retail this object's slot 0 holds a small integer
    // subtype tag (the manual-vtable-stamp tag device shared with CStatusBarMgr),
    // not a real C++ vptr. We model the vtable via `virtual`,
    // so slot 0 cannot also be a named field here without dropping the vtable;
    // the raw `*(int*)this` read is the faithful model. See report (flagged).
    if (m_position == kSubtypeTag) {
        return Activate();
    }
    if (!BuildStatusBarTabs()) {
        g_gameReg->ReportError(kActivateErrId, kActivateErrTag);
        return 0;
    }
    SetTabState(m_activeTab, 3);
    return 1;
}

// ---------------------------------------------------------------------------
// 0x104dd0 - CStatusBarMgr::Activate: lazily create the status-bar sprite. Both
// SetState (subtype-2 probe) and TryActivate call it with `mov ecx,this` (proven:
// ecx==this in both call-sites' disasm), so `this` IS the CStatusBarMgr - not the
// former StatusBarSpriteHolder @orphan view. Its m_8/m_c/m_24/m_28 are the base
// CStatusBarItem fields reused here as sprite/factory-holder/x/y (a proven-
// heterogeneous slot set: Setup args on other paths, sprite state on this one).
// @early-stop
// scheduling wall: retail computes m_8c-0x22 via lea eax,[ecx-0x22] and loads m_x
// late; cl uses sub + an earlier m_x load. Clamp logic, factory call and literal faithful.
RVA(0x00104dd0, 0x6b)
i32 CStatusBarMgr::Activate() {
    if (m_8 != 0) {
        return 0;
    }
    i32 a = g_gameReg->m_modeW - 0x22;
    i32 d = g_gameReg->m_modeH;
    if (m_24 > a) {
        m_24 = a;
    }
    if (m_28 > d - 9) {
        m_28 = d - 0x22;
    }
    m_8 = reinterpret_cast<i32>((m_c)->m_childGroup->CreateSprite(0, m_24, m_28, 0xf4240, "StatusBarSprite", 1));
    return m_8 != 0;
}

// ===========================================================================
// One-TU merge block [0x104e60 .. 0x10b320] - see the include-block note.
// Referent decls carried over from the absorbed TUs:
// ===========================================================================

// (the running game clock g_frameTime is declared once at the top of this TU)

// The ?::CopyRect@@3P6GXPAUtagRECT@@PBU1@@ZA global fn-pointer (VA 0x6c44bc): a
// __stdcall RECT copier called `call ds:[::CopyRect]`.

// ===========================================================================
// CStatusBarMgr::LoadStatzTabToggleSprite @0x104e60
// ===========================================================================
//
// Toggles the per-statz-tab indicator `idx` to `value`: a no-op if it already
// holds `value`; otherwise, gated on the tab's group-record being live, it stamps
// the toggle item (this[idx]+0x150), kicks the tab sub-helper when the view mode
// is 3, runs the STATZTABTOGGLE status-bar advance, and latches the new value.
// __thiscall ret 8. Always returns 1.

// @early-stop
// ~80.8%: logic + offsets + the advance-tail are byte-faithful. Residual is a
// constant/register-pinning coin-flip: retail keeps a 4th callee-saved reg (ebp) live
// and PINS the constant 1 in ecx, reusing it for `item->m_active=1`, the `==1` gate
// and the Toggle(...,1) arg (`mov ecx,1; ... push ecx`); this toolchain uses fewer
// registers and emits the 1 as inline immediates instead. Already spelled with a
// shared `i32 one=1` local, which MSVC5 declines to keep in a register - a regalloc
// pressure coin-flip, not source-steerable; deferred to the final sweep.
// 0x104e60 is a real CStatusBarMgr method (ToggleStat calls it unqualified on `this`),
// typed against the canonical layout (m_statFlags @+0x114, m_hitRects @+0x150,
// m_statObj @+0x18c, m_activeTab, m_position). The +0x150 element's toggle facet is the
// same CSbiRect (m_enabled/+0x44 m_toggleValue). The +0x68 "unit-record table"
// IS CTriggerMgr - its +0x1c slot array is m_grid.
RVA(0x00104e60, 0xed)
i32 CStatusBarMgr::LoadStatzTabToggleSprite(i32 value, i32 idx) {
    if (m_statFlags[idx] == value) {
        return 1;
    }

    i32 slot = idx + 15 * g_curPlayer;
    if (g_gameReg->m_cmdGrid->m_grid[slot] == 0) {
        return 0;
    }

    CSbiRect* item = m_hitRects[idx];
    i32 one = 1;
    if (item) {
        item->m_toggleValue = value;
        item->m_enabled = one;
        if (m_activeTab == one) {
            m_statObj[idx]->Toggle(m_position, one);
            CSndHost* h = g_gameReg->m_world->m_soundRegistry;
            if (h->m_emitGate == 0) {
                void* spr_ob = 0;
                h->m_10.Lookup("GAME_STATZTABTOGGLE", spr_ob);
                LeafCue* spr = static_cast<LeafCue*>(spr_ob);
                if (spr) {
                    if (g_sndEnabled != 0 && g_killCueClock - spr->m_14 >= spr->m_18) {
                        spr->m_14 = g_killCueClock;
                        spr->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                    }
                }
            }
        }
    }
    m_statFlags[idx] = value;
    return 1;
}

// ===========================================================================
// CStatusBarMgr::UpdateGruntOvenStatusBar @0x105310
// ===========================================================================
//
// Walks the 5 grunt-oven cooking tabs: while a tab is COOKING (m_state==1) it derives
// the cooking-progress frame index from the elapsed clock / GruntOvenDelay, caps
// at 0x1a (completion - flips m_state to 2 and runs the COOKINGCOMPLETE advance), and
// pushes the new frame into the widget when it changes (the +0x30 virtual).
// @early-stop
// ~79.9%: logic + every store/offset/advance-tail is byte-faithful. Residual is the
// 64-bit signed-clamp `elapsed = (d>=0)?(i32)d:0` codegen: retail emits the un-folded
// `cmp hi,ebx(0); jg; jl; cmp lo,ebx; jae` compare then a branch-select `xor esi,esi;
// jmp / mov esi,eax(lo)` keeping the raw lo in a temp, whereas this toolchain FOLDS it
// to the sbb sign-flag (`js`) and fuses lo directly into the elapsed reg (esi) - i.e.
// cl here is MORE optimized than retail's exact MSVC5 build. A toolchain-microversion
// codegen wall, not source-steerable; deferred to the final sweep.
RVA(0x00105310, 0x11a)
void CStatusBarMgr::UpdateGruntOvenStatusBar() {
    // The 5 grunt-oven cooking tabs ARE this class's own +0x220 slot records
    // (m_slots, CSbiSlot) with their +0x204 notifier array (m_slotNotify): the
    // slot's m_value (+4) is the animation frame, m_8/m_c (+8/+c) the 64-bit
    // start clock, and the notifier's slot-12 Notify(frame) is the set-frame hook.
    // (Folded off the fake EngineLabelBacklog view onto the canonical class.)
    CSbiSlotPtr** slot = m_slotNotify;
    CSbiSlot* tab = m_slots;
    i32 n = 5;
    do {
        if (tab->m_state == 1) {
            i64 d = static_cast<i64>(static_cast<u32>(g_frameTime)) - *reinterpret_cast<i64*>(&tab->m_8);
            i32 elapsed = (d >= 0) ? static_cast<i32>(d) : 0;
            u32 delay = g_buteMgr.GetDwordDef("StatusBar", "GruntOvenDelay", 0xc8);
            i32 frame = static_cast<i32>((static_cast<u32>(elapsed) / delay)) + 1;
            if (frame >= 0x1a) {
                tab->m_state = 2;
                frame = 0x1a;
                CSndHost* h = g_gameReg->m_world->m_soundRegistry;
                if (h->m_emitGate == 0) {
                    void* spr_ob = 0;
                    h->m_10.Lookup("GAME_COOKINGCOMPLETE", spr_ob);
                    LeafCue* spr = static_cast<LeafCue*>(spr_ob);
                    if (spr) {
                        if (g_sndEnabled != 0 && g_killCueClock - spr->m_14 >= spr->m_18) {
                            spr->m_14 = g_killCueClock;
                            spr->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                        }
                    }
                }
            }
            if (frame != tab->m_value) {
                tab->m_value = frame;
                CSbiSlotPtr* w = *slot;
                if (w) {
                    w->Notify(frame);
                }
            }
        }
        ++slot;
        ++tab;
    } while (--n != 0);
}

// ===========================================================================
// CStatusBarMgr::UpdateChipGrinderStatusBar @0x1076a0
// ===========================================================================
//
// Drives the rez chip-grinder conveyor while it is RUNNING (m_4e8 != 0): it pulls
// the FallingItem delay/speed (then the ShredderDelay/Speed once the conveyor
// reaches the shredder at m_510 >= 0x1bf, where it also runs the one-shot
// REZGRINDING status-bar advance and flips to phase 2), advances the two conveyor
// extents (m_508/m_510) by the speed each time the retrigger clock elapses, and
// re-stamps the grinder rect-target widget (m_500) from the scroll origin. When
// the conveyor runs out (m_508 >= 0x1c7) it stops (m_4e8 = 0). A final
// ChipGrinderFinishStep runs while the widget is live and a step happened.
// @early-stop
// ~80.7%: instruction count matches retail exactly (164==164); logic + all offsets/
// stores/advance-tail are byte-faithful. Residual is a pervasive zero-register-pinning
// role swap (docs/patterns/zero-register-pinning.md): retail pins 0 in ebx and the
// phase-constant 3 in edi, this toolchain pins 0 in edi + spills a second zero into
// ebp - a 1-instr phase shift cascading through every `mov [field],0` store. A
// regalloc coin-flip, not source-steerable; deferred to the final sweep.
RVA(0x001076a0, 0x1f3)
void CStatusBarMgr::UpdateChipGrinderStatusBar() {
    // Every offset is the canonical member - the grinder conveyor
    // is the m_fall* band and the rect-target widget is m_extraNotify1's own
    // +0x14 screen rect (CSbiSlotPtr::m_rect14 - the same slot-map rect band
    // CSbiRect carries as m_xLo..m_yHi).
    if (m_fallActive == 0) {
        return;
    }

    i32 stepped = 0;
    if (m_fallActive == 1 || m_fallActive == 2) {
        u32 delay = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
        i32 speed = g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 4);

        if (m_fallRectT >= 0x1c7) {
            m_fallActive = 0;
            m_extraNotifyArg1 = 0;
        } else if (m_fallRectB >= 0x1bf) {
            if (m_fallActive != 2) {
                if (m_activeTab == 3 && m_position != 2) {
                    CSndHost* h = g_gameReg->m_world->m_soundRegistry;
                    if (h->m_emitGate == 0) {
                        void* spr_ob = 0;
                        h->m_10.Lookup("GAME_REZGRINDING", spr_ob);
                        LeafCue* spr = static_cast<LeafCue*>(spr_ob);
                        if (spr) {
                            if (g_sndEnabled != 0 && g_killCueClock - spr->m_14 >= spr->m_18) {
                                spr->m_14 = g_killCueClock;
                                spr->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                            }
                        }
                    }
                }
                m_fallActive = 2;
            }
            delay = g_buteMgr.GetDwordDef("StatusBar", "FallingItemShredderDelay", 0x64);
            speed = g_buteMgr.GetIntDef("StatusBar", "FallingItemShredderSpeed", 2);
        }

        i64 d = static_cast<i64>(static_cast<u32>(g_frameTime)) - *reinterpret_cast<i64*>(&m_fallLast);
        if (d >= *reinterpret_cast<i64*>(&m_fallDelay)) {
            i32 newLo = m_fallRectT + speed;
            m_fallRectT = newLo;
            i32 newHi = m_fallRectB + speed;
            m_fallRectB = newHi;
            CSbiSlotPtr* w = m_extraNotify1;
            if (w) {
                i32 sx = m_10;
                i32 sy = m_rect14.m_0;
                i32* p = w->m_rect14;
                p[0] = m_fallRectL + sx;
                p[1] = sy + newLo;
                p[2] = m_fallRectR + sx;
                p[3] = sy + newHi;
            }
            m_fallDelay = static_cast<i32>(delay);
            m_fallDelayHi = 0;
            m_fallLast = static_cast<i32>(g_frameTime);
            m_fallLastHi = 0;
            stepped = 1;
        }
    }

    if (m_extraNotify1 != 0 && stepped) {
        ChipGrinderFinishStep();
    }
}

// ===========================================================================
// CWarpStoneFly::Init @0x109bd0
// ===========================================================================
//
// Sets up the warp-stone "fly" animation toward the warp tab. It records arg0 at
// m_3c, resolves the frame for (phase+1) out of the GAME_STATUSBAR_TABZ_GAMETAB_WARP
// sprite (m_38), and on success computes the screen target (m_4/m_8) for the phase
// (a per-phase pixel offset off the tab base m_3c->m_10/m_14), the euclidean
// distance to the source (srcX/srcY), and the per-axis fly velocity scaled by
// FlyTime, then runs the GAME_WARPSTONEFLY status-bar advance. __thiscall ret 0x10.
// @early-stop
// ~81.2%: logic + the sqrt/fly-velocity FP block + the advance-tail are byte-faithful.
// Residuals are two regalloc/scheduling coin-flips: (1) the prologue orders the
// `mov [esp+X],0` stack-init vs the `lea ecx,[esp+X]` differently, and (2) the frame
// lookup `(spr && n in range) ? spr->m_frames[n] : 0` keeps the loaded pointer in a
// temp (eax) and branch-selects into edi in retail, where this toolchain fuses the
// load directly into edi (`mov edi,[ecx+4*edi]`). Same select-register-fusion family
// as the 64-bit clamp; not source-steerable; deferred to the final sweep.
// 0x109bd0 IS CWarpStoneFly::Init - CStatusBarMgr::EnsureSub news a CWarpStoneFly and
// calls o->Init(this,a,b,c) on it (the void* owner is the CStatusBarMgr back-ptr).
// Typed against the canonical <Gruntz/WarpStoneFly.h> layout.
RVA(0x00109bd0, 0x1b5)
i32 CWarpStoneFly::Init(void* owner, i32 phase, i32 srcX, i32 srcY) {
    m_owner = static_cast<CStatusBarMgr*>(owner);

    void* spr_ob = 0;
    i32 n = phase + 1;
    g_gameReg->m_world->m_soundRegistry->m_10.Lookup("GAME_STATUSBAR_TABZ_GAMETAB_WARP", spr_ob);
    CSprite* spr = static_cast<CSprite*>(spr_ob);
    CImage* frame =
        (spr && n >= spr->m_minIndex && n <= spr->m_maxIndex) ? static_cast<CImage*>(spr->m_items.GetAt(n)) : 0;
    m_sprite = frame;
    if (frame == 0) {
        return 1;
    }

    m_arrivalMode = phase;
    i32 cx, dy;
    switch (phase) {
        case 2:
            cx = 0x69;
            dy = 0x26;
            break;
        case 3:
            cx = 0x65;
            dy = 0x50;
            break;
        case 4:
            cx = 0x69;
            dy = 0x54;
            break;
        default:
            cx = 0x34;
            dy = 0x29;
            break;
    }

    CStatusBarMgr* base = m_owner;
    i32 tx = base->m_10 + cx;
    m_targetX = tx;
    i32 ty = base->m_rect14.m_0 + dy;
    m_targetY = ty;

    i32 dxv = tx - srcX;
    i32 dyv = ty - srcY;
    i32 dist2 = dxv * dxv + dyv * dyv;
    double dist = sqrt(static_cast<double>(dist2));
    u32 flyTime = g_buteMgr.GetDwordDef("WarpStone", "FlyTime", 0x5dc);

    m_velocityScale = static_cast<double>(flyTime) / dist;
    m_xDirection = static_cast<double>(dist2) / dist;
    m_yDirection = static_cast<double>(dxv) / dist;

    CSndHost* h = g_gameReg->m_world->m_soundRegistry;
    if (h->m_emitGate == 0) {
        void* fly_ob = 0;
        h->m_10.Lookup("GAME_WARPSTONEFLY", fly_ob);
        LeafCue* fly = static_cast<LeafCue*>(fly_ob);
        if (fly) {
            if (g_sndEnabled != 0 && g_killCueClock - fly->m_14 >= fly->m_18) {
                fly->m_14 = g_killCueClock;
                fly->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
            }
        }
    }

    m_currentX = static_cast<double>(dxv);
    m_currentY = static_cast<double>(dyv);
    return 1;
}

// (CTabzBuilder::BuildTabzDialog @0x10a340 moved OUT to src/Gruntz/SBI_TabzDialogEh.cpp -
// its own retail obj. It needs the OUT-OF-LINE base ctor (retail `call ??0CStatusBarItem`)
// while this TU's builders need the INLINE one, and MSVC5 allows only one spelling per TU.
// Un-merging is what lets both be right; see that file's banner.)

// ===========================================================================
// CStatusBarMgr::UpdateDestructButtonStatusBar @0x10b320
// ===========================================================================
//
// The destruct-button warning blinker: in state 1 it counts the warning frame UP
// toward 6 (then latches state 2), in state 2 it counts DOWN toward 2 (then
// latches state 1); each step is gated on the retrigger clock having elapsed past
// DestructButtonWarningDelay, after which the 64-bit retrigger clock is restamped
// and the new frame pushed into the widget (the +0x30 virtual). State 0 = idle.
// @early-stop
// ~94.7%: logic + the 64-bit compare + every field store are byte-exact. The sole
// residual (in BOTH symmetric cases) is a store-scheduling coin-flip: retail emits
// `mov [+560],retriggerLo; mov [+564],0; mov ecx,[+570](widget)` in source order,
// while MSVC's scheduler hoists the widget load between the two retrigger stores and
// defers the `m_retriggerHi=0` store past it. Tried a single 64-bit retrigger store
// and an inlined widget test; neither pins the store order. Not source-steerable;
// deferred to the final sweep.
RVA(0x0010b320, 0x167)
void CStatusBarMgr::UpdateDestructButtonStatusBar() {
    // The destruct-warning block is this class's own +0x558 state machine:
    // m_destructWarnActive (+0x558) = 3-state (0 idle / 1 warn-down / 2 warn-up),
    // m_modeState (+0x55c) = the warning frame counter, m_destructWarnLast/Delay
    // (+0x560/+0x568) the 64-bit retrigger clock + delay, m_modeNotify (+0x570) the
    // widget (slot-12 Notify == the set-frame hook). (Folded off the EngineLabelBacklog
    // CDestructBlock view onto the canonical class.)
    switch (m_destructWarnActive) {
        case 1: {
            i64 d = static_cast<i64>(static_cast<u32>(g_frameTime)) - *reinterpret_cast<i64*>(&m_destructWarnLast);
            if (d >= *reinterpret_cast<i64*>(&m_destructWarnDelay)) {
                if (++m_modeState >= 6) {
                    m_modeState = 6;
                    m_destructWarnActive = 2;
                }
                m_destructWarnDelay =
                    g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32);
                m_destructWarnDelayHi = 0;
                m_destructWarnLast = g_frameTime;
                m_destructWarnLastHi = 0;
                CSbiSlotPtr* w = m_modeNotify;
                if (w) {
                    w->Notify(m_modeState);
                }
            }
            break;
        }
        case 2: {
            i64 d = static_cast<i64>(static_cast<u32>(g_frameTime)) - *reinterpret_cast<i64*>(&m_destructWarnLast);
            if (d >= *reinterpret_cast<i64*>(&m_destructWarnDelay)) {
                if (--m_modeState <= 2) {
                    m_modeState = 2;
                    m_destructWarnActive = 1;
                }
                m_destructWarnDelay =
                    g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32);
                m_destructWarnDelayHi = 0;
                m_destructWarnLast = g_frameTime;
                m_destructWarnLastHi = 0;
                CSbiSlotPtr* w = m_modeNotify;
                if (w) {
                    w->Notify(m_modeState);
                }
            }
            break;
        }
    }
}

// Enter mode: latch m_modeArmed, conditionally reset the toggle pair, notify m_modeNotify.
RVA(0x0010bb90, 0x3f)
void CStatusBarMgr::SetMode(i32 mode) {
    m_modeArmed = 1;
    if (mode && m_modeState != 7) {
        m_destructWarnActive = 0;
        m_modeState = 1;
        if (m_modeNotify) {
            m_modeNotify->Notify(1);
        }
    }
}

// ---------------------------------------------------------------------------
// CStatusBarMgr::GetActiveValue @0x10bbe0. `this` == [[0x24556c+0x2c]+0x2dc] ==
// g_gameReg->m_curState->
// m_guts, the ONE CStatusBarMgr - and every viewed field is canonical:
// +0x4cc m_extraNotifyArg0, +0x528 m_rezActive, +0x52c m_rezTick, +0x534/+0x538
// the m_ptrPool CPtrArray's m_pData/m_nSize (inline GetSize/GetAt loads).
// Getter: the notify arg when the rez machine is idle; else the active pooled
// cell's value when the tick index is in range, else 0.
RVA(0x0010bbe0, 0x34)
i32 CStatusBarMgr::GetActiveValue() {
    if (m_rezActive == 0) {
        return m_extraNotifyArg0;
    }
    if (m_ptrPool.GetSize() > 0 && m_ptrPool.GetSize() > m_rezTick) {
        return *static_cast<i32*>(m_ptrPool.GetAt(m_rezTick));
    }
    return 0;
}

// Find the rect widget under (x,y) by walking the three hit-test lists (the +0x30
// list, the active-tab list at +tab*0x1c+0x30, then the +0xd8 list); return the
// first enabled rect whose span contains the point, else null. Same point-in-rect
// predicate as HitTest, materialized to a bool per retail.
// @early-stop
// ~95.3%: the walk is now byte-exact instruction-for-instruction (the CPtrList
// GetNext two-copy idiom - cur=n; n=n->m_next; r=cur->m_payload - reproduces retail's
// `mov eax,esi; mov esi,[esi]; mov eax,[eax+8]` in all three loops; raised 94.08->95.27).
// Residual is a pure esi<->edx register-naming
// SWAP: retail loads the loop cursor into the callee-saved esi (after `push esi`) and
// puts the {enabled,hit} temps in edx; MSVC's prologue scheduler pulls the head-load
// `mov edx,[ecx+0x30]` up BEFORE `push esi`, pinning the cursor in edx and forcing the
// temps into esi. The whole allocation cascades from that one scheduling choice - the
// same regalloc coin-flip as ResetWidgets/ClearTabGroup, not source-steerable. Logic
// byte-correct; deferred to the final sweep.
RVA(0x000ffcb0, 0xe2)
CSbiRect* CStatusBarMgr::HitTestRects(i32 x, i32 y) {
    POSITION n = m_tabLists[0].GetHeadPosition();
    while (n) {
        CSbiRect* r = static_cast<CSbiRect*>(m_tabLists[0].GetNext(n));
        if (r && r->m_enabled) {
            i32 hit = x < r->m_xHi && x >= r->m_xLo && y < r->m_yHi && y >= r->m_yLo;
            if (hit) {
                return r;
            }
        }
    }
    CPtrList& tab = m_tabLists[m_activeTab];
    n = tab.GetHeadPosition();
    while (n) {
        CSbiRect* r = static_cast<CSbiRect*>(tab.GetNext(n));
        if (r && r->m_enabled) {
            i32 hit = x < r->m_xHi && x >= r->m_xLo && y < r->m_yHi && y >= r->m_yLo;
            if (hit) {
                return r;
            }
        }
    }
    n = m_tabLists[6].GetHeadPosition();
    while (n) {
        CSbiRect* r = static_cast<CSbiRect*>(m_tabLists[6].GetNext(n));
        if (r && r->m_enabled) {
            i32 hit = x < r->m_xHi && x >= r->m_xLo && y < r->m_yHi && y >= r->m_yLo;
            if (hit) {
                return r;
            }
        }
    }
    return 0;
}

// Initialize the two HUD rects (+0x504, +0x514) via Win32 SetRect through its IAT
// thunk (CSE'd into one load), clear the four highlight-grid groups, latch flags,
// reset the pending-row index.
RVA(0x00106900, 0x8d)
void CStatusBarMgr::InitTabRects() {
    for (i32 i = 0; i < 4; i++) {
        ClearHlCell(0, i);
        ClearHlCell(1, i);
        ClearHlCell(2, i);
    }
    m_machinePhase = 1;
    m_extraNotifyArg0 = 0;
    m_fallActive = 0;
    m_extraNotifyArg1 = 0;
    // authentic: the 4-int rect blocks are Win32 RECTs; overlay the modeled first field.
    SetRect(reinterpret_cast<LPRECT>(&m_fallRectL), 0, 0, 1, 1);
    SetRect(reinterpret_cast<LPRECT>(&m_itemRectL), 0x49, 0xd7, 0x61, 0xef);
    m_pendingHlRow = -1;
}

// Group-A click handler: hit-test the lists; if no rect, drop the tab sprites.
// Otherwise dispatch the rect's slot-9 click; if it is a kind-2 (subtype) widget
// and no hit-test is disabled, play the GAME_TABHIGHLIGHT1 cue on the draw-clock
// window, then forward the offset command id.
RVA(0x000ff9f0, 0xe4)
i32 CStatusBarMgr::ClickToggle(i32 x, i32 y, i32 z) {
    CSbiRect* r = HitTestRects(x, y);
    if (r == 0) {
        ClearTabSprites(-1);
        return 1;
    }
    r->Click24(z, x, y);
    if (r->m_kind != 2) {
        ClearTabSprites(-1);
        return 1;
    }
    i32 cmd = r->m_cmd;
    if (m_hitTestDisabled == 0) {
        if (cmd >= 1 && cmd <= 5) {
            SetTabState(cmd, 2);
        } else {
            ClearTabSprites(0);
        }
    }
    if (m_activeTab == 5) {
        if (r->m_tab == 5) {
            SetTabState(cmd, 2);
        } else {
            ClearTabSprites(5);
        }
    }
    if (m_toggleActive) {
        if (r->m_tab == 6) {
            SetTabState(cmd, 2);
            return 1;
        }
        ClearTabSprites(5);
    }
    return 1;
}

// CSbiResetHost moved to <Gruntz/StatusBarMgr.h>.

// Reset every per-tab widget list (8 notify lists at +0x2c, stride 0x1c) - notify
// each payload, then RemoveAll - then (when keepHost is set) flag the +0x8 host as
// aborted, and finally zero the whole widget/notify field block.
// @early-stop
// ~78%: instruction selection + scheduling are byte-identical end to end (the
// 8-list walk, the host flag-OR, every field zero, both rep-stos memsets), but
// MSVC pins `this` in ebx / the zero in ebp where the recompile uses esi/ebx -
// the same regalloc register-naming coin-flip as ClearTabGroup. Not steerable
// from C; documented regalloc wall, deferred to the final sweep.
RVA(0x00100930, 0x16c)
void CStatusBarMgr::ResetWidgets(i32 keepHost) {
    char* B = reinterpret_cast<char*>(this);
    char* list = B + 0x2c;
    for (i32 outer = 8; outer != 0; outer--) {
        POSITION n = (reinterpret_cast<CPtrList*>(list))->GetHeadPosition();
        while (n) {
            CSbiNotifyTarget* cur = static_cast<CSbiNotifyTarget*>((reinterpret_cast<CPtrList*>(list))->GetNext(n));
            if (cur) {
                cur->Notify(1);
            }
        }
        (reinterpret_cast<CPtrList*>(list))->RemoveAll();
        list += 0x1c;
    }
    if (keepHost) {
        CSbiResetHost* h = *reinterpret_cast<CSbiResetHost**>((B + 8));
        if (h) {
            h->m_40 |= 1;
            h = *reinterpret_cast<CSbiResetHost**>((B + 8));
            h->m_8 |= 0x10000;
        }
    }
    m_tabSprite0 = 0;
    m_tabSprite1 = 0;
    m_tabSprite2 = 0;
    m_tabSprite3 = 0;
    m_tabSprite4 = 0;
    m_tabSprite5 = 0;
    m_tabSprite6 = 0;
    m_tabSprite7 = 0;
    m_tabSprite8 = 0;
    m_tabSprite9 = 0;
    m_tabSprite10 = 0;
    m_tabSprite11 = 0;
    m_tabSprite12 = 0;
    m_tabSprite13 = 0;
    m_tabSprite14 = 0;
    m_8 = 0;
    i32 i;
    for (i = 0; i < 15; i++) {
        m_hitRects[i] = 0;
    }
    for (i = 0; i < 15; i++) {
        m_statObj[i] = 0;
    }
    i32* sp = reinterpret_cast<i32*>((B + 0x204));
    sp[0] = 0;
    sp[1] = 0;
    sp[2] = 0;
    sp[3] = 0;
    sp[4] = 0;
    i32* gp = reinterpret_cast<i32*>((B + 0x308));
    gp[0] = 0;
    gp[1] = 0;
    gp[2] = 0;
    for (i = 0; i < 12; i++) {
        m_hlNotify[i] = 0;
    }
    i32* tp = m_61c;
    tp[0] = 0;
    tp[1] = 0;
    tp[2] = 0;
    tp[3] = 0;
    m_extraNotify0 = 0;
    m_extraNotify1 = 0;
    m_modeNotify = 0;
    m_notify0 = 0;
    m_notify2 = 0;
    m_notify3 = 0;
    m_notify1 = 0;
    *reinterpret_cast<i32*>((B + 0x348)) = 0;
    m_gaugeNotify = 0;
    m_gaugeSink = 0;
    *reinterpret_cast<i32*>((B + 0x358)) = 0;
}

// Exit the alternate (toggle) mode: bail if not active; notify+clear the +0xd4
// list, drop the trailing sprites + the +0x548 flag, then either re-arm the
// active tab (when no handle is pending and the game is not over) or just clear
// the hit-test flag; finish through Deactivate.
RVA(0x0010b210, 0xc5)
void CStatusBarMgr::ExitMode() {
    if (m_toggleActive == 0) {
        return;
    }
    POSITION n = m_tabLists[6].GetHeadPosition();
    while (n) {
        CSbiNotifyTarget* cur = static_cast<CSbiNotifyTarget*>(m_tabLists[6].GetNext(n));
        if (cur) {
            cur->Notify(1);
        }
    }
    m_tabLists[6].RemoveAll();
    i32 handle = m_toggleHandle;
    m_tabSprite11 = 0;
    m_tabSprite12 = 0;
    m_tabSprite13 = 0;
    m_tabSprite14 = 0;
    m_hlBusy = 0;
    if (handle == 0 && g_gameReg->m_134 != 1) {
        if (m_position == 2) {
            RefreshState();
        }
        if (m_activeTab != 5) {
            SetTabState(5, 3);
        }
        SetTab(5, 1);
        Deactivate();
    } else {
        m_hitTestDisabled = 0;
    }
    m_toggleActive = 0;
    m_toggleHandle = 0;
    Deactivate();
}

// Tear down the widgets owned by the active tab: notify+RemoveAll the active-tab
// list, then (switch on the tab index) zero the per-tab field group. Bails when no
// tab is active or the tab index is out of [1,5].
// @early-stop
// ~67%: instruction selection + scheduling are byte-identical (the list walk, the
// RemoveAll, the jump-table dispatch, every per-case field zero), but MSVC pins
// `this` in ebx and the zero constant in ebp (5 callee-saved pushes) where the
// recompile uses esi/ebx (4 pushes) - a regalloc register-naming coin-flip not
// steerable from C. Documented regalloc wall; deferred to the final sweep.
RVA(0x00100b00, 0x139)
void CStatusBarMgr::ClearTabGroup() {
    char* B = reinterpret_cast<char*>(this);
    if (m_activeTab == 0) {
        return;
    }
    CPtrList& tab = m_tabLists[m_activeTab];
    POSITION n = tab.GetHeadPosition();
    while (n) {
        CSbiNotifyTarget* cur = static_cast<CSbiNotifyTarget*>(tab.GetNext(n));
        if (cur) {
            cur->Notify(1);
        }
    }
    (reinterpret_cast<CPtrList*>((B + m_activeTab * 0x1c + 0x2c)))->RemoveAll();
    switch (m_activeTab) {
        case 1:
            m_tabSprite5 = 0;
            m_tabSprite6 = 0;
            m_tabSprite7 = 0;
            m_tabSprite8 = 0;
            m_tabSprite9 = 0;
            m_tabSprite10 = 0;
            m_modeNotify = 0;
            break;
        case 2: {
            for (i32 i = 0; i < 15; i++) {
                m_statObj[i] = 0;
            }
            break;
        }
        case 3: {
            i32* p = m_61c;
            p[0] = 0;
            p[1] = 0;
            p[2] = 0;
            p[3] = 0;
            break;
        }
        case 4: {
            i32* p = reinterpret_cast<i32*>((B + 0x204));
            p[0] = 0;
            p[1] = 0;
            p[2] = 0;
            p[3] = 0;
            p[4] = 0;
            m_gaugeNotify = 0;
            m_gaugeSink = 0;
            break;
        }
        case 5: {
            i32* p = reinterpret_cast<i32*>((B + 0x308));
            p[0] = 0;
            p[1] = 0;
            p[2] = 0;
            *reinterpret_cast<i32*>((B + 0x348)) = 0;
            for (i32 i = 0; i < 12; i++) {
                m_hlNotify[i] = 0;
            }
            m_notify0 = 0;
            m_notify2 = 0;
            m_notify3 = 0;
            m_notify1 = 0;
            m_extraNotify0 = 0;
            m_extraNotify1 = 0;
            break;
        }
    }
}

// (0xe72f0 "ConfigureRect" -> CSBI_ImageSet::SetupImage in SBI_ImageSet.cpp
// (vtbl 0x1eac4c slot [11]) - dossier #16.)

// Drive the tab-selection sprites: require the five bank-A sprites all present,
// then switch on the tab id to Show the selected sprite (with flag) and Hide the
// rest, gated per case by m_hlBusy (busy => early-return 1). Banks A (1-5), B
// (0x1f4-0x1fa), C (0x324/0x325), D (0x327/0x328). `state` is the sprite index arg.
// @early-stop
// ~85.5% (1305 B): the 5-sprite guard, both jump-table dispatches and every
// Show/Hide sequence are byte-correct; the residual is the shared-tail merging
// retail performs across adjacent cases (the `jmp` convergence at the m_tabSprite3/m_tabSprite4
// and bank-B tails) which the recompile duplicates per case instead. A
// scheduling/tail-merge wall on a big switch; not steerable from C. Deferred.
RVA(0x00100d70, 0x519)
i32 CStatusBarMgr::SetTabState(i32 tab, i32 state) {
    if (m_tabSprite0 == 0 || m_tabSprite1 == 0 || m_tabSprite2 == 0 || m_tabSprite3 == 0
        || m_tabSprite4 == 0) {
        return 0;
    }
    switch (tab) {
        case 1:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->SetState(state, 1);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->ProbeState(state);
            return 1;
        case 2:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->SetState(state, 1);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->ProbeState(state);
            return 1;
        case 3:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->SetState(state, 1);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->ProbeState(state);
            return 1;
        case 4:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->SetState(state, 1);
            m_tabSprite4->ProbeState(state);
            return 1;
        case 5:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->SetState(state, 1);
            return 1;
        case 0x1f4:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->SetState(state, 1);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->ProbeState(state);
            return 1;
        case 0x1f5:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->SetState(state, 1);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->ProbeState(state);
            return 1;
        case 0x1f6:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->SetState(state, 1);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->ProbeState(state);
            return 1;
        case 0x1f7:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->SetState(state, 1);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->ProbeState(state);
            return 1;
        case 0x1f8:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->SetState(state, 1);
            m_tabSprite10->ProbeState(state);
            return 1;
        case 0x1f9:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->SetState(state, 1);
            return 1;
        case 0x1fa:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite10->SetState(state, 1);
            return 1;
        case 0x324:
            if (m_tabSprite11) {
                m_tabSprite11->SetState(state, 1);
            }
            m_tabSprite12->ProbeState(state);
            return 1;
        case 0x325:
            if (m_tabSprite11) {
                m_tabSprite11->ProbeState(state);
            }
            m_tabSprite12->SetState(state, 1);
            return 1;
        case 0x327:
            m_tabSprite13->SetState(state, 1);
            m_tabSprite14->ProbeState(state);
            return 1;
        case 0x328:
            m_tabSprite13->ProbeState(state);
            m_tabSprite14->SetState(state, 1);
            return 1;
    }
    return 1;
}

// Enter a highlight handle at the pending row m_pendingHlRow: pick the group from arg1's
// range, then either (arg0 != 0) clear the group's row m_pendingHlRow and shift every set
// row below it down by one (inserting at the top), or (arg0 == 0) just latch the
// handle into row m_pendingHlRow's cell. Always re-notify and reset m_pendingHlRow.
RVA(0x00106820, 0xa8)
void CStatusBarMgr::EnterHlRow(i32 shift, i32 key) {
    if (m_pendingHlRow == -1) {
        return;
    }
    i32 group;
    if (key >= 0x22) {
        group = 2;
    } else {
        group = (key >= 0x17);
    }
    if (shift != 0) {
        ClearHlCell(group, m_pendingHlRow);
        for (i32 row = m_pendingHlRow - 1; row >= 0; row--) {
            CSbiHlRow* cell = &m_hlGrid[row + group * 4];
            if (cell->m_state == 1) {
                m_hlGrid[row + group * 4 + 1].m_state = 1;
                cell[1].m_handle = cell->m_handle;
                cell->m_state = 0;
                cell->m_handle = 0;
            }
        }
    } else {
        m_hlGrid[m_pendingHlRow + group * 4].m_handle = key;
    }
    NotifyAllSlots();
    m_pendingHlRow = -1;
}

// Group-A highlight click: hit-test the rect under (x,y); dispatch its slot-7
// click; then, only for a tab-1 widget whose command is in the highlight range
// and when nothing blocks it, play the GAME_TABHIGHLIGHT1 cue + activate the
// cursor for the offset command. The fallback (any gate fails) drives the tab
// highlight directly. Single-exit, success-deepest (shared fallback tail).
// @early-stop
// ~89%: the whole hit-test / dispatch / gate chain / cue play / cursor activation
// is byte-correct; the residual is two documented walls - the bare `return 1`
// null-path tail-merges with the other identical `return 1` epilogues where retail
// duplicates them inline (identical-return-epilogue-tailmerge), and retail tests
// the tab via `dec; jne` where the recompile uses `cmp 1; jne`. Logic correct,
// deferred to the final sweep.
RVA(0x000ff850, 0x121)
i32 CStatusBarMgr::ClickHilite(i32 a, i32 x, i32 y) {
    CSbiRect* r = HitTestRects(x, y);
    if (r == 0) {
        return 1;
    }
    r->Click1c(a, x, y);
    i32 cmd = r->m_cmd;
    if (r->m_tab == 1 && m_hitTestDisabled == 0 && g_gameReg->m_cmdGrid->m_groupFlag != 0
        && cmd >= 0x13b && cmd <= 0x149) {
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
        if (host->m_30 == 0) {
            void* found = 0;
            CMapStringToPtr* map = &host->m_10; // CMapStringToPtr per the mfc_class audit (the facet said Ob - the documented band inversion)
            map->Lookup("GAME_TABHIGHLIGHT1", found);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                    if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                        p->m_14 = g_killCueClock;
                        p->m_10->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
        ActivateCursor(cmd - 0x13b, 1);
        return 1;
    }
    UpdateStatusBarTabHighlight(a, x, y);
    return 1;
}

// Clear stat[idx]: disable its hit-rect (zero +0x44 + m_enabled); on the active
// (tab 1) screen notify the stat object and play the GAME_STATZTABTOGGLE cue on
// the draw-clock window; then clear the stat flag. Returns 1.
RVA(0x00104f90, 0xa8)
i32 CStatusBarMgr::ClearStat(i32 idx) {
    CSbiRect* r = m_hitRects[idx];
    if (r != 0) {
        r->m_toggleValue = 0;
        r->m_enabled = 0;
        if (m_activeTab == 1) {
            (reinterpret_cast<CStatusBarMgr*>(m_statObj[idx]))->ResetGroupA();
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
            if (host->m_30 == 0) {
                void* found = 0;
                CMapStringToPtr* map = &host->m_10; // CMapStringToPtr per the mfc_class audit (the facet said Ob - the documented band inversion)
                map->Lookup("GAME_STATZTABTOGGLE", found);
                if (found) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0) {
                        CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                        if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                            p->m_14 = g_killCueClock;
                            p->m_10->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
        }
    }
    m_statFlags[idx] = 0;
    return 1;
}

// Position a falling-item status bar: bail unless a highlight row is pending; hit
// the rect under (x,y) and require it be a falling-item widget (cmd 0xce/0xd0);
// clamp x into the rect's inset span, convert to item-local coords, drive the
// falling-item bar, then arm highlight row arg2.
// @early-stop
// ~70%: the gate chain, the hit-test, the cmd filter, the x-clamp, the coord
// conversion and both calls are byte-correct, but retail reserves a 0x10 RECT
// frame and spills the rect's yLo/yHi into it (dead stores left by an inlined
// RECT-staging the optimizer half-removed). MSVC DCEs any unread local I add, so
// the spills are not reproducible from C - a dead-store/scheduling wall; deferred.
RVA(0x00107920, 0xb7)
i32 CStatusBarMgr::SetFallRect(i32 x, i32 y, i32 item) {
    char* B = reinterpret_cast<char*>(this);
    if (m_pendingHlRow == -1) {
        return 0;
    }
    CSbiRect* r = HitTestRects(x, y);
    if (r == 0) {
        return 0;
    }
    if (r->m_cmd != 0xce && r->m_cmd != 0xd0) {
        return 0;
    }
    i32* rc = &r->m_xLo; // rect cursor {xLo,yLo,xHi,yHi}
    i32 cx = x;
    i32 lo = rc[0] + 0x1b;
    i32 xHi = rc[2];
    if (x < lo) {
        cx = lo;
    } else if (x > xHi - 0x1a) {
        cx = xHi - 0x1a;
    }
    i32 localX = cx - *reinterpret_cast<i32*>((B + 0x10));
    i32 localY = 0x1b3 - *reinterpret_cast<i32*>((B + 0x14));
    UpdateFallingItemStatusBar(item, localX, localY);
    EnterHlRow(1, item);
    return 1;
}

// Activate a ready slot: bail if the highlight sub-manager is busy. With idx == -1
// find the first slot whose state is 2 (ready); otherwise require slot idx to be
// ready. Validate handle 0x66, play the GAME_TABHIGHLIGHT1 cue on the draw-clock
// window, then latch the active slot, mark its value, and notify its pointer.
// @early-stop
// ~68%: both the find-first-ready and the indexed branch - the handle validation,
// the cue play, the activeSlot/value latch and the slot notify - are byte-correct;
// the residual is the identical-return-epilogue wall (the three `return 0` paths
// tail-merge where retail inlines them, flipping the busy-gate je/jne) plus a
// one-instruction eager read in the find-slot loop. Documented walls; deferred.
RVA(0x0010b930, 0x1a7)
i32 CStatusBarMgr::ActivateSlot(i32 idx) {
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_4f0 != 0) {
        return 0;
    }
    if (idx == -1) {
        i32 slot = 0;
        while (m_slots[slot].m_state != kSlotReady) {
            slot++;
            if (slot >= 5) {
                return 0;
            }
        }
        if (!ResolveHandle(0x66)) {
            return 0;
        }
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
        if (host->m_30 == 0) {
            void* found = 0;
            CMapStringToPtr* map = &host->m_10; // CMapStringToPtr per the mfc_class audit (the facet said Ob - the documented band inversion)
            map->Lookup("GAME_TABHIGHLIGHT1", found);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                    if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                        p->m_14 = g_killCueClock;
                        p->m_10->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
        m_activeSlot = slot;
        m_slots[slot].m_value = 1;
        if (m_slotNotify[slot]) {
            m_slotNotify[slot]->Notify(1);
        }
        return 1;
    }
    if (m_slots[idx].m_state != kSlotReady) {
        return 0;
    }
    if (!ResolveHandle(0x66)) {
        return 0;
    }
    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
    if (host->m_30 == 0) {
        void* found = 0;
        CMapStringToPtr* map = &host->m_10; // CMapStringToPtr per the mfc_class audit (the facet said Ob - the documented band inversion)
        map->Lookup("GAME_TABHIGHLIGHT1", found);
        if (found) {
            i32 gate = g_sndEnabled;
            i32 item = g_sndCueTag;
            if (gate != 0) {
                CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                    p->m_14 = g_killCueClock;
                    p->m_10->ConfigureItem(item, 0, 0, 0);
                }
            }
        }
    }
    m_activeSlot = idx;
    m_slots[idx].m_value = 1;
    if (m_slotNotify[idx]) {
        m_slotNotify[idx]->Notify(1);
    }
    return 1;
}

// CSbiLayer/CSbiRenderObj/CSbiPtrColl2/CSbiFreeNode moved to <Gruntz/StatusBarMgr.h>.

// 0xfe3e0 - SetState(state): if the mode gate (m_hlBusy) is up, no-op (return 1);
// if already in `state`, return 1. For the subtype-2 cursor state, run the
// activation probe (bail 0 on failure) and mirror the subtype tag into m_4;
// otherwise fire the plain notify. Then latch the new state into slot 0 and tell
// the highlight sub-manager (new, old). Returns 1.
RVA(0x000fe3e0, 0x55)
i32 CStatusBarMgr::SetState(i32 state) {
    if (m_hlBusy != 0) {
        return 1;
    }
    i32 old = m_position;
    if (old == state) {
        return 1;
    }
    if (state == 2) {
        if (Activate() == 0) { // 0x104dd0 - the subtype-2 lazy sprite-create probe
            return 0;
        }
        m_4 = m_position;
    } else {
        Deactivate();
    }
    old = m_position;
    m_position = state;
    (static_cast<CPlay*>(g_gameReg->m_curState))->PositionBridgeToggle(state, old);
    return 1;
}

// 0xfe460 - RefreshA: the armed-refresh rect-setup variant (RefreshState's m_4==1
// leg). Same call shape as winapi_0fe520_SetRect but gated on state!=1 with the fixed
// 0xa0-wide x full-height rect and SetState(1). Was the ScreenRegionMgr::Open fake-view
// (interleaved in this .text at 0x0fe4xx; the ScreenRegionMgr class was our invention).
RVA(0x000fe460, 0x83)
i32 CStatusBarMgr::RefreshA() {
    if (m_hlBusy == 0 && m_position != 1) {
        ResetWidgets(1);
        SetRect(reinterpret_cast<LPRECT>(&m_10), 0, 0, 0xa0, 0x1e0);
        SetState(1);
        (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
        if (BuildStatusBarTabs() == 0) {
            g_gameReg->ReportError(kActivateErrId, 0x448);
            return 0;
        }
        SetTabState(m_activeTab, 3);
    }
    return 1;
}

// 0xfe520 - place the rect-only HUD panel: gated on the mode (m_hlBusy) and the
// offset-0 subtype tag; pre-teardown notify, set the right-anchored 0xa0-wide
// full-height rect (+0x10) from the view width (g_gameReg->m_modeW), notify, refresh
// the highlight sub-manager, then probe-and-apply (m_10c). On probe failure log
// the placement error and bail. Returns 1.
RVA(0x000fe520, 0xa9)
i32 CStatusBarMgr::winapi_0fe520_SetRect() {
    if (m_hlBusy != 0) {
        return 1;
    }
    if (m_position == 0) {
        return 1;
    }
    ResetWidgets(1);
    // Retail reads the view extent (m_modeW,m_modeH) as a POINT but only uses x for the
    // rect; the y store survives as a dead 8-byte-frame spill. `volatile` reproduces
    // that preserved store (a plain local is dead-eliminated by MSVC5 /O2).
    i32 w = g_gameReg->m_modeW;
    volatile POINT pt;
    pt.y = g_gameReg->m_modeH;
    SetRect(reinterpret_cast<LPRECT>(&m_10), w - 0xa0, 0, w, 0x1e0);
    SetState(0);
    (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
    if (BuildStatusBarTabs() == 0) {
        g_gameReg->ReportError(kActivateErrId, 0x449);
        return 0;
    }
    SetTabState(m_activeTab, 3);
    return 1;
}

// 0xfe600 - HideRect: the hide/off-screen rect-setup variant (state!=2 gate) - moves
// the +0x10 rect off-screen (-1,-1,-1,-1), latches state 2, resets the play viewport.
// Was the ScreenRegionMgr::Reset fake-view (interleaved in this .text; reached via an
// ILT jmp-thunk, no direct in-TU caller).
RVA(0x000fe600, 0x49)
i32 CStatusBarMgr::HideRect() {
    if (m_hlBusy == 0 && m_position != 2) {
        ResetWidgets(1);
        SetRect(reinterpret_cast<LPRECT>(&m_10), -1, -1, -1, -1);
        SetState(2);
        (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
    }
    return 1;
}

// 0xfe670 - RefreshState: gated on m_hlBusy (return 1) and the subtype-2 tag
// (return 1 when not cursor); for the cursor subtype, tail-call the armed (m_4==1)
// or idle refresh path.
RVA(0x000fe670, 0x2b)
i32 CStatusBarMgr::RefreshState() {
    if (m_hlBusy != 0) {
        return 1;
    }
    if (m_position != 2) {
        return 1;
    }
    if (m_4 == 1) {
        return RefreshA();
    }
    return winapi_0fe520_SetRect();
}

// 0xfe860 - SetSpritePos(x, y): push the position into the render object
// (m_8->m_5c/m_60) and mirror it into m_24/m_28. Bails (0) if there is no render
// object. Returns 1.
// @early-stop
// ~96.7%: every store/offset is byte-correct; the residual is a regalloc choice -
// retail keeps `y` unloaded until after the m_8 reload (y in edx, m_8 in esi),
// while the recompile parks y in esi loaded early. Not source-steerable; deferred.
RVA(0x000fe860, 0x2d)
i32 CStatusBarMgr::SetSpritePos(i32 x, i32 y) {
    CSbiRenderObj* r = reinterpret_cast<CSbiRenderObj*>(m_8);
    if (r == 0) {
        return 0;
    }
    r->m_5c = x;
    (reinterpret_cast<CSbiRenderObj*>(m_8))->m_60 = y;
    m_28 = y;
    m_24 = x;
    return 1;
}

// 0xfe8a0 - HitTestLayer(x, y): test the point against the render object's layer
// rect - origin (m_198->m_10/m_14) plus the position-relative inset
// (m_198->m_18/m_1c offset by m_5c/m_60). Returns 1 inside, 0 outside.
RVA(0x000fe8a0, 0x4e)
i32 CStatusBarMgr::HitTestLayer(i32 x, i32 y) {
    CSbiRenderObj* r = reinterpret_cast<CSbiRenderObj*>(m_8);
    CSbiLayer* L = r->m_layer;
    i32 xlo = r->m_5c - L->m_18;
    i32 ylo = r->m_60 - L->m_1c;
    i32 xhi = L->m_10 + xlo;
    i32 yhi = L->m_14 + ylo;
    if (x >= xhi || x < xlo || y >= yhi || y < ylo) {
        return 0;
    }
    return 1;
}

// 0x108410 - InsertPtr(a, b): pull a node off the engine free-list, fill it with
// (a, b), then insert it into the m_ptrPool pooled-ptr collection - at the first slot
// whose key (m_4) exceeds b, or appended at the end. Returns 1.
// @early-stop
// ~75.9%: every operation/offset is byte-correct; the residual is regalloc + block
// layout - retail pins the free-list head in edx and reads arg `a` at the top
// (before the pushes), and falls through from the scan loop into the append block
// (jl loop), while the recompile colors the head into eax, loads `a` late, and
// reaches append via an extra jmp. Not source-steerable; deferred to the final sweep.
RVA(0x00108410, 0x8e)
i32 CStatusBarMgr::InsertPtr(i32 a, i32 b) {
    CSbiFreeNode* head = reinterpret_cast<CSbiFreeNode*>(g_coordPool.m_freeHead);
    CSbiFreeNode* node = 0;
    if (head->m_0 != 0) {
        node = reinterpret_cast<CSbiFreeNode*>(&head->m_4);
        node->m_0 = a;
        node->m_4 = b;
        g_coordPool.m_freeHead
            = reinterpret_cast<CoordPoolNode*>((reinterpret_cast<CSbiFreeNode*>(g_coordPool.m_freeHead))->m_0);
    }
    i32 n = m_ptrPool.GetSize();
    i32 i = 0;
    if (n > 0) {
        CSbiFreeNode** t = reinterpret_cast<CSbiFreeNode**>(m_ptrPool.GetData());
        do {
            CSbiFreeNode* e = *t;
            if (e != 0 && b < e->m_4) {
                m_ptrPool.InsertAt(i, node, 1);
                return 1;
            }
            i++;
            t++;
        } while (i < n);
    }
    m_ptrPool.SetAtGrow(m_ptrPool.GetSize(), node);
    return 1;
}

// 0x10bb50 - ReportTab(tab): log the tab with the (0x4f, 0x1b3) id pair, then
// apply it on `this` as (1, tab). Both helpers are reloc-masked siblings.
RVA(0x0010bb50, 0x24)
void CStatusBarMgr::ReportTab(i32 tab) {
    UpdateFallingItemStatusBar(tab, 0x4f, 0x1b3);
    EnterHlRow(1, tab);
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// SbiTabRect/CSbiTab/CSbiRectSub/CSBI_MenuItem/SbiTabFrame*/CTabList moved to
// <Gruntz/StatusBarMgr.h>.

// 0xffde0 - build the five top-level status-bar tabs (STATZ/GRUNTZ/RESOURCE/
// MULTIPLAYER/GAMETAB) plus three rect-only sub-widgets, once (m_tabsBuilt gate). Each
// widget is heap-allocated, its retail vtable + type tag stamped, then Setup/
// Configure'd from the tab base coords (m_10/m_14) + a per-widget geometry rect
// (and, for the tabs, an asset key + type code); on success it is appended to the
// +0x2c list and stashed in its per-tab slot (m_tabSprite0..m_tabSprite4). Built under a /GX EH
// frame (a just-created item is deleted if its setup throws / returns 0). The
// MULTIPLAYER tab gets a mode-specific configure in a battlez game (m_134==1).
// Finishes with three validity probes; returns 0 on any failure, else latches
// m_tabsBuilt = 1 and returns 1.
// @early-stop
// /GX EH-frame wall - identical archetype to StatusBarGameMenu::BuildGameMenu (also
// @early-stop ~37%). The dominant residual is structural: retail carries a /GX frame +
// a per-item incrementing EH state machine because each item's operator-new + ctor is
// inlined and its setup call is a delete-on-throw region; modeled here with real
// polymorphic sub-widget classes (CSbiRectSub / CSBI_MenuItem, whose ctors MSVC
// auto-stamps - no manual vtable stamp), cl's EH bookkeeping + the by-value rect arg
// scheduling diverge, shifting the frame. The rect sub-widget's true class CStatusBarMgr
// collides with the HOST name so its vtable stays reloc-masked; the CSBI_MenuItem vtable
// is now correctly named. Logic complete; deferred to the final sweep (re-attack once
// the CSBI_* item ctors land and the frame can be reproduced).
// @early-stop (~57%, DOWN from 71.6 - and the drop is the point)
// The old 71.6% was propped up by a LIE: the leaves derived from a fabricated 13-slot
// CSbiTab base where retail has 11 (rect widget) and 12 (menu item). Deleting it fixes the
// emitted vtables and kills 13 unresolved externals; the number falls because the wrong
// vtable happened to score better. Correct shape over the number - the vtable was a
// shipping bug, not a cosmetic one.
//
// THE REMAINING GAP IS ARG MATERIALIZATION, AND I DID NOT CRACK IT. Retail builds the
// slot-2 rect argument as a BY-VALUE STRUCT copied into the outgoing frame -
// `sub esp,0x10; mov ecx,esp; mov [ecx],..; mov [ecx+4],..; mov [ecx+8],..; mov [ecx+0xc],..`
// - not four `push`es, which is what the canonical CStatusBarItem::Setup(10 loose ints)
// makes the caller emit. Callee-side the two are ABI-identical (10 dwords, same `ret`), so
// ONLY the call site can distinguish them, and it says by-value.
// I TRIED the obvious fix - retyping slot 2 as Setup(a1,a2,a3,a4, SbiRect rc, a9,a10)
// across the family - and it made things WORSE, not better (this fn 56.7 -> 52.2, and it
// cratered CSBI_MenuItem::DecCounter 92 -> 74). So the by-value observation is right but
// that spelling is not the source shape; REVERTED rather than banked. Whoever picks this up
// should start from the caller bytes, not from my guess. The struct-copy is real; the
// signature that produces it is not yet known.
// Retail INLINES the CStatusBarItem base ctor at this function's `new` sites (BuildTabzDialog,
// which needs the out-of-line CALL, is in its own TU). This TU leaves SBI_ITEM_OWN_CTOR
// off, so the base ctor inlines here - the retail spelling.
RVA(0x000ffde0, 0x5b1)
i32 CStatusBarMgr::BuildStatusBarTabs() {
    if (m_tabsBuilt != 0) {
        return 1;
    }
    if (m_c == 0) {
        return 0;
    }
    i32 bx = m_10;
    i32 by = m_rect14.m_0;
    CDDrawSurfaceMgr* code = m_c;
    CStatusBarItem* it;

    // ---- rect-only sub-widget A (id 0x259) ----
    it = new CSbiRectSub;
    if (!it->Setup(
            reinterpret_cast<i32>(this),
            reinterpret_cast<i32>(code),
            0x259,
            0,
            SbiRect(bx + 0x7c, by + 0xad, bx + 0x88, by + 0xb9),
            0,
            -1
        )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);

    // ---- rect-only sub-widget B (id 0x25a) ----
    it = new CSbiRectSub;
    if (!it->Setup(
            reinterpret_cast<i32>(this),
            reinterpret_cast<i32>(code),
            0x25a,
            0,
            SbiRect(bx + 0x8a, by + 0xb9, bx + 0x96, by + 0xc7),
            0,
            -1
        )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);

    // ---- rect-only sub-widget C (id 0x25b) ----
    it = new CSbiRectSub;
    if (!it->Setup(
            reinterpret_cast<i32>(this),
            reinterpret_cast<i32>(code),
            0x25b,
            0,
            SbiRect(bx + 0x83, by + 0xbb, bx + 0x8f, by + 0xc7),
            0,
            -1
        )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);

    // ---- STATZTAB (menu item, type 1) ----
    it = new CSBI_MenuItem;
    if (!(static_cast<CSBI_MenuItem*>(it))
             ->SetupImage(
                 this,
                 static_cast<CDDrawSurfaceMgr*>(code),
                 1,
                 0,
                 SbRect(bx + 0x42, by + 0x82, bx + 0x62, by + 0x99),
                 "GAME_STATUSBAR_TABZ_STATZTAB",
                 -1,
                 0
             )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);
    m_tabSprite0 = static_cast<CSBI_MenuItem*>(it);

    // ---- GRUNTZTAB (menu item, type 2) ----
    it = new CSBI_MenuItem;
    if (!(static_cast<CSBI_MenuItem*>(it))
             ->SetupImage(
                 this,
                 static_cast<CDDrawSurfaceMgr*>(code),
                 2,
                 0,
                 SbRect(bx + 0x04, by + 0x82, bx + 0x24, by + 0x99),
                 "GAME_STATUSBAR_TABZ_GRUNTZTAB",
                 -1,
                 0
             )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);
    m_tabSprite2 = static_cast<CSBI_MenuItem*>(it);

    // ---- RESOURCETAB (menu item, type 3) ----
    it = new CSBI_MenuItem;
    if (!(static_cast<CSBI_MenuItem*>(it))
             ->SetupImage(
                 this,
                 static_cast<CDDrawSurfaceMgr*>(code),
                 3,
                 0,
                 SbRect(bx + 0x24, by + 0x82, bx + 0x44, by + 0x99),
                 "GAME_STATUSBAR_TABZ_RESOURCETAB",
                 -1,
                 0
             )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);
    m_tabSprite1 = static_cast<CSBI_MenuItem*>(it);

    // ---- MULTIPLAYERTAB (menu item, type 4) ----
    it = new CSBI_MenuItem;
    if (!(static_cast<CSBI_MenuItem*>(it))
             ->SetupImage(
                 this,
                 static_cast<CDDrawSurfaceMgr*>(code),
                 4,
                 0,
                 SbRect(bx + 0x60, by + 0x82, bx + 0x80, by + 0x99),
                 "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB",
                 -1,
                 0
             )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);
    m_tabSprite3 = static_cast<CSBI_MenuItem*>(it);
    if (g_gameReg->m_134 == 1) {
        CSBI_MenuItem* mp = static_cast<CSBI_MenuItem*>(it);
        mp->m_34 = 4;
        CImageSet* f = mp->m_38;
        CImage* v;
        if (f != 0 && f->m_minIndex <= 4 && f->m_maxIndex >= 4) {
            v = static_cast<CImage*>(f->m_items.GetAt(4)); // the ex-SbiTabFrame 'm_14->m_10' hop == frames[4]
        } else {
            v = 0;
        }
        mp->m_30 = v;
        mp->m_4 = 0;
        mp->SetSubtype(); // slot 10 (the view called it "Refresh")
    }

    // ---- GAMETAB (menu item, type 5; inline ctor) ----
    it = new CSBI_MenuItem;
    if (!(static_cast<CSBI_MenuItem*>(it))
             ->SetupImage(
                 this,
                 static_cast<CDDrawSurfaceMgr*>(code),
                 5,
                 0,
                 SbRect(bx + 0x7e, by + 0x82, bx + 0x9e, by + 0x99),
                 "GAME_STATUSBAR_TABZ_GAMETAB",
                 -1,
                 0
             )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);
    m_tabSprite4 = static_cast<CSBI_MenuItem*>(it);

    if (Probe2e69() == 0) {
        return 0;
    }
    if (RefreshState() == 0) {
        return 0;
    }
    if (Probe41a1() == 0) {
        return 0;
    }
    m_tabsBuilt = 1;
    return 1;
}

// The inlined game RNG (shared with BootyWalkAnim / CGruntSpawnConfig): the LCG
// seed + the seed-init flag (bit 0) + the timeGetTime entry pointer. Bound (DATA)
// by BootyWalkAnim / m5_SoundTickCtor; referenced here as reloc-masked externs.

// MSVC-style LCG rand() (x = x*214013 + 2531011), lazily seeded from timeGetTime.
// The range test is first so the divisor is proven non-zero (no idiv guard) and
// the rand-gen is duplicated across both arms exactly as retail inlines it: range
// 0 -> a 0/1 coin, else 1..range. Inlined at each of the roulette's four nodes.
static __inline i32 WapRand(i32 range) {
    u32 x;
    if (range == 0) {
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            x = ::timeGetTime();
        } else {
            x = g_randSeed;
        }
        g_randSeed = x * 214013 + 2531011;
        return (static_cast<u32>(g_randSeed) >> 16) & 1;
    }
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        x = ::timeGetTime();
    } else {
        x = g_randSeed;
    }
    g_randSeed = x * 214013 + 2531011;
    return ((static_cast<i32>(g_randSeed) >> 16) & 0x7fff) % range + 1;
}

// 0x107d00 - the falling-item weighted-random roulette. When the game is over
// (m_134==1), return the first pooled item's id to the free-list (or notify the
// falling-item sink with 0). Otherwise pick an item type from the running-sum
// battlez percent table via a three-tier weighted random (a top-level toolz/toyz/
// brickz split, then a per-category item roll), map the roll to the item-type id,
// place the falling-item HUD rect (SetRect + the notify-object rect block), and
// tick the pending fall count. Returns 1.
// @early-stop
// 88.4%: inlined-LCG-rand + weighted-tree archetype (the twin left as a pure 0%
// stub at CGruntSpawnConfig::PickWeighted 0x11bee0). Logic + the whole three-tier
// tree + the LCG lea-chain are byte-faithful (verified llvm-objdump -dr). Residual
// is a callee-saved-reg CSE pick: retail caches g_randSeeded in bl (a 4th push ebx)
// and the ::timeGetTime ptr in edi (mov edi,[ptr]; call edi), while cl emits a
// 3-register solution (flag in cl, call [ptr] indirect) and a `mov ecx,[m_134];cmp`
// vs retail's `cmp [m_134],1` direct memory compare. Plus the ?g_gameReg vs
// _g_mgrSettings shared-global DIR32 naming tail. Not source-steerable under /O2.
RVA(0x00107d00, 0x591)
i32 CStatusBarMgr::winapi_107d00_SetRect() {
    i32 result;
    if (g_gameReg->m_134 == 1) {
        if (m_ptrPool.GetSize() > 0) {
            void* p = m_ptrPool.GetData()[0];
            result = *static_cast<i32*>(p);
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
            m_ptrPool.RemoveAt(0, 1);
        } else {
            result = 0;
            if (m_extraNotify0) {
                m_extraNotify0->Notify(0);
            }
        }
    } else {
        i32 r1 = WapRand(m_battlezPct[2]);
        if (r1 <= m_battlezPct[0]) {
            i32 r = WapRand(m_battlezPct[37]);
            if (r <= m_battlezPct[17]) {
                result = 1;
            } else if (r <= m_battlezPct[18]) {
                result = 2;
            } else if (r <= m_battlezPct[19]) {
                result = 3;
            } else if (r <= m_battlezPct[20]) {
                result = 4;
            } else if (r <= m_battlezPct[21]) {
                result = 5;
            } else if (r <= m_battlezPct[22]) {
                result = 6;
            } else if (r <= m_battlezPct[23]) {
                result = 7;
            } else if (r <= m_battlezPct[24]) {
                result = 8;
            } else if (r <= m_battlezPct[25]) {
                result = 9;
            } else if (r <= m_battlezPct[26]) {
                result = 10;
            } else if (r <= m_battlezPct[27]) {
                result = 11;
            } else if (r <= m_battlezPct[28]) {
                result = 12;
            } else if (r <= m_battlezPct[29]) {
                result = 13;
            } else if (r <= m_battlezPct[30]) {
                result = 14;
            } else if (r <= m_battlezPct[31]) {
                result = 15;
            } else if (r <= m_battlezPct[32]) {
                result = 16;
            } else if (r <= m_battlezPct[33]) {
                result = 17;
            } else if (r <= m_battlezPct[34]) {
                result = 18;
            } else if (r <= m_battlezPct[35]) {
                result = 19;
            } else {
                result = 0x15 + (r > m_battlezPct[36]);
            }
        } else if (r1 <= m_battlezPct[1]) {
            i32 r = WapRand(m_battlezPct[16]);
            if (r <= m_battlezPct[7]) {
                result = 0x17;
            } else if (r <= m_battlezPct[8]) {
                result = 0x18;
            } else if (r <= m_battlezPct[9]) {
                result = 0x19;
            } else if (r <= m_battlezPct[10]) {
                result = 0x1a;
            } else if (r <= m_battlezPct[11]) {
                result = 0x1b;
            } else if (r <= m_battlezPct[12]) {
                result = 0x1c;
            } else if (r <= m_battlezPct[13]) {
                result = 0x1d;
            } else if (r <= m_battlezPct[14]) {
                result = 0x1e;
            } else {
                result = 0x1f + (r > m_battlezPct[15]);
            }
        } else {
            i32 r = WapRand(m_battlezPct[6]);
            if (r <= m_battlezPct[3]) {
                result = 0x23;
            } else if (r <= m_battlezPct[4]) {
                result = 0x24;
            } else {
                result = 0x25 + (r > m_battlezPct[5]);
            }
        }
        if (result == 0x14) {
            result = 5;
        }
    }
    m_extraNotifyArg1 = result;
    m_machinePhase = 1;
    SetRect(reinterpret_cast<LPRECT>(&m_itemRectL), 0x49, 0xd7, 0x61, 0xef);
    if (m_extraNotify0) {
        i32 x = m_10;
        i32 y = m_rect14.m_0;
        m_extraNotify0->m_rect14[0] = m_itemRectL + x;
        m_extraNotify0->m_rect14[1] = m_itemRectT + y;
        m_extraNotify0->m_rect14[2] = m_itemRectR + x;
        m_extraNotify0->m_rect14[3] = m_itemRectB + y;
    }
    RefreshFallRect();
    i32 c = m_rezTick;
    m_rezActive = 0;
    if (c > 0) {
        m_rezTick = c - 1;
        FallItemTick();
    }
    return 1;
}

// 0xfdc00 - reset the item to the multiplayer-item-percent (battlez) layout: place
// the right-anchored full-height rect from the view width, seed the cursor coords,
// reset the widgets, then (on a successful placement probe) fill the running-sum
// item-percent table from the Multiplayer config section (one GetInt per item, each
// summed with the prior entry), apply the (5,3) rect, and - if the placement query
// succeeds - refresh. Returns 1 (0 on probe failure).
// @early-stop
// ~93.7%: every store/offset/GetInt-sum is byte-exact vs retail. Residual is the
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): retail pins
// the constant 0 in edi + view-x/y in ebx/ebp, while MSVC5 here pins 0 in ebx +
// view-x in edi - a 1-instr phase shift cascading through every =0 store, the
// running-sum reload registers, and the trailing `push 0`. Plus the SetRect import
// (ff 15 -> __imp_SetRect vs PTR_SetRect) + g_gameReg DIR32 naming. Not source-
// steerable under /O2; deferred to the final sweep.
RVA(0x000fdc00, 0x5c2)
i32 CStatusBarMgr::LoadBattlezItemConfig(CDDrawSurfaceMgr* world) {
    m_c = world;
    m_4 = 0;
    m_position = 0;
    i32 vx = g_gameReg->m_modeW;
    i32 vy = g_gameReg->m_modeH;
    SetRect(reinterpret_cast<LPRECT>(&m_10), vx - 0xa0, 0, vx, 0x1e0); // the +0x10..+0x1f tab-strip rect
    m_rect14.m_c = 0;
    m_24 = vx - 0x45;
    m_28 = vy - 0x30;
    m_itemKind = 5;
    m_tabCycle = g_curPlayer;
    ResetTabWidgets2b44();
    if (BuildStatusBarTabs() == 0) {
        return 0;
    }
    m_activeSlot = -1;
    m_pendingHlRow = -1;
    m_rezActive = 0;
    m_rezTick = 0;
    m_toggleActive = 0;
    m_toggleHandle = 0;
    m_battlezPct[0] = g_buteMgr.GetInt("Multiplayer", "ToolzPercent");
    m_battlezPct[1] = m_battlezPct[0] + g_buteMgr.GetInt("Multiplayer", "ToyzPercent");
    m_battlezPct[2] = m_battlezPct[1] + g_buteMgr.GetInt("Multiplayer", "BrickzPercent");
    m_battlezPct[3] = m_battlezPct[2] + g_buteMgr.GetInt("Multiplayer", "RedBrick");
    m_battlezPct[4] = m_battlezPct[3] + g_buteMgr.GetInt("Multiplayer", "BlueBrick");
    m_battlezPct[5] = m_battlezPct[4] + g_buteMgr.GetInt("Multiplayer", "GoldBrick");
    m_battlezPct[6] = m_battlezPct[5] + g_buteMgr.GetInt("Multiplayer", "BlackBrick");
    m_battlezPct[7] = m_battlezPct[6] + g_buteMgr.GetInt("Multiplayer", "BabyWalkerz");
    m_battlezPct[8] = m_battlezPct[7] + g_buteMgr.GetInt("Multiplayer", "BeachBallz");
    m_battlezPct[9] = m_battlezPct[8] + g_buteMgr.GetInt("Multiplayer", "BigWheelz");
    m_battlezPct[10] = m_battlezPct[9] + g_buteMgr.GetInt("Multiplayer", "GoKartz");
    m_battlezPct[11] = m_battlezPct[10] + g_buteMgr.GetInt("Multiplayer", "JackInTheBoxz");
    m_battlezPct[12] = m_battlezPct[11] + g_buteMgr.GetInt("Multiplayer", "JumpRopez");
    m_battlezPct[13] = m_battlezPct[12] + g_buteMgr.GetInt("Multiplayer", "PogoStickz");
    m_battlezPct[14] = m_battlezPct[13] + g_buteMgr.GetInt("Multiplayer", "Scrollz");
    m_battlezPct[15] = m_battlezPct[14] + g_buteMgr.GetInt("Multiplayer", "SqueakToyz");
    m_battlezPct[16] = m_battlezPct[15] + g_buteMgr.GetInt("Multiplayer", "Yoyoz");
    m_battlezPct[17] = m_battlezPct[16] + g_buteMgr.GetInt("Multiplayer", "Bombz");
    m_battlezPct[18] = m_battlezPct[17] + g_buteMgr.GetInt("Multiplayer", "Boomerangz");
    m_battlezPct[19] = m_battlezPct[18] + g_buteMgr.GetInt("Multiplayer", "Brickz");
    m_battlezPct[20] = m_battlezPct[19] + g_buteMgr.GetInt("Multiplayer", "Clubz");
    m_battlezPct[21] = m_battlezPct[20] + g_buteMgr.GetInt("Multiplayer", "Gauntletz");
    m_battlezPct[22] = m_battlezPct[21] + g_buteMgr.GetInt("Multiplayer", "Glovez");
    m_battlezPct[23] = m_battlezPct[22] + g_buteMgr.GetInt("Multiplayer", "Gooberz");
    m_battlezPct[24] = m_battlezPct[23] + g_buteMgr.GetInt("Multiplayer", "GravityBootz");
    m_battlezPct[25] = m_battlezPct[24] + g_buteMgr.GetInt("Multiplayer", "GunHatz");
    m_battlezPct[26] = m_battlezPct[25] + g_buteMgr.GetInt("Multiplayer", "NerfGunz");
    m_battlezPct[27] = m_battlezPct[26] + g_buteMgr.GetInt("Multiplayer", "Rockz");
    m_battlezPct[28] = m_battlezPct[27] + g_buteMgr.GetInt("Multiplayer", "Shieldz");
    m_battlezPct[29] = m_battlezPct[28] + g_buteMgr.GetInt("Multiplayer", "Shovelz");
    m_battlezPct[30] = m_battlezPct[29] + g_buteMgr.GetInt("Multiplayer", "Springz");
    m_battlezPct[31] = m_battlezPct[30] + g_buteMgr.GetInt("Multiplayer", "Spyz");
    m_battlezPct[32] = m_battlezPct[31] + g_buteMgr.GetInt("Multiplayer", "Swordz");
    m_battlezPct[33] = m_battlezPct[32] + g_buteMgr.GetInt("Multiplayer", "TimeBombz");
    m_battlezPct[34] = m_battlezPct[33] + g_buteMgr.GetInt("Multiplayer", "Toobz");
    m_battlezPct[35] = m_battlezPct[34] + g_buteMgr.GetInt("Multiplayer", "Wandz");
    m_battlezPct[36] = m_battlezPct[35] + g_buteMgr.GetInt("Multiplayer", "Welderz");
    m_battlezPct[37] = m_battlezPct[36] + g_buteMgr.GetInt("Multiplayer", "Wingz");
    SetTabState(5, 3);
    if ((static_cast<Utils::RegistryHelper*>(g_gameReg->m_settings))->GetValueDword("StatusBar Position", 0)
        == 1) {
        RefreshA();
    }
    return 1;
}

// CSbiMainSetup/CSbiMainL2/CSbiMainL1/CSbiFrameEntry/CSbiMainBarCfg + the
// MainBarDrawFrame decl moved to <Gruntz/StatusBarMgr.h>.

// 0xfe6b0 - drive the main status-bar sprite. For the non-cursor subtype, when the
// countdown (+0x20) is live, tick it; if the frame gate (m_barFrameGate) exceeds the screen
// height push the current rect to the game-manager's setup chain; look up
// GAME_STATUSBAR_MAINBAR and, on the resolved frame, draw it at the item's origin. Then
// fire the +0x30 and active-tab notify lists (slot 0x14) and refresh the +0x54c object.
// The trailing +0xd8 list (slot 0x28 then 0x14) runs for every subtype. Returns 1.
// @early-stop
// the two stack-struct arg builds (the +0x14 rect handed to SetRectXY and the frame-draw
// args) are byte-correct in content but MSVC schedules the interleaved local stores /
// packs the 0x14 frame differently than retail (the statement-schedule wall shared with
// Setup); the three notify walks + the config lookup are byte-faithful. Residual is that
// scheduling plus the g_gameReg / GAME_STATUSBAR_MAINBAR DIR32 naming. Deferred.
// NEIGHBOR TRIGGER: the sibling cue functions (HlClickGroup*/HiCue*), and now the
// CSbiMusicHost::m_map10 real-member de-cast, reshuffle this function's
// MainBarDrawFrame arg-block register allocation (the documented MSVC5 cross-function
// codegen leak), dropping the byte-match 95.6%->88.6% with NO source change here; the
// frame-draw args are still byte-content-correct. Accepted per the de-cast mandate.
// A second trigger (the one-TU merge): absorbing the updater/warpstone/serialize
// cluster into this TU re-fired the same stack-store/arg-block reshuffle
// (95.6% -> 88.6% again, no source change here). Accepted per the merge mandate.
// A third trigger (the BuildTabzDialog un-merge): removing that 3019-byte
// function from this TU fired the SAME 95.6% -> 88.6% reshuffle a third time, again with
// no source change here. A/B ruled out the SBI_ITEM_OWN_CTOR knob (with the knob forced
// back ON it still read 88.55), so the trigger was purely this TU's /O2 budget.
//
// AND IT IS NOW BACK AT 95.64 - the leak was never a wall, it was a TU-COMPOSITION
// READOUT. Finishing the un-merge restored it: CWarpStoneFly / CMgrSettings /
// CStatzTabBuilder carved out to their own TUs, CLevelSync::Sync RE-MERGED in (the binary
// wants it here - see its note). No source change to this function in any of it. So this
// function is effectively a sensor for "is this TU's content right?", and it reads
// correct only when the TU holds exactly the objs retail compiled together. Nothing to
// fix here; if it drops again, the TU's membership changed, not this code.
RVA(0x000fe6b0, 0x145)
i32 CStatusBarMgr::LoadMainStatusBarSprite() {
    if (m_position != kSubtypeTag) {
        if (m_rect14.m_c > 0) {
            m_rect14.m_c--;
            i32 v = m_barFrameGate;
            if (v > 0x1e0) {
                CSbiMainSetup* tgt = (reinterpret_cast<CSbiMainL1*>(g_gameReg->m_world->m_drawTarget))->m_14->m_mainSetup;
                struct {
                    i32 a, b, c, d;
                } rc;
                rc.a = m_10;
                rc.d = v;
                rc.b = m_rect14.m_8;
                rc.c = m_rect14.m_4;
                (reinterpret_cast<CDDSurface*>(tgt))->Restore(&rc, 0);
            }
            CMapStringToOb* map = &m_c->m_imageRegistry->m_10map;
            CObject* found = 0;

            map->Lookup("GAME_STATUSBAR_MAINBAR", found);
            if (found) {
                CSbiMainBarCfg* cfg = reinterpret_cast<CSbiMainBarCfg*>(found); // the cfg-record view of the stored element (next fold layer)
                CSbiFrameEntry* entry = cfg->m_14[cfg->m_64];
                if (entry) {
                    CSbiMainL1* l1 = reinterpret_cast<CSbiMainL1*>(g_gameReg->m_world->m_drawTarget); // L1 = the pages' SBI facet (next fold layer)
                    MainBarDrawFrame(l1->m_14, entry->m_18 + m_10, entry->m_1c + m_rect14.m_0, 0);
                }
            }
        }

        char* B = reinterpret_cast<char*>(this);
        POSITION n = m_tabLists[0].GetHeadPosition();
        while (n) {
            CSbiNotifyPayload* cur = static_cast<CSbiNotifyPayload*>(m_tabLists[0].GetNext(n));
            if (cur) {
                cur->Tick();
            }
        }
        CPtrList& tab = m_tabLists[m_activeTab];
        POSITION m = tab.GetHeadPosition();
        while (m) {
            CSbiNotifyPayload* cur = static_cast<CSbiNotifyPayload*>(tab.GetNext(m));
            if (cur) {
                cur->Tick();
            }
        }
        if (m_retabNotify) {
            m_retabNotify->Draw();
        }
    }

    POSITION k = m_tabLists[6].GetHeadPosition();
    while (k) {
        CSbiNotifyPayload* p = static_cast<CSbiNotifyPayload*>(m_tabLists[6].GetNext(k));
        if (p) {
            p->Refresh();
            p->Tick();
        }
    }
    return 1;
}

// CSbiHiWidget moved to <Gruntz/StatusBarMgr.h>.

// The cached PostMessageA entry point (game-owned fn pointer; the highlight
// dispatcher posts WM_COMMAND via it, not the direct import).

// Play GAME_TABHIGHLIGHT1 immediately (no clock gate) - variant 1: the record is
// resolved by a direct FindCue on the host (returns the record) and played.
static __inline void HiCueFind() {
    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
    if (host->m_30 == 0) {
        void* obj = ((host))->Lookup_05b7e0("GAME_TABHIGHLIGHT1");
        if (obj) {
            (static_cast<LeafCue*>(obj))->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
        }
    }
}

// Variant 2: resolve via the +0x10 string map (Lookup out-param) then play now.
static __inline void HiCueLookup() {
    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
    if (host->m_30 == 0) {
        void* out = 0;
        host->m_10.Lookup("GAME_TABHIGHLIGHT1", out); // CMapStringToPtr (mfc_class band)
        if (out) {
            (static_cast<LeafCue*>(out))->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
        }
    }
}

// Variant 3: the standard draw-clock-gated cue play (like LoadGooCookingSprite).
static __inline void HiCueTimed() {
    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
    if (host->m_30 == 0) {
        void* found = 0;
        host->m_10.Lookup("GAME_TABHIGHLIGHT1", found); // CMapStringToPtr (mfc_class band)
        if (found && g_sndEnabled != 0) {
            i32 item = g_sndCueTag;
            CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
            if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                p->m_14 = g_killCueClock;
                p->m_10->ConfigureItem(item, 0, 0, 0);
            }
        }
    }
}

// Post WM_COMMAND(cmdId) to the game window via the cached PostMessageA pointer.
static __inline void HiPost(i32 cmdId) {
    ::PostMessageA(g_gameReg->m_gameWnd->m_hwnd, 0x111, cmdId, 0);
}

// 0xfe910 - UpdateStatusBarTabHighlight(a1, a2, a3). Resolve the hit tab-highlight
// widget, run its Update, then dispatch on the widget kind (m_10, 0..6) and command
// (m_c): play the GAME_TABHIGHLIGHT1 cue and either refresh a sub-panel, arm a tab,
// post a WM_COMMAND to the game window, or toggle the destruct-button warning. The
// resource/gruntz/game tabs gate on m_354 + the active object's tab-highlight-enable
// flag (m_68->m_400). Returns 1 (0 on an out-of-range command).
// @early-stop
// COMPLETE reconstruction of a 2958-byte 7-way widget-kind switch (jump table
// 0x4ff4a0) with three nested command sub-switches lowered by MSVC to byte-indexed
// jump tables (0x4ff4bc / 0x4ff4ec+0x4ff4e0 / 0x4ff51c+0x4ff50c / 0x4ff528). Walls:
// (1) each dense sub-switch is a separate-COMDAT jump table (docs/patterns/
// switch-jumptable-separate-comdat.md, ~75-80%) and the byte-index tables aren't
// source-steerable; (2) the ~15 inlined cue-play blocks + PostMessage tails
// tail-merge/schedule differently. Logic is byte-faithful; deferred to the final
// sweep. (The shared-global DIR32 operands -- g_gameReg/g_sndCueTag/
// ::PostMessageA/GAME_TABHIGHLIGHT1 $SG -- are RELOC-MASKED by objdiff, so their
// symbol names are scoring-neutral; the singleton at 0x24556c is now referenced by
// its canonical extern-C _g_mgrSettings, not the colliding ?g_gameReg@@ alias that
// also names the real g_gameReg at 0x245460. The residual is purely the code bytes.)
RVA(0x000fe910, 0xb8e)
i32 CStatusBarMgr::UpdateStatusBarTabHighlight(i32 a1, i32 a2, i32 a3) {
    CSbiHiWidget* w = HiResolve(a2, a3);
    if (w == 0) {
        return 1;
    }
    w->Update(a1, a2, a3);
    i32 cmd = w->m_c;
    switch (w->m_10) {
        case 0:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                return 1;
            }
            if (cmd > 0x259) {
                if (cmd == 0x25a) {
                    HiCueFind();
                    winapi_0fe520_SetRect();
                    return 1;
                }
                if (cmd == 0x25b) {
                    HiCueFind();
                    HiRefreshResource();
                    return 1;
                }
                return 0;
            }
            if (cmd == 0x259) {
                HiCueFind();
                RefreshA();
                return 1;
            }
            if (cmd <= 0 || cmd > 5) {
                return 0;
            }
            HiCueFind();
            SetTabState(cmd, 3);
            return 1;

        case 1:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                return 1;
            }
            if (cmd < 0x12c || cmd > 0x149) {
                return 0;
            }
            if (cmd <= 0x13a) {
                HiCueLookup();
                HiTabA(cmd - 0x12c);
            } else {
                HiCueLookup();
                HiTabB(cmd - 0x13b, 0);
            }
            return 1;

        case 2:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                return 1;
            }
            if (cmd < 0x64 || cmd > 0x68) {
                return 0;
            }
            HiSelectStat(cmd - 0x64);
            return 1;

        case 3:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                return 1;
            }
            if (cmd < 0xd3 || cmd > 0xde) {
                return 1;
            }
            if (cmd <= 0xd6) {
                HiGrunt0(cmd - 0xd3);
            } else if (cmd <= 0xda) {
                HiGrunt1(cmd - 0xd7);
            } else {
                HiGrunt2(cmd - 0xdb);
            }
            return 1;

        case 4:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                return 1;
            }
            if (cmd < 0x190 || cmd > 0x193) {
                return 0;
            }
            HiCueLookup();
            m_tabCycle = cmd - 0x190;
            ResetWidgets(0);
            FinishReset16ea();
            Deactivate();
            return 1;

        case 5:
            if (m_toggleActive != 0) {
                return 1;
            }
            switch (cmd) {
                case 0x1f4:
                    HiCueFind();
                    HiPost(0x8007);
                    return 1;
                case 0x1f5:
                    HiCueFind();
                    HiPost(0x80ce);
                    return 1;
                case 0x1f6:
                    HiCueFind();
                    HiPost(0x80cf);
                    return 1;
                case 0x1f8:
                    HiCueFind();
                    HiPost(0x8035);
                    return 1;
                case 0x1f7:
                    HiCueLookup();
                    HiPost(0x80e2);
                    return 1;
                case 0x1f9:
                    HiCueLookup();
                    if (g_gameReg->m_frameGate != 0) {
                        g_gameReg->m_frameGate ^= 1;
                        g_gameReg->FinishLevel(g_gameReg->m_frameGate, 1);
                    }
                    (static_cast<CPlay*>(g_gameReg->m_curState))->EnterOverlayDrag(1);
                    return 1;
                case 0x1fa:
                    HiCueLookup();
                    ArmTab(5, 0);
                    return 1;
                case 0x1fc:
                    if (g_gameReg->m_134 != 1) {
                        return 1;
                    }
                    if (m_modeArmed != 0) {
                        return 1;
                    }
                    if (m_hitTestDisabled != 0) {
                        return 1;
                    }
                    HiCueLookup();
                    {
                        CPlay* sm = static_cast<CPlay*>(g_gameReg->m_curState);
                        if (m_destructWarnActive == 0) {
                            m_destructWarnActive = 1;
                            m_modeState = 2;
                            m_destructWarnDelay = g_buteMgr.GetIntDef(
                                "StatusBar",
                                "DestructButtonWarningDelay",
                                0x32
                            );
                            m_destructWarnDelayHi = 0;
                            m_destructWarnLast = g_frameTime;
                            m_destructWarnLastHi = 0;
                            sm->ArmSnapshot(1, 0xbb7);
                        } else {
                            CSbiSlotPtr* n = m_modeNotify;
                            m_destructWarnActive = 0;
                            m_modeState = 1;
                            if (n) {
                                n->Notify(1);
                            }
                            sm->ArmSnapshot(0, 0xbb7);
                        }
                    }
                    return 1;
                default:
                    return 0;
            }

        case 6:
            switch (cmd) {
                case 0x324:
                    if (g_gameReg->m_cmdGrid->m_phase == 1) {
                        HiCueLookup();
                        g_gameReg->AccrueScoreTime();
                    } else if (g_gameReg->m_134 == 1) {
                        HiCueLookup();
                        HiPost(0x806b);
                    } else {
                        HiCueLookup();
                        (static_cast<CPlay*>(g_gameReg->m_curState))->HiRefresh(0);
                    }
                    return 1;
                case 0x325:
                    if (g_gameReg->m_134 == 1) {
                        if (g_gameReg->m_cmdGrid->m_phase == 1) {
                            g_gameReg->UpdateScoreHud();
                        }
                        HiCueLookup();
                        HiPost(0x8023);
                    } else {
                        HiCueTimed();
                        g_gameReg->AccrueScoreTime();
                    }
                    return 1;
                case 0x327:
                    if (g_gameReg->m_134 == 1) {
                        if (g_gameReg->m_cmdGrid->m_phase == 1) {
                            g_gameReg->UpdateScoreHud();
                        }
                        HiCueTimed();
                        HiPost(0x8023);
                    } else {
                        HiCueTimed();
                        g_gameReg->AccrueScoreTime();
                    }
                    return 1;
                case 0x328:
                    HiCueTimed();
                    (static_cast<CPlay*>(g_gameReg->m_curState))->HiRefresh(0);
                    return 1;
                default:
                    return 0;
            }

        default:
            return 0;
    }
}

// 0xffb20 - build (or release) the DESTRUCT button display object. Only touches it
// while the presence gate (g_gameReg->m_soundEnabled) is up: when the mode is armed (m_destructWarnActive!=0)
// and not held (m_574==0), and the object is not yet built, look up GAME_DESTRUCT and
// build+configure it; otherwise release any existing object. Then refresh the host and
// fire the notify value `arg` down the three per-tab notify lists (+0x30, active tab,
// +0xd8) plus the +0x54c notifier. Returns 1.
// @early-stop
// ~97.2%: the code bytes are byte-exact vs retail (verified llvm-objdump -dr). The only
// scored diffs are the two DIR32 g_gameReg operands (retail _g_mgrSettings) and the
// GAME_DESTRUCT $SG string; every other reloc (Lookup/Build/Configure/Release/RefreshHost/
// Notify0/TabCommit) is a reloc-masked REL32. TU-wide rename, matcher.md reloc artifact.
RVA(0x000ffb20, 0x13a)
i32 CStatusBarMgr::LoadDestructButtonSprite(i32 arg) {
    if (g_gameReg->m_soundEnabled != 0) {
        if (m_destructWarnActive != 0 && m_modeArmed == 0) {
            if (m_destructButton == 0) {
                CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
                void* found = 0;
                host->m_10.Lookup("GAME_DESTRUCT", found); // CMapStringToPtr (mfc_class band)
                if (found) {
                    DSoundCloneInst* f = (static_cast<CSbiSpriteCfg*>(found))->m_playFactory;
                    if (f) {
                        DirectSoundMgr* obj = f->GetItem();
                        m_destructButton = obj;
                        if (obj) {
                            obj->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
                        }
                    }
                }
            }
        } else {
            if (m_destructButton) {
                m_destructButton->StopAndRewind();
                m_destructButton = 0;
            }
        }
    }
    RefreshHost();

    char* B = reinterpret_cast<char*>(this);
    POSITION n = m_tabLists[0].GetHeadPosition();
    while (n) {
        CSbiNotifyPayload* cur = static_cast<CSbiNotifyPayload*>(m_tabLists[0].GetNext(n));
        if (cur) {
            cur->Poll(arg);
        }
    }
    CPtrList& tab = m_tabLists[m_activeTab];
    POSITION m = tab.GetHeadPosition();
    while (m) {
        CSbiNotifyPayload* cur = static_cast<CSbiNotifyPayload*>(tab.GetNext(m));
        if (cur) {
            cur->Poll(arg);
        }
    }
    POSITION k = m_tabLists[6].GetHeadPosition();
    while (k) {
        CSbiNotifyPayload* cur = static_cast<CSbiNotifyPayload*>(m_tabLists[6].GetNext(k));
        if (cur) {
            cur->Poll(arg);
        }
    }
    if (m_retabNotify) {
        m_retabNotify->Tick(arg);
        Deactivate();
    }
    return 1;
}

// 0x102180 - build the RESUME game-tab button. If this is the subtype-2 cursor item,
// refresh it; when shown and not already on the gauge tab (5), apply the (5,3) rect;
// then (if the RESUME slot exists) configure it with the RESUME asset key, commit, and
// refresh it. Latch the show-RESUME gate (m_354 = 1).
RVA(0x00102180, 0x5f)
void CStatusBarMgr::BuildGameTabResumeButton(i32 show) {
    if (m_position == kSubtypeTag) {
        RefreshState();
    }
    if (show && m_activeTab != 5) {
        SetTabState(5, 3);
    }
    if (m_tabSprite5) {
        m_tabSprite5->ResolveFrame(reinterpret_cast<i32>("GAME_STATUSBAR_TABZ_GAMETAB_RESUME"), 1);
        Deactivate();
        m_tabSprite5->SetSubtype(); // slot 10
    }
    m_hitTestDisabled = 1;
}

// 0x102200 - build the PAUSE game-tab button. If the RESUME/PAUSE slot exists, configure
// it with the PAUSE asset key, commit, and refresh it. Clear the show-RESUME gate.
RVA(0x00102200, 0x37)
void CStatusBarMgr::BuildGameTabPauseButton() {
    if (m_tabSprite5) {
        m_tabSprite5->ResolveFrame(reinterpret_cast<i32>("GAME_STATUSBAR_TABZ_GAMETAB_PAUSE"), 1);
        Deactivate();
        m_tabSprite5->SetSubtype(); // slot 10
    }
    m_hitTestDisabled = 0;
}

// 0x1055b0 - build the GOOCOOKING1 status-bar sprite for stat slot `idx`. Bails (0)
// if the slot is already active; when the game is running (m_134==1) and the mode
// gate is down (m_hlBusy==0), refresh the subtype-2 cursor, apply the (2,3) rect off the
// gauge tab, and commit. Latch the gauge span for slot idx+0x17 (base g_frameTime, hi
// 0x7fffffff); then, on the gauge tab and not the cursor subtype, play the
// GAME_GOOCOOKING1 cue on the draw-clock window if the music gate is free. Returns 1.
// @early-stop
// ~96.7%: the code bytes are byte-exact vs retail (verified llvm-objdump -dr base vs
// target). The residual is purely the reloc-symbol-naming scoring tail - this TU models
// the shared singletons as ?g_gameReg@@... / ?g_frameTime@@... / ?g_sndEnabled@@... etc.
// while retail names them _g_mgrSettings / _g_645588 / ?g_sndEnabled@@... / ?g_sndCueTag@@
// ... / _g_killCueClock (+ the GAME_GOOCOOKING1 $SG string), so those DIR32 operands
// don't pair. A TU-wide rename, not a per-function fix; matcher.md reloc artifact.
RVA(0x001055b0, 0x109)
i32 CStatusBarMgr::LoadGooCookingSprite(i32 idx) {
    CSbiSlot* sp = &m_slots[idx];
    if (sp->m_state != 0) {
        return 0;
    }
    if (g_gameReg->m_134 == 1 && m_hlBusy == 0) {
        if (m_position == kSubtypeTag) {
            RefreshState();
        }
        if (m_activeTab != 2) {
            SetTabState(2, 3);
        }
        Deactivate();
    }
    sp->m_state = 1;
    CSbiSlot* g = reinterpret_cast<CSbiSlot*>(this) + (idx + 0x17);
    g->m_8 = 0x7fffffff;
    g->m_c = 0;
    g->m_state = g_frameTime;
    g->m_value = 0;
    if (m_activeTab == 2 && m_position != 2) {
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
        if (host->m_30 == 0) {
            void* found = 0;
            CMapStringToPtr* map = &host->m_10; // CMapStringToPtr per the mfc_class audit (the facet said Ob - the documented band inversion)
            map->Lookup("GAME_GOOCOOKING1", found);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                    if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                        p->m_14 = g_killCueClock;
                        p->m_10->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
    }
    return 1;
}

// 0x105990 - tick the rez-conveyor belt animation across the three group-A phase
// slots (each a 24-byte phase record: state, counter, 64-bit last-clock, 64-bit
// interval). Per slot, a state machine (1..7) advances the belt counter on a
// draw-clock gate, reconfigures the belt/hold delays at phase transitions, and plays
// the REZBELTRETURN/REZBELTBACKUP cues on the gauge tab. After the switch each slot's
// notifier is fired with the current counter.
// @early-stop
// complete reconstruction; residual is the 64-bit draw-clock gate scheduling +
// zero/1-register pinning across the 3-slot loop + the shared-global DIR32 naming
// (g_gameReg/g_frameTime/g_sndEnabled...); documented regalloc/scheduling walls.
RVA(0x00105990, 0x398)
void CStatusBarMgr::UpdateRezConveyorStatusBar() {
    i32 count = 3;
    CSbiSlotPtr** notify = m_groupNotify;
    SbiPhaseSlot* ph = reinterpret_cast<SbiPhaseSlot*>(m_groupSlots);
    do {
        switch (ph->m_state) {
            case 1:
                if (++ph->m_counter > 9) {
                    ph->m_counter = 1;
                }
                break;
            case 2:
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - ph->m_last >= ph->m_interval) {
                    if (++ph->m_counter >= 0x12) {
                        ph->m_counter = 0x12;
                        ph->m_state = 7;
                        ph->m_interval =
                            g_buteMgr.GetIntDef("StatusBar", "ConveyorBeltHoldDelay", 0x1f4);
                        ph->m_last = static_cast<u32>(g_frameTime);
                        UpdateFallingItemStatusBar(
                            m_extraNotifyArg0,
                            m_itemRectL + 0xc,
                            m_itemRectT + 0xc
                        );
                        ConveyorReturn();
                    }
                }
                break;
            case 3:
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - ph->m_last >= ph->m_interval) {
                    if (--ph->m_counter < 0xa) {
                        ph->m_state = 0;
                        ph->m_counter = 1;
                    }
                }
                break;
            case 4:
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - ph->m_last >= ph->m_interval) {
                    if (++ph->m_counter >= 0x18) {
                        ph->m_counter = 0x18;
                        ph->m_state = 6;
                        ph->m_interval =
                            g_buteMgr.GetIntDef("StatusBar", "ConveyorBeltHoldInDelay", 0x1f4);
                        ph->m_last = static_cast<u32>(g_frameTime);
                        m_machinePhase = 8;
                        m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "FallingItemDelay", 0x32);
                        m_beltLast = static_cast<u32>(g_frameTime);
                    }
                }
                break;
            case 5:
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - ph->m_last >= ph->m_interval) {
                    if (--ph->m_counter < 0x13) {
                        ph->m_state = 0;
                        ph->m_counter = 1;
                    }
                }
                break;
            case 6:
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - ph->m_last >= ph->m_interval) {
                    if (m_activeTab == 3 && m_position != 2) {
                        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
                        if (host->m_30 == 0) {
                            void* found = 0;
                            host->m_10.Lookup("GAME_REZBELTRETURN", found); // CMapStringToPtr (mfc_class band)
                            if (found && g_sndEnabled != 0) {
                                i32 item = g_sndCueTag;
                                CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                                if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                                    p->m_14 = g_killCueClock;
                                    p->m_10->ConfigureItem(item, 0, 0, 0);
                                }
                            }
                        }
                    }
                    ph->m_state = 5;
                }
                break;
            case 7:
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - ph->m_last >= ph->m_interval) {
                    if (m_activeTab == 3 && m_position != 2) {
                        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
                        if (host->m_30 == 0) {
                            void* found = 0;
                            host->m_10.Lookup("GAME_REZBELTBACKUP", found); // CMapStringToPtr (mfc_class band)
                            if (found && g_sndEnabled != 0) {
                                i32 item = g_sndCueTag;
                                CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                                if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                                    p->m_14 = g_killCueClock;
                                    p->m_10->ConfigureItem(item, 0, 0, 0);
                                }
                            }
                        }
                    }
                    ph->m_state = 3;
                }
                break;
        }
        if (*notify) {
            (*notify)->Notify(ph->m_counter);
        }
        notify++;
        ph++;
    } while (--count);
}

// 0x105e40 - drive the rez-machine (goo-machine) status bar: a two-record phase
// machine. Record A (+0x318) handles the spew/run phases (states 5/6) throttled on a
// 64-bit draw-clock gate; record B (+0x330) is a 1..4 switch cycling snooze / turning-
// wheel / (on phase 0x26) the machine-slot distribution: pick a machine column from
// the config percent (m_4cc), scan its highlight rows, and either RETRACT (empty slot
// found) or DROP (full), arming m_groupSlots[col] + playing the cue. Then set the
// conveyor-belt timer, feed the snooze/lever stat, and refresh the display object.
// @early-stop
// complete reconstruction; residual is the 64-bit draw-clock gates + MSVC cross-jump/
// tail-merge of the shared GetIntDef/SetStatBar sequences + zero-register pinning +
// shared-global DIR32 naming (g_gameReg/g_frameTime/g_buteMgr/g_sndEnabled). Walls.
RVA(0x00105e40, 0x62c)
void CStatusBarMgr::LoadRezMachineConfig() {
    SbiPhaseSlot* pA = reinterpret_cast<SbiPhaseSlot*>(&m_hudRectB_x);
    SbiPhaseSlot* pB = reinterpret_cast<SbiPhaseSlot*>(&m_hudRectA_x);
    SbiPhaseSlot* g = reinterpret_cast<SbiPhaseSlot*>(m_groupSlots);
    if (pA->m_state == 5) {
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - pA->m_last >= pA->m_interval) {
            if (++pA->m_counter > 0x34) {
                SetHudRectB(
                    0x2b,
                    5,
                    g_buteMgr.GetIntDef("StatusBar", "RightMachineRunningDelay", 0x7d)
                );
            } else {
                pA->m_interval = g_buteMgr.GetIntDef("StatusBar", "RightMachineRunningDelay", 0x7d);
                pA->m_last = static_cast<u32>(g_frameTime);
            }
        }
    } else if (pA->m_state == 6) {
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - pA->m_last >= pA->m_interval) {
            if (++pA->m_counter > 0x44) {
                SetHudRectB(0x2b, 0, 0x7fffffff);
            } else {
                pA->m_interval = g_buteMgr.GetIntDef("StatusBar", "RightMachineSpewingDelay", 0x7d);
                pA->m_last = static_cast<u32>(g_frameTime);
            }
        }
    }

    switch (pB->m_state) {
        case 1:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter > 8) {
                    SetHudRectA(
                        1,
                        1,
                        g_buteMgr.GetIntDef("StatusBar", "LeftMachineSnoozingDelay", 0x64)
                    );
                } else {
                    pB->m_interval =
                        g_buteMgr.GetIntDef("StatusBar", "LeftMachineSnoozingDelay", 0x64);
                    pB->m_last = static_cast<u32>(g_frameTime);
                }
            }
            break;
        case 2:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter > 0x13) {
                    SetHudRectA(
                        0x14,
                        3,
                        g_buteMgr.GetIntDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64)
                    );
                    SetHudRectB(
                        0x2b,
                        5,
                        g_buteMgr.GetIntDef("StatusBar", "RightMachineRunningDelay", 0x7d)
                    );
                    CSbiHlRow* s = m_groupSlots;
                    for (i32 i = 0; i < 3; i++) {
                        s->m_state = 1;
                        s->m_value = 1;
                        s++;
                    }
                    m_machinePhase = 2;
                    m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "NextItemDelay", 0x64);
                    m_beltLast = static_cast<u32>(g_frameTime);
                    if (m_activeTab == 3 && m_position != 2) {
                        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
                        if (host->m_30 == 0) {
                            void* found = 0;
                            host->m_10.Lookup("GAME_REZMACHINE", found); // CMapStringToPtr (mfc_class band)
                            if (found && g_sndEnabled != 0) {
                                i32 item = g_sndCueTag;
                                CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                                if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                                    p->m_14 = g_killCueClock;
                                    p->m_10->ConfigureItem(item, 0, 0, 0);
                                }
                            }
                        }
                    }
                } else {
                    pB->m_interval =
                        g_buteMgr.GetIntDef("StatusBar", "LeftMachineWakingDelay", 0x64);
                    pB->m_last = static_cast<u32>(g_frameTime);
                }
            }
            break;
        case 3:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter > 0x1d) {
                    SetHudRectA(
                        0x14,
                        3,
                        g_buteMgr.GetIntDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64)
                    );
                } else {
                    pB->m_interval =
                        g_buteMgr.GetIntDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64);
                    pB->m_last = static_cast<u32>(g_frameTime);
                }
            }
            break;
        case 4:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter == 0x26) {
                    i32 col;
                    i32 which = m_extraNotifyArg0;
                    if (which >= 0x22) {
                        col = 2;
                    } else {
                        col = (which >= 0x17) ? 1 : 0;
                    }
                    i32 found = 0;
                    for (i32 r = 3; r >= 0; r--) {
                        if (m_hlGrid[col * 4 + r].m_state == 0) {
                            found = 1;
                            break;
                        }
                    }
                    if (found) {
                        g[col].m_state = 4;
                        g[col].m_counter = 0x13;
                        if (m_activeTab == 3 && m_position != 2) {
                            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
                            if (host->m_30 == 0) {
                                void* fnd = 0;
                                host->m_10.Lookup("GAME_REZBELTRETRACT", fnd); // CMapStringToPtr (mfc_class band)
                                if (fnd && g_sndEnabled != 0) {
                                    i32 item = g_sndCueTag;
                                    CSbiCueRecord* p = static_cast<CSbiCueRecord*>(fnd);
                                    if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                                        p->m_14 = g_killCueClock;
                                        p->m_10->ConfigureItem(item, 0, 0, 0);
                                    }
                                }
                            }
                        }
                    } else {
                        g[col].m_state = 2;
                        g[col].m_counter = 0xa;
                        if (m_activeTab == 3 && m_position != 2) {
                            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
                            if (host->m_30 == 0) {
                                void* fnd = 0;
                                host->m_10.Lookup("GAME_REZBELTDROP", fnd); // CMapStringToPtr (mfc_class band)
                                if (fnd && g_sndEnabled != 0) {
                                    i32 item = g_sndCueTag;
                                    CSbiCueRecord* p = static_cast<CSbiCueRecord*>(fnd);
                                    if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                                        p->m_14 = g_killCueClock;
                                        p->m_10->ConfigureItem(item, 0, 0, 0);
                                    }
                                }
                            }
                        }
                    }
                    g[0].m_interval = g_buteMgr.GetIntDef("StatusBar", "ConveyorBeltDelay", 0x64);
                    g[0].m_last = static_cast<u32>(g_frameTime);
                    if (pB->m_counter > 0x2a) {
                        SetHudRectA(
                            1,
                            1,
                            g_buteMgr.GetIntDef("StatusBar", "LeftMachineSnoozingDelay", 0x64)
                        );
                    } else {
                        pB->m_interval =
                            g_buteMgr.GetIntDef("StatusBar", "LeftMachineLeverDelay", 0x64);
                        pB->m_last = static_cast<u32>(g_frameTime);
                    }
                }
            }
            break;
    }

    if (m_machineDisplay) {
        m_machineDisplay->SetFrames(pB->m_counter, pA->m_counter);
    }
}

// 0x106660 - snooze phase of the rez-machine status bar: pull the LeftMachineSnoozing
// delay from the StatusBar config, feed it to the stat bar (slot 1,1) and reset the
// gauge span, refresh the snooze display object (if present) from the HUD-rect A/B
// y-coords, then clear the snooze/wake state pair.
RVA(0x00106660, 0x68)
void CStatusBarMgr::UpdateRezMachineSnoozeStatusBar() {
    SetHudRectA(1, 1, g_buteMgr.GetDwordDef("StatusBar", "LeftMachineSnoozingDelay", 100));
    SetHudRectB(0x2b, 0, 0x7fffffff);
    if (m_machineDisplay) {
        m_machineDisplay->SetFrames(m_hudRectA_y, m_hudRectB_y);
    }
    m_rezActive = 0;
    m_rezTick = 0;
}

// 0x106bb0 - drive the chip-machine status bar: a 7-phase (m_machinePhase = 2..8) switch on a
// single 64-bit belt timer (+0x4d0). Each phase accumulates the NextItem/FallingItem
// speed/delay config into the rect-corner running sums (m_itemRectL/m_itemRectT/m_itemRectR/m_itemRectB),
// advances the phase on span thresholds, plays CHIPLAND/CHIPFALLOUT cues on the gauge
// tab, and picks a machine column from m_4cc. A rect-write flag and a refresh flag are
// latched per phase; the tail pushes the composed rect into the falling-item notifier
// and fires the fall-rect refresh.
// @early-stop
// complete reconstruction; residual is the 64-bit belt gates + heavy MSVC cross-jump/
// tail-merge of the shared GetSpeed/GetIntDef sequences and the two flag-set tails +
// zero-register pinning + jump-table reloc typing + shared-global DIR32 naming. Walls.
RVA(0x00106bb0, 0x7bc)
void CStatusBarMgr::LoadChipMachineConfig() {
    i32 refreshFlag = 0;
    i32 rectFlag = 0;
    switch (m_machinePhase) {
        case 2:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_beltLast >= m_beltInterval) {
                m_itemRectL += g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_itemRectR += g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "NextItemDelay", 0x64);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            if (m_itemRectL >= 0x6d) {
                m_itemRectL = 0x6d;
                m_itemRectR = 0x84;
                m_machinePhase = 3;
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "NextItemInMachineTime", 0x7d0);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            refreshFlag = 1;
            break;
        case 3:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_beltLast >= m_beltInterval) {
                SetHudRectB(
                    0x35,
                    6,
                    g_buteMgr.GetIntDef("StatusBar", "RightMachineSpewingDelay", 0x7d)
                );
                m_machinePhase = 4;
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "NextItemWaitTime", 0x1f4);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            break;
        case 4:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_beltLast >= m_beltInterval) {
                m_machinePhase = 5;
                if (m_activeTab == 3 && m_position != 2) {
                    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
                    if (host->m_30 == 0) {
                        void* found = 0;
                        host->m_10.Lookup("GAME_CHIPFALLOUT", found); // CMapStringToPtr (mfc_class band)
                        if (found && g_sndEnabled != 0) {
                            i32 item = g_sndCueTag;
                            CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                            if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                                p->m_14 = g_killCueClock;
                                p->m_10->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "FallingItemDelay", 0x32);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            break;
        case 5:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_beltLast >= m_beltInterval) {
                m_itemRectT += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                m_itemRectB += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "FallingItemDelay", 0x32);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            if (m_itemRectB >= 0x11c) {
                m_itemRectB = 0x11c;
                m_itemRectT = 0x104;
                rectFlag = 1;
                if (m_activeTab == 3 && m_position != 2) {
                    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
                    if (host->m_30 == 0) {
                        void* found = 0;
                        host->m_10.Lookup("GAME_CHIPLAND", found); // CMapStringToPtr (mfc_class band)
                        if (found && g_sndEnabled != 0) {
                            i32 item = g_sndCueTag;
                            CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                            if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                                p->m_14 = g_killCueClock;
                                p->m_10->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                m_machinePhase = 7;
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "NextItemDelay", 0x64);
                m_beltLast = static_cast<u32>(g_frameTime);
                if (m_extraNotifyArg0 >= 0x22) {
                    m_itemBaseX = 0x6d;
                } else if (m_extraNotifyArg0 >= 0x17) {
                    m_itemBaseX = 0x45;
                } else {
                    m_itemBaseX = 0x1d;
                }
            }
            refreshFlag = 1;
            break;
        case 7:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_beltLast >= m_beltInterval) {
                m_itemRectL -= g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_itemRectR -= g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "NextItemDelay", 0x64);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            if (m_itemRectL <= m_itemBaseX) {
                m_itemRectL = m_itemBaseX;
                m_itemRectR = m_itemBaseX + 0x17;
                rectFlag = 1;
                ChipNotify27f7();
                SetHudRectA(
                    0x1e,
                    4,
                    g_buteMgr.GetIntDef("StatusBar", "LeftMachineLeverDelay", 0x64)
                );
                m_machinePhase = 1;
            }
            refreshFlag = 1;
            break;
        case 8: {
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_beltLast >= m_beltInterval) {
                m_itemRectT += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                m_itemRectB += g_buteMgr.GetIntDef("StatusBar", "(FallingItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetIntDef("StatusBar", "FallingItemDelay", 0x32);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            i32 col;
            if (m_extraNotifyArg0 >= 0x22) {
                col = 2;
            } else {
                col = (m_extraNotifyArg0 >= 0x17) ? 1 : 0;
            }
            i32 row = 3;
            while (m_hlGrid[col * 4 + row].m_state == 1) {
                row--;
                if (row < 0) {
                    break;
                }
            }
            if (m_itemRectT >= row * 0x20 + 0x13e) {
                if (m_activeTab == 3 && m_position != 2) {
                    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry; // the REAL +0x28 sound registry (ex CSbiGameMgr/CSbiMusicHost facet)
                    if (host->m_30 == 0) {
                        void* found = 0;
                        host->m_10.Lookup("GAME_CHIPLAND", found); // CMapStringToPtr (mfc_class band)
                        if (found && g_sndEnabled != 0) {
                            i32 item = g_sndCueTag;
                            CSbiCueRecord* p = static_cast<CSbiCueRecord*>(found);
                            if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                                p->m_14 = g_killCueClock;
                                p->m_10->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                ChipFinish(col, m_extraNotifyArg0, row);
                ConveyorReturn();
            }
            refreshFlag = 1;
            break;
        }
    }

    if (m_extraNotify0) {
        if (rectFlag) {
            m_extraNotify0->m_rect14[0] = m_itemRectL + m_10;
            m_extraNotify0->m_rect14[1] = m_itemRectT + m_rect14.m_0;
            m_extraNotify0->m_rect14[2] = m_itemRectR + m_10;
            m_extraNotify0->m_rect14[3] = m_itemRectB + m_rect14.m_0;
        }
        if (refreshFlag) {
            RefreshFallRect();
        }
    }
}

// 0x107590 - configure the falling-item HUD gauge from a center point (a2,a3). Latch
// the notify arg + active flag, pull the FallingItemDelay config, seed the rect base
// from g_frameTime, build the relative +/-0xc rect (m_fallRectL block), and - if the notify
// object exists - write the absolute rect (offset by the item base coords m_10/m_14)
// into its +0x14 slot. Finish with the fall-rect refresh helper.
// @early-stop
// ~77%: logic + every field store/arithmetic is byte-correct. Residual is a regalloc/
// store-schedule wall in the notify-object rect block: retail pins the notify ptr in
// ebp (callee-saved) + spills rect0 to reserve eax for m_10, then stores the 4 rect
// ints in index order 0/1/2/3; MSVC5 here keeps the ptr in eax with ebp free (no
// spill) and stores 0/2/1/3. A register-assignment coin-flip (compute-all-then-store
// spelling regressed it to 69%); not steerable from C. Deferred to the final sweep.
RVA(0x00107590, 0xc4)
i32 CStatusBarMgr::UpdateFallingItemStatusBar(i32 a1, i32 a2, i32 a3) {
    m_extraNotifyArg1 = a1;
    m_fallActive = 1;
    m_fallDelay = g_buteMgr.GetIntDef("StatusBar", "FallingItemDelay", 0x32);
    m_fallDelayHi = 0;
    m_fallLast = g_frameTime;
    m_fallLastHi = 0;
    CSbiSlotPtr* n = m_extraNotify1;
    i32 l = a2 - 0xc;
    i32 t = a3 - 0xc;
    i32 rr = a2 + 0xc;
    i32 b = a3 + 0xc;
    m_fallRectL = l;
    m_fallRectT = t;
    m_fallRectR = rr;
    m_fallRectB = b;
    if (n) {
        i32 x = m_10;
        n->m_rect14[0] = l + x;
        n->m_rect14[2] = x + rr;
        i32 y = m_rect14.m_0;
        n->m_rect14[1] = t + y;
        n->m_rect14[3] = y + b;
    }
    RefreshFallRect();
    return 1;
}

// 0x107a10 - wake phase of the rez-machine status bar. On the first pass (state not
// yet set) bail unless the extra-notify arg is armed, else pull the LeftMachineWaking
// delay, feed the stat bar (slot 9,2), latch the state and return 1. On later passes
// just bump the wake tick counter and return 1.
// @early-stop
// ~96%: byte-exact except the `m_rezActive = 1; return 1;` tail - retail stores the
// immediate directly (mov [esi+0x528],1) then loads eax=1 separately, while MSVC5
// here reuses eax (mov eax,1; mov [esi+0x528],eax). A store-imm-vs-register-reuse
// regalloc coin-flip on the shared return constant; no C-level lever. Deferred.
RVA(0x00107a10, 0x62)
i32 CStatusBarMgr::UpdateRezMachineWakeStatusBar() {
    if (m_rezActive == 0) {
        if (m_extraNotifyArg0 == 0) {
            return 0;
        }
        SetHudRectA(9, 2, g_buteMgr.GetIntDef("StatusBar", "LeftMachineWakingDelay", 100));
        m_rezActive = 1;
        return 1;
    }
    m_rezTick++;
    return 1;
}

// 0x107ae0 - reset the item to the multiplayer/battlez starting layout: prep, refresh
// the subtype-2 cursor, force the tab to 5, arm it, clear the per-stat flags, reset the
// widgets; then (by game mode m_134) pre-fill the first N slots (N = StartingGruntz from
// the Multiplayer / Battlez config section) to (state=ready, value=0x1a); return each
// pooled ptr to the free-list, clear the +0x530 collection and the reset block, free the
// +0x54c notifier, and finish. The trailing i32 arg is ABI-accepted but unused.
// @early-stop
// ~90.5%: the prologue, tab-force, memset, both config loops, and the free-list return
// loop are byte-exact (the free-loop matched once m_ptrPool.GetData() was typed void**). Residual:
// (1) the teardown tail reorders the m_2b0/2b8/2b4/2bc zero-block vs the m_retabNotify load / m_hlBusy
// store (a store-vs-load MSVC scheduling coin-flip; an explicit-temp rewrite was neutral)
// and (2) the g_gameReg + StartingGruntz/Multiplayer/Battlez string DIR32 naming. Not
// source-steerable; deferred to the final sweep.
RVA(0x00107ae0, 0x1aa)
void CStatusBarMgr::LoadMultiplayerBattlezConfig(i32) {
    PrepMultiReset();
    if (m_position == kSubtypeTag) {
        RefreshState();
    }
    if (m_activeTab != 5) {
        NotifyTabExit();
        m_activeTab = 5;
    }
    ArmTab(5, 1);
    memset(m_statFlags, 0, sizeof(m_statFlags));
    ResetTabWidgets2b44();

    i32 mode = g_gameReg->m_134;
    if (mode == 2) {
        CSbiSlot* s = m_slots;
        for (i32 i = 0; i < g_buteMgr.GetIntDef("Multiplayer", "StartingGruntz", 0); i++) {
            s->m_value = kSlotCommitLevel;
            s->m_state = kSlotReady;
            s++;
        }
    } else if (mode == 3) {
        CSbiSlot* s = m_slots;
        for (i32 i = 0; i < g_buteMgr.GetIntDef("Battlez", "StartingGruntz", 0); i++) {
            s->m_value = kSlotCommitLevel;
            s->m_state = kSlotReady;
            s++;
        }
    }

    for (i32 j = 0; j < m_ptrPool.GetSize(); j++) {
        void* p = m_ptrPool.GetData()[j];
        if (p) {
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_ptrPool.SetSize(0, -1);
    m_2b0 = 0;
    m_2b8 = 0;
    m_2b4 = 0;
    m_2bc = 0;
    m_hlBusy = 0;
    if (m_retabNotify) {
        free(m_retabNotify);
        m_retabNotify = 0;
    }
    FinishReset1f6e();
    m_578 = 0;
    m_modeArmed = 0;
    FinishReset16ea();
}
