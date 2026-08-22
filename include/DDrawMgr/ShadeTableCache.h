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
    i32 LoadFromMem(u8* buf, u32 len, i32 id);
    i32 ReadFrom(CFile* file, i32 id);
    i32 SaveToFile(CString path);
};

struct CShadeTableArray : CObject {
    CShadeTable** m_pData;
    i32 m_nSize;
    i32 m_nMaxSize;
    i32 m_nGrowBy;

    CShadeTableArray();
    virtual ~CShadeTableArray() OVERRIDE;
    virtual void Serialize(CArchive& ar) OVERRIDE;

    void SetSizeGrow(i32 n, i32 grow);

    i32 GetSize() const {
        return m_nSize;
    }

    CShadeTable*& operator[](i32 i) {
        return m_pData[i];
    }
};

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
    CShadeTable* AddFromBuffer(u8* data, i32 size);
    CShadeTable* GreyTable();
    CShadeTable* AddTable(float scale);
    CShadeTable* SubTable(i32 color);
    CShadeTable* AlphaTable(PALETTEENTRY* pal);
    CShadeTable* FindByKey(i32 key);
    void FindRemove(CShadeTable* t);

    static i32 __cdecl CompareLuma(const void* a, const void* b);

    static i32 __cdecl CompareHue(const void* a, const void* b);

    static i32 __cdecl FindNearestColor(PALETTEENTRY* pal, u8 r, u8 g, u8 b);

    i32 m_initialized;
    CShadeTableArray m_arr;
};

extern const float g_one;
extern const float g_255;
extern const float g_p01;
extern const float g_lumaR;
extern const float g_lumaG;
extern const float g_lumaB;
extern const float g_inv255;
#endif // GRUNTZ_DDRAWMGR_SHADETABLECACHE_H
