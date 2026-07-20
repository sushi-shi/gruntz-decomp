#include <Utils/RegistryHelper.h>
#include <EmptyString.h> // g_emptyString
#include <rva.h>
#include <string.h>

namespace Utils {

    // -------------------------------------------------------------------------
    // RegistryHelper::Open
    // Opens/creates a 4-deep chain of nested registry subkeys (rooted at hKey,
    // default top subkey "Software") via GetRegistryKey, saving szKeyName2 and
    // szLastKey into the m_keyNameBuf/m_lastKeyBuf buffers, then resolves the deepest value key
    // through InitializeLastKey. Returns 1 on full success, 0 on any failure.
    RVA(0x00139210, 0x11c)
    i32 RegistryHelper::Open(
        char* szKeyName1,
        char* szKeyName2,
        char* szKeyName3,
        char* szLastKey,
        HKEY hKey,
        char* szSubKey
    ) {
        strcpy(m_keyNameBuf, szKeyName2);
        if (szLastKey) {
            strcpy(m_lastKeyBuf, szLastKey);
        } else {
            m_lastKeyBuf[0] = 0;
        }

        m_baseKey = hKey;

        char szSoftware[] = "Software";

        if (GetRegistryKey(hKey, szSubKey ? szSubKey : szSoftware, &m_key1)
            && GetRegistryKey(m_key1, szKeyName1, &m_key2)
            && GetRegistryKey(m_key2, szKeyName2, &m_key3)
            && GetRegistryKey(m_key3, szKeyName3, &m_key4)) {
            m_open = 1;
            if (InitializeLastKey(szLastKey)) {
                return 1;
            }
            m_open = 0;
        }
        return 0;
    }

    // -------------------------------------------------------------------------
    // RegistryHelper::InitializeLastKey
    // Resolves m_valueKey (the deepest value key): when szLastKey is null it aliases
    // m_key4; otherwise it opens szLastKey under m_key4 via GetRegistryKey.
    RVA(0x00139370, 0x37)
    i32 RegistryHelper::InitializeLastKey(char* szLastKey) {
        if (!m_open) {
            return 0;
        }

        if (!szLastKey) {
            m_valueKey = m_key4;
            return 1;
        }

        return GetRegistryKey(m_key4, szLastKey, &m_valueKey) != 0;
    }

