// CDDScreen.cpp - three methods of the big tiled-DirectDraw display manager
// (the CDDPageMgr bring-up class behind 0x17c040; members run out to ~0x86a4).
// Homed together because 0x17cc80/0x17cdf0/0x17cfc0 are adjacent methods of the
// same object. Field names recovered from usage; offsets + emitted bytes are
// load-bearing. A few fields only touched once (m_20/m_28/m_50c/m_514/m_86a0)
// keep their offset names - role unproven from these three methods.
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
    i32(__stdcall* Blt)(
        IDDObj*,
        void* dst,
        IDDObj* src,
        void* srcRect,
        u32 flags,
        void* fx
    ); // +0x14 (surface slot 5)
    void* s18;
    i32(__stdcall* BltFast)(
        IDDObj*,
        i32 x,
        i32 y,
        IDDObj* src,
        void* srcRect,
        u32 trans
    );                                           // +0x1c (surface slot 7)
    void* s20[11];                               // +0x20..+0x4b
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
    u32 dwSize;      // 0x00
    u32 pad04;       // 0x04 dwDDFX
    u32 dwROP;       // 0x08
    u32 pad0c[17];   // 0x0c..0x4f
    u32 dwFillColor; // 0x50
    u32 pad54[4];    // 0x54..0x63
};

// 0x17ca60 lives in another TU as CSurfacePalette::ResetPalette; reference it by
// its real mangled name so the rel32 call is named (not just reloc-masked).
class CSurfacePalette {
public:
    void ResetPalette(); // 0x17ca60
};

// The engine heap allocator (NAFXCW operator new replacement) - 16-byte RECT
// nodes Configure allocates for the explicit-blit case. _RezAlloc (named, rel32).
extern "C" void* RezAlloc(u32 size); // 0x1b9b46

// A blit rectangle (left/top/right/bottom). m_destRect points at one of these.
struct DDRect {
    i32 left;   // +0x0
    i32 top;    // +0x4
    i32 right;  // +0x8
    i32 bottom; // +0xc
};

// The origin point Configure's caller may pass (validated against the screen
// extents - unsigned, so the compares lower to `ja`).
struct DDPoint {
    u32 x; // +0x0
    u32 y; // +0x4
};

// The tile-size descriptor m_tileInfo points at (only the tile width/height are read;
// unsigned so the validation/divide lower to `ja`/`div`).
struct CTileInfo {
    i32 m_0;      // +0x0
    u32 m_width;  // +0x4  tile width
    u32 m_height; // +0x8  tile height
};

class CDDScreen {
public:
    void HandleError();                                                // 0x17cc80
    i32 BlitRegion(i32 col, i32 row, i32 nCols, i32 nRows);            // 0x17cdf0
    i32 Configure(i32 mode, i32 flags, DDPoint* origin, DDRect* rect); // 0x17cfc0
    i32 CheckGrid();      // 0x17cbe0 (sibling, external)
    void UploadPalette(); // 0x17ca10 (palette re-realize on 8bpp restore; body in PaletteCopy.cpp)

    char m_pad00[0x0c];
    i32 m_0c; // +0x0c   ==0 gates full DDraw-stack teardown (owns-vs-borrows, unproven)
    CTileInfo* m_tileInfo; // +0x10
    IDDObj* m_dd2;         // +0x14   IDirectDraw2
    IDDObj* m_dd;          // +0x18   IDirectDraw
    IDDObj* m_primary;     // +0x1c   primary surface
    IDDObj* m_20;          // +0x20   surface (only Release'd here; role unproven)
    IDDObj* m_srcSurf;     // +0x24   blit source surface
    IDDObj* m_28;          // +0x28   surface (only Release'd here; role unproven)
    IDDObj* m_palette;     // +0x2c
    char m_pad30[0x50c - 0x30];
    i32 m_50c; // +0x50c   reset to 0 by Configure
    char m_pad510[0x514 - 0x510];
    i32 m_514;            // +0x514  set in mode-2 fallback (unproven)
    u32 m_screenWidth;    // +0x518
    u32 m_screenHeight;   // +0x51c
    i32 m_bpp;            // +0x520
    i32 m_tilesAcross;    // +0x524
    i32 m_tilesDown;      // +0x528
    i32 m_originX;        // +0x52c
    i32 m_originY;        // +0x530
    DDRect* m_destRect;   // +0x534  explicit dest rect (or 0)
    i32 m_forceSingleRow; // +0x538
    char m_pad53c[0x86a0 - 0x53c];
    i32 m_86a0; // +0x86a0   reset to 0 by Configure
};

