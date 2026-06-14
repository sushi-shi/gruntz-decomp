// RegistryHelper.h - Utils::RegistryHelper, the engine's registry/config wrapper
// (callers of ADVAPI32!Reg*). Minimal reconstruction sufficient to byte-match
// the leaf value-getters. Field names are placeholders (m_<hexoffset>); only the
// OFFSETS are load-bearing.
#ifndef UTILS_REGISTRYHELPER_H
#define UTILS_REGISTRYHELPER_H

// ---------------------------------------------------------------------------
// Minimal Win32 surface. We do NOT pull in <windows.h> - keep the visible
// symbol SET small (the compiler hashes it; entropy follows header churn; see
// docs/matching-patterns.md). Only the ADVAPI32 Reg* imports the matched
// methods call are declared, as a __declspec(dllimport) __stdcall block (this
// reproduces the FF15 [IAT] direct-call form).
// ---------------------------------------------------------------------------
typedef int            BOOL;
typedef unsigned long  DWORD;
typedef long           LONG;
typedef const char *   LPCSTR;
typedef char *         LPSTR;
typedef void *         HANDLE;
typedef HANDLE         HKEY;
typedef HKEY *         PHKEY;
typedef DWORD *        LPDWORD;
typedef unsigned char *LPBYTE;

struct _SECURITY_ATTRIBUTES;
typedef struct _SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;
typedef DWORD REGSAM;

extern "C" {
__declspec(dllimport) LONG __stdcall RegQueryValueExA(
    HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved,
    LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
__declspec(dllimport) LONG __stdcall RegCloseKey(HKEY hKey);
__declspec(dllimport) LONG __stdcall RegCreateKeyExA(
    HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass,
    DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult, LPDWORD lpdwDisposition);
}

namespace Utils {

// ---------------------------------------------------------------------------
// RegistryHelper - opens a chain of nested registry subkeys and reads/writes
// values from the deepest one.
//   +0x00 m_0  : open/result gate (nonzero when the chain is open).
//   +0x08 m_8  : opened key #1   } a path of nested HKEYs created/opened by
//   +0x0c m_c  : opened key #2   } Open(); Close() RegCloseKey()s them all.
//   +0x10 m_10 : opened key #3   }
//   +0x14 m_14 : opened key #4   } (Close skips m_14 when it equals m_18)
//   +0x18 m_18 : deepest key - the one GetValue*/SetValue* operate on.
// (Layout pinned by the byte-exact getters + Close.)
// ---------------------------------------------------------------------------
class RegistryHelper {
public:
    char *GetValueString(char *szValueName, char *szValueBuffer,
                         unsigned int *pValueBufferSize, char *szDefault);
    DWORD GetValueDword(char *szValueName, DWORD valueDefault);
    void  Close();
    static int __stdcall GetRegistryKey(HKEY hKey, char *szSubKey, PHKEY phKeyResult);

    int  m_0;            // +0x00  open/result gate
    char m_pad4[0x08 - 0x04];
    HKEY m_8;            // +0x08
    HKEY m_c;            // +0x0c
    HKEY m_10;           // +0x10
    HKEY m_14;           // +0x14
    HKEY m_18;           // +0x18  deepest/open registry key
};

} // namespace Utils

#endif // UTILS_REGISTRYHELPER_H
