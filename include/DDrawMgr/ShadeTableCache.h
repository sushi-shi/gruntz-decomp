#ifndef GRUNTZ_DDRAWMGR_SHADETABLECACHE_H
#define GRUNTZ_DDRAWMGR_SHADETABLECACHE_H

#include <rva.h>          // Ints + the OVERRIDE/SIZE/VTBL label macros
#include <Wap32/Object.h> // CObject - the MFC-free WAP grand-base (no windows.h dep)

class CFile;
class CString;

struct CShadeTable {
    i32 m_alloc; // +0x00  loaded/valid flag
    i32 m_size;  // +0x04
    u8* m_data;  // +0x08  byte/pixel buffer (RezAlloc'd blob)
    i32 m_key;   // +0x0c  id / lookup key

    CShadeTable();             // 0x150180
    i32 Set(u32 size, i32 id); // 0x1501a0
    void Reset();              // 0x150190
    void Free();               // 0x1503c0
    // Element loaders: LoadFromFile takes a filesystem path (via a CString temp the
    // caller builds); LoadFromMem takes a raw (buf,len,id) memory blob - it wraps
    // the buffer in its OWN CMemFile internally, so the caller passes the raw ptr.
    i32 LoadFromFile(const char* path, i32 id);  // 0x150250
    i32 LoadFromMem(void* buf, u32 len, i32 id); // 0x150330
    i32 ReadFrom(CFile* file, i32 id);           // 0x1501f0  (LoadFrom* wrap a local CFile)
    i32 SaveToFile(CString path);                // 0x1503f0  (completeness; no caller)
};
SIZE(0x10); // array-element stride (0x10-byte buffer wrapper)

struct CShadeTableArray : CObject {
    CShadeTable** m_pData; // +0x04 (cache +0x08)
    i32 m_nSize;           // +0x08 (cache +0x0c)
    i32 m_nMaxSize;        // +0x0c (cache +0x10)
    i32 m_nGrowBy;         // +0x10 (cache +0x14)

    CShadeTableArray();
    virtual ~CShadeTableArray() OVERRIDE;          // 0x150020  overrides CObject dtor slot 1
    virtual void Serialize(CArchive& ar) OVERRIDE; // slot 2  0x14fe90
    // slots 0/3/4 (GetRuntimeClass/AssertValid/Dump) inherited from MFC CObject
    void SetSizeGrow(i32 n, i32 grow); // 0x150040
};
SIZE(0x14); // MFC CObArray-shaped subobject (cache 0x18 - 0x04)
SIZE(0x14); // vptr + 4 array fields over the CObject base

struct PalEntry {
    u8 r, g, b, pad;
};
SIZE(0x4); // 4-byte palette record (256-entry array stride)

class CShadeTableCache {
public:
    CShadeTableCache();  // 0x14de30
    ~CShadeTableCache(); // 0x14de50
    i32 Init();          // 0x14dec0 (out-of-line: m_initialized = 1; return 1)
    void FreeNodes();    // 0x14ded0
    // 0x14df40 - a two-phase per-palette brightness-pulse ramp (fade-in over nA
    // steps, +16 highlight, fade-out over nB steps), mapped to nearest palette.
    CShadeTable* FlashTable(PalEntry* pal, i32 nA, i32 nB, i32 startPct, i32 endPct);
    CShadeTable*
    HsvShiftTable(PalEntry* pal, i32 steps, i32 pct, i32 gamma, i32 baseArg); // 0x14e540
    CShadeTable* HueRampTable(PalEntry* pal, i32 steps, i32 packedColor);     // 0x14e830
    CShadeTable* GammaTable(PalEntry* pal, i32 wRow, i32 wCol);               // 0x14e9f0
    CShadeTable* LumaSortTable(PalEntry* pal);                                // 0x14ec00
    CShadeTable* HueSortTable(PalEntry* pal);                                 // 0x14ede0
    CShadeTable* AddFromArray(const char* name);                              // 0x14f6c0
    CShadeTable* AddFromFile(const char* name, i32 size);                     // 0x14f8b0
    CShadeTable* GreyTable();                                                 // 0x14eef0
    CShadeTable* AddTable(float scale);                                       // 0x14f080
    CShadeTable* SubTable(i32 color);                                         // 0x14f310
    CShadeTable* AlphaTable(u8* pal);                                         // 0x14f5b0
    CShadeTable* FindByKey(i32 key);                                          // 0x14fb40
    void FindRemove(CShadeTable* t);                                          // 0x14fb80

    // 0x14ed10 - __cdecl qsort comparator: sort palette indices by luma.
    static i32 __cdecl CompareLuma(const void* a, const void* b);

    // 0x14fa60 - __cdecl qsort comparator: sort palette indices by hue.
    static i32 __cdecl CompareHue(const void* a, const void* b);

    // 0x14fbf0 - __cdecl nearest-color search: scan all 256 entries of `pal` for
    // the one minimizing the squared (r,g,b) distance to the target, return its
    // index. The shade-table builders use it to remap a source color into `pal`.
    static i32 __cdecl FindNearestColor(PalEntry* pal, i32 r, i32 g, i32 b);

    i32 m_initialized;      // +0x00 gate
    CShadeTableArray m_arr; // +0x04 element array subobject
};
SIZE(0x18); // RE'd heap-alloc size (CGruntzMgr +0x50)

#endif // GRUNTZ_DDRAWMGR_SHADETABLECACHE_H
