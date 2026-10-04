#ifndef GRUNTZ_GRUNTZ_SERIALWORKERREFMACROS_H
#define GRUNTZ_GRUNTZ_SERIALWORKERREFMACROS_H

#include <DDrawMgr/WorkerLookup.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Utils/MapTyped.h>
#include <Utils/Text.h>

#include <string.h>

#define SERIAL_READ_WORKER(ar, mgr, name, field)                                                       do {                                                                                                   g_serialCounter++;                                                                                 (ar)->Read(name, SERIAL_NAME_LEN);                                                                 if (strlen(name) != 0) {                                                                               (field) = (mgr)->FindWorker(name);                                                             } else {                                                                                               (field) = NULL;                                                                                }                                                                                              } while (0)

#define SERIAL_READ_FRAME(ar, mgr, name, index, field)                                                 do {                                                                                                   g_serialCounter++;                                                                                 (ar)->Read(name, SERIAL_NAME_LEN);                                                                 (ar)->Read(&(index), sizeof(index));                                                               if (strlen(name) != 0) {                                                                               (field) = (mgr)->FindFrame(name, index);                                                       } else {                                                                                               (field) = NULL;                                                                                }                                                                                              } while (0)

#define SERIAL_WRITE_ANIMATION(ar, mgr, name, field) \
do { \
    g_serialCounter++; \
    memset(name, 0, sizeof(name)); \
    if ((field) != NULL && !copyTextToBuffer((mgr)->m_animRegistry->FindAnimationKey(field), name, SERIAL_NAME_LEN)) return 0; \
    (ar)->Write(name, SERIAL_NAME_LEN); \
} while (0)

#define SERIAL_WRITE_WORKER(ar, name, field) \
do { \
    g_serialCounter++; \
    memset(name, 0, sizeof(name)); \
    if ((field) != NULL && !copyTextToBuffer((field)->m_name, name, SERIAL_NAME_LEN)) return 0; \
    (ar)->Write(name, SERIAL_NAME_LEN); \
} while (0)

#define SERIAL_WRITE_FRAME(ar, mgr, name, index, field) \
do { \
    g_serialCounter++; \
    memset(name, 0, sizeof(name)); \
    const FrameReference frameReference = (mgr)->m_imageRegistry->FindFrameReference(field); \
    if (!copyTextToBuffer(frameReference.workerName, name, SERIAL_NAME_LEN)) return 0; \
    index = frameReference.frameIndex; \
    (ar)->Write(name, SERIAL_NAME_LEN); \
    (ar)->Write(&(index), sizeof(index)); \
} while (0)

#define SERIAL_READ_ANIMATION(ar, mgr, name, field)                                                    do {                                                                                                   g_serialCounter++;                                                                                 (ar)->Read(name, SERIAL_NAME_LEN);                                                                 if (strlen(name) != 0) {                                                                               (field) = (mgr)->m_animRegistry->FindAnimation(name);                     } else {                                                                                               (field) = NULL;                                                                                }                                                                                              } while (0)

#define GS_IDXREF(field)                                                                               g_serialCounter++;                                                                                 s->Read(buf, SERIAL_NAME_LEN);                                                                     s->Read(&idx, 4);                                                                                  if (strlen(buf) != 0) {                                                                                i32 i = idx;                                                                                       out = 0;                                                                                           out = reg->m_imageRegistry->FindWorker(buf);                                            CDDrawWorker* gm = static_cast<CDDrawWorker*>(out);                                                CImage* r = gm != 0 ? gm->GetAt(i) : 0;                                                            field = r;                                                                                     } else {                                                                                               field = 0;                                                                                     }

#define GS_NAMEREF(field)                                                                              g_serialCounter++;                                                                                 s->Read(buf, SERIAL_NAME_LEN);                                                                     if (strlen(buf) != 0) {                                                                                out = 0;                                                                                           out = reg->m_imageRegistry->FindWorker(buf);                                            field = static_cast<CDDrawWorker*>(out);                                                       } else {                                                                                               field = 0;                                                                                     }

#endif
