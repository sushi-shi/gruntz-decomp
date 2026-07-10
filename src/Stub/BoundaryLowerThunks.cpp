// BoundaryLowerThunks.cpp - small leaf thunks (tail-forwards / register-thunks /
// vptr-stamps / setters) recovered from the engine_boundary backlog (lower half,
// RVA < 0x133370). RTTI cannot attribute these COMDAT-folded one-liners, so the
// owning class names are placeholders; only the OFFSETS + code bytes are
// load-bearing. Unmodeled engine callees/globals are declared NO-body so their
// rel32/DIR32 operands reloc-mask in objdiff. Defined in retail-RVA order.
//
// RE-HOME STATUS (matcher-1 verify pass): NONE of these are ILT/incremental-link
// artifacts (every RVA is a real body well above the 0x1000-0x7c20 ILT band). HOMED
// (byte-neutral, next to the owning global's single DATA pin): the 7 range-register
// static-inits (-> LogicActReg/LogicActRegistrars/TimeBomb/StaticHazard), the g_str62c264
// /g_6473d8/g_levelStr/g_tinyFont init thunks (-> CustomWorldDialog/Multi/CustomWorldInfoDlg
// /Fonts). REMAINING (no clean home):
//   * class dtors 0x80cf0 (~CGameApp) / 0x85540+0x855a0 (~CGameMgr+scalar) / 0x94c10
//     (~CGameWnd): BYTE-PROVEN un-homeable, not merely risky (matcher-7 verified via
//     llvm-objdump -t on the base objs). Wap32.h makes these dtors INLINE, so gameapp.obj
//     /gamewnd.obj emit ONLY ??_GCGameApp/??_GCGameWnd (the scalar-deleting dtor, with the
//     plain ??1 fully INLINED into it) + ??_7CGameApp/??_7CGameWnd - there is NO separate
//     out-of-line ??1 in the real class's obj. Retail DOES emit a standalone ??1 (at these
//     RVAs), so it must come from somewhere. De-inlining the header dtor to emit ??1
//     out-of-line would REGRESS the derived dtors: CGruntzApp::~CGruntzApp / CGruntzWnd::
//     ~CGruntzWnd INLINE the base-subobject teardown (CloseResources()+counter dec / Destroy
//     ()+s_activeWnd=0), byte-exact, which requires the base dtor stay inline-visible. And
//     CGameMgr is namespaced WAP32:: in the reconstruction (to disambiguate CGruntzMgr), so
//     its emission mangles ??1CGameMgr@WAP32@@ - not retail's ??1CGameMgr@@. So the local
//     view IS the byte-necessary out-of-line ??1 emitter; it CANNOT dissolve without
//     regressing the derived dtors. KEEP (100% byte-exact). Final-sweep: leave as-is.
//   * g_obj646778 (CFileIO) / g_str649618 / g_font64ead8 / g_mgr6451a8: DATA-pinned HERE
//     (no external owner TU) - the thunk + its global stay together.
//   * placeholder-identity leaves (CInitd5d70 / CSettere56b0 / CStatusBaseSub100780 /
//     CPred1182f0 / NotNull118310 / ResetCoordPool82fa0 / Register82aa0): COMDAT-folded
//     one-liners whose owning class is unrecovered.
#include <Mfc.h> // real MFC CString (globals typed VCString, matching the target relocs)
#include <Ints.h>
#include <Wap32/Object.h>
#include <rva.h>

#include <Gruntz/ActReg.h>       // shared activation-registrar archetype (CActReg + aliases)
#include <Gruntz/GameKeyStr.h>   // canonical GameKeyStr (g_levelStr; Free1b9b93)
#include <Gruntz/FreeNodePool.h> // canonical coord free-pool (g_coordPool)
#include <Gruntz/HaznColl.h>     // shared coordinate/activation-registry collection
#include <Gruntz/TBombColl.h>    // shared coordinate/activation-registry collection
#include <Io/FileStream.h>       // real CFileIO (the static MFC CFile global at 0x646778)
#include <Font/Font.h>           // real Font / FontRenderer (the global font objects)
#include <Globals.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

// ===========================================================================
// Tiny string/object globals (already pinned in their owning TUs - reuse the
// mangled name, NO new DATA pin). The thunks default-CONSTRUCT the global CString
// (0x1b9b93 == CString::CString(), a tail-jmp `mov ecx,&g; jmp ctor`) via the
// explicit-ctor-call extension `g.CString::CString();` (placement-new would emit a
// null check; see docs/patterns/explicit-ctor-call-inplace-tail-jmp.md). The globals
// stay the REAL MFC CString so their ?...@@3VCString@@A relocs are authentic.
// g_levelStr is a genuinely distinct type (?g_levelStr@@3UGameKeyStr@@A, NOT
// VCString): the canonical GameKeyStr (<Gruntz/GameKeyStr.h>).
// ===========================================================================
// (Global-object init/teardown thunks re-homed next to their single-owner DATA pin:
// g_str62c264 (0x3acb0) -> CustomWorldDialog.cpp; g_6473d8 (0xb5380) -> Multi.cpp;
// g_levelStr (0x3ad30) -> CustomWorldInfoDlg.cpp.)

