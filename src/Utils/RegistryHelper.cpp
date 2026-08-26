#include <rva.h>

#include <Utils/RegistryHelper.h>

#include <MsgParam.h>

#include <string.h>

namespace Utils {

    RVA(0x00139210, 0x11c)
    i32 RegistryHelper::Open(
        char* vendorName,
        char* productName,
        char* versionName,
        char* valueSubkeyName,
        HKEY rootKey,
        char* softwareSubkeyName
    ) {
        strcpy(m_productName, productName);
        if (valueSubkeyName) {
            strcpy(m_valueSubkeyName, valueSubkeyName);
        } else {
            m_valueSubkeyName[0] = 0;
        }

        m_baseKey = rootKey;

        char defaultSoftwareSubkey[] = DATA_COMPGEN(0x0021a064, "Software");

        if (GetRegistryKey(
                rootKey,
                softwareSubkeyName ? softwareSubkeyName : defaultSoftwareSubkey,
                &m_softwareKey
            )
            && GetRegistryKey(m_softwareKey, vendorName, &m_vendorKey)
            && GetRegistryKey(m_vendorKey, productName, &m_productKey)
            && GetRegistryKey(m_productKey, versionName, &m_versionKey)) {
            m_open = true;
            if (InitializeLastKey(valueSubkeyName)) {
                return 1;
            }
            m_open = false;
        }
        return 0;
    }

    RVA(0x00139330, 0x3d)
    void RegistryHelper::Close() {
        if (m_open) {
            m_open = false;
            RegCloseKey(m_softwareKey);
            RegCloseKey(m_vendorKey);
            RegCloseKey(m_productKey);
            if (m_versionKey != m_valueKey) {
                RegCloseKey(m_versionKey);
            }
            RegCloseKey(m_valueKey);
        }
    }

    RVA(0x00139370, 0x37)
    i32 RegistryHelper::InitializeLastKey(char* valueSubkeyName) {
        if (!m_open) {
            return 0;
        }

        if (!valueSubkeyName) {
            m_valueKey = m_versionKey;
            return 1;
        }

        return GetRegistryKey(m_versionKey, valueSubkeyName, &m_valueKey) != 0;
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

    // @dead-code
    // Zero-ref: retail has no caller or address-taking reference.
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
                    NULL,
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
        return NULL;
    }

    // @dead-code
    // Zero-ref: retail has no caller or address-taking reference.
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
                    NULL,
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
        return NULL;
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
                    NULL,
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

    // @dead-code
    // Zero-ref: retail has no caller or address-taking reference.
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

    RVA(0x00139650, 0x32)
    i32 RegistryHelper::GetRegistryKey(HKEY parentKey, char* subkeyName, PHKEY resultKey) {
        DWORD dwDisposition;
        return RegCreateKeyExA(
                   parentKey,
                   subkeyName,
                   0,
                   "",
                   0,
                   KEY_ALL_ACCESS,
                   NULL,
                   resultKey,
                   &dwDisposition
               )
               == 0;
    }

} // namespace Utils
