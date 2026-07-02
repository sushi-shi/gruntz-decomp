// CTypeNameEntryView.h - the shared view of a type-name-registry entry whose
// Assign (0x1b9e74, CString::operator= __thiscall) stores a key name into the
// entry's embedded CString. Modeled NO-body so the call reloc-masks; invoked from
// every per-type registrar (KitchenSlime, ProjActRegistry, ActReg4, ...).
//
// Placeholder name; only the signature + emitted code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CTYPENAMEENTRYVIEW_H
#define GRUNTZ_GRUNTZ_CTYPENAMEENTRYVIEW_H

#include <rva.h>

struct CTypeNameEntryView {
    void Assign(const char* name); // 0x1b9e74 (CString::operator=)
};

#endif // GRUNTZ_GRUNTZ_CTYPENAMEENTRYVIEW_H
