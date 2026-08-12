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
    CTRL_PLAYER_COMBO_C0 = 0x51e,
    CTRL_PLAYER_CTRL_A0 = 0x51f,
    CTRL_PLAYER_COMBO_C1 = 0x520,
    CTRL_PLAYER_COMBO_C2 = 0x521,
    CTRL_PLAYER_COMBO_C3 = 0x522,
    CTRL_PLAYER_CTRL_A1 = 0x523,
    CTRL_PLAYER_CTRL_A2 = 0x524,
    CTRL_PLAYER_CTRL_A3 = 0x525,
    CTRL_PLAYER_LATENCY0 = 0x531,
    CTRL_PLAYER_LATENCY1 = 0x532,
    CTRL_PLAYER_LATENCY2 = 0x533,
    CTRL_PLAYER_READY0 = 0x534,
    CTRL_PLAYER_LATENCY3 = 0x535,
    CTRL_PLAYER_READY1 = 0x536,
    CTRL_PLAYER_READY2 = 0x537,
    CTRL_PLAYER_READY3 = 0x538
GZ_ENUM_CONST_END(DialogCtrlId)

typedef LRESULT(WINAPI* WapSendMessageA)(HWND, UINT, WPARAM, LPARAM);

class CLatencyList;

class CBattlezDlg : public CDialog {
public:
    CBattlezDlg(class CGruntzMgr* mgr, CWnd* pParent);

    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE;
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE;
    virtual i32 OnInitDialog() OVERRIDE;
    virtual void OnOK() OVERRIDE;

    class CGruntzMgr* m_slots;
    char m_pad60[8];
    i32 m_customNameFlag;
    CString m_worldName;

    CWnd* GetCtrlA(i32 index);
    CWnd* GetCtrlB(i32 index);
    CWnd* GetCtrlC(i32 index);
    CWnd* GetCtrlD(i32 index);

    i32 SetCurSelA(i32 id, i32 sel);
    i32 GetPlayerTypeSelection(i32 slot);
    i32 GetMaxGruntzSelection(i32 slot);
    i32 SetCurSelC(i32 id, i32 sel);

    i32 SaveOptionCombo0();
    i32 SaveOptionCombo1();
    i32 SaveOptionCombo2();
    i32 SaveOptionCombo3();

    void SetCtrlBText(i32 index, const char* text);

    i32 SetSlotValue(i32 index, ColorTint val);

    void ReadPlayerName(i32 index);

    void FlashCtrlD();

    void ShowCustomDlg();

    void ToggleRow(i32 option);
    void RefreshOptionState();

    void ApplyOption0();
    void ApplyOption1();
    void ApplyOption2();
    void ApplyOption3();

    void ApplyColorSlot0();
    void ApplyColorSlot1();
    void ApplyColorSlot2();
    void ApplyColorSlot3();

    void CopyComboSelToChild();

    void OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis);

    void OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis);

    i32 UnusedMsgHandler();

    void OnOkCommand();

    void OnPlayerNameChange0();
    void OnPlayerNameChange1();
    void OnPlayerNameChange2();
    void OnPlayerNameChange3();

    void HandlePlayerNameChange(i32 index);

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
    CBattlezDlgColors(class CGruntzMgr* mgr, i32 slotIndex, i32 networked, CWnd* pParent);

    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY _messageEntries[];

    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE;
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE;

    void OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis);
    void OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis);

    class CGruntzMgr* m_slots;
    i32 m_slotIndex;
    ColorTint m_pickedColor;
    i32 m_networked;
};

class CMultiStartDlg : public CDialog {
public:
    CMultiStartDlg(class CGruntzMgr* mgr, CWnd* pParent);

    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY _messageEntries[];

    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE;
    virtual i32 DestroyWindow() OVERRIDE;
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE;
    virtual i32 OnInitDialog() OVERRIDE;

    virtual void OnOK() OVERRIDE;

    i32 BuildSlotList();

    i32 UpdateSlot();

    i32 GetSlotIndex();

    i32 SetupWorldCombo();

    void CommitWorldHost();

    void SetListCurSel(i32 id, i32 wParam);
    void AppendChatLine(char* str);
    i32 UpdatePlayers(i32 force);
    void OnSlotSelect0();
    void OnSlotSelect1();
    void OnSlotSelect2();
    void OnSlotSelect3();
    void ToggleReady(i32 idx);

    i32 SelectColor(i32 colorIndex, ColorTint playerColor);
    void OnColorSlot0();
    void OnColorSlot1();
    void OnColorSlot2();
    void OnColorSlot3();

    void OnCustomWorld();

    void OnTimer(u32 nIDEvent);
    void OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis);
    void OnEnChange50a();
    void OnEnChange50b();
    void OnEnChange50c();
    void OnEnChange50d();
    void OnCmd51f();
    void OnCmd523();
    void OnCmd524();
    void OnCmd525();

    void OnChatSend();

    void CommitLatencyOption();

    void EchoLatencySettings();

    void Drive();
    i32 UpdateColorItems();

    void SyncChannelSlot(i32 ch);
    i32 EnableControls();
    void ReconcileChannel0();
    void ConnectStep();

    void ReconcileChannel2();
    void ReconcileChannel3();

    void Watchdog();

    CWnd* GetCtrlA(i32 index);
    CWnd* GetCtrlB(i32 index);
    CWnd* GetCtrlC(i32 index);
    CWnd* GetCtrlD(i32 index);
    CWnd* GetCtrlE(i32 index);
    void SetComboSelE(i32 index, i32 sel);
    i32 GetComboSelE(i32 index);
    i32 GetComboSelC(i32 id);

    void OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis);

    i32 FlashCtrlD();

    HWND GetSafe1c() {
        return this == NULL ? 0 : m_hWnd;
    }

    class CGruntzMgr* m_host;

    CLatencyList* m_slotList;
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

extern CString g_gruntNames[4];

extern "C" i32 g_watchBusy;
extern "C" i32 g_watchBlinkA;
extern "C" i32 g_watchBlinkB;

extern "C" i32 CALLBACK MultiMapComboEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern "C" i32 CALLBACK BattlezMapComboEditProc(HWND, UINT, WPARAM, LPARAM);

#endif // SRC_GRUNTZ_DIALOGS_H
