// SaveInfo.h - the save/quicksave record family shared by CGruntzMgr's manager
// TU (GruntzMgr.cpp: FillSaveInfo/Quicksave/Quickload) and the WM_COMMAND
// dispatcher TU (GruntzMgrCmd.cpp: the 0x807e quick-load command + the warp
// cheats). Extracted from GruntzMgr.cpp so the record is ONE canonical shape
// (it was a .cpp-local def + an i32-typed manager member with per-site casts).
//
// Field names are placeholders (m_<off>) except where a consumer proves the
// role; only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_SAVEINFO_H
#define GRUNTZ_GRUNTZ_SAVEINFO_H

#include <Ints.h>
#include <rva.h> // SIZE_UNKNOWN

// The save-slot info record FillSaveInfo populates and Quickload/HandleCommand
// (0x807e) consume: +0x00 flags (bit 0 = record valid), +0x04 the level id the
// quick-load passes to PassClickToPlayState, a 0x20-byte snapshot block at
// +0x14, the serial buffer at +0x35 (handed to ParseSerial / the 0x81a7 chat
// notify), the level name at +0x75 (CString-wrapped by the 0x807e path), and
// the two save-state ints at +0xf8/+0xfc.
SIZE_UNKNOWN(SaveInfo);
struct SaveInfo {
    u8 m_flags; // +0x00  bit 0 = record valid (the Quickload/0x807e gate)
    char m_pad1[0x4 - 0x1];
    i32 m_levelId; // +0x04  saved level id (0x807e passes it to PassClickToPlayState)
    char m_pad8[0x14 - 0x8];
    char m_snapshot[0x20]; // +0x14  snapshot block (FillSaveInfo EngineCopy dst)
    char m_pad34[0x1];
    char m_serial[0x75 - 0x35]; // +0x35  serial/name buffer (ParseSerial; 0x81a7 notify)
    char m_levelName[0x80];     // +0x75  level name (strcpy'd from GetWorldFileName() @0x928c0)
    char m_padf5[0xf8 - 0xf5];
    i32 m_f8;    // +0xf8  mirror of the manager's m_130 sub-mode gate (role unproven there)
    i32 m_isWon; // +0xfc  "won" flag (FillSaveInfo writes m_134 == 3)
};

// The +0x58 manager save-record sink. FillSaveInfo forwards the record +
// source-state ptr to Store; Quickload validates via Check; the warp cheats
// (0x8240..0x8245, 0x81a9) SetCurLevel the target then (0x81a9) re-Set. All
// reloc-masked thiscalls; +0x18 is the saved current-level id the 0x8174
// command passes to PassClickToPlayState.
SIZE_UNKNOWN(SaveSink58);
struct SaveSink58 {
    char m_pad0[0x18];
    i32 m_curLevel;                       // +0x18  saved current-level id (0x8174 restart source;
                                          //         SetCurLevel's target slot)
    void Store(SaveInfo* dst, char* src); // (this, dst, src) reloc-masked
    void Teardown();                      // (this) reloc-masked (Close)
    i32 Check(SaveInfo* rec);             // (this, rec) reloc-masked (Quickload load; @0x0e52c0)
    void SetCurLevel(i32 level);          // @0x0e5660 (thunk 0x4408; warp-cheat target)
    void Set();                           // @0x0e56b0 (thunk 0x3463; ghidra "Set")
};

// The manager's +0x44 HUD first-frame guard (only its +0x124 flag is touched:
// seeded by the manager, cleared by the 0x81d7 "Cheatz cleared" command).
SIZE_UNKNOWN(HudGuard44);
struct HudGuard44 {
    char m_pad0[0x124];
    i32 m_124;       // +0x124
    void Teardown(); // (this) reloc-masked (Close)
};

#endif // GRUNTZ_GRUNTZ_SAVEINFO_H
