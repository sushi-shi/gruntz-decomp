#include <Wap32/Object.h> // CObject grand-base (real virtual dtor)
#include <rva.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GameModeBase.h>
#include <Dsndmgr/SoundDevice.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
// ReconBatch2.cpp - reconstructed engine/game leaf methods recovered from the
// engine_unmatched worklist (matcher batch 2). Each function below is homed
// against a best-guess class shape (placeholder m_<hexoffset> field names; only
// the OFFSETS + emitted code bytes are load-bearing, campaign doctrine). All are
// plain /O2 /MT frameless leaves (no SEH/EH frame). External engine callees are
// modeled with NO body so their rel32 calls reloc-mask.
#include <Mfc.h> // real MFC (CPtrList/CString) + windows.h via afx.h (superset of Win32.h)

#include <DDrawMgr/DDSurface.h>           // canonical CDDSurface (Blt @0x13ee60)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // canonical CDDrawSubMgrLeafScan (RemoveKeysEqual_157c70)
#include <Gruntz/Multi.h>                 // real CMulti (the 0x64bd5c multiplayer singleton)
#include <Font/Font.h>                    // real Font (the g_mediumFont global)
#include <Dsndmgr/SfManager.h>            // real SFMANL101API device (was the sfman32.h shadow)
#include <Globals.h>

// (EnableButtons_be820 @0x000be820 re-homed to src/Net/LobbyDialogs.cpp as
// NetLobby::Init_2ed7 - the drop-in dialog's accept/reject button enabler; obj is a
// CMulti and m_528 == CMulti::m_isHost. xref-proven via NetLobby::DropInDlgProc /
// NetDlgInitDropIn.)

// (AddTail_bf580 @0x000bf580 re-homed to src/Gruntz/LobbySync.cpp as RecycleCmd - the
// g_pool command-recycle helper; g_pool (0x64aca8) is LobbySync.cpp's own datum and
// 0xbf580 sits inside its RVA span, drained by CLobbySync::Reset. NetCmdSlot's callers
// already reference it as RecycleCmd.)

// (EntryOwner_c0460::Find @0x000c0460 re-homed to src/Net/NetSession2.cpp as
// CNetSession::FindSlot - the +0x20 4x0x64 slot table IS m_slots[4] of CNetCmdSlot
// (m_0==m_state, m_4==m_resetGuard, m_10==m_latency). The old EntryOwner_c0460 view
// folded onto the canonical CNetSession (<Net/NetMgr.h>); no Multi.h edit needed.)

// (Host_c2a80::Run @0x000c2a80 re-homed to src/Net/NetMgrMisc.cpp as
// CMultiStartDlg::Method_c2a80 - reconcile channel 3 (SyncChannelSlot) then Drive;
// PROVEN CMultiStartDlg (self-calls this->SyncChannelSlot(0xc2ab0) + this->Drive(0xc40b0)).)

// (OptOwner_c4b30::Resolve @0x000c4b30 re-homed to src/Gruntz/MultiStartDlgRoster.cpp
// as CMultiStartDlg::GetSlotIndex - the local player's options-slot index via the
// m_host (+0x5c) CNetDlgHost facet; xref-proven (all callers are CMultiStartDlg
// methods invoking it on `this`).)

// ===========================================================================
// 0x000f9840 / 0x000de140 (cleanup pair, CGameModeBase) - stop the owned sub-
// resource (SoundStream::Stop), clear/prune the sub-manager map, then run the
// base teardown (CGameModeBase::BaseCleanup). m_c->m_28 (the map holder) is
// reloaded each statement, not cached.
// ===========================================================================
// Local placeholder view of the real SoundStream (include/Dsndmgr/SoundStream.h);
// 0x137a80 is canonically SoundStream::Stop (minervainner / SoundStreamTeardown.cpp),
// so name the method Stop here too (was the mislabel "Free").
// (CGameModeBase::ResetPreview @0xde140 re-homed to src/Gruntz/GameMode.cpp;
// the Holder_f9840 context view is hoisted to <Gruntz/GameModeBase.h>.)

// (SfDeviceInitKeys @0x000f8ec0 re-homed to src/Gruntz/SoundFontPath.cpp - the
// SFMAN32 device key-table re-seed; xref-proven (only CloseSoundFontDevice +
// BuildSoundFontPath, both in that TU, call it). Its g_sfCfgA0/A2/DeviceId DATA()
// bindings moved there too.)

// (CGameModeBase::Reset @0xf9840 re-homed to src/Gruntz/GameMode.cpp.)

