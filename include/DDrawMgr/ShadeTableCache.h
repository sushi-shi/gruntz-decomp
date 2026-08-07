#ifndef GRUNTZ_DDRAWMGR_SHADETABLECACHE_H
#define GRUNTZ_DDRAWMGR_SHADETABLECACHE_H

#include <rva.h>

#include <DDrawMgr/ShadeMode.h>
#include <Enums.h>
#include <Wap32/Object.h>

class CFile;
class CString;

struct CShadeTable {
    i32 m_alloc;
    i32 m_size;

    union {
        u8* m_data;
        u16* m_lut16;
    };
    i32 m_key;

    u16* Lut16() const {
        return m_lut16;
    }

    CShadeTable();
    i32 Set(u32 size, i32 id);
    void Reset();
    void Free();

    i32 LoadFromFile(CString path, i32 id);
    i32 LoadFromMem(void* buf, u32 len, i32 id);
    i32 ReadFrom(CFile* file, i32 id);
    i32 SaveToFile(CString path);
};
SIZE(0x10);

struct CShadeTableArray : CObject {
    CShadeTable** m_pData;
    i32 m_nSize;
    i32 m_nMaxSize;
    i32 m_nGrowBy;

    CShadeTableArray();
    virtual ~CShadeTableArray() OVERRIDE;
    virtual void Serialize(CArchive& ar) OVERRIDE;

    void SetSizeGrow(i32 n, i32 grow);
};
SIZE(0x14);
SIZE(0x14);

class CShadeTableCache {
public:
    CShadeTableCache();
    ~CShadeTableCache();
    i32 Init();
    void FreeNodes();

    CShadeTable* FlashTable(PALETTEENTRY* pal, i32 nA, i32 nB, i32 startPct, i32 endPct);
    CShadeTable* HsvShiftTable(PALETTEENTRY* pal, i32 steps, i32 pct, i32 gamma, i32 baseArg);
    CShadeTable* HueRampTable(PALETTEENTRY* pal, i32 steps, i32 packedColor);
    CShadeTable* GammaTable(PALETTEENTRY* pal, i32 wRow, i32 wCol);
    CShadeTable* LumaSortTable(PALETTEENTRY* pal);
    CShadeTable* HueSortTable(PALETTEENTRY* pal);
    CShadeTable* AddFromArray(CString name);
    CShadeTable* AddFromFile(const char* name, i32 size);
    CShadeTable* GreyTable();
    CShadeTable* AddTable(float scale);
    CShadeTable* SubTable(i32 color);
    CShadeTable* AlphaTable(PALETTEENTRY* pal);
    CShadeTable* FindByKey(i32 key);
    void FindRemove(CShadeTable* t);

    static i32 __cdecl CompareLuma(const void* a, const void* b);

    static i32 __cdecl CompareHue(const void* a, const void* b);

    static i32 __cdecl FindNearestColor(PALETTEENTRY* pal, i32 r, i32 g, i32 b);

    i32 m_initialized;
    CShadeTableArray m_arr;
};
SIZE(0x18);

// retail inlines this per call site (no out-of-line body exists)
static __inline u8 NearestPaletteIndex(i32 r, PALETTEENTRY* pal, i32 g, i32 b) {
    i32 best = 0;
    i32 bestDist = 0x7fffffff;
    for (i32 i = 0; i < 256; i++) {
        i32 dr = r - pal[i].peRed;
        i32 dg = g - pal[i].peGreen;
        i32 db = b - pal[i].peBlue;
        i32 d = dr * dr + dg * dg + db * db;
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return static_cast<u8>(best);
}

extern float g_one;
extern float g_255;
extern float g_p01;
extern float g_lumaR;
extern float g_lumaG;
extern float g_lumaB;
extern float g_inv255;
#endif // GRUNTZ_DDRAWMGR_SHADETABLECACHE_H
