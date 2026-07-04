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
SIZE_UNKNOWN(CResSource);
struct CResSource {
    void* LookupSet(char* szName); // 0x13bae0 __thiscall
    void* LoadGroup(char* szName); // 0x13a230 __thiscall
};

// The bank/namespace manager at CState::m_8. Lookup resolves a named bank into a
// CResSource (the in-game loader caches the result in m_30/m_34).
SIZE_UNKNOWN(CBankMgr);
struct CBankMgr {
    CResSource* Lookup(char* szName); // 0x13c030 __thiscall
};

#endif // GRUNTZ_GRUNTZ_CBANKMGR_H
