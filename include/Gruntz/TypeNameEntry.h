// TypeNameEntry.h - one record of the shared type-name registry table
// (g_typeColl.m_base, strided by g_typeColl.m_stride). Every per-type registrar (CProjectile /
// CKitchenSlime / CProjActObj / ActReg4 RegisterType) resolves its slot via
// TypeLookup and stores the key name into the record's leading CString - the
// retail call at 0x1b9e74 IS ??4CString@@QAEABV0@PBD@Z (CString::operator=,
// FID-confirmed), so the member is a real MFC CString, assigned directly
// (reloc-masked; no view/cast). The record stride is runtime data
// (g_typeColl.m_stride), so only the +0x00 member is modeled.
#ifndef GRUNTZ_GRUNTZ_CTYPENAMEENTRY_H
#define GRUNTZ_GRUNTZ_CTYPENAMEENTRY_H

#include <Mfc.h> // real MFC CString
#include <rva.h>

struct CTypeNameEntry {
    CString m_name; // +0x00  the registered type-name key
};
SIZE_UNKNOWN(CTypeNameEntry);

#endif // GRUNTZ_GRUNTZ_CTYPENAMEENTRY_H
