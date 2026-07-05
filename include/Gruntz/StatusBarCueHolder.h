// StatusBarCueHolder.h - the cue-playing status-bar holder shared by the
// level-preview (LevelPreview.cpp) and finish-level (FinishLevelSprite.cpp) state
// drivers. ONE shape, ONE definition: an embedded name->cue hash table at +0x10
// (CCueHashTable::Lookup @0x1b8438, __thiscall ret 8), the audio-kill sound mgr at
// +0x2c (only the preview driver reads it), the live-surface gate at +0x30.
//
// NOT a different object from the sprite-keyed holder in StatusBarUpdatersViews.h -
// PROVEN the SAME object (a fold is pending, see below). The old "distinct, do NOT
// union" note reasoned from the view NAMES (CCueHashTable vs CSpriteHashTable), but
// both are views of one MFC CMapStringToOb (their Lookup is CMapStringToOb::Lookup
// @0x1b8438). This holder is CState::m_c's canonical CSpriteFactoryHolder at +0x28
// (the CSndHost cue registry): CState::m_0c == g_gameReg->m_world (CGameRegistry+0x30)
// == the one CSpriteFactoryHolder (traced via State.h / Play.h:47 / GameMode.h:132).
// So this CStatusBarHolder, StatusBarUpdatersViews.h's, and LevelPreview's PreviewMgr
// are all facets of it. Kept split ONLY until the CSpriteFactoryHolder cascade fold
// (holder->CSndHost, CCueHashTable/CSpriteHashTable->CMapStringToOb) lands.
// Only offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CSTATUSBARCUEHOLDER_H
#define GRUNTZ_CSTATUSBARCUEHOLDER_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SoundCueMgr.h> // CSoundCueMgr (ConfigureItem @0x1360d0, +0x28 cue duration)
#include <Gruntz/Sprite.h> // CSpriteHashTable - the ONE name->CObject map (both cue + sprite views)

class SoundDevice;

// The named cue a Lookup resolves: its CSoundCueMgr at +0x10, last-played clock at
// +0x14, cue interval at +0x18.
struct CueObj {
    char m_pad00[0x10];
    CSoundCueMgr* m_10; // +0x10
    i32 m_14;           // +0x14  last-played clock
    i32 m_18;           // +0x18  cue interval
};
SIZE_UNKNOWN(CueObj);

// The embedded name->object map is the ONE CSpriteHashTable (Sprite.h) - the MFC
// CMapStringToOb whose Lookup (0x1b8438 == 0x1b8008, both CMapStringToOb::Lookup)
// every consumer calls; the CueObj** overload types the cue value cast-free. (The
// former local CCueHashTable was a second view of it - folded away.)

// The status-bar holder: name->object map at +0x10, audio-kill sound mgr at +0x2c,
// live-surface gate at +0x30.
struct CStatusBarHolder {
    char m_pad00[0x10];
    CSpriteHashTable m_10map; // +0x10  the ONE name->CObject map (Sprite.h)
    char m_pad14[0x2c - 0x14];
    SoundDevice* m_2c; // +0x2c  audio-kill sound mgr
    i32 m_30;          // +0x30  live-surface gate
};
SIZE_UNKNOWN(CStatusBarHolder);

#endif // GRUNTZ_CSTATUSBARCUEHOLDER_H
