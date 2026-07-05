// NetDlgHost.h - CNetDlgHost, the object CMultiStartDlg stores at m_host (+0x5c) and
// the multiplayer-start dialog reaches through two facets (previously two divergent
// .cpp-local views: CNetXform in NetMgrMisc.cpp + MpDlgHost in MultiStartDlgWorld.cpp;
// unified here).
//
// Disasm-settled facets (one object, two use-sites):
//   Drive (0xc40b0): `mov ecx,[this+0x5c]` (m_host) -> call 0x92e80 - a method invoked
//     DIRECTLY on m_host: the id transform (roster player id).
//   SetupWorldCombo (0xc1840): `mov eax,[this+0x5c]` (m_host); `mov ecx,[eax+0x34]` ->
//     call 0x13c030 (CSymParser::ResolvePath) - so m_host+0x34 is a CSymParser (the
//     world/name registry).
//
// IDENTITY CONFLICT (flagged, not resolved here): 0x92e80 is reconstructed by matcher-2
// as CGruntzMgr::FindOptionsSlot - which would make m_host a CGruntzMgr. But m_host+0x34
// is a CSymParser here, while the CGruntzMgr header models +0x34 as CRezSurface94
// (m_recolorSurface). Either m_host IS CGruntzMgr and that header's +0x34 is mis-modelled,
// or m_host is a distinct class that shares FindOptionsSlot's `this` shape. The full fold
// onto CGruntzMgr is DEFERRED: CGruntzMgr is matcher-2's reserved singleton domain. This
// header is the honest union of the two observed facets in the meantime.
#ifndef INCLUDE_GRUNTZ_NETDLGHOST_H
#define INCLUDE_GRUNTZ_NETDLGHOST_H

#include <Ints.h>
#include <rva.h>

class CSymParser; // +0x34 world/name registry (full def <Bute/SymParser.h>)

class CNetDlgHost {
public:
    // 0x92e80 (via ILT thunk 0x2e00): matcher-2's CGruntzMgr::FindOptionsSlot; Drive
    // uses its result as the transformed roster player id. Reloc-masked declared-only.
    i32 FindOptionsSlot(i32 id); // 0x92e80

    char m_pad0[0x34];
    CSymParser* m_registry; // +0x34  the world/name registry (ResolvePath)
};
SIZE_UNKNOWN(CNetDlgHost); // two facets pinned (+0x34 registry); full size TBD

#endif // INCLUDE_GRUNTZ_NETDLGHOST_H
