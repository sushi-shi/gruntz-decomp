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

// DISSOLVED (Fable A2, 2026-07-14): the "+0x58 manager save-record sink" view
// (SaveSink58) WAS the CSaveGame (<Io/SaveGame.h>) - every claimed method is
// CSaveGame's (SetCurLevel @0xe5660, "Set" == SetMagic @0xe56b0, "Check" ==
// VerifySlot @0xe52c0), and the "+0x18 m_curLevel" claim was WRONG BY 4:
// SetCurLevel's body reads/writes +0x1c (CSaveGame::m_curLevel), while the
// 0x8174 restart command's `mov ecx,[eax+0x18]` reads CSaveGame::m_maxLevel.
// Its SaveInfo record is layout-identical to CSaveGame's SaveSlot (+0x04
// levelId / +0x14 name / +0x35 path / +0x75 levelName / +0xf8/+0xfc) -
// @fold-TODO SaveInfo == SaveSlot (the m_saveInfoRec consumers).

// (HudGuard44 is DISSOLVED, 2026-07-16: the manager's +0x44 object IS the
// canonical CCheatMgr (<Gruntz/CheatMgr.h>) - the layouts coincide exactly
// (i32 m_124 at +0x124, the "a cheat was used" flag the HUD warning + the
// 0x81d7 "Cheatz cleared" command touch), CGruntzMgr::Run news it with
// Init(hwnd) @0x22ad0 + RegisterCheats @0x22c80, and the "Teardown" was
// ~CCheatMgr @0x85e60.)

#endif // GRUNTZ_GRUNTZ_SAVEINFO_H
