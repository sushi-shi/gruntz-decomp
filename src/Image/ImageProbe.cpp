#include <Mfc.h>
#include <ddraw.h>
#include <smack.h>
#undef u8
#undef u16
#undef u32
#undef u64
#undef s8
#undef s16
#undef s32
#undef s64
#include <Io/MoviePlayer.h>
#include <Ints.h>
#include <rva.h>
#include <ComOutRef.h>
#include <string.h>

RVA(0x0017cbe0, 0x97)
i32 CMoviePlayer::CheckGrid() {
    memset(&m_srcDesc, 0, 0x6c);
    m_srcDesc.dwSize = 0x6c;
    m_srcDesc.dwFlags = 7;
    m_srcDesc.ddsCaps.dwCaps = 0x840;
    m_srcDesc.dwHeight = m_smackHandle->Height;
    m_srcDesc.dwWidth = m_smackHandle->Width;
    if (m_dd2->CreateSurface(&m_srcDesc, &m_srcSurfRaw, 0) != 0) {
        return 0;
    }
    ComOutRef<IDirectDrawSurface> srcOut;
    srcOut.m_asTyped = &m_srcSurf;
    if (m_srcSurfRaw->QueryInterface(IID_IDirectDrawSurface3, srcOut.m_asVoid) != 0) {
        return 0;
    }
    if (m_bpp == 8) {
        m_srcSurf->SetPalette(m_palette);
    }
    return 1;
}
