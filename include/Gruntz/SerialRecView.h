// SerialRecView.h - the shared registry-access views for the WAP "binary record
// load" idiom loaders (CEventLoadRec / CTriggerLoadRec / CGruntStateRec). Each Load
// resolves string-valued fields against the game registry: g_gameReg->m_world is a
// CRegSub30 whose +0x10 is the CDDrawWorkerRegistry name map (CMapStringToOb::Lookup),
// and an indexed field resolves through a CRegTypeTable (a bounded element array).
//
// Split out of StreamRecordLoaders.cpp when the three foreign record loaders were
// carved into their own TUs (operation REHOME, package D8): the views are shared by
// all three plus the main CEventLoadRec, so they live in one header instead of being
// duplicated per .cpp. Field names are placeholders; only offsets + code bytes are
// load-bearing. (These duplicate MgrSettings.h's CMgrActiveHolder / CMgrLookupRec;
// unifying onto those is deferred cleanup.)
#ifndef GRUNTZ_GRUNTZ_SERIALRECVIEW_H
#define GRUNTZ_GRUNTZ_SERIALRECVIEW_H

#include <Ints.h>
#include <rva.h>

class CDDrawWorkerRegistry; // MgrSettings.h - the name map + AnyValueMatches_155630

// g_gameReg->m_world viewed by the record loaders: +0x10 is the name table.
struct CRegSub30 {
    char m_pad00[0x10];
    CDDrawWorkerRegistry* m_10; // +0x10  the name table
};
SIZE_UNKNOWN(CRegSub30);

// The looked-up "type table" value an indexed field resolves through: a bounded
// array (m_elems[m_lowerBound .. m_upperBound]) whose element at the read index
// becomes the field.
struct CRegTypeTable {
    char m_pad00[0x14];
    void** m_elems; // +0x14  element array
    char m_pad18[0x64 - 0x18];
    i32 m_lowerBound; // +0x64  lower bound (inclusive)
    i32 m_upperBound; // +0x68  upper bound (inclusive)
};
SIZE_UNKNOWN(CRegTypeTable);

#endif // GRUNTZ_GRUNTZ_SERIALRECVIEW_H