// ===========================================================================
// 0x000faec0 (103B) - per-frame present/refresh of the bound view. If the
// suppress flag is set, clear it and return; else Refresh the sub-view, blit the
// front surface with arg0, Flip the back surface, then Refresh again.
// __thiscall(arg0). m_c->m_4 is reloaded each statement (not cached across).
// @orphan: reached only from CGruntzMgr::RunModalDialog/ExitModalUI (xref) on an
// unidentified modal-UI presenter object; the `this` owner class has no recoverable
// identity (only inbound edge is ILT thunk 0x1ec9, no ctor/new trace).
//
// NOT a one-class-many-views over-split: it is a genuine 4-deep pointer chain into
// distinct real DDraw objects (every sub-object call is direct/reloc-masked, so the
// typing below is matching-neutral):
//   SubView_faec0.Refresh  = CDDrawWorkerMgr::Method_158c70 (0x158c70, ddrawsubmgr, EXACT)
//   CSpriteFactoryHolder.m_2c .Flip       = CDDSurface::Flip               (0x13e850, directdrawmgr, EXACT)
//   CSpriteFactoryHolder.m_2c .CopyRect   = CFileImage::ShadeRect          (0x13f460, lutshaderect, src-claim ~67%)
// unidentified: PresentHost_faec0 (the `this` owner) + the Mid/CSpriteFactoryHolder hops -- the
// only inbound edge is ILT thunk 0x1ec9 (no clean ctor/new trace).
// ===========================================================================
struct Mid_faec0 {
    char m_pad0[0x4];
    CDDrawSubMgrPages* m_4; // +0x04
};
struct
    PresentHost_faec0 { // unidentified owner of Present @0xfaec0 (only inbound edge: ILT thunk 0x1ec9)
    char m_pad0[0xc];
    Mid_faec0* m_c; // +0x0c
    void Present(i32 arg0);
};
RVA(0x000faec0, 0x67)
void PresentHost_faec0::Present(i32 arg0) {
    if (g_suppress_64e360 != 0) {
        g_suppress_64e360 = 0;
        return;
    }
    m_c->m_4->Method_158c70(m_c->m_4->m_backPair);
    m_c->m_4->m_backPair->m_surface->ShadeRect(arg0, (RECT*)0);
    m_c->m_4->m_frontPair->m_surface->Flip((CDDSurface*)0);
    m_c->m_4->Method_158c70(m_c->m_4->m_backPair);
}

