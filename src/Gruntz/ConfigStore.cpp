// ConfigStore.cpp - CConfigStore registry/INI config getters.
//
// The 0x1bf*-0x1d5* registry/COM helper module. Retail compiled it with frame
// pointers retained (push ebp/mov ebp,esp) yet fully /O2-register-allocated; the
// frameless /O2 backlog aggregate stalled these in the mid-50%/24% range purely on
// the missing ebp frame. This TU uses the `framed` profile (/O2 /Oy-) which keeps
// ebp as the frame pointer while preserving the /O2 register allocation.
// docs/patterns/o2-optimizer-bailout-framed.md.
#include <rva.h>

#include <Gruntz/ConfigStore.h>

// CConfigStore::WriteString - registry mode (m_7c set): szKey==0 deletes the
// section subkey under OpenRoot(); szValue==0 deletes the named value under
// OpenSubKey(szSection); else writes a REG_SZ value (length lstrlen+1). Returns
// whether the Reg op succeeded (and 0 if the key could not be opened). INI mode
// forwards to WritePrivateProfileString against m_90.
// @early-stop
// same /Oy- regalloc/CSE residual as the rest of this module (branch structure +
// all externs match retail; only the register allocation differs). ~30%; final sweep.
RVA(0x001ccb5c, 0xa0)
i32 CConfigStore::WriteString(char* szSection, char* szKey, char* szValue) {
    if (m_7c != 0) {
        HKEY hKey;
        LONG rc;
        if (szKey == 0) {
            hKey = OpenRoot();
            if (hKey == 0) {
                return 0;
            }
            rc = RegDeleteKeyA(hKey, szSection);
        } else if (szValue == 0) {
            hKey = OpenSubKey(szSection);
            if (hKey == 0) {
                return 0;
            }
            rc = RegDeleteValueA(hKey, szKey);
        } else {
            hKey = OpenSubKey(szSection);
            if (hKey == 0) {
                return 0;
            }
            rc = RegSetValueExA(
                hKey,
                szKey,
                0,
                1 /*REG_SZ*/,
                (LPBYTE)szValue,
                lstrlenA(szValue) + 1
            );
        }
        RegCloseKey(hKey);
        return rc == 0;
    }

    return WritePrivateProfileStringA(szSection, szKey, szValue, m_90);
}

// CConfigStore::OpenRoot - opens/creates HKCU\Software\<m_7c>\<m_90> and returns
// the deepest key (0 on failure), closing the two intermediate keys.
// @early-stop
// /Oy- now reproduces the ebp frame (the missing-frame wall is cleared), but the
// residual is the optimizer's constant-hoisting/regalloc STATE: retail caches the
// repeated 0x2001f in esi and 0 in edi and pins this in ebx (bigger frame, this
// spilled); the recompile re-materializes the immediates. /Oy- necessary, not
// sufficient - the CSE state can't be forced from source. ~58%; final sweep.
RVA(0x001d4ee3, 0x94)
HKEY CConfigStore::OpenRoot() {
    HKEY hSoftware = 0;
    HKEY hMid = 0;
    HKEY hResult = 0;
    DWORD dwDisp;
    CConfigStore* self = this;

    if (RegOpenKeyExA((HKEY)0x80000001, "Software", 0, 0x2001f, &hSoftware) == 0
        && RegCreateKeyExA(hSoftware, m_7c, 0, 0, 0, 0x2001f, 0, &hMid, &dwDisp) == 0) {
        RegCreateKeyExA(hMid, self->m_90, 0, 0, 0, 0x2001f, 0, &hResult, &dwDisp);
    }

    if (hSoftware) {
        RegCloseKey(hSoftware);
    }
    if (hMid) {
        RegCloseKey(hMid);
    }
    return hResult;
}

// CConfigStore::OpenSubKey - opens szSub under the OpenRoot() key, then closes the
// root and returns the opened subkey (0 on failure).
// @early-stop
// same /Oy- regalloc/CSE residual as OpenRoot (frame matches; the constant-hoist
// state differs). ~57%; final sweep.
RVA(0x001d4f77, 0x46)
HKEY CConfigStore::OpenSubKey(char* szSub) {
    HKEY hResult = 0;
    DWORD dwDisp;

    HKEY hRoot = OpenRoot();
    if (hRoot == 0) {
        return 0;
    }

    RegCreateKeyExA(hRoot, szSub, 0, 0, 0, 0x2001f, 0, &hResult, &dwDisp);
    RegCloseKey(hRoot);
    return hResult;
}

// CConfigStore::GetInt - reads an integer value: through the registry when m_7c is
// set (open szSection subkey, RegQueryValueEx szKey), else via the INI file m_90
// (GetPrivateProfileInt). Returns nDefault on any failure.
// @early-stop
// /Oy- regalloc residual + retail reuses the dead szSection arg slot [ebp+8] as
// cbData (a fresh local here) and compares m_7c directly (cmp [ecx+0x7c],0 vs a
// load+test). Frame matches; the arg-slot reuse can't be forced. ~18%; final sweep.
RVA(0x001d4fbd, 0x6c)
i32 CConfigStore::GetInt(char* szSection, char* szKey, i32 nDefault) {
    DWORD dwType;
    DWORD dwData;

    if (m_7c != 0) {
        HKEY hKey = OpenSubKey(szSection);
        if (hKey == 0) {
            return nDefault;
        }
        DWORD cbData = 4;
        i32 rc = RegQueryValueExA(hKey, szKey, 0, &dwType, (LPBYTE)&dwData, &cbData);
        RegCloseKey(hKey);
        if (rc == 0) {
            return dwData;
        }
        return nDefault;
    }

    return GetPrivateProfileIntA(szSection, szKey, nDefault, m_90);
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CConfigStore);
