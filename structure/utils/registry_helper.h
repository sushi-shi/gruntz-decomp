#ifndef UTILS_REGISTRY_HELPER_H
#define UTILS_REGISTRY_HELPER_H

/*
 * Utils::RegistryHelper — the HKLM\Software\Monolith Productions\Gruntz\1.0 key
 * chain wrapper. CGruntzMgr holds one (m_pRegistryHelper @0x38; see
 * ../game/cgruntzmgr.h).
 *
 * Layout ported from tomalla (@approx tomalla 1.0.1.77; field OFFSETS are
 * version-independent). Has graduated into src/Utils/RegistryHelper.h (matched
 * 0x21C layout); this comprehension restatement exists only so `gruntz structs` emits
 * a RegistryHelper layout standalone (`gruntz structs` prefers the src/ definition on
 * overlap). Self-contained (no <afxwin.h>): the HKEY handle type is typedef'd
 * here. Non-polymorphic — the first field is at +0x00.
 *
 * Provenance: not in RTTI; "Utils::RegistryHelper" is a tomalla-invented name for
 * a real, well-reconstructed binary class. The value-names it reads/writes are
 * enumerated in ../registry.h.
 */

namespace Utils
{
    typedef void *RegHKEY;  // HKEY

    class RegistryHelper
    {
    public:
        RegistryHelper();
        ~RegistryHelper();

        bool m_isInitialized;     // +0x000  open/result gate
        RegHKEY m_hKey;           // +0x004  base HKEY
        RegHKEY m_hKey2;          // +0x008  } a path of nested HKEYs created/opened
        RegHKEY m_hKey3;          // +0x00c  } by Open(); Close() RegCloseKey()s them
        RegHKEY m_hKey4;          // +0x010  }
        RegHKEY m_hKey5;          // +0x014  }
        RegHKEY m_hKey6;          // +0x018  deepest key (Get/SetValue operate here)
        char    m_szParam2[256];  // +0x01c  Open() strcpy's szKeyName3 here
        char    m_szParam4[256];  // +0x11c  Open() strcpy's szLastKey here
    };                            // 0x21c
}

#endif /* UTILS_REGISTRY_HELPER_H */
