#include <rva.h>
// EngineExternFns.cpp - non-member engine externs (free + C-linkage), reloc-correlation. See All.cpp.

// C-linkage engine functions (reloc-correlation).
extern "C" {
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x002e6e, 0x5)
    SYMBOL(_CheckExePath)
    int CheckExePath() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x003936, 0x5)
    SYMBOL(_Eng_RegionCueA)
    int Eng_RegionCueA() {
        return 0;
    }
    // @confidence: high
    // @source: reloc-correlation (4 callers)
    // @stub
    RVA(0x003cab, 0x5)
    SYMBOL(_MultiDispatch @12)
    int __stdcall MultiDispatch(int, int, int) {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x1b9b46, 0x3c)
    SYMBOL(_RezAlloc)
    int RezAlloc() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x184e00, 0x55)
    SYMBOL(_RezAssertFail)
    int RezAssertFail() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x11fdf0, 0xd0)
    SYMBOL(_RezStricmp)
    int RezStricmp() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x120680, 0x27)
    SYMBOL(_RezStrrchr)
    int RezStrrchr() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x002f59, 0x5)
    SYMBOL(_StartupGate)
    int StartupGate() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x120090, 0x85)
    SYMBOL(_SubstringMatch)
    int SubstringMatch() {
        return 0;
    }
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x120900, 0x42)
    SYMBOL(_VersionScan)
    int VersionScan() {
        return 0;
    }
}

// @confidence: high
// @source: reloc-correlation (11 callers)
// @stub
RVA(0x1706c0, 0x4b)
int CButeMgr_ReportError(class CButeMgr*, char const*, ...) {
    return 0;
}

// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x1437e0, 0xa)
void RelayHwnd(void*) {}
