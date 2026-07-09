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
//     (~CGameWnd): the bodies match Wap32.h's INLINE dtors exactly, but their real home
//     is the out-of-line vtable-anchor emission in GameApp/GruntzMgr/GameWnd.cpp - which
//     needs the compiler-emitted-vtable + @rva-symbol labeling (vtable-gate risk). DEFER.
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

// ===========================================================================
// 0x082aa0 - register thunk: hand the address of a global descriptor (0x60aac8)
// to a manager singleton (0x6451a8) method (0x1d38c5, __thiscall(void*)). __cdecl.
// ===========================================================================
struct CMgr6451a8 {
    // Register @0x3742 IS CZDArrayDerived::Construct (2nd arg reloc-masked); cast at the call.
};
SIZE_UNKNOWN(CMgr6451a8);
DATA(0x002451a8)
extern CMgr6451a8 g_mgr6451a8;
RVA(0x00082aa0, 0x10)
void Register82aa0() {
    ((CZDArrayDerived*)&g_mgr6451a8)->Construct((i32)(void*)&g_desc60aac8, 0);
}

// ===========================================================================
// 0x082fa0 - reset the grunt coordinate free-pool: zero the four consecutive
// pool globals (head @0x645540, freelist @0x645544, scratch @0x645548, bias
// @0x64554c). __cdecl. The three pinned-elsewhere globals reuse their names.
// ===========================================================================
extern FreeNodePool g_coordPool; // 0x645540 (the BattlezMapConfig RUN-phase unit)
extern void* g_freeList;         // 0x645544 (Projectile.cpp)
extern i32 g_freeListNodeBias;   // 0x64554c (Projectile.cpp)
RVA(0x00082fa0, 0x17)
void ResetCoordPool82fa0() {
    *(i32*)&g_coordPool = 0;
    g_freeList = 0;
    g_poolScratch645548 = 0;
    g_freeListNodeBias = 0;
}

// ===========================================================================
// 0x085540 - stamp the vtable (0x5e9b8c) then tail-call the base teardown
// (0x13ddb0). __thiscall.
// ===========================================================================
// REALIZED: model the base WAP32::CGameMgr as a global 6-slot (0x18) polymorphic
// CGameMgr - the delinker names its vtable ??_7CGameMgr@@6B@ globally (config/
// vtable_names.csv). 0x85540 IS that dtor (restamp the base vtable + Close),
// so cl's implicit entry vptr-store emits ??_7CGameMgr@@6B@ (masks 0x5e9b8c).
struct CGameMgr {
    virtual ~CGameMgr(); // 0x85540 (slot 0): implicit base-vtable restamp + Close
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    void Close(); // 0x13ddb0 (reloc-masked)
};
SIZE_UNKNOWN(CGameMgr);
RVA(0x00085540, 0xb)
CGameMgr::~CGameMgr() {
    Close();
}

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

// ===========================================================================
// 0x0b5400 / 0x0bd430 - construct / Close the engine's ONE static MFC CFile global
// at 0x646778. Its real type is CFileIO (RTTI 0x1ed15c == CFile; class in
// <Io/FileStream.h>); this TU owns its canonical DATA symbol. 0x0b5400 re-runs the
// ctor in place via the explicit-ctor-call extension (mov ecx,&g; jmp CFileIO::CFileIO
// 0x1befd7 - placement-new here would emit a null-check, so the explicit call is the
// clean tail-jmp); 0x0bd430 tail-forwards CFileIO::Close (0x1bf426), devirtualized on
// the concrete global.
DATA(0x00246778)
extern CFileIO g_obj646778;
RVA(0x000b5400, 0xa)
void Forwardb5400() {
    g_obj646778.CFileIO::CFileIO();
}
RVA(0x000bd430, 0xa)
void Forwardbd430() {
    g_obj646778.Close();
}

// ===========================================================================
// 0x0bd7f0 - tail-forward a CString teardown (0x1b9b93) onto the global at
// 0x649618. __thiscall.
// ===========================================================================
DATA(0x00249618)
extern CString g_str649618;
RVA(0x000bd7f0, 0xa)
void StrFreebd7f0() {
    g_str649618.CString::CString();
}

// ===========================================================================
// 0x0d5d70 - init/restamp: seed members (+0x04 = -1, +0x08/+0x0c = 0) then stamp
// the worker-dtor vtable (0x5e8cb4, reuse the pinned name). __thiscall.
// ===========================================================================
struct CInitd5d70 : CObject {
    i32 m_4; // +0x04
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

// ===========================================================================
// 0x0e56b0 - setter: store the constant 0x42a at +0x20. __thiscall.
// ===========================================================================
struct CSettere56b0 {
    char pad0[0x20];
    i32 m_20; // +0x20
    void Set();
};
SIZE_UNKNOWN(CSettere56b0);
RVA(0x000e56b0, 0x8)
void CSettere56b0::Set() {
    m_20 = 0x42a;
}

// ===========================================================================
// 0x100780 - a CStatusBarItem-base vptr restore: cl's implicit vptr-restore stamps
// the status-base vtable (0x5eabcc) then tail-jumps the base init/teardown (0x1d6b).
// Placeholder polymorphic class (the real CStatusBarItem dtor is modeled in
// StatusBarItem.cpp; this is a distinct restore, so its ??_7 reloc-masks by shape).
// ===========================================================================
struct CStatusBaseSub100780 {
    void Base1d6b(); // 0x1d6b (reloc-masked)
    virtual ~CStatusBaseSub100780();
    virtual void VtSlotFill0(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill1(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill2(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill3(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill4(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill5(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill6(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill7(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill8(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill9(); // vtable-slot filler (real slot; declared-only)
};
SIZE_UNKNOWN(CStatusBaseSub100780);
RELOC_VTBL(
    CStatusBaseSub100780,
    0x001eabcc
); // vtable reloc-masks a bound datum (dtor-stamp verified)
RVA(0x00100780, 0xb)
CStatusBaseSub100780::~CStatusBaseSub100780() {
    Base1d6b();
}

// ===========================================================================
// 0x1157b0 - construct the global g_font64ead8 (a FontRenderer, ctor 0x179be0,
// ??0FontRenderer@@QAE@XZ; DIFFERENT type from the bitmap Font globals) in place via
// the explicit-ctor-call tail-jmp (docs/patterns/explicit-ctor-call-inplace-tail-jmp.md).
// Its DATA symbol is owned here (no external pin). From <Font/Font.h>.
// (The sibling g_tinyFont ctor thunk 0x115730 re-homed to Fonts.cpp, next to the
// g_mediumFont one, where g_tinyFont's DATA pin lives.)
// ===========================================================================
DATA(0x0024ead8)
extern FontRenderer g_font64ead8;
RVA(0x001157b0, 0xa)
void FontForward1157b0() {
    g_font64ead8.FontRenderer::FontRenderer();
}

// ===========================================================================
// 0x1182f0 - predicate: is +0x08 equal to 1? __thiscall.
// ===========================================================================
struct CPred1182f0 {
    char pad0[8];
    i32 m_8; // +0x08
    i32 IsOne();
};
SIZE_UNKNOWN(CPred1182f0);
RVA(0x001182f0, 0xc)
i32 CPred1182f0::IsOne() {
    return m_8 == 1;
}

// ===========================================================================
// 0x118310 - predicate: is the arg non-null? __cdecl(void*).
// ===========================================================================
RVA(0x00118310, 0xc)
i32 NotNull118310(void* p) {
    return p != 0;
}

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
VTBL(CStatusBarItem, 0x001eabcc);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
