#ifndef MANAGERS_CDIRECTDRAWMGR_H
#define MANAGERS_CDIRECTDRAWMGR_H

/*
 * CDirectDrawMgr — DirectDraw graphics/surface/palette manager.
 *
 * Leaked source TU:  C:\Proj\DDrawMgr\DDRAWMGR.CPP
 *   companions:      DIRPAL.CPP (palette), DIRSURF.CPP (surface), ddrawmgr.h,
 *                    c:\proj\incs\ddrawmgr.h (shared header)
 *
 * Provenance: the class name CDirectDrawMgr is NOT in RTTI; it is mined from the
 * error string "CDirectDrawMgr::GetGDISurface() - Cannot get the GDI surface!"
 * (STRINGS_ANALYSIS.md §6). DirPal / DirSurf are inferred from the file names; no
 * RTTI name survives for them either. NO layout recovered — all @todo.
 *
 * This is a shared WAP32 engine manager (same code is expected in Claw / Get
 * Medieval). DirectX 5 baseline ("DirectX 5 is required").
 *
 * HYPOTHESIS: tomalla's reconstructed "harry_potter" surface/page-manager class
 * family (held in CGruntzMgr @0x30, inits 640x480x16, owns a DDSURFACEDESC-shaped
 * static struct) is believed to BE this CDirectDrawMgr family. That hierarchy —
 * with full version-independent offsets/sizes/inheritance — is modeled separately
 * in ddrawmgr_surface_family.h (tagged as a HYPOTHESIS, names are placeholders).
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>
#include <ddraw.h>

/* DirPal — palette wrapper (DIRPAL.CPP). No RTTI; name inferred from file. */
class DirPal
{
public:
    //@size: unknown @todo
    //@todo: wraps IDirectDrawPalette + PALETTEENTRY[256] — layout unknown
};

/* DirSurf — surface wrapper (DIRSURF.CPP). No RTTI; name inferred from file. */
class DirSurf
{
public:
    //@size: unknown @todo
    //@todo: wraps IDirectDrawSurface (+ width/height/depth/pitch). The DDraw
    //       diagnostic dump prints: width,height,depth,pitch; 16-bit R/G/B masks;
    //       source color key; Z-buffer depth (STRINGS_ANALYSIS.md §5).
};

/*
 * CDirectDrawMgr (a.k.a. "DirectDrawMgr" in error strings).
 * Name source: error-string only — no RTTI, so kept without an @rtti tag.
 */
class CDirectDrawMgr
{
public:
    //@size: unknown @todo

    // Verbatim from the binary:
    //@todo
    IDirectDrawSurface* GetGDISurface();  // "CDirectDrawMgr::GetGDISurface()"

    //@todo: everything else (init/page-manager/primary-surface/flip) unknown.
    //       Related error strings: "Can't initialize DirectDraw.",
    //       "Can't initialize the page manager.", "Can't get the primary surface.",
    //       "Can't create the background page.", "Resolution is now %ix%ix%i".
};

#endif /* MANAGERS_CDIRECTDRAWMGR_H */