// ===========================================================================
// 0x17cc80 - HandleError: release owned interfaces; if still mid-bringup black
// the primary surface, then release the remaining objects.
// ===========================================================================
RVA(0x0017cc80, 0x109)
void CDDScreen::HandleError() {
    if (m_srcSurf) {
        m_srcSurf->vtbl->Release(m_srcSurf);
        m_srcSurf = 0;
    }
    if (m_28) {
        m_28->vtbl->Release(m_28);
        m_28 = 0;
    }
    if (m_bpp == 8) {
        ((CSurfacePalette*)this)->ResetPalette();
    }
    if (m_primary) {
        DDBLTFX_ fx;
        memset(&fx, 0, sizeof(fx));
        fx.dwSize = 0x64;
        fx.dwROP = 0x42;
        void* rc = (void*)m_primary->vtbl->Blt(m_primary, 0, 0, 0, 0x1020000, &fx);
        if (rc) {
            memset(&fx, 0, sizeof(fx));
            fx.dwSize = 0x64;
            fx.dwFillColor = 0;
            m_primary->vtbl->Blt(m_primary, 0, 0, 0, 0x1000400, &fx);
        }
    }
    if (m_0c == 0) {
        if (m_palette) {
            m_palette->vtbl->Release(m_palette);
            m_palette = 0;
        }
        if (m_primary) {
            m_primary->vtbl->Release(m_primary);
            m_primary = 0;
        }
        if (m_20) {
            m_20->vtbl->Release(m_20);
            m_20 = 0;
        }
        if (m_dd2) {
            m_dd2->vtbl->RestoreDisplayMode(m_dd2);
            m_dd2->vtbl->Release(m_dd2);
            m_dd2 = 0;
        }
        if (m_dd) {
            m_dd->vtbl->Release(m_dd);
            m_dd = 0;
        }
    }
}

// ===========================================================================
// 0x17cdf0 - BlitRegion: blit the (col,row,nCols,nRows) tile region from the
// source surface (m_srcSurf) onto the primary (m_primary). Single 1x1 untiled grids use
// BltFast; everything else (or an explicit dest rect at m_destRect) uses Blt. On
// DDERR_SURFACELOST restore the lost surface(s) (re-attaching the palette on the
// 8bpp primary) and retry; any other HRESULT is returned.
// ===========================================================================
// @early-stop
// instruction-scheduling wall (~90.8%): everything outside the dest-rect setup is
// byte-exact (incl. the BltFast/Blt vtable dispatch, the SURFACELOST restore-retry
// loop and the cross-jumped 8bpp SetPalette tail). In the m_destRect==0 branch retail
// keeps m_tilesAcross in eax / m_tilesDown in ecx across the left/top stores and computes
// nCols*m_tilesAcross / nRows*m_tilesDown lazily afterwards; cl groups the two products of each
// shared operand and hoists them early (folding nCols as a memory operand). A pure
// MSVC5 /O2 scheduler coin-flip with no source lever (tile-dim locals regress it to
// 90.2%); deferred to the final sweep.
RVA(0x0017cdf0, 0x1c6)
i32 CDDScreen::BlitRegion(i32 col, i32 row, i32 nCols, i32 nRows) {
    DDRect dst, src;
    if (m_destRect) {
        dst.left = m_destRect->left;
        dst.top = m_destRect->top;
        dst.right = m_destRect->right;
        dst.bottom = m_destRect->bottom;
    } else {
        dst.left = col * m_tilesAcross + m_originX;
        dst.top = row * m_tilesDown + m_originY;
        dst.right = nCols * m_tilesAcross + dst.left;
        dst.bottom = nRows * m_tilesDown + dst.top;
    }
    src.left = col;
    src.top = row;
    src.right = col + nCols;
    src.bottom = row + nRows;

    for (;;) {
        i32 hr;
        if (m_tilesAcross == 1 && m_tilesDown == 1 && m_destRect == 0) {
            hr = m_primary->vtbl->BltFast(m_primary, dst.left, dst.top, m_srcSurf, &src, 0x10);
            if (hr != 0x887601c2) {
                return hr;
            }
            if (m_primary->vtbl->IsLost(m_primary) == 0x887601c2
                && m_primary->vtbl->Restore(m_primary) == 0) {
                if (m_bpp == 8) {
                    m_primary->vtbl->SetPalette(m_primary, m_palette);
                    UploadPalette();
                }
            } else {
                hr = m_srcSurf->vtbl->IsLost(m_srcSurf);
                if (hr != 0x887601c2) {
                    return hr;
                }
                hr = m_srcSurf->vtbl->Restore(m_srcSurf);
                if (hr != 0) {
                    return hr;
                }
            }
        } else {
            hr = m_primary->vtbl->Blt(m_primary, &dst, m_srcSurf, &src, 0x1000000, 0);
            if (hr != 0x887601c2) {
                return hr;
            }
            if (m_primary->vtbl->IsLost(m_primary) == 0x887601c2
                && m_primary->vtbl->Restore(m_primary) == 0) {
                if (m_bpp == 8) {
                    m_primary->vtbl->SetPalette(m_primary, m_palette);
                    UploadPalette();
                }
            } else {
                hr = m_srcSurf->vtbl->IsLost(m_srcSurf);
                if (hr != 0x887601c2) {
                    return hr;
                }
                hr = m_srcSurf->vtbl->Restore(m_srcSurf);
                if (hr != 0) {
                    return hr;
                }
            }
        }
    }
}

