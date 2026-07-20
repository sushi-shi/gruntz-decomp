#ifndef GRUNTZ_GRUNTZ_CTYPENAMEENTRY_H
#define GRUNTZ_GRUNTZ_CTYPENAMEENTRY_H

#include <Mfc.h> // real MFC CString
#include <rva.h>

struct CTypeNameEntry {
    CString m_name; // +0x00  the registered type-name key
};
SIZE_UNKNOWN(CTypeNameEntry);

#endif // GRUNTZ_GRUNTZ_CTYPENAMEENTRY_H
