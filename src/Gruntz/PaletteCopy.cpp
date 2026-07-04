// PaletteCopy.cpp - CDDScreen::UploadPalette (0x17ca10, homed as a declaration in
// CDDScreen.cpp, defined here): expand a 256-entry RGB palette (3 bytes/entry from
// m_10+0x6c) into the screen's 4-byte-per-entry slot array at +0x108, then push it
// to the held IDirectDrawPalette at +0x2c through its SetEntries slot (DDRAW COM).
#include <Ints.h>
#include <rva.h>

// The held DirectDraw palette interface: the canonical real-polymorphic COM
// interface (IDirectDrawPaletteZ, STDMETHOD SetEntries @slot 6 / +0x18) from the
// single-source DDraw header. `pal->SetEntries(...)` lowers to the same
// `mov eax,[pal]; call [eax+0x18]` the old manual `struct Vtbl` view emitted -
// self is pushed as the STDMETHOD (__stdcall) implicit first stack arg.
#include <DDrawMgr/CDirectDrawMgr.h>

// The screen object (CDDScreen). Only the palette-upload offsets are pinned here;
// the full layout lives in CDDScreen.cpp (which declares UploadPalette + BlitRegion).
struct CDDScreen {
    char m_pad0[0x10];
    u8* m_10; // +0x10  RGB source base (frame palette entries at +0x6c)
    char m_pad14[0x2c - 0x14];
    IDirectDrawPaletteZ* m_palette; // +0x2c  held DirectDraw palette
    char m_pad30[0x108 - 0x30];
    u8 m_slots[0x400]; // +0x108  256 * 4-byte PALETTEENTRY slots

    void UploadPalette(); // 0x17ca10
};

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
void CDDScreen::UploadPalette() {
    u8* src = m_10 + 0x6c;
    u8* dst = m_slots;
    int n = 0x100;
    do {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst += 4;
        src += 3;
    } while (--n);
    m_palette->SetEntries(0, 0, 0x100, m_slots);
}
