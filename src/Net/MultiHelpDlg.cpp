#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Ints.h>
#include <Net/LobbyDialogs.h>
#include <Net/NetLobby.h>
#include <Net/NetLobbyCtrlId.h>
#include <Net/NetMgr.h>
#include <Wap32/Wap32.h>

#include <stdio.h>
#include <string.h>

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000beb60, 0x1e)
CMultiHelpDlg::CMultiHelpDlg(CWnd* pParent) : CDialog(0xcb, pParent) {}

RVA_COMPGEN(0x000beb90, 0x1e, ??_GCMultiHelpDlg@@UAEPAXI@Z)
RVA_COMPGEN(0x000bebc0, 0x5, ??1CMultiHelpDlg@@UAE@XZ)

RVA(0x000bebe0, 0x3)
void CMultiHelpDlg::DoDataExchange(CDataExchange*) {}

RVA(0x000bec00, 0x6)
DATA_MESSAGE_MAP(0x001ea448, 0x001ea450)
BEGIN_MESSAGE_MAP(CMultiHelpDlg, CDialog)
END_MESSAGE_MAP()
