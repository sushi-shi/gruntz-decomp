#ifndef SRC_GRUNTZ_DIALOGS_H
#define SRC_GRUNTZ_DIALOGS_H

#include <rva.h>

#include <MfcWin.h>

#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/ObList.h>
#include <Gruntz/PlayerSlot.h>
#include <Gruntz/String.h>
#include <Ints.h>
#include <Net/NetLobby.h>
#include <Wap32/Object.h>

class CString;
struct HWND__;
struct tagMEASUREITEMSTRUCT;
struct tagDRAWITEMSTRUCT;

// Resource control ids of the player-slot dialog template, shared by the
// Battlez setup dialog (CBattlezDlg) and the multiplayer start dialog
// (CMultiStartDlg). A constant BAG, never a variable's type - every carrier is
// an MFC `int nID` (GetDlgItem, OnDrawItem, the ON_* message-map entries).
//
GZ_ENUM_CONST_BEGIN(DialogCtrlId)
    CTRL_PLAYER_TYPE0 = 0x500,
    CTRL_PLAYER_COLOR0 = 0x501,
    CTRL_PLAYER_COLOR1 = 0x503,
    CTRL_PLAYER_COLOR2 = 0x505,
    CTRL_PLAYER_COLOR3 = 0x507,
    CTRL_PLAYER_NAME0 = 0x50a,
    CTRL_PLAYER_NAME1 = 0x50b,
    CTRL_PLAYER_NAME2 = 0x50c,
    CTRL_PLAYER_NAME3 = 0x50d,
    CTRL_PLAYER_TYPE1 = 0x50e,
    CTRL_PLAYER_TYPE2 = 0x50f,
    CTRL_PLAYER_TYPE3 = 0x510,
    CTRL_PLAYER_MAX_GRUNTZ0 = 0x51e,
    CTRL_PLAYER_READY0 = 0x51f,
    CTRL_PLAYER_MAX_GRUNTZ1 = 0x520,
    CTRL_PLAYER_MAX_GRUNTZ2 = 0x521,
    CTRL_PLAYER_MAX_GRUNTZ3 = 0x522,
    CTRL_PLAYER_READY1 = 0x523,
    CTRL_PLAYER_READY2 = 0x524,
    CTRL_PLAYER_READY3 = 0x525,
    CTRL_PLAYER_LATENCY_VALUE0 = 0x531,
    CTRL_PLAYER_LATENCY_VALUE1 = 0x532,
    CTRL_PLAYER_LATENCY_VALUE2 = 0x533,
    CTRL_PLAYER_LATENCY_UNIT0 = 0x534,
    CTRL_PLAYER_LATENCY_VALUE3 = 0x535,
    CTRL_PLAYER_LATENCY_UNIT1 = 0x536,
    CTRL_PLAYER_LATENCY_UNIT2 = 0x537,
    CTRL_PLAYER_LATENCY_UNIT3 = 0x538
GZ_ENUM_CONST_END(DialogCtrlId)

// Resource control ids of the colour-picker dialog template (CBattlezDlgColors,
// IDD 0xc2). Same constant-bag rule as DialogCtrlId - every carrier is an MFC
// `int nID`.
GZ_ENUM_CONST_BEGIN(ColorDlgCtrlId)
    CTRL_COLOR_LIST = 0x515
GZ_ENUM_CONST_END(ColorDlgCtrlId)

// WM_TIMER ids of the multiplayer start dialog. OnInitDialog arms the watchdog
// at a 50 ms period; OnTimer dispatches it to Watchdog(). Carried as MFC's
// `UINT nIDEvent`.
GZ_ENUM_CONST_BEGIN(MultiStartTimerId)
    MULTI_START_WATCHDOG_TIMER = 1
GZ_ENUM_CONST_END(MultiStartTimerId)

typedef LRESULT(WINAPI* WapSendMessageA)(HWND, UINT, WPARAM, LPARAM);

class CLatencyList;

class CBattlezDlg : public CDialog {
public:
    CBattlezDlg(class CGruntzMgr* gameManager, CWnd* pParent);

    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE;
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE;
    virtual i32 OnInitDialog() OVERRIDE;
    virtual void OnOK() OVERRIDE;

    class CGruntzMgr* m_gameManager;
    char m_pad60[8];
    i32 m_customNameFlag;
    CString m_worldName;

    CWnd* GetPlayerTypeControl(i32 slot);
    CWnd* GetPlayerNameControl(i32 slot);
    CWnd* GetMaxGruntzControl(i32 slot);
    CWnd* GetPlayerColorControl(i32 slot);

    i32 SetPlayerTypeSelection(i32 slot, i32 selection);
    i32 GetPlayerTypeSelection(i32 slot);
    i32 GetMaxGruntzSelection(i32 slot);
    i32 SetMaxGruntzSelection(i32 slot, i32 count);

    i32 OnMaxGruntzSelection0();
    i32 OnMaxGruntzSelection1();
    i32 OnMaxGruntzSelection2();
    i32 OnMaxGruntzSelection3();

    void SetPlayerName(i32 slot, const char* name);

    i32 SetPlayerColor(i32 slot, ColorTint color);

    void ReadPlayerName(i32 slot);

    void PaintPlayerColorControls();

    void ShowCustomDlg();

    void UpdatePlayerSlotEnabled(i32 slot);
    void OnPlayerOptionsChanged();

    void OnPlayerTypeSelection0();
    void OnPlayerTypeSelection1();
    void OnPlayerTypeSelection2();
    void OnPlayerTypeSelection3();

