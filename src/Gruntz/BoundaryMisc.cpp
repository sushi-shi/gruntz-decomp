// BoundaryMisc.cpp - small non-EH leaf methods recovered from the engine_boundary
// backlog (C:\Proj\Gruntz). Each sits at a class boundary in GRUNTZ.EXE; RTTI
// cannot attribute them, so the owning class names here are placeholders. Only the
// OFFSETS + code bytes are load-bearing. Unmodeled engine callees are declared
// NO-body so their rel32/DIR32 operands reloc-mask.
#include <Ints.h>
#include <rva.h>
#include <string.h> // inline strlen / memset intrinsics
#include <Win32.h>  // WINAPI (windows.h) for the g_p* import-pointer types
#include <Globals.h>

// ===========================================================================
// 0x0087b0 - CUserBase base destructor: cl's implicit vptr-restore stamps
// ??_7CUserBase@@6B@ (0x5e70b4, config/vtable_names.csv) then returns (7-byte
// `mov [ecx],offset ??_7CUserBase + ret`, the empty final-base dtor). Real
// polymorphic dtor; 3 vtable slots (0xc) so the emitted ??_7 pairs the retail vtable.
// __thiscall, no args.
// ===========================================================================
struct CUserBase {
    virtual ~CUserBase(); // 0x87b0  slot 0 (+0x00)
    virtual void s1();
    virtual void s2();
};
SIZE_UNKNOWN(CUserBase);
RVA(0x000087b0, 0x7)
CUserBase::~CUserBase() {}

// ===========================================================================
// 0x008b90 - a finalize/teardown that fires up to two registered __thiscall
// callbacks (m_4, m_8) passing `this`, the m_8 one guarded by m_14->m_1c == m_28,
// nulls each fired slot, and resets m_28 to 0x3e9 (1001). __thiscall, one unused
// stack arg (ret 4). Self-contained (the callbacks are indirect).
// ===========================================================================
struct CFinalizeSub8b90 {
    char m_pad0[0x1c];
    i32 m_1c; // +0x1c
};
SIZE_UNKNOWN(CFinalizeSub8b90);
struct CFinalize8b90 {
    typedef void (CFinalize8b90::*PMF)(); // single-inheritance, non-virtual -> 4 bytes
    void* m_0;                            // +0x00
    PMF m_4;                              // +0x04  callback (called with `this`)
    PMF m_8;                              // +0x08  callback (guarded)
    char m_pad0c[0x14 - 0x0c];
    CFinalizeSub8b90* m_14; // +0x14
    char m_pad18[0x28 - 0x18];
    i32 m_28; // +0x28
    void Finalize(i32 arg);
};
SIZE_UNKNOWN(CFinalize8b90);
RVA(0x00008b90, 0x40)
void CFinalize8b90::Finalize(i32 arg) {
    if (m_4 == 0) {
        return;
    }
    if (m_8 != 0 && m_14->m_1c == m_28) {
        (this->*m_8)();
        m_8 = 0;
    }
    (this->*m_4)();
    m_4 = 0;
    m_28 = 0x3e9;
}

// ===========================================================================
// 0x00af50 - reset a global DWORD to 0 (the global at VA 0x6295d8 / RVA 0x2295d8).
// __cdecl free function.
// ===========================================================================
RVA(0x0000af50, 0xb)
void ResetDat6295d8() {
    g_dat6295d8 = 0;
}

// ===========================================================================
// 0x00b940 - a CUserBase-derived vptr restore: cl's implicit vptr-restore stamps the
// CUserBase base vtable (0x5e70b4) then zeros members at +0x04 and +0x3c. Placeholder
// polymorphic class (a distinct restore, not the 0x87b0 final-base dtor, so its ??_7
// reloc-masks by shape). __thiscall (no return-this).
// ===========================================================================
struct CUserBaseSubB940 {
    i32 m_4; // +0x04
    char m_pad8[0x3c - 0x08];
    i32 m_3c; // +0x3c
    virtual ~CUserBaseSubB940();
};
SIZE_UNKNOWN(CUserBaseSubB940);
RVA(0x0000b940, 0xf)
CUserBaseSubB940::~CUserBaseSubB940() {
    m_4 = 0;
    m_3c = 0;
}

