// ImageProbe.cpp - CMoviePlayer::CheckGrid (0x17cbe0): (re)create the tiled-display
// SOURCE surface. Fills the +0x9c DDSURFACEDESC (dwSize 0x6c, dwFlags 7 =
// DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH, ddsCaps 0x840 = DDSCAPS_OFFSCREENPLAIN|
// DDSCAPS_SYSTEMMEMORY) from the m_tileInfo tile dims, CreateSurface's it on the
// held IDirectDraw2 (+0x14) into the raw +0x28 slot, QIs IDirectDrawSurface3 into
// the +0x24 blit source, and for the 8bpp mode SetPalette's the held palette onto
// it. Returns 0 on any COM failure, else 1.
//
// IDENTITY (ex "CImageProbe" fake-view TU): every offset lands on the canonical
// CDDScreen (<DDrawMgr/DDScreen.h>) - +0x10 m_tileInfo (the "info" +0x4/+0x8 dims
// ARE CTileInfo::m_width/m_height), +0x14 m_dd2, +0x24/+0x28 the source-surface
// pair HandleError already Releases in QI-then-raw order, +0x2c m_palette,
// +0x520 m_bpp - and its ONLY caller is CMoviePlayer::Configure (0x17cfc0), which
// declares it as this class's CheckGrid. The "provider slot 6" was
// IDirectDraw::CreateSurface, "source slot 0" QueryInterface, "payload slot 31"
// (+0x7c) IDirectDrawSurface3::SetPalette - the real DX6 COM vtables, not a
// private interface; the three hand-rolled 30+-slot filler views are gone.
#include <Mfc.h> // afx.h FIRST (before ddraw.h's windows.h): <Io/MoviePlayer.h> below is
                 // the real (MFC-bearing) class this TU's method belongs to.
#include <ddraw.h>
#include <smack.h> // the genuine RAD Smacker SDK - the real Smack handle (m_smackHandle):
                   // Width/+0x4, Height/+0x8, NewPalette/+0x68, Palette[772]/+0x6c
// smack.h pulls rad.h, whose u8/u16/u32/... object-like macros shadow <Ints.h>; undo
// them (matching-neutral: rad's u32==unsigned long is the same 4 bytes).
#undef u8
#undef u16
#undef u32
#undef u64
#undef s8
#undef s16
#undef s32
#undef s64
#include <Io/MoviePlayer.h> // THE class (CDDScreen is a typedef alias of it)
#include <Ints.h>
#include <rva.h>
#include <string.h> // inline memset (rep stos) for the descriptor zero

// (IID_IDirectDrawSurface3 comes from <ddraw.h>; the retail .rdata datum 0x1ef888
// is DATA-pinned in ddpagemgr - the `push OFFSET` reloc-masks here.)

// 0x17cbe0: build the source-surface descriptor, create + QI the tile source
// surface, and hand the 8bpp palette to it.
RVA(0x0017cbe0, 0x97)
i32 CMoviePlayer::CheckGrid() {
    memset(&m_srcDesc, 0, 0x6c);
    m_srcDesc.dwSize = 0x6c;
    m_srcDesc.dwFlags = 7;            // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH
    m_srcDesc.ddsCaps.dwCaps = 0x840; // DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY
    m_srcDesc.dwHeight = m_smackHandle->Height;
    m_srcDesc.dwWidth = m_smackHandle->Width;
    if (m_dd2->CreateSurface(&m_srcDesc, &m_28, 0) != 0) {
        return 0;
    }
    if (m_28->QueryInterface(IID_IDirectDrawSurface3, reinterpret_cast<void**>(&m_srcSurf)) != 0) {
        return 0;
    }
    if (m_bpp == 8) {
        m_srcSurf->SetPalette(m_palette);
    }
    return 1;
}
