#ifndef GRUNTZ_GRUNTZ_SPAWNLISTINLINE_H
#define GRUNTZ_GRUNTZ_SPAWNLISTINLINE_H

#include <Gruntz/SpawnList.h>

inline CSpawnEntry* CSpawnList::FirstEntry() {
    m_cursor = m_list.GetHeadPosition();
    if (m_cursor == NULL) {
        return NULL;
    }
    return NextEntry(m_cursor);
}

inline CSpawnEntry* CSpawnList::NextEntry() {
    if (m_cursor == NULL) {
        return NULL;
    }
    return NextEntry(m_cursor);
}

#endif // GRUNTZ_GRUNTZ_SPAWNLISTINLINE_H
