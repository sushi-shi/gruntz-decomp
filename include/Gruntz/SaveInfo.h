#ifndef GRUNTZ_GRUNTZ_SAVEINFO_H
#define GRUNTZ_GRUNTZ_SAVEINFO_H

#include <Ints.h>
#include <rva.h> // SIZE_UNKNOWN

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

#endif // GRUNTZ_GRUNTZ_SAVEINFO_H
