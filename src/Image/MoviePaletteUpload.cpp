#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/DirectDrawMgr.h>
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
#include <DDrawMgr/PaletteSize.h>

RVA(0x0017ca10, 0x49)
void CMoviePlayer::UploadPalette() {
    u8* src = m_smackHandle->Palette;
    u8* p = &m_palEntries[0].peGreen;
    i32 n = 0x100;
    do {
        p[-1] = *src++;
        p[0] = *src++;
        p[1] = *src++;
        p += 4;
        --n;
    } while (n != 0);
    m_palette->SetEntries(0, 0, PALETTE_ENTRY_COUNT, m_palEntries);
}
