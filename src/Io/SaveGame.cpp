#include <rva.h>

#include <Io/SaveGame.h>

#include <MfcWin.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Enums.h>
#include <Gruntz/ChainForward.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/SaveSlotCtrlId.h>
#include <Gruntz/WaitCursorScope.h>
#include <Image/Image.h>
#include <Image/ImagePool.h>
#include <Image/RezDecodeKind.h>
#include <Io/GameSave.h>
#include <MsgParam.h>
#include <Utils/RegistryHelper.h>
#include <Wap32/ScreenGeometry.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DATA(0x00214a34)
i32 g_savedMenuCmd = -1;
DATA(0x0024d76c)
CImagePool* g_previewMgr;
DATA(0x0024d7bc)
SaveSlot* g_slotState;
DATA(0x0024d7c0)
CRezImage* g_previewImage;
DATA(0x0024d7c4)
CSaveGame* g_saveDlgSink = NULL;

static const i32 SAVE_FILE_HEADER_BYTES = 0xa1c;
static const i32 SAVE_PREVIEW_BYTES = 0x3843a;
static const i32 SAVE_PREVIEW_BITMAP_OFFSET = 0xe;
static const u32 SAVE_PROGRESS_MAGIC = 0x42a;

RVA(0x000e4c90, 0x158)
i32 CSaveGame::InitializeSaveDirectory(const char* saveDirectory) {
    if (saveDirectory == NULL) {
        return 0;
    }
    m_saveDirectory = saveDirectory;
    m_progressFilePath = m_saveDirectory + "Gruntz.sav";
    memset(m_header, 0, SAVE_FILE_HEADER_BYTES);
    Init();
    Load();
    for (i32 i = 0; i < SAVE_SLOT_COUNT; i++) {
        SaveSlot* slot = GetSlot(i);
        if (slot != NULL) {
            char numbuf[16];
            _itoa(i + 1, numbuf, 10);
            wsprintfA(slot->m_savePath, m_saveDirectory + "Slot" + numbuf + ".sav");
        }
    }
    return 1;
}

RVA(0x000e4e50, 0x12)
void CSaveGame::Reset() {
    Init();
    m_progressFilePath.Empty();
}

RVA(0x000e4e80, 0x2f)
void CSaveGame::Init() {
    m_maxLevel = QUESTLEVEL_TRAINING_FIRST;
    for (i32 i = 0; i < SAVE_SLOT_COUNT; i++) {
        SaveSlot* p = GetSlot(i);
        if (p != NULL) {
            memset(p, 0, sizeof(SaveSlot));
        }
    }
}

RVA(0x000e4ec0, 0xcc)
i32 CSaveGame::Load() {
    CFile file;
    if (!file.Open(m_progressFilePath, CFile::modeRead, NULL)) {
        return 0;
    }
    file.Read(m_header, SAVE_FILE_HEADER_BYTES);
    file.Read(m_slots, sizeof(m_slots));
    file.Close();
    if (!Verify()) {
        Init();
    }
    return 1;
}

