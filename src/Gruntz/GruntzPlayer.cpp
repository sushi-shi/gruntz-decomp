#include <rva.h>

#include <Gruntz/GruntzPlayer.h>

#include <Mfc.h>

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
#include <Gruntz/CurPlayer.h>
#include <Gruntz/DrawDebugStats.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/FreeNodePool.h>
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

class CImage;

#define CLEAR_TAB_HINT(sndHost)                                                                    \
    do {                                                                                           \
        SoundCueRegistry* _s = (sndHost);                                                          \
        if (_s->m_silentMode == 0) {                                                               \
            SoundCue* found = NULL;                                                                \
            MapLookup(_s->m_cues, "GAME_TABHIGHLIGHT1", found);                                    \
            if (found != NULL)                                                                     \
                found->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);                               \
        }                                                                                          \
    } while (0)

#define CLEAR_GRUNTZ_PLAYER                                                                        \
    m_playerIndex = -1;                                                                            \
    m_networkPlayerId = -2;                                                                        \
    m_active = 0;                                                                                  \
    m_humanControlled = 1;                                                                         \
    m_name = "";                                                                                   \
    m_color = TINT_ORANGE;                                                                         \
    m_difficulty = BZDIFF_EASY;                                                                    \
    m_focusX = 0;                                                                                  \
    m_focusY = 0;                                                                                  \
    m_maxGruntz = 0xf;                                                                             \
    m_doneFlag = 0;                                                                                \
    m_optionsPresenceCounted = 0;                                                                  \
    m_latency.Clear()

RVA(0x000da790, 0xb0)
GruntzPlayer::GruntzPlayer() {
    m_playerIndex = -1;
    m_networkPlayerId = -2;
    m_active = 0;
    m_joined = 0;
    m_humanControlled = 1;
    m_name = "";
    m_color = TINT_ORANGE;
    m_difficulty = BZDIFF_EASY;
    m_focusX = 0;
    m_focusY = 0;
    m_maxGruntz = 0xf;
    m_doneFlag = 0;
    m_optionsPresenceCounted = 0;
    m_latency.Clear();
}

RVA(0x000da870, 0xb8)
i32 GruntzPlayer::SeedForSlot(i32 index) {
    m_playerIndex = index;
    m_networkPlayerId = -2;
    m_active = 0;
    m_joined = 0;
    m_humanControlled = 1;
    m_name = "";

    m_color = static_cast<ColorTint>(index);
    m_difficulty = BZDIFF_EASY;
    m_focusX = 0;
    m_focusY = 0;
    m_maxGruntz = 0xf;
    m_doneFlag = 0;
    m_optionsPresenceCounted = 0;
    m_name = GetDefaultName(0);
    m_latency.Clear();
    return 1;
}

RVA(0x000da960, 0x5b)
void GruntzPlayer::Clear() {
    CLEAR_GRUNTZ_PLAYER;
}

RVA(0x000da9e0, 0x60)
i32 GruntzPlayer::Reset() {
    CLEAR_GRUNTZ_PLAYER;
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000daa60, 0x24)
i32 GruntzPlayer::ClearRoundState() {
    m_active = 1;
    m_ready = 0;
    m_doneFlag = 0;
    m_optionsPresenceCounted = 0;
    m_latency.Clear();
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000daaa0, 0xd3)
i32 FillColorCombo(HWND hDlg, i32 nID, i32 curSel) {
    if (hDlg == NULL) {
        return 0;
    }
    HWND cb = GetDlgItem(hDlg, nID);
    if (cb == NULL) {
        return 0;
    }
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = SendMessageA;
    pSend(cb, CB_RESETCONTENT, 0, 0);
    for (i32 i = 0; i < 0x11; i++) {
        pSend(
            cb,
            CB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(static_cast<const char*>(GetColorName(i, 0)))
        );
    }
    if (curSel >= 0) {
        pSend(cb, CB_SETCURSEL, curSel, 0);
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000dabc0, 0xd3)
i32 FillDifficultyCombo(HWND hDlg, i32 nID, i32 curSel) {
    if (hDlg == NULL) {
        return 0;
    }
    HWND cb = GetDlgItem(hDlg, nID);
    if (cb == NULL) {
        return 0;
    }
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = SendMessageA;
    pSend(cb, CB_RESETCONTENT, 0, 0);
    for (i32 i = 0; i < 3; i++) {
        pSend(
            cb,
            CB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(static_cast<const char*>(GetDifficultyName(i, 0)))
        );
    }
    if (curSel >= 0) {
        pSend(cb, CB_SETCURSEL, curSel, 0);
    }
    return 1;
}

RVA(0x000dace0, 0x239)
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
        strcpy(tmp, static_cast<const char*>(m_name));
        ar->Write(tmp, SERIAL_NAME_LEN);
        ar->Write(&m_focusX, sizeof(m_focusX));
        ar->Write(&m_focusY, sizeof(m_focusY));
        ar->Write(&m_maxGruntz, sizeof(m_maxGruntz));
    }
    return (static_cast<CBattlezMapConfig*>(&m_battlezConfig))
               ->SerializeState(ar, mode, typeId, payload)
           != 0;
}

RVA(0x000dafb0, 0x71)
CString GruntzPlayer::GetDefaultName(i32) {

    CString name("Player");
    return name;
}

RVA(0x000db050, 0x90)
CString GetColorName(i32 colorIdx, i32 upper) {
    CString s;
    s = g_colorNames[colorIdx];
    if (upper) {
        s.MakeUpper();
    }
    return s;
}

RVA(0x000db110, 0x90)
CString GetDifficultyName(i32 diffIdx, i32 upper) {
    CString s;
    s = g_difficultyNames[diffIdx];
    if (upper) {
        s.MakeUpper();
    }
    return s;
}

RVA(0x000db1d0, 0x14)
void ResetPlayerColorAvailability() {
    for (i32 i = 0; i < TINT_COUNT; i++) {
        g_playerColorAvailable[i] = true;
    }
}

RVA(0x000db200, 0x51)
i32 GruntzPlayer::TrySetColor(ColorTint color) {
    if (m_color == color) {
        return 1;
    }
    if (IsPlayerColorAvailable(color)) {
        SetPlayerColorAvailable(m_color, 1);
        SetPlayerColorAvailable(color, 0);
        m_color = color;
        return 1;
    }
    return 0;
}

RVA(0x000db280, 0x1b)
ColorTint FindAvailablePlayerColor() {
    for (i32 i = 0; i < TINT_COUNT; i++) {
        if (g_playerColorAvailable[i] != false) {
            return static_cast<ColorTint>(i);
        }
    }
    return TINT_ORANGE;
}

RVA(0x000db2b0, 0x10)
void SetPlayerColorAvailable(ColorTint color, i32 available) {
    g_playerColorAvailable[IDX(color)] = available;
}

RVA(0x000db2d0, 0xc)
i32 IsPlayerColorAvailable(ColorTint color) {
    return g_playerColorAvailable[IDX(color)];
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000db2f0, 0x2b)
i32 GruntzPlayer::Deactivate() {
    if (m_active == 0) {
        return 0;
    }
    if (m_humanControlled == 0) {
        (static_cast<CBattlezMapConfig*>(&m_battlezConfig))->Clear();
    }
    m_active = 0;
    return 1;
}
