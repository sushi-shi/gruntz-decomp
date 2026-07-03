// BoundaryLowerThunks.cpp - small leaf thunks (tail-forwards / register-thunks /
// vptr-stamps / setters) recovered from the engine_boundary backlog (lower half,
// RVA < 0x133370). RTTI cannot attribute these COMDAT-folded one-liners, so the
// owning class names are placeholders; only the OFFSETS + code bytes are
// load-bearing. Unmodeled engine callees/globals are declared NO-body so their
// rel32/DIR32 operands reloc-mask in objdiff. Defined in retail-RVA order.
#include <Mfc.h> // real MFC CString (globals typed VCString, matching the target relocs)
#include <Ints.h>
#include <rva.h>

#include <Gruntz/ActReg.h>     // shared activation-registrar archetype (CActReg + aliases)
#include <Gruntz/CHaznColl.h>  // shared coordinate/activation-registry collection
#include <Gruntz/CTBombColl.h> // shared coordinate/activation-registry collection
#include <Globals.h>

// ===========================================================================
// Tiny string/object globals (already pinned in their owning TUs - reuse the
// mangled name, NO new DATA pin). The thunks default-CONSTRUCT the global CString
// (0x1b9b93 == CString::CString(), a tail-jmp `mov ecx,&g; jmp ctor`). C++ cannot
// tail-jmp to a ctor (placement-new inserts a null check), so the ctor address is
// forwarded through a tiny CStrCtor reloc-mask view - the globals stay the REAL
// MFC CString so their ?...@@3VCString@@A relocs are authentic. g_levelStr is a
// genuinely distinct type (?g_levelStr@@3UGameKeyStr@@A, NOT VCString), keep it.
// ===========================================================================
struct CStrCtor {
    void Ctor1b9b93(); // 0x1b9b93 == CString::CString() (reloc-masked address forward)
};
SIZE_UNKNOWN(CStrCtor);
struct GameKeyStr {
    void Free1b9b93(); // 0x1b9b93 (reloc-masked)
};
SIZE_UNKNOWN(GameKeyStr);
extern CString g_str62c264;   // 0x62c264 (pinned in CustomWorldDialog.cpp)
extern GameKeyStr g_levelStr; // 0x62c260 (pinned in Backlog.cpp)
extern CString g_6473d8;      // 0x6473d8 (pinned in CMulti.cpp)

// ===========================================================================
// Range-register thunks: hand the fixed [0x7d0, 0x7da] id range to a managed
// collection's register method (0x3742, __thiscall(int,int)). Each global keeps
// its owning TU's type/name; the method reloc-masks by address.
// ===========================================================================
// CLogicActTable / CLookupColl / CActReg are the shared <Gruntz/ActReg.h> archetype
// + aliases (RegisterRange 0x3742 seeds the [lo,hi] range); their SIZE annotations
// stay here (the single tree-wide tag for each).
SIZE_UNKNOWN(CLogicActTable);
SIZE_UNKNOWN(CLookupColl);
SIZE_UNKNOWN(CActReg);
SIZE_UNKNOWN(CTBombColl);                   // CTBombColl defined in <Gruntz/CTBombColl.h> (shared)
SIZE_UNKNOWN(CHaznColl);                    // CHaznColl defined in <Gruntz/CHaznColl.h> (shared)
extern CLogicActTable g_logicActReg_62bfa0; // 0x62bfa0 (LogicActReg.cpp)
extern CLookupColl g_reg_644af0;            // 0x644af0 (LogicActRegistrars.cpp)
extern CLogicActTable g_logicActReg_646010; // 0x646010 (LogicActReg.cpp)
extern CActReg g_actReg_646188;             // 0x646188 (LogicActRegistrars.cpp)
extern CActReg g_actReg_646250;             // 0x646250 (LogicActRegistrars.cpp)
extern CTBombColl g_tbombColl;              // 0x64c780 (CTimeBomb.cpp)
extern CHaznColl g_haznColl;                // 0x64e3d0 (CStaticHazard.cpp)

