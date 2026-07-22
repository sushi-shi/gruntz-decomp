#ifndef GRUNTZ_GRUNTZ_SPAWNLIST_H
#define GRUNTZ_GRUNTZ_SPAWNLIST_H

#include <rva.h>

#include <Ints.h>
#include <Mfc.h> // CPtrList (embedded) + CString (name member / by-value returns)

class CSpawnEntry {
public:
    CSpawnEntry(CString name, i32 data); // 0x11c630 (__thiscall ret 8; /GX by-value temp)
    RVA(0x0009a260, 0x1d)
    CString GetName() {
        return m_name;
    }
    CString GetTail(); // 0x9a830  the name past its 8-char group prefix

    CString m_name; // +0x00  the record name
    i32 m_flag;     // +0x04  = 0 at ctor; "wanted" mark (LoadObject*Resources set 1)
    i32 m_data;     // +0x08  = the ctor's 2nd arg (0 from the voice builder)
};
SIZE(0xc);

struct CSpawnNode {
    CSpawnNode* m_next;   // +0x00
    CSpawnNode* m_prev;   // +0x04
    CSpawnEntry* m_entry; // +0x08
};
SIZE(0xc);

class CSpawnList {
public:
    CSpawnList() {
        m_cursor = 0;
        m_lastPicked = -1;
    }
    ~CSpawnList();           // 0x99ca0  DeleteAllEntries + member ~CPtrList (def: AreaMgr.cpp)
    void ClearFlags();       // 0x9a420  zero every entry's m_flag
    void DeleteAllEntries(); // 0x9a450  delete every entry, then m_list.RemoveAll()
    CSpawnEntry* FindEntry(CString name, i32 useHash); // 0x9a0d0  (hash / strcmp match)
    CSpawnEntry* FindByName(const CString& name);      // 0x9a290  (was Extract/FindAdd)
    void AddVoiceSound(CString s, i32 flag);           // 0x11c560 (def: GruntSpawnConfig.cpp)

    CPtrList m_list;      // +0x00  the entry list (0x1c B; block size 10)
    CSpawnNode* m_cursor; // +0x1c  scan cursor (LoadObject*Resources' re-scan)
    i32 m_lastPicked;     // +0x20  last-picked index (-1; the weighted picker's memory)
};
SIZE(0x24);

#endif // GRUNTZ_GRUNTZ_SPAWNLIST_H
