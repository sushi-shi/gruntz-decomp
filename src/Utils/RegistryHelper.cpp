#include <rva.h>

#include <Utils/RegistryHelper.h>

#include <EmptyString.h>
#include <MsgParam.h>

#include <string.h>

namespace Utils {

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

    RVA(0x001394a0, 0x97)
    char* RegistryHelper::GetValueString(
        char* szValueName,
        char* szValueBuffer,
        DWORD* pValueBufferSize,
        char* szDefault
    ) {
        DWORD dwType;
        RegBuf data;

        if (m_open && szValueName && szValueBuffer && *pValueBufferSize > 0) {
            if (RegQueryValueExA(
                    m_valueKey,
                    szValueName,
                    0,
                    &dwType,
                    (data.m_chars = szValueBuffer, data.m_bytes),
                    pValueBufferSize
                ) == 0
                && dwType == 1) {
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

    RVA(0x00139540, 0x8a)
    void* RegistryHelper::GetValueBinary(
        char* szValueName,
        void* pBuffer,
        DWORD* pBufferSize,
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
                    pBufferSize
                ) == 0
                && dwType == REG_BINARY) {
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

    RVA(0x001395d0, 0x50)
    DWORD RegistryHelper::GetValueDword(char* szValueName, DWORD valueDefault) {
        DWORD dwType;
        DWORD dwData;
        DWORD cbData;
        RegBuf data;

        if (m_open && szValueName) {
            cbData = 4;
            if (RegQueryValueExA(
                    m_valueKey,
                    szValueName,
                    0,
                    &dwType,
                    (data.m_dword = &dwData, data.m_bytes),
                    &cbData
                ) == 0
                && dwType == REG_DWORD) {
                return dwData;
            }
        }

        return valueDefault;
    }

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
        RegBufC data;
        return RegSetValueExA(
                   m_valueKey,
                   szValueName,
                   0,
                   1,
                   (data.m_chars = szValue, data.m_bytes),
                   strlen(szValue) + 1
               )
               == 0;
    }

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
        return RegSetValueExA(m_valueKey, szValueName, 0, 3, static_cast<LPBYTE>(pData), dataSize)
               == 0;
    }

    RVA(0x00139460, 0x33)
    i32 RegistryHelper::SetValueDword(char* szValueName, DWORD value) {
        if (!m_open) {
            return 0;
        }
        if (!szValueName) {
            return 0;
        }
        RegBuf data;
        return RegSetValueExA(
                   m_valueKey,
                   szValueName,
                   0,
                   4,
                   (data.m_dword = &value, data.m_bytes),
                   4
               )
               == 0;
    }

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

    RVA(0x00139650, 0x32)
    i32 RegistryHelper::GetRegistryKey(HKEY hKey, char* szSubKey, PHKEY phKeyResult) {
        DWORD dwDisposition;
        return RegCreateKeyExA(
                   hKey,
                   szSubKey,
                   0,
                   g_emptyString,
                   0,
                   KEY_ALL_ACCESS,
                   0,
                   phKeyResult,
                   &dwDisposition
               )
               == 0;
    }

} // namespace Utils
