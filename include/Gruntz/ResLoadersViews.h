// ResLoadersViews.h - shared view structs for the re-homed Win32/GDI resource +
// counter-draw helpers (namespace ResLoaders). Each is a local view onto its
// (not-yet-recovered) owning class; offsets + emitted bytes are load-bearing.
// Promoted from ResourceLoaders.cpp so downstream homers (e.g. the 0x15a650
// counter method) can use the real type without re-declaring a per-TU view.
#ifndef GRUNTZ_GRUNTZ_RESLOADERSVIEWS_H
#define GRUNTZ_GRUNTZ_RESLOADERSVIEWS_H

#include <Win32.h> // RECT
#include <ddraw.h> // IDirectDrawSurface (the counter window's GetDC/ReleaseDC source)
#include <Ints.h>

namespace ResLoaders {

    // The counter-draw window: a DC-capable DirectDraw surface at +0x08. Its
    // GetDC/ReleaseDC are the standard __stdcall COM slots (self pushed on the
    // stack), so the calls fold to `push arg; push surface; call [vtbl+slot]`.
    struct CounterWnd_164380 {
        char m_pad0[8];
        IDirectDrawSurface* m_8; // +0x08  DC-capable DirectDraw surface
    };

    // Draws a numeric count centred into a rect via the counter window's DC.
    struct DrawHost_164380 {
        char m_pad0[0x2c];
        CounterWnd_164380* m_2c; // +0x2c
        void DrawCount(RECT* rc, i32 n); // 0x00164380
    };

    // Draws a text label centred into a rect via the counter window's DC.
    struct DrawHost2_164420 {
        char m_pad0[0x2c];
        CounterWnd_164380* m_2c; // +0x2c
        void DrawLabel(RECT* rc, char* text); // 0x00164420
    };

} // namespace ResLoaders

#endif // GRUNTZ_GRUNTZ_RESLOADERSVIEWS_H
