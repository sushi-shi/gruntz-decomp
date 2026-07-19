// PaletteReset.cpp - CMoviePlayer::ResetPalette (0x17ca60): clear the inline
// 256-entry (4-byte stride) color table at +0x108 - leaving the 4th byte of each
// slot - then push it to the surface/palette interface held at +0x2c via its
// vtable slot 6. Owner confirmed by xref: the sole caller is CMoviePlayer::HandleError
// (0x17cc80), reached on `this` (the +0x2c palette sink is CDDScreen's m_palette).
// CDDScreen is defined fully in src/Gruntz/DDScreen.cpp; this TU carries only the
// offset-compatible partial layout ResetPalette touches, so the definition mangles
// to ?ResetPalette@CDDScreen@@ and pairs with the caller's named reference.
#include <Ints.h>
#include <rva.h>

// The +0x2c palette sink is the real IDirectDrawPalette (SetEntries @slot 6 / +0x18)
// from <ddraw.h>: `pal->SetEntries(...)` lowers to the same `mov ecx,[pal]; push pal;
// call [ecx+0x18]` COM dispatch the old hand-rolled IPalSink view emitted. The full
// CDDScreen layout is the shared canonical (<DDrawMgr/DDScreen.h>).
#include <DDrawMgr/DirectDrawMgr.h>
#include <Mfc.h> // afx.h FIRST (before ddraw.h's windows.h): <Io/MoviePlayer.h> below is
                 // the real (MFC-bearing) class this TU's method belongs to.
#include <ddraw.h>
#include <Io/MoviePlayer.h> // THE class (CDDScreen is a typedef alias of it)

RVA(0x0017ca60, 0x35)
void CMoviePlayer::ResetPalette() {
    for (i32 i = 0; i < 256; i++) {
        m_colorSlots[i * 4 + 0] = 0;
        m_colorSlots[i * 4 + 1] = 0;
        m_colorSlots[i * 4 + 2] = 0;
    }
    m_palette->SetEntries(0, 0, 0x100, reinterpret_cast<LPPALETTEENTRY>(m_colorSlots));
}
