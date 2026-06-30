#include <rva.h>
// CPlay.cpp - engine-label stubs for CPlay (reloc-correlation).

class CPlay {
public:
    void BuildWorldLevelPath(i32);
};

// The two low-RVA 5-byte ILT `jmp rel32` thunks for CPlay::RegionEnter (0xd88f0)
// and CPlay::RegionLeave (0xd8960), both reconstructed byte-exact in
// src/Gruntz/CPlay.cpp. There is no plain-C++ source form for a bare unframed
// `jmp`, so each is transcribed as a __declspec(naked) body whose single jmp's
// rel32 reloc-masks against the named target (same approach as NetThunks.cpp).
// They carry distinct names so they do not collide with the real bodies' symbols.
extern "C" {
    void n_RegionEnter_d88f0(); // CPlay::RegionEnter (0xd88f0)
    void n_RegionLeave_d8960(); // CPlay::RegionLeave (0xd8960)
}

RVA(0x000019f1, 0x5)
__declspec(naked) void Ilt_RegionLeave_19f1() {
    __asm { jmp n_RegionLeave_d8960 }
}

RVA(0x00001b9a, 0x5)
__declspec(naked) void Ilt_RegionEnter_1b9a() {
    __asm { jmp n_RegionEnter_d88f0 }
}

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
