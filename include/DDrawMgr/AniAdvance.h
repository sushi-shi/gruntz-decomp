#ifndef GRUNTZ_DDRAWMGR_ANIADVANCE_H
#define GRUNTZ_DDRAWMGR_ANIADVANCE_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/Sprite.h> // CDDrawWorker - the bound frame sequence (CWwdGameObjectA::m_sprite)

struct LeafCue; // <Gruntz/LeafCue.h> - the +0x30 random cue table's element

class DSoundCloneInst; // the pooled cue player (ex DSoundCloneInst; Dsndmgr/DirectSoundMgr.h)

// (CAniRenderCtx DISSOLVED 2026-07-27: it was CWwdGameObjectA read through pads - the
// owner CAniAdvanceCursor::m_10 points back at. PROVEN by the cursor's only binder,
// CWwdGameObjectA::Setup @0x15b940, which does `push esi / lea ecx,[esi+0x1a0] / call
// Construct` - it passes its own `this`. Every field matched an existing member:
//   +0x08 m_flags      -> CLoadable::m_flags
//   +0x10/+0x14        -> CResolveNode::m_10 / m_14 (the per-frame plot deltas)
//   +0x38 m_anchor     -> CResolveNode::m_dirty.m_armed (the -1 "disarmed" sentinel it tests)
//   +0x40 m_byteFlags  -> CResolveNode::m_stateFlags
//   +0x5c/+0x60        -> CResolveNode::m_screenX / m_screenY (same names already)
//   +0x190/+0x194/+0x198 -> CWwdGameObjectA::m_190 / m_sprite / m_layer
// Its ClampFirst/ClampLast (0x15cc50/0x15cc90) are CWwdGameObjectA methods now.)

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
