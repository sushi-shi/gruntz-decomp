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
#include <incs/CString.h>

// ---------------------------------------------------------------------------
// CContainerErr - the Monolith container-library exception object (the class
// whose vtable Ghidra's "__non_rtti_object" is an FID guess for).
// Its ctor takes an optional custom message (defaulting to a shared base) and
// lazily seeds a static table of 8 container-error message strings on first use.
//   +0x00  m_vtbl : void*  - the vtable pointer.
//   +0x04  m_msg  : const char*  - the error message this instance carries.
//
// Modeled as a NON-virtual class with an explicit vtable-pointer member so the
// ctor stores m_msg THEN m_vtbl (the target order); a `virtual` decl would make
// MSVC emit the implicit vptr store at ctor entry (before m_msg) instead.
// ---------------------------------------------------------------------------
extern void *g_containerErrVtbl;

class CContainerErr {
public:
    CContainerErr(const char *msg);
public:
    void       *m_vtbl;  // +0x00  the vtable pointer
    const char *m_msg;   // +0x04  the error message this instance carries
};

#endif // SRC_GRUNTZ_GAMETEXT_H
