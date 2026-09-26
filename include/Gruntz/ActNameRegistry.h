#ifndef GRUNTZ_ACTNAMEREGISTRY_H
#define GRUNTZ_ACTNAMEREGISTRY_H

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/TypeKeyColl.h>
#include <ZTools/ZDArray.h>

struct CString;

#define ACT_NAME_ID(idvar, key)                                                                    \
    i32 idvar = ActFindId(key);                                                                    \
    if (idvar == 0) {                                                                              \
        ActInsertId((key), g_typeCounter);                                                         \
        idvar = g_typeCounter;                                                                     \
        g_typeColl[g_typeCounter] = (key);                                                         \
        g_typeCounter++;                                                                           \
    }

// clang-format off
#define REGISTER_ACT(registry, key, handler)                                                       \
    {                                                                                              \
        ACT_NAME_ID(id, key)                                                                       \
        {                                                                                          \
            CActHandler converted = static_cast<CActHandler>(handler);                             \
            {                                                                                      \
                CActHandler& slot = (registry)[id];                                                \
                slot = converted;                                                                  \
            }                                                                                      \
        }                                                                                          \
    }
// clang-format on

#endif // GRUNTZ_ACTNAMEREGISTRY_H
