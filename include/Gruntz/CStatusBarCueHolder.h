// CStatusBarCueHolder.h - the cue-playing status-bar holder shared by the
// level-preview (LevelPreview.cpp) and finish-level (FinishLevelSprite.cpp) state
// drivers. ONE shape, ONE definition: an embedded name->cue hash table at +0x10
// (CCueHashTable::Lookup @0x1b8438, __thiscall ret 8), the audio-kill sound mgr at
// +0x2c (only the preview driver reads it), the live-surface gate at +0x30.
//
// DISTINCT from the sprite-keyed status-bar holder in StatusBarUpdatersViews.h
// (CSpriteHashTable @+0x10, a different object) - kept split, do NOT union.
// Only offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CSTATUSBARCUEHOLDER_H
#define GRUNTZ_CSTATUSBARCUEHOLDER_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SoundCueMgr.h> // CSoundCueMgr (ConfigureItem @0x1360d0, +0x28 cue duration)

// The named cue a Lookup resolves: its CSoundCueMgr at +0x10, last-played clock at
// +0x14, cue interval at +0x18.
struct CueObj {
    char m_pad00[0x10];
    CSoundCueMgr* m_10; // +0x10
    i32 m_14;           // +0x14  last-played clock
    i32 m_18;           // +0x18  cue interval
};
SIZE_UNKNOWN(CueObj);

// The embedded name->cue hash table (Lookup @0x1b8438, __thiscall, ret 8).
class CCueHashTable {
public:
    i32 Lookup(const char* szName, CueObj** ppOut); // 0x1b8438
};
SIZE_UNKNOWN(CCueHashTable);

// The countdown's audio-kill sound mgr held at CStatusBarHolder+0x2c; its full
// definition (KillCue @0x136e20) lives in the TU that dispatches it (LevelPreview).
class CSoundMgr;

// The status-bar holder: name->cue hash table at +0x10, audio-kill sound mgr at
// +0x2c, live-surface gate at +0x30.
struct CStatusBarHolder {
    char m_pad00[0x10];
    CCueHashTable m_10map; // +0x10
    char m_pad14[0x2c - 0x14];
    CSoundMgr* m_2c; // +0x2c  audio-kill sound mgr
    i32 m_30;        // +0x30  live-surface gate
};
SIZE_UNKNOWN(CStatusBarHolder);

#endif // GRUNTZ_CSTATUSBARCUEHOLDER_H
