// BoundaryUpperViews.h - shared referent/owner views for the upper-half
#include <Wap32/Object.h> // CObject grand-base (real virtual dtor)
// (RVA >= 0x133370) engine_boundary leaf methods reconstructed in BoundaryUpper.cpp
// (DinMgr2 / Dsndmgr / DDrawMgr / Rez engine modules).
//
// RTTI cannot attribute these COMDAT-folded methods, so the owning class names are
// placeholders; only OFFSETS + emitted code bytes are load-bearing (campaign
// doctrine).
#ifndef GRUNTZ_BOUNDARYUPPERVIEWS_H
#define GRUNTZ_BOUNDARYUPPERVIEWS_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Blk6c.h> // the 0x6c-byte CImageOwned transform descriptor

// Embedded base-subobject vptr restamp (member dtor of the grand-base): the 7-byte
// `mov [this],&g_wapObjectDtorVtbl; ret` leaf. Three distinct leaf classes share it.
SIZE_UNKNOWN(C16be60);

// (Cb151d20/B_151d20 are GONE - the 0x151d20 notify is CGameObject::NotifyHooked:
// the "+0x7c hook owner" was the AnimWorkerObj aux (fn@+0x10 == m_notify, +0x1c ==
// the m_1c role-union slot).)

// --- vtable catalog ---

#endif // GRUNTZ_BOUNDARYUPPERVIEWS_H
