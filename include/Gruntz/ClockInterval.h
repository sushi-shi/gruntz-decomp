#ifndef GRUNTZ_CLOCKINTERVAL_H
#define GRUNTZ_CLOCKINTERVAL_H

#include <rva.h>

#include <Clock64.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <Rez/FrameClock.h>

class CFileMemBase;
struct CGameObject;

struct ClockInterval {
    Clock64 m_start;
    Clock64 m_interval;

    ClockInterval() {
        m_start.m_v = 0;
        m_interval.m_v = 0;
    }

    i64 Deadline() const {
        return m_interval.m_v + m_start.m_v;
    }

    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object);
};

#endif // GRUNTZ_CLOCKINTERVAL_H
