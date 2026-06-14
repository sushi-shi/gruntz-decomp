// RegistryHelper.cpp - Utils::RegistryHelper value-getters (config wrapper over
// ADVAPI32!Reg*). Matched leaf methods:
//   GetValueString @ RVA 0x1394a0 (151 B)
//   GetValueDword  @ RVA 0x1395d0 (80 B)
#include "RegistryHelper.h"
#include <string.h>

namespace Utils {

// -------------------------------------------------------------------------
// RegistryHelper::GetValueString  @ RVA 0x1394a0 (151 B).
// Reads a REG_SZ value into szValueBuffer; on any failure copies szDefault
// into the buffer (and reports its length via *pValueBufferSize), or clears
// the size and returns 0 when there is no default.
// -------------------------------------------------------------------------
char *RegistryHelper::GetValueString(char *szValueName, char *szValueBuffer,
                                     unsigned int *pValueBufferSize, char *szDefault)
{
    DWORD dwType;

    if (m_0 && szValueName && szValueBuffer && *pValueBufferSize > 0) {
        if (RegQueryValueExA(m_18, szValueName, 0, &dwType,
                             (LPBYTE)szValueBuffer, (LPDWORD)pValueBufferSize) == 0
            && dwType == 1 /*REG_SZ*/) {
            return szValueBuffer;
        }
    }

    if (szDefault) {
        strcpy(szValueBuffer, szDefault);
        *pValueBufferSize = strlen(szValueBuffer);
        return szValueBuffer;
    }

    *pValueBufferSize = 0;
    return 0;
}

// -------------------------------------------------------------------------
// RegistryHelper::GetValueDword  @ RVA 0x1395d0 (80 B).
// Reads a REG_DWORD value, returning valueDefault on any failure.
// -------------------------------------------------------------------------
DWORD RegistryHelper::GetValueDword(char *szValueName, DWORD valueDefault)
{
    DWORD dwType;
    DWORD dwData;
    DWORD cbData;

    if (m_0 && szValueName) {
        cbData = 4;
        if (RegQueryValueExA(m_18, szValueName, 0, &dwType,
                             (LPBYTE)&dwData, &cbData) == 0
            && dwType == 4 /*REG_DWORD*/) {
            return dwData;
        }
    }

    return valueDefault;
}

// -------------------------------------------------------------------------
// RegistryHelper::Close  @ RVA 0x139330 (61 B).
// Closes the whole chain of opened keys (skipping the duplicate close when
// the last two keys are the same handle).
// -------------------------------------------------------------------------
void RegistryHelper::Close()
{
    if (m_0) {
        m_0 = 0;
        RegCloseKey(m_8);
        RegCloseKey(m_c);
        RegCloseKey(m_10);
        if (m_14 != m_18)
            RegCloseKey(m_14);
        RegCloseKey(m_18);
    }
}

// -------------------------------------------------------------------------
// RegistryHelper::GetRegistryKey  @ RVA 0x139650 (50 B), static __stdcall.
// Creates/opens szSubKey under hKey with KEY_ALL_ACCESS; returns success.
// (lpClass is an empty mutable global string in the original .data.)
// -------------------------------------------------------------------------
static char s_lpClass[1];

int __stdcall RegistryHelper::GetRegistryKey(HKEY hKey, char *szSubKey, PHKEY phKeyResult)
{
    DWORD dwDisposition;
    return RegCreateKeyExA(hKey, szSubKey, 0, s_lpClass, 0, 0xf003f /*KEY_ALL_ACCESS*/,
                           0, phKeyResult, &dwDisposition) == 0;
}

} // namespace Utils
