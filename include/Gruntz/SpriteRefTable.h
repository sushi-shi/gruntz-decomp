#ifndef GRUNTZ_SPRITEREFTABLE_H
#define GRUNTZ_SPRITEREFTABLE_H

#include <Ints.h>
#include <DDrawMgr/ShadeTableCache.h> // CShadeTable - the alpha/shade table
#include <rva.h>

class CSpriteRef {
public:
    i32 Build(CShadeTableCache* cache, void* shade, i32 kind); // 0xe2df0, ret 0xc
    void Free();                                               // 0xe32e0
    CShadeTableCache* m_cache;                                 // +0x00
    CShadeTable* m_alphaKey; // +0x04  the alpha/shade table (returned by GetSel)
    u16 m_teamColor1;        // +0x08  192/255 shade
    u16 m_teamColor3;        // +0x0a  128/255 shade
    u16 m_teamColor2;        // +0x0c  full intensity
    u16 m_pad0e;             // +0x0e  pad to 0x10
};
SIZE_UNKNOWN();

// (The ex CLookupResult / CLookupSprite reader views are dissolved - see the note on
// Add() below: the map value IS a CAniRecordBase2 and its +0x10 IS a CDDPalette.)

class CShadeTableCache;

class CDDrawSurfaceMgr;

class CSymParser; // fwd
class CSpriteRefTable {
public:
    // Cache the two engine sub-objects (m_factory, m_spriteMgrHolder) and clear m_built;
    // returns 1 (FALSE only when `cache` is null). 0xe2250. The two params were `i32`
    // (with a reinterpret_cast per store) until the sole retail caller - CGruntzMgr::Run
    // @0x84537 - was read: it pushes m_world (+0x30) then m_shadeCache (+0x50) straight in.
    i32 Init(CShadeTableCache* cache, CDDrawSurfaceMgr* holder);

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
    // (the sprite/frame pointer), or null. 0xe23c0.
    CShadeTable* GetSel(i32 i, i32 bAlt); // 0x0e23c0 (out-of-line)

    // Look the named PALETTE up in m_spriteMgrHolder's worker map, build a CSpriteRef of
    // the given kind from it, and return the node (null on miss / alloc fail). 0xe2890.
    //
    // The map value's identity is SETTLED (2026-07-27), replacing the @identity-TODO that
    // claimed "it is NOT CAniRecordBase2": every writer of CDDrawWorkerMapSmall::m_map1 -
    // Factory_1658c0 / CreateWorker28 / the two siblings, i.e. all of them - stores a
    // `new CAniRecordBase2`, and LoadGruntzPalette below is what put these very entries
    // in. So the ex CLookupResult IS CAniRecordBase2 (its +0x10 is m_buf) and the ex
    // CLookupSprite IS CDDPalette (its +0x0c is m_cacheA, the live 0x400-byte
    // PALETTEENTRY cache) - which is exactly what CShadeTableCache::AlphaTable wants.
    CSpriteRef* Add(char* szName, i32 kind);

    // Register a level's "GRUNTZ_PALETTEZ_<name>" palette into the sprite registry
    // (m_spriteMgrHolder->m_spriteMgr). src is the source resolver, name the level/name string. 0xe2d10.
    i32 LoadGruntzPalette(CSymParser* src, const char* name);

    // Register every tool/toy color palette (34 fixed names) via LoadGruntzPalette;
    // returns 1 only when all succeed (short-circuits to 0 on the first miss). 0xe2980.
    i32 LoadToolToyPalettes(CSymParser* src);

    // Build the 17-color tool/toy sprite-ref table: register the palettes
    // (LoadToolToyPalettes) then Add() each color's TOOL/TOY sprite into bucket A/B
    // at the color's kind slot; latches m_built when complete. 0xe2400.
    i32 BuildToolToyColorTable(CSymParser* src);

    CShadeTableCache* m_factory;         // +0x00  Init arg0 (the alpha/shade-table factory)
    CDDrawSurfaceMgr* m_spriteMgrHolder; // +0x04  Init arg1 (holder->m_workerMap = the sprite mgr)
    CSpriteRef* m_refA[0x11];            // +0x08  bucket A nodes (17 slots)
    CSpriteRef* m_refB[0x11];            // +0x4c  bucket B nodes (17 slots)
    i32 m_built;                         // +0x90  count/flag (reset to 0 on Init/Clear)
};
SIZE_UNKNOWN();

#endif // GRUNTZ_SPRITEREFTABLE_H
