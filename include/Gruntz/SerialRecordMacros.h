#ifndef GRUNTZ_GRUNTZ_SERIALRECORDMACROS_H
#define GRUNTZ_GRUNTZ_SERIALRECORDMACROS_H

#include <Gruntz/SerialRefLookup.h>

#define SERIALREF(field)                                                                           \
    do {                                                                                           \
        i32 id;                                                                                    \
        ++g_serialCounter;                                                                         \
        ar->Read(&id, 4);                                                                          \
        CWwdSpriteObject* r = LookupSerialRef(dir->m_childGroup->m_registeredGameObjectsById, id); \
        (field) = r;                                                                               \
        if (r == NULL && id != 0) {                                                                \
            return 0;                                                                              \
        }                                                                                          \
    } while (0)

#define READCSTR(field)                                                                            \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(buf, SERIAL_NAME_LEN);                                                            \
        (field) = buf;                                                                             \
    } while (0)

#endif // GRUNTZ_GRUNTZ_SERIALRECORDMACROS_H
