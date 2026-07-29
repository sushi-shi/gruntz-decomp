#ifndef SRC_IO_SAVEGAME_H
#define SRC_IO_SAVEGAME_H
#include <rva.h>
#include <Gruntz/GameRegistry.h>

#include <Mfc.h>    // CString + <windows.h>
#include <string.h> // strncpy (the FillSlot CRT helper, reloc-masked)

#include <Io/FileStream.h> // CFile (the stack-local file wrapper)

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
        i32 m_pathLo;   // +0xf8  BuildLevelRezPath `lo` arg
        i32 m_f8;       //        mirror of the manager's m_130 sub-mode gate
        i32 m_isCustom; //        custom-level flag (BuildLevelTitleString @0xe44f9)
    };
    union {
        i32 m_pathHi;    // +0xfc  BuildLevelRezPath `hi` arg
        i32 m_isWon;     //        "won" flag (FillSaveInfo writes m_134 == 3)
        i32 m_isBattlez; //        battlez-vs-questz flag (BuildLevelTitleString @0xe44ff)
    };
};
SIZE(0x100); // 0x100-byte slot record (m_slots[] array stride)

class CSaveGame {
public:
    ~CSaveGame();

    i32 SaveGameFile(const char* dir); // 0x000e4b60: build the save-file paths
    void Reset();                      // 0x000e4d20: Init() + m_name.Empty()
    void Init();                       // 0x000e4d50: zero all 10 slots, set header field = 0x25
    i32 Load();                        // 0x000e4d90
    i32 Save(char* path, i32 b);       // 0x000e4ea0  (the slot's m_savePath / m_serial)
    i32 ComputeAll();                  // 0x000e50a0 (returns 1; the caller ignores it)
    i32 Verify();                      // 0x000e50f0
    i32 FillSlot(SaveSlot* dst, const char* name, void* src);  // 0x000e5130
    i32 CopySlot(SaveSlot* dst, const SaveSlot* src);          // 0x000e51d0
    i32 FillSlot2(SaveSlot* dst, i32 name, void* src);         // 0x000e5240
    i32 VerifySlot(SaveSlot* slot);                            // 0x000e52c0
    i32 Register(SaveSlot* slot);                              // 0x000e5390
    i32 Encode(u8* buf);                                       // 0x000e5410
    i32 Decode(u8* buf);                                       // 0x000e5460
    SaveSlot* GetSlot(i32 i);                                  // 0x000e54b0
    i32 FillSlotByIndex(i32 idx, const char* name, void* src); // 0x000e54e0
    i32 StoreSlot(i32 idx, const SaveSlot* src);               // 0x000e5520
    // 0x000e5550: a __thiscall METHOD that never touches `this` (the sole caller,
    // winapi_0e3a40_EndDialog, sets ecx = g_gameReg->m_saveSink before the call) - as a
    // free __stdcall it emitted the identical bytes and silently dropped the receiver.
    i32 CloseTempFile(SaveSlot* r); // 0x000e5550
    void SetMaxLevel(i32 v);        // 0x0e5620 (out-of-line: clamped max-level update)
    void SetCurLevel(i32 v);        // 0x0e5660 (out-of-line: clamped cur-level update)
    i32 CheckMagic();               // 0x000e5690
    void SetMagic();                // 0x000e56b0 (m_magic = 0x42a)

    CString m_str0; // +0x00  the directory CString
    CString m_name; // +0x04  the file-name CString passed to CFile::Open
    // 0xa1c-byte header blob at +0x08 (Read/Write/memset as a whole); the three
    // scalar fields below are named overlays inside it, the rest is opaque tail.
    i32 m_header[4];      // +0x08  header: {0, 1, checksum, 0} (ComputeAll)
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
SIZE_UNKNOWN(); // fully modeled but tail not proven; owner may upgrade

// The save/load modal dialog procs. They used to be three `extern "C" void ...Proc();`
// ILT-thunk placeholders; every thunk resolves (E9 rel32 out of retail's ILT band, and
// the DrawSaveGameMenu @0xe3f40 call-site relocations agree) to a proc defined right
// here: 0x2892 -> InfoLineDialogProc, 0x121c -> winapi_0e3a40_EndDialog,
// 0x1e3d -> LevelPreviewDlgProc. LoadGameMenu.cpp's GAME_INFO/GAME_DELETE sites push
// the SAME two thunks, so its LoadInfoDlgProc/LoadDeleteDlgProc placeholders were
// duplicates of these.
i32 CALLBACK winapi_0e35f0_EndDialog(HWND, UINT, WPARAM, LPARAM); // 0xe35f0 GAME_SAVE
i32 CALLBACK LevelPreviewDlgProc(HWND, UINT, WPARAM, LPARAM);     // 0xe3690 GAME_INFO
i32 CALLBACK winapi_0e3a40_EndDialog(HWND, UINT, WPARAM, LPARAM); // 0xe3a40 GAME_DELETE
i32 CALLBACK InfoLineDialogProc(HWND, UINT, WPARAM, LPARAM);      // 0xe3b20 GAME_OVERWRITE
i32 CALLBACK OkCancelDialogProc(HWND, UINT, WPARAM, LPARAM);      // 0xe3be0 GAME_SAVEMSG

extern CSaveGame* g_saveDlgSink; // 0x24c86c  the save dialog's active CSaveGame

extern char* g_areaNames[];
class CImagePool;
extern CImagePool* g_previewMgr;
extern void* g_previewImage;

void FillSaveDialog(HWND hDlg, CSaveGame* saveGame);               // 0x0e3c60
i32 DrawSaveGameMenu(HWND hDlg, i32 command, CSaveGame* saveGame); // 0x0e3f40

// File-scope prototypes moved from the .cpp (external linkage
// belongs in the owner header).
int TempFileExists(SaveSlot* p); // 0x0e5700 (defined below)
void LabelSaveSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6); // 0x0e3e80
void winapi_0e4850_SetDlgItemTextA(HWND hWnd, void* gate, SaveSlot* item);
// The record the GAME_INFO dialog is describing IS the save slot g_slotState points
// at: retail hands BuildLevelTitleString the very global winapi_0e4850_SetDlgItemTextA
// reads m_name (+0x14) off, and the five fields it touches (+0x04/+0x35/+0x75/+0xf8/
// +0xfc) are SaveSlot's, not CLevelInfo's. The `CLevelInfo*` spelling was a mis-decl.
void BuildLevelTitleString(HWND hDlg, CSaveGame* gate, SaveSlot* lev);

// 0x64c864 - the save/load dialog family's "record under the cursor". Every writer
// stores a CSaveGame::GetSlot() result and every reader dereferences a SaveSlot.
extern SaveSlot* g_slotState;

#endif // SRC_IO_SAVEGAME_H
