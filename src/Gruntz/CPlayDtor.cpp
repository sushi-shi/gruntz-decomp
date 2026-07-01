// CPlayDtor.cpp - the /GX destructor of the in-game PLAY state CPlay (0x8c830) and
// of its CPlay-derived sibling state (0x8d0d0). Modeled in a SELF-CONTAINED layout
// view (NOT the shared CPlay.h, whose Render-matched member typing must stay
// untouched) so the /GX member-teardown machinery + the most-derived vptr stamp fall
// out exactly. CPlay owns five destructible MFC members the dtor tears down in reverse
// declaration order under descending /GX trylevels:
//   +0x1b4  CString      (CString::~CString  0x1b9cde)            state 0
//   +0x370  CByteArray   (CByteArray::~CByteArray 0x1b4f3e)       state 1
//   +0x3a4  CByteArray[4] (__ehvec_dtor 0x11f640 over 0x1b4f3e)   state 2
//   +0x410  CString      (0x1b9cde)                               state 3
//   +0x488  CByteArray   (0x1b4f3e)                               state 4
// The dtor body first runs the explicit teardown CPlayDtorBody (0xc8700) at the top
// trylevel (state 5), then the members fold, then the CState base subobject restamps
// its dtor vtable (0x5ea21c) and runs CState::~CState (0xfa150). The cl-emitted
// CPlay/CState vtables reloc-mask the retail manual vtables; only OFFSETS + code shape
// are load-bearing.
#include <Mfc.h> // real CString (0x1b9cde) + CByteArray (0x1b4f3e)
#include <rva.h>

// The CState base subobject: a polymorphic base whose dtor inlines the vptr restamp
// (??_7 reloc-masks 0x5ea21c) then calls the out-of-line CState dtor BODY (0xfa150).
// Inline so the base-subobject fold emits `[esi]=0x5ea21c; call CState-body` exactly
// like retail (the body is reloc-masked).
struct CStateBaseD {
    virtual ~CStateBaseD();
    void StateBody(); // 0xfa150  CState::~CState body (reloc-masked)
};
inline CStateBaseD::~CStateBaseD() {
    StateBody();
}

// CPlay layout view: the CState base + the five destructible members at their pinned
// offsets. The most-derived vptr the dtor stamps (??_7CPlayD reloc-masks 0x5ea0bc).
struct CPlayD : CStateBaseD {
    char p04[0x1b4 - 0x04];
    CString m_1b4; // +0x1b4
    char p1b8[0x370 - 0x1b8];
    CByteArray m_370; // +0x370 (0x14 bytes)
    char p384[0x3a4 - 0x384];
    CByteArray m_3a4[4]; // +0x3a4 (4 * 0x14 = 0x50)
    char p3f4[0x410 - 0x3f4];
    CString m_410; // +0x410
    char p414[0x488 - 0x414];
    CByteArray m_488; // +0x488 (0x14 bytes)
    char p49c[0x518 - 0x49c];

    void DtorBody(); // 0xc8700  the explicit pre-member teardown (CPlayDtorBody)
    ~CPlayD();
};

// 0x8c830 - CPlay::~CPlay (/GX): stamp the CPlay vtable (prologue), run CPlayDtorBody
// at the top trylevel, fold the five members (reverse decl order, descending /GX
// states), then fold the CState base subobject (restamp 0x5ea21c, call CState body).
// The dtor is INLINE so it folds into the CPlay-derived state's dtor (0x8d0d0) exactly
// as retail inlines the base dtor; MSVC still emits one out-of-line COMDAT copy (driven
// by ??_GCPlayD), which lands at 0x8c830. An inline dtor can't hang an RVA() (it would
// also tag the synthesized ??_G -> duplicate-RVA), so it is pinned by mangled name:
// @rva-symbol: ??1CPlayD@@UAE@XZ 0x0008c830 0xaf
inline CPlayD::~CPlayD() {
    DtorBody();
}

// CPlay-derived sibling state (its own most-derived vtable 0x5e9f0c). Adds the leading
// derived-cleanup (0x3c010) then folds CPlay's whole teardown inline (the inline
// CPlayD::~CPlayD above lets MSVC inline the base-subobject teardown, matching retail).
struct CDerivedStateD : CPlayD {
    void DerivedCleanup(); // 0x3c010
    ~CDerivedStateD();
};

// 0x8d0d0 - the derived state dtor (/GX): stamp the derived vtable (0x5e9f0c), run the
// derived cleanup (0x3c010) at trylevel 0, then INLINE-fold CPlay's teardown (restamp
// 0x5ea0bc, CPlayDtorBody, the five members, the CState base).
RVA(0x0008d0d0, 0xc4)
CDerivedStateD::~CDerivedStateD() {
    DerivedCleanup();
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CDerivedStateD);
SIZE_UNKNOWN(CPlayD);
SIZE_UNKNOWN(CStateBaseD);
