// ShadeTableCache.h - the DDrawMgr color/shade lookup-table cache (tracer
// placeholder ClassUnknown_2). A CGruntzMgr member (heap-allocated at CGruntzMgr
// member +0x50, size 0x18): a polymorphic owner of a growable array of 0x10-byte
// CShadeTable buffers, each a memory-buffer wrapper holding a 64KB RGB565 (or
// raw-RGB) color-conversion / translucency table. The four builder methods
// (GreyTable/AddTable/SubTable/AlphaTable) each allocate a CShadeTable, push it
// into the array, and fill it from the live screen RGB-format globals at
// 0x683ea0..0x683eb4 (the same g_rUp/g_gUp/g_rDown/g_gDown/g_bDown shared with
// CDDrawShadeBlit). FindRemove(key) unlinks+destroys a table by its key field.
//
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing. The class is a leading gate field m_initialized (+0x00) followed by the
// embedded polymorphic element-array subobject m_arr (+0x04): its vtable at
// +0x04, m_pData at +0x08, m_nSize at +0x0c, m_nMaxSize at +0x10, m_nGrowBy at
// +0x14. The cache's destructor inlines the array subobject's teardown (restore
// vtable, free m_pData, restore the grand-base vtable) under the /GX EH frame.
// Engine callees (the array grow / element ctor/alloc/free helpers, operator
// new/delete) are reloc-masked.
#ifndef GRUNTZ_DDRAWMGR_SHADETABLECACHE_H
#define GRUNTZ_DDRAWMGR_SHADETABLECACHE_H

#include <Ints.h>

// A 0x10-byte memory-buffer wrapper (the array element). The ctor zeros
// m_alloc/m_size/m_data; 0x1501a0 Alloc(size,key) frees+reallocs m_data; 0x1503c0
// / 0x150190 free it. m_alloc(+0) gates teardown, m_size(+4) the byte size,
// m_data(+8) the buffer, m_key(+c) the lookup key. (Bodies live in the sibling
// element TU; declared here so the builder calls bind by shape, reloc-masked.)
struct CShadeTable {
    i32 m_alloc; // +0x00
    i32 m_size;  // +0x04
    u8* m_data;  // +0x08
    i32 m_key;   // +0x0c

    CShadeTable();
    void Free();              // 0x150190
    i32 Alloc(i32 sz, i32 k); // 0x1501a0 -> bool
    void Destroy();           // 0x1503c0
    // The two AddFrom* element loaders (sibling element TU, reloc-masked).
    i32 Load(struct CStr& s, i32 key);                 // 0x150250
    i32 LoadFile(struct CMemFile* f, i32 size, i32 k); // 0x150330
};

// The growable element-array subobject (lives at cache +0x04). ALL-VTABLES phase:
// modeled as a REAL 2-level polymorphic hierarchy (like CAniElementBase) so cl
// auto-emits both vtables + auto-stamps/resets the vptr:
//   CShadeArrayBase  - the CObArray-like grand base, masks ??_7CObject@@6B@
//                      (g_remusBaseDtorVtbl @0x5e8cb4); 5 CObject-interface slots.
//   CShadeTableArray - the element array, masks ??_7CShadeTableArray (0x5efb28);
//                      overrides slot 1 (dtor 0x150020) + slot 2 (0x14fe90).
// The cache ctor/dtor inline the array ctor/dtor: cl stamps 0x5efb28 in the ctor,
// resets 0x5efb28 then chains the CObject grand-base reset (0x5e8cb4) in the dtor.
struct CShadeArrayBase {
    virtual void Vf0();         // [0] 0x1bef01
    virtual ~CShadeArrayBase(); // [1] CObject dtor slot
    virtual void Vf2();         // [2] 0x0028ec
    virtual void Vf3();         // [3] 0x00106e
    virtual void Vf4();         // [4] 0x004034

    // vptr @ +0x00 (cache +0x04); fields follow.
    CShadeTable** m_pData; // +0x04 (cache +0x08)
    i32 m_nSize;           // +0x08 (cache +0x0c)
    i32 m_nMaxSize;        // +0x0c (cache +0x10)
    i32 m_nGrowBy;         // +0x10 (cache +0x14)
};

struct CShadeTableArray : CShadeArrayBase {
    CShadeTableArray();
    ~CShadeTableArray();               // 0x150020  overrides slot 1
    void Vf2();                        // 0x14fe90  overrides slot 2 (declared-only)
    void SetSizeGrow(i32 n, i32 grow); // 0x150040
};

// The live 256-entry RGB palette base (0x6bf224), each entry a 4-byte
// {r,g,b,pad} record. The sort/remap builders publish the working palette here
// for their __cdecl qsort comparators; the hue comparator (0x14fa60) and the
// RGB->HSV helper (0x14fcc0) read it. Reloc-masked DATA.
struct PalEntry {
    u8 r, g, b, pad;
};

class CShadeTableCache {
public:
    CShadeTableCache();  // 0x14de30
    ~CShadeTableCache(); // 0x14de50
    i32 Init();          // 0x14dec0
    void FreeNodes();    // 0x14ded0
    // 0x14df40 - a two-phase per-palette brightness-pulse ramp (fade-in over nA
    // steps, +16 highlight, fade-out over nB steps), mapped to nearest palette.
    CShadeTable* FlashTable(PalEntry* pal, i32 nA, i32 nB, i32 startPct, i32 endPct);
    CShadeTable* HsvShiftTable(PalEntry* pal, i32 steps, i32 packedColor); // 0x14e540
    CShadeTable* HueRampTable(PalEntry* pal, i32 steps, i32 packedColor);  // 0x14e830
    CShadeTable* GammaTable(PalEntry* pal, i32 wRow, i32 wCol);            // 0x14e9f0
    CShadeTable* LumaSortTable(PalEntry* pal);                             // 0x14ec00
    CShadeTable* HueSortTable(PalEntry* pal);                              // 0x14ede0
    CShadeTable* AddFromArray(const char* name);                           // 0x14f6c0
    CShadeTable* AddFromFile(const char* name, i32 size);                  // 0x14f8b0
    CShadeTable* GreyTable();                                              // 0x14eef0
    CShadeTable* AddTable(float scale);                                    // 0x14f080
    CShadeTable* SubTable(i32 color);                                      // 0x14f310
    CShadeTable* AlphaTable(u8* pal);                                      // 0x14f5b0
    void FindRemove(CShadeTable* t);                                       // 0x14fb80

    // 0x14fa60 - __cdecl qsort comparator: sort palette indices by hue.
    static i32 __cdecl CompareHue(const void* a, const void* b);

    // 0x14fbf0 - __cdecl nearest-color search: scan all 256 entries of `pal` for
    // the one minimizing the squared (r,g,b) distance to the target, return its
    // index. The shade-table builders use it to remap a source color into `pal`.
    static i32 __cdecl FindNearestColor(PalEntry* pal, i32 r, i32 g, i32 b);

    i32 m_initialized;      // +0x00 gate
    CShadeTableArray m_arr; // +0x04 element array subobject
};

#endif // GRUNTZ_DDRAWMGR_SHADETABLECACHE_H
