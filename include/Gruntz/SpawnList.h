#ifndef GRUNTZ_GRUNTZ_SPAWNLIST_H
#define GRUNTZ_GRUNTZ_SPAWNLIST_H

#include <list>
class CSpawnEntry;

#include <string>

#include <Ints.h>

#include <Enums.h>
#include <Ints.h>

class CSpawnEntry {
public:
    CSpawnEntry(std::string name, i32 data);

    std::string GetName() {
        return m_name;
    }
    std::string GetTail();

    std::string m_name;
    b32 m_flag;
    i32 m_data;
};

class CSpawnList {
public:
    CSpawnList() {
        m_cursor = m_list.end();
        m_lastPicked = -1;
    }
    ~CSpawnList();
    void ClearFlags();
    void DeleteAllEntries();
    CSpawnEntry* FindEntry(std::string name, b32 useHash);
    CSpawnEntry* FindByName(const std::string& name);
    void AddVoiceSound(std::string resourceName, i32 data);

    i32 GetCount() const {
        return static_cast<i32>(m_list.size());
    }

    std::list<CSpawnEntry*> m_list;

    CSpawnEntry* NextEntry(std::list<CSpawnEntry*>::iterator& pos) {
        return static_cast<CSpawnEntry*>(*(pos++));
    }
    CSpawnEntry* FirstEntry();
    CSpawnEntry* NextEntry();
    CSpawnEntry* GetEntry(i32 index);
    std::list<CSpawnEntry*>::iterator m_cursor;
    i32 m_lastPicked;
};

inline CSpawnList::~CSpawnList() {
    DeleteAllEntries();
}

#endif
