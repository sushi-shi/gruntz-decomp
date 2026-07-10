// RegistryHelper.h - Utils::RegistryHelper, the engine's registry/config wrapper
// (callers of ADVAPI32!Reg*). Minimal reconstruction sufficient to byte-match
// the leaf value-getters. Field names are placeholders (m_<hexoffset>); only the
// OFFSETS are load-bearing.
#ifndef UTILS_REGISTRYHELPER_H
#define UTILS_REGISTRYHELPER_H

#include <Ints.h>

// ---------------------------------------------------------------------------
// Reg* (ADVAPI32) + HKEY/DWORD/REGSAM/... come from the real <windows.h>
// (winreg.h), pulled the MFC-controlled way via <Mfc.h>.
#include <Mfc.h>

namespace Utils {

    // ---------------------------------------------------------------------------
    // RegistryHelper - opens a chain of nested registry subkeys and reads/writes
    // values from the deepest one.
    //   +0x00 m_open     : open/result gate (nonzero when the chain is open).
    //   +0x04 m_baseKey  : base HKEY saved by Open().
    //   +0x08 m_key1     : opened key #1   } a path of nested HKEYs created/opened by
    //   +0x0c m_key2     : opened key #2   } Open(); Close() RegCloseKey()s them all.
    //   +0x10 m_key3     : opened key #3   }
    //   +0x14 m_key4     : opened key #4   } (Close skips m_key4 when it == m_valueKey)
    //   +0x18 m_valueKey : deepest key - the one GetValue*/SetValue* operate on.
    //   +0x1c m_keyNameBuf : char[0x100] - Open() strcpy's szKeyName2 here.
    //   +0x11c m_lastKeyBuf: char[0x100] - Open() strcpy's szLastKey here.
    // (Layout pinned by the byte-exact getters + Close + Open.)
    // ---------------------------------------------------------------------------
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
            u32* pValueBufferSize,
            char* szDefault
        );
        void* GetValueBinary(
            char* szValueName,
            void* pBuffer,
            u32* pBufferSize,
            void* pDefault,
            u32 defaultSize
        );
        DWORD GetValueDword(char* szValueName, DWORD valueDefault);
        i32 SetValueString(char* szValueName, char* szValue);
        i32 SetValueBinary(char* szValueName, void* pData, u32 dataSize);
        i32 SetValueDword(char* szValueName, DWORD value);
        i32 DeleteValue(char* szValueName); // 0x139620
        void Close();
        i32 GetRegistryKey(HKEY hKey, char* szSubKey, PHKEY phKeyResult);

        i32 m_open;               // +0x00  open/result gate
        HKEY m_baseKey;           // +0x04  base HKEY
        HKEY m_key1;              // +0x08
        HKEY m_key2;              // +0x0c
        HKEY m_key3;              // +0x10
        HKEY m_key4;              // +0x14
        HKEY m_valueKey;          // +0x18  deepest/open registry key
        char m_keyNameBuf[0x100]; // +0x1c
        char m_lastKeyBuf[0x100]; // +0x11c
    };

} // namespace Utils

#endif // UTILS_REGISTRYHELPER_H
