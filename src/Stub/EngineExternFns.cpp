#include <rva.h>
// EngineExternFns.cpp - non-member engine externs (free + C-linkage), reloc-correlation. See All.cpp.

// C-linkage engine functions (reloc-correlation).
extern "C" {
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x00002e6e, 0x5)
    SYMBOL(_CheckExePath)
    int CheckExePath() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x00003936, 0x5)
    SYMBOL(_Eng_RegionCueA)
    int Eng_RegionCueA() {
        return 0;
    }
    // @confidence: high
    // @source: reloc-correlation (4 callers)
    // @stub
    RVA(0x00003cab, 0x5)
    SYMBOL(_MultiDispatch @12)
    int __stdcall MultiDispatch(int, int, int) {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x001b9b46, 0x3c)
    SYMBOL(_RezAlloc)
    void* RezAlloc(unsigned int size) {
        return 0;
    }
    // @confidence: high
    // @source: reloc-correlation (CFileImage decoders)
    // @stub
    RVA(0x001b9b82, 0xb)
    SYMBOL(_RezFree)
    int RezFree() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x00184e00, 0x55)
    SYMBOL(_RezAssertFail)
    int RezAssertFail() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x0011fdf0, 0xd0)
    SYMBOL(_RezStricmp)
    int RezStricmp() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x00120680, 0x27)
    SYMBOL(_RezStrrchr)
    int RezStrrchr() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x00002f59, 0x5)
    SYMBOL(_StartupGate)
    int StartupGate() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x00120090, 0x85)
    SYMBOL(_SubstringMatch)
    int SubstringMatch() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x00120900, 0x42)
    SYMBOL(_VersionScan)
    int VersionScan() {
        return 0;
    }
}

// CButeMgr_ReportError (0x1706c0) graduated to src/Bute/ButeMgr.cpp as the real
// variadic member CButeMgr::ReportError.

// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x001437e0, 0xa)
void RelayHwnd(void*) {}
