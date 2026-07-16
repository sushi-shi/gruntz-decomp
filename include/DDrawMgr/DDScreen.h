// DDScreen.h - THE canonical shape of CDDScreen, the big tiled-DirectDraw display
// manager (the CDDPageMgr bring-up class behind 0x17c040; members run out to
// ~0x86a4). Unifies the three formerly per-TU views (DDScreen.cpp's full class +
// PaletteReset.cpp's IPalSink/partial + PaletteCopy.cpp's partial).
//
// LAYOUT-ONLY header: the DirectDraw COM interfaces (IDirectDraw/IDirectDraw2/
// IDirectDrawSurface/IDirectDrawPalette) and the POINT/RECT descriptor structs come
// from the real SDK, so an includer must pull <Win32.h> (windows.h) then <ddraw.h>
// BEFORE this header (matching the established include order of the owning .cpp).
//
// Field names recovered from usage; offsets + emitted bytes are load-bearing. A few
// fields only touched once (m_20/m_28/m_50c/m_514/m_86a0) keep their offset names -
// role unproven from the matched methods.
#ifndef GRUNTZ_DDRAWMGR_DDSCREEN_H
#define GRUNTZ_DDRAWMGR_DDSCREEN_H

#include <Ints.h>
#include <rva.h>

// The tile/mode descriptor m_tileInfo (+0x10) points at: tile width/height at
// +0x4/+0x8 (read by Configure); the frame's 3-byte-per-entry RGB palette source
// sits at +0x6c (read by UploadPalette off the same pointer). unsigned width/height
// so the validation/divide lower to `ja`/`div`.
struct CTileInfo {
    i32 m_0;      // +0x0
    u32 m_width;  // +0x4  tile width
    u32 m_height; // +0x8  tile height
};

// CDDScreen IS CMoviePlayer (<Io/MoviePlayer.h>) - ONE retail class: all 17 methods
// of the ex CDDScreen / CDDPageMgr / CMoviePlayer views interleave in ONE contiguous
// RVA band in ONE .cpp, and the three layouts tile byte-for-byte (the +0x8690/94/98
// CArray header, the +0x9c DDSURFACEDESC chain, m_520==m_bpp, +0x10 == the Smack
// handle). Full proof in <Io/MoviePlayer.h>.
//
// The name survives as a TYPEDEF ALIAS so the existing definitions/consumers keep
// compiling (the CBrickzGrid==CMapMgr precedent: MSVC5 mangles a member defined
// through the typedef as the REAL class, so they all mangle to ?X@CMoviePlayer@@).
// Declared with a fwd decl + typedef ONLY - pulling <Io/MoviePlayer.h> in here would
// leak MFC/afxtempl into every DirectDraw consumer. A TU that needs the members
// includes <Io/MoviePlayer.h> itself (and puts <Mfc.h> first).
// @fold-TODO: rename the consumers to CMoviePlayer, then drop this alias.
class CMoviePlayer;
typedef CMoviePlayer CDDScreen;
SIZE_UNKNOWN(CTileInfo);

#endif // GRUNTZ_DDRAWMGR_DDSCREEN_H
