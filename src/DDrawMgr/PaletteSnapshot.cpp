// PaletteSnapshot.cpp - CDDScreen::Snapshot (0x17cd90): read the current system
// palette into the inline 256-entry (4-byte stride) color table at +0x108, then
// blank every entry to a reserved black (peFlags = PC_RESERVED = 4) so a fresh
// remap can be rebuilt against it. __thiscall(hWnd), ret 4.
//
// Owner recovered by xref (was ResLoaders::PalCache_17cd90::Snapshot, split out of
// the ResourceLoaders.cpp holding TU): the callers are CDDPageMgr::Init (0x17c040)
// and CDDScreen::InitMode (0x17c3f0), and this body is RVA-embedded in the DDPageMgr
// obj between CDDScreen::HandleError (0x17cc80) and CDDScreen::BlitRegion (0x17cdf0) -
// so it is a CDDScreen method. The former PalCache view is dissolved onto the
// canonical <DDrawMgr/DDScreen.h> CDDScreen (+0x108 IS CDDScreen::m_colorSlots, the
// same 256*4 table ResetPalette clears). Kept a stray here per the existing
// PaletteReset.cpp / PaletteCopy.cpp CDDScreen-palette precedent; SHOULD ultimately
// fold into DDPageMgr.cpp with the other palette strays (see that file's BOUNDARY note).
#include <Ints.h>
#include <rva.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <Win32.h> // GetDC / GetSystemPaletteEntries / ReleaseDC + PALETTEENTRY
#include <ddraw.h> // real IDirectDraw* (DDScreen.h layout dependency)
#include <DDrawMgr/DDScreen.h>

RVA(0x0017cd90, 0x58)
void CDDScreen::Snapshot(HWND hWnd) {
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
