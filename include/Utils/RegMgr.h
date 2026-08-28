#ifndef UTILS_REGMGR_H
#define UTILS_REGMGR_H

#include <Mfc.h>

#include <Ints.h>

class CRegMgr {
public:
    CRegMgr() {
        m_bInitialized = FALSE;
    }
    ~CRegMgr() {
        Term();
    }
    BOOL Init(
        const char* company,
        const char* app,
        const char* version,
        const char* subKey = NULL,
        HANDLE rootKey = HKEY_LOCAL_MACHINE,
        char* rootSubKey = NULL
    );
    void Term();
    BOOL SetSubKey(const char* subKey);
    BOOL Set(const char* key, const char* value);
    BOOL Set(const char* key, void* value, i32 length);
    BOOL Set(const char* key, DWORD value);
    char* Get(const char* key, char* buffer, DWORD& bufferSize, const char* defaultValue = NULL);
    DWORD Get(const char* key, DWORD defaultValue = 0);
    void*
    Get(const char* key,
        void* buffer,
        DWORD& bufferSize,
        void* defaultValue = NULL,
        DWORD defaultSize = 0);
    BOOL Delete(const char* key);

private:
    BOOL CreateKey(HKEY key, const char* subKey, HKEY& newKey);

    BOOL m_bInitialized;
    HKEY m_hRootKey;
    HKEY m_hSoftwareKey;
    HKEY m_hCompanyKey;
    HKEY m_hAppKey;
    HKEY m_hVersionKey;
    HKEY m_hSubKey;
    char m_sApp[0x100];
    char m_sSubKey[0x100];
};

#endif // UTILS_REGMGR_H
