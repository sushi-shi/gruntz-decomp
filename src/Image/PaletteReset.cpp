// PaletteReset.cpp - CSurfacePalette::ResetPalette (0x17ca60): clear the inline
// 256-entry (4-byte stride) color table at +0x108 - leaving the 4th byte of each
// slot - then push it to the surface/palette interface held at +0x2c via its
// vtable slot 6. Best-guess owner; the virtual dispatch is reloc-independent.
#include <Ints.h>
#include <rva.h>

struct IPalSink {
    struct Vtbl {
        char m_pad0[0x18];
        i32(__stdcall* SetEntries)(IPalSink*, i32, i32, i32, void*); // +0x18 (slot 6)
    };
    Vtbl* m_vptr;
};

struct CSurfacePalette {
    char m_pad0[0x2c];
    IPalSink* m_paletteSink; // +0x2c
    char m_pad30[0x108 - 0x30];
    u8 m_colorSlots[0x400]; // +0x108  256 * 4-byte color slots

    void ResetPalette(); // 0x17ca60
};

RVA(0x0017ca60, 0x35)
void CSurfacePalette::ResetPalette() {
    for (i32 i = 0; i < 256; i++) {
        m_colorSlots[i * 4 + 0] = 0;
        m_colorSlots[i * 4 + 1] = 0;
        m_colorSlots[i * 4 + 2] = 0;
    }
    m_paletteSink->m_vptr->SetEntries(m_paletteSink, 0, 0, 0x100, m_colorSlots);
}

SIZE_UNKNOWN(IPalSink);
