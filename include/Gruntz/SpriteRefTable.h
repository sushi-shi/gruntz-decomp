// SpriteRefTable.h - the game-registry sprite/animation reference table (trace
// placeholder tomalla-10; recovered from the 7-method __thiscall cluster at
// 0xe2250, 0xe2290, 0xe22d0, 0xe2360, 0xe2390, 0xe23c0, 0xe2890).
//
// A WAP32 game-registry sub-object (lives at g_gameReg+0x74; built in the game
// bootstrap at 0x83450). It maps a small kind enum (0..16, i.e. 17 slots) to two
// parallel buckets of CSpriteRef nodes: m_refA[17] (+0x08) and m_refB[17] (+0x4c)
// -- a "normal" and an "alternate" set. Init(p0,p1) caches two engine sub-objects
// (m_factory = sprite source, m_spriteMgrHolder = the sprite-mgr holder), Add() looks a named sprite
// up in m_spriteMgrHolder's CMapStringToOb and builds a CSpriteRef from it, and GetSel(i,bAlt)
// returns the resolved sprite/frame pointer of the chosen bucket's node -- the hot
// accessor (~30 call sites, incl. BuildGruntSprintAnimation). m_built (+0x90) is a
// count/flag reset to 0 on Init/Clear.
//
// No vtable: none of the 7 methods is referenced from a vftable and no ctor stamps
// one - plain __thiscall methods, no /GX frame. Field names are placeholders; only
// offsets + code bytes are load-bearing.
#ifndef GRUNTZ_SPRITEREFTABLE_H
#define GRUNTZ_SPRITEREFTABLE_H

#include <Ints.h>
#include <rva.h>

// The 0x10-byte sprite/animation reference node (trace placeholder tomalla-42,
// the +0x8/+0x4c bucket element). Build (0xe2df0) caches a CShadeTableCache (m_cache)
// + its CShadeTable alpha key (m_alphaKey) and bakes a 3-shade team-color triple
// (m_teamColor1 / m_teamColor3 / m_teamColor2, each an RGB565 pixel) from the `kind` enum (0..16); Free (0xe32e0)
// drops the table back to the cache via FindRemove and zeros both. __thiscall.
// Bodies live in the sibling SpriteRef.cpp; modeled NO-body here so this table's
// `call`s through them reloc-mask. Fields kept i32 so Add()/GetSel() stay byte-exact.
SIZE_UNKNOWN(CSpriteRef);
class CSpriteRef {
public:
    i32 Build(i32 cache, void* shade, i32 kind); // 0xe2df0, ret 0xc
    void Free();                                 // 0xe32e0
    i32 m_cache;                                 // +0x00  CShadeTableCache*
    i32 m_alphaKey;   // +0x04  CShadeTable* alpha key (returned by GetSel)
    u16 m_teamColor1; // +0x08  192/255 shade
    u16 m_teamColor3; // +0x0a  128/255 shade
    u16 m_teamColor2; // +0x0c  full intensity
    u16 m_pad0e;      // +0x0e  pad to 0x10
};

// The sprite name->object hash table (CMapStringToOb at +0x10 of the sprite mgr).
// Lookup is the engine's 0x1b8008; modeled NO-body so its `call` reloc-masks.
SIZE_UNKNOWN(CSpriteRefHashTable);
class CSpriteRefHashTable {}; // MFC CMapStringToPtr (Lookup @0x1b8008); cast at the call

// The animation/alpha factory cached as Init's arg0 (m_factory) IS the canonical
// CShadeTableCache (<DDrawMgr/ShadeTableCache.h>). XREF proof: its builder passes
// RezSync+0x50 (RezSync.cpp: `CShadeTableCache* m_50`) as arg0, and the "factory"
// method Add() calls on it - AlphaTable @0x14f5b0 - IS CShadeTableCache::AlphaTable
// (same RVA). The former CSpriteRefFactory placeholder is DISSOLVED (2026-07-14).
// Forward-declared for the pointer member; the deref TUs include ShadeTableCache.h.
class CShadeTableCache;

SIZE_UNKNOWN(CSpriteRefTable);
class CSpriteRefTable {
public:
    // Cache the two engine sub-objects (m_factory, m_spriteMgrHolder) and clear both buckets; returns
    // 1 (FALSE only when p0 is null). 0xe2250.
    i32 Init(i32 p0, i32 p1);

    // Free both buckets, then zero m_factory/m_spriteMgrHolder/m_built and re-null both bucket arrays
    // (the teardown / clear-all). 0xe2290.
    void Reset();

    // Free every CSpriteRef node in both buckets and re-null the slots; clears m_built.
    // 0xe22d0.
    void Clear();

    // Return bucket-A node for slot i (null if i out of [0,17)). 0xe2360.
    CSpriteRef* GetA(i32 i); // 0x0e2360 (out-of-line: i<0x11 ? m_refA[i] : 0)

    // Return bucket-B node for slot i (null if i out of [0,17)). 0xe2390.
    CSpriteRef* GetB(i32 i); // 0x0e2390 (out-of-line: i<0x11 ? m_refB[i] : 0)

    // Resolve slot i: pick bucket B when bAlt else bucket A, return its node's m_alphaKey
    // (the sprite/frame pointer), or null. 0xe23c0. (CPlay's BeginGridWalk names this
    // LoadSprite for its role and InGameIcon names it GetByIndex - all the same 0xe23c0.)
    i32 GetSel(i32 i, i32 bAlt); // 0x0e23c0 (out-of-line)

    // Load a sprite by descriptor (the CPlay grid-walk facet on this same +0x74 object):
    // desc is the per-grunt-type descriptor from the world config array, flag the variant.
    // Reloc-masked no-body (its retail RVA is unrecovered); modeled here so CPlay reaches
    // the +0x74 table cast-free instead of via a per-TU SpriteLoader view.
    void* LoadSprite(void* desc, i32 flag);

    // Look the named sprite up in m_spriteMgrHolder's hash table, build a CSpriteRef of the given
    // kind from it, and return the node (null on miss / alloc fail). 0xe2890.
    CSpriteRef* Add(char* szName, i32 kind);

    // Register a level's "GRUNTZ_PALETTEZ_<name>" palette into the sprite registry
    // (m_spriteMgrHolder->m_spriteMgr). src is the source resolver, name the level/name string. 0xe2d10.
    i32 LoadGruntzPalette(i32 src, i32 name);

    // Register every tool/toy color palette (34 fixed names) via LoadGruntzPalette;
    // returns 1 only when all succeed (short-circuits to 0 on the first miss). 0xe2980.
    i32 LoadToolToyPalettes(i32 src);

    // Build the 17-color tool/toy sprite-ref table: register the palettes
    // (LoadToolToyPalettes) then Add() each color's TOOL/TOY sprite into bucket A/B
    // at the color's kind slot; latches m_built when complete. 0xe2400.
    i32 BuildToolToyColorTable(i32 src);

    CShadeTableCache* m_factory; // +0x00  Init arg0 (the alpha/shade-table factory)
    void* m_spriteMgrHolder;      // +0x04  Init arg1 (holder->m_spriteMgr -> the sprite mgr)
    CSpriteRef* m_refA[0x11];     // +0x08  bucket A nodes (17 slots)
    CSpriteRef* m_refB[0x11];     // +0x4c  bucket B nodes (17 slots)
    i32 m_built;                  // +0x90  count/flag (reset to 0 on Init/Clear)
};

#endif // GRUNTZ_SPRITEREFTABLE_H
