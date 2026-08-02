#ifndef SRC_GRUNTZ_DIALOGS_H
#define SRC_GRUNTZ_DIALOGS_H

#include <Wap32/Object.h>
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <rva.h>
#include <Ints.h>
#include <Net/NetLobby.h>
#include <Gruntz/String.h>
#include <Gruntz/ObList.h>

class CString;
struct HWND__;
struct tagMEASUREITEMSTRUCT;
struct tagDRAWITEMSTRUCT;

extern "C" HWND g_sharedFlag;
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

    i32 SetSlotValue(i32 index, i32 val);

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
SIZE_UNKNOWN();

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
SIZE_UNKNOWN();

class CBattlezDlgColors : public CDialog {
public:
    CBattlezDlgColors(class CGruntzMgr* mgr, i32 slotIndex, i32 networked, CWnd* pParent);

    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE;
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE;

    void OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis);

    class CGruntzMgr* m_slots;
    i32 m_slotIndex;
    i32 m_pickedColor;
    i32 m_networked;
};
SIZE_UNKNOWN();

class CMultiStartDlg : public CDialog {
public:
    CMultiStartDlg(class CGruntzMgr* mgr, CWnd* pParent);

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

    i32 SelectColor(i32 colorIndex, i32 playerId);
    void OnColorSlot0();
    void OnColorSlot1();
    void OnColorSlot2();
    void OnColorSlot3();

    void OnCustomWorld();

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
        return this == 0 ? 0 : m_hWnd;
    }

    class CGruntzMgr* m_host;

    CLatencyList* m_slotList;
    char m_pad64[8];
    i32 m_customWorldFlag;
    CString m_worldName;
    CStringList m_74;
};
SIZE_UNKNOWN();

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
SIZE_UNKNOWN();

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
SIZE_UNKNOWN();
VTBL(CMultiHelpDlg, 0x001ea474);

extern CString g_gruntNames[4];

extern "C" i32 g_watchBusy;
extern "C" i32 g_watchBlinkA;
extern "C" i32 g_watchBlinkB;

extern "C" i32 CALLBACK MultiMapComboEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern "C" i32 CALLBACK BattlezMapComboEditProc(HWND, UINT, WPARAM, LPARAM);

extern const i32 g_msgmap_CBattlezDlgColors;
extern const i32 g_msgmap_CMultiStartDlg;
#endif // SRC_GRUNTZ_DIALOGS_H
