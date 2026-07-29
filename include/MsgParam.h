#ifndef GRUNTZ_MSGPARAM_H
#define GRUNTZ_MSGPARAM_H

#include <Win32.h> // LPARAM/WPARAM/HWND (inert when <Mfc.h> already pulled windows.h)

// A window-message parameter slot.
//
// The Win32 message ABI passes POINTERS and HANDLES through its integer WPARAM /
// LPARAM slots by contract: CB_ADDSTRING's sender writes a string address into
// lParam and the combo box reads that same word back as a string; WM_COMMAND and
// WM_HSCROLL deliver the sending control's HWND in lParam. One slot, both readings
// real - so they are named here instead of punned at every SendMessage.
//
// (Cold paths only. A union spelling makes cl materialise a stack temporary where a
// cast stays a register expression, which is measurably wrong inside a tight walk -
// see the container-of note in <Dsndmgr/SoundVoiceList.h>.)
union MsgParam {
    LPARAM m_lparam;
    WPARAM m_wparam;
    LONG m_long; // SetWindowLong/GetWindowLong's word
    const char* m_str;
    void* m_ptr;
    HWND m_hwnd;
    WNDPROC m_wndproc; // the GWL_WNDPROC slot's real content
    // the same slot when the subclass procedure is declared int-returning (retail's
    // own shape - the value is the code address either way)
    int(__stdcall* m_intProc)(HWND, UINT, WPARAM, LPARAM);
};

// A registry value buffer. RegQueryValueEx/RegSetValueEx type their data parameter
// `BYTE*` while the caller's REG_SZ buffer is a char string and its REG_DWORD buffer
// is a DWORD - one buffer, the API's reading and the caller's, both real.
union RegBuf {
    LPBYTE m_bytes;
    char* m_chars;
    DWORD* m_dword;
};
union RegBufC {
    const BYTE* m_bytes;
    const char* m_chars;
};

#endif // GRUNTZ_MSGPARAM_H
