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
// load-bearing. The class is a leading gate field m_00 (+0x00) followed by the
// embedded polymorphic element-array subobject m_arr (+0x04): its vtable at
// +0x04, m_pData at +0x08, m_nSize at +0x0c, m_nMaxSize at +0x10, m_nGrowBy at
// +0x14. The cache's destructor inlines the array subobject's teardown (restore
// vtable, free m_pData, restore the grand-base vtable) under the /GX EH frame.
// Engine callees (the array grow / element ctor/alloc/free helpers, operator
// new/delete) are reloc-masked.
#ifndef GRUNTZ_DDRAWMGR_SHADETABLECACHE_H
#define GRUNTZ_DDRAWMGR_SHADETABLECACHE_H

#include <Ints.h>

// A 0x10-byte memory-buffer wrapper (the array element). 0x150180 ctor zeros
// m_alloc/m_size/m_data; 0x1501a0 Alloc(size,key) frees+reallocs m_data; 0x1503c0
// / 0x150190 free it. m_alloc(+0) gates teardown, m_size(+4) the byte size,
// m_data(+8) the buffer, m_key(+c) the lookup key. (Bodies live in the sibling
// element TU; declared here so the builder calls bind by shape, reloc-masked.)
struct CShadeTable {
    i32 m_alloc; // +0x00
    i32 m_size;  // +0x04
    u8* m_data;  // +0x08
    i32 m_key;   // +0x0c

    CShadeTable* Ctor();      // 0x150180 (returns this)
    void Free();              // 0x150190
    i32 Alloc(i32 sz, i32 k); // 0x1501a0 -> bool
    void Destroy();           // 0x1503c0
};

// The growable element-array subobject (lives at class +0x04). Polymorphic: its
// own vftable is at +0x00. Its destructor restores the array vtable, frees
// m_pData, and restores the grand-base vtable - the cache dtor inlines this.
struct CShadeTableArray {
    void* m_vtbl;          // +0x00 (parent +0x04)
    CShadeTable** m_pData; // +0x04 (parent +0x08)
    i32 m_nSize;           // +0x08 (parent +0x0c)
    i32 m_nMaxSize;        // +0x0c (parent +0x10)
    i32 m_nGrowBy;         // +0x10 (parent +0x14)

    CShadeTableArray();
    ~CShadeTableArray();
    void SetSizeGrow(i32 n, i32 grow); // 0x150040
};

class CShadeTableCache {
public:
    CShadeTableCache();                 // 0x14de30
    ~CShadeTableCache();                // 0x14de50
    i32 Init();                         // 0x14dec0
    void FreeNodes();                   // 0x14ded0
    CShadeTable* GreyTable();           // 0x14eef0
    CShadeTable* AddTable(float scale); // 0x14f080
    CShadeTable* SubTable(i32 color);   // 0x14f310
    CShadeTable* AlphaTable(u8* pal);   // 0x14f5b0
    void FindRemove(CShadeTable* t);    // 0x14fb80

    i32 m_00;               // +0x00 gate
    CShadeTableArray m_arr; // +0x04 element array subobject
};

#endif // GRUNTZ_DDRAWMGR_SHADETABLECACHE_H
