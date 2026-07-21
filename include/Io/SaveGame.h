#ifndef SRC_IO_SAVEGAME_H
#define SRC_IO_SAVEGAME_H
#include <rva.h>
#include <Gruntz/GameRegistry.h>

#include <Mfc.h>    // CString + <windows.h>
#include <string.h> // strncpy (the FillSlot CRT helper, reloc-masked)

#include <Io/FileStream.h> // CFileIO (the stack-local file wrapper)

#include <EmptyString.h> // g_emptyString (the shared "" constant)

// The ONE 0x100 save record (the ex-SaveInfo twin is MERGED here: same layout,
// the in-memory quickload code's role names live as union arms).
struct SaveSlot {
    union {
        i32 m_type; // +0x00  (1 = normal, 3 = custom-world)
        u8 m_flags; //        low byte: bit 0 = record valid (the Quickload/0x807e gate)
    };
    i32 m_levelId;  // +0x04  level id (BuildLevelRezPath id arg / PassClickToPlayState)
    i32 m_count;    // +0x08  (init 0)
    i32 m_active;   // +0x0c  (init 1)
    i32 m_checksum; // +0x10  (Register(this) result)
    union {
        char m_name[0x20];     // +0x14  short display name (strncpy'd 0x20)
        char m_snapshot[0x20]; //        snapshot block (FillSaveInfo EngineCopy dst)
    };
    char m_pad34; // +0x34
    union {
        char m_savePath[0x40]; // +0x35  ".sav" file name (wsprintf'd)
        char m_serial[0x40];   //        serial/name buffer (ParseSerial; 0x81a7 notify)
    };
    char m_levelName[0x83]; // +0x75  level path name (Register/VerifySlot; quickload strcpy)
    union {
        i32 m_pathLo; // +0xf8  BuildLevelRezPath `lo` arg
        i32 m_f8;     //        mirror of the manager's m_130 sub-mode gate
    };
    union {
        i32 m_pathHi; // +0xfc  BuildLevelRezPath `hi` arg
        i32 m_isWon;  //        "won" flag (FillSaveInfo writes m_134 == 3)
    };
};

class CSaveGame {
public:
    ~CSaveGame();

    i32 SaveGameFile(const char* dir); // 0x000e4b60: build the save-file paths
    void Reset();                      // 0x000e4d20: Init() + m_name.Empty()
    void Init();                       // 0x000e4d50: zero all 10 slots, set header field = 0x25
    i32 Load();                        // 0x000e4d90
    i32 Save(i32 a, i32 b);            // 0x000e4ea0
    void ComputeAll();                 // 0x000e50a0
    i32 Verify();                      // 0x000e50f0
    i32 FillSlot(SaveSlot* dst, const char* name, void* src); // 0x000e5130
    i32 CopySlot(SaveSlot* dst, const SaveSlot* src);         // 0x000e51d0
    i32 FillSlot2(SaveSlot* dst, i32 name, void* src);        // 0x000e5240
    i32 VerifySlot(SaveSlot* slot);                           // 0x000e52c0
    i32 Register(SaveSlot* slot);                             // 0x000e5390
    i32 Encode(u8* buf);                                      // 0x000e5410
    i32 Decode(u8* buf);                                      // 0x000e5460
    SaveSlot* GetSlot(i32 i);                                 // 0x000e54b0
    i32 FillSlotByIndex(i32 idx, i32 name, void* src);        // 0x000e54e0
    i32 StoreSlot(i32 idx, const SaveSlot* src);              // 0x000e5520
    void SetMaxLevel(i32 v); // 0x0e5620 (out-of-line: clamped max-level update)
    void SetCurLevel(i32 v); // 0x0e5660 (out-of-line: clamped cur-level update)
    i32 CheckMagic();        // 0x000e5690
    void SetMagic();         // 0x000e56b0 (m_magic = 0x42a)

    CString m_str0; // +0x00  the directory CString
    CString m_name; // +0x04  the file-name CString passed to CFileIO::Open
    // 0xa1c-byte header blob at +0x08 (Read/Write/memset as a whole); the three
    // scalar fields below are named overlays inside it, the rest is opaque tail.
    char m_header[0x10];  // +0x08  header base (ComputeAll writes [0..0xc])
    u32 m_maxLevel;       // +0x18  (Init = 0x25; SetMaxLevel clamps, unsigned ja/jbe)
    u32 m_curLevel;       // +0x1c  (SetCurLevel; == 0x20 -> Init, unsigned jbe)
    u32 m_magic;          // +0x20  (CheckMagic == 0x42a)
    char m_pad24[0x51];   // +0x24  opaque header tail
    char m_pad75[0x83];   // +0x75  opaque header tail
    i32 m_tailF8;         // +0xf8
    i32 m_tailFC;         // +0xfc
    char m_pad100[0x924]; // +0x100 to +0xa24 (end of 0xa1c header)
    SaveSlot m_slots[10]; // +0xa24, 10 x 0x100
};

#endif // SRC_IO_SAVEGAME_H
