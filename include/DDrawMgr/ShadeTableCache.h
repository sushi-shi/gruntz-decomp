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

#include <rva.h>          // Ints + the OVERRIDE/SIZE/VTBL label macros
#include <Wap32/Object.h> // CObject - the MFC-free WAP grand-base (no windows.h dep)

// A 0x10-byte memory-buffer wrapper (the array element). The ctor zeros
// m_alloc/m_size/m_data; 0x1501a0 Alloc(size,key) frees+reallocs m_data; 0x1503c0
// / 0x150190 free it. m_alloc(+0) gates teardown, m_size(+4) the byte size,
// m_data(+8) the buffer, m_key(+c) the lookup key. (Bodies live in the sibling
// element TU; declared here so the builder calls bind by shape, reloc-masked.)
// CShadeTable is the canonical 0x10-byte shade-table buffer (retail name, proven by
// the builder call relocs): 0x150180 ctor / 0x1501a0 Set / 0x150190 Reset / 0x1503c0
// Free / 0x150250 LoadFromFile / 0x150330 LoadFromMem / 0x1501f0 ReadFrom / 0x1503f0
// SaveToFile. The out-of-line bodies live in the DataBuffer.cpp TU. This header stays
// MFC-free (no <Mfc.h>) so the pure-Win32 includers still compile - the two
// MFC-signature methods (ReadFrom/SaveToFile) use forward-declared CFile/CString.
// The MFC file types the buffer readers use in their bodies (this header + its many
// pure-Win32 includers stay MFC-free; these are only complete in DataBuffer.cpp, which
// pulls <Mfc.h>). Forward-declared so the by-value/by-ptr method DECLS below need no MFC.
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

// The growable element-array subobject (lives at cache +0x04). CShadeTableArray is a
// real RTTI class (??_7CShadeTableArray @0x5efb28) that derives from the shared WAP
// grand-base CObject (RTTI "CObject", grand-base vtable @0x5e8cb4). Its 5-slot
// vtable is CObject's prefix (GetRuntimeClass / dtor / Serialize / AssertValid / Dump):
// slots 0/3/4 are inherited, and it overrides slot 1 (dtor 0x150020) + slot 2
// (Serialize 0x14fe90 = Serialize). dump_target proves the classic 2-level CObject
// codegen - the cache ctor (0x14de30) stamps ONLY 0x5efb28 (the dead CObject base stamp
// elided), the dtor (0x14de50) stamps 0x5efb28 then the CObject-destruction base
// (0x5e8cb4).
//
// CObject is the MFC-free engine CObject (namespace Wap, no windows.h dependency) -
// the same grand-base the Net nodes / CGruntzSoundInnerZ derive from - so this header
// compiles in the PURE-WIN32 includers (LightEffectSetup.cpp pulls <Win32.h> = windows.h
// FIRST, where <Mfc.h>'s real CObject HARD-ERRORS) as well as the MFC ones. cl inherits
// the 5 base slots, auto-emits both vtables + auto-stamps/resets the vptr, reproducing
// retail's exact stamp schedule (the cache ctor/dtor are already 100%).
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
// SIZE/VTBL for CShadeTableArray are kept in ShadeTableCache.cpp (out of this
// windows.h-adjacent header, whose includers pull the Win32 SIZE struct type).

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

#endif // GRUNTZ_DDRAWMGR_SHADETABLECACHE_H
