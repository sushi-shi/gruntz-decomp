#include <StdAfx.h>

#include <rva.h>

#include <Utils/RegMgr.h>

#include <MsgParam.h>

#include <string.h>

RVA(0x00139210, 0x11c)
BOOL CRegMgr::Init(
    const char* company,
    const char* app,
    const char* version,
    const char* subKey,
    HANDLE rootKey,
    char* rootSubKey
) {
    strcpy(m_sApp, app);
    if (subKey != NULL) {
        strcpy(m_sSubKey, subKey);
    } else {
        m_sSubKey[0] = 0;
    }
    m_hRootKey = static_cast<HKEY>(rootKey);
    char software[] = DATA_COMPGEN(0x0021a064, "Software");
    char* softwareRoot;
    if (rootSubKey == NULL) {
        softwareRoot = software;
    } else {
        softwareRoot = rootSubKey;
    }
    if (CreateKey(m_hRootKey, softwareRoot, m_hSoftwareKey)) {
        if (CreateKey(m_hSoftwareKey, company, m_hCompanyKey)) {
            if (CreateKey(m_hCompanyKey, app, m_hAppKey)) {
                if (CreateKey(m_hAppKey, version, m_hVersionKey)) {
                    m_bInitialized = true;
                    if (SetSubKey(subKey)) {
                        return true;
                    } else {
                        m_bInitialized = false;
                        return false;
                    }
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        return false;
    }
}

RVA(0x00139330, 0x3d)
void CRegMgr::Term() {
    if (!m_bInitialized) {
        return;
    }
    m_bInitialized = false;
    RegCloseKey(m_hSoftwareKey);
    RegCloseKey(m_hCompanyKey);
    RegCloseKey(m_hAppKey);
    if (m_hVersionKey != m_hSubKey) {
        RegCloseKey(m_hVersionKey);
    }
    RegCloseKey(m_hSubKey);
}

RVA(0x00139370, 0x37)
BOOL CRegMgr::SetSubKey(const char* subKey) {
    if (!m_bInitialized) {
        return false;
    }
    if (subKey == NULL) {
        m_hSubKey = m_hVersionKey;
        return true;
    } else {
        if (CreateKey(m_hVersionKey, subKey, m_hSubKey)) {
            return true;
        } else {
            return false;
        }
    }
}

RVA(0x001393b0, 0x58)
BOOL CRegMgr::Set(const char* key, const char* value) {
    if (!m_bInitialized) {
        return false;
    }
    if (key == NULL) {
        return false;
    }
    if (value == NULL) {
        return false;
    }
    RegBufC data;
    if (RegSetValueExA(
            m_hSubKey,
            key,
            0,
            REG_SZ,
            (data.m_chars = value, data.m_bytes),
            strlen(value) + 1
        )
        == ERROR_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139410, 0x45)
BOOL CRegMgr::Set(const char* key, void* value, i32 length) {
    if (!m_bInitialized) {
        return false;
    }
    if (key == NULL) {
        return false;
    }
    if (value == NULL) {
        return false;
    }
    if (RegSetValueExA(m_hSubKey, key, 0, REG_BINARY, static_cast<LPBYTE>(value), length)
        == ERROR_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

RVA(0x00139460, 0x33)
BOOL CRegMgr::Set(const char* key, DWORD value) {
    if (!m_bInitialized) {
        return false;
    }
    if (key == NULL) {
        return false;
    }
    RegBuf data;
    if (RegSetValueExA(
            m_hSubKey,
            key,
            0,
            REG_DWORD,
            (data.m_dword = &value, data.m_bytes),
            sizeof(value)
        )
        == ERROR_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

RVA(0x001394a0, 0x97)
char* CRegMgr::Get(const char* key, char* buffer, DWORD& bufferSize, const char* defaultValue) {
    DWORD dwType;
    RegBuf data;
    if (!m_bInitialized) {
        goto Default;
    }
    if (key == NULL) {
        goto Default;
    }
    if (buffer == NULL) {
        goto Default;
    }
    if (bufferSize <= 0) {
        goto Default;
    }
    if (RegQueryValueExA(
            m_hSubKey,
            key,
            NULL,
            &dwType,
            (data.m_chars = buffer, data.m_bytes),
            &bufferSize
        )
        == ERROR_SUCCESS) {
        if (dwType != REG_SZ) {
            goto Default;
        }
        return buffer;
    } else {
        goto Default;
    }

Default:
    if (defaultValue != NULL) {
        strcpy(buffer, defaultValue);
        bufferSize = strlen(buffer);
        return buffer;
    } else {
        bufferSize = 0;
        return NULL;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139540, 0x8a)
void* CRegMgr::Get(
    const char* key,
    void* buffer,
    DWORD& bufferSize,
    void* defaultValue,
    DWORD defaultSize
) {
    DWORD dwType;
    if (!m_bInitialized) {
        goto Default;
    }
    if (key == NULL) {
        goto Default;
    }
    if (buffer == NULL) {
        goto Default;
    }
    if (bufferSize <= 0) {
        goto Default;
    }
    if (RegQueryValueExA(m_hSubKey, key, NULL, &dwType, static_cast<LPBYTE>(buffer), &bufferSize)
        == ERROR_SUCCESS) {
        if (dwType != REG_BINARY) {
            goto Default;
        }
        return buffer;
    } else {
        goto Default;
    }

Default:
    if (defaultValue != NULL && defaultSize > 0) {
        memcpy(buffer, defaultValue, defaultSize);
        bufferSize = defaultSize;
        return buffer;
    } else {
        bufferSize = 0;
        return NULL;
    }
}

RVA(0x001395d0, 0x50)
DWORD CRegMgr::Get(const char* key, DWORD defaultValue) {
    if (!m_bInitialized) {
        return defaultValue;
    }
    if (key == NULL) {
        return defaultValue;
    }
    DWORD dwType;
    DWORD dwData;
    DWORD cbData = sizeof(dwData);
    RegBuf data;
    if (RegQueryValueExA(
            m_hSubKey,
            key,
            NULL,
            &dwType,
            (data.m_dword = &dwData, data.m_bytes),
            &cbData
        )
        == ERROR_SUCCESS) {
        if (dwType != REG_DWORD) {
            return defaultValue;
        }
        return dwData;
    } else {
        return defaultValue;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139620, 0x28)
BOOL CRegMgr::Delete(const char* key) {
    if (!m_bInitialized) {
        return false;
    }
    if (key == NULL) {
        return false;
    }
    if (RegDeleteValueA(m_hSubKey, key) == ERROR_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

RVA(0x00139650, 0x32)
BOOL CRegMgr::CreateKey(HKEY key, const char* subKey, HKEY& newKey) {
    DWORD dwDisposition;
    if (RegCreateKeyExA(
            key,
            subKey,
            0,
            "",
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &newKey,
            &dwDisposition
        )
        == ERROR_SUCCESS) {
        return true;
    } else {
        return false;
    }
}
