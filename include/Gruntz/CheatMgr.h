#ifndef GRUNTZ_GRUNTZ_CHEATMGR_H
#define GRUNTZ_GRUNTZ_CHEATMGR_H

#include <rva.h>

#include <Ints.h>
#include <Mfc.h> // CMapStringToPtr / CString / POSITION + <windows.h>

struct CheatEntry {
    i32 commandId; // +0x00  the WM_COMMAND id this cheat fires
    i32 flag;      // +0x04  enable flag (always 1 for the built-ins)
};
SIZE_UNKNOWN();

class CCheatMgr {
public:
    BOOL Init(i32 owner);                                 // 0x22ad0  seed +0, clear scalars
    void Empty();                                         // 0x22b00  free entries + clear
    BOOL AddCheat(const char* code, i32 cmdId, i32 flag); // 0x22be0
    void RegisterCheats();                                // 0x22c80  seed built-ins + load
    void LoadCheatConfig();                               // 0x22e60  bute/registry config loader
    BOOL CheckCode(CString code);                         // 0x23090  player-typed cheat probe
    ~CCheatMgr();                                         // 0x85e60

    // --- fields (placeholders; offsets load-bearing) ---
    i32 m_count;           // +0x00
    CMapStringToPtr m_map; // +0x04  (vptr@+0x04 .. +0x20) - 0x1c bytes
    u8 m_flag;             // +0x20
    char m_21[0x120 - 0x21];
    i32 m_120; // +0x120
    i32 m_124; // +0x124
};
SIZE_UNKNOWN();

// ---------------------------------------------------------------------------
// The 19 built-in cheat-code strings (obfuscated byte buffers in .data). Each is
// passed to AddCheat by address as the map key - reloc-masked DIR32 references.
// These are bound to their real .data RVAs via DATA_SYMBOL(..) rows in CheatMgr.cpp
// (a DATA() macro in a header is not seen by labels.py's per-.cpp text scan, and
// the char[] array mangling diverges clang-vs-cl5, so DATA_SYMBOL names the exact
// cl5 symbol `?s_cheat_<rva>@@3PADA`).
// ---------------------------------------------------------------------------
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
