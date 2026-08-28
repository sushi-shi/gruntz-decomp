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
    if (subKey) {
        strcpy(m_sSubKey, subKey);
    } else {
        m_sSubKey[0] = 0;
    }

    m_hRootKey = static_cast<HKEY>(rootKey);

    char defaultSoftwareSubkey[] = DATA_COMPGEN(0x0021a064, "Software");

    if (CreateKey(m_hRootKey, rootSubKey ? rootSubKey : defaultSoftwareSubkey, m_hSoftwareKey)
        && CreateKey(m_hSoftwareKey, company, m_hCompanyKey)
        && CreateKey(m_hCompanyKey, app, m_hAppKey)
        && CreateKey(m_hAppKey, version, m_hVersionKey)) {
        m_bInitialized = TRUE;
        if (SetSubKey(subKey)) {
            return 1;
        }
        m_bInitialized = FALSE;
    }
    return 0;
}

RVA(0x00139330, 0x3d)
void CRegMgr::Term() {
    if (m_bInitialized) {
        m_bInitialized = FALSE;
        RegCloseKey(m_hSoftwareKey);
        RegCloseKey(m_hCompanyKey);
        RegCloseKey(m_hAppKey);
        if (m_hVersionKey != m_hSubKey) {
            RegCloseKey(m_hVersionKey);
        }
        RegCloseKey(m_hSubKey);
    }
}

RVA(0x00139370, 0x37)
BOOL CRegMgr::SetSubKey(const char* subKey) {
    if (!m_bInitialized) {
        return 0;
    }

    if (!subKey) {
        m_hSubKey = m_hVersionKey;
        return 1;
    }

    return CreateKey(m_hVersionKey, subKey, m_hSubKey) != 0;
}

RVA(0x001393b0, 0x58)
BOOL CRegMgr::Set(const char* key, const char* value) {
    if (!m_bInitialized) {
        return 0;
    }
    if (!key) {
        return 0;
    }
    if (!value) {
        return 0;
    }
    RegBufC data;
    return RegSetValueExA(
               m_hSubKey,
               key,
               0,
               1,
               (data.m_chars = value, data.m_bytes),
               strlen(value) + 1
           )
           == 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139410, 0x45)
BOOL CRegMgr::Set(const char* key, void* value, i32 length) {
    if (!m_bInitialized) {
        return 0;
    }
    if (!key) {
        return 0;
    }
    if (!value) {
        return 0;
    }
    return RegSetValueExA(m_hSubKey, key, 0, 3, static_cast<LPBYTE>(value), length) == 0;
}

RVA(0x00139460, 0x33)
BOOL CRegMgr::Set(const char* key, DWORD value) {
    if (!m_bInitialized) {
        return 0;
    }
    if (!key) {
        return 0;
    }
    RegBuf data;
    return RegSetValueExA(m_hSubKey, key, 0, 4, (data.m_dword = &value, data.m_bytes), 4) == 0;
}

RVA(0x001394a0, 0x97)
char* CRegMgr::Get(const char* key, char* buffer, DWORD& bufferSize, const char* defaultValue) {
    DWORD dwType;
    RegBuf data;

    if (m_bInitialized && key && buffer && bufferSize > 0) {
        if (RegQueryValueExA(
                m_hSubKey,
                key,
                NULL,
                &dwType,
                (data.m_chars = buffer, data.m_bytes),
                &bufferSize
            ) == 0
            && dwType == 1) {
            return buffer;
        }
    }

    if (defaultValue) {
        strcpy(buffer, defaultValue);
        bufferSize = strlen(buffer);
        return buffer;
    }

    bufferSize = 0;
    return NULL;
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

    if (m_bInitialized && key && buffer && bufferSize > 0) {
        if (RegQueryValueExA(
                m_hSubKey,
                key,
                NULL,
                &dwType,
                static_cast<LPBYTE>(buffer),
                &bufferSize
            ) == 0
            && dwType == REG_BINARY) {
            return buffer;
        }
    }

    if (defaultValue && defaultSize > 0) {
        memcpy(buffer, defaultValue, defaultSize);
        bufferSize = defaultSize;
        return buffer;
    }

    bufferSize = 0;
    return NULL;
}

RVA(0x001395d0, 0x50)
DWORD CRegMgr::Get(const char* key, DWORD defaultValue) {
    DWORD dwType;
    DWORD dwData;
    DWORD cbData;
    RegBuf data;

    if (m_bInitialized && key) {
        cbData = 4;
        if (RegQueryValueExA(
                m_hSubKey,
                key,
                NULL,
                &dwType,
                (data.m_dword = &dwData, data.m_bytes),
                &cbData
            ) == 0
            && dwType == REG_DWORD) {
            return dwData;
        }
    }

    return defaultValue;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139620, 0x28)
BOOL CRegMgr::Delete(const char* key) {
    if (!m_bInitialized) {
        return 0;
    }
    if (!key) {
        return 0;
    }
    return RegDeleteValueA(m_hSubKey, key) == 0;
}

RVA(0x00139650, 0x32)
BOOL CRegMgr::CreateKey(HKEY key, const char* subKey, HKEY& newKey) {
    DWORD dwDisposition;
    return RegCreateKeyExA(key, subKey, 0, "", 0, KEY_ALL_ACCESS, NULL, &newKey, &dwDisposition)
           == 0;
}