RVA(0x0003a530, 0x15)
void RegRange3a530() {
    g_logicActReg_62bfa0.RegisterRange(0x7d0, 0x7da);
}

RVA(0x0003acb0, 0xa)
void StrFree3acb0() {
    ((CStrCtor*)&g_str62c264)->Ctor1b9b93();
}

RVA(0x0003ad30, 0xa)
void StrFree3ad30() {
    g_levelStr.Free1b9b93();
}

RVA(0x0005bc50, 0x15)
void RegRange5bc50() {
    g_reg_644af0.RegisterRange(0x7d0, 0x7da);
}

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
    void Register(void* desc); // 0x1d38c5 (reloc-masked)
};
SIZE_UNKNOWN(CMgr6451a8);
DATA(0x002451a8)
extern CMgr6451a8 g_mgr6451a8;
RVA(0x00082aa0, 0x10)
void Register82aa0() {
    g_mgr6451a8.Register((void*)&g_desc60aac8);
}

// ===========================================================================
// 0x082fa0 - reset the grunt coordinate free-pool: zero the four consecutive
// pool globals (head @0x645540, freelist @0x645544, scratch @0x645548, bias
// @0x64554c). __cdecl. The three pinned-elsewhere globals reuse their names.
// ===========================================================================
struct CoordPool {
    void Recycle(void* elem); // 0x0311b0
};
SIZE_UNKNOWN(CoordPool);
extern CoordPool g_coordPool;  // 0x645540 (the BattlezMapConfig RUN-phase unit)
extern void* g_freeList;       // 0x645544 (Projectile.cpp)
extern i32 g_freeListNodeBias; // 0x64554c (Projectile.cpp)
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
// vtable_names.csv). 0x85540 IS that dtor (restamp the base vtable + UnknownClose),
// so cl's implicit entry vptr-store emits ??_7CGameMgr@@6B@ (masks 0x5e9b8c).
struct CGameMgr {
    virtual ~CGameMgr(); // 0x85540 (slot 0): implicit base-vtable restamp + UnknownClose
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    void UnknownClose(); // 0x13ddb0 (reloc-masked)
};
SIZE_UNKNOWN(CGameMgr);
RVA(0x00085540, 0xb)
CGameMgr::~CGameMgr() {
    UnknownClose();
}

