// CDDScreen.cpp - three methods of the big tiled-DirectDraw display manager
// (the CDDPageMgr bring-up class behind 0x17c040; members run out to ~0x86a4).
// Homed together because 0x17cc80/0x17cdf0/0x17cfc0 are adjacent methods of the
// same object. Field names are placeholders; only offsets + emitted bytes are
// load-bearing (campaign doctrine).
//
//   HandleError (0x17cc80) - release the owned surfaces/DDraw interfaces and, when
//       still partway up, black the primary surface (ROP-blackness blit, falling
//       back to a COLORFILL blit) before tearing the rest down.
//   BlitRegion  (0x17cdf0) - BltFast/Blt a source rect to a dest rect, handling
//       DDERR_SURFACELOST by restoring + retrying.
//   Configure   (0x17cfc0) - compute the tile grid / scroll origin for a mode.
#include <Ints.h>
#include <rva.h>

extern "C" void* memset(void* d, i32 c, u32 n); // inlined to rep stos

// A COM object (surface / IDirectDraw2 / palette). One vtable view covers every
// slot these three methods reach; members only touching Release just use slot 2.
struct IDDObj;
struct IDDVtbl {
    void* s00;
    void* s04;
    u32(__stdcall* Release)(IDDObj*); // +0x08 (IUnknown slot 2)
    void* s0c;
    void* s10;
    i32(__stdcall* Blt)(IDDObj*, void* dst, IDDObj* src, void* srcRect, u32 flags,
                        void* fx); // +0x14 (surface slot 5)
    void* s18;
    i32(__stdcall* BltFast)(IDDObj*, i32 x, i32 y, IDDObj* src, void* srcRect,
                            u32 trans); // +0x1c (surface slot 7)
    void* s20[11];                     // +0x20..+0x4b
    i32(__stdcall* RestoreDisplayMode)(IDDObj*); // +0x4c (IDirectDraw slot 19)
    void* s50[4];                                // +0x50..+0x5f
    i32(__stdcall* IsLost)(IDDObj*);             // +0x60 (surface slot 24)
    void* s64;
    void* s68;
    i32(__stdcall* Restore)(IDDObj*); // +0x6c (surface slot 27)
    void* s70;
    void* s74;
    void* s78;
    i32(__stdcall* SetPalette)(IDDObj*, IDDObj* pal); // +0x7c (surface slot 31)
};
struct IDDObj {
    IDDVtbl* vtbl;
};

// DDBLTFX (0x64 bytes): only dwSize@0x00, dwROP@0x08, dwFillColor@0x50 are set.
struct DDBLTFX_ {
    u32 dwSize;        // 0x00
    u32 pad04;         // 0x04 dwDDFX
    u32 dwROP;         // 0x08
    u32 pad0c[17];     // 0x0c..0x4f
    u32 dwFillColor;   // 0x50
    u32 pad54[4];      // 0x54..0x63
};

// 0x17ca60 lives in another TU as CSurfacePalette::ResetPalette; reference it by
// its real mangled name so the rel32 call is named (not just reloc-masked).
class CSurfacePalette {
public:
    void ResetPalette(); // 0x17ca60
};

class CDDScreen {
public:
    void HandleError(); // 0x17cc80

    char m_pad00[0x0c];
    i32 m_0c; // +0x0c
    char m_pad10[0x14 - 0x10];
    IDDObj* m_14; // +0x14  IDirectDraw2
    IDDObj* m_18; // +0x18  IDirectDraw
    IDDObj* m_1c; // +0x1c  primary surface
    IDDObj* m_20; // +0x20  primary surface (raw)
    IDDObj* m_24; // +0x24  surface
    IDDObj* m_28; // +0x28  surface
    IDDObj* m_2c; // +0x2c  palette
    char m_pad30[0x520 - 0x30];
    i32 m_520; // +0x520  bpp
};

// ===========================================================================
// 0x17cc80 - HandleError: release owned interfaces; if still mid-bringup black
// the primary surface, then release the remaining objects.
// ===========================================================================
RVA(0x0017cc80, 0x109)
void CDDScreen::HandleError() {
    if (m_24) {
        m_24->vtbl->Release(m_24);
        m_24 = 0;
    }
    if (m_28) {
        m_28->vtbl->Release(m_28);
        m_28 = 0;
    }
    if (m_520 == 8) {
        ((CSurfacePalette*)this)->ResetPalette();
    }
    if (m_1c) {
        DDBLTFX_ fx;
        memset(&fx, 0, sizeof(fx));
        fx.dwSize = 0x64;
        fx.dwROP = 0x42;
        void* rc = (void*)m_1c->vtbl->Blt(m_1c, 0, 0, 0, 0x1020000, &fx);
        if (rc) {
            memset(&fx, 0, sizeof(fx));
            fx.dwSize = 0x64;
            fx.dwFillColor = 0;
            m_1c->vtbl->Blt(m_1c, 0, 0, 0, 0x1000400, &fx);
        }
    }
    if (m_0c == 0) {
        if (m_2c) {
            m_2c->vtbl->Release(m_2c);
            m_2c = 0;
        }
        if (m_1c) {
            m_1c->vtbl->Release(m_1c);
            m_1c = 0;
        }
        if (m_20) {
            m_20->vtbl->Release(m_20);
            m_20 = 0;
        }
        if (m_14) {
            m_14->vtbl->RestoreDisplayMode(m_14);
            m_14->vtbl->Release(m_14);
            m_14 = 0;
        }
        if (m_18) {
            m_18->vtbl->Release(m_18);
            m_18 = 0;
        }
    }
}
