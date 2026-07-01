// All.cpp - aggregate TU for the src/Stub/ @stub backlog.
//
// These are LABELED-BUT-UNMATCHED functions (empty `{}` bodies, each with a real
// RVA()+size). This file #includes the per-class stubs, so their RVA()
// annotations surface in All.cpp's LLVM IR; labels.py reads them like any unit ->
// build/gen/symbol_names.csv -> the delinker carves the retail bytes at each RVA
// -> objdiff diffs them. So the `engine_label_stubs` objdiff unit lists all of
// them (initially ~0%), they ARE binary-verified (delinked against GRUNTZ.EXE),
// they count in the started-units denominator like any unmatched function, and
// they are covered by the labels.py duplicate-RVA guard + verify_stub_labels.py's
// stub-vs-matched cross-check.
//
// This unit IS the matching worklist for the backlog: pick a stub off it in
// objdiff and reconstruct it. The plan is to MOVE each stub into its real class's
// TU (as already done for CGrunt -> Grunt, CBootyState/CCreditsState -> GameMode,
// CGameMgr -> GameApp, CMultiStartDlg -> Dialogs); src/Stub/ shrinks toward empty
// as classes are reconstructed.

#include <rva.h> // RVA()/RVAU() label macros the included stub files use

#include "Backlog.cpp"
#include "ApiCallers.cpp"
// CButeTree defines CButeTree + g_buteTree, shared by the game-object ctors that
// remain here (and by GameObjectCtors.cpp / UserLogic.cpp) -> include early.
#include "CButeTree.cpp"
// reloc-correlation extern stubs (split from GenExterns):
// CButeMgrHelper.cpp re-homed: FuncA/FuncB moved into src/Bute/ButeMgr.cpp (the
// CButeMgrHelper class TU; ClearHelper already calls them there).
// CButeNodeBase.cpp re-homed: CButeNodeEntry/CButeNodeBase ctors moved into the
// dedicated src/Bute/ButeNode.cpp unit (self-contained model; own unit avoids the
// ButeMgr.h minimal-CButeNodeBase ODR conflict).
// CGMMenuUI.cpp removed: it was a duplicate candidate body set (carrying NO RVA
// bindings, so it diffed nothing) of the CChatBox menu-drive methods
// (0x182c70..0x183150) already bound + 100% byte-exact in src/Gruntz/ChatBox.cpp.
// CGMMenuUI == CChatBox (same RVAs).
#include "CGameModeBase.cpp"
#include "CGruntzCmdList.cpp"
// CStatusBarMgr.cpp re-homed: GetItem moved into the dedicated src/Gruntz/
// CStatusBarMgrGetItem.cpp unit (self-contained view; own unit avoids the
// LoadTabSprites field-view conflict).
#include "EngineExternFns.cpp"
#include "CSBI_Image.cpp"
#include "RezSync.cpp"
#include "MallocConstructors.cpp"
