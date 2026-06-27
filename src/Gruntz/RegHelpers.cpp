// RegHelpers.cpp - the 0x1bf*-0x1c7* registry/COM/mouse-wheel helper module.
//
// Retail compiled this module /O2 with frame pointers retained (push ebp/mov
// ebp,esp WITH full register allocation), which the frameless /O2 backlog
// aggregate could not reproduce. This TU uses the `framed` profile (/O2 /Oy-).
// See docs/patterns/o2-optimizer-bailout-framed.md.
#include <rva.h>

#include <Win32.h>

#include <stdlib.h> // strtol (0x1240b0, reloc-masked CRT)
#include <string.h> // memset (0x121380, reloc-masked CRT)

// MFC CString modeled by the two operations 1bf702 drives (reloc-masked
// __thiscall externals).
class CString {
public:
    char* GetBuffer(int nMinBufLength); // 0x1ba11c
    void ReleaseBuffer(int nNewLength); // 0x1ba16b
    char* m_pszData;                    // +0x00
};

// The empty global string used as RegQueryValueEx's value name (default value).
extern "C" char g_emptyString[]; // 0x6293f4

// ClsidToInProcServer (0x1bf702) - reads HKCR\CLSID\<szClsid>\InProcServer32's
// default value into the out CString (the COM in-proc server DLL path). Returns
// whether it succeeded; closes every opened key on the way out.
// @early-stop
// /Oy- regalloc/CSE residual (the o2-optimizer-bailout-framed wall): branch
// structure + every extern/string match retail; only the optimizer's register
// allocation of `this`/the key handles differs. docs/patterns/o2-optimizer-bailout-framed.md.
RVA(0x001bf702, 0xac)
i32 __stdcall ClsidToInProcServer(char* szClsid, CString* out) {
    HKEY hClsidRoot = 0;
    HKEY hClsid = 0;
    HKEY hServer;
    DWORD dwType;
    i32 result = 0;

    if (RegOpenKeyA((HKEY)0x80000000 /*HKCR*/, "CLSID", &hClsidRoot) == 0) {
        if (RegOpenKeyA(hClsidRoot, szClsid, &hClsid) == 0) {
            if (RegOpenKeyA(hClsid, "InProcServer32", &hServer) == 0) {
                DWORD cb = 0x104;
                char* pszBuf = out->GetBuffer(0x104);
                i32 rc = RegQueryValueExA(hServer, g_emptyString, 0, &dwType, (LPBYTE)pszBuf, &cb);
                out->ReleaseBuffer(-1);
                result = (rc == 0);
                RegCloseKey(hServer);
            }
            RegCloseKey(hClsid);
        }
        RegCloseKey(hClsidRoot);
    }
    return result;
}

// The cached IntelliMouse wheel-scroll-lines state (lazily detected once).
DATA(0x00253484)
extern i32 g_wheelInited; // 0x653484
DATA(0x00253488)
extern i32 g_wheelScrollLines; // 0x653488
DATA(0x0025348c)
extern UINT g_wheelMsg; // 0x65348c  RegisterWindowMessage("MSH_SCROLL_LINES_MSG")
DATA(0x00253490)
extern u16 g_wheelMode; // 0x653490  0=undetected, 1=no wheel msg, 2=wheel present

// GetWheelScrollLines (0x1c7cb3) - the IntelliMouse "lines per wheel notch"
// detection. Re-detects when force!=0 or not yet cached. Tries the
// MSH_SCROLL_LINES_MSG window message first; otherwise reads the desktop registry
// value (pre-NT4) or SystemParametersInfo SPI_GETWHEELSCROLLLINES (NT4+).
// @early-stop
// zero-register-pinning wall (~58%): retail pins 0 in edi and uses `cmp edi,X`
// everywhere this /O2 recompile emits `test X,X`; it also frames 0x11c vs our
// 0x118 (a +4 local-layout slot). Logic + every Win32 import + string match.
// docs/patterns/zero-register-pinning.md.
RVA(0x001c7cb3, 0x177)
i32 GetWheelScrollLines(i32 force) {
    if (force == 0 && g_wheelInited != 0) {
        return g_wheelScrollLines;
    }
    g_wheelInited = 1;

    if (g_wheelMode == 0) {
        UINT msg = RegisterWindowMessageA("MSH_SCROLL_LINES_MSG");
        g_wheelMsg = msg;
        g_wheelMode = (u16)(msg != 0 ? 2 : 1);
    }

    if (g_wheelMode == 2) {
        HWND hwnd = FindWindowA("MouseZ", "Magellan MSWHEEL");
        if (hwnd != 0 && g_wheelMsg != 0) {
            g_wheelScrollLines = (i32)SendMessageA(hwnd, g_wheelMsg, 0, 0);
            return g_wheelScrollLines;
        }
    }

    OSVERSIONINFOA osvi;
    memset(&osvi, 0, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    g_wheelScrollLines = 3;
    if (!GetVersionExA(&osvi)) {
        return g_wheelScrollLines;
    }
    if (osvi.dwPlatformId != 1 && osvi.dwPlatformId != 2) {
        return g_wheelScrollLines;
    }

    if (osvi.dwMajorVersion < 4) {
        HKEY hKey;
        if (RegOpenKeyExA((HKEY)0x80000001 /*HKCU*/, "Control Panel\\Desktop", 0, 1, &hKey) != 0) {
            return g_wheelScrollLines;
        }
        char buf[0x80];
        DWORD cb = sizeof(buf);
        if (RegQueryValueExA(hKey, "WheelScrollLines", 0, 0, (LPBYTE)buf, &cb) == 0) {
            g_wheelScrollLines = (i32)strtol(buf, 0, 10);
        }
        RegCloseKey(hKey);
        return g_wheelScrollLines;
    }

    if (osvi.dwPlatformId == 2 && osvi.dwMajorVersion >= 4) {
        SystemParametersInfoA(0x68 /*SPI_GETWHEELSCROLLLINES*/, 0, &g_wheelScrollLines, 0);
    }
    return g_wheelScrollLines;
}
