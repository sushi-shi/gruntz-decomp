#ifndef GRUNTZ_DDRAWMGR_ANIADVANCE_H
#define GRUNTZ_DDRAWMGR_ANIADVANCE_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/Sprite.h> // CDDrawWorker - the active frame sequence (m_frameSeq)

struct LeafCue; // <Gruntz/LeafCue.h> - the +0x30 random cue table's element

class DSoundCloneInst; // the pooled cue player (ex DSoundCloneInst; Dsndmgr/DirectSoundMgr.h)

class CAniRenderCtx {
public:
    void ClampFirst();  // 0x15cc50  __thiscall on the context (I obj)
    void ClampLast();   // 0x15cc90  __thiscall on the context (I obj)
    char m_pad00[0x08]; // +0x00..0x07
    i32 m_flags;        // +0x08  flags (bit 0x2000000 tested)
    char m_pad0c[0x10 - 0x0c];
    i32 m_posModeX; // +0x10  pos-mode X
    i32 m_posModeY; // +0x14  pos-mode Y
    char m_pad18[0x38 - 0x18];
    i32 m_anchor;              // +0x38  pos anchor (compared to -1)
    char m_pad3c[0x40 - 0x3c]; // +0x3c
    char m_byteFlags;          // +0x40  byte flags (bit 0x2 tested)
    char m_pad41[0x5c - 0x41];
    i32 m_screenX;              // +0x5c  screen X
    i32 m_screenY;              // +0x60  screen Y
    char m_pad64[0x190 - 0x64]; // +0x64..0x18f
    i32 m_frameCursor;          // +0x190  sequence frame cursor
    CDDrawWorker* m_frameSeq;   // +0x194  active frame sequence
    CImage* m_curFrame;         // +0x198  the resolved current frame
};
SIZE_UNKNOWN();

class CAniDesc : public CObject { // the CObArray-stored frame record (vptr from CObject)
public:
    unsigned char
        m_flags; // +0x04  byte flags (bit1 = no-decrement, bit2 = pos-sub, bit3 = trigger-blit, bit8 = anchor)
    char m_pad05[0x08 - 0x05]; // +0x05..0x07
    i32 m_stepMode;            // +0x08  step-mode
    i32 m_loopMode;            // +0x0c  loop-mode word
    i32 m_posMode;             // +0x10  pos-mode
    i32 m_param;               // +0x14  step param
    i32 m_frameTime;           // +0x18  frame-time reload
    i32 m_drawValue;           // +0x1c  draw value
    i32 m_posDX;               // +0x20  pos delta X
    i32 m_posDY;               // +0x24  pos delta Y
    char m_pad28[0x2c - 0x28]; // +0x28
    i32 m_randMod;             // +0x2c  random modulus
    // +0x30  random SOUND-CUE table: retail 0x15c360 loads the dispatch `this`
    // straight out of it (`mov ecx,[eax+edx*4]`) for BOTH the trigger-blit and
    // the throttled-play arm, and both entrypoints are LeafCue methods.
    LeafCue** m_randTable;
};
SIZE_UNKNOWN();

// @identity-TODO (evidence complete, fold BLOCKED on src/DDrawMgr/DDrawSubMgr.cpp,
// which another lane owns): CAniBlitTrigger IS LeafCue. Three independent proofs -
//   layout: +0x00..0x0b is the CLoadable header, +0x0c is CLoadable::m_ownerCtx (the
//     CDDrawSurfaceMgr) and +0x10 is LeafCue::m_10, the DSoundCloneInst;
//   body: TriggerBlit 0x1587f0 and LeafCue::PlayIfElapsed 0x1f940 open with the SAME
//     `ds:0x61ab20` sound gate, both `ret 0x10`, and both end `mov ecx,[this+0x10];
//     call 0x1360d0` - one player slot, one play entrypoint;
//   owner: CDDrawSubMgrLeafScan::Fire dispatches TriggerBlit on a value out of the
//     very map whose sibling accessor GetFirstValue returns LeafCue*.
// The fold is: declare TriggerBlit on LeafCue, retype m_ctx -> OwnerMgr() and
// m_soundPlayer -> m_10, move the 0x1587f0 body to LeafCue::TriggerBlit, delete this
// class. It needs the two DDrawSubMgr.cpp edits (Fire @958, the body @1179).
class CAniBlitTrigger {
public:
    i32 TriggerBlit(
        i32 pos,
        i32 center,
        i32 range1,
        i32 range2
    );                  // 0x1587f0  __thiscall on the cursor (G obj)
    char m_pad00[0x0c]; // +0x00..0x0b
    // The geometry context IS the world/display root: its +0x24 level -> +0x5c main
    // plane -> +0x84 snappedX and +0x04 drawTarget -> +0x10 frontPair -> +0x10 width
    // are exactly CDDrawSurfaceMgr's modeled chain (typed 2026-07-19).
    class CDDrawSurfaceMgr* m_ctx;  // +0x0c geometry context (the world root)
    DSoundCloneInst* m_soundPlayer; // +0x10 sound player
};
SIZE_UNKNOWN();

#endif // GRUNTZ_DDRAWMGR_ANIADVANCE_H
