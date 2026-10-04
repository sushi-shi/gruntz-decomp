#ifndef GRUNTZ_GRUNTZ_SPAWNLISTINLINE_H
#define GRUNTZ_GRUNTZ_SPAWNLISTINLINE_H

#include <Gruntz/SpawnList.h>

inline CResourceNameEntry* CResourceNameList::FirstEntry() {
    m_cursor = m_entries.GetHeadPosition();
    return NextEntry();
}

inline CResourceNameEntry* CResourceNameList::NextEntry() {
    if (m_cursor == NULL) {
        return NULL;
    }
    return NextEntry(m_cursor);
}

inline CResourceNameEntry* CResourceNameList::GetEntry(i32 index) {
    if (index >= GetCount()) {
        return NULL;
    }
    CResourceNameEntry* entry = FirstEntry();
    for (i32 remaining = index; remaining > 0; remaining--) {
        entry = NextEntry();
    }
    return entry;
}

#endif // GRUNTZ_GRUNTZ_SPAWNLISTINLINE_H
