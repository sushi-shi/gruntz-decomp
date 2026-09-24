#ifndef GRUNTZ_GRUNTZ_SERIALCLOCKINLINE_H
#define GRUNTZ_GRUNTZ_SERIALCLOCKINLINE_H

#include <Gruntz/PathHazard.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>

static __inline void SerializeClockPair(CFileMemBase* ar, SerialMode mode, i64* pair) {
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(pair, sizeof(*pair));
            ar->Write(pair + 1, sizeof(*pair));
            break;
        case SERIAL_LOAD:
            ar->Read(pair, sizeof(*pair));
            ar->Read(pair + 1, sizeof(*pair));
            break;
    }
}

static inline void SerQuadPair(CFileMemBase* ar, SerialMode mode, CHazardTimer* timer) {
    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            ar->Read(&timer->m_deadline, sizeof(timer->m_deadline));
            ar->Read(&timer->m_window, sizeof(timer->m_window));
        }
    } else {
        ar->Write(&timer->m_deadline, sizeof(timer->m_deadline));
        ar->Write(&timer->m_window, sizeof(timer->m_window));
    }
}

static inline void SyncClockPair(CFileMemBase* s, SerialMode mode, i64* pair) {
    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            s->Read(pair, sizeof(*pair));
            s->Read(pair + 1, sizeof(*pair));
        }
    } else {
        s->Write(pair, sizeof(*pair));
        s->Write(pair + 1, sizeof(*pair));
    }
}

#endif // GRUNTZ_GRUNTZ_SERIALCLOCKINLINE_H
