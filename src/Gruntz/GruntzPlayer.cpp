#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntzPlayer.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <DinMgr2/InputMgrPtr.h>
#include <Dsndmgr/MidiManager.h>
#include <Enums.h>
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/AreaMgr.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CBrickz.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/DrawDebugStats.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/GameText.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/InputState.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/Minimap.h>
#include <Gruntz/Multi.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/PlayerCommandKind.h>
#include <Gruntz/PlayStringId.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/SBI_Image.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/String.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/View.h>
#include <Gruntz/VoiceManager.h>
#include <Gruntz/Warlord.h>
#include <Gruntz/WorldSoundSet.h>
#include <Gruntz/WwdGameReg.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Io/SaveGame.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/EngStr.h>
#include <Wap32/Object.h>
#include <Wap32/ScreenGeometry.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <ddraw.h>
#include <new>
#include <stdio.h>
#include <string.h>
#include <windowsx.h>

class CImage;

GruntzPlayer::GruntzPlayer() {
    m_playerIndex = -1;
    m_networkPlayerId = -2;
    m_active = false;
    m_joined = false;
    m_humanControlled = true;
    m_name = "";
    m_color = TINT_ORANGE;
    m_difficulty = BZDIFF_EASY;
    m_focusX = 0;
    m_focusY = 0;
    m_maxGruntz = 0xf;
    m_doneFlag = false;
    m_optionsPresenceCounted = false;
    m_latency.Clear();
}

i32 GruntzPlayer::SeedForSlot(i32 index) {
    m_playerIndex = index;
    m_networkPlayerId = -2;
    m_active = false;
    m_joined = false;
    m_humanControlled = true;
    m_name = "";

    m_color = static_cast<ColorTint>(index);
    m_difficulty = BZDIFF_EASY;
    m_focusX = 0;
    m_focusY = 0;
    m_maxGruntz = 0xf;
    m_doneFlag = false;
    m_optionsPresenceCounted = false;
    m_name = GetDefaultName(0);
    m_latency.Clear();
    return 1;
}

void GruntzPlayer::Clear() {
    CLEAR_GRUNTZ_PLAYER;
}

i32 GruntzPlayer::Reset() {
    CLEAR_GRUNTZ_PLAYER;
    return 1;
}

i32 GruntzPlayer::ClearRoundState() {
    m_active = true;
    m_ready = false;
    m_doneFlag = false;
    m_optionsPresenceCounted = false;
    m_latency.Clear();
    return 1;
}

i32 FillColorCombo(HWND hDlg, i32 nID, i32 curSel) {
    if (hDlg == NULL) {
        return 0;
    }
    HWND cb = GetDlgItem(hDlg, nID);
    if (cb == NULL) {
        return 0;
    }
    ComboBox_ResetContent(cb);
    for (i32 i = 0; i < 0x11; i++) {
        ComboBox_AddString(cb, (GetColorName(i, false)).c_str());
    }
    if (curSel >= 0) {
        ComboBox_SetCurSel(cb, curSel);
    }
    return 1;
}

i32 FillDifficultyCombo(HWND hDlg, i32 nID, i32 curSel) {
    if (hDlg == NULL) {
        return 0;
    }
    HWND cb = GetDlgItem(hDlg, nID);
    if (cb == NULL) {
        return 0;
    }
    ComboBox_ResetContent(cb);
    for (i32 i = 0; i < 3; i++) {
        ComboBox_AddString(cb, (GetDifficultyName(i, false)).c_str());
    }
    if (curSel >= 0) {
        ComboBox_SetCurSel(cb, curSel);
    }
    return 1;
}

