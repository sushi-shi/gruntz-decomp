#ifndef GRUNTZ_GRUNTZ_SPAWNLIST_H
#define GRUNTZ_GRUNTZ_SPAWNLIST_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>

class CSpawnEntry {
public:
    CSpawnEntry(CString name, i32 data);
    RVA(0x0009a260, 0x1d)
    CString GetName() {
        return m_name;
    }
    CString GetTail();

    CString m_name;
    i32 m_flag;
    i32 m_data;
};

struct CSpawnNode {
    CSpawnNode* m_next;
    CSpawnNode* m_prev;
    CSpawnEntry* m_entry;
};

class CSpawnList {
public:
    CSpawnList() {
        m_cursor = NULL;
        m_lastPicked = -1;
    }
    ~CSpawnList();
    void ClearFlags();
    void DeleteAllEntries();
    CSpawnEntry* FindEntry(CString name, i32 useHash);
    CSpawnEntry* FindByName(const CString& name);
    void AddVoiceSound(CString resourceName, i32 data);

    CPtrList m_list;

    CSpawnEntry* NextEntry(POSITION& pos) {
        return static_cast<CSpawnEntry*>(m_list.GetNext(pos));
    }
    POSITION m_cursor;
    i32 m_lastPicked;
};

#endif // GRUNTZ_GRUNTZ_SPAWNLIST_H
