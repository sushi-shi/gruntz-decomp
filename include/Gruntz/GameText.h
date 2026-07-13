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

// CContainerErr - the Monolith container-library exception base. THE DUPLICATE VIEW
// THAT LIVED HERE IS GONE: it was a second model of the class <Wap32/zBitVec.h> already
// owns (same class, same one-slot vtable ??_7CContainerErr@@6B@ @0x1f04cc). It was kept
// as a "byte-forced dual-view / required ODR split", but that wall does not exist - this
// header has exactly ONE includer (GameText.cpp), so nothing ever saw both models. The
// canonical class is included instead, and GameText.cpp defines its ctor (0x16d9c0) on it.
#include <Wap32/zBitVec.h>

#endif // SRC_GRUNTZ_GAMETEXT_H
