// CActName.h - the CString held in a resolved activation-name slot, shared by
// every CUserLogic leaf's RegisterActs (and the static-hazard registry). ~CString
// (0x1b9b93) frees the old entry list; CString::operator=(char const*) (0x1b9e74)
// assigns the new key. Both are external/no-body so the calls reloc-mask. The
// per-leaf ActNameLookup that resolves a slot stays inline in each TU (its codegen
// is tuned to that TU's RegisterActs).
#ifndef GRUNTZ_GRUNTZ_CACTNAME_H
#define GRUNTZ_GRUNTZ_CACTNAME_H

#include <rva.h>

struct CActName {
    void Free();                  // 0x1b9b93 (~CString)
    void Assign(const char* key); // 0x1b9e74 (CString::operator=(char const*))
};

#endif // GRUNTZ_GRUNTZ_CACTNAME_H
