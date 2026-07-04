// PlayDtor.cpp - the /GX destructor of the in-game PLAY state CPlay (0x8c830) and
// of its CPlay-derived sibling state CDemo (0x8d0d0). Uses the ONE canonical CPlay
// (<Gruntz/Play.h>, `class CPlay : public CState`): its five destructible MFC
// members are typed there (CString m_1b4, CByteArray m_startMarkers, CByteArray m_3a4[4],
// CString m_cueText, CByteArray m_488), so the dtor's /GX member-teardown machinery +
// the most-derived vptr stamp fall out from cl. The members fold in reverse decl
// (= reverse offset) order under descending /GX trylevels:
//   +0x488  CByteArray   state 4
//   +0x410  CString      state 3
//   +0x3a4  CByteArray[4] (__ehvec_dtor over CByteArray::~CByteArray)  state 2
//   +0x370  CByteArray   state 1
//   +0x1b4  CString      state 0
// The dtor body first runs the explicit teardown CPlayDtorBody (0xc8700) at the top
// trylevel (state 5), then the members fold, then the CState base subobject restamps
// its dtor vtable (0x5ea21c, reloc-masked) and runs the CState dtor body (0xfa150).
//
// cl auto-emits ??_7CPlay@@6B@ (masks retail 0x5ea0bc, paired via vtable_names.csv) +
// ??_7CDemo@@6B@ (masks 0x5e9f0c). Only OFFSETS + code shape are load-bearing.
#include <Gruntz/Play.h> // canonical CPlay (typed MFC members -> the /GX dtor fold)
#include <rva.h>

// CDemo - the CPlay-derived sibling state (its own most-derived vtable 0x5e9f0c).
// Adds the leading derived-cleanup (0x3c010) then folds CPlay's whole teardown inline
// (the inline CPlay::~CPlay below lets cl inline the base-subobject teardown, matching
// retail). cl auto-emits ??_7CDemo (masks 0x5e9f0c).
struct CDemo : CPlay {
    void DerivedCleanup(); // 0x3c010
    ~CDemo() OVERRIDE;
};

// 0x8c830 - CPlay::~CPlay (/GX): stamp the CPlay vtable (prologue), run CPlayDtorBody
// at the top trylevel, fold the five members (reverse decl order, descending /GX
// states), then fold the CState base subobject (restamp 0x5ea21c, call CState body).
// INLINE so it folds into CDemo's dtor (0x8d0d0) exactly as retail inlines the base
// dtor; cl still emits one out-of-line COMDAT copy (driven by ??_GCPlay), which lands
// at 0x8c830. An inline dtor can't hang an RVA() (it would also tag the synthesized
// ??_G -> duplicate-RVA), so it is pinned by mangled name:
// @rva-symbol: ??1CPlay@@UAE@XZ 0x0008c830 0xaf
inline CPlay::~CPlay() {
    CPlayDtorBody();
}

// 0x8d0d0 - CDemo::~CDemo (/GX): stamp the derived vtable (0x5e9f0c), run the derived
// cleanup (0x3c010) at trylevel 0, then INLINE-fold CPlay's teardown (restamp 0x5ea0bc,
// CPlayDtorBody, the five members, the CState base).
RVA(0x0008d0d0, 0xc4)
CDemo::~CDemo() {
    DerivedCleanup();
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF. CPlay's size is tracked in CPlay.cpp; CState's in its own header.
SIZE_UNKNOWN(CDemo);
