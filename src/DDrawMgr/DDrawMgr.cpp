// DDrawMgr.cpp - CDirectDrawMgr error-string mapper.
// Maps DDERR HRESULT codes to symbol names + descriptions, outputs via
// the configured reporting channels. The largest GetErrorString in the
// codebase (2101 B), with a dense jump table for the 0x88760230..0x8876023d
// range.
//
// @address: 0x141400
// @size:    0x835
#include "DDrawMgr.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Module-level globals (pinned @data addresses).
// ---------------------------------------------------------------------------
int  g_ddrawBeep      = 0;   // @0x683ec0
int  g_ddrawDebug     = 0;   // @0x683eb8
int  g_ddrawMsgBox    = 0;   // @0x683ebc
int  g_ddrawOutputDbg = 0;   // @0x683ec4
char g_szDDrawModule[] = "DirectDrawMgr";  // @0x61a378

// Format strings
static const char s_fmtDebug[]      = "%s, line %i: %s (%i) - %s\n";   // @0x619a20
static const char s_fmtDebugShort[] = "%s (%i) - %s\n";                // @0x619a10
static const char s_fmtMsgBox[]     = "%s, line %i\n\n%s (%i)\n\n%s";  // @0x6199f4
static const char s_fmtMsgBoxShort[]= "%s (%i)\n\n%s";                 // @0x6199e8

// Utility strings
static const char s_unknownErrMsg[] = "Unknown Error Message";   // @0x619ec0
static const char s_unknownErrCode[]= "Unknown Error Code";      // @0x619eac
static const char s_empty[]         = "";                        // @0x6293f4
static const char s_noError[]       = "No error";                // @0x619ad4
static const char s_noMsg[]         = "No message";              // @0x619d48
static const char s_ddOk[]          = "DD_OK";                   // @0x619ae0

// DDERR symbol names + descriptions (where present)
static const char s_dderrUnsupported[]    = "DDERR_UNSUPPORTED";           // @0x61a9e4
static const char s_descUnsupported[]     = "Action not supported";        // @0x61a9cc
static const char s_dderrGeneric[]        = "DDERR_GENERIC";               // @0x61a9bc
static const char s_descGeneric[]         = "Generic failure";             // @0x61a9ac
static const char s_dderrOutOfMemory[]    = "DDERR_OUTOFMEMORY";           // @0x61a998
static const char s_dderrInvalidParams[]  = "DDERR_INVALIDPARAMS";         // @0x61a984
static const char s_dderrInvalidCaps[]    = "DDERR_INVALIDCAPS";           // @0x61a970
static const char s_descInvalidCaps[]     = "One or more of the caps bits passed to the callback are incorrect"; // @0x61a92c
static const char s_dderrInvalidMode[]    = "DDERR_INVALIDMODE";           // @0x61a918
static const char s_dderrInvalidObject[]  = "DDERR_INVALIDOBJECT";         // @0x61a904
static const char s_dderrInvalidPixelFmt[]= "DDERR_INVALIDPIXELFORMAT";    // @0x61a8e8
static const char s_descInvalidPixelFmt[] = "Pixel format was invalid as specified."; // @0x61a8c0
static const char s_dderrInvalidRect[]    = "DDERR_INVALIDRECT";           // @0x61a8ac
static const char s_dderrNo3D[]           = "DDERR_NO3D";                  // @0x61a8a0
static const char s_dderrNoAlphaHW[]      = "DDERR_NOALPHAHW";            // @0x61a890
static const char s_dderrNoCoopLevel[]    = "DDERR_NOCOOPERATIVELEVELSET"; // @0x61a874
static const char s_descNoCoopLevel[]     = "Create function called without DirectDraw object method SetCooperativeLevel being called"; // @0x61a818
static const char s_dderrNoColorConvHW[]  = "DDERR_NOCOLORCONVHW";         // @0x61a804
static const char s_dderrNoExclusiveMode[]= "DDERR_NOEXCLUSIVEMODE";       // @0x61a7ec
static const char s_dderrNoGDI[]          = "DDERR_NOGDI";                 // @0x61a7e0
static const char s_descNoGDI[]           = "There is no GDI present";     // @0x61a7c8
static const char s_dderrNoMirrorHW[]     = "DDERR_NOMIRRORHW";           // @0x61a7b4
static const char s_descNoMirrorHW[]      = "Operation could not be carried out because there is no hardware present or available."; // @0x61a75c
static const char s_dderrNotFound[]       = "DDERR_NOTFOUND";             // @0x61a74c
static const char s_descNotFound[]        = "Request item was not found"; // @0x61a730
static const char s_dderrNoOverlayHW[]    = "DDERR_NOOVERLAYHW";          // @0x61a71c
static const char s_dderrNoRasterOpHW[]   = "DDERR_NORASTEROPHW";         // @0x61a708
static const char s_dderrNoRotateHW[]     = "DDERR_NOROTATEHW";           // @0x61a6f4
static const char s_dderrNoStretchHW[]    = "DDERR_NOSTRETCHHW";          // @0x61a6e0
static const char s_dderrNot8BitColor[]   = "DDERR_NOT8BITCOLOR";         // @0x61a6cc
static const char s_dderrNoTextureHW[]    = "DDERR_NOTEXTUREHW";          // @0x61a6b8
static const char s_dderrNoVSyncHW[]      = "DDERR_NOVSYNCHW";            // @0x61a6a8
static const char s_dderrNoZBufferHW[]    = "DDERR_NOZBUFFERHW";          // @0x61a694
static const char s_dderrOutOfCaps[]      = "DDERR_OUTOFCAPS";           // @0x61a684
static const char s_dderrOutOfVideoMem[]  = "DDERR_OUTOFVIDEOMEMORY";     // @0x61a66c
static const char s_dderrPaletteBusy[]    = "DDERR_PALETTEBUSY";          // @0x61a658
static const char s_dderrSurfaceBusy[]    = "DDERR_SURFACEBUSY";          // @0x61a644
static const char s_dderrSurfaceObscured[]= "DDERR_SURFACEISOBSCURED";    // @0x61a62c
static const char s_dderrSurfaceLost[]    = "DDERR_SURFACELOST";          // @0x61a618
static const char s_dderrSurfaceNotAtt[]  = "DDERR_SURFACENOTATTACHED";   // @0x61a5fc
static const char s_descSurfaceNotAtt[]   = "The requested surface is not attached"; // @0x61a5d4
static const char s_dderrTooBigSize[]     = "DDERR_TOOBIGSIZE";           // @0x61a5c0
static const char s_dderrTooBigWidth[]    = "DDERR_TOOBIGWIDTH";          // @0x61a5ac
static const char s_dderrVBInProgress[]   = "DDERR_VERTICALBLANKINPROGRESS"; // @0x61a58c
static const char s_dderrWasStillDrawing[]= "DDERR_WASTILLDRAWING";       // @0x61a574
static const char s_descWasStillDrawing[] = "The previous Blt which is transfering information to or from this Surface is incomplete"; // @0x61a51c

