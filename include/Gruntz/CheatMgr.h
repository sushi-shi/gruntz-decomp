// CheatMgr.h - the cheat-code dictionary (C:\Proj\Gruntz\, the 0x22bxx region).
//
// CCheatMgr is a plain (non-CObject, no vtable) value bag that owns a
// CMapStringToPtr keyed by each cheat's (obfuscated) code string. Each map value
// is a heap-allocated 8-byte CheatEntry {commandId, flag}; AddCheat inserts one,
// Empty() frees every entry's payload + RemoveAll()s the map + clears the scalar
// state, and the destructor runs Empty() then the embedded map's ~CMapStringToPtr.
// RegisterCheats seeds the 19 built-in codes (each -> a WM_COMMAND id) then calls
// LoadCheatConfig (the bute/registry config loader, defined in another TU).
//
// Recovered shape (the map sits at +0x04, NOT offset 0 - so CCheatMgr EMBEDS a
// CMapStringToPtr member rather than deriving from it):
//   m_count  +0x00  a DWORD reset to 0 by Empty()
//   m_map    +0x04  CMapStringToPtr (0x1c bytes: CObject vptr + 6 fields, spans
//                   +0x04..+0x20), keyed by the cheat code, value = CheatEntry*
//   m_flag   +0x20  a BYTE flag reset to 0 by Empty()
//   ...      +0x21..+0x120  inline state (not touched by these four methods)
//   m_120    +0x120 a DWORD reset to 0 by Empty()
//   m_124    +0x124 a DWORD reset to 0 by Empty()
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing (campaign doctrine). Layout recovered from Empty()/the dtor + the
// AddCheat stores; engine callees / MFC helpers are reloc-masked (no body).
#ifndef GRUNTZ_GRUNTZ_CHEATMGR_H
#define GRUNTZ_GRUNTZ_CHEATMGR_H

#include <rva.h>

#include <Ints.h>
#include <Mfc.h> // CMapStringToPtr / CString / POSITION + <windows.h>

// ---------------------------------------------------------------------------
// The per-cheat map value: a heap-allocated {commandId, flag} pair (operator
// new(8), freed via operator delete from Empty()).
// ---------------------------------------------------------------------------
struct CheatEntry {
    i32 commandId; // +0x00  the WM_COMMAND id this cheat fires
    i32 flag;      // +0x04  enable flag (always 1 for the built-ins)
};

// ---------------------------------------------------------------------------
// CCheatMgr - the cheat-code dictionary (no vtable; a value bag).
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// The 19 built-in cheat-code strings (obfuscated byte buffers in .data). Each is
// passed to AddCheat by address as the map key - reloc-masked DIR32 references.
// ---------------------------------------------------------------------------
DATA(0x0020c920)
extern char s_cheat_20c920[];
DATA(0x0020c918)
extern char s_cheat_20c918[];
DATA(0x0020c90c)
extern char s_cheat_20c90c[];
DATA(0x0020c900)
extern char s_cheat_20c900[];
DATA(0x0020c8f0)
extern char s_cheat_20c8f0[];
DATA(0x0020c8e0)
extern char s_cheat_20c8e0[];
DATA(0x0020c8d4)
extern char s_cheat_20c8d4[];
DATA(0x0020c8c4)
extern char s_cheat_20c8c4[];
DATA(0x0020c8b8)
extern char s_cheat_20c8b8[];
DATA(0x0020c8ac)
extern char s_cheat_20c8ac[];
DATA(0x0020c8a4)
extern char s_cheat_20c8a4[];
DATA(0x0020c89c)
extern char s_cheat_20c89c[];
DATA(0x0020c894)
extern char s_cheat_20c894[];
DATA(0x0020c884)
extern char s_cheat_20c884[];
DATA(0x0020c878)
extern char s_cheat_20c878[];
DATA(0x0020c868)
extern char s_cheat_20c868[];
DATA(0x0020c85c)
extern char s_cheat_20c85c[];
DATA(0x0020c84c)
extern char s_cheat_20c84c[];
DATA(0x0020c838)
extern char s_cheat_20c838[];

#endif // GRUNTZ_GRUNTZ_CHEATMGR_H
