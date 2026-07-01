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
#include "MallocConstructors.cpp"

// ---------------------------------------------------------------------------
// Class metadata (SIZE sweep) - hosted at end of aggregate TU (Backlog/MallocConstructors views); labels.py scans tree-wide.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(BootyAssetRoot);
SIZE_UNKNOWN(BootyNamespace);
SIZE_UNKNOWN(BootyRegistrar);
SIZE_UNKNOWN(BootyRegistrarVtbl);
SIZE_UNKNOWN(BootySndEntry);
SIZE_UNKNOWN(BootySndMgr);
SIZE_UNKNOWN(BootySndPlayer);
SIZE_UNKNOWN(BootySndSet);
SIZE_UNKNOWN(BootySndTable);
SIZE_UNKNOWN(BootyState);
SIZE_UNKNOWN(Boxed16a);
SIZE_UNKNOWN(Boxed16b);
SIZE_UNKNOWN(Boxed24);
SIZE_UNKNOWN(BoxedStr);
SIZE_UNKNOWN(CActionResMgr);
SIZE_UNKNOWN(CActionResRegistry);
SIZE_UNKNOWN(CActionTileOwner);
SIZE_UNKNOWN(CGameInfo);
SIZE_UNKNOWN(CGameInfoTime);
SIZE_UNKNOWN(CGruntDataLoader);
SIZE_UNKNOWN(CGruntResurrector);
SIZE_UNKNOWN(CHelpBookSprite);
SIZE_UNKNOWN(CSaveGameMenu);
SIZE_UNKNOWN(CSoundOwner);
SIZE_UNKNOWN(CSoundResMgr);
SIZE_UNKNOWN(CSoundResRegistry);
SIZE_UNKNOWN(CSoundSetSource);
SIZE_UNKNOWN(CStatzFactoryHolder);
SIZE_UNKNOWN(CStatzRect60);
SIZE_UNKNOWN(CStatzSprite);
SIZE_UNKNOWN(CStatzSpriteFactory);
SIZE_UNKNOWN(CStatzSpriteInitVtbl);
SIZE_UNKNOWN(CStatzTabSmall);
SIZE_UNKNOWN(CTileSetSource);
SIZE_UNKNOWN(EngStr4);
SIZE_UNKNOWN(HbCellMgr);
SIZE_UNKNOWN(HbF14);
SIZE_UNKNOWN(HbFoundObj);
SIZE_UNKNOWN(HbLogic);
SIZE_UNKNOWN(HbMgr);
SIZE_UNKNOWN(HbOwner);
SIZE_UNKNOWN(HbSub1a0);
SIZE_UNKNOWN(MallocCtor_139c80);
SIZE_UNKNOWN(MallocCtor_13cac0);
SIZE_UNKNOWN(MallocCtor_168e70);
SIZE_UNKNOWN(MallocCtor_169ad0);
SIZE_UNKNOWN(MallocCtor_16b510);
SIZE_UNKNOWN(MallocCtor_16bfa0);
SIZE_UNKNOWN(MallocCtor_16c570);
SIZE_UNKNOWN(MallocCtor_184960);
SIZE_UNKNOWN(MallocPayload16);
SIZE_UNKNOWN(MallocPayload24);
SIZE_UNKNOWN(MallocStr);
SIZE_UNKNOWN(Node174d00);
SIZE_UNKNOWN(PowerupKeyRegistry);
SIZE_UNKNOWN(ResButeMgr);
SIZE_UNKNOWN(ResCfgSub38);
SIZE_UNKNOWN(ResFactoryHost);
SIZE_UNKNOWN(ResGrunt);
SIZE_UNKNOWN(ResGruntLogic);
SIZE_UNKNOWN(ResLightCfg);
SIZE_UNKNOWN(ResMgrCfgEntry);
SIZE_UNKNOWN(ResNode);
SIZE_UNKNOWN(ResSettings);
SIZE_UNKNOWN(ResSprite);
SIZE_UNKNOWN(ResSpriteCtl);
SIZE_UNKNOWN(ResSpriteFactory);
SIZE_UNKNOWN(SaveMenuMgr);
SIZE_UNKNOWN(SbiCfgEntry);
SIZE_UNKNOWN(SbiChild);
SIZE_UNKNOWN(SbiCoordSrc);
SIZE_UNKNOWN(SbiEntry);
SIZE_UNKNOWN(SbiHost);
SIZE_UNKNOWN(SbiHost24);
SIZE_UNKNOWN(SbiHostInner);
SIZE_UNKNOWN(SbiMgr);
SIZE_UNKNOWN(SbiMgr68);
SIZE_UNKNOWN(SbiPoint);
SIZE_UNKNOWN(SbiProbe);
SIZE_UNKNOWN(SbiRectHost);
SIZE_UNKNOWN(SbiRectSrc);
SIZE_UNKNOWN(SbiSndEntry);
SIZE_UNKNOWN(SbiSndSet);
SIZE_UNKNOWN(SbiSndTable);
SIZE_UNKNOWN(SbiSpawner);
SIZE_UNKNOWN(SbiToggle);
SIZE_UNKNOWN(SbiVtbl);
SIZE_UNKNOWN(StatusBarItem);
