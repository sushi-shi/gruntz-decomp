#ifndef GRUNTZ_GRUNTZ_SPAWNLIST_H
#define GRUNTZ_GRUNTZ_SPAWNLIST_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>

class CResourceNameEntry {
public:
    CResourceNameEntry(CString name, i32 data);
    RVA(0x0009a260, 0x1d)
    CString GetName() {
        return m_name;
    }
    CString GetObjectResourceSuffix();

    CString m_name;
    b32 m_resourcePresent;
    // @identity-TODO: constructor-written payload; no consumer establishes its meaning.
    i32 m_data;
};

class CResourceNameList {
public:
    CResourceNameList() {
        m_cursor = NULL;
        m_lastPicked = -1;
    }
    ~CResourceNameList();
    void ClearPresenceMarks();
    void DeleteAllEntries();
    CResourceNameEntry* FindEntry(CString name, b32 allowPrefixMatch);
    CResourceNameEntry* FindByName(const CString& name);
    void AddEntry(CString resourceName, i32 data);

    i32 GetCount() const {
        return m_entries.GetCount();
    }

    i32 GetLastPicked() const {
        return m_lastPicked;
    }

    void SetLastPicked(i32 index) {
        m_lastPicked = index;
    }

    CPtrList m_entries;

    CResourceNameEntry* NextEntry(POSITION& pos) {
        return static_cast<CResourceNameEntry*>(m_entries.GetNext(pos));
    }
    CResourceNameEntry* FirstEntry();
    CResourceNameEntry* NextEntry();
    CResourceNameEntry* GetEntry(i32 index);
    POSITION m_cursor;
    i32 m_lastPicked;
};

inline CResourceNameList::~CResourceNameList() {
    DeleteAllEntries();
}

#endif // GRUNTZ_GRUNTZ_SPAWNLIST_H