// ===========================================================================
// 0x01f870 - guarded virtual dispatch: if the 4th virtual (vtbl slot +0xc) reports
// nonzero, run the non-virtual handler (0xfac70) and return its success as a bool
// (the retail neg/sbb/neg idiom); else return 0. __thiscall, no args.
// ===========================================================================
struct CGuardedDispatch1f870 {
    virtual i32 v0();
    virtual i32 v1();
    virtual i32 v2();
    virtual i32 IsActive(); // slot +0x0c
    i32 Handle();           // 0xfac70 (non-virtual; reloc-masked)
    i32 Run();
};
SIZE_UNKNOWN(CGuardedDispatch1f870);
RVA(0x0001f870, 0x1d)
i32 CGuardedDispatch1f870::Run() {
    if (!IsActive()) {
        return 0;
    }
    return Handle() != 0;
}

// ===========================================================================
// 0x0238d0 / 0x023960 - register thunks: invoke a 1-int method (0x1b4867) on a
// global container object (VA 0x62b5d0 / 0x62b640) with arg 0xa. __cdecl free fns.
// ===========================================================================
struct CGlobalContainer {
    void Register(i32 n); // 0x1b4867 (reloc-masked)
};
SIZE_UNKNOWN(CGlobalContainer);
DATA(0x0022b5d0)
extern CGlobalContainer g_container62b5d0;
DATA(0x0022b640)
extern CGlobalContainer g_container62b640;

RVA(0x000238d0, 0xd)
void Init238d0() {
    g_container62b5d0.Register(0xa);
}

RVA(0x00023960, 0xd)
void Init23960() {
    g_container62b640.Register(0xa);
}

// ===========================================================================
// 0x024ac0 - predicate: returns whether the manager-settings singleton's +0x30
// slot is non-null (0 when the arg is null). __stdcall, one arg (ret 4). The
// singleton is the already-pinned _g_mgrSettings (VA 0x64556c / RVA 0x24556c).
// ===========================================================================
struct CMgrSettingsView {
    char m_pad0[0x30];
    void* m_30; // +0x30
    char m_pad34[0x10c - 0x34];
    i32 m_10c; // +0x10c
    i32 m_110; // +0x110
};
SIZE_UNKNOWN(CMgrSettingsView);
extern "C" CMgrSettingsView* g_mgrSettings;
RVA(0x00024ac0, 0x20)
i32 __stdcall HasMgrSlot30(void* a) {
    if (a == 0) {
        return 0;
    }
    return g_mgrSettings->m_30 != 0;
}

// ===========================================================================
// 0x037870 - dialog init: when the settings singleton is live, push its +0x10c /
// +0x110 flags into two dialog check-boxes (IDs 0x46f / 0x4d5) via the cached
// CheckDlgButton import pointer (VA 0x6c44b4). __cdecl, one HWND arg.
// ===========================================================================
typedef int(WINAPI* PFN_CheckDlgButton)(void* hwnd, int id, unsigned check);
DATA(0x002c44b4)
extern PFN_CheckDlgButton p_CheckDlgButton;
// @early-stop
// 93.33% - regalloc wall: cl pins hwnd in edi and the cached import ptr in esi;
// retail swaps them (ptr in edi, hwnd in esi). The cached-pointer shape, both
// CheckDlgButton calls, the arg tuples and the null guard are byte-exact; the
// edi/esi assignment is not source-steerable.
RVA(0x00037870, 0x3c)
void DialogInit37870(void* hwnd) {
    if (g_mgrSettings == 0) {
        return;
    }
    PFN_CheckDlgButton fn = p_CheckDlgButton; // retail caches the import ptr in edi
    fn(hwnd, 0x46f, g_mgrSettings->m_10c);
    fn(hwnd, 0x4d5, g_mgrSettings->m_110);
}