RVA(0x000e4fd0, 0x18c)
i32 CSaveGame::Save(char* screenshotPath, i32 messageId) {
    CWaitCursorScope wait;
    CFile file;
    if (!file.Open(m_progressFilePath, CFile::modeCreate, NULL)) {
        return 0;
    }
    file.Close();
    if (!file.Open(m_progressFilePath, CFile::modeWrite, NULL)) {
        return 0;
    }
    ComputeAll();
    file.Write(m_header, SAVE_FILE_HEADER_BYTES);
    file.Write(m_slots, sizeof(m_slots));
    file.Close();
    Verify();
    if (screenshotPath != NULL) {
        CPlay* state = static_cast<CPlay*>(g_gameReg->m_curState);
        g_gameReg->m_world->m_drawTarget->TransEnter();
        state->LoadSBITextEdges(messageId);
        if (!SaveGame(g_gameReg, screenshotPath)) {
            return 0;
        }
        if (!SaveOverlayBufferShot(
                g_gameReg->m_settings,
                g_gameReg,
                SCREEN_HALF_W_PX,
                SCREEN_HALF_H_PX,
                screenshotPath,
                1
            )) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000e51d0, 0x3e)
i32 CSaveGame::ComputeAll() {
    i32 sum = 0;
    for (i32 i = 0; i < SAVE_SLOT_COUNT; i++) {
        // Byte-forced checksum view.

        sum += Encode(reinterpret_cast<u8*>(GetSlot(i)));
    }
    m_header[0] = 0;
    m_header[1] = 1;
    m_header[2] = sum;
    m_header[3] = 0;
    return 1;
}

RVA(0x000e5220, 0x2f)
i32 CSaveGame::Verify() {
    i32 sum = 0;
    for (i32 i = 0; i < SAVE_SLOT_COUNT; i++) {
        // Byte-forced checksum view.
        sum += Decode(reinterpret_cast<u8*>(GetSlot(i)));
    }
    return m_header[2] == sum;
}

RVA(0x000e5260, 0x78)
i32 CSaveGame::InitializeNamedSlot(SaveSlot* dst, const char* name, CGruntzMgr* reg) {
    if (dst == NULL) {
        return 0;
    }
    if (reg == NULL) {
        return 0;
    }
    dst->m_type = SAVESLOT_PRESENT;
    dst->m_levelId = (static_cast<CPlay*>(reg->m_curState))->m_levelIndex;
    dst->m_count = 0;
    dst->m_active = true;
    if (reg->m_cheatMgr->m_cheatsUsed != false) {
        dst->m_type = SAVESLOT_PRESENT | SAVESLOT_CHEATS_USED;
    }
    strncpy(dst->m_name, name, sizeof(dst->m_name));
    dst->m_checksum = Register(dst);
    return 1;
}

RVA(0x000e5300, 0x49)
i32 CSaveGame::CopySlot(SaveSlot* dst, const SaveSlot* src) {
    if (dst == NULL) {
        return 0;
    }
    if (src == NULL) {
        return 0;
    }
    dst->m_type = src->m_type;
    dst->m_levelId = src->m_levelId;
    dst->m_count = src->m_count;
    dst->m_active = src->m_active;
    dst->m_checksum = src->m_checksum;
    dst->m_checksum = Register(dst);
    return 1;
}

RVA(0x000e5370, 0x54)
i32 CSaveGame::InitializeLevelSlot(SaveSlot* dst, i32 levelId, CGruntzMgr* mgr) {
    if (dst == NULL) {
        return 0;
    }
    if (mgr == NULL) {
        return 0;
    }
    dst->m_type = SAVESLOT_PRESENT;
    dst->m_levelId = levelId;
    dst->m_count = 0;
    if (mgr->m_cheatMgr->m_cheatsUsed != false) {
        dst->m_type = SAVESLOT_PRESENT | SAVESLOT_CHEATS_USED;
    }
    dst->m_checksum = Register(dst);
    return 1;
}

RVA(0x000e53f0, 0x99)
i32 CSaveGame::VerifySlot(SaveSlot* slot) {
    if (slot == NULL) {
        return 0;
    }
    b32 isBattlez = slot->m_isBattlez;
    b32 isCustom = slot->m_isCustom;
    const char* name = (isBattlez == false && isCustom == false) ? "" : slot->m_levelName;
    i32 r = g_gameReg->ResolveLevelChecksum(
        isBattlez == false,
        isBattlez,
        isCustom,
        slot->m_levelId,
        CString(name)
    );
    if (r == 0) {
        g_gameReg->EnterModalUI(
            "The level that this game was saved on does not exist!\n\nThis "
            "saved game cannot be loaded and should be deleted."
        );
        return 0;
    }
    if (slot->m_checksum != r) {
        g_gameReg->EnterModalUI(
            "The level that this game was saved on has changed!\n\nThis "
            "saved game cannot be loaded and should be deleted."
        );
        return 0;
    }
    return 1;
}

RVA(0x000e54c0, 0x59)
i32 CSaveGame::Register(SaveSlot* slot) {
    if (slot == NULL) {
        return 0;
    }
    b32 isBattlez = slot->m_isBattlez;
    b32 isCustom = slot->m_isCustom;
    const char* name = (isBattlez == false && isCustom == false) ? "" : slot->m_levelName;

    return g_gameReg->ResolveLevelChecksum(
        isBattlez == false,
        isBattlez,
        isCustom,
        slot->m_levelId,
        CString(name)
    );
}

RVA(0x000e5540, 0x3d)
i32 CSaveGame::Encode(u8* buf) {
    if (buf == NULL) {
        return 0;
    }
    i32 acc = 0;
    for (u32 i = 0; i < sizeof(SaveSlot); i++) {
        u8 t = buf[i];
        acc += static_cast<i32>((t & 0xff)) * static_cast<i32>(i);
        buf[i] = static_cast<u8>((t ^ i));
    }
    return acc;
}

RVA(0x000e5590, 0x3f)
i32 CSaveGame::Decode(u8* buf) {
    if (buf == NULL) {
        return 0;
    }
    i32 acc = 0;
    for (u32 i = 0; i < sizeof(SaveSlot); i++) {
        u8 t = static_cast<u8>((i ^ buf[i]));
        buf[i] = t;
        acc += static_cast<i32>((t & 0xff)) * static_cast<i32>(i);
    }
    return acc;
}

RVA(0x000e55e0, 0x1f)
SaveSlot* CSaveGame::GetSlot(i32 i) {
    if (i < 0 || i >= SAVE_SLOT_COUNT) {
        return NULL;
    }
    return &m_slots[i];
}

RVA(0x000e5610, 0x25)
i32 CSaveGame::InitializeNamedSlotAt(i32 index, const char* name, CGruntzMgr* mgr) {

    return InitializeNamedSlot(GetSlot(index), name, mgr);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000e5650, 0x20)
i32 CSaveGame::StoreSlot(i32 idx, const SaveSlot* src) {
    return CopySlot(GetSlot(idx), src);
}

RVA(0x000e5680, 0x9a)
i32 CSaveGame::CloseTempFile(SaveSlot* p) {
    if (p == NULL) {
        return 0;
    }
    CFile file;
    if (file.Open(p->m_savePath, CFile::modeRead, NULL)) {
        file.Close();
        CFile::Remove(p->m_savePath);
    }
    p->m_type = SAVESLOT_EMPTY;
    return 1;
}

RVA(0x000e5750, 0x27)
void CSaveGame::SetMaxLevel(QuestLevel v) {
    if ((v < QUESTLEVEL_CAMPAIGN_END
         && (static_cast<u32>(IDX(v)) > static_cast<u32>(IDX(m_maxLevel))
             || static_cast<u32>(IDX(m_maxLevel)) > IDX(QUESTLEVEL_LAST)))
        || (static_cast<u32>(IDX(m_maxLevel)) > IDX(QUESTLEVEL_LAST)
            && static_cast<u32>(IDX(v)) > static_cast<u32>(IDX(m_maxLevel)))) {
        m_maxLevel = v;
    }
}

RVA(0x000e5790, 0x1e)
void CSaveGame::SetCurLevel(QuestLevel v) {
    if (v >= QUESTLEVEL_CAMPAIGN_END) {
        return;
    }
    if (v <= m_curLevel) {
        return;
    }
    m_curLevel = v;
    if (v == QUESTLEVEL_CAMPAIGN_LAST) {
        SetMagic();
    }
}

RVA(0x000e57c0, 0xf)
i32 CSaveGame::CheckMagic() {
    i32 v = m_magic;
    return v == SAVE_PROGRESS_MAGIC;
}

RVA(0x000e57e0, 0x8)
void CSaveGame::SetMagic() {
    m_magic = SAVE_PROGRESS_MAGIC;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000e5800, 0x16)
i32 CSaveGame::TempFileExistsAt(i32 index) {
    return TempFileExists(GetSlot(index));
}

RVA(0x000e5830, 0x9e)
int TempFileExists(SaveSlot* p) {
    if (p != NULL && (p->m_type & SAVESLOT_PRESENT)) {
        CFile file;
        if (file.Open(p->m_savePath, CFile::modeRead, NULL)) {
            file.Close();
            return 1;
        }
    }
    return 0;
}