    // -------------------------------------------------------------------------
    // RegistryHelper::GetValueString
    // Reads a REG_SZ value into szValueBuffer; on any failure copies szDefault
    // into the buffer (and reports its length via *pValueBufferSize), or clears
    // the size and returns 0 when there is no default.
    RVA(0x001394a0, 0x97)
    char* RegistryHelper::GetValueString(
        char* szValueName,
        char* szValueBuffer,
        u32* pValueBufferSize,
        char* szDefault
    ) {
        DWORD dwType;

        if (m_open && szValueName && szValueBuffer && *pValueBufferSize > 0) {
            if (RegQueryValueExA(
                    m_valueKey,
                    szValueName,
                    0,
                    &dwType,
                    reinterpret_cast<LPBYTE>(szValueBuffer),
                    reinterpret_cast<LPDWORD>(pValueBufferSize)
                ) == 0
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
    // RegistryHelper::GetValueBinary
    // Reads a REG_BINARY value into pBuffer (with *pBufferSize the in/out byte
    // count); on any failure copies defaultSize bytes from pDefault (reporting the
    // count via *pBufferSize), or clears the size and returns 0 when there is no
    // default.
    RVA(0x00139540, 0x8a)
    void* RegistryHelper::GetValueBinary(
        char* szValueName,
        void* pBuffer,
        u32* pBufferSize,
        void* pDefault,
        u32 defaultSize
    ) {
        DWORD dwType;

        if (m_open && szValueName && pBuffer && *pBufferSize > 0) {
            if (RegQueryValueExA(
                    m_valueKey,
                    szValueName,
                    0,
                    &dwType,
                    static_cast<LPBYTE>(pBuffer),
                    reinterpret_cast<LPDWORD>(pBufferSize)
                ) == 0
                && dwType == 3 /*REG_BINARY*/) {
                return pBuffer;
            }
        }

        if (pDefault && defaultSize > 0) {
            memcpy(pBuffer, pDefault, defaultSize);
            *pBufferSize = defaultSize;
            return pBuffer;
        }

        *pBufferSize = 0;
        return 0;
    }

    // -------------------------------------------------------------------------
    // RegistryHelper::GetValueDword
    // Reads a REG_DWORD value, returning valueDefault on any failure.
    RVA(0x001395d0, 0x50)
    DWORD RegistryHelper::GetValueDword(char* szValueName, DWORD valueDefault) {
        DWORD dwType;
        DWORD dwData;
        DWORD cbData;

        if (m_open && szValueName) {
            cbData = 4;
            if (RegQueryValueExA(m_valueKey, szValueName, 0, &dwType, reinterpret_cast<LPBYTE>(&dwData), &cbData) == 0
                && dwType == 4 /*REG_DWORD*/) {
                return dwData;
            }
        }

        return valueDefault;
    }

    // -------------------------------------------------------------------------
    // RegistryHelper::DeleteValue (0x139620) - RegDeleteValueA on the deepest key.
    // Guards the open gate + a null name; returns success (LONG == ERROR_SUCCESS).
    // Orphan copy (inlined at all call sites).
    RVA(0x00139620, 0x28)
    i32 RegistryHelper::DeleteValue(char* szValueName) {
        if (!m_open) {
            return 0;
        }
        if (!szValueName) {
            return 0;
        }
        return RegDeleteValueA(m_valueKey, szValueName) == 0;
    }

    // -------------------------------------------------------------------------
    // RegistryHelper::SetValueString
    // Writes szValue as a REG_SZ value (length includes the terminator).
    // Returns nonzero on success.
    RVA(0x001393b0, 0x58)
    i32 RegistryHelper::SetValueString(const char* szValueName, const char* szValue) {
        if (!m_open) {
            return 0;
        }
        if (!szValueName) {
            return 0;
        }
        if (!szValue) {
            return 0;
        }
        return RegSetValueExA(
                   m_valueKey,
                   szValueName,
                   0,
                   1 /*REG_SZ*/,
                   reinterpret_cast<LPBYTE>(const_cast<char*>(szValue)),
                   strlen(szValue) + 1
               )
               == 0;
    }

    // -------------------------------------------------------------------------
    // RegistryHelper::SetValueBinary
    // Writes dataSize bytes of pData as a REG_BINARY value. Returns nonzero on success.
    RVA(0x00139410, 0x45)
    i32 RegistryHelper::SetValueBinary(char* szValueName, void* pData, u32 dataSize) {
        if (!m_open) {
            return 0;
        }
        if (!szValueName) {
            return 0;
        }
        if (!pData) {
            return 0;
        }
        return RegSetValueExA(m_valueKey, szValueName, 0, 3 /*REG_BINARY*/, static_cast<LPBYTE>(pData), dataSize)
               == 0;
    }

    // -------------------------------------------------------------------------
    // RegistryHelper::SetValueDword
    // Writes value as a 4-byte REG_DWORD value. Returns nonzero on success.
    RVA(0x00139460, 0x33)
    i32 RegistryHelper::SetValueDword(char* szValueName, DWORD value) {
        if (!m_open) {
            return 0;
        }
        if (!szValueName) {
            return 0;
        }
        return RegSetValueExA(m_valueKey, szValueName, 0, 4 /*REG_DWORD*/, reinterpret_cast<LPBYTE>(&value), 4) == 0;
    }

    // -------------------------------------------------------------------------
    // RegistryHelper::Close
    // Closes the whole chain of opened keys (skipping the duplicate close when
    // the last two keys are the same handle).
    RVA(0x00139330, 0x3d)
    void RegistryHelper::Close() {
        if (m_open) {
            m_open = 0;
            RegCloseKey(m_key1);
            RegCloseKey(m_key2);
            RegCloseKey(m_key3);
            if (m_key4 != m_valueKey) {
                RegCloseKey(m_key4);
            }
            RegCloseKey(m_valueKey);
        }
    }

    // lpClass is the shared empty mutable global string in .data (0x6293f4); bound
    // to the canonical extern "C" _g_emptyString (the tree-wide keep-last winner).

    // -------------------------------------------------------------------------
    // RegistryHelper::GetRegistryKey  (static __stdcall)
    // Creates/opens szSubKey under hKey with KEY_ALL_ACCESS; returns success.
    RVA(0x00139650, 0x32)
    i32 RegistryHelper::GetRegistryKey(HKEY hKey, char* szSubKey, PHKEY phKeyResult) {
        DWORD dwDisposition;
        return RegCreateKeyExA(
                   hKey,
                   szSubKey,
                   0,
                   g_emptyString,
                   0,
                   0xf003f /*KEY_ALL_ACCESS*/,
                   0,
                   phKeyResult,
                   &dwDisposition
               )
               == 0;
    }

} // namespace Utils
