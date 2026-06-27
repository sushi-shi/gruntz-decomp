#include <rva.h>
// CPlay.cpp - engine-label stubs for CPlay (reloc-correlation).

class CPlay {
public:
    void RegionEnter();
    void RegionLeave();
    void BuildWorldLevelPath(i32);
};
// @confidence: high
// @source: reloc-correlation (4 callers)
// @stub
RVA(0x00001b9a, 0x5)
void CPlay::RegionEnter() {}
// @confidence: high
// @source: reloc-correlation (4 callers)
// @stub
RVA(0x000019f1, 0x5)
void CPlay::RegionLeave() {}

// Proximity-attributed (HIGH, both-sides RVA bracket) - re-homed from
// src/Stub/ApiCallers.cpp (was ThisStubOwnerUnknown).
// @confidence: med
// @source: decomp-xref
// 0xd1710 (CPlay::LoadSBITextEdges) reconstructed in src/Gruntz/CPlay.cpp.
// @confidence: med
// @source: string-xref
// @stub
RVA(0x000dbc80, 0x309)
void CPlay::BuildWorldLevelPath(i32) {}
// 0xdd050 (CPlay::BuildGruntNamespaceList) reconstructed in src/Gruntz/CPlay.cpp.
