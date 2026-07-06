// BankMgr.h - CState's bank/source facet (afx-free). The +0x08 bank manager
// (CState::m_8) resolves a named bank/namespace into a CResSource; the resolved
// source (CState::m_2c/m_28/m_30/m_34) then looks up / loads named sets.
//
// One shared shape for both the in-game CPlay loaders (LoadImageBanks: m_8->Lookup
// "GRUNTZ"/"GAME") and the front-end state loaders (CSplashState/CHelpState:
// m_8->Lookup "STATEZ_*", then m_2c->LoadGroup a "SOUNDZ" set). The front-end call
// sites reach these SAME engine methods via the VA form of the RVAs below (VA ==
// RVA + 0x400000 image base: 0x53c030==0x13c030, 0x53a230==0x13a230). All methods
// are external / no-body so their `call rel32` reloc-masks; only OFFSETS + code
// bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CBANKMGR_H
#define GRUNTZ_GRUNTZ_CBANKMGR_H

#include <Ints.h>
#include <rva.h> // SIZE_UNKNOWN class-metadata macro

// A resolved asset source (CState::m_2c/m_28/m_30/m_34). LookupSet resolves a named
// set ("TILEZ"/"IMAGEZ"/"SOUNDZ"/"ANIZ"); LoadGroup loads a named sub-group/set.
//
// IDENTITY (matcher-2, verified via sema): CResSource is the bank-source facet of the
// ButeMgr symbol table CSymTab (<Bute/SymTab.h>) - its two entries are that class's
// methods: LookupSet == CSymTab::ResolvePath (0x13bae0), LoadGroup == CSymTab::FindSub
// (0x13a230). The state activators reach the SAME +0x2c/+0x28/+0x30 source through this
// facet (LookupSet) - so folding a state's CSymTab-typed asset-source shadow (e.g.
// StateImages' bootySymTab/gruntzSymTab) onto CState's CResSource* fields is byte-neutral.
// Kept as this 2-method facet rather than retyped to CSymTab because the excluded Play.cpp
// reaches m_levelBank/m_gameBank by the LookupSet name (full CResSource<->CSymTab merge
// deferred; see the reports).
SIZE_UNKNOWN(CResSource);
struct CResSource {};

// The bank/namespace manager at CState::m_8. Lookup resolves a named bank into a
// CResSource (the in-game loader caches the result in m_30/m_34).
SIZE_UNKNOWN(CBankMgr);
struct CBankMgr {};

#endif // GRUNTZ_GRUNTZ_CBANKMGR_H
