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

#include "ImplementedLabels.cpp"
#include "Backlog.cpp"
#include "ApiCallers.cpp"
// CButeTree defines CButeTree + g_buteTree, shared by the game-object ctors that
// remain here (and by GameObjectCtors.cpp / UserLogic.cpp) -> include early.
#include "CButeTree.cpp"
#include "CActionArea.cpp"
#include "CBoomerang.cpp"
// reloc-correlation extern stubs (split from GenExterns):
#include "CButeMgrHelper.cpp"
#include "CButeNodeBase.cpp"
#include "CFileImageDecode.cpp"
// CGMMenuUI.cpp removed: it was a duplicate candidate body set (carrying NO RVA
// bindings, so it diffed nothing) of the CChatBox menu-drive methods
// (0x182c70..0x183150) already bound + 100% byte-exact in src/Gruntz/ChatBox.cpp.
// CGMMenuUI == CChatBox (same RVAs).
#include "CGameModeBase.cpp"
#include "CGruntzCmdList.cpp"
#include "CNetMgr.cpp"
#include "CPlay.cpp"
#include "CRegSink.cpp"
#include "CStatusBarMgr.cpp"
#include "EngineExternFns.cpp"
#include "RezMgr.cpp"
#include "CCoveredPowerup.cpp"
#include "CCursorSnapSprite.cpp"
#include "CDoNothing.cpp"
#include "CDroppedObjectShadow.cpp"
#include "CExplosion.cpp"
#include "CEyeCandyAni.cpp"
#include "CFortressFlag.cpp"
#include "CGameMgrDerived.cpp"
#include "CGiantRock.cpp"
#include "CGruntCreationPoint.cpp"
#include "CGruntPuddle.cpp"
#include "CGruntStartingPoint.cpp"
#include "CGruntVoice.cpp"
#include "CGruntzCommand.cpp"
#include "CGruntzMgr.cpp"
#include "CGuardPoint.cpp"
#include "CInGameIcon.cpp"
#include "CInGameText.cpp"
#include "CKitchenSlime.cpp"
#include "CLevelTime.cpp"
#include "CLightFx.cpp"
#include "CMovingLogic.cpp"
#include "CObjectDropper.cpp"
#include "CPathHazard.cpp"
#include "CProjectile.cpp"
#include "CRollingBall.cpp"
#include "CSBI_Image.cpp"
#include "CSBI_SideTab.cpp"
#include "CSplashState.cpp"
#include "CSpotLight.cpp"
#include "CStatusBarSprite.cpp"
#include "CTileSecretTrigger.cpp"
#include "CToyPeek.cpp"
#include "CUserLogic.cpp"
#include "CWayPoint.cpp"
#include "DirectSoundMgr.cpp"
#include "RezSync.cpp"
#include "MallocConstructors.cpp"
