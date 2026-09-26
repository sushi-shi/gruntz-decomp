#ifndef GRUNTZ_CLOCKINTERVAL_H
#define GRUNTZ_CLOCKINTERVAL_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <Rez/FrameClock.h>

class CFileMemBase;
struct CGameObject;

struct ClockInterval {
    union {
        i64 m_start;
        struct {
            i32 m_startLo;
            i32 m_startHi;
        };
    };
    union {
        i64 m_interval;
        struct {
            i32 m_intervalLo;
            i32 m_intervalHi;
        };
    };

    ClockInterval() {
        m_start = 0;
        m_interval = 0;
    }

    void Start(u32 interval) {
        m_interval = interval;
        m_start = g_frameTime;
    }

    b32 Expired() const {
        return static_cast<i64>(g_frameTime) - m_start >= m_interval;
    }

    u32 Elapsed() const {
        i64 elapsed = static_cast<i64>(g_frameTime) - m_start;
        return elapsed < 0 ? 0 : static_cast<u32>(elapsed);
    }

    i64 Deadline() const {
        return m_interval + m_start;
    }

    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object);
};

#endif // GRUNTZ_CLOCKINTERVAL_H
