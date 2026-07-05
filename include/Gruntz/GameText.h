// GameText.h - small name/string lookup + table-init leaves recovered from the
// Gruntz engine. These are the human-readable name/label tables the UI reads:
//
//   GetWorldDisplayName    - seeds the 8 world display names.
//   GetEndLevelStatLabels  - seeds the 8 end-of-level stat labels.
//   GetWarlordName         - boss/warlord name by id (returns CString).
//   CContainerErr::CContainerErr - the container-library exception
//                                  ctor (lazy-inits its message table).
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing
// (campaign doctrine).
#ifndef SRC_GRUNTZ_GAMETEXT_H
#define SRC_GRUNTZ_GAMETEXT_H

// ---------------------------------------------------------------------------
// CString - the engine's MFC-style CString (a single char* @+0). Only the
// const-char* ctor (CString::CString(const char*), NAFXCW) is used by
// these leaves; declared with NO body so the `call rel32` displacement is
// reloc-masked in objdiff.
// ---------------------------------------------------------------------------
#include <rva.h>
#include <Gruntz/String.h>

// ---------------------------------------------------------------------------
// CContainerErr - the Monolith container-library exception object (vtable
// ??_7CContainerErr@@6B@ @0x5f04cc). This is the SAME class the canonical
// <Wap32/zBitVec.h> models (as zBitVec's polymorphic base); GameText.h and
// zBitVec.h are ONE class expressed twice, a byte-forced DUAL-VIEW exactly like
// the CGruntzMgr/CGameRegistry MFC/Win32 split (a required ODR split - the two
// headers never coexist in one TU). zBitVec.h is the RTTI-true VIRTUAL view
// (virtual dtor + 6 declared engine virtuals); THIS view is the NON-virtual
// spelling the ctor's byte layout forces.
//
// WHY NON-VIRTUAL HERE (byte-proven CLASS-MODEL WALL, not a rogue view - a
// real-class virtual spelling does NOT reproduce the bytes): the ctor at
// 0x16d9c0 stores the message at +0x04 FIRST, then the vptr at +0x00 LAST:
//     mov  ecx,DWORD PTR [esp+0x4]      ; ecx = msg arg
//     test ecx,ecx / jne .. / mov ecx,0x6bf430   ; default msg
//     mov  DWORD PTR [eax+0x4],ecx      ; m_msg   (+0x04 stored FIRST)
//     mov  DWORD PTR [eax],0x5f04cc     ; vptr    (+0x00 stored LAST)
// A `virtual` decl makes MSVC emit the implicit vptr store at ctor ENTRY (before
// m_msg), inverting that order. Empirically confirmed (vtable-conversion-log.md
// 2026-07-01 + the disasm above): the real-virtual spelling regresses this ctor
// 100%->non-exact. So the non-virtual model with an explicit m_vtbl member is the
// only byte-exact spelling; it is retained as a documented class-model wall (per
// the matcher doctrine's allowance for a byte-proven CLASS wall).
//   +0x00  m_vtbl : void*        - the vtable pointer (stamped LAST by the ctor).
//   +0x04  m_msg  : const char*  - the message string (ctor stores the arg or the
//          default here). NAME DIVERGENCE FLAG: zBitVec.h names this same +0x04
//          field `CVariantSlot* m_errSink` (an error-sink pointer, from its
//          Set 0x16d850 / Remove 0x16e360 usage). The CTOR evidence (a const char*
//          message stored here) backs `m_msg`; the two are unreconciled - endgame
//          to prove whether +0x04 is the message or a sink (or the sink reads the
//          message through it). Not renamed either way pending disasm of Set/Remove.
// ---------------------------------------------------------------------------
extern void* g_containerErrVtbl;

SIZE_UNKNOWN(CContainerErr);
class CContainerErr {
public:
    CContainerErr(const char* msg);

public:
    void* m_vtbl;      // +0x00  the vtable pointer (stamped LAST - see wall note above)
    const char* m_msg; // +0x04  the error message this instance carries
};

#endif // SRC_GRUNTZ_GAMETEXT_H
