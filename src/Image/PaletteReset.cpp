// PaletteReset.cpp - CDDScreen::ResetPalette (0x17ca60): clear the inline
// 256-entry (4-byte stride) color table at +0x108 - leaving the 4th byte of each
// slot - then push it to the surface/palette interface held at +0x2c via its
// vtable slot 6. Owner confirmed by xref: the sole caller is CDDScreen::HandleError
// (0x17cc80), reached on `this` (the +0x2c palette sink is CDDScreen's m_palette).
// CDDScreen is defined fully in src/Gruntz/DDScreen.cpp; this TU carries only the
// offset-compatible partial layout ResetPalette touches, so the definition mangles
// to ?ResetPalette@CDDScreen@@ and pairs with the caller's named reference.
#include <Ints.h>
#include <rva.h>

// The surface/palette interface (a DirectDrawPalette-shaped COM object). Modeled
// REAL-POLYMORPHIC with __stdcall virtuals in slot order so the retail
// `mov ecx,[obj]; push obj; call [ecx+0x18]` COM dispatch falls out of the
// language; the interface is never constructed here, so cl emits no ??_7.
struct IPalSink {
    virtual i32 __stdcall QueryInterface(const void* riid, void** out);           // slot 0 +0x00
    virtual u32 __stdcall AddRef();                                               // slot 1 +0x04
    virtual u32 __stdcall Release();                                              // slot 2 +0x08
    virtual i32 __stdcall GetCaps(void* caps);                                    // slot 3 +0x0c
    virtual i32 __stdcall GetEntries(u32 flags, u32 start, u32 count, void* out); // slot 4 +0x10
    virtual i32 __stdcall Initialize(void* dd, u32 flags, void* entries);         // slot 5 +0x14
    virtual i32 __stdcall
    SetEntries(i32 flags, i32 start, i32 count, void* entries); // slot 6 +0x18
};

// Partial view of CDDScreen (full class in src/Gruntz/DDScreen.cpp) - only the
// offset-compatible fields ResetPalette touches. m_palette (the palette COM sink)
// is CDDScreen's +0x2c IDDObj*, viewed here as the DirectDrawPalette IPalSink.
struct CDDScreen {
    char m_pad0[0x2c];
    IPalSink* m_palette; // +0x2c
    char m_pad30[0x108 - 0x30];
    u8 m_colorSlots[0x400]; // +0x108  256 * 4-byte color slots

    void ResetPalette(); // 0x17ca60
};

RVA(0x0017ca60, 0x35)
void CDDScreen::ResetPalette() {
    for (i32 i = 0; i < 256; i++) {
        m_colorSlots[i * 4 + 0] = 0;
        m_colorSlots[i * 4 + 1] = 0;
        m_colorSlots[i * 4 + 2] = 0;
    }
    m_palette->SetEntries(0, 0, 0x100, m_colorSlots);
}

SIZE_UNKNOWN(IPalSink);
