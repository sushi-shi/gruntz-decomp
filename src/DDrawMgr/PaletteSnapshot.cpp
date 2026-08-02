#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <Ints.h>
#include <Io/MoviePlayer.h>

#include <ddraw.h>

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
