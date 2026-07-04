// DDScreen.cpp - three methods of the big tiled-DirectDraw display manager
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

// The DirectDraw COM interfaces (IDirectDraw/IDirectDraw2/IDirectDrawSurface/
// IDirectDrawPalette) + descriptor structs (DDBLTFX, RECT, POINT) come from the real
// SDK: <Win32.h> (windows.h) then <ddraw.h> (interfaces via <ddraw.h> whose slots
// lower to `mov eax,[obj]; call [eax+slot]`). The primary/source surfaces drive the
// surface interface, the IDirectDraw2 device the DirectDraw interface, and the
// palette the palette interface - each at its own retail vtable slots.
#include <DDrawMgr/DirectDrawMgr.h>
#include <Win32.h>
#include <ddraw.h>

#include <string.h> // memset - inlined to rep stos

// The engine heap allocator (NAFXCW operator new replacement) - 16-byte RECT
// nodes Configure allocates for the explicit-blit case. _RezAlloc (named, rel32).
extern "C" void* RezAlloc(u32 size); // 0x1b9b46

// The tile-size descriptor m_tileInfo points at (only the tile width/height are read;
// unsigned so the validation/divide lower to `ja`/`div`).
struct CTileInfo {
    i32 m_0;      // +0x0
    u32 m_width;  // +0x4  tile width
    u32 m_height; // +0x8  tile height
};

class CDDScreen {
public:
    void HandleError();  // 0x17cc80
    void ResetPalette(); // 0x17ca60 (body in PaletteReset.cpp; clears the +0x108 table)
    i32 BlitRegion(i32 col, i32 row, i32 nCols, i32 nRows);        // 0x17cdf0
    i32 Configure(i32 mode, i32 flags, POINT* origin, RECT* rect); // 0x17cfc0
    i32 CheckGrid();                                               // 0x17cbe0 (sibling, external)
    void UploadPalette(); // 0x17ca10 (palette re-realize on 8bpp restore; body in PaletteCopy.cpp)

    char m_pad00[0x0c];
    i32 m_0c; // +0x0c   ==0 gates full DDraw-stack teardown (owns-vs-borrows, unproven)
    CTileInfo* m_tileInfo;         // +0x10
    IDirectDraw2* m_dd2;           // +0x14   IDirectDraw2
    IDirectDraw* m_dd;             // +0x18   IDirectDraw
    IDirectDrawSurface* m_primary; // +0x1c   primary surface
    IDirectDrawSurface* m_20;      // +0x20   surface (only Release'd here; role unproven)
    IDirectDrawSurface* m_srcSurf; // +0x24   blit source surface
    IDirectDrawSurface* m_28;      // +0x28   surface (only Release'd here; role unproven)
    IDirectDrawPalette* m_palette; // +0x2c
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
    RECT* m_destRect;     // +0x534  explicit dest rect (or 0)
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
        m_srcSurf->Release();
        m_srcSurf = 0;
    }
    if (m_28) {
        m_28->Release();
        m_28 = 0;
    }
    if (m_bpp == 8) {
        ResetPalette();
    }
    if (m_primary) {
        DDBLTFX fx;
        memset(&fx, 0, sizeof(fx));
        fx.dwSize = 0x64;
        fx.dwROP = 0x42;
        void* rc = (void*)m_primary->Blt(0, 0, 0, 0x1020000, &fx);
        if (rc) {
            memset(&fx, 0, sizeof(fx));
            fx.dwSize = 0x64;
            fx.dwFillColor = 0;
            m_primary->Blt(0, 0, 0, 0x1000400, &fx);
        }
    }
    if (m_0c == 0) {
        if (m_palette) {
            m_palette->Release();
            m_palette = 0;
        }
        if (m_primary) {
            m_primary->Release();
            m_primary = 0;
        }
        if (m_20) {
            m_20->Release();
            m_20 = 0;
        }
        if (m_dd2) {
            m_dd2->RestoreDisplayMode();
            m_dd2->Release();
            m_dd2 = 0;
        }
        if (m_dd) {
            m_dd->Release();
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
RVA(0x0017cdf0, 0x1c6)
i32 CDDScreen::BlitRegion(i32 col, i32 row, i32 nCols, i32 nRows) {
    RECT dst, src;
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
            hr = m_primary->BltFast(dst.left, dst.top, m_srcSurf, &src, 0x10);
            if (hr != 0x887601c2) {
                return hr;
            }
            if (m_primary->IsLost() == 0x887601c2 && m_primary->Restore() == 0) {
                if (m_bpp == 8) {
                    m_primary->SetPalette(m_palette);
                    UploadPalette();
                }
            } else {
                hr = m_srcSurf->IsLost();
                if (hr != 0x887601c2) {
                    return hr;
                }
                hr = m_srcSurf->Restore();
                if (hr != 0) {
                    return hr;
                }
            }
        } else {
            hr = m_primary->Blt(&dst, m_srcSurf, &src, 0x1000000, 0);
            if (hr != 0x887601c2) {
                return hr;
            }
            if (m_primary->IsLost() == 0x887601c2 && m_primary->Restore() == 0) {
                if (m_bpp == 8) {
                    m_primary->SetPalette(m_palette);
                    UploadPalette();
                }
            } else {
                hr = m_srcSurf->IsLost();
                if (hr != 0x887601c2) {
                    return hr;
                }
                hr = m_srcSurf->Restore();
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
i32 CDDScreen::Configure(i32 mode, i32 flags, POINT* origin, RECT* rect) {
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
                m_destRect = (RECT*)RezAlloc(0x10);
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
            RECT* r = (RECT*)RezAlloc(0x10);
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
SIZE_UNKNOWN(CTileInfo);
