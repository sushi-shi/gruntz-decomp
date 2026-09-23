#ifndef GRUNTZ_GRUNTZ_CHEATMGR_H
#define GRUNTZ_GRUNTZ_CHEATMGR_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>
#include <Utils/MapTyped.h>

struct CheatEntry {
    i32 m_commandId;
    i32 m_flag;
};

class CCheatMgr {
public:
    CCheatMgr() {
        m_owner = NULL;
        m_flag = false;
        m_pendingCodeLength = 0;
        m_cheatsUsed = false;
    }

    BOOL Init(HWND owner);
    void Empty();
    BOOL AddCheat(const char* code, i32 cmdId, i32 flag);
    CheatEntry* FindCheat(const char* code) {
        CheatEntry* entry = NULL;
        if (!MapLookup(m_map, code, entry)) {
            return NULL;
        }
        return entry;
    }
    void RegisterCheats();
    void LoadCheatConfig();
    BOOL CheckCode(CString code);
    ~CCheatMgr();

    HWND m_owner;
    CMapStringToPtr m_map;
    u8 m_flag;
    char m_pendingCode[0x120 - 0x21];
    i32 m_pendingCodeLength;
    b32 m_cheatsUsed;
};

#endif // GRUNTZ_GRUNTZ_CHEATMGR_H
