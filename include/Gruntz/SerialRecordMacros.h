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

#define NAMEREF(field)                                                                             \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(buf, SERIAL_NAME_LEN);                                                            \
        if (strlen(buf) != 0) {                                                                    \
            CAniElement* value = NULL;                                                             \
            MapLookup(dir->m_animRegistry->m_animations, buf, value);                              \
            (field) = value;                                                                       \
        } else {                                                                                   \
            (field) = NULL;                                                                        \
        }                                                                                          \
    } while (0)

#define GS_SUBREC(field)                                                                           \
    g_serialCounter++;                                                                             \
    memset(buf, 0, sizeof(buf));                                                                   \
    v = 0;                                                                                         \
    if (field != 0) {                                                                              \
        reg->m_imageRegistry->AnyValueMatches(field, buf, &v);                                     \
    }                                                                                              \
    s->Write(buf, SERIAL_NAME_LEN);                                                                \
    s->Write(&v, 4)

#define GS_IDXREF(field)                                                                           \
    g_serialCounter++;                                                                             \
    s->Read(buf, SERIAL_NAME_LEN);                                                                 \
    s->Read(&idx, 4);                                                                              \
    if (strlen(buf) != 0) {                                                                        \
        i32 i = idx;                                                                               \
        out = 0;                                                                                   \
        reg->m_imageRegistry->m_workersByName.Lookup(buf, out);                                    \
        CDDrawWorker* gm = static_cast<CDDrawWorker*>(out);                                        \
        CImage* r = gm != 0 ? gm->GetAt(i) : 0;                                                    \
        field = r;                                                                                 \
    } else {                                                                                       \
        field = 0;                                                                                 \
    }

#define GS_NAMEREF(field)                                                                          \
    g_serialCounter++;                                                                             \
    s->Read(buf, SERIAL_NAME_LEN);                                                                 \
    if (strlen(buf) != 0) {                                                                        \
        out = 0;                                                                                   \
        reg->m_imageRegistry->m_workersByName.Lookup(buf, out);                                    \
        field = static_cast<CDDrawWorker*>(out);                                                   \
    } else {                                                                                       \
        field = 0;                                                                                 \
    }

#endif // GRUNTZ_GRUNTZ_SERIALRECORDMACROS_H
