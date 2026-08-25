#ifndef UTILS_REGISTRYHELPER_H
#define UTILS_REGISTRYHELPER_H

#include <Mfc.h>

#include <Ints.h>

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
            char* vendorName,
            char* productName,
            char* versionName,
            char* valueSubkeyName,
            HKEY rootKey,
            char* softwareSubkeyName
        );
        i32 InitializeLastKey(char* valueSubkeyName);
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
        i32 GetRegistryKey(HKEY parentKey, char* subkeyName, PHKEY resultKey);

        i32 m_open;
        HKEY m_baseKey;
        HKEY m_softwareKey;
        HKEY m_vendorKey;
        HKEY m_productKey;
        HKEY m_versionKey;
        HKEY m_valueKey;
        char m_productName[0x100];
        char m_valueSubkeyName[0x100];
    };

} // namespace Utils

#endif // UTILS_REGISTRYHELPER_H
