#ifndef GRUNTZ_GRUNTZ_SERIALWORKERREFMACROS_H
#define GRUNTZ_GRUNTZ_SERIALWORKERREFMACROS_H

#include <DDrawMgr/WorkerLookup.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Utils/MapTyped.h>

#include <string.h>

#define SERIAL_READ_WORKER(ar, mgr, name, field)                                                   \
    do {                                                                                           \
        g_serialCounter++;                                                                         \
        (ar)->Read(name, SERIAL_NAME_LEN);                                                         \
        if (strlen(name) != 0) {                                                                   \
            (field) = (mgr)->FindWorker(name);                                                     \
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
                static_cast<const char*>((mgr)->m_animRegistry->FindAnimationKey(field))           \
            );                                                                                     \
        }                                                                                          \
        (ar)->Write(name, SERIAL_NAME_LEN);                                                        \
    } while (0)

#define SERIAL_READ_ANIMATION(ar, mgr, name, field)                                                \
    do {                                                                                           \
        g_serialCounter++;                                                                         \
        (ar)->Read(name, SERIAL_NAME_LEN);                                                         \
        if (strlen(name) != 0) {                                                                   \
            (field) = MapFind<CAniElement>((mgr)->m_animRegistry->m_animations, name);             \
        } else {                                                                                   \
            (field) = NULL;                                                                        \
        }                                                                                          \
    } while (0)

#endif // GRUNTZ_GRUNTZ_SERIALWORKERREFMACROS_H
