#include <Win32.h>

#include <rva.h>
#include <string.h>

// Auto-generated API-caller stubs from docs/api-caller-name-plan.tsv.
// Greenfield only: tracked/already-tried and named-untracked library rows are intentionally excluded.
// One stub is emitted per RVA; rows with multiple API categories are merged.

// The CGameRegistry singleton (reloc-masked DATA symbol ?g_gameReg@@3PAUCGameReg@@A).
// Declared at global scope so it keeps the retail (un-namespaced) mangling.
struct CGameReg {
    char m_pad0[0x2c];
    void* m_2c; // +0x2c
    char m_pad30[0x58 - 0x30];
    void* m_58; // +0x58
    char m_pad5c[0x118 - 0x5c];
    int m_118; // +0x118
    char m_pad11c[0x130 - 0x11c];
    int m_130; // +0x130
    int m_134; // +0x134
    char m_pad138[0x7e8 - 0x138];
    int m_7e8;                            // +0x7e8
    void Method92340(int state);          // __thiscall helper at RVA 0x92340
    void ReportError(unsigned int, long); // CGruntzMgr::ReportError, RVA 0x346d
};
DATA(0x64556c)
extern CGameReg* g_gameReg;

// Miles Sound System (AIL) imports - reached through the IAT (ff 15 [__imp]).
extern "C" {
    __declspec(dllimport) int __stdcall AIL_set_XMIDI_master_volume(int driver, int volume);
    __declspec(dllimport) int __stdcall AIL_start_sequence(int seq);
    __declspec(dllimport) int __stdcall AIL_set_sequence_loop_count(int seq, int count);
    __declspec(dllimport) int __stdcall AIL_resume_sequence(int seq);
}

// The AIL MIDI driver handle (DAT_00653c5c), 0 when no driver is open.
DATA(0x653c5c)
extern int g_ailMidiDriver;

// MS-CRT-style LCG RNG state shared by the timeGetTime random helpers.
DATA(0x6c127d)
extern char g_rngSeeded; // bit0 set once the generator has been seeded
DATA(0x6c1288)
extern int g_rngState; // the current 32-bit LCG state

// Per-frame cached random bit used by the deterministic coin-flip helper.
DATA(0x64c22c)
extern char g_coinRolled; // bit0 set once this frame's coin was rolled
DATA(0x64c26c)
extern int g_coinValue; // the cached 0/1 result

namespace ApiCallerStubs {

