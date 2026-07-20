#ifndef GRUNTZ_SPRITEREFTABLE_H
#define GRUNTZ_SPRITEREFTABLE_H

#include <Ints.h>
#include <rva.h>

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

SIZE_UNKNOWN(CSpriteRefHashTable);
class CSpriteRefHashTable {}; // MFC CMapStringToPtr (Lookup @0x1b8008); cast at the call

// Reduced reader views of the map value Add() resolves (`out`), only the two fields
// Add() reads. @identity-TODO: the sprite value's concrete class needs an xref chase
// from the sprite LOADER side (it is NOT CAniRecordBase2). Kept minimal until then.
struct CLookupSprite {
    char m_pad00[0xc];
    u8* m_frameData;   // +0x0c  the frame's raw RLE/pixel payload
};
struct CLookupResult {
    char m_pad00[0x10];
    CLookupSprite* m_sprite; // +0x10
};
SIZE_UNKNOWN(CLookupSprite);
SIZE_UNKNOWN(CLookupResult);

class CShadeTableCache;

class CDDrawSurfaceMgr;

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

    CShadeTableCache* m_factory;         // +0x00  Init arg0 (the alpha/shade-table factory)
    CDDrawSurfaceMgr* m_spriteMgrHolder; // +0x04  Init arg1 (holder->m_workerMap = the sprite mgr)
    CSpriteRef* m_refA[0x11];            // +0x08  bucket A nodes (17 slots)
    CSpriteRef* m_refB[0x11];            // +0x4c  bucket B nodes (17 slots)
    i32 m_built;                         // +0x90  count/flag (reset to 0 on Init/Clear)
};

#endif // GRUNTZ_SPRITEREFTABLE_H
