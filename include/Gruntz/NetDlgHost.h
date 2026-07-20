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
