// ConfigStore.h - CConfigStore, the engine's registry-or-INI config wrapper.
//
// When m_7c (a registry subkey path under HKCU\Software\...) is set the value
// getters go through ADVAPI32!Reg*; when it is null they fall back to
// GetPrivateProfile* against the INI file named by m_90. Only offsets are
// load-bearing.
//
// This module (the 0x1bf*-0x1d5* registry/COM helpers) is compiled /O2 /Oy- (the
// `framed` profile): retail keeps frame pointers here (push ebp/mov ebp,esp WITH
// register-allocated locals - optimized, not /Od), which the frameless /O2 backlog
// aggregate could not reproduce. /Oy- keeps ebp as the frame pointer while leaving
// the rest of the /O2 regalloc intact. See docs/patterns/o2-optimizer-bailout-framed.md.
#ifndef GRUNTZ_CONFIGSTORE_H
#define GRUNTZ_CONFIGSTORE_H

#include <Win32.h>

#include <Ints.h>

class CString; // MFC CString (4-byte m_pszData); modeled locally in ConfigStoreRead.cpp

class CConfigStore {
public:
    // 0x1d5029 __thiscall: read a string value, returned by value. Registry mode
    // RegQueryValueEx into a CString buffer; INI mode GetPrivateProfileString.
    CString GetString(char* szSection, char* szKey, char* szDefault);
    // 0x1d513b __thiscall: read a binary blob (registry REG_BINARY; INI mode reads
    // the A-P nibble-encoded string WriteProfileBinary wrote and decodes it).
    i32 GetBinary(char* szSection, char* szKey, void** ppData, i32* pLen);
    // 0x1ccb5c __thiscall: write/delete a string value (szKey==0 -> delete the
    // section subkey, szValue==0 -> delete the value, else RegSetValueEx REG_SZ;
    // INI fallback writes via WritePrivateProfileString).
    i32 WriteString(char* szSection, char* szKey, char* szValue);
    HKEY OpenRoot();                                        // 0x1d4ee3 __thiscall
    HKEY OpenSubKey(char* szSub);                           // 0x1d4f77 __thiscall
    i32 GetInt(char* szSection, char* szKey, i32 nDefault); // 0x1d4fbd __thiscall
    // 0x1ccae7 __thiscall: registry-mode writes a REG_DWORD, INI-mode formats the
    // value with "%d" and forwards to WritePrivateProfileString.
    i32 WriteProfileInt(char* szSection, char* szKey, i32 nValue);
    // 0x1ccbfc __thiscall: registry-mode writes REG_BINARY; INI-mode hex-encodes
    // the bytes (each nibble + 'A') and forwards through WriteString.
    i32 WriteProfileBinary(char* szSection, char* szKey, BYTE* pData, u32 nBytes);

    char m_pad00[0x7c];
    char* m_7c; // +0x7c  registry subkey path (null -> use INI)
    char m_pad80[0x90 - 0x80];
    char* m_90; // +0x90  INI file path (used in INI fallback)
};

#endif // GRUNTZ_CONFIGSTORE_H