// Jump-table handled cases
static const char s_dderrNoDirectDrawHW[]     = "DDERR_NODIRECTDRAWHW";      // @0x61a504
static const char s_dderrDirectDrawCreated[]  = "DDERR_DIRECTDRAWALREADYCREATED"; // @0x61a4e4
static const char s_dderrXAlign[]             = "DDERR_XALIGN";              // @0x61a4d4
static const char s_descXAlign[]              = "Rectangle provided was not horizontally aligned on a DWORD boundary"; // @0x61a490
static const char s_dderrHwndSubclassed[]     = "DDERR_HWNDSUBCLASSED";      // @0x61a478
static const char s_dderrHwndAlreadySet[]     = "DDERR_HWNDALREADYSET";      // @0x61a460
static const char s_dderrNoPaletteHW[]        = "DDERR_NOPALETTEHW";         // @0x61a44c
static const char s_descNoPaletteHW[]         = "No hardware support for 16 or 256 color palettes"; // @0x61a418
static const char s_dderrPrimarySurfaceExists[] = "DDERR_PRIMARYSURFACEALREADYEXISTS"; // @0x61a3f4
static const char s_descPrimarySurfaceExists[] = "This process already has created a primary surface"; // @0x61a3c0
static const char s_dderrExclusiveModeSet[]   = "DDERR_EXCLUSIVEMODEALREADYSET"; // @0x61a3a0
static const char s_dderrLockedSurfaces[]     = "DDERR_LOCKEDSURFACES";       // @0x61a388

