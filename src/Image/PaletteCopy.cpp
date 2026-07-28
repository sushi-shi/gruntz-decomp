#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/DirectDrawMgr.h>
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
#include <Io/MoviePlayer.h> // THE class (CMoviePlayer is a typedef alias of it) // canonical CMoviePlayer (shared layout)

// ---------------------------------------------------------------------------
// 0x17ca10 - copy RGB triples into the slot array (alpha byte left untouched),
// then hand the slot array to the DirectDraw palette.
// ---------------------------------------------------------------------------
RVA(0x0017ca10, 0x49)
void CMoviePlayer::UploadPalette() {
    u8* src = m_smackHandle->Palette; // the real SDK field (was an offset-cast at +0x6c)
    u8* p =
        &m_palEntries[0].peGreen; // green-anchored cursor: p[-1]/p[0]/p[1] = R/G/B, peFlags skipped
    i32 n = 0x100;
    do {
        p[-1] = *src++;
        p[0] = *src++;
        p[1] = *src++;
        p += 4;
        --n;
    } while (n != 0);
    m_palette->SetEntries(0, 0, 0x100, m_palEntries);
}
