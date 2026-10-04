#ifndef GRUNTZ_GRUNTZ_CHEATMGR_H
#define GRUNTZ_GRUNTZ_CHEATMGR_H

#include <map>
#include <string>


#include <Ints.h>

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
    BOOL AddCheat(const std::string& code, i32 cmdId, i32 flag);
    CheatEntry* FindCheat(const std::string& code) {
        CheatEntry* entry = NULL;
        if (!MapLookup(m_map, code, entry)) {
            return NULL;
        }
        return entry;
    }
    void RegisterCheats();
    void LoadCheatConfig();
    BOOL CheckCode(std::string code);
    ~CCheatMgr();

    HWND m_owner;
    u8 m_flag;
    char m_pendingCode[0x120 - 0x21];
    i32 m_pendingCodeLength;
    b32 m_cheatsUsed;

private:
    std::map<std::string, CheatEntry*> m_map;
};

#endif
