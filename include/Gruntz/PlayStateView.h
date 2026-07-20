// PlayStateView.h - reduced views of the live PLAY state's per-player status array.
//
// CPlayStateView exposes the +0x520 status-array base of the live PLAY state (a CState
// subclass); PlayStatusSlot is one 0x64-byte entry (status id at +0x20; 3 == "won/done").
// @identity-TODO: the concrete PLAY-state class (CPlay) is not folded here; these are the
// reduced reader views CGruntzMgr reaches via ((CPlayStateView*)m_curState)->m_520.
// Used in src/Gruntz/GruntzMgr.cpp.
//
// Only offsets + code bytes are load-bearing; names are placeholders.
#ifndef GRUNTZ_PLAYSTATEVIEW_H
#define GRUNTZ_PLAYSTATEVIEW_H

#include <rva.h>
#include <Ints.h>

struct PlayStatusSlot {
    char m_pad0[0x20];
    i32 m_status; // +0x20  (3 == won/done)
    char m_pad24[0x64 - 0x24];
};

struct CPlayStateView {
    char m_pad0[0x520];
    PlayStatusSlot* m_520; // +0x520
};
SIZE_UNKNOWN(PlayStatusSlot);
SIZE_UNKNOWN(CPlayStateView);

#endif // GRUNTZ_PLAYSTATEVIEW_H
