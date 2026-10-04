#include <StdAfx.h>
#include <Io/File.h>
#include <Utils/Text.h>

#include <Ints.h>

#include <Io/SaveGame.h>

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
#include <Io/FileTransaction.h>
#include <Io/SavePaths.h>
#include <MsgParam.h>
#include <Io/Settings.h>
#include <Wap32/ScreenGeometry.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

i32 g_savedMenuCmd = -1;

CDibMgr* g_previewMgr;

SaveSlot* g_slotState;

CDib* g_previewImage;

CSaveGame* g_saveDlgSink = NULL;

static const i32 s_saveFileHeaderBytes = 0xa1c;
static const i32 s_savePreviewBytes = 0x3843a;
static const i32 s_savePreviewBitmapOffset = 0xe;
static const u32 s_saveProgressMagic = 0x42a;

i32 CSaveGame::InitializeSaveDirectory(const std::string& saveDirectory) {
    if (!io::absolutePath(saveDirectory, m_saveDirectory)) return 0;
    m_progressFilePath = m_saveDirectory + "/Gruntz.sav";
    memset(m_header, 0, s_saveFileHeaderBytes);
    Init();
    Load();
    return 1;
}

void CSaveGame::Reset() {
    Init();
    (m_progressFilePath).erase();
}

void CSaveGame::Init() {
    memset(m_header, 0, s_saveFileHeaderBytes);
    m_maxLevel = QUESTLEVEL_TRAINING_FIRST;
    for (i32 i = 0; i < SAVE_SLOT_COUNT; i++) {
        SaveSlot* p = GetSlot(i);
        if (p != NULL) {
            memset(p, 0, sizeof(SaveSlot));
            copyTextToBuffer(io::slotBaseName(i), p->m_savePath, sizeof(p->m_savePath));
        }
    }
}

i32 CSaveGame::Load() {
    io::File file;
    if (!file.open((m_progressFilePath).c_str(), io::ReadOnly)) {
        return 0;
    }
    if (file.read(m_header, s_saveFileHeaderBytes) != s_saveFileHeaderBytes
        || file.read(m_slots, sizeof(m_slots)) != sizeof(m_slots)) {
        Init();
        return 0;
    }
    if (!Verify()) { Init(); return 0; }
    for (i32 index = 0; index < SAVE_SLOT_COUNT; ++index) {
        SaveSlot* slot = GetSlot(index);
        std::string name = io::slotBaseName(index);
        if (slot->m_type & SAVESLOT_PRESENT) {
            if (!io::snapshotName(index, slot->m_savePath, sizeof(slot->m_savePath), name)
                || !memchr(slot->m_name, 0, sizeof(slot->m_name))
                || !memchr(slot->m_levelName, 0, sizeof(slot->m_levelName))) {
                Init(); return 0;
            }
        }
        copyTextToBuffer(name, slot->m_savePath, sizeof(slot->m_savePath));
    }
    return 1;
}

i32 CSaveGame::SaveProgress() {
    io::FileTransaction file(m_progressFilePath);
    return file.good() && WriteProgress(file) && file.commit();
}

bool CSaveGame::WriteProgress(io::Output& target) {
    ComputeAll();
    const bool written = target.write(m_header, s_saveFileHeaderBytes)
        && target.write(m_slots, sizeof(m_slots));
    Verify();
    return written;
}

i32 CSaveGame::SlotIndex(const SaveSlot* slot) const {
    for (i32 index = 0; index < SAVE_SLOT_COUNT; ++index) {
        if (slot == &m_slots[index]) return index;
    }
    return -1;
}

std::string CSaveGame::SnapshotPath(const SaveSlot* slot) const {
    const i32 index = SlotIndex(slot);
    if (index < 0) return "";
    std::string name;
    if (!io::snapshotName(index, slot->m_savePath, sizeof(slot->m_savePath), name)) return "";
    return m_saveDirectory + "/" + name;
}

bool CSaveGame::SnapshotExists(const SaveSlot* slot) const {
    if (!slot || !(slot->m_type & SAVESLOT_PRESENT)) return false;
    io::File file;
    return file.open(SnapshotPath(slot), io::ReadOnly) && file.finish();
}

