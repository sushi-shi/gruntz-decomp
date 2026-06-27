#include <rva.h>
// CPlay.cpp - engine-label stubs for CPlay (reloc-correlation).

class CPlay {
public:
    void RegionEnter();
    void RegionLeave();
    void LoadSBITextEdges(i32);
    void BuildWorldLevelPath(i32);
    void BuildGruntNamespaceList(i32);
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
// @stub
RVA(0x000d1710, 0x122)
void CPlay::LoadSBITextEdges(i32) {}
// @confidence: med
// @source: string-xref
// @stub
RVA(0x000dbc80, 0x309)
void CPlay::BuildWorldLevelPath(i32) {}
// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x000dd050, 0x24b)
void CPlay::BuildGruntNamespaceList(i32) {}
