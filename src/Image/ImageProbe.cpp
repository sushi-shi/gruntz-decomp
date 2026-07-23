#include <Mfc.h> // afx.h FIRST (before ddraw.h's windows.h): <Io/MoviePlayer.h> below is
#include <ddraw.h>
#include <smack.h> // the genuine RAD Smacker SDK - the real Smack handle (m_smackHandle):
#undef u8
#undef u16
#undef u32
#undef u64
#undef s8
#undef s16
#undef s32
#undef s64
#include <Io/MoviePlayer.h> // THE class (CMoviePlayer is a typedef alias of it)
#include <Ints.h>
#include <rva.h>
#include <string.h> // inline memset (rep stos) for the descriptor zero

RVA(0x0017cbe0, 0x97)
i32 CMoviePlayer::CheckGrid() {
    memset(&m_srcDesc, 0, 0x6c);
    m_srcDesc.dwSize = 0x6c;
    m_srcDesc.dwFlags = 7;            // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH
    m_srcDesc.ddsCaps.dwCaps = 0x840; // DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY
    m_srcDesc.dwHeight = m_smackHandle->Height;
    m_srcDesc.dwWidth = m_smackHandle->Width;
    if (m_dd2->CreateSurface(&m_srcDesc, &m_srcSurfRaw, 0) != 0) {
        return 0;
    }
    if (m_srcSurfRaw->QueryInterface(IID_IDirectDrawSurface3, reinterpret_cast<void**>(&m_srcSurf))
        != 0) {
        return 0;
    }
    if (m_bpp == 8) {
        m_srcSurf->SetPalette(m_palette);
    }
    return 1;
}
