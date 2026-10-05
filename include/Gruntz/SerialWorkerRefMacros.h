#ifndef GRUNTZ_GRUNTZ_SERIALWORKERREFMACROS_H
#define GRUNTZ_GRUNTZ_SERIALWORKERREFMACROS_H

#include <DDrawMgr/WorkerLookup.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Utils/MapTyped.h>

#include <string.h>

#define SERIAL_READ_IMAGE_SET(ar, mgr, name, field)                                                \
    do {                                                                                           \
        g_serialCounter++;                                                                         \
        (ar)->Read(name, SERIAL_NAME_LEN);                                                         \
        if (strlen(name) != 0) {                                                                   \
            (field) = (mgr)->FindImageSet(name);                                                   \
        } else {                                                                                   \
            (field) = NULL;                                                                        \
        }                                                                                          \
    } while (0)

#define SERIAL_READ_FRAME(ar, mgr, name, index, field)                                             \
    do {                                                                                           \
        g_serialCounter++;                                                                         \
        (ar)->Read(name, SERIAL_NAME_LEN);                                                         \
        (ar)->Read(&(index), sizeof(index));                                                       \
        if (strlen(name) != 0) {                                                                   \
            (field) = (mgr)->FindFrame(name, index);                                               \
        } else {                                                                                   \
            (field) = NULL;                                                                        \
        }                                                                                          \
    } while (0)

#define SERIAL_WRITE_ANIMATION(ar, mgr, name, field)                                               \
    do {                                                                                           \
        g_serialCounter++;                                                                         \
        memset(name, 0, sizeof(name));                                                             \
        if ((field) != NULL) {                                                                     \
            strcpy(                                                                                \
                name,                                                                              \
                static_cast<const char*>((mgr)->GetAnimationRegistry()->FindAnimationKey(field))   \
            );                                                                                     \
        }                                                                                          \
        (ar)->Write(name, SERIAL_NAME_LEN);                                                        \
    } while (0)

#define SERIAL_WRITE_IMAGE_SET(ar, name, field)                                                    \
    g_serialCounter++;                                                                             \
    memset(name, 0, sizeof(name));                                                                 \
    if ((field) != NULL) {                                                                         \
        strcpy(name, (field)->GetName());                                                          \
    }                                                                                              \
    (ar)->Write(name, SERIAL_NAME_LEN)

#define SERIAL_WRITE_FRAME(ar, mgr, name, index, field)                                            \
    g_serialCounter++;                                                                             \
    memset(name, 0, sizeof(name));                                                                 \
    index = 0;                                                                                     \
    if ((field) != NULL) {                                                                         \
        (mgr)->GetImageRegistry()->FindFrameIdentity(field, name, &(index));                       \
    }                                                                                              \
    (ar)->Write(name, SERIAL_NAME_LEN);                                                            \
    (ar)->Write(&(index), sizeof(index))

#define SERIAL_READ_ANIMATION(ar, mgr, name, field)                                                \
    do {                                                                                           \
        g_serialCounter++;                                                                         \
        (ar)->Read(name, SERIAL_NAME_LEN);                                                         \
        if (strlen(name) != 0) {                                                                   \
            (field) =                                                                              \
                MapFind<CAnimationSequence>((mgr)->GetAnimationRegistry()->m_animations, name);    \
        } else {                                                                                   \
            (field) = NULL;                                                                        \
        }                                                                                          \
    } while (0)

#define GS_IDXREF(field)                                                                           \
    g_serialCounter++;                                                                             \
    s->Read(buf, SERIAL_NAME_LEN);                                                                 \
    s->Read(&idx, sizeof(idx));                                                                    \
    if (strlen(buf) != 0) {                                                                        \
        i32 i = idx;                                                                               \
        out = 0;                                                                                   \
        reg->GetImageRegistry()->m_imageSetsByName.Lookup(buf, out);                               \
        CImageSet* gm = static_cast<CImageSet*>(out);                                              \
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
        reg->GetImageRegistry()->m_imageSetsByName.Lookup(buf, out);                               \
        field = static_cast<CImageSet*>(out);                                                      \
    } else {                                                                                       \
        field = 0;                                                                                 \
    }

#endif // GRUNTZ_GRUNTZ_SERIALWORKERREFMACROS_H
