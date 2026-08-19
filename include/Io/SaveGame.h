#ifndef SRC_IO_SAVEGAME_H
#define SRC_IO_SAVEGAME_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/BattlezRecord.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/QuestLevel.h>
#include <Io/FileStream.h>

#include <string.h>

GZ_ENUM_CONST_BEGIN(SaveSlotFlags)
    SAVESLOT_EMPTY = 0,
    SAVESLOT_PRESENT = 0x1,
    SAVESLOT_CHEATS_USED = 0x2
GZ_ENUM_CONST_END(SaveSlotFlags)

GZ_ENUM_CONST_BEGIN(SaveGameStringId)
    SAVE_STRING_SAVING_GAME = 0x81a6
GZ_ENUM_CONST_END(SaveGameStringId)

enum {
    SAVE_SLOT_COUNT = 10,
    SAVE_BATTLEZ_RECORD_COUNT = 40
};

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
        i32 m_isCustom;
    };
    union {
        i32 m_pathHi;
        i32 m_isWon;
        i32 m_isBattlez;
    };
};

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
    i32 InitializeNamedSlot(SaveSlot* dst, const char* name, CGruntzMgr* mgr);
    i32 CopySlot(SaveSlot* dst, const SaveSlot* src);
    i32 InitializeLevelSlot(SaveSlot* dst, i32 levelId, CGruntzMgr* mgr);
    i32 VerifySlot(SaveSlot* slot);
    i32 Register(SaveSlot* slot);
    i32 Encode(u8* buf);
    i32 Decode(u8* buf);
    SaveSlot* GetSlot(i32 i);
    i32 TempFileExistsAt(i32 index);
    i32 InitializeNamedSlotAt(i32 index, const char* name, CGruntzMgr* mgr);
    i32 StoreSlot(i32 idx, const SaveSlot* src);

    i32 CloseTempFile(SaveSlot* r);
    void SetMaxLevel(QuestLevel v);
    void SetCurLevel(QuestLevel v);
    QuestLevel CurrentLevel() const {
        return static_cast<QuestLevel>(m_curLevel);
    }
    i32 CheckMagic();
    void SetMagic();

    CString m_str0;
    CString m_name;

    i32 m_header[4];
    GZ_ENUM_STORAGE(QuestLevel, u32) m_maxLevel;
    GZ_ENUM_STORAGE(QuestLevel, u32) m_curLevel;
    u32 m_magic;
    BattlezRecord m_battlezRecords[SAVE_BATTLEZ_RECORD_COUNT];
    SaveSlot m_slots[SAVE_SLOT_COUNT];
};

// Inline in retail: CGruntzMgr::Run expands it (Reset plus the two CString members
// in reverse declaration order) at its delete site, and CGruntzMgr::Close calls the
// COMDAT copy the same object file emits at 0x85b50.
inline CSaveGame::~CSaveGame() {
    Reset();
}

BOOL CALLBACK SaveGameDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK LevelPreviewDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DeleteSaveDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK InfoLineDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK OkCancelDialogProc(HWND, UINT, WPARAM, LPARAM);

extern CSaveGame* g_saveDlgSink;

class CImagePool;
class CRezImage;
extern CImagePool* g_previewMgr;
extern CRezImage* g_previewImage;

void FillSaveDialog(HWND hDlg, CSaveGame* saveGame);
i32 DrawSaveGameMenu(HWND hDlg, i32 command, CSaveGame* saveGame);

int TempFileExists(SaveSlot* p);
void LabelSaveSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6);
void SetSaveSlotDialogName(HWND hWnd, CSaveGame* gate, SaveSlot* item);

void BuildLevelTitleString(HWND hDlg, CSaveGame* gate, SaveSlot* lev);

extern SaveSlot* g_slotState;

#endif // SRC_IO_SAVEGAME_H
