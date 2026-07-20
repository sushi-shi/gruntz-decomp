#include <Ints.h>
#include <rva.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <Mfc.h> // afx.h FIRST (before ddraw.h's windows.h): <Io/MoviePlayer.h> below is
#include <ddraw.h>
#include <Io/MoviePlayer.h> // THE class (CDDScreen is a typedef alias of it)

RVA(0x0017cd90, 0x58)
void CMoviePlayer::Snapshot(HWND hWnd) {
    HDC hdc = GetDC(hWnd);
    GetSystemPaletteEntries(hdc, 0, 0x100, m_palEntries);
    for (i32 i = 0; i < 0x100; i++) {
        m_palEntries[i].peRed = 0;
        m_palEntries[i].peBlue = 0;
        m_palEntries[i].peGreen = 0;
        m_palEntries[i].peFlags = 4;
    }
    ReleaseDC(hWnd, hdc);
}
