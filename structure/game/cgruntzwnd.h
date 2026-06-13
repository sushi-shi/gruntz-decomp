#ifndef GAME_CGRUNTZWND_H
#define GAME_CGRUNTZWND_H

/*
 * CGruntzWnd — the Gruntz main window (CGameWnd subclass).
 * .?AVCGruntzWnd@@
 *
 * Layout PORTED FROM tomalla (refs/tomalla-gruntz/gruntz/cgruntzwnd.h), attributed.
 * @address values = 1.0.1.77. Same instance size as the base (0x10); the vtable is
 * mostly @todo.
 */

#include "../wap32/cgamewnd.h"

class CGruntzWnd : public WAP32::CGameWnd
{
public:
    //@size: 0x10 (same as the base)
    CGruntzWnd();
    virtual ~CGruntzWnd();

    //@vftable: 0  vector deleting destructor (-> @address: 00494590)
    //@todo: vftable

    //@address: 006464d4
    static HWND unknown_active_window_handle;
};

#endif /* GAME_CGRUNTZWND_H */
