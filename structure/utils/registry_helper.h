#ifndef UTILS_REGISTRY_HELPER_H
#define UTILS_REGISTRY_HELPER_H

/*
 * Utils::RegistryHelper — the HKLM\Software\Monolith Productions\Gruntz\1.0 key
 * chain wrapper. CGruntzMgr holds one (m_pRegistryHelper @0x38; see
 * ../game/cgruntzmgr.h).
 *
 * LAYOUT PORTED FROM tomalla (refs/tomalla-gruntz/utils/registry_helper.h).
 * @approx tomalla 1.0.1.77  — field OFFSETS are version-independent (high
 * confidence); the function @address values are 1.0.1.77 and are NOT recorded
 * here (function addresses are deferred to the v1.01 re-anchor).
 *
 * Provenance: not in RTTI; "Utils::RegistryHelper" is a tomalla-invented name for
 * a real, well-reconstructed binary class. The key path it walks
 * (HKLM -> "Software" -> "Monolith Productions" -> "Gruntz" -> "1.0") matches the
 * registry strings mined from GRUNTZ.EXE (STRINGS_ANALYSIS.md §2). tomalla notes
 * the binary uses RegCreateKeyExA (CREATE), not merely RegOpenKeyA.
 *
 * The value-names read/written through GetValueDword/GetValueString/SetValueDword
 * are enumerated in ../registry.h.
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>

namespace Utils
{
    class RegistryHelper
    {
    public:
        //@size: 0x21C   @approx tomalla 1.0.1.77 (offsets version-independent)
        RegistryHelper();
        ~RegistryHelper();

        // Opens a chain of registry keys forming the path:
        //   hKey -> szSubKey ("Software" by default) -> szKeyName1 -> szKeyName2
        //        -> szKeyName3 -> szKeyName4 (optional).
        bool Open(char* szKeyName1, char* szKeyName2, char* szKeyName3,
                  char* szKeyName4, HKEY hKey, char* szSubKey);
        // Closes all opened keys.
        void Close();
        // Reads a REG_SZ into szValueBuffer; pValueBufferSize is in/out.
        char* GetValueString(char* szValueName, char* szValueBuffer,
                             unsigned int* pValueBufferSize, char* szDefault /*= 0*/);
        // Reads a DWORD value (returns valueDefault if absent).
        DWORD GetValueDword(char* szValueName, DWORD valueDefault);
        // Writes a DWORD value.
        bool SetValueDword(char* szValueName, DWORD value);

    private:
        bool InitializeLastKey(char* szLastKey);
        bool GetRegistryKey(HKEY hKey, char* szSubKey, PHKEY phKeyResult);

        //@offset: 0
        bool m_isInitialized;
        //@offset: 4   six nested-key HKEY handles (the walked path)
        HKEY m_hKey;
        //@offset: 8
        HKEY m_hKey2;
        //@offset: c
        HKEY m_hKey3;
        //@offset: 10
        HKEY m_hKey4;
        //@offset: 14
        HKEY m_hKey5;
        //@offset: 18
        HKEY m_hKey6;
        //@offset: 1c
        char m_szParam2[256];
        //@offset: 11c
        char m_szParam4[256];
        // (total 0x21C)
    };
}

#endif /* UTILS_REGISTRY_HELPER_H */
