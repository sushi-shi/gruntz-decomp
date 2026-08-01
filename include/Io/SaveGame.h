#ifndef SRC_IO_SAVEGAME_H
#define SRC_IO_SAVEGAME_H
#include <rva.h>
#include <Gruntz/GameRegistry.h>

#include <Mfc.h>
#include <string.h>

#include <Io/FileStream.h>

#include <EmptyString.h>

struct SaveSlot {
    union {
        i32 m_type;
        u8 m_flags;
    };
    i32 m_levelId;
    i32 m_count;
    i32 m_active;
    i32 m_checksum;
    union {
        char m_name[0x20];
        char m_snapshot[0x20];
    };
    char m_pad34;
    union {
        char m_savePath[0x40];
        char m_serial[0x40];
    };
    char m_levelName[0x83];
    union {
        i32 m_pathLo;
        i32 m_f8;
        i32 m_isCustom;
    };
    union {
        i32 m_pathHi;
        i32 m_isWon;
        i32 m_isBattlez;
    };
};
SIZE(0x100);

class CSaveGame {
public:
    ~CSaveGame();

    i32 SaveGameFile(const char* dir);
    void Reset();
    void Init();
    i32 Load();
    i32 Save(char* path, i32 b);
    i32 ComputeAll();
    i32 Verify();
    i32 FillSlot(SaveSlot* dst, const char* name, void* src);
    i32 CopySlot(SaveSlot* dst, const SaveSlot* src);
    i32 FillSlot2(SaveSlot* dst, i32 name, void* src);
    i32 VerifySlot(SaveSlot* slot);
    i32 Register(SaveSlot* slot);
    i32 Encode(u8* buf);
    i32 Decode(u8* buf);
    SaveSlot* GetSlot(i32 i);
    i32 FillSlotByIndex(i32 idx, const char* name, void* src);
    i32 StoreSlot(i32 idx, const SaveSlot* src);

    i32 CloseTempFile(SaveSlot* r);
    void SetMaxLevel(i32 v);
    void SetCurLevel(i32 v);
    i32 CheckMagic();
    void SetMagic();

    CString m_str0;
    CString m_name;

    i32 m_header[4];
    u32 m_maxLevel;
    u32 m_curLevel;
    u32 m_magic;
    char m_pad24[0x51];
    char m_pad75[0x83];
    i32 m_tailF8;
    i32 m_tailFC;
    char m_pad100[0x924];
    SaveSlot m_slots[10];
};
SIZE_UNKNOWN();

i32 CALLBACK winapi_0e35f0_EndDialog(HWND, UINT, WPARAM, LPARAM);
i32 CALLBACK LevelPreviewDlgProc(HWND, UINT, WPARAM, LPARAM);
i32 CALLBACK winapi_0e3a40_EndDialog(HWND, UINT, WPARAM, LPARAM);
i32 CALLBACK InfoLineDialogProc(HWND, UINT, WPARAM, LPARAM);
i32 CALLBACK OkCancelDialogProc(HWND, UINT, WPARAM, LPARAM);

extern CSaveGame* g_saveDlgSink;

extern char* g_areaNames[];
class CImagePool;
extern CImagePool* g_previewMgr;
extern void* g_previewImage;

void FillSaveDialog(HWND hDlg, CSaveGame* saveGame);
i32 DrawSaveGameMenu(HWND hDlg, i32 command, CSaveGame* saveGame);

int TempFileExists(SaveSlot* p);
void LabelSaveSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6);
void winapi_0e4850_SetDlgItemTextA(HWND hWnd, void* gate, SaveSlot* item);

void BuildLevelTitleString(HWND hDlg, CSaveGame* gate, SaveSlot* lev);

extern SaveSlot* g_slotState;

#endif // SRC_IO_SAVEGAME_H
