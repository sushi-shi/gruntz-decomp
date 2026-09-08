#ifndef GRUNTZ_ACTNAMEREGISTRY_H
#define GRUNTZ_ACTNAMEREGISTRY_H

#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/TypeKeyColl.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

class CVariantSlot;
struct CString;

#define ACT_NAME_ID(idvar, key)                                                                    \
    i32 idvar = ActFindId(key);                                                                    \
    if (idvar == 0) {                                                                              \
        ActInsertId((key), g_typeCounter);                                                         \
        idvar = g_typeCounter;                                                                     \
        g_typeColl[g_typeCounter] = (key);                                                         \
        g_typeCounter++;                                                                           \
    }

#endif // GRUNTZ_ACTNAMEREGISTRY_H
