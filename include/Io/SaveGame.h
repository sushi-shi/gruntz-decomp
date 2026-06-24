// SaveGame.h - CSaveGame, the WAP32 save-game / saved-slot roster manager. Loads
// and writes a fixed-layout progress file through a stack-local CFileIO (the
// KERNEL32 file wrapper in FileStream.cpp): a 0xa1c-byte header blob followed by
// ten 0x100-byte slot records, each XOR-checksummed and registered into the
// global WwdGameReg (g_gameReg).
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

#include <Mfc.h> // CString + <windows.h>

#include <Io/FileStream.h> // CFileIO (the stack-local file wrapper)

// External WwdGameReg singleton (g_gameReg) referenced through a typed global so
// the `mov ecx,ds:g_gameReg; call <slot>` falls out as in retail. BuildLevelRezPath
// is the __thiscall method invoked from Register() (reloc-masked no-body callee).
struct WwdGameReg {
    int BuildLevelRezPath(int isEmpty, int hi, int lo, int id);
};
extern WwdGameReg* g_gameReg;

// _strncpy: the CRT helper the FillSlot path calls (reloc-masked).
extern "C" char* __cdecl strncpy(char* dest, const char* src, unsigned int n);

// The shared empty C string global (RVA 0x2293f4 / VA 0x6293f4), referenced by
// Register's name-fallback path (reloc-masked DIR32).
extern "C" char g_emptyString[];

// ---------------------------------------------------------------------------
// A single 0x100-byte saved-game slot record.
struct SaveSlot {
    int m_type;      // +0x00  (1 = normal, 3 = ...)
    int m_04;        // +0x04
    int m_08;        // +0x08
    int m_0c;        // +0x0c
    int m_checksum;  // +0x10  (Register(this) result)
    char m_14[0x20]; // +0x14  (name, strncpy'd 0x20)
    char m_34[0xcc]; // +0x34  pad to 0x100
};

class CSaveGame {
public:
    ~CSaveGame();

    void Reset();           // 0x000e4d20: Init() + m_name.Empty()
    void Init();            // 0x000e4d50: zero all 10 slots, set header field = 0x25
    int Load();             // 0x000e4d90
    int Save(int a, int b); // 0x000e4ea0
    void ComputeAll();      // 0x000e50a0
    int Verify();           // 0x000e50f0
    int FillSlot(SaveSlot* dst, const char* name, void* src); // 0x000e5130
    int CopySlot(SaveSlot* dst, const SaveSlot* src);         // 0x000e51d0
    int FillSlot2(SaveSlot* dst, int name, void* src);        // 0x000e5240
    int Register(SaveSlot* slot);                             // 0x000e5390
    int Encode(unsigned char* buf);                           // 0x000e5410
    int Decode(unsigned char* buf);                           // 0x000e5460
    SaveSlot* GetSlot(int i);                                 // 0x000e54b0
    int FillSlotByIndex(int idx, int name, void* src);        // 0x000e54e0
    void SetField18(int v);                                   // 0x000e5620
    void SetField1c(int v);                                   // 0x000e5660
    int CheckField20();                                       // 0x000e5690

    CString m_str0; // +0x00
    CString m_name; // +0x04
    // header blob at +0x08, 0xa1c bytes; named fields overlaid below.
    char m_08[0x10];      // +0x08
    unsigned int m_18;    // +0x18  (Init = 0x25; unsigned: SetField18 uses ja/jbe)
    unsigned int m_1c;    // +0x1c  (unsigned: SetField1c uses jbe)
    unsigned int m_20;    // +0x20  (magic == 0x42a check)
    char m_24[0x51];      // +0x24  pad to +0x75
    char m_75[0x83];      // +0x75  (a name string read by Register; pad to +0xf8)
    int m_f8;             // +0xf8
    int m_fc;             // +0xfc
    char m_100[0x924];    // +0x100 pad to +0xa24 (end of 0xa1c-byte header)
    SaveSlot m_slots[10]; // +0xa24, 10 x 0x100
};

#endif // SRC_IO_SAVEGAME_H
