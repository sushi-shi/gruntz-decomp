#ifndef UTILS_REGISTRYHELPER_H
#define UTILS_REGISTRYHELPER_H

#include <Ints.h>

#include <Mfc.h>

namespace Utils {

    class RegistryHelper {
    public:
        RegistryHelper() {
            m_open = 0;
        }
        ~RegistryHelper() {
            Close();
        }
        i32 Open(
            char* szKeyName1,
            char* szKeyName2,
            char* szKeyName3,
            char* szLastKey,
            HKEY hKey,
            char* szSubKey
        );
        i32 InitializeLastKey(char* szLastKey);
        char* GetValueString(
            char* szValueName,
            char* szValueBuffer,
            DWORD* pValueBufferSize,
            char* szDefault
        );

        void* GetValueBinary(
            char* szValueName,
            void* pBuffer,
            DWORD* pBufferSize,
            void* pDefault,
            u32 defaultSize
        );
        DWORD GetValueDword(char* szValueName, DWORD valueDefault);
        i32 SetValueString(const char* szValueName, const char* szValue);
        i32 SetValueBinary(char* szValueName, void* pData, u32 dataSize);
        i32 SetValueDword(char* szValueName, DWORD value);
        i32 DeleteValue(char* szValueName);
        void Close();
        i32 GetRegistryKey(HKEY hKey, char* szSubKey, PHKEY phKeyResult);

        i32 m_open;
        HKEY m_baseKey;
        HKEY m_key1;
        HKEY m_key2;
        HKEY m_key3;
        HKEY m_key4;
        HKEY m_valueKey;
        char m_keyNameBuf[0x100];
        char m_lastKeyBuf[0x100];
    };

} // namespace Utils

#endif // UTILS_REGISTRYHELPER_H
