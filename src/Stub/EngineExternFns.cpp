#include <rva.h>
// EngineExternFns.cpp - non-member engine externs (free + C-linkage), reloc-correlation. See All.cpp.

// ---------------------------------------------------------------------------
// Incremental-link jump islands (ILT thunks): each is a 5-byte `jmp rel32` to
// the real function body (reconstructed in its own TU). Modeled as a naked __asm
// jmp so the e9+rel32 is literal and the rel32 is reloc-masked; declaring each
// target with its exact retail signature lets the jmp reloc resolve to the
// retail symbol. (Same sanctioned approach as Wap32/M3CompilerThunks.cpp.)
// ---------------------------------------------------------------------------
void Cmd_ApplyScrollParams_0ec1c0(i32, i32, i32, i32, i32); // 0x0ec1c0 (src/Gruntz/CmdScrollApply.cpp)
struct HWND__;
i32 StartUpPrompt(HWND__* hWnd); // 0x01f9b0 (src/Gruntz/StartUpPrompt.cpp)
namespace Utils {
    namespace WinAPI {
        void Stub_118ce0(); // 0x118ce0 (src/Utils/WinAPI.cpp)
    }
} // namespace Utils
using Utils::WinAPI::Stub_118ce0;

// RezAssertFail referents: a CRangeSet view of the debug-channel set at g_6bf850
// (m5_DebugPrintf types the same object as CDebugSink; Contains is external - only
// its mangled name is load-bearing) and the inline-vsprintf'd sink.
struct CRangeSet {
    bool Contains(u32 value); // 0x184ba0 (src/Gruntz/CRangeSet.cpp, external)
};
struct CDebugSink {
    i32 m_0;
};
extern CDebugSink g_6bf850; // 0x6bf850 (DATA-bound in m5_DebugPrintf.cpp)
extern i32 g_6bf8dc;        // debug-output mode (DATA-bound in Globals.cpp)

// C-linkage engine functions (reloc-correlation).
extern "C" {
    int vsprintf(char* buf, const char* fmt, char* va); // 0x121770 (CRT)
    void DebugSink_184df0(char* line);                  // 0x184df0 (1-byte sink)
    // CMulti::RunErrorDialog (0x0bc250) is a __thiscall member; MSVC5 inline asm
    // cannot name a Class::Method through `jmp`, so the island below references it
    // through this reloc-masked placeholder (the e9 opcode still matches byte-exact;
    // only the rel32 operand's symbol name differs).
    void RunErrorDialog_0bc250();

    // 0x2e6e - incremental-link island for CheckExePath -> Stub_118ce0 (0x118ce0).
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    RVA(0x00002e6e, 0x5)
    SYMBOL(_CheckExePath)
    __declspec(naked) i32 CheckExePath() {
        __asm {
            jmp Stub_118ce0
        }
    }
    // 0x3936 - incremental-link island for Eng_RegionCueA -> Cmd_ApplyScrollParams (0x0ec1c0).
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    RVA(0x00003936, 0x5)
    SYMBOL(_Eng_RegionCueA)
    __declspec(naked) i32 Eng_RegionCueA() {
        __asm {
            jmp Cmd_ApplyScrollParams_0ec1c0
        }
    }
    // 0x3cab - incremental-link island for MultiDispatch -> CMulti::RunErrorDialog (0x0bc250).
    // @confidence: high
    // @source: reloc-correlation (4 callers)
    RVA(0x00003cab, 0x5)
    SYMBOL(_MultiDispatch @12)
    __declspec(naked) i32 __stdcall MultiDispatch(i32, i32, i32) {
        __asm {
            jmp RunErrorDialog_0bc250
        }
    }
    // 0x1b9b46 - MSVC5 CRT global `operator new` (new-handler retry loop around
    // malloc / _callnewh); Ghidra FID = operator_new. SKIP per game-not-CRT policy.
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x001b9b46, 0x3c)
    SYMBOL(_RezAlloc)
    void* RezAlloc(u32 size) {
        return 0;
    }
    // 0x1b9b82 - RezFree (resource free wrapper) graduated to src/Rez/RezUtil.cpp:
    // its retail TU is /O1 (push [mem] arg-forward + pop-ecx cleanup), so it cannot
    // match inside this /O2 aggregate.

    // 0x184e00 - debug-gated assert/printf: when the debug mode (g_6bf8dc) is armed
    // and channel 0 is enabled in the debug-channel set (g_6bf850, viewed as a
    // CRangeSet), vsprintf the varargs into a 256-byte stack buffer and hand it to
    // the sink (0x184df0). Body byte-identical (the vsprintf / sink call relocs are
    // reloc-masked). va_list is spelled `(char*)(&fmt + 1)` to avoid <stdarg.h>.
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    RVA(0x00184e00, 0x55)
    SYMBOL(_RezAssertFail)
    void RezAssertFail(char* fmt, ...) {
        char buf[256];
        if (g_6bf8dc != 1 && g_6bf8dc != 0 && !((CRangeSet*)&g_6bf850)->Contains(0)) {
            vsprintf(buf, fmt, (char*)(&fmt + 1));
            DebugSink_184df0(buf);
        }
    }
    // 0x11fdf0 - MSVC5 CRT `_strcmpi`/`_stricmp` (locale-locked case-insensitive
    // compare); Ghidra FID = __strcmpi. SKIP per game-not-CRT policy.
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x0011fdf0, 0xd0)
    SYMBOL(_RezStricmp)
    i32 RezStricmp(const char*, const char*) {
        return 0;
    }
    // 0x120680 - MSVC5 CRT `strrchr` (repne-scasb intrinsic form); Ghidra FID =
    // _strrchr. SKIP per game-not-CRT policy.
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x00120680, 0x27)
    SYMBOL(_RezStrrchr)
    char* RezStrrchr(const char*, i32) {
        return 0;
    }
    // 0x2f59 - incremental-link island for StartupGate -> StartUpPrompt (0x01f9b0).
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    RVA(0x00002f59, 0x5)
    SYMBOL(_StartupGate)
    __declspec(naked) i32 StartupGate() {
        __asm {
            jmp StartUpPrompt
        }
    }
    // 0x120090 - MSVC5 CRT `strstr`; Ghidra FID = strstr. SKIP per game-not-CRT
    // policy (the "SubstringMatch" name is the campaign placeholder).
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x00120090, 0x85)
    SYMBOL(_SubstringMatch)
    i32 SubstringMatch() {
        return 0;
    }
    // 0x120900 - MSVC5 CRT `sscanf` (wraps _input); Ghidra FID = sscanf. SKIP per
    // game-not-CRT policy (the "VersionScan" name is the campaign placeholder).
    // @confidence: med
    // @source: reloc-correlation (1 caller)
    // @stub
    RVA(0x00120900, 0x42)
    SYMBOL(_VersionScan)
    i32 VersionScan() {
        return 0;
    }
}

// CButeMgr_ReportError (0x1706c0) graduated to src/Bute/ButeMgr.cpp as the real
// variadic member CButeMgr::ReportError.

// 0x1437e0 - install the DDraw "restore lost surfaces" handler: store the supplied
// function pointer to g_restoreHandler (0x683edc; read back by
// RestoreLostSurfaces_1437f0). __cdecl.
extern i32 (*g_restoreHandler)(); // 0x683edc (DATA-bound in BoundaryUpper2.cpp)
RVA(0x001437e0, 0xa)
void RelayHwnd(i32 (*handler)()) {
    g_restoreHandler = handler;
}