    // @confidence: low
    // @source: winapi:CopyRect;SetRect
    // @stub
    RVA(0x00c840, 0x13d)
    int winapi_00c840_CopyRect_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // __cdecl rand(): lazily seed from timeGetTime, then advance the MS-CRT LCG.
    RVA(0x00cd00, 0x46)
    int winapi_00cd00_timeGetTime() {
        int seed;
        if (!(g_rngSeeded & 1)) {
            g_rngSeeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_rngState;
        }
        g_rngState = seed * 214013 + 2531011;
        return (g_rngState >> 0x10) & 0x7fff;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x00cd70, 0xe5)
    int winapi_00cd70_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x0143e0, 0xfb)
    int winapi_0143e0_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x014720, 0x37)
    int winapi_014720_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetWindow;GetWindowLongA;SetWindowLongA
    // @stub
    RVA(0x014d00, 0xa68)
    int winapi_014d00_GetWindow_GetWindowLongA_SetWindowLongA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x015cc0, 0x23)
    int winapi_015cc0_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // A CWnd-ish object whose HWND lives at +0x1c (returned by the dialog-item
    // resolver thunks that several wrappers below call).
    struct WndItem {
        char m_pad0[0x1c];
        HWND m_hwnd; // +0x1c
    };
    // Dialog-item resolver at RVA 0x15ac0 (reached through the 0x1e7e jmp-thunk).
    WndItem* __stdcall ResolveItem_15ac0(int id);

    // @source: winapi:SendMessageA
    // __stdcall(id): send 0x147 (clear listbox selection) to the resolved item.
    RVA(0x015d00, 0x20)
    void __stdcall winapi_015d00_SendMessageA(int id) {
        HWND h = ResolveItem_15ac0(id)->m_hwnd;
        SendMessageA(h, 0x147, 0, 0);
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x015d30, 0x21)
    int winapi_015d30_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x015d70, 0x24)
    int winapi_015d70_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x015fe0, 0xbe)
    int winapi_015fe0_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CreateSolidBrush;FillRect;GetClientRect
    // @stub
    RVA(0x0160f0, 0x245)
    int winapi_0160f0_CreateSolidBrush_FillRect_GetClientRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:InvalidateRect
    // @stub
    RVA(0x016cd0, 0x98)
    int winapi_016cd0_InvalidateRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:InvalidateRect
    // @stub
    RVA(0x016dc0, 0x97)
    int winapi_016dc0_InvalidateRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:InvalidateRect
    // @stub
    RVA(0x016e90, 0x98)
    int winapi_016e90_InvalidateRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:InvalidateRect
    // @stub
    RVA(0x016f60, 0x98)
    int winapi_016f60_InvalidateRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetWindow
    // @stub
    RVA(0x017030, 0xc1)
    int winapi_017030_GetWindow() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetWindow;SendMessageA
    // @stub
    RVA(0x0171b0, 0xca)
    int winapi_0171b0_GetWindow_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x0180e0, 0x23f)
    int winapi_0180e0_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x0183f0, 0x2e)
    int winapi_0183f0_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x019f50, 0xb2)
    int winapi_019f50_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x01a700, 0x6b6)
    int winapi_01a700_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x01f8a0, 0x30)
    int winapi_01f8a0_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:DrawTextA
    // @stub
    RVA(0x021f20, 0x162)
    int winapi_021f20_DrawTextA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetAsyncKeyState;SelectObject
    // @stub
    RVA(0x022160, 0x18e)
    int winapi_022160_GetAsyncKeyState_SelectObject() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:DrawTextA;SelectObject;SetTextColor
    // @stub
    RVA(0x022360, 0x2f4)
    int winapi_022360_DrawTextA_SelectObject_SetTextColor() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:DrawTextA;SelectObject;SetBkColor;SetBkMode;SetTextColor
    // @stub
    RVA(0x022810, 0x22a)
    int winapi_022810_DrawTextA_SelectObject_SetBkColor_SetBkMode_SetTextColor() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x023090, 0xfc)
    int winapi_023090_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect;PtInRect
    // @stub
    RVA(0x0267c0, 0x281d)
    int winapi_0267c0_IntersectRect_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x02a570, 0x4c6)
    int winapi_02a570_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PtInRect
    // @stub
    RVA(0x02ab80, 0x15e)
    int winapi_02ab80_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x02ae00, 0x42e)
    int winapi_02ae00_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x02b340, 0xaa)
    int winapi_02b340_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect;PtInRect
    // @stub
    RVA(0x02c140, 0x3e7)
    int winapi_02c140_IntersectRect_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x02c690, 0xdb4)
    int winapi_02c690_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x02dfa0, 0x325)
    int winapi_02dfa0_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PtInRect
    // @stub
    RVA(0x02e3a0, 0x7e1)
    int winapi_02e3a0_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x031ca0, 0x2f2)
    int winapi_031ca0_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x032060, 0x7bd)
    int winapi_032060_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x032ce0, 0x448)
    int winapi_032ce0_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x033520, 0xbc3)
    int winapi_033520_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EnableWindow;GetDlgItem;IsDlgButtonChecked
    // __cdecl(hWnd): mirror checkbox 0x46d into the game state + enable ctrl 0x470.
    RVA(0x036d00, 0x40)
    void winapi_036d00_EnableWindow_GetDlgItem_IsDlgButtonChecked(HWND hWnd) {
        if (g_gameReg) {
            int state = IsDlgButtonChecked(hWnd, 0x46d);
            g_gameReg->Method92340(state);
            EnableWindow(GetDlgItem(hWnd, 0x470), state);
        }
    }

    // @confidence: low
    // @source: winapi:EnableWindow;GetDlgItem;IsDlgButtonChecked
    // @stub
    RVA(0x036d50, 0x3c)
    int winapi_036d50_EnableWindow_GetDlgItem_IsDlgButtonChecked() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EnableWindow;GetDlgItem;IsDlgButtonChecked
    // @stub
    RVA(0x036da0, 0x40)
    int winapi_036da0_EnableWindow_GetDlgItem_IsDlgButtonChecked() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IsDlgButtonChecked
    // __cdecl: cache the checkbox state into g_gameReg->m_118.
    RVA(0x036e10, 0x26)
    void winapi_036e10_IsDlgButtonChecked(HWND hWnd) {
        if (g_gameReg) {
            g_gameReg->m_118 = IsDlgButtonChecked(hWnd, 0x455);
        }
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;GetScrollInfo
    // @stub
    RVA(0x036ec0, 0x41)
    int winapi_036ec0_GetDlgItem_GetScrollInfo() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;SetScrollInfo
    // @stub
    RVA(0x0371e0, 0x5b)
    int winapi_0371e0_GetDlgItem_SetScrollInfo() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetScrollInfo;SetScrollInfo
    // @stub
    RVA(0x037260, 0x1fd)
    int winapi_037260_GetScrollInfo_SetScrollInfo() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;SendMessageA
    // @stub
    RVA(0x037ff0, 0xe7)
    int winapi_037ff0_GetDlgItem_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem
    // __stdcall(hDlg, id, lo, hi): find the list item whose data == MAKELONG(lo,hi)
    // and select it. Returns 1 if found.
    RVA(0x038150, 0x91)
    int __stdcall winapi_038150_GetDlgItem(HWND hDlg, int id, int lo, int hi) {
        HWND list = GetDlgItem(hDlg, id);
        if (!list) {
            return 0;
        }
        int searching = 1;
        int i = 0;
        while (searching) {
            int data = SendMessageA(list, 0x150, i, 0);
            if (data != -1) {
                int itemLo = data & 0xffff;
                int itemHi = (unsigned)data >> 0x10;
                if (itemLo == lo && itemHi == hi) {
                    if (SendMessageA(list, 0x147, 0, 0) != i) {
                        SendMessageA(list, 0x14e, i, 0);
                    }
                    return 1;
                }
            } else {
                searching = 0;
            }
            i++;
        }
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem
    // @stub
    RVA(0x038220, 0x73)
    int winapi_038220_GetDlgItem() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x039440, 0x46)
    int winapi_039440_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // __thiscall(x, _, y): if (x,y) is in the 0..0x64 box, run the click handler;
    // otherwise post a 0x111 command (0x8023/0x8027 by mode). Always returns 1.
    struct ClickWnd_0394b0 {
        char m_pad0[4];
        ClickWnd_0394b0* m_4; // +0x04 -> m_4 -> m_4 = HWND
    };
    struct ClickHost_0394b0 {
        char m_pad0[4];
        ClickWnd_0394b0* m_4; // +0x04
        char m_pad8[0x24 - 8];
        int m_24; // +0x24
        int OnClick(int x, int unused, int y);
        void Activate(); // RVA 0x3d41
    };
    // @source: winapi:PostMessageA;PtInRect
    RVA(0x0394b0, 0x86)
    int ClickHost_0394b0::OnClick(int x, int unused, int y) {
        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.right = 0x64;
        rc.bottom = 0x64;
        POINT pt;
        pt.x = x;
        pt.y = y;
        if (PtInRect(&rc, pt)) {
            Activate();
            return 1;
        }
        int cmd;
        if (m_24 == 5) {
            cmd = 0x8023;
        } else {
            cmd = 0x8027;
        }
        PostMessageA((HWND)m_4->m_4->m_4, 0x111, cmd, 0);
        return 1;
    }

    // @confidence: low
    // @source: winapi:SelectClipRgn;SetBkMode
    // @stub
    RVA(0x0396f0, 0x2b8)
    int winapi_0396f0_SelectClipRgn_SetBkMode() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CreateRectRgn;DrawTextA;SetRect
    // @stub
    RVA(0x039a60, 0x179)
    int winapi_039a60_CreateRectRgn_DrawTextA_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;SendMessageA
    // @stub
    RVA(0x03af90, 0x194)
    int winapi_03af90_GetDlgItem_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem
    // @stub
    RVA(0x03b1a0, 0x118)
    int winapi_03b1a0_GetDlgItem() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem
    // @stub
    RVA(0x03b310, 0x10d)
    int winapi_03b310_GetDlgItem() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect;OffsetRect
    // @stub
    RVA(0x04a9f0, 0x1aa)
    int winapi_04a9f0_CopyRect_OffsetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x04d800, 0x423)
    int winapi_04d800_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x057db0, 0x8f8)
    int winapi_057db0_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x064540, 0x11c)
    int winapi_064540_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect;SetRect
    // @stub
    RVA(0x075c60, 0x1ba)
    int winapi_075c60_CopyRect_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PtInRect
    // @stub
    RVA(0x077df0, 0x13d)
    int winapi_077df0_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x078060, 0x18d)
    int winapi_078060_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x0861e0, 0xc5)
    int winapi_0861e0_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA;wsprintfA
    // @stub
    RVA(0x0862f0, 0x3d5a)
    int winapi_0862f0_PostMessageA_wsprintfA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x08c380, 0x1e)
    int winapi_08c380_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x08e3a0, 0x94)
    int winapi_08e3a0_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x08e6c0, 0x85)
    int winapi_08e6c0_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:MessageBoxA
    // @stub
    RVA(0x08ee70, 0x7c)
    int winapi_08ee70_MessageBoxA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CreateProcessA;RegQueryValueA
    // @stub
    RVA(0x08f120, 0x168)
    int winapi_08f120_CreateProcessA_RegQueryValueA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x08f340, 0xf6)
    int winapi_08f340_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x08f480, 0x49)
    int winapi_08f480_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x08f530, 0xbd)
    int winapi_08f530_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // __thiscall(int code): clamp code into (0,0x29] and post a 0x111 command.
    struct WndOwner_090220 {
        int m_0;
        HWND m_4; // +0x04 = HWND
    };
    struct CmdHost_090220 {
        int m_0;
        WndOwner_090220* m_4; // +0x04
        void Post(int code);
    };
    RVA(0x090220, 0x2f)
    void CmdHost_090220::Post(int code) {
        if (code > 0 && code <= 0x29) {
            int v = (code == 0x29) ? 1 : code;
            PostMessageA(m_4->m_4, 0x111, 0x807f, v);
        }
    }

    // @confidence: low
    // @source: winapi:DialogBoxParamA
    // @stub
    RVA(0x090260, 0x13e)
    int winapi_090260_DialogBoxParamA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CreateProcessA;wsprintfA
    // @stub
    RVA(0x090860, 0xd3)
    int winapi_090860_CreateProcessA_wsprintfA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x092710, 0x77)
    int winapi_092710_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EndDialog
    // @stub
    RVA(0x092ab0, 0x20d)
    int winapi_092ab0_EndDialog() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x092f00, 0x1ef)
    int winapi_092f00_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:ValidateRect
    // @stub
    RVA(0x094bc0, 0x31)
    int winapi_094bc0_ValidateRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // __thiscall(int code, int): on ESC/SPACE/ENTER post a 0x111 command. Returns 1.
    struct WndChain_0953f0 {
        int m_0;
        WndChain_0953f0* m_4; // +0x04
    };
    struct CmdHost_0953f0 {
        int m_0;
        WndChain_0953f0* m_4; // +0x04
        int Key(int code, int unused);
    };
    RVA(0x0953f0, 0x37)
    int CmdHost_0953f0::Key(int code, int unused) {
        if (code == 0x1b || code == 0x20 || code == 0xd) {
            PostMessageA(m_4->m_4->m_4, 0x111, 0x8036, 0);
        }
        return 1;
    }

    // @confidence: low
    // @source: winapi:EndDialog
    // @stub
    RVA(0x09dff0, 0x8c)
    int winapi_09dff0_EndDialog() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetDlgItemTextA
    // @stub
    RVA(0x09e2d0, 0x84)
    int winapi_09e2d0_SetDlgItemTextA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EndDialog;PostMessageA
    // @stub
    RVA(0x09e390, 0x243)
    int winapi_09e390_EndDialog_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x09ea60, 0x168)
    int winapi_09ea60_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime;wsprintfA
    // @stub
    RVA(0x0b6b40, 0x29e)
    int winapi_0b6b40_timeGetTime_wsprintfA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x0b6e90, 0x34d)
    int winapi_0b6e90_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: directx-wrapper-caller:calls 0x1780b0 (DPLAYX.#1)
    // @stub
    RVA(0x0b77a0, 0xb5)
    int directx_wrapper_caller_0b77a0_DPLAYX_1() {
        return 0;
    }

    // @confidence: low
    // App-instance chain: this->m_4->m_8->m_c is the HINSTANCE for LoadString.
    struct AppRes_0b7ec0 {
        char m_pad0[0xc];
        HINSTANCE m_c; // +0x0c
    };
    struct AppHolder_0b7ec0 {
        char m_pad0[8];
        AppRes_0b7ec0* m_8; // +0x08
    };
    struct StrHost_0b7ec0 {
        char m_pad0[4];
        AppHolder_0b7ec0* m_4;             // +0x04
        void SetText(char* text, int arg); // RVA 0xb7e30 (thunk 0x1af0)
        void Load(int id, int dest);
    };
    // @source: winapi:LoadStringA
    // __thiscall(id, dest): load string `id`, defaulting to "Error", then push it.
    RVA(0x0b7ec0, 0x7d)
    void StrHost_0b7ec0::Load(int id, int dest) {
        char buf[0x12a];
        if (m_4 && m_4->m_8->m_c) {
            if (!LoadStringA(m_4->m_8->m_c, id, buf, 0xfa)) {
                strcpy(buf, "Error");
            }
            SetText(buf, dest);
        }
    }

    // @confidence: low
    // @source: winapi:GetWindowTextLengthA
    // @stub
    RVA(0x0bb3e0, 0xe5)
    int winapi_0bb3e0_GetWindowTextLengthA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetAsyncKeyState;Sleep;timeGetTime;wsprintfA
    // @stub
    RVA(0x0bb700, 0x265)
    int winapi_0bb700_GetAsyncKeyState_Sleep_timeGetTime_wsprintfA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:Sleep
    // @stub
    RVA(0x0bba10, 0x1fb)
    int winapi_0bba10_Sleep() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;SetTimer
    // @stub
    RVA(0x0bda00, 0x3e)
    int winapi_0bda00_GetDlgItem_SetTimer() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;SetTimer
    // @stub
    RVA(0x0bdb90, 0x3e)
    int winapi_0bdb90_GetDlgItem_SetTimer() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EndDialog;KillTimer
    // @stub
    RVA(0x0bdc00, 0x10c)
    int winapi_0bdc00_EndDialog_KillTimer() {
        return 0;
    }

    // @confidence: low
    // Init helper at RVA 0xbddb0 (__cdecl(hWnd, ctx)).
    void Init_bddb0(HWND hWnd, void* ctx);
    // GetDlgItem(hWnd,0x4b6) cache (DAT_00648ce0).
    DATA(0x648ce0)
    extern HWND g_dlgItem_648ce0;
    // @source: winapi:GetDlgItem;SetTimer
    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache a child control handle.
    RVA(0x0bdd60, 0x3e)
    void winapi_0bdd60_GetDlgItem_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_bddb0(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;SetTimer
    // @stub
    RVA(0x0bdfe0, 0x3e)
    int winapi_0bdfe0_GetDlgItem_SetTimer() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EndDialog;KillTimer;PostMessageA
    // @stub
    RVA(0x0be0a0, 0x1c7)
    int winapi_0be0a0_EndDialog_KillTimer_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;SetDlgItemTextA;SetTimer
    // @stub
    RVA(0x0be2f0, 0xb9)
    int winapi_0be2f0_GetDlgItem_SetDlgItemTextA_SetTimer() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetWindowTextA;SetWindowTextA
    // @stub
    RVA(0x0be400, 0x6c)
    int winapi_0be400_GetWindowTextA_SetWindowTextA() {
        return 0;
    }

    // @confidence: low
    // Session host (arg2). Stop() at RVA 0xb95f0; SetStatus(text, flag) at 0xb7e30
    // (reached via thunk 0x1af0). m_584 marks a normal exit; m_5c4 carries its code.
    struct SessionHost_0be490 {
        char m_pad0[0x584];
        int m_584; // +0x584
        char m_pad588[0x5c4 - 0x588];
        int m_5c4;                         // +0x5c4
        void Stop();                       // RVA 0xb95f0
        void SetStatus(char* text, int f); // RVA 0xb7e30 (thunk 0x1af0)
    };
    // A peer dialog/session whose m_52c gates the abnormal-termination path.
    struct PeerSession_0be490 {
        char m_pad0[0x52c];
        int m_52c; // +0x52c
    };
    DATA(0x6487e0)
    extern char g_sessionFlag; // DAT_006487e0
    DATA(0x6496ac)
    extern PeerSession_0be490* g_peerSession; // DAT_006496ac
    // @source: winapi:EndDialog;KillTimer
    // __cdecl(hWnd, session): stop the session and end the dialog appropriately.
    RVA(0x0be490, 0x84)
    void winapi_0be490_EndDialog_KillTimer(HWND hWnd, SessionHost_0be490* session) {
        if (hWnd && session) {
            g_sessionFlag = 0;
            session->Stop();
            if (session->m_584) {
                KillTimer(hWnd, 1);
                EndDialog(hWnd, session->m_5c4);
            } else if (g_peerSession->m_52c) {
                KillTimer(hWnd, 1);
                session->SetStatus("The game session has been terminated.", 0);
                EndDialog(hWnd, 0x4ce);
            } else {
                g_sessionFlag = 0;
            }
        }
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;SetDlgItemTextA;SetTimer
    // @stub
    RVA(0x0be760, 0x82)
    int winapi_0be760_GetDlgItem_SetDlgItemTextA_SetTimer() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetWindow;GetWindowLongA;SetWindowLongA
    // @stub
    RVA(0x0c1840, 0x16e)
    int winapi_0c1840_GetWindow_GetWindowLongA_SetWindowLongA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetWindow;SendMessageA
    // @stub
    RVA(0x0c1aa0, 0x2f8)
    int winapi_0c1aa0_GetWindow_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x0c2980, 0x28)
    int winapi_0c2980_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // __thiscall OnInitDialog: chain to CDialog::OnInitDialog (RVA 0x1bac5e),
    // then arm a 50 ms timer on this dialog's HWND (m_1c). Returns TRUE.
    struct TimerDlg_0c2cb0 {
        char m_pad0[0x1c];
        HWND m_1c; // +0x1c
        int OnInitDialog();
        int BaseOnInitDialog(); // RVA 0x1bac5e (CDialog::OnInitDialog)
    };
    // @source: winapi:SetTimer
    RVA(0x0c2cb0, 0x1f)
    int TimerDlg_0c2cb0::OnInitDialog() {
        BaseOnInitDialog();
        SetTimer(m_1c, 1, 0x32, 0);
        return 1;
    }

    // @confidence: low
    // @source: winapi:GetWindowTextLengthA
    // @stub
    RVA(0x0c2ce0, 0xf3)
    int winapi_0c2ce0_GetWindowTextLengthA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CreateSolidBrush;FillRect;GetClientRect
    // @stub
    RVA(0x0c2e20, 0x21d)
    int winapi_0c2e20_CreateSolidBrush_FillRect_GetClientRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x0c3e30, 0xfe)
    int winapi_0c3e30_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetFocus;SendMessageA
    // @stub
    RVA(0x0c4230, 0x38e)
    int winapi_0c4230_GetFocus_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:KillTimer;timeGetTime;wsprintfA
    // @stub
    RVA(0x0c46b0, 0x371)
    int winapi_0c46b0_KillTimer_timeGetTime_wsprintfA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x0c4ee0, 0x33)
    int winapi_0c4ee0_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x0c4f30, 0x33)
    int winapi_0c4f30_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // Item resolver at RVA 0xc27c0 (push id; stdcall; returns the CWnd-ish item).
    WndItem* __stdcall ResolveItem_c27c0(int id);
    // __thiscall host for winapi_0c4f80: resolves dialog item 2, clears its
    // selection, caches the new count in g_gameReg->m_7e8, then refreshes.
    struct SelHost_0c4f80 {
        void Update();
        void Refresh(); // RVA 0xc40b0
    };
    // @source: winapi:SendMessageA
    RVA(0x0c4f80, 0x33)
    void SelHost_0c4f80::Update() {
        HWND h = ResolveItem_c27c0(2)->m_hwnd;
        g_gameReg->m_7e8 = SendMessageA(h, 0x147, 0, 0) + 1;
        Refresh();
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x0c4fd0, 0x33)
    int winapi_0c4fd0_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x0c50f0, 0x9b)
    int winapi_0c50f0_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x0c7ec0, 0x5f5)
    int winapi_0c7ec0_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetCursorPos
    // @stub
    RVA(0x0c8a10, 0x119)
    int winapi_0c8a10_GetCursorPos() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x0cbaf0, 0x16f)
    int winapi_0cbaf0_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x0cdb10, 0x80c)
    int winapi_0cdb10_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x0ceae0, 0x268)
    int winapi_0ceae0_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // Sub-objects reached through the dispatcher's members; each has a __thiscall
    // reset/notify with no args (RVAs 0x137a80/0x138530/0x40b660/0x51af90).
    struct DispWnd_0cfbd0 {
        char m_pad0[4];
        DispWnd_0cfbd0* m_4; // +0x04
    };
    struct DispBoard_0cfbd0 {
        void Reset137a80(); // RVA 0x137a80
    };
    struct DispAudio_0cfbd0 {
        void Reset138530(); // RVA 0x138530
    };
    struct DispNet_0cfbd0 {
        void Reset40b660(); // RVA 0x40b660 (thunk 0x28ab)
    };
    struct DispUi_0cfbd0 {
        void Reset51af90(); // RVA 0x51af90 (thunk 0x244b)
    };
    struct DispOwner_0cfbd0 {
        char m_pad0[4];
        DispWnd_0cfbd0* m_4; // +0x04 -> m_4 -> m_4 = HWND
        char m_pad8[0x48 - 8];
        DispAudio_0cfbd0* m_48; // +0x48
        char m_pad4c[0x54 - 0x4c];
        DispNet_0cfbd0* m_54; // +0x54
        char m_pad58[0x60 - 0x58];
        DispUi_0cfbd0* m_60; // +0x60
        void Post(int code); // RVA 0x90220
    };
    struct DispInner_0cfbd0 {
        char m_pad0[0x2c];
        DispBoard_0cfbd0* m_2c; // +0x2c
    };
    struct DispCtx_0cfbd0 {
        char m_pad0[0x28];
        DispInner_0cfbd0* m_28; // +0x28
    };
    struct Dispatcher_0cfbd0 {
        char m_pad0[4];
        DispOwner_0cfbd0* m_4; // +0x04
        char m_pad8[0xc - 8];
        DispCtx_0cfbd0* m_c; // +0x0c
        char m_pad10[0x1c - 0x10];
        int m_1c; // +0x1c
        char m_pad20[0x40 - 0x20];
        int m_40; // +0x40
        char m_pad44[0x1bc - 0x44];
        int m_1bc; // +0x1bc
        int m_1c0; // +0x1c0
        int Dispatch();
    };
    // @source: winapi:PostMessageA
    RVA(0x0cfbd0, 0x8f)
    int Dispatcher_0cfbd0::Dispatch() {
        if (m_1c == 0x20) {
            m_1c0 = 1;
            m_40 = 1;
            DispInner_0cfbd0* inner = m_c->m_28;
            if (inner->m_2c) {
                inner->m_2c->Reset137a80();
            }
            m_4->m_48->Reset138530();
            m_4->m_54->Reset40b660();
            m_4->m_60->Reset51af90();
            PostMessageA((HWND)m_4->m_4->m_4, 0x111, 0x8023, 0);
            return 1;
        }
        if (m_1bc) {
            PostMessageA((HWND)m_4->m_4->m_4, 0x111, 0x8023, 0);
            return 1;
        }
        m_4->Post(m_1c + 1);
        return 1;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x0d00a0, 0x5a)
    int winapi_0d00a0_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x0d0b30, 0x200)
    int winapi_0d0b30_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x0d5f90, 0xd7)
    int winapi_0d5f90_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:wsprintfA
    // @stub
    RVA(0x0d60b0, 0x2cd)
    int winapi_0d60b0_wsprintfA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x0d7220, 0x7b)
    int winapi_0d7220_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:wsprintfA
    // @stub
    RVA(0x0d7520, 0x3b9)
    int winapi_0d7520_wsprintfA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x0d8c60, 0xea)
    int winapi_0d8c60_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:wsprintfA
    // @stub
    RVA(0x0d95f0, 0x756)
    int winapi_0d95f0_wsprintfA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // __thiscall coin-flip: deterministic ((m_1c+1)%2) in replay mode, otherwise a
    // once-per-frame random bit lazily seeded from timeGetTime.
    struct CoinHost_0da200 {
        char m_pad0[0x1c];
        int m_1c; // +0x1c
        int Flip();
    };
    RVA(0x0da200, 0x9b)
    int CoinHost_0da200::Flip() {
        CGameReg* gr = g_gameReg;
        if (gr->m_134 == 1 && gr->m_130 == 0) {
            return (m_1c + 1) % 2;
        }
        if (!(g_coinRolled & 1)) {
            int seed;
            g_coinRolled |= 1;
            if (!(g_rngSeeded & 1)) {
                g_rngSeeded |= 1;
                seed = timeGetTime();
            } else {
                seed = g_rngState;
            }
            g_rngState = seed * 214013 + 2531011;
            g_coinValue = ((g_rngState >> 0x10) & 0x7fff) % 2;
        }
        return g_coinValue;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    // @stub
    RVA(0x0de590, 0x2e)
    int winapi_0de590_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EndDialog
    // @stub
    RVA(0x0e35f0, 0x77)
    int winapi_0e35f0_EndDialog() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EndDialog
    // @stub
    RVA(0x0e3a40, 0xb0)
    int winapi_0e3a40_EndDialog() {
        return 0;
    }

    // @confidence: low
    // SetDlgItemTextA helper defined below (RVA 0xe4850, reached via thunk 0x103c).
    void winapi_0e4850_SetDlgItemTextA(HWND hWnd, void* gate, char* item);
    // The optional info-line text shown on WM_INITDIALOG (DAT_0064c864).
    DATA(0x64c864)
    extern char* g_dlgInfoText;
    // @source: winapi:EndDialog
    // __stdcall DialogProc: OK/Cancel close the dialog; WM_INITDIALOG fills a line.
    RVA(0x0e3b20, 0x86)
    int __stdcall winapi_0e3b20_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case 0x110:
                if (g_dlgInfoText == 0) {
                    EndDialog(hDlg, (INT_PTR)g_dlgInfoText);
                    return 1;
                }
                winapi_0e4850_SetDlgItemTextA(hDlg, g_gameReg->m_58, g_dlgInfoText);
                return 1;
            case 0x111:
                if (wParam == 2) {
                    EndDialog(hDlg, 0);
                    return 1;
                }
                if (wParam == 1) {
                    EndDialog(hDlg, wParam);
                    return 1;
                }
                break;
        }
        return 0;
    }

    // @confidence: low
    // @source: winapi:EndDialog
    // @stub
    RVA(0x0e3be0, 0x52)
    int winapi_0e3be0_EndDialog() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetDlgItemTextA
    // @stub
    RVA(0x0e3e80, 0x86)
    int winapi_0e3e80_SetDlgItemTextA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetDlgItemTextA
    // __cdecl: SetDlgItemTextA(hWnd, 0x40d, &item->text) when all ptrs non-null.
    RVA(0x0e4850, 0x29)
    void winapi_0e4850_SetDlgItemTextA(HWND hWnd, void* gate, char* item) {
        if (hWnd && gate && item) {
            SetDlgItemTextA(hWnd, 0x40d, item + 0x14);
        }
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x0e6020, 0x288)
    int winapi_0e6020_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x0ecc90, 0x86a)
    int winapi_0ecc90_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PtInRect
    // @stub
    RVA(0x0ed9f0, 0x8dd)
    int winapi_0ed9f0_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect;PtInRect
    // @stub
    RVA(0x0ee800, 0x971)
    int winapi_0ee800_IntersectRect_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect;PtInRect
    // @stub
    RVA(0x0f0e20, 0x928)
    int winapi_0f0e20_IntersectRect_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x0f36a0, 0x78e)
    int winapi_0f36a0_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PtInRect
    // @stub
    RVA(0x0f42f0, 0x1193)
    int winapi_0f42f0_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x0f60f0, 0xb30)
    int winapi_0f60f0_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:FreeLibrary
    // @stub
    RVA(0x0f8e20, 0x56)
    int winapi_0f8e20_FreeLibrary() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:BeginPaint;EndPaint
    // @stub
    RVA(0x0fac70, 0x4c)
    int winapi_0fac70_BeginPaint_EndPaint() {
        return 0;
    }

    // @confidence: low
    // Free init helper at RVA 0x500930 (__stdcall(int)).
    void __stdcall Prep_500930(int flag);
    // A sub-object reached via g_gameReg->m_2c whose Refresh() is at RVA 0x4d8c60.
    struct SubMgr_0fe460 {
        void Refresh(); // RVA 0x4d8c60
    };
    // The screen object this method initialises (RVA 0xfe460).
    struct Screen_0fe460 {
        int m_0; // +0x00
        char m_pad4[0x10 - 4];
        RECT m_10; // +0x10
        char m_pad20[0x10c - 0x20];
        int m_10c; // +0x10c
        char m_pad110[0x548 - 0x110];
        int m_548; // +0x548
        int Open();
        void Resize(int n);          // RVA 0x4fe3e0
        int Validate();              // RVA 0x4ffde0 (thunk via 0x3a08)
        void Activate(int a, int n); // RVA 0x500d70
    };
    // @source: winapi:SetRect
    // __thiscall: lay out the 0xa0x0x1e0 screen, validate it, else report error.
    RVA(0x0fe460, 0x83)
    int Screen_0fe460::Open() {
        if (m_548 == 0 && m_0 != 1) {
            Prep_500930(1);
            SetRect(&m_10, 0, 0, 0xa0, 0x1e0);
            Resize(1);
            ((SubMgr_0fe460*)g_gameReg->m_2c)->Refresh();
            if (!Validate()) {
                g_gameReg->ReportError(0x80e4, 0x448);
                return 0;
            }
            Activate(m_10c, 3);
        }
        return 1;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x0fe520, 0xa9)
    int winapi_0fe520_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x0fe600, 0x49)
    int winapi_0fe600_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x107d00, 0x591)
    int winapi_107d00_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x115300, 0xf5)
    int winapi_115300_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect;OffsetRect
    // @stub
    RVA(0x115930, 0x15b)
    int winapi_115930_CopyRect_OffsetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x115b30, 0x15)
    int winapi_115b30_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetActiveWindow;SetFocus
    // __cdecl: activate + focus the same window.
    RVA(0x118930, 0x15)
    void winapi_118930_SetActiveWindow_SetFocus(HWND hWnd) {
        SetActiveWindow(hWnd);
        SetFocus(hWnd);
    }

    // @confidence: low
    // @source: winapi:OutputDebugStringA
    // @stub
    RVA(0x118b50, 0x5b)
    int winapi_118b50_OutputDebugStringA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IsIconic
    // @stub
    RVA(0x1192d0, 0x39)
    int winapi_1192d0_IsIconic() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x11b3b0, 0x338)
    int winapi_11b3b0_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x11b7c0, 0x304)
    int winapi_11b7c0_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetLastError;HeapValidate;HeapWalk
    // @stub
    RVA(0x1206b0, 0x1ad)
    int winapi_1206b0_GetLastError_HeapValidate_HeapWalk() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetCurrentThreadId;TlsSetValue
    // @stub
    RVA(0x123d10, 0x8c)
    int winapi_123d10_GetCurrentThreadId_TlsSetValue() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:FindResourceA;LoadResource;LockResource
    // @stub
    RVA(0x136a30, 0x76)
    int winapi_136a30_FindResourceA_LoadResource_LockResource() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:FindResourceA;LoadResource;LockResource
    // @stub
    RVA(0x136ce0, 0x92)
    int winapi_136ce0_FindResourceA_LoadResource_LockResource() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x136e20, 0xa8)
    int winapi_136e20_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x136fe0, 0x7b)
    int winapi_136fe0_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: directx-wrapper-caller:calls 0x136550 (DSOUND.#1_DirectSoundCreate)
    // @stub
    RVA(0x137720, 0x14)
    int directx_wrapper_caller_137720_DSOUND_1_DirectSoundCreate() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x137ac0, 0xa2)
    int winapi_137ac0_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x137e30, 0x98)
    int winapi_137e30_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // A device object whose Prepare(flag) lives at RVA 0x135a70.
    struct Device_1380d0 {
        int Prepare(int flag); // RVA 0x135a70
    };
    // __thiscall(timestamp) host: timestamp -1 means "now"; prep the device, then
    // run the work pass (RVA 0x137f30) over m_c/m_10. Returns whether it ran.
    struct Timer_1380d0 {
        char m_pad0[0x8];
        Device_1380d0* m_8; // +0x08
        int m_c;            // +0x0c
        int m_10;           // +0x10
        char m_pad14[0x20 - 0x14];
        int m_20; // +0x20
        char m_pad24[0x28 - 0x24];
        int m_28; // +0x28
        int Tick(int timestamp);
        int Work(int a, int b); // RVA 0x137f30
    };
    // @source: winapi:timeGetTime
    RVA(0x1380d0, 0x4e)
    int Timer_1380d0::Tick(int timestamp) {
        int t = (timestamp == -1) ? (int)timeGetTime() : timestamp;
        m_28 = t;
        m_c = 0;
        if (!m_8->Prepare(0)) {
            return 0;
        }
        m_20 = 0;
        return Work(m_c, m_10) != 0;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_midiOutOpen@12;_AIL_startup@0
    // @stub
    RVA(0x138490, 0x5e)
    int thirdparty_138490_AIL_midiOutOpen_12_AIL_startup_0() {
        return 0;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_shutdown@0
    // @stub
    RVA(0x1384f0, 0x3b)
    int thirdparty_1384f0_AIL_shutdown_0() {
        return 0;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_set_XMIDI_master_volume@8
    // __stdcall(volume 0..100): scale to 0..127 and push to the XMIDI driver.
    RVA(0x138950, 0x70)
    int __stdcall thirdparty_138950_AIL_set_XMIDI_master_volume_8(int volume) {
        int scaled;
        if (!g_ailMidiDriver) {
            return 0;
        }
        if (volume <= 0) {
            scaled = 0;
        } else if (volume >= 100) {
            scaled = 0x7f;
        } else {
            scaled = volume * 127 / 100;
        }
        AIL_set_XMIDI_master_volume(g_ailMidiDriver, scaled);
        return 1;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_XMIDI_master_volume@4
    // @stub
    RVA(0x1389c0, 0x47)
    int thirdparty_1389c0_AIL_XMIDI_master_volume_4() {
        return 0;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_allocate_sequence_handle@4;_AIL_init_sequence@12;_AIL_release_sequence_handle@4
    // @stub
    RVA(0x138c20, 0x122)
    int thirdparty_138c20_AIL_allocate_sequence_handle_4_AIL_init_sequence_12_AIL_() {
        return 0;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_release_sequence_handle@4
    // @stub
    RVA(0x138dd0, 0x36)
    int thirdparty_138dd0_AIL_release_sequence_handle_4() {
        return 0;
    }

    // AIL sequence player. The virtual at slot 8 (vtable +0x20) gates playback;
    // the sequence handle lives at m_58, loop/cursor state at m_44/m_48/m_4c.
    struct AilSeq {
        // vptr at +0x00 (compiler-managed). Eight leading virtuals put CanPlay at
        // vtable offset 0x20; only its slot is ever called here.
        virtual void v0();
        virtual void v1();
        virtual void v2();
        virtual void v3();
        virtual void v4();
        virtual void v5();
        virtual void v6();
        virtual void v7();
        virtual int CanPlay(); // slot 8 == vtable +0x20
        char m_pad4[0x44 - 4];
        int m_44; // +0x44
        int m_48; // +0x48
        int m_4c; // +0x4c
        char m_pad50[0x58 - 0x50];
        int m_58; // +0x58
        int Play(int cursor, int loop);
        int Resume(int restart);
        int SetLoop(int loop);
        int ResumeGate(); // the m_138f60 helper
    };

    // @source: thirdparty:_AIL_set_sequence_loop_count@8;_AIL_start_sequence@4
    // __thiscall(cursor, loop): start the sequence; if looping, clear loop count.
    RVA(0x138e10, 0x4a)
    int AilSeq::Play(int cursor, int loop) {
        if (!CanPlay()) {
            return 0;
        }
        m_4c = cursor;
        m_48 = loop;
        AIL_start_sequence(m_58);
        if (loop) {
            AIL_set_sequence_loop_count(m_58, 0);
        }
        m_44 = 0;
        return 1;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_end_sequence@4
    // @stub
    RVA(0x138e60, 0x26)
    int thirdparty_138e60_AIL_end_sequence_4() {
        return 0;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_stop_sequence@4
    // @stub
    RVA(0x138e90, 0x3a)
    int thirdparty_138e90_AIL_stop_sequence_4() {
        return 0;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_resume_sequence@4
    // __thiscall(restart): count down the resume delay and re-issue the resume.
    RVA(0x138ed0, 0x4f)
    int AilSeq::Resume(int restart) {
        if (!CanPlay()) {
            return 0;
        }
        if (ResumeGate()) {
            return 1;
        }
        if (m_44 > 0) {
            m_44--;
            if (restart) {
                m_44 = 0;
            }
            if (m_44 <= 0) {
                AIL_resume_sequence(m_58);
            }
        }
        return 1;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_set_sequence_tempo@12
    // @stub
    RVA(0x138f90, 0x32)
    int thirdparty_138f90_AIL_set_sequence_tempo_12() {
        return 0;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_set_sequence_volume@12
    // @stub
    RVA(0x138fd0, 0x5e)
    int thirdparty_138fd0_AIL_set_sequence_volume_12() {
        return 0;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_set_sequence_loop_count@8
    // __thiscall(loop): update the cached loop flag, re-arming the driver count.
    RVA(0x139030, 0x4c)
    int AilSeq::SetLoop(int loop) {
        if (!CanPlay()) {
            return 0;
        }
        if (m_48 != loop) {
            m_48 = loop;
            if (loop) {
                AIL_set_sequence_loop_count(m_58, 0);
            } else {
                AIL_set_sequence_loop_count(m_58, 1);
            }
        }
        return 1;
    }

    // @confidence: low
    // @source: winapi:DestroyWindow
    // @stub
    RVA(0x13d4c0, 0x1e)
    int winapi_13d4c0_DestroyWindow() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x13f460, 0x2da)
    int winapi_13f460_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:FindResourceA;LoadResource;LockResource
    // @stub
    RVA(0x144270, 0xd2)
    int winapi_144270_FindResourceA_LoadResource_LockResource() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:FindResourceA;LoadResource;LockResource
    // @stub
    RVA(0x1479e0, 0xbb)
    int winapi_1479e0_FindResourceA_LoadResource_LockResource() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x147f30, 0xbe)
    int winapi_147f30_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x147ff0, 0xa9)
    int winapi_147ff0_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x1480a0, 0x1a7)
    int winapi_1480a0_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CreateDCA;DeleteDC;GetSystemPaletteEntries
    // @stub
    RVA(0x1485b0, 0x162)
    int winapi_1485b0_CreateDCA_DeleteDC_GetSystemPaletteEntries() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x1538c0, 0x257)
    int winapi_1538c0_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x153b20, 0x270)
    int winapi_153b20_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x153d90, 0x259)
    int winapi_153d90_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x153ff0, 0x280)
    int winapi_153ff0_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x154270, 0x257)
    int winapi_154270_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x1544d0, 0x275)
    int winapi_1544d0_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x154750, 0x275)
    int winapi_154750_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x15cbe0, 0x46)
    int winapi_15cbe0_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // __thiscall(RECT*): cache the bounds rect + derived size/center, then recompute.
    struct GeoHost_161e80 {
        char m_pad0[0x50];
        RECT m_50; // +0x50 bounds
        char m_pad60[0x70 - 0x60];
        int m_70; // +0x70 width
        int m_74; // +0x74 height
        int m_78; // +0x78 half-width
        int m_7c; // +0x7c half-height
        void Build(RECT* pRect);
        void Recompute(); // RVA 0x161c90
    };
    // @source: winapi:CopyRect
    RVA(0x161e80, 0x79)
    void GeoHost_161e80::Build(RECT* pRect) {
        if (pRect->left != (LONG)0x80000000) {
            RECT local;
            CopyRect(&local, pRect);
            m_50 = local;
            int width = m_50.right - m_50.left + 1;
            int height = m_50.bottom - m_50.top + 1;
            m_70 = width;
            m_74 = height;
            m_78 = width / 2;
            m_7c = height / 2;
            Recompute();
        }
    }

    // @confidence: low
    // @source: winapi:DrawTextA;SetBkMode;SetTextColor
    // @stub
    RVA(0x164380, 0x98)
    int winapi_164380_DrawTextA_SetBkMode_SetTextColor() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:DrawTextA;SetBkMode;SetTextColor
    // @stub
    RVA(0x164420, 0x79)
    int winapi_164420_DrawTextA_SetBkMode_SetTextColor() {
        return 0;
    }

    // @confidence: low
    // @source: directx-wrapper-caller:calls 0x141dc0 (DirectDrawCreate;DirectDrawEnumerateA)
    // @stub
    RVA(0x1644a0, 0x19b)
    int directx_wrapper_caller_1644a0_DirectDrawCreate_DirectDrawEnumerateA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x168080, 0x1f6)
    int winapi_168080_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:InitializeCriticalSection
    // __cdecl thunk over InitializeCriticalSection.
    RVA(0x16c9c0, 0xc)
    void winapi_16c9c0_InitializeCriticalSection(CRITICAL_SECTION* cs) {
        InitializeCriticalSection(cs);
    }

    // @confidence: low
    // @source: winapi:DeleteCriticalSection
    // @stub
    RVA(0x16c9d0, 0xc)
    int winapi_16c9d0_DeleteCriticalSection() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EnterCriticalSection
    // @stub
    RVA(0x16c9e0, 0xc)
    int winapi_16c9e0_EnterCriticalSection() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:LeaveCriticalSection
    // __cdecl thunk over LeaveCriticalSection.
    RVA(0x16c9f0, 0xc)
    void winapi_16c9f0_LeaveCriticalSection(CRITICAL_SECTION* cs) {
        LeaveCriticalSection(cs);
    }

    // @confidence: low
    // @source: winapi:GetDC;ReleaseDC;SelectPalette
    // @stub
    RVA(0x174fe0, 0xfe)
    int winapi_174fe0_GetDC_ReleaseDC_SelectPalette() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDC;ReleaseDC;SelectPalette
    // @stub
    RVA(0x1750e0, 0x103)
    int winapi_1750e0_GetDC_ReleaseDC_SelectPalette() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDC;ReleaseDC;SelectPalette
    // @stub
    RVA(0x1751f0, 0xf9)
    int winapi_1751f0_GetDC_ReleaseDC_SelectPalette() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDC;ReleaseDC;SelectPalette
    // @stub
    RVA(0x1752f0, 0xfc)
    int winapi_1752f0_GetDC_ReleaseDC_SelectPalette() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDC;ReleaseDC;SelectPalette
    // @stub
    RVA(0x1753f0, 0xf4)
    int winapi_1753f0_GetDC_ReleaseDC_SelectPalette() {
        return 0;
    }

    // 0x1757c0 (CreateDIBSection) removed: already matched as CImage::DecodeBmpHeader in src/Image/Image.cpp.

    // @confidence: low
    // @source: winapi:DeleteObject
    // @stub
    RVA(0x175c90, 0x45)
    int winapi_175c90_DeleteObject() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CreatePalette
    // @stub
    RVA(0x176df0, 0x71)
    int winapi_176df0_CreatePalette() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:DeleteObject
    // __thiscall: delete the owned GDI object, then clear a far flag.
    struct DeleteObjHost_177070 {
        HGDIOBJ m_obj; // +0x00
        char m_pad[0x408 - 4];
        int m_408; // +0x408
        void Run();
    };
    RVA(0x177070, 0x22)
    void DeleteObjHost_177070::Run() {
        if (m_obj) {
            DeleteObject(m_obj);
            m_obj = 0;
        }
        m_408 = 0;
    }

    // @confidence: low
    // @source: winapi:CreateICA;DeleteDC;GetDeviceCaps
    // @stub
    RVA(0x1770a0, 0x3a)
    int winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CreateDCA;DeleteDC;GetSystemPaletteEntries
    // @stub
    RVA(0x1770e0, 0x7c)
    int winapi_1770e0_CreateDCA_DeleteDC_GetSystemPaletteEntries() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CreatePalette;DeleteObject;GetDC;RealizePalette;ReleaseDC
    // __cdecl: realize an all-black 256-entry palette on the screen DC to reset it.
    RVA(0x177160, 0x81)
    void winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD() {
        char buf[4 + 256 * sizeof(PALETTEENTRY)];
        LOGPALETTE* lp = (LOGPALETTE*)buf;
        HDC hdc = GetDC(0);
        lp->palVersion = 0x300;
        lp->palNumEntries = 256;
        for (int i = 0; i < 256; i++) {
            lp->palPalEntry[i].peRed = 0;
            lp->palPalEntry[i].peGreen = 0;
            lp->palPalEntry[i].peBlue = 0;
            lp->palPalEntry[i].peFlags = 4;
        }
        HPALETTE hpal = CreatePalette(lp);
        if (hpal) {
            HPALETTE old = SelectPalette(hdc, hpal, FALSE);
            RealizePalette(hdc);
            DeleteObject(SelectPalette(hdc, old, FALSE));
        }
        ReleaseDC(0, hdc);
    }

    // @confidence: low
    // @source: winapi:FindResourceA;LoadResource;LockResource
    // @stub
    RVA(0x1775f0, 0x62)
    int winapi_1775f0_FindResourceA_LoadResource_LockResource() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x178470, 0x11e)
    int winapi_178470_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x179e70, 0x5ec)
    int winapi_179e70_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: directx-wrapper-caller:calls 0x17c040 (DirectDrawCreate)
    // @stub
    RVA(0x17c2a0, 0x14e)
    int directx_wrapper_caller_17c2a0_DirectDrawCreate() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:ShowCursor
    // @stub
    RVA(0x17c3f0, 0x11f)
    int winapi_17c3f0_ShowCursor() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:ShowCursor
    // @stub
    RVA(0x17c510, 0x5e)
    int winapi_17c510_ShowCursor() {
        return 0;
    }

    // @confidence: low
    // @source: thirdparty:_SmackOpen@12;_SmackSoundUseDirectSound@4
    // @stub
    RVA(0x17c570, 0xc0)
    int thirdparty_17c570_SmackOpen_12_SmackSoundUseDirectSound_4() {
        return 0;
    }

    // @confidence: low
    // @source: thirdparty:_SmackOpen@12;_SmackSoundUseDirectSound@4
    // @stub
    RVA(0x17c630, 0xc0)
    int thirdparty_17c630_SmackOpen_12_SmackSoundUseDirectSound_4() {
        return 0;
    }

    // @confidence: low
    // @source: thirdparty,winapi:_SmackWait@4 | DispatchMessageA;PeekMessageA;TranslateMessage
    // @stub
    RVA(0x17c790, 0x14a)
    int thirdparty_winapi_17c790_SmackWait_4_DispatchMessageA_PeekMessageA_TranslateMessa() {
        return 0;
    }

    // @confidence: low
    // @source: thirdparty:_SmackGoto@8;_SmackWait@4
    // @stub
    RVA(0x17c8e0, 0xca)
    int thirdparty_17c8e0_SmackGoto_8_SmackWait_4() {
        return 0;
    }

    // @confidence: low
    // Smacker import (IAT) + the Rez allocator's free (RVA 0x1b9b82).
    extern "C" __declspec(dllimport) unsigned long __stdcall SmackClose(int smk);
    extern "C" void RezFree_call(void* p); // RVA 0x1b9b82 (cdecl)
    // The embedded sub-player whose Shutdown() lives at RVA 0x17b570.
    struct SmackSub_17c9b0 {
        void Shutdown(); // RVA 0x17b570
    };
    struct SmackHost_17c9b0 {
        char m_pad0[8];
        int m_8; // +0x08 active flag
        char m_pad0c[0x10 - 0xc];
        int m_10; // +0x10 Smacker handle
        char m_pad14[0x534 - 0x14];
        void* m_534; // +0x534 Rez buffer
        char m_pad538[0x540 - 0x538];
        SmackSub_17c9b0 m_540; // +0x540 sub-player
        int Close();
    };
    // @source: thirdparty:_SmackClose@4
    // __thiscall: shut the sub-player, close the Smacker stream, free buffers.
    RVA(0x17c9b0, 0x5b)
    int SmackHost_17c9b0::Close() {
        if (!m_8) {
            return 0;
        }
        m_540.Shutdown();
        if (!m_10) {
            return 0;
        }
        SmackClose(m_10);
        m_10 = 0;
        if (m_534) {
            RezFree_call(m_534);
            m_534 = 0;
        }
        m_8 = 0;
        return 1;
    }

    // @confidence: low
    // @source: thirdparty:_SmackDoFrame@4;_SmackNextFrame@4;_SmackToBuffer@28
    // @stub
    RVA(0x17caa0, 0x13b)
    int thirdparty_17caa0_SmackDoFrame_4_SmackNextFrame_4_SmackToBuffer_28() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDC;GetSystemPaletteEntries;ReleaseDC
    // @stub
    RVA(0x17cd90, 0x58)
    int winapi_17cd90_GetDC_GetSystemPaletteEntries_ReleaseDC() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetTickCount
    // @stub
    RVA(0x17e620, 0x13b)
    int winapi_17e620_GetTickCount() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x17fe00, 0x12d)
    int winapi_17fe00_timeGetTime() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PtInRect
    // @stub
    RVA(0x1804a0, 0x182)
    int winapi_1804a0_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x182ab0, 0x7b)
    int winapi_182ab0_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CreateDialogIndirectParamA;GetSystemMetrics;GlobalLock
    // @stub
    RVA(0x1ba677, 0x188)
    int winapi_1ba677_CreateDialogIndirectParamA_GetSystemMetrics_GlobalLock() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:DestroyWindow;GlobalFree;GlobalUnlock
    // @stub
    RVA(0x1ba819, 0x7c)
    int winapi_1ba819_DestroyWindow_GlobalFree_GlobalUnlock() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EnableWindow;FindResourceA;IsWindowEnabled;LoadResource;LockResource
    // @stub
    RVA(0x1ba9d2, 0x100)
    int winapi_1ba9d2_EnableWindow_FindResourceA_IsWindowEnabled_LoadResource_() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EnableWindow;GetActiveWindow;SetActiveWindow
    // @stub
    RVA(0x1baaef, 0x48)
    int winapi_1baaef_EnableWindow_GetActiveWindow_SetActiveWindow() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CallWindowProcA;GetPropA;RemovePropA;SetWindowLongA
    // @stub
    RVA(0x1bb31b, 0x111)
    int winapi_1bb31b_CallWindowProcA_GetPropA_RemovePropA_SetWindowLongA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetClassInfoA;RegisterClassA
    // @stub
    RVA(0x1bbff4, 0x93)
    int winapi_1bbff4_GetClassInfoA_RegisterClassA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:FreeLibrary
    // __thiscall: free the owned module handle if present.
    struct LibHost_1bf577 {
        HMODULE m_0; // +0x00
        void Run();
    };
    RVA(0x1bf577, 0xe)
    void LibHost_1bf577::Run() {
        if (m_0) {
            FreeLibrary(m_0);
        }
    }

    // @confidence: low
    // @source: winapi:GlobalFree
    // @stub
    RVA(0x1c09de, 0xe)
    int winapi_1c09de_GlobalFree() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:FreeLibrary
    // @stub
    RVA(0x1d4a18, 0x3c)
    int winapi_1d4a18_FreeLibrary() {
        return 0;
    }

} // namespace ApiCallerStubs
