#ifndef GRUNTZ_GRUNTZ_CHEATMGR_H
#define GRUNTZ_GRUNTZ_CHEATMGR_H

#include <rva.h>

#include <Ints.h>
#include <Utils/MapTyped.h>

struct CheatEntry {
    i32 m_commandId;
    i32 m_nonCheat;
};

class CCheatMgr {
public:
    CCheatMgr() {
        m_commandWindow = NULL;
        m_flag = false;
        m_pendingCodeLength = 0;
        m_cheatsUsed = false;
    }

    BOOL Init(HWND commandWindow);
    void Empty();
    BOOL AddCheat(const char* code, i32 cmdId, i32 nonCheat);
    CheatEntry* FindCheat(const char* code) {
        CheatEntry* entry = NULL;
        if (!MapLookup(m_entries, code, entry)) {
            return NULL;
        }
        return entry;
    }
    b32 HasUsedCheats() const {
        return m_cheatsUsed;
    }

    void RegisterCheats();
    void LoadCheatConfig();
    BOOL CheckCode(CString code);
    ~CCheatMgr();

    HWND m_commandWindow;
    CMapStringToPtr m_entries;
    // @identity-TODO: only cleared by initialization, reset, and accepted-code handling;
    // no reader establishes the role of this byte.
    u8 m_flag;
    char m_pendingCode[0x120 - 0x21];
    i32 m_pendingCodeLength;
    b32 m_cheatsUsed;
};

#endif // GRUNTZ_GRUNTZ_CHEATMGR_H
