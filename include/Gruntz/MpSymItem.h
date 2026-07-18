// MpSymItem.h - MpSymItem, a CSymTab iteration payload reader.
//
// The GAME_MULTI registry path resolves to a CSymTab symbol table; the table is
// iterated FirstSym/NextSym2/NextSym3 and each returned payload's +0x00 is the entry
// name. @identity-TODO: folding this onto CSymTab is proven but the payload/item
// identity (CSymRec vs child scope) is unsettled. Used in src/Gruntz/MultiStartDlg.cpp.
#ifndef GRUNTZ_MPSYMITEM_H
#define GRUNTZ_MPSYMITEM_H

#include <rva.h>

struct MpSymItem {
    char* m_name; // +0x00  entry name (LPCSTR)
};
SIZE_UNKNOWN(MpSymItem);

#endif // GRUNTZ_MPSYMITEM_H
