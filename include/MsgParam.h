#ifndef GRUNTZ_MSGPARAM_H
#define GRUNTZ_MSGPARAM_H

#include <Win32.h>

struct InterfaceObject;
class CNetPlayerListNode;
class CNetSessionNode;

union MsgParam {
    LPARAM m_lparam;
    WPARAM m_wparam;
    LONG m_long;
    const char* m_str;
    InterfaceObject* m_interface;
    CNetPlayerListNode* m_player;
    CNetSessionNode* m_session;
    HICON m_icon;
    HWND m_hwnd;
    WNDPROC m_wndproc;

    int(__stdcall* m_intProc)(HWND, UINT, WPARAM, LPARAM);
};

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