// ===========================================================================
// 0x17cfc0 - Configure: derive the tile grid (m_tilesAcross across / m_tilesDown down) and the
// scroll origin (m_originX/m_originY) for a layout `mode` (0..3), validating the caller's
// optional origin point and clip rect against the screen extents (m_screenWidth/m_screenHeight)
// first. Modes 2/3 may pin an explicit dest rect at m_destRect (RezAlloc'd). m_forceSingleRow
// forces a single tile row; m_50c/m_86a0 are reset. Returns 1 / 0 on rejection.
// ===========================================================================
// @early-stop
// reloc-mask scoring artifact (~97.4%): the CODE BYTES are byte-exact (verified by
// llvm-objdump -dr base vs target - the instruction streams are identical, only
// trailing inter-function padding differs). The residual is two differently-named
// reloc operands the delinker can't reconcile: the rel32 call to the sibling grid
// validator at 0x17cbe0 (still stubbed as ?Unmatched_17cbe0@@YAXXZ, modeled here as
// CDDScreen::CheckGrid), and the compiler-emitted switch jump table (base $L385 vs
// delinked switchdataD_0057d2a0). Both resolve when those symbols are named; not a
// codegen issue. topic:scoring-artifact - no further code change possible.
RVA(0x0017cfc0, 0x2dd)
i32 CDDScreen::Configure(i32 mode, i32 flags, DDPoint* origin, DDRect* rect) {
    if (origin) {
        if (origin->x > m_screenWidth) {
            return 0;
        }
        if (origin->y > m_screenHeight) {
            return 0;
        }
    }
    if (rect) {
        if (rect->left > rect->right) {
            return 0;
        }
        if (rect->top > rect->bottom) {
            return 0;
        }
        if ((u32)rect->right > m_screenWidth) {
            return 0;
        }
        if ((u32)rect->bottom > m_screenHeight) {
            return 0;
        }
    }
    if (m_tileInfo->m_width > m_screenWidth) {
        return 0;
    }
    if (m_tileInfo->m_height > m_screenHeight) {
        return 0;
    }
    if (!CheckGrid()) {
        return 0;
    }

    switch (mode) {
        case 0:
            m_tilesAcross = m_screenWidth / m_tileInfo->m_width;
            m_tilesDown = m_screenHeight / m_tileInfo->m_height;
            if (flags & 0x10) {
                if (!origin) {
                    return 0;
                }
                m_originX = origin->x;
                m_originY = origin->y;
            } else {
                m_originX = (m_screenWidth - m_tilesAcross * m_tileInfo->m_width) >> 1;
                m_originY = (m_screenHeight - m_tilesDown * m_tileInfo->m_height) >> 1;
            }
            break;
        case 1:
            m_tilesAcross = 1;
            m_tilesDown = 1;
            if (flags & 0x10) {
                if (!origin) {
                    return 0;
                }
                m_originX = origin->x;
                m_originY = origin->y;
            } else {
                m_originX = (m_screenWidth - m_tileInfo->m_width) >> 1;
                m_originY = (m_screenHeight - m_tileInfo->m_height) >> 1;
            }
            break;
        case 2:
            if (m_screenWidth % m_tileInfo->m_width == 0
                && m_screenHeight % m_tileInfo->m_height == 0) {
                m_tilesAcross = m_screenWidth / m_tileInfo->m_width;
                m_tilesDown = m_screenHeight / m_tileInfo->m_height;
                if (flags & 0x10) {
                    if (!origin) {
                        return 0;
                    }
                    m_originX = origin->x;
                    m_originY = origin->y;
                } else {
                    m_originX = (m_screenWidth - m_tilesAcross * m_tileInfo->m_width) >> 1;
                    m_originY = (m_screenHeight - m_tilesDown * m_tileInfo->m_height) >> 1;
                }
            } else {
                m_tilesAcross = 1;
                m_tilesDown = 1;
                m_originX = 0;
                m_originY = 0;
                m_destRect = (DDRect*)RezAlloc(0x10);
                m_destRect->top = 0;
                m_destRect->left = 0;
                m_destRect->bottom = m_screenHeight;
                m_destRect->right = m_screenWidth;
                m_514 = 1;
            }
            break;
        case 3: {
            m_tilesAcross = 1;
            m_tilesDown = 1;
            m_originX = 0;
            m_originY = 0;
            if (!rect) {
                return 0;
            }
            DDRect* r = (DDRect*)RezAlloc(0x10);
            m_destRect = r;
            r->left = rect->left;
            r->top = rect->top;
            r->right = rect->right;
            r->bottom = rect->bottom;
            break;
        }
        default:
            return 0;
    }

    if (m_forceSingleRow != 0) {
        m_tilesDown = 1;
    }
    m_50c = 0;
    m_86a0 = 0;
    return 1;
}

SIZE_UNKNOWN(CDDScreen);
SIZE_UNKNOWN(CSurfacePalette);
SIZE_UNKNOWN(CTileInfo);
SIZE_UNKNOWN(DDBLTFX_);
SIZE_UNKNOWN(DDPoint);
SIZE_UNKNOWN(DDRect);
SIZE_UNKNOWN(IDDObj);
SIZE_UNKNOWN(IDDVtbl);
