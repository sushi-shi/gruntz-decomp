// SpriteRefTable.h - the game-registry sprite/animation reference table (trace
// placeholder ClassUnknown_10; recovered from the 7-method __thiscall cluster at
// 0xe2250, 0xe2290, 0xe22d0, 0xe2360, 0xe2390, 0xe23c0, 0xe2890).
//
// A WAP32 game-registry sub-object (lives at g_gameReg+0x74; built in the game
// bootstrap at 0x83450). It maps a small kind enum (0..16, i.e. 17 slots) to two
// parallel buckets of CSpriteRef nodes: m_refA[17] (+0x08) and m_refB[17] (+0x4c)
// -- a "normal" and an "alternate" set. Init(p0,p1) caches two engine sub-objects
// (m_00 = sprite source, m_04 = the sprite-mgr holder), Add() looks a named sprite
// up in m_04's CSpriteHashTable and builds a CSpriteRef from it, and GetSel(i,bAlt)
// returns the resolved sprite/frame pointer of the chosen bucket's node -- the hot
// accessor (~30 call sites, incl. BuildGruntSprintAnimation). m_90 (+0x90) is a
// count/flag reset to 0 on Init/Clear.
//
// No vtable: none of the 7 methods is referenced from a vftable and no ctor stamps
// one - plain __thiscall methods, no /GX frame. Field names are placeholders; only
// offsets + code bytes are load-bearing.
#ifndef GRUNTZ_SPRITEREFTABLE_H
#define GRUNTZ_SPRITEREFTABLE_H

#include <Ints.h>
#include <rva.h>

// The 0x10-byte sprite/animation reference node (trace placeholder ClassUnknown_42,
// the +0x8/+0x4c bucket element). Its ctor (0xe2df0) and dtor (0xe32e0) live in a
// sibling cluster; modeled NO-body so this table's `call`s through them reloc-mask.
// __thiscall throughout: Build(p0, sprite, kind) -> BOOL, Free() the teardown.
class CSpriteRef {
public:
    i32 Build(i32 p0, void* sprite, i32 kind); // 0xe2df0, ret 0xc
    void Free();                               // 0xe32e0
    i32 m_00;                                  // +0x00
    i32 m_04; // +0x04  resolved sprite/frame pointer (returned by GetSel)
    char m_pad08[0x10 - 0x8];
};

// The sprite name->object hash table (CSpriteHashTable at +0x10 of the sprite mgr).
// Lookup is the engine's 0x1b8008; modeled NO-body so its `call` reloc-masks.
class CSpriteRefHashTable {
public:
    i32 Lookup(char* szName, void** ppOut); // 0x1b8008
};

// The animation/alpha factory cached as Init's arg0 (m_00). Its AlphaTable method
// (0x14f5b0) turns a looked-up sprite's frame data into an alpha object; modeled
// NO-body so its `call` reloc-masks. Build (the node ctor) also receives it.
class CSpriteRefFactory {
public:
    void* AlphaTable(void* spriteData); // 0x14f5b0, __thiscall ret 0x4
};

class CSpriteRefTable {
public:
    // Cache the two engine sub-objects (m_00, m_04) and clear both buckets; returns
    // 1 (FALSE only when p0 is null). 0xe2250.
    i32 Init(i32 p0, i32 p1);

    // Free both buckets, then zero m_00/m_04/m_90 and re-null both bucket arrays
    // (the teardown / clear-all). 0xe2290.
    void Reset();

    // Free every CSpriteRef node in both buckets and re-null the slots; clears m_90.
    // 0xe22d0.
    void Clear();

    // Return bucket-A node for slot i (null if i out of [0,17)). 0xe2360.
    CSpriteRef* GetA(i32 i);

    // Return bucket-B node for slot i (null if i out of [0,17)). 0xe2390.
    CSpriteRef* GetB(i32 i);

    // Resolve slot i: pick bucket B when bAlt else bucket A, return its node's m_04
    // (the sprite/frame pointer), or null. 0xe23c0.
    i32 GetSel(i32 i, i32 bAlt);

    // Look the named sprite up in m_04's hash table, build a CSpriteRef of the given
    // kind from it, and return the node (null on miss / alloc fail). 0xe2890.
    CSpriteRef* Add(char* szName, i32 kind);

    CSpriteRefFactory* m_00;  // +0x00  Init arg0 (the alpha/anim factory)
    void* m_04;               // +0x04  Init arg1 (m_04->m_18 -> the sprite mgr)
    CSpriteRef* m_refA[0x11]; // +0x08  bucket A nodes (17 slots)
    CSpriteRef* m_refB[0x11]; // +0x4c  bucket B nodes (17 slots)
    i32 m_90;                 // +0x90  count/flag (reset to 0 on Init/Clear)
};

#endif // GRUNTZ_SPRITEREFTABLE_H