// The registrar-collection archetypes' single tree-wide SIZE anchors stay here (the
// classes live in <Gruntz/ActReg.h> / TBombColl.h / HaznColl.h; used across many TUs).
SIZE_UNKNOWN(CLogicActTable);
SIZE_UNKNOWN(CLookupColl);
SIZE_UNKNOWN(CActReg);
SIZE_UNKNOWN(CTBombColl); // CTBombColl defined in <Gruntz/TBombColl.h> (shared)
SIZE_UNKNOWN(CHaznColl);  // CHaznColl defined in <Gruntz/HaznColl.h> (shared)

// (The 7 range-register static-initializers [0x3a530/0x5bc50/0xadde0/0xb15b0/0xb3ae0/
// 0xe17b0/0xfbb70] were re-homed next to their owning registrar globals:
// LogicActReg.cpp (62bfa0/646010), LogicActRegistrars.cpp (644af0/646188/646250),
// TimeBomb.cpp (g_tbombColl), StaticHazard.cpp (g_haznColl). Each is the
// `(CZDArrayDerived*)&g_X)->Construct(0x7d0,0x7da)` static init - same pattern as
// Projectile.cpp/KitchenSlime.cpp.)

// ===========================================================================
// 0x080cf0 - CGameApp base destructor: cl's implicit vptr-restore stamps
// ??_7CGameApp@@6B@ (0x5e9b0c, config/vtable_names.csv), then the devirtualized
// CloseResources() (0x13d8c0) and the live-instance-counter decrement (0x653c6c).
// Real polymorphic dtor - the 6-byte `mov [ecx],offset ??_7CGameApp` is cl's, not a
// manual stamp. 17 vtable slots (0x44) so the emitted ??_7 pairs the retail vtable.
// ===========================================================================
struct CGameApp {
    virtual ~CGameApp();           // 0x80cf0  slot 0 (+0x00)
    virtual void s1();             // +0x04
    virtual void s2();             // +0x08
    virtual void s3();             // +0x0c
    virtual void CloseResources(); // +0x10  (0x13d8c0, devirtualized reloc-masked call)
    virtual void s5();             // +0x14
    virtual void s6();             // +0x18
    virtual void s7();             // +0x1c
    virtual void s8();             // +0x20
    virtual void s9();             // +0x24
    virtual void s10();            // +0x28
    virtual void s11();            // +0x2c
    virtual void s12();            // +0x30
    virtual void s13();            // +0x34
    virtual void s14();            // +0x38
    virtual void s15();            // +0x3c
    virtual void s16();            // +0x40
};
SIZE_UNKNOWN(CGameApp);
VTBL(CGameApp, 0x001e9b0c); // vtable_names -> code (RTTI game class)
RVA(0x00080cf0, 0x12)
CGameApp::~CGameApp() {
    CloseResources();
    --g_instCount653c6c;
}

// (0x082aa0 Register82aa0 (+ its g_mgr6451a8 DATA pin) re-homed to src/Gruntz/
// GameText.cpp, RVA-contiguous with that TU's g_worldName initializer @0x82990.)

// (0x082fa0 ResetCoordPool re-homed to src/Gruntz/FreeNodePool.cpp - the non-freeing
// counterpart of ClearCoordPool, modeled through the canonical g_coordPool.)

// (0x085540 ~CGameMgr (the GLOBAL-namespace ??1CGameMgr@@ base dtor) re-homed to
// src/Rez/RezSync.cpp - that TU IS the CGameMgr-derived bootstrap; its scalar-deleting
// twin 0x855a0 stays here.)

// ===========================================================================
// 0x0855a0 - the scalar-deleting-destructor twin of CGameMgr::~CGameMgr (0x85540):
// the base-vptr-restore dtor (stamp + 0x13ddb0 teardown) folded into the delete-flag
// tail, then (flags&1) RezFree(this) and return this. Retail labelled it
// CGameMgrDerived::`scalar deleting destructor'. Real polymorphic: the inline
// vptr-restore dtor supplies the stamp + teardown call; scalar-dtor folds it. RezFree
// (0x1b9b82, _RezFree) is the engine's allocator free for this Rez-managed object.
// ===========================================================================
extern "C" void RezFree(void* p); // 0x1b9b82 (_RezFree)
struct CObj855a0 {
    void Base13ddb0(); // 0x13ddb0 (reloc-masked)
    // inline base-vptr-restore dtor: `mov [ecx],offset ??_7 + call 0x13ddb0`. Inline
    // so scalar-dtor's explicit dtor call folds the stamp (like the ??_G thunk).
    virtual ~CObj855a0() {
        Base13ddb0();
    }
};
SIZE_UNKNOWN(CObj855a0);
RELOC_VTBL(
    CObj855a0,
    0x001e9b8c
); // aliases CGameMgr (slot-fn RVAs match its vtable, 100% majority)
// @rva-symbol: ??_GCObj855a0@@UAEPAXI@Z 0x000855a0 0x24  (cl-auto-gen scalar-deleting dtor)

