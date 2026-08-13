#ifndef GRUNTZ_GRUNTZ_CHEATMGR_H
#define GRUNTZ_GRUNTZ_CHEATMGR_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>

struct CheatEntry {
    i32 commandId;
    i32 flag;
};

class CCheatMgr {
public:
    CCheatMgr() {
        m_owner = NULL;
        m_flag = 0;
        m_pendingCodeLength = 0;
        m_cheatsUsed = 0;
    }

    BOOL Init(HWND owner);
    void Empty();
    BOOL AddCheat(const char* code, i32 cmdId, i32 flag);
    void RegisterCheats();
    void LoadCheatConfig();
    BOOL CheckCode(CString code);
    ~CCheatMgr();

    HWND m_owner;
    CMapStringToPtr m_map;
    u8 m_flag;
    char m_pendingCode[0x120 - 0x21];
    i32 m_pendingCodeLength;
    i32 m_cheatsUsed;
};

extern char s_cheat_20c920[];
extern char s_cheat_20c918[];
extern char s_cheat_20c90c[];
extern char s_cheat_20c900[];
extern char s_cheat_20c8f0[];
extern char s_cheat_20c8e0[];
extern char s_cheat_20c8d4[];
extern char s_cheat_20c8c4[];
extern char s_cheat_20c8b8[];
extern char s_cheat_20c8ac[];
extern char s_cheat_20c8a4[];
extern char s_cheat_20c89c[];
extern char s_cheat_20c894[];
extern char s_cheat_20c884[];
extern char s_cheat_20c878[];
extern char s_cheat_20c868[];
extern char s_cheat_20c85c[];
extern char s_cheat_20c84c[];
extern char s_cheat_20c838[];

#endif // GRUNTZ_GRUNTZ_CHEATMGR_H
