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
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes are
// load-bearing (campaign doctrine). Layout recovered from the ctor-less leaf
// accessors (Init zeroes [this+0x18]; GetSlot returns this+0xa24+i*0x100).
//
//   +0x00  m_str0   : CString (destroyed last in the dtor)
//   +0x04  m_name   : CString - the file name passed to CFileIO::Open
//   +0x08  m_header : 0xa1c-byte header blob (Read target). Named fields live
//                     INSIDE it: this+0x18 == m_header+0x10, etc.
//   +0xa24 m_slots  : 10 x 0x100-byte slot records (Read target #2).
#ifndef SRC_IO_SAVEGAME_H
#define SRC_IO_SAVEGAME_H
#include <rva.h>
#include <Gruntz/CGameRegistry.h>

#include <Mfc.h> // CString + <windows.h>

#include <Io/FileStream.h> // CFileIO (the stack-local file wrapper)

// External CGameRegistry singleton (g_gameReg) referenced through a typed global so
// the `mov ecx,ds:g_gameReg; call <slot>` falls out as in retail. BuildLevelRezPath
// is the __thiscall method invoked from Register() (reloc-masked no-body callee).
extern CGameRegistry* g_gameReg;

// _strncpy: the CRT helper the FillSlot path calls (reloc-masked).
extern "C" char* __cdecl strncpy(char* dest, const char* src, u32 n);

// The shared empty C string global (RVA 0x2293f4 / VA 0x6293f4), referenced by
// Register's name-fallback path (reloc-masked DIR32).
extern "C" char g_emptyString[];

// ---------------------------------------------------------------------------
// A single 0x100-byte saved-game slot record.
struct SaveSlot {
    i32 m_type;      // +0x00  (1 = normal, 3 = ...)
    i32 m_04;        // +0x04  level id
    i32 m_08;        // +0x08
    i32 m_0c;        // +0x0c
    i32 m_checksum;  // +0x10  (Register(this) result)
    char m_14[0x20]; // +0x14  (short name, strncpy'd 0x20)
    char m_34;       // +0x34
    char m_35[0x40]; // +0x35  (".sav" file name, wsprintf'd)
    char m_75[0x83]; // +0x75  (level path name read by Register/VerifySlot)
    i32 m_f8;        // +0xf8
    i32 m_fc;        // +0xfc
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
    void SetField18(i32 v);                                   // 0x000e5620
    void SetField1c(i32 v);                                   // 0x000e5660
    i32 CheckField20();                                       // 0x000e5690

    CString m_str0; // +0x00
    CString m_name; // +0x04
    // header blob at +0x08, 0xa1c bytes; named fields overlaid below.
    char m_08[0x10];      // +0x08
    u32 m_18;             // +0x18  (Init = 0x25; unsigned: SetField18 uses ja/jbe)
    u32 m_1c;             // +0x1c  (unsigned: SetField1c uses jbe)
    u32 m_20;             // +0x20  (magic == 0x42a check)
    char m_24[0x51];      // +0x24  pad to +0x75
    char m_75[0x83];      // +0x75  (a name string read by Register; pad to +0xf8)
    i32 m_f8;             // +0xf8
    i32 m_fc;             // +0xfc
    char m_100[0x924];    // +0x100 pad to +0xa24 (end of 0xa1c-byte header)
    SaveSlot m_slots[10]; // +0xa24, 10 x 0x100
};

#endif // SRC_IO_SAVEGAME_H
