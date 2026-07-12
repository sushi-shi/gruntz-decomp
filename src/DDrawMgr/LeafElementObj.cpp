// LeafElementObj.cpp - a small leaf game-element object that loads a sound asset
// on demand through the global SoundDevice (Dsndmgr). Each load method reaches the
// owning level/world object (m_c), pulls its SoundDevice (owner+0x20), and - if the
// device is live - asks it to acquire/decode a sound from the element's source blob
// (m_8), caching the resulting buffer wrapper at m_10. Returns whether the cache slot
// was populated.
//
// Two parallel loaders differ only in the SoundDevice entry point they call
// (Acquire @0x136910 vs the sibling decoder @0x136860); both pass the same
// (source, 0x100ea, 0) argument triple. Field names are placeholders; only the
// OFFSETS + emitted code bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------
#include <rva.h>
#include <Dsndmgr/SoundDevice.h>

#include <Ints.h>

// The owning level/world object reached through this+0xc; its SoundDevice hangs
// off +0x20. Modeled as an opaque view (offsets load-bearing).
struct SoundDevice; // defined below
struct LeafSoundOwner {
    char m_pad0[0x20];
    SoundDevice* m_sound; // +0x20  the global SoundDevice (null until brought up)
};

// The SoundDevice (Dsndmgr) the loaders dispatch into. Only the two acquire/decode
// entry points are modeled (no body -> their `call rel32` reloc-masks). __thiscall.

class LeafElementObj {
public:
    i32 LoadSoundA(void* src); // 0x1586e0
    i32 LoadSoundB(void* src); // 0x158720

    char m_pad0[0x0c];   // +0x00
    LeafSoundOwner* m_c; // +0x0c  owning level/world object
    void* m_10;          // +0x10  cached acquired sound-buffer wrapper
};

// ===========================================================================

// acquire the element's sound through SoundDevice::Acquire; cache it.
RVA(0x001586e0, 0x34)
i32 LeafElementObj::LoadSoundA(void* src) {
    SoundDevice* dev = m_c->m_sound;
    if (!dev) {
        return 0;
    }
    m_10 = dev->Acquire(src, 0x100ea, 0);
    return m_10 != 0;
}

// acquire the element's sound through SoundDevice::Decode; cache it.
RVA(0x00158720, 0x34)
i32 LeafElementObj::LoadSoundB(void* src) {
    SoundDevice* dev = m_c->m_sound;
    if (!dev) {
        return 0;
    }
    m_10 = dev->AcquireFile((char*)src, 0x100ea, 0);
    return m_10 != 0;
}

SIZE_UNKNOWN(SoundDevice);
SIZE_UNKNOWN(LeafSoundOwner);
