// SaveGame.h - CSaveGame, the WAP32 save-game / saved-slot roster manager. Loads
// and writes a fixed-layout progress file through a stack-local CFileIO (the
// KERNEL32 file wrapper in FileStream.cpp): a 0xa1c-byte header blob followed by
// ten 0x100-byte slot records, each XOR-checksummed and registered into the
// global CGameRegistry (g_gameReg).
//
// NOTE ON PROVENANCE: the dynamic this/ecx tracer grouped these 18 methods under
// "CFileIO" because every file-touching method constructs a CFileIO temporary on
// its stack. They are NOT CFileIO methods - this is the OWNER class that *uses*
// CFileIO. Modeled here as its own class (the devs' true shape) per the
// correctness-over-artifacts doctrine; the build pairs objects by unit name, so
// the separate TU is matching-neutral.
//
// Fields are named from usage (roles inferred from the accessors); only the
// OFFSETS + code bytes are load-bearing (campaign doctrine). Layout recovered from
// the ctor-less leaf accessors (Init zeroes [this+0x18]; GetSlot returns
// this+0xa24+i*0x100).
//
//   +0x00  m_str0   : CString (destroyed last in the dtor)
//   +0x04  m_name   : CString - the file name passed to CFileIO::Open
//   +0x08  m_header : 0xa1c-byte header blob (Read target). Named fields live
//                     INSIDE it: this+0x18 == m_header+0x10, etc.
//   +0xa24 m_slots  : 10 x 0x100-byte slot records (Read target #2).
#ifndef SRC_IO_SAVEGAME_H
#define SRC_IO_SAVEGAME_H
#include <rva.h>
#include <Gruntz/GameRegistry.h>

#include <Mfc.h>    // CString + <windows.h>
#include <string.h> // strncpy (the FillSlot CRT helper, reloc-masked)

#include <Io/FileStream.h> // CFileIO (the stack-local file wrapper)

// External CGameRegistry singleton (g_gameReg) referenced through a typed global so
// the `mov ecx,ds:g_gameReg; call <slot>` falls out as in retail. BuildLevelRezPath
// is the __thiscall method invoked from Register() (reloc-masked no-body callee).

// The shared empty C string global (RVA 0x2293f4 / VA 0x6293f4), referenced by
// Register's name-fallback path (reloc-masked DIR32).
extern "C" char g_emptyString[];

// ---------------------------------------------------------------------------
// A single 0x100-byte saved-game slot record.
struct SaveSlot {
    i32 m_type;             // +0x00  (1 = normal, 3 = custom-world)
    i32 m_levelId;          // +0x04  level id (BuildLevelRezPath id arg)
    i32 m_count;            // +0x08  (init 0)
    i32 m_active;           // +0x0c  (init 1)
    i32 m_checksum;         // +0x10  (Register(this) result)
    char m_name[0x20];      // +0x14  short display name (strncpy'd 0x20)
    char m_pad34;           // +0x34
    char m_savePath[0x40];  // +0x35  ".sav" file name (wsprintf'd)
    char m_levelName[0x83]; // +0x75  level path name (read by Register/VerifySlot)
    i32 m_pathLo;           // +0xf8  BuildLevelRezPath `lo` arg
    i32 m_pathHi;           // +0xfc  BuildLevelRezPath `hi` arg
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