    void OnPlayerColor0();
    void OnPlayerColor1();
    void OnPlayerColor2();
    void OnPlayerColor3();

    void OnWorldSelectionChange();

    void OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis);

    void OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis);

    i32 UnusedMsgHandler();

    void OnOkCommand();

    void OnPlayerNameChange0();
    void OnPlayerNameChange1();
    void OnPlayerNameChange2();
    void OnPlayerNameChange3();

    void HandlePlayerNameChange(i32 slot);

    void OnPlayerNameKillFocus0();
    void OnPlayerNameKillFocus1();
    void OnPlayerNameKillFocus2();
    void OnPlayerNameKillFocus3();

    long OnPaint();

protected:
    static const AFX_MSGMAP messageMap;

private:
    static const AFX_MSGMAP_ENTRY _messageEntries[];
};

class CBattlezDlgCustom : public CDialog {
public:
    CBattlezDlgCustom(CWnd* pParent);

    static const AFX_MSGMAP messageMap;
    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE;
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE;

    void PickIfSelected();

    CString m_customName;

protected:
private:
    static const AFX_MSGMAP_ENTRY _messageEntries[];
};

class CBattlezDlgColors : public CDialog {
public:
    CBattlezDlgColors(class CGruntzMgr* gameManager, i32 slotIndex, i32 networked, CWnd* pParent);

    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY _messageEntries[];

    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE;
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE;

    void OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis);
    void OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis);

    class CGruntzMgr* m_gameManager;
    i32 m_slotIndex;
    ColorTint m_pickedColor;
    i32 m_networked;
};

class CMultiStartDlg : public CDialog {
public:
    CMultiStartDlg(class CGruntzMgr* gameManager, CWnd* pParent);

    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY _messageEntries[];

    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE;
    virtual i32 DestroyWindow() OVERRIDE;
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE;
    virtual i32 OnInitDialog() OVERRIDE;

    virtual void OnOK() OVERRIDE;

    i32 BuildLatencyOptions();

    i32 RefreshLatencyControl();

    i32 GetLocalPlayerSlotIndex();

    i32 InitializeWorldCombo();

    void CommitWorldSelection();

    void SetMaxGruntzSelection(i32 slot, i32 count);
    void SetPlayerName(i32 slot, const char* name);
    void AppendChatLine(char* line);
    i32 RefreshPlayerControls(i32 force);
    void OnMaxGruntzSelection0();
    void OnMaxGruntzSelection1();
    void OnMaxGruntzSelection2();
    void OnMaxGruntzSelection3();
    void CommitReadySelection(i32 slot);

    i32 SetPlayerColor(i32 slot, ColorTint color);
    void OnPlayerColor0();
    void OnPlayerColor1();
    void OnPlayerColor2();
    void OnPlayerColor3();

    void OnCustomWorld();

    void OnTimer(u32 nIDEvent);
    void OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis);
    void OnPlayerNameChange0();
    void OnPlayerNameChange1();
    void OnPlayerNameChange2();
    void OnPlayerNameChange3();
    void HandlePlayerNameChange(i32 slot);
    void OnReadyToggle0();
    void OnReadyToggle1();
    void OnReadyToggle2();
    void OnReadyToggle3();

    void OnChatSend();

    void CommitLatencySelection();

    void EchoLatencySettings();

    void BroadcastPlayerSlotChanges();
    i32 RefreshWorldControls();

    void ApplyPlayerTypeSelection(i32 slot);
    i32 EnableChatControls();
    void OnPlayerTypeSelection0();
    void OnPlayerTypeSelection1();

    void OnPlayerTypeSelection2();
    void OnPlayerTypeSelection3();

    void Watchdog();

    CWnd* GetReadyControl(i32 slot);
    CWnd* GetPlayerNameControl(i32 slot);
    CWnd* GetMaxGruntzControl(i32 slot);
    CWnd* GetPlayerColorControl(i32 slot);
    CWnd* GetPlayerTypeControl(i32 slot);
    void SetPlayerTypeSelection(i32 slot, i32 selection);
    i32 GetPlayerTypeSelection(i32 slot);
    i32 GetMaxGruntzSelection(i32 slot);

    void OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis);

    i32 PaintPlayerColorControls();

    class CGruntzMgr* m_gameManager;

    CLatencyList* m_latencyOptions;
    char m_pad64[8];
    i32 m_customWorldFlag;
    CString m_worldName;
    CStringList m_reserved74;
};

class CCheckpointDlg : public CDialog {
public:
    CCheckpointDlg(CWnd* pParent);

    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE;
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE;

    void OnToggleCheckpointPrompts();

    static const AFX_MSGMAP messageMap;

private:
    static const AFX_MSGMAP_ENTRY _messageEntries[];
};

class CMultiHelpDlg : public CDialog {
public:
    CMultiHelpDlg(CWnd* pParent);

    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE;

protected:
    static const AFX_MSGMAP messageMap;
    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE;

private:
    static const AFX_MSGMAP_ENTRY _messageEntries[];
};

extern CString g_defaultPlayerNames[4];

extern i32 g_watchdogBusy;
extern i32 g_netStatsTick;
extern i32 g_latencyDisplayTick;

LRESULT CALLBACK MultiMapComboEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

i32 CALLBACK BattlezMapComboEditProc(HWND, UINT, WPARAM, LPARAM);

#endif // SRC_GRUNTZ_DIALOGS_H