i32 CSaveGame::SaveSnapshot(SaveSlot* slot, i32 messageId) {
    const i32 index = SlotIndex(slot);
    if (index < 0 || !g_gameReg || !g_gameReg->m_curState || !g_gameReg->World()) return 0;
    CWaitCursorScope wait;
    const SaveSlot original = *slot;
    io::FileTransaction snapshot(m_saveDirectory + "/" + io::slotBaseName(index));
    if (!snapshot.good()) return 0;
    CPlay* state = g_gameReg->PickPlayOrPausedState();
    if (!state) return 0;
    g_gameReg->World()->GetDrawTarget()->TransEnter();
    state->LoadSBITextEdges(messageId);
    if (!SaveGame(g_gameReg, snapshot)
        || !SaveOverlayBufferShot(g_gameReg, SCREEN_HALF_W_PX, SCREEN_HALF_H_PX, snapshot)
        || !snapshot.finish()) return 0;
    std::string name;
    if (!io::snapshotName(index, snapshot.uniquePath().c_str(), snapshot.uniquePath().size() + 1, name)
        || !copyTextToBuffer(name, slot->m_savePath, sizeof(slot->m_savePath))) return 0;
    // The complete snapshot exists before its filename is made visible in progress.
    io::FileTransaction progress(m_progressFilePath);
    if (!progress.good() || !WriteProgress(progress) || !progress.commitReferencing(snapshot)) {
        *slot = original; return 0;
    }
    io::removePreviousSnapshot(index, m_saveDirectory, original.m_savePath, sizeof(original.m_savePath), name);
    return 1;
}

i32 CSaveGame::ComputeAll() {
    i32 sum = 0;
    for (i32 i = 0; i < SAVE_SLOT_COUNT; i++) {

        sum += Encode(reinterpret_cast<u8*>(GetSlot(i)));
    }
    m_header[0] = 0;
    m_header[1] = 1;
    m_header[2] = sum;
    m_header[3] = 0;
    return 1;
}

i32 CSaveGame::Verify() {
    i32 sum = 0;
    for (i32 i = 0; i < SAVE_SLOT_COUNT; i++) {

        sum += Decode(reinterpret_cast<u8*>(GetSlot(i)));
    }
    return m_header[2] == sum;
}

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
    if (reg->CheatMgr()->m_cheatsUsed != false) {
        dst->m_type = SAVESLOT_PRESENT | SAVESLOT_CHEATS_USED;
    }
    strncpy(dst->m_name, name, sizeof(dst->m_name) - 1);
    dst->m_checksum = Register(dst);
    return 1;
}

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
    if (mgr->CheatMgr()->m_cheatsUsed != false) {
        dst->m_type = SAVESLOT_PRESENT | SAVESLOT_CHEATS_USED;
    }
    dst->m_checksum = Register(dst);
    return 1;
}

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
        std::string(name)
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
        std::string(name)
    );
}

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

SaveSlot* CSaveGame::GetSlot(i32 i) {
    if (i < 0 || i >= SAVE_SLOT_COUNT) {
        return NULL;
    }
    return &m_slots[i];
}

i32 CSaveGame::InitializeNamedSlotAt(i32 index, const char* name, CGruntzMgr* mgr) {

    return InitializeNamedSlot(GetSlot(index), name, mgr);
}

i32 CSaveGame::StoreSlot(i32 idx, const SaveSlot* src) {
    return CopySlot(GetSlot(idx), src);
}

i32 CSaveGame::DeleteSnapshot(SaveSlot* slot) {
    if (SlotIndex(slot) < 0) return 0;
    const SaveSlot original = *slot;
    const std::string path = SnapshotPath(slot);
    slot->m_type = SAVESLOT_EMPTY;
    if (!SaveProgress()) { *slot = original; return 0; }
    // A failed cleanup leaves an unreferenced file, never a dangling progress entry.
    if (!path.empty()) remove(path.c_str());
    return 1;
}

void CSaveGame::SetMaxLevel(QuestLevel v) {
    if ((v < QUESTLEVEL_CAMPAIGN_END
         && (static_cast<u32>(IDX(v)) > static_cast<u32>(IDX(m_maxLevel))
             || static_cast<u32>(IDX(m_maxLevel)) > IDX(QUESTLEVEL_LAST)))
        || (static_cast<u32>(IDX(m_maxLevel)) > IDX(QUESTLEVEL_LAST)
            && static_cast<u32>(IDX(v)) > static_cast<u32>(IDX(m_maxLevel)))) {
        m_maxLevel = v;
    }
}

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

i32 CSaveGame::CheckMagic() {
    i32 v = m_magic;
    return v == s_saveProgressMagic;
}

void CSaveGame::SetMagic() {
    m_magic = s_saveProgressMagic;
}

i32 CSaveGame::TempFileExistsAt(i32 index) {
    return SnapshotExists(GetSlot(index));
}
