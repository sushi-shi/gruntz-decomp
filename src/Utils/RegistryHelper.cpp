// RegistryHelper.cpp - Utils::RegistryHelper value-getters (config wrapper over
// ADVAPI32!Reg*). Matched leaf methods:
//   GetValueString @ RVA 0x1394a0 (151 B)
//   GetValueDword  @ RVA 0x1395d0 (80 B)
#include "RegistryHelper.h"
#include "../rva.h"
#include <string.h>

namespace Utils {

// -------------------------------------------------------------------------
// RegistryHelper::Open
// Opens/creates a 4-deep chain of nested registry subkeys (rooted at hKey,
// default top subkey "Software") via GetRegistryKey, saving szKeyName2 and
// szLastKey into the m_1c/m_11c buffers, then resolves the deepest value key
// through InitializeLastKey. Returns 1 on full success, 0 on any failure.
RVA(0x139210, 0x11c)
int RegistryHelper::Open(char *szKeyName1, char *szKeyName2, char *szKeyName3,
                         char *szLastKey, HKEY hKey, char *szSubKey)
{
    strcpy(m_1c, szKeyName2);
    if (szLastKey)
        strcpy(m_11c, szLastKey);
    else
        m_11c[0] = 0;

    m_4 = hKey;

    char szSoftware[] = "Software";

    if (GetRegistryKey(hKey, szSubKey ? szSubKey : szSoftware, &m_8)
        && GetRegistryKey(m_8, szKeyName1, &m_c)
        && GetRegistryKey(m_c, szKeyName2, &m_10)
        && GetRegistryKey(m_10, szKeyName3, &m_14)) {
        m_0 = 1;
        if (InitializeLastKey(szLastKey))
            return 1;
        m_0 = 0;
    }
    return 0;
}

// -------------------------------------------------------------------------
// RegistryHelper::InitializeLastKey
// Resolves m_18 (the deepest value key): when szLastKey is null it aliases
// m_14; otherwise it opens szLastKey under m_14 via GetRegistryKey.
RVA(0x139370, 0x37)
int RegistryHelper::InitializeLastKey(char *szLastKey)
{
    if (!m_0)
        return 0;

    if (!szLastKey) {
        m_18 = m_14;
        return 1;
    }

    return GetRegistryKey(m_14, szLastKey, &m_18) != 0;
}

// -------------------------------------------------------------------------
// RegistryHelper::GetValueString
// Reads a REG_SZ value into szValueBuffer; on any failure copies szDefault
// into the buffer (and reports its length via *pValueBufferSize), or clears
// the size and returns 0 when there is no default.
RVA(0x1394a0, 0x97)
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
// RegistryHelper::GetValueDword
// Reads a REG_DWORD value, returning valueDefault on any failure.
RVA(0x1395d0, 0x50)
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
// RegistryHelper::SetValueString
// Writes szValue as a REG_SZ value (length includes the terminator).
// Returns nonzero on success.
RVA(0x1393b0, 0x58)
int RegistryHelper::SetValueString(char *szValueName, char *szValue)
{
    if (!m_0)
        return 0;
    if (!szValueName)
        return 0;
    if (!szValue)
        return 0;
    return RegSetValueExA(m_18, szValueName, 0, 1 /*REG_SZ*/,
                          (LPBYTE)szValue, strlen(szValue) + 1) == 0;
}

// -------------------------------------------------------------------------
// RegistryHelper::SetValueDword
// Writes value as a 4-byte REG_DWORD value. Returns nonzero on success.
RVA(0x139460, 0x33)
int RegistryHelper::SetValueDword(char *szValueName, DWORD value)
{
    if (!m_0)
        return 0;
    if (!szValueName)
        return 0;
    return RegSetValueExA(m_18, szValueName, 0, 4 /*REG_DWORD*/,
                          (LPBYTE)&value, 4) == 0;
}

// -------------------------------------------------------------------------
// RegistryHelper::Close
// Closes the whole chain of opened keys (skipping the duplicate close when
// the last two keys are the same handle).
RVA(0x139330, 0x3d)
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

static char s_lpClass[1];

// -------------------------------------------------------------------------
// RegistryHelper::GetRegistryKey  (static __stdcall)
// Creates/opens szSubKey under hKey with KEY_ALL_ACCESS; returns success.
// (lpClass is an empty mutable global string in the original .data.)
RVA(0x139650, 0x32)
int RegistryHelper::GetRegistryKey(HKEY hKey, char *szSubKey, PHKEY phKeyResult)
{
    DWORD dwDisposition;
    return RegCreateKeyExA(hKey, szSubKey, 0, s_lpClass, 0, 0xf003f /*KEY_ALL_ACCESS*/,
                           0, phKeyResult, &dwDisposition) == 0;
}

} // namespace Utils