// ===========================================================================
// 0x0855a0 - the scalar-deleting-destructor twin of CGameMgr::~CGameMgr (0x85540):
// the base-vptr-restore dtor (stamp + 0x13ddb0 teardown) folded into the delete-flag
// tail, then (flags&1) RezFree(this) and return this. Retail labelled it
// CGameMgrDerived::`scalar deleting destructor'. Real polymorphic: the inline
// vptr-restore dtor supplies the stamp + teardown call; ScalarDtor folds it. RezFree
// (0x1b9b82, _RezFree) is the engine's allocator free for this Rez-managed object.
// ===========================================================================
extern "C" void RezFree(void* p); // 0x1b9b82 (_RezFree)
struct CScalarDtor855a0 {
    void Base13ddb0(); // 0x13ddb0 (reloc-masked)
    // inline base-vptr-restore dtor: `mov [ecx],offset ??_7 + call 0x13ddb0`. Inline
    // so ScalarDtor's explicit dtor call folds the stamp (like the ??_G thunk).
    virtual ~CScalarDtor855a0() {
        Base13ddb0();
    }
    void* ScalarDtor(u32 flags);
};
SIZE_UNKNOWN(CScalarDtor855a0);
RVA(0x000855a0, 0x24)
void* CScalarDtor855a0::ScalarDtor(u32 flags) {
    this->CScalarDtor855a0::~CScalarDtor855a0();
    if (flags & 1) {
        RezFree(this);
    }
    return this;
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
RVA(0x00094c10, 0x16)
CGameWnd::~CGameWnd() {
    Destroy();
    g_singleton653c68 = 0;
}

RVA(0x000adde0, 0x15)
void RegRangeadde0() {
    g_logicActReg_646010.RegisterRange(0x7d0, 0x7da);
}

RVA(0x000b15b0, 0x15)
void RegRangeb15b0() {
    g_actReg_646188.RegisterRange(0x7d0, 0x7da);
}

RVA(0x000b3ae0, 0x15)
void RegRangeb3ae0() {
    g_actReg_646250.RegisterRange(0x7d0, 0x7da);
}

RVA(0x000b5380, 0xa)
void StrFreeb5380() {
    ((CStrCtor*)&g_6473d8)->Ctor1b9b93();
}

// ===========================================================================
// 0x0b5400 / 0x0bd430 - tail-forward two distinct teardown methods (0x1befd7,
// 0x1bf426) onto the shared object at 0x646778. __thiscall (no args).
// ===========================================================================
struct C646778 {
    void M1befd7(); // 0x1befd7 (reloc-masked)
    void M1bf426(); // 0x1bf426 (reloc-masked)
};
SIZE_UNKNOWN(C646778);
DATA(0x00246778)
extern C646778 g_obj646778;
RVA(0x000b5400, 0xa)
void Forwardb5400() {
    g_obj646778.M1befd7();
}
RVA(0x000bd430, 0xa)
void Forwardbd430() {
    g_obj646778.M1bf426();
}

// ===========================================================================
// 0x0bd7f0 - tail-forward a CString teardown (0x1b9b93) onto the global at
// 0x649618. __thiscall.
// ===========================================================================
DATA(0x00249618)
extern CString g_str649618;
RVA(0x000bd7f0, 0xa)
void StrFreebd7f0() {
    ((CStrCtor*)&g_str649618)->Ctor1b9b93();
}

// ===========================================================================
// 0x0d5d70 - init/restamp: seed members (+0x04 = -1, +0x08/+0x0c = 0) then stamp
// the worker-dtor vtable (0x5e8cb4, reuse the pinned name). __thiscall.
// ===========================================================================
extern void* g_wapObjectDtorVtbl; // 0x5e8cb4 (pinned in GameLevel.cpp et al.)
struct CInitd5d70 {
    void* vptr; // +0x00
    i32 m_4;    // +0x04
    i32 m_8;    // +0x08
    i32 m_c;    // +0x0c
    void Init();
};
SIZE_UNKNOWN(CInitd5d70);
RVA(0x000d5d70, 0x16)
void CInitd5d70::Init() {
    m_4 = -1;
    m_8 = 0;
    m_c = 0;
    vptr = &g_wapObjectDtorVtbl;
}

RVA(0x000e17b0, 0x15)
void RegRangee17b0() {
    g_tbombColl.RegisterRange(0x7d0, 0x7da);
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

RVA(0x000fbb70, 0x15)
void RegRangefbb70() {
    g_haznColl.RegisterRange(0x7d0, 0x7da);
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
};
SIZE_UNKNOWN(CStatusBaseSub100780);
RVA(0x00100780, 0xb)
CStatusBaseSub100780::~CStatusBaseSub100780() {
    Base1d6b();
}

// ===========================================================================
// 0x115730 / 0x1157b0 - tail-forward a font teardown onto two font globals
// (g_tinyFont @0x64ea58 via 0x179700, g_64ead8 via 0x179be0). __thiscall.
// ===========================================================================
class Font {
public:
    void M179700(); // 0x179700 (reloc-masked)
    void M179be0(); // 0x179be0 (reloc-masked)
};
extern Font g_tinyFont; // 0x64ea58 (pinned in Fonts.cpp)
DATA(0x0024ead8)
extern Font g_font64ead8;
RVA(0x00115730, 0xa)
void FontForward115730() {
    g_tinyFont.M179700();
}
RVA(0x001157b0, 0xa)
void FontForward1157b0() {
    g_font64ead8.M179be0();
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
