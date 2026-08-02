#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <Ints.h>
#include <Io/MoviePlayer.h>

#include <ddraw.h>

RVA(0x0017ca60, 0x35)
void CMoviePlayer::ResetPalette() {
    for (i32 i = 0; i < 256; i++) {
        m_palEntries[i].peRed = 0;
        m_palEntries[i].peGreen = 0;
        m_palEntries[i].peBlue = 0;
    }
    m_palette->SetEntries(0, 0, 0x100, m_palEntries);
}