// ===========================================================================
// 0x00d210 - validate a serial/key string: reject null obj/string or empty
// string, clear the save scratch buffer (g_saveBuf, 0x24 dwords) and the serial
// counter (g_serialCounter), then (only if the obj's +0x30 slot is live) run the
// parse callback (0x156530) over the fixed code-table entry (0x4024e6) and the
// string, returning success as a bool. __cdecl, two args.
// ===========================================================================
extern int g_saveBuf[];     // ?g_saveBuf@@3PAHA       (VA 0x629930)
extern int g_serialCounter; // ?g_serialCounter@@3HA   (VA 0x629ad0)
extern void Lab4024e6();    // VA 0x4024e6 (code-table entry passed as a ptr)
int __stdcall Parse156530(void* table, char* s, int z); // 0x156530
struct CSerialObj {
    char m_pad0[0x30];
    void* m_30; // +0x30
};
// @early-stop
// 97.55% - regalloc wall: the test-only obj->m_30 temp lands in eax; retail uses
// ecx (both are 0/free after the rep-stosd). Logic + the inline strlen/memset +
// the parse-callback dispatch are byte-exact; the single reg pick is not
// source-steerable.
RVA(0x0000d210, 0x65)
i32 ParseSerial(CSerialObj* obj, char* s) {
    if (obj == 0) {
        return 0;
    }
    if (s == 0) {
        return 0;
    }
    if (strlen(s) == 0) {
        return 0;
    }
    g_serialCounter = 0;
    memset(g_saveBuf, 0, 0x90);
    if (obj->m_30 == 0) {
        return 0;
    }
    return Parse156530((void*)&Lab4024e6, s, 0) != 0;
}

// ===========================================================================
// 0x0212a0 - reset: recursively clear a child table (0x16e070) then null members
// at +0x18, +0x28, +0x14 (in that order). __thiscall, no args.
// ===========================================================================
struct CClear212a0 {
    char m_pad0[0x14];
    void* m_14; // +0x14
    i32 m_18;   // +0x18
    char m_pad1c[0x28 - 0x1c];
    i32 m_28;                 // +0x28
    void ClearRecursive(i32); // 0x16e070 (reloc-masked)
    void Reset();
};
SIZE_UNKNOWN(CClear212a0);
RVA(0x000212a0, 0x21)
void CClear212a0::Reset() {
    ClearRecursive(0);
    m_18 = 0;
    m_28 = 0;
    m_14 = 0;
}

// ===========================================================================
// 0x018430 - tail-call wrapper: end the wait cursor on the current MFC thread
// (AfxGetModuleState()->m_thread->EndWaitCursor()). __cdecl, no args.
// ===========================================================================
struct AfxThread18430 {
    void EndWaitCursor(); // 0x1beb10 (reloc-masked)
};
SIZE_UNKNOWN(AfxThread18430);
struct AfxModuleState18430 {
    void* m_pad0;             // +0x00
    AfxThread18430* m_thread; // +0x04
};
SIZE_UNKNOWN(AfxModuleState18430);
extern AfxModuleState18430* AfxGetModuleState18430(); // 0x1d3631 (reloc-masked)
RVA(0x00018430, 0xd)
void EndWaitCursor18430() {
    AfxGetModuleState18430()->m_thread->EndWaitCursor();
}

// ===========================================================================
// 0x029af0 - conditionally consume two random draws (the CRT rand() LCG) then
// dispatch a tile-switch (0x4b320 via the 0x1640 thunk) with a fixed
// (a2, a3, 0, 0x9c7, 0, 0) tuple. __stdcall, 6 args (ret 0x18).
// ===========================================================================
extern "C" int rand(void);
struct CTileSwitch29af0 {
    void Dispatch(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x4b320 (via 0x1640)
};
SIZE_UNKNOWN(CTileSwitch29af0);
RVA(0x00029af0, 0x3b)
void __stdcall TileSwitch29af0(CTileSwitch29af0* a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    if (a4) {
        rand();
    }
    if (a5) {
        rand();
    }
    a1->Dispatch(a2, a3, 0, 0x9c7, 0, 0);
}
