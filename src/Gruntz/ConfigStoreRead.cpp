// ConfigStoreRead.cpp - CConfigStore::GetString / GetBinary.
//
// The 0x1bf*-0x1d5* registry/COM helper module (framed profile, /O2 /Oy-): retail
// keeps ebp frame pointers with full /O2 register allocation. These two getters
// build / decode a CString temp, so they additionally carry the /GX exception
// frame (the unit adds /GX via `extra`). Companion to ConfigStore.cpp.
// docs/patterns/o2-optimizer-bailout-framed.md.
#include <rva.h>

#include <Gruntz/ConfigStore.h>

// MFC CString (the engine's statically-linked copy); only the operations these
// getters drive are modeled, all reloc-masked engine calls.
class CString {
public:
    CString();                 // 0x1b9b93  default ctor
    CString(const char* s);    // 0x1b9d4c  ctor from C-string
    CString(const CString& o); // 0x1b9ba3  copy ctor
    ~CString();                // 0x1b9cde
    char* GetBuffer(int nMinBufLength);                 // 0x1ba11c
    void ReleaseBuffer(int nNewLength);                 // 0x1ba16b
    int GetLength() const { return ((int*)m_pszData)[-2]; }
    operator const char*() const { return m_pszData; }
    char* m_pszData; // +0x00
};

extern "C" void* RezAlloc(u32 n); // 0x1b9b46 (engine operator new)
extern "C" void RezFree(void* p); // 0x1b9b82

// CConfigStore::GetString - registry mode reads the REG_SZ value into a CString
// buffer (size-query, GetBuffer, read, ReleaseBuffer); INI mode reads it via
// GetPrivateProfileString. Returns szDefault on any failure.
// @early-stop
// /GX EH-state + /Oy- regalloc residual: branch structure, the CString
// construct/copy/destroy sequence, every Reg* extern and the GetPrivateProfile
// fallback match retail; only the EH scope-table numbering + register allocation
// of the return-value/temp differ. docs/patterns/o2-optimizer-bailout-framed.md.
RVA(0x001d5029, 0x112)
CString CConfigStore::GetString(char* szSection, char* szKey, char* szDefault) {
    if (m_7c != 0) {
        HKEY hKey = OpenSubKey(szSection);
        if (hKey == 0) {
            return CString(szDefault);
        }
        CString result;
        DWORD dwType = 0;
        DWORD cbData;
        LONG rc = RegQueryValueExA(hKey, szKey, 0, &dwType, 0, &cbData);
        if (rc == 0) {
            char* pszBuf = result.GetBuffer(cbData);
            rc = RegQueryValueExA(hKey, szKey, 0, &dwType, (LPBYTE)pszBuf, &cbData);
            result.ReleaseBuffer(-1);
        }
        RegCloseKey(hKey);
        if (rc == 0) {
            return result;
        }
        return CString(szDefault);
    }

    if (szDefault == 0) {
        szDefault = "";
    }
    char buf[0x1000];
    GetPrivateProfileStringA(szSection, szKey, szDefault, buf, 0x1000, m_90);
    return CString(buf);
}

// CConfigStore::GetBinary - registry mode reads the REG_BINARY blob into a
// RezAlloc'd buffer; INI mode reads the GetString value and decodes the A-P
// nibble pairs WriteProfileBinary wrote (byte = (hi<<4)+lo - 0x51). Allocates
// *ppData, sets *pLen, returns whether a blob was produced.
// @early-stop
// same /GX EH-state + /Oy- regalloc residual as GetString; logic + externs match
// retail. docs/patterns/o2-optimizer-bailout-framed.md.
RVA(0x001d513b, 0x11b)
i32 CConfigStore::GetBinary(char* szSection, char* szKey, void** ppData, i32* pLen) {
    *ppData = 0;
    *pLen = 0;

    if (m_7c != 0) {
        HKEY hKey = OpenSubKey(szSection);
        if (hKey == 0) {
            return 0;
        }
        DWORD dwType;
        DWORD cbData;
        LONG rc = RegQueryValueExA(hKey, szKey, 0, &dwType, 0, &cbData);
        *pLen = cbData;
        if (rc == 0) {
            void* buf = RezAlloc(cbData);
            *ppData = buf;
            rc = RegQueryValueExA(hKey, szKey, 0, &dwType, (LPBYTE)buf, &cbData);
        }
        RegCloseKey(hKey);
        if (rc != 0) {
            RezFree(*ppData);
            *ppData = 0;
            return 0;
        }
        return 1;
    }

    CString s = GetString(szSection, szKey, 0);
    i32 len = s.GetLength();
    if (len == 0) {
        return 0;
    }
    i32 nBytes = len / 2;
    *pLen = nBytes;
    char* buf = (char*)RezAlloc(nBytes);
    *ppData = buf;
    const char* p = (const char*)s;
    for (i32 i = 0; i < len; i += 2) {
        buf[i >> 1] = (char)((p[i + 1] << 4) + p[i] - 0x51);
    }
    return 1;
}
