#ifndef GRUNTZ_DDRAWMGR_ANIADVANCE_H
#define GRUNTZ_DDRAWMGR_ANIADVANCE_H

// AniAdvance.h - the CAniAdvanceCursor satellite advance types (wave4-L): the
// sprite-render context the cursor drives, the animation descriptor (playlist
// entry), and the per-frame blit/sound trigger overlay. Shared by the G obj
// (TriggerBlit @0x1587f0, src/DDrawMgr/DDrawSubMgr.cpp) and the I obj (Advance
// @0x15c360 + the Clamp* pair, src/Wwd/WwdFactoryObject.cpp) - dossier #15.
//
// Field names are placeholders; only OFFSETS + emitted code bytes are load-bearing.

#include <Ints.h>
#include <rva.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/Sprite.h> // CSprite - the active frame sequence (m_frameSeq)

class DSoundCloneInst; // the pooled cue player (ex DSoundCloneInst; Dsndmgr/DirectSoundMgr.h)

// The sprite-render context the cursor drives (held at cursor+0x10). ClampFirst/
// ClampLast (0x15cc50/0x15cc90) clamp its +0x190 cursor to the sequence ends and
// re-resolve +0x198.
class CAniRenderCtx {
public:
    void ClampFirst_15cc50(); // 0x15cc50  __thiscall on the context (I obj)
    void ClampLast_15cc90();  // 0x15cc90  __thiscall on the context (I obj)
    char m_pad00[0x08];       // +0x00..0x07
    i32 m_flags;              // +0x08  flags (bit 0x2000000 tested)
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
    CSprite* m_frameSeq;        // +0x194  active frame sequence
    i32 m_curFrame;             // +0x198  resolved current frame pointer
};
SIZE_UNKNOWN(CAniRenderCtx);

// The animation descriptor (cursor+0x18, a playlist entry). +0x08 step-mode keys
// the 7-way frame-step switch; +0x0c loop-mode word keys the 10-way next-descriptor
// switch (range-checked against the 9 sentinel); +0x2c/+0x30 drive a random
// per-frame trigger.
class CAniDesc {
public:
    char m_pad00[0x04];
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
    i32* m_randTable;          // +0x30  random-trigger table
};
SIZE_UNKNOWN(CAniDesc);

// The per-frame draw trigger (the context's +0x5c screen-X is the blit cue arg).
// Overlaid on the cursor: +0x0c context, +0x10 sound player.
class CAniBlitTrigger {
public:
    i32 TriggerBlit_1587f0(
        i32 pos,
        i32 center,
        i32 range1,
        i32 range2
    );                  // 0x1587f0  __thiscall on the cursor (G obj)
    char m_pad00[0x0c]; // +0x00..0x0b
    // authentic: geometry context, reached only by raw offset (+0x24 -> +0x5c -> +0x84,
    // +0x4); its class is not modeled here - kept void*.
    void* m_ctx;                    // +0x0c geometry context
    DSoundCloneInst* m_soundPlayer; // +0x10 sound player
};
SIZE_UNKNOWN(CAniBlitTrigger);

#endif // GRUNTZ_DDRAWMGR_ANIADVANCE_H