// ===========================================================================
// 0x08c470 - the out-of-line CState base-subobject destructor (byte-necessary keep,
// like the CGameApp/CGameWnd base dtors here): restamp the CState dtor vftable
// (??_7CState@@6B@ @0x5ea21c) then tail-call the base cleanup (CGameModeBase::BaseCleanup
// @0xfa150, via ILT 0x3f53). xref/RTTI-proven: this IS CState::~CState (State.h) - but
// State.h models CState's dtor INLINE so CPlay/CDemo/... FOLD the base teardown into
// their own dtors (matching retail's inlined base teardown). Emitting a real out-of-line
// CState::~CState() would de-inline the base and regress every CState-derived dtor, so
// the standalone ??1 at this RVA stays a distinct byte-necessary emitter (borrows CState's
// retail vtable, reloc-masked). Self-contained (the base-cleanup callee is a view method).
// ===========================================================================
struct CStateSub8c470 {
    void BaseCleanup(); // 0x0fa150 (reloc-masked, via ILT 0x3f53)
    virtual ~CStateSub8c470();
};
SIZE_UNKNOWN(CStateSub8c470);
RELOC_VTBL(CStateSub8c470, 0x001ea21c); // borrows ??_7CState@@6B@ (dtor-stamp verified)
RVA(0x0008c470, 0xb)
CStateSub8c470::~CStateSub8c470() {
    BaseCleanup();
}

// ===========================================================================
// 0x094c10 - CGameWnd base destructor: cl's implicit vptr-restore stamps
// ??_7CGameWnd@@6B@ (0x5ea344, config/vtable_names.csv), then Destroy() (0x13cfb0)
// and clears the active-window singleton (0x653c68). Real polymorphic dtor; 22 vtable
// slots (0x58) so the emitted ??_7 pairs the retail vtable.
// ===========================================================================
struct CGameWnd {
    virtual ~CGameWnd(); // 0x94c10  slot 0 (+0x00)
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    virtual void s6();
    virtual void s7();
    virtual void s8();
    virtual void s9();
    virtual void s10();
    virtual void s11();
    virtual void s12();
    virtual void s13();
    virtual void s14();
    virtual void s15();
    virtual void s16();
    virtual void s17();
    virtual void s18();
    virtual void s19();
    virtual void s20();
    virtual void s21();
    void Destroy(); // 0x13cfb0 (non-virtual reloc-masked call)
};
SIZE_UNKNOWN(CGameWnd);
VTBL(CGameWnd, 0x001ea344); // vtable_names -> code (RTTI game class)
RVA(0x00094c10, 0x16)
CGameWnd::~CGameWnd() {
    Destroy();
    g_singleton653c68 = 0;
}

// (0x0bd7f0 StrFreebd7f0 re-homed to src/Net/LobbyDialogs.cpp as InitPlayerNameStr -
// the CString g_str649618 dynamic initializer, next to that TU's g_playerName_649618
// char* view of the same 4 bytes.)

// ===========================================================================
// 0x0d5d70 - a CImage-family base-subobject destructor (byte-necessary keep): mark the
// object inactive (m_4 = -1), clear m_8/m_c, then re-stamp the CObject grand-base dtor
// vftable (0x5e8cb4). xref: reached ONLY via ctor-unwind thunks (EH cleanup); the
// +0x04/+0x08/+0x0c header it resets is the one CImage + its CDDrawSurfacePair sibling
// share (CImage.h), but the exact owning family member is unrecovered (IDENTITY-TODO,
// hence still a src/Stub placeholder). CObject re-stamp auto-folds via the base.
// ===========================================================================
struct CInitd5d70 : CObject {
    i32 m_4; // +0x04  status word (-1 inactive)
    i32 m_8; // +0x08
    i32 m_c; // +0x0c
    void Init();
};
SIZE_UNKNOWN(CInitd5d70);
RVA(0x000d5d70, 0x16)
void CInitd5d70::Init() {
    m_4 = -1;
    m_8 = 0;
    m_c = 0;
    // base vptr auto-stamped via CObject (manual stamp dropped, % ok)
}

// (0x1182f0 CPred1182f0::IsOne -> src/Gruntz/GameInfoString.cpp as CGameInfo::Check1;
// 0x118310 NotNull118310 -> src/Gruntz/GameInfoString.cpp as ValidateGameTime.)

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
VTBL(CStatusBarItem, 0x001eabcc);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