// ===========================================================================
// 0x000fafa0 (59B) - __stdcall(4) validity gate: returns 0 if arg0 is null;
// for kind 4 / 7 validates arg0 through a per-kind checker (return 0 on fail);
// otherwise (and on success) returns 1.
// ===========================================================================
i32 __stdcall Check4_2ce8(i32 h); // 0x0faff0 (kind 4)
i32 __stdcall Check7_36bb(i32 h); // 0x0fb1c0 (kind 7)
// @early-stop
// regalloc wall (topic:wall topic:regalloc): the switch body (cmp 4 je / cmp 7
// jne / kind7 inline / kind4 trailing) is byte-identical to retail; residual is
// a0 landing in ecx (cl) vs eax (retail) - so the a0==0 return needs an extra
// xor eax, and the push/cmp register encodings shift. The switch dispatch claims
// eax for `kind`; not source-steerable. ~93.3%.
RVA(0x000fafa0, 0x3b)
i32 __stdcall Validate_fafa0(i32 a0, i32 kind, i32 a2, i32 a3) {
    if (a0 == 0) {
        return 0;
    }
    switch (kind) {
        case 4:
            if (Check4_2ce8(a0) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Check7_36bb(a0) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// (0x001104f0 re-homed to src/Gruntz/TileTriggerSwitchLogic.cpp as
// CTileTriggerSwitchLogic::Vf0 - the one-shot Setup virtual, slot 0 (thunk 0x1749)
// shared across the whole *TriggerSwitchLogic family; xref: referenced at
// ??_7CTile*TriggerSwitchLogic@@6B@+0x0. The old Init8_1104f0 view WAS the base
// switch-logic layout; the 8-arg build signature is corroborated by CheckpointSwitchBuild.)

// (0x00112840 re-homed to src/Gruntz/TileTriggerDerivedCtors.cpp as
// CTileTimeTriggerSwitchLogic::Vf2 - the slot-2 override that normalizes the base
// slot-2 probe (thunk 0x2e0f) to a bool; xref: ??_7CTileTimeTriggerSwitchLogic@@6B@+0x8.)

// (Forward_115630 @0x00115630 re-homed to src/Gruntz/Fonts.cpp - the
// compiler-generated dynamic initializer that constructs the g_mediumFont global;
// Fonts.cpp is the TU that owns the four font globals.)

// ===========================================================================
// 0x0011e8dc (7B) - __thiscall vptr re-stamp: store the base dtor vtable
// (g_wapObjectDtorVtbl) into [this]. TERMINAL manual stamp (not convertible to
// `: public CObject`): this 7-byte fn IS the entire retail restamp - there is
// no ctor for cl to fold an auto-stamp into, and Obj_11e8dc is a placeholder.
// ===========================================================================
struct Obj_11e8dc : CObject {};
RVA(0x0011e8dc, 0x7)
// ===========================================================================
// 0x0016f6e0 (118B) - __stdcall(src, dst): while the descriptor-defined flag bit
// of src is clear, read an 8-byte record from src, transform it (Fn16f7f0), and
// write it to dst; remember src->m_8; finally Finish dst with the last m_8.
// ===========================================================================
struct Desc_16f6e0 {
    char m_pad0[4];
    i32 m_4; // +0x04 byte offset of the flag within the object
};
struct Src_16f6e0 {
    Desc_16f6e0* m_0; // +0x00 descriptor
    char m_pad4[0x8 - 0x4];
    i32 m_8;                           // +0x08
    void ReadRecord(void* buf, i32 n); // 0x16a510
};
struct Dst_16f6e0 {
    void WriteRecord(void* buf, i32 n); // 0x16ab20
    void Finish(i32 last);              // 0x16aab0
};
void Blowfish_encipher(unsigned int* lo, unsigned int* hi); // 0x16f7f0
// @early-stop
// regalloc wall (topic:wall topic:regalloc, const-materialize-into-reg-vs-
// immediate): the whole control flow + record read/encipher/write + Blowfish
// reloc match retail; residual is that retail pins the test mask 1 in bl
// (`movb $1,%bl; testb %bl,mem`) and re-zeros the record buffer with a fresh
// `xor edx` inside the loop, while cl tests with an immediate `$1` and hoists the
// zero into ebx outside the loop. Not source-steerable, ~84.9%.
RVA(0x0016f6e0, 0x76)
void __stdcall Copy_16f6e0(Src_16f6e0* src, Dst_16f6e0* dst) {
    i32 last = 0;
    while ((*((char*)src + src->m_0->m_4 + 8) & 1) == 0) {
        unsigned int rec[2];
        rec[0] = 0;
        rec[1] = 0;
        src->ReadRecord(rec, 8);
        last = src->m_8;
        Blowfish_encipher(&rec[0], &rec[1]);
        dst->WriteRecord(rec, 8);
    }
    dst->Finish(last);
}

// (0x00181660 + 0x001816a0 re-homed to src/DDrawMgr/LightEffectSetup.cpp as
// CFaderLight::v3 (AddItem) / v4 (DropItem) - the overlay light-fader's pooled-surface
// acquire/release virtuals (vtable slots 3/4, ??_7CFaderLight@@6B@+0xc/+0x10). The old
// Worker181x_181x view WAS CFaderLight (size 0x206c; m_38=activeSurface, m_2060=span
// count, m_2064/68=dims - the same offsets as LightEffectSetup's CFaderLightApply view;
// the +0x2c pool is the CFader base's dual-role m_set2cArg). Xref-proven vtable slots.)

// (0x00193340 re-homed to src/Bute/ButeTree.cpp as CButeTree::Walk - the crit-bit
// trie callback traversal; CButeMgr::ParseGroup (xref) calls it on a CButeTree
// sub-tree, `this` root at +0x18 == CButeTree::m_root, node layout == CButeTreeNode
// (child[0]/child[1]/bit/key/value). Filled the class's declared-only Walk slot; the
// old Tree_193340/TNode_193340 views were CButeTree/CButeTreeNode.)

// ===========================================================================
// 0x001b9b8d (6B) - getter that returns the address of a global descriptor
// (PTR_DAT_006156f4). `mov eax, OFFSET g; ret`.
// @identity-TODO -> likely NAFXCW library: it sits in the MFC CString/collection
// text region (immediate neighbors 0x1b9b46 ??2@YAPAXI@Z operator new, 0x1b9b93
// ??0CString@@QAE@XZ) and its only retail callers are MFC template-collection
// helpers (ConstructElements / CList::NewNode / CMap::NewAssoc / CString, via xref).
// Carve to config/library_labels.csv once the exact MFC symbol for the &0x6156f4
// descriptor getter is pinned (kept as a src claim meanwhile; EXACT).
// ===========================================================================
RVA(0x001b9b8d, 0x6)
void** Get_1b9b8d() {
    return &g_desc_6156f4;
}

SIZE_UNKNOWN(Desc_16f6e0);
SIZE_UNKNOWN(Dst_16f6e0);
SIZE_UNKNOWN(Mid_faec0);
SIZE_UNKNOWN(Obj_11e8dc);
SIZE_UNKNOWN(PresentHost_faec0);
SIZE_UNKNOWN(Src_16f6e0);
