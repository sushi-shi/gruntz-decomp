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
#include <Io/MoviePlayer.h> // THE class (CDDScreen is a typedef alias of it) // canonical CDDScreen (shared layout)

// ---------------------------------------------------------------------------
// 0x17ca10 - copy RGB triples into the slot array (alpha byte left untouched),
// then hand the slot array to the DirectDraw palette.
// ---------------------------------------------------------------------------
// @early-stop
// scheduling+regalloc wall (61%): the RGB->4-byte expand loop and the +0x2c palette
// SetEntries __stdcall upload are byte-faithful in operation, but retail keeps the
// running dst in edx (started at this+0x109, advanced mid-iteration) and recomputes
// this+0x108 at the end, while cl pins the dst base in ebp (extra push/pop) and
// advances src by 3 up-front instead of inc-per-byte. A loop-scheduling + base-
// register coin-flip; not source-steerable. Logic 100% correct; deferred.
RVA(0x0017ca10, 0x49)
void CMoviePlayer::UploadPalette() {
    u8* src = m_smackHandle->Palette; // the real SDK field (was an offset-cast at +0x6c)
    u8* dst = m_colorSlots;
    int n = 0x100;
    do {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst += 4;
        src += 3;
    } while (--n);
    m_palette->SetEntries(0, 0, 0x100, reinterpret_cast<LPPALETTEENTRY>(m_colorSlots));
}