// ---------------------------------------------------------------------------
// @address: 0x141400
// @size:    0x835
// ---------------------------------------------------------------------------
void CDirectDrawMgr::GetErrorString(const char *file, int line, long hr)
{
    char sym[0x40];    // [esp+0x10]
    char desc[0x100];  // [esp+0x50]
    char out[0x100];   // [esp+0x150]

    int hrLow = hr & 0xFFFF;

    // Optional beep
    if (g_ddrawBeep)
        MessageBeep(0x30);

    // Early exit if no output channel active
    if (!g_ddrawDebug && !g_ddrawMsgBox && !g_ddrawOutputDbg)
        return;

    // Default strings
    strcpy(desc, s_unknownErrMsg);
    EngFormat(sym, s_unknownErrCode);
    strcpy(out, s_empty);

    // Map HRESULT -> symbol name (+ description for many cases)
    if (hr == 0x80004001) {                              // E_NOTIMPL
        strcpy(sym, s_dderrUnsupported);
        strcpy(desc, s_descUnsupported);
    } else if (hr == 0x80004005) {                       // E_FAIL
        strcpy(sym, s_dderrGeneric);
        strcpy(desc, s_descGeneric);
    } else if (hr == 0x8007000e) {                       // E_OUTOFMEMORY
        strcpy(sym, s_dderrOutOfMemory);
    } else if (hr == 0x80070057) {                       // E_INVALIDARG
        strcpy(sym, s_dderrInvalidParams);
    } else if (hr == 0x88760064) {                       // DDERR_INVALIDCAPS
        strcpy(sym, s_dderrInvalidCaps);
        strcpy(desc, s_descInvalidCaps);
    } else if (hr == 0x88760078) {                       // DDERR_INVALIDMODE
        strcpy(sym, s_dderrInvalidMode);
    } else if (hr == 0x88760082) {                       // DDERR_INVALIDOBJECT
        strcpy(sym, s_dderrInvalidObject);
    } else if (hr == 0x88760091) {                       // DDERR_INVALIDPIXELFORMAT
        strcpy(sym, s_dderrInvalidPixelFmt);
        strcpy(desc, s_descInvalidPixelFmt);
    } else if (hr == 0x88760096) {                       // DDERR_INVALIDRECT
        strcpy(sym, s_dderrInvalidRect);
    } else if (hr == 0x887600a0) {                       // DDERR_NOALPHAHW (wait, check)
        strcpy(sym, s_dderrLockedSurfaces);              // DDERR_LOCKEDSURFACES at this value
    } else if (hr == 0x887600aa) {                       // DDERR_NO3D
        strcpy(sym, s_dderrNo3D);
    } else if (hr == 0x887600b4) {                       // DDERR_NOALPHAHW
        strcpy(sym, s_dderrNoAlphaHW);
    } else if (hr == 0x887600d2) {                       // DDERR_NOCOLORCONVHW
        strcpy(sym, s_dderrNoColorConvHW);
    } else if (hr == 0x887600d4) {                       // DDERR_NOCOOPERATIVELEVELSET
        strcpy(sym, s_dderrNoCoopLevel);
        strcpy(desc, s_descNoCoopLevel);
    } else if (hr == 0x887600e1) {                       // DDERR_NOEXCLUSIVEMODE
        strcpy(sym, s_dderrNoExclusiveMode);
    } else if (hr == 0x887600f0) {                       // DDERR_NOGDI
        strcpy(sym, s_dderrNoGDI);
        strcpy(desc, s_descNoGDI);
    } else if (hr == 0x887600fa) {                       // DDERR_NOMIRRORHW
        strcpy(sym, s_dderrNoMirrorHW);
        strcpy(desc, s_descNoMirrorHW);
    } else if (hr == 0x887600ff) {                       // DDERR_NOTFOUND
        strcpy(sym, s_dderrNotFound);
        strcpy(desc, s_descNotFound);
    } else if (hr == 0x88760104) {                       // DDERR_NOOVERLAYHW
        strcpy(sym, s_dderrNoOverlayHW);
    } else if (hr == 0x88760118) {                       // DDERR_NORASTEROPHW
        strcpy(sym, s_dderrNoRasterOpHW);
    } else if (hr == 0x88760122) {                       // DDERR_NOROTATEHW
        strcpy(sym, s_dderrNoRotateHW);
    } else if (hr == 0x88760136) {                       // DDERR_NOSTRETCHHW
        strcpy(sym, s_dderrNoStretchHW);
    } else if (hr == 0x88760140) {                       // DDERR_NOT8BITCOLOR
        strcpy(sym, s_dderrNot8BitColor);
    } else if (hr == 0x8876014a) {                       // DDERR_NOTEXTUREHW
        strcpy(sym, s_dderrNoTextureHW);
    } else if (hr == 0x8876014f) {                       // DDERR_NOVSYNCHW
        strcpy(sym, s_dderrNoVSyncHW);
    } else if (hr == 0x88760154) {                       // DDERR_NOZBUFFERHW
        strcpy(sym, s_dderrNoZBufferHW);
    } else if (hr == 0x88760168) {                       // DDERR_OUTOFCAPS
        strcpy(sym, s_dderrOutOfCaps);
    } else if (hr == 0x8876017c) {                       // DDERR_OUTOFVIDEOMEMORY
        strcpy(sym, s_dderrOutOfVideoMem);
    } else if (hr == 0x88760183) {                       // DDERR_PALETTEBUSY
        strcpy(sym, s_dderrPaletteBusy);
    } else if (hr == 0x887601ae) {                       // DDERR_SURFACEBUSY
        strcpy(sym, s_dderrSurfaceBusy);
    } else if (hr == 0x887601b8) {                       // DDERR_SURFACEISOBSCURED
        strcpy(sym, s_dderrSurfaceObscured);
    } else if (hr == 0x887601c2) {                       // DDERR_SURFACELOST
        strcpy(sym, s_dderrSurfaceLost);
    } else if (hr == 0x887601cc) {                       // DDERR_SURFACENOTATTACHED
        strcpy(sym, s_dderrSurfaceNotAtt);
        strcpy(desc, s_descSurfaceNotAtt);
    } else if (hr == 0x887601e0) {                       // DDERR_TOOBIGSIZE
        strcpy(sym, s_dderrTooBigSize);
    } else if (hr == 0x887601ea) {                       // DDERR_TOOBIGWIDTH
        strcpy(sym, s_dderrTooBigWidth);
    } else if (hr == 0x88760219) {                       // DDERR_VERTICALBLANKINPROGRESS
        strcpy(sym, s_dderrVBInProgress);
    } else if (hr == 0x8876021c) {                       // DDERR_WASTILLDRAWING
        strcpy(sym, s_dderrWasStillDrawing);
        strcpy(desc, s_descWasStillDrawing);
    } else if (hr == 0x88760245) {                       // DDERR_EXCLUSIVEMODEALREADYSET
        strcpy(sym, s_dderrExclusiveModeSet);
    } else if (hr == 0x88760248) {                       // DDERR_LOCKEDSURFACES (dup of 0x887600a0)
        strcpy(sym, s_dderrLockedSurfaces);
    }
    // Jump-table range 0x88760230..0x8876023d
    else if (hr == 0x88760230) {                         // DDERR_XALIGN
        strcpy(sym, s_dderrXAlign);
        strcpy(desc, s_descXAlign);
    } else if (hr == 0x88760232) {                       // DDERR_DIRECTDRAWALREADYCREATED
        strcpy(sym, s_dderrDirectDrawCreated);
    } else if (hr == 0x88760233) {                       // DDERR_NODIRECTDRAWHW
        strcpy(sym, s_dderrNoDirectDrawHW);
    } else if (hr == 0x88760234) {                       // DDERR_PRIMARYSURFACEALREADYEXISTS
        strcpy(sym, s_dderrPrimarySurfaceExists);
        strcpy(desc, s_descPrimarySurfaceExists);
    } else if (hr == 0x8876023a) {                       // DDERR_HWNDSUBCLASSED
        strcpy(sym, s_dderrHwndSubclassed);
    } else if (hr == 0x8876023b) {                       // DDERR_HWNDALREADYSET
        strcpy(sym, s_dderrHwndAlreadySet);
    } else if (hr == 0x8876023d) {                       // DDERR_NOPALETTEHW
        strcpy(sym, s_dderrNoPaletteHW);
        strcpy(desc, s_descNoPaletteHW);
    } else if (hr == 0) {                                // DD_OK
        strcpy(sym, s_ddOk);
        strcpy(desc, s_noError);
    }

    // Output via debugger channel
    if (g_ddrawDebug) {
        if (file && line > 0)
            EngFormat(out, s_fmtDebug, file, line, sym, hrLow, desc);
        else
            EngFormat(out, s_fmtDebugShort, sym, hrLow, desc);
        OutputDebugStringA(out);
    }

    // Output via message box channel
    if (g_ddrawMsgBox) {
        if (file && line > 0)
            EngFormat(out, s_fmtMsgBox, file, line, sym, hrLow, desc);
        else
            EngFormat(out, s_fmtMsgBoxShort, sym, hrLow, desc);
        MessageBoxA(0, out, g_szDDrawModule, 0x30);
    }
}
