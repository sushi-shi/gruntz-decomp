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

// Backlog.cpp fully re-homed to real class TUs (semantic stub-elim); file removed.
#include "ApiCallers.cpp"
// CButeTree.cpp removed: it only re-declared the shared g_buteTree DATA binding
// (0x2bf620), already bound by TypeKeyColl.cpp (the real dyn-initializer) + the
// game-object ctor TUs; nothing left in this aggregate referenced it.
// EngineExternFns.cpp removed: its 4 ILT `jmp` islands (CheckExePath / StartupGate /
// Eng_RegionCueA / MultiDispatch) were carved to config/library_labels.csv
// (asm-carveout), and its 2 real bodies re-homed - RezAssertFail (0x184e00) ->
// src/Gruntz/DebugPrintf.cpp, RelayHwnd (0x1437e0) -> src/Gruntz/BoundaryUpper2.cpp.
// MallocConstructors.cpp removed: its 21 op-new/malloc-site ctors were all
// SEMANTICALLY re-homed (Bute CButeValue setters + zPTree node -> src/Bute/ButeNode.cpp;
// DSoundCloneCtor/CSymTab/CRez/CWwd/CAniRecord ctors -> their real class TUs; the
// MSVC iostream (LIBCIMT) ctors -> config/library_labels.csv FID carve-out).
