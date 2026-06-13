#ifndef GAME_DIALOGS_H
#define GAME_DIALOGS_H

/*
 * Game dialogs (MFC CDialog subclasses) — Battlez (multiplayer match) setup and
 * its color/custom sub-dialogs, multiplayer start/help, and the checkpoint prompt.
 *
 * Provenance: ALL names from RTTI (mangled names kept). Name-only / @todo.
 * These derive from MFC CDialog (see ../mfc_runtime.h). Corroborating:
 * "Battlez Setup", "Gruntz Network Configuration Help", checkpoint prompts
 * (STRINGS_ANALYSIS.md §10).
 *
 * (The advanced-options dialog tomalla reconstructed is not in RTTI here — it may
 * be a non-polymorphic dialog or named differently; not represented as a class.)
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>

class CBattlezDlg       { /* .?AVCBattlezDlg@@ */ };        // Battlez (MP) setup
class CBattlezDlgColors { /* .?AVCBattlezDlgColors@@ */ };  // color picker sub-dialog
class CBattlezDlgCustom { /* .?AVCBattlezDlgCustom@@ */ };  // custom-level sub-dialog
class CMultiStartDlg    { /* .?AVCMultiStartDlg@@ */ };     // MP start
class CMultiHelpDlg     { /* .?AVCMultiHelpDlg@@ */ };      // MP / network help
class CCheckpointDlg    { /* .?AVCCheckpointDlg@@ */ };     // checkpoint prompt

#endif /* GAME_DIALOGS_H */
