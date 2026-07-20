// BoundaryLowerMethodsViews.h - shared referent/owner views for the lower-half
// engine_boundary leaf methods reconstructed in BoundaryLowerMethods.cpp.
//
// RTTI cannot attribute these COMDAT-folded methods, so the owning class names are
// placeholders; only the OFFSETS + emitted code bytes are load-bearing (campaign
// doctrine). The serialize/archive object folds to the canonical CSerialArchive
// (Read @ +0x2c / Write @ +0x30).
// BLOCKED - vtable-attributed but the home needs a base-model/conflict fix a follow-up
// must do (each stays @early-stop in place):
//   C112bf0  IS CCheckpointTriggerSwitchLogic::M (vtbl slot 3, thunk 0x36fc); home
//                needs the grid-access chain (g_reg->m_world->m_level->m_5c ==
//                holder->CGameLevel->m_mainPlane, C77dc0's grid) modeled on the
//                CheckpointSwitchBuild g_statzGameReg view + 0x24556c dual-view fold.
//   C104dd0  this == an unmodeled +0x2c status-bar-sprite holder (CSBI_RectOnly's m_2c;
//                SetState calls `mov ecx,[ecx+0x2c]; call 0x3f8a`) - owner class unrecovered.
//
// ORPHAN - caller known but `this`'s CLASS genuinely unrecovered (a sub-object reached
// by an offset/return, no RTTI, no named owner); cannot home as a method of a class:
//   C10bbe0  this == [[0x24556c+0x2c]+0x2dc] - a game-registry sub-object getter
//                (LoadPickupSprites: mov eax,ds:0x64556c; mov ebp,[eax+0x2c];
//                mov ecx,[ebp+0x2dc]). NOT CGrunt (the +0x528 table lives on reg->m_2c's
//                +0x2dc member, identity unrecovered).
//   C213a0   <- CChatBoxOwner::ProcessCheatInput (virtual-base field getter; owner is
//                the class holding that vbase - unmodeled).
//   CTypeColl464 RESOLVED: it was the shared CActReg archetype (this=g_actionTable
//                @0x644610 / g_reg_644af0@0x644af0); Resolve @0x464e0 is the standalone
//                copy of CActReg::ResolveEntry (now in <Gruntz/ActReg.h>/FortressFlag.cpp).
//   C9cab0   registry Lookup (m_10 sub's Get @0x1b8008 = CMapStringToOb::Lookup - the
//                0x1b8438 one is CMapStringToPtr; mfc_class. This pair was recorded
//                INVERTED here, which is what made the registry look unidentifiable).
//   Cbd450   no direct caller (c:\gruntz.log opener init; owner unrecovered).
//   Ccef50   <- Gap_0b63f0/Gap_0c8b80 (~CLobbySlot/CPlay teardown; owner unrecovered).
//   Cdb200   <- CNetMgr::DispatchRecvMsg but this==eax (a returned net sub-object, NOT
//                CNetMgr); the returned holder's class is unrecovered.
//   Cdb2f0   no direct caller (custom-level map-file object; owner unrecovered).
//   Cdb750   <- CPlayLevelLoad::LoadByMode (this==CPlayLevelLoad, mov ecx,esi) BUT
//                CPlayLevelLoad is a 3-way-CONFLATED placeholder (LoadLevelByMode /
//                BridgeMoveSprites / PyramidBridgeSprites views disagree on +0xc's
//                type) - the conflation must be split before a clean home.
//   Cea170   <- CStatusBarMgr::LoadTabSprites but this==ebp (a sub-object, NOT
//                CStatusBarMgr); the +0x38-virtual dispatch object's class is unrecovered.
//   C1181d0 / C118260  no direct caller (bounds-grow updaters; orphans).
#ifndef GRUNTZ_BOUNDARYLOWERMETHODSVIEWS_H
#define GRUNTZ_BOUNDARYLOWERMETHODSVIEWS_H

#include <Ints.h>
#include <Wap32/Object.h>
#include <rva.h>
#include <Gruntz/SerialArchive.h>     // canonical Read@+0x2c / Write@+0x30 archive stream
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)

SIZE_UNKNOWN(Ccef50);

SIZE_UNKNOWN(Cdb2f0);

#endif // GRUNTZ_BOUNDARYLOWERMETHODSVIEWS_H
