#ifndef GRUNTZ_GRUNTZ_SERIALWORKERREFMACROS_H
#define GRUNTZ_GRUNTZ_SERIALWORKERREFMACROS_H

#include <DDrawMgr/WorkerLookup.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>

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

#endif // GRUNTZ_GRUNTZ_SERIALWORKERREFMACROS_H
