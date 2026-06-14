#ifndef GAME_CGRUNTZWND_H
#define GAME_CGRUNTZWND_H

/*
 * CGruntzWnd — the Gruntz main window (CGameWnd subclass).
 * .?AVCGruntzWnd@@  (size 0x10, same as the base; CGruntzWnd adds no fields)
 *
 * Layout ported from tomalla (@approx tomalla 1.0.1.77). The vtable is mostly
 * unrecovered. CGameWnd has graduated into src/Wap32/Wap32.h (matched 0x10
 * layout); a minimal matching base is restated here only so this comprehension
 * subclass lays out standalone (`gruntz structs` prefers the src/ CGameWnd on overlap).
 */

namespace WAP32
{
    // Minimal CGameWnd base (0x10), mirroring the graduated src/Wap32/Wap32.h
    // layout: vptr @0, HWND m_4 @4, owner ptr m_8 @8, guard int m_c @c.
    class CGameWnd
    {
    public:
        CGameWnd();
        virtual ~CGameWnd();
        virtual int Wap32GameWndVfunc0();

        void *m_4;   // +0x04  HWND
        void *m_8;   // +0x08  owner pointer
        int   m_c;   // +0x0c  guard flag
    };
}

class CGruntzWnd : public WAP32::CGameWnd
{
public:
    CGruntzWnd();
    virtual ~CGruntzWnd();    // vector deleting destructor @0x494590 (1.0.1.77)

    //@address: 006464d4
    static void *unknown_active_window_handle;  // HWND
};                            // 0x10 (no fields beyond the base)

#endif /* GAME_CGRUNTZWND_H */
