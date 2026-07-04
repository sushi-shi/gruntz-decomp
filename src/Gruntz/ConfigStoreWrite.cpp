// ConfigStoreWrite.cpp - CConfigStore::WriteProfileInt / WriteProfileBinary.
//
// The 0x1bf*-0x1d5* registry/COM helper module (framed profile, /O2 /Oy-): retail
// keeps ebp frame pointers with full /O2 register allocation. Companion to
// ConfigStore.cpp. docs/patterns/o2-optimizer-bailout-framed.md.
#include <rva.h>

#include <Gruntz/ConfigStore.h>

extern "C" void* RezAlloc(u32 n); // 0x1b9b46 (engine operator new)
extern "C" void RezFree(void* p); // 0x1b9b82

// CConfigStore::WriteProfileInt - registry mode stores a REG_DWORD; INI mode
// formats the int with "%d" and writes it via WritePrivateProfileString.
// @early-stop
// framed module (/O2 /Oy-) regalloc/CSE wall - logic + externs match retail; only
// the optimizer's memory-operand-vs-eax state differs (cmp [mem],0 vs mov;test,
// shared vs duplicated epilogue). Same residual as ConfigStore.cpp WriteString/
// OpenRoot. docs/patterns/o2-optimizer-bailout-framed.md. ~47%; final sweep.
RVA(0x001ccae7, 0x75)
i32 CConfigStore::WriteProfileInt(char* szSection, char* szKey, i32 nValue) {
    if (m_7c != 0) {
        HKEY hKey = OpenSubKey(szSection);
        if (hKey == 0) {
            return 0;
        }
        LONG rc = RegSetValueExA(hKey, szKey, 0, REG_DWORD, (CONST BYTE*)&nValue, 4);
        RegCloseKey(hKey);
        return rc == 0;
    }
    char buf[16];
    wsprintfA(buf, "%d", nValue);
    return WritePrivateProfileStringA(szSection, szKey, buf, m_90);
}

// CConfigStore::WriteProfileBinary - registry mode stores a REG_BINARY blob; INI
// mode hex-encodes each byte into two A-P nibble chars and forwards to WriteString.
// @early-stop
// same framed (/O2 /Oy-) regalloc/CSE wall as WriteProfileInt; logic + externs
// match retail. docs/patterns/o2-optimizer-bailout-framed.md. ~29%; final sweep.
RVA(0x001ccbfc, 0xa1)
i32 CConfigStore::WriteProfileBinary(char* szSection, char* szKey, BYTE* pData, u32 nBytes) {
    if (m_7c != 0) {
        HKEY hKey = OpenSubKey(szSection);
        if (hKey == 0) {
            return 0;
        }
        LONG rc = RegSetValueExA(hKey, szKey, 0, REG_BINARY, pData, nBytes);
        RegCloseKey(hKey);
        return rc == 0;
    }
    char* szValue = (char*)RezAlloc(nBytes * 2 + 1);
    u32 i = 0;
    char* p = szValue;
    if (nBytes != 0) {
        do {
            BYTE* pb = pData + i;
            *p = (char)((*pb & 0xf) + 0x41);
            i++;
            p[1] = (char)((*pb >> 4) + 0x41);
            p += 2;
        } while (i < nBytes);
    }
    szValue[i * 2] = '\0';
    i32 r = WriteString(szSection, szKey, szValue);
    RezFree(szValue);
    return r;
}