i32 GruntzPlayer::Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload) {
    char tmp[SERIAL_NAME_LEN];

    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {

            ar->Read(&m_playerIndex, sizeof(m_playerIndex));
            ar->Read(&m_color, sizeof(m_color));
            ar->Read(&m_warlordObjectId, sizeof(m_warlordObjectId));
            ar->Read(&m_difficulty, sizeof(m_difficulty));
            ar->Read(&m_humanControlled, sizeof(m_humanControlled));
            ar->Read(&m_networkPlayerId, sizeof(m_networkPlayerId));
            ar->Read(&m_ready, sizeof(m_ready));
            ar->Read(&m_active, sizeof(m_active));
            ar->Read(&m_joined, sizeof(m_joined));
            ar->Read(&m_clearedRound, sizeof(m_clearedRound));
            g_serialCounter++;
            ar->Read(tmp, SERIAL_NAME_LEN);
            m_name = tmp;
            ar->Read(&m_focusX, sizeof(m_focusX));
            ar->Read(&m_focusY, sizeof(m_focusY));
            ar->Read(&m_maxGruntz, sizeof(m_maxGruntz));
        }
    } else {

        ar->Write(&m_playerIndex, sizeof(m_playerIndex));
        ar->Write(&m_color, sizeof(m_color));
        ar->Write(&m_warlordObjectId, sizeof(m_warlordObjectId));
        ar->Write(&m_difficulty, sizeof(m_difficulty));
        ar->Write(&m_humanControlled, sizeof(m_humanControlled));
        ar->Write(&m_networkPlayerId, sizeof(m_networkPlayerId));
        ar->Write(&m_ready, sizeof(m_ready));
        ar->Write(&m_active, sizeof(m_active));
        ar->Write(&m_joined, sizeof(m_joined));
        ar->Write(&m_clearedRound, sizeof(m_clearedRound));
        g_serialCounter++;
        memset(tmp, 0, sizeof(tmp));
        strcpy(tmp, (m_name).c_str());
        ar->Write(tmp, SERIAL_NAME_LEN);
        ar->Write(&m_focusX, sizeof(m_focusX));
        ar->Write(&m_focusY, sizeof(m_focusY));
        ar->Write(&m_maxGruntz, sizeof(m_maxGruntz));
    }
    return (static_cast<CBattlezMapConfig*>(&m_battlezConfig))
               ->SerializeState(ar, mode, typeId, payload)
           != 0;
}

std::string GruntzPlayer::GetDefaultName(i32) {

    std::string name("Player");
    return name;
}

std::string GetColorName(i32 colorIdx, b32 upper) {
    std::string s;
    s = g_colorNames[colorIdx];
    if (upper) {
        std::transform((s).begin(), (s).end(), (s).begin(), asciiUpper);
    }
    return s;
}

std::string GetDifficultyName(i32 diffIdx, b32 upper) {
    std::string s;
    s = g_difficultyNames[diffIdx];
    if (upper) {
        std::transform((s).begin(), (s).end(), (s).begin(), asciiUpper);
    }
    return s;
}

void ResetPlayerColorAvailability() {
    for (i32 i = 0; i < TINT_COUNT; i++) {
        g_playerColorAvailable[i] = true;
    }
}

i32 GruntzPlayer::TrySetColor(ColorTint color) {
    if (m_color == color) {
        return 1;
    }
    if (IsPlayerColorAvailable(color)) {
        SetPlayerColorAvailable(m_color, true);
        SetPlayerColorAvailable(color, false);
        m_color = color;
        return 1;
    }
    return 0;
}

ColorTint FindAvailablePlayerColor() {
    for (i32 i = 0; i < TINT_COUNT; i++) {
        if (g_playerColorAvailable[i] != false) {
            return static_cast<ColorTint>(i);
        }
    }
    return TINT_ORANGE;
}

void SetPlayerColorAvailable(ColorTint color, b32 available) {
    g_playerColorAvailable[IDX(color)] = available;
}

i32 IsPlayerColorAvailable(ColorTint color) {
    return g_playerColorAvailable[IDX(color)];
}

i32 GruntzPlayer::Deactivate() {
    if (m_active == false) {
        return 0;
    }
    if (m_humanControlled == false) {
        (static_cast<CBattlezMapConfig*>(&m_battlezConfig))->Clear();
    }
    m_active = false;
    return 1;
}
