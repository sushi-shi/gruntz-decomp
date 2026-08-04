#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/PaletteSize.h>
#include <Ints.h>
#include <Io/MoviePlayer.h>

#include <ddraw.h>

RVA(0x0017ca60, 0x35)
void CMoviePlayer::ResetPalette() {
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        m_palEntries[i].peRed = 0;
        m_palEntries[i].peGreen = 0;
        m_palEntries[i].peBlue = 0;
    }
    m_palette->SetEntries(0, 0, PALETTE_ENTRY_COUNT, m_palEntries);
}
