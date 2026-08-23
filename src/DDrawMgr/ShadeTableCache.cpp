#include <rva.h>

#include <DDrawMgr/ShadeTableCache.h>

#include <Win32.h>

#include <DDrawMgr/ColorHsv.h>
#include <DDrawMgr/PaletteSize.h>
#include <DDrawMgr/PixelFormatMacros.h>
#include <DDrawMgr/PixelShift.h>
#include <Enums.h>
#include <Ints.h>
#include <Pix16.h>

#include <afxtempl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// TU-STATE FINGERPRINT (diagnostic; probes are never shipped). Sweeping a
// throwaway file-scope declaration above the first definition - N = 1..16 graded
// prototypes, and one each of fwd-decl / typedef / empty class / class with a
// member / class with inline member bodies / static function with a body /
// file-scope float / string literal - moves exactly ONE of this unit's functions
// (CompareLuma, by 0.03) and, at N = 4, GammaTable. Every other function here is
// dead flat to the whole axis. So this unit's residue is intra-function, not TU
// composition - which is where the CompareLuma/CompareHue/FindNearestColor/
// GammaTable body fixes came from. Do not re-run the sweep on this file.
#define HSV_MAX(a, b) ((a) > (b) ? (a) : (b))
#define HSV_MIN(a, b) ((a) < (b) ? (a) : (b))

DATA(0x002bf224)
PALETTEENTRY* g_pal = NULL;

DATA(0x001efb40)
const float g_one = 1.0f;
DATA(0x001efb44)
const float g_255 = 255.0f;
DATA(0x001efb48)
const float g_p01 = 0.01f;
DATA(0x001efb4c)
const float g_lumaR = 0.5859375f;
DATA(0x001efb50)
const float g_lumaG = 0.296875f;
DATA(0x001efb54)
const float g_lumaB = 0.109375f;
DATA(0x001efb58)
const float g_inv255 = 0.003921568859368563f;
DATA(0x001efb5c)
const float g_negone = -1.0f;

inline CShadeTableArray::CShadeTableArray() {

    m_pData = NULL;
    m_nGrowBy = 0;
    m_nMaxSize = 0;
    m_nSize = 0;
}

inline CShadeTableArray::~CShadeTableArray() {

    if (m_pData) {
        delete[] m_pData;
    }
}

RVA(0x0014de30, 0x1a)
CShadeTableCache::CShadeTableCache() {
    m_initialized = 0;
}

RVA(0x0014de50, 0x6b)
CShadeTableCache::~CShadeTableCache() {
    if (m_initialized) {
        FreeNodes();
    }
}

RVA(0x0014dec0, 0xc)
i32 CShadeTableCache::Init() {
    m_initialized = 1;
    return 1;
}

RVA(0x0014ded0, 0x64)
void CShadeTableCache::FreeNodes() {
    for (i32 i = 0; i < m_arr.m_nSize; i++) {
        m_arr.m_pData[i]->Free();
        CShadeTable* t = m_arr.m_pData[i];
        if (t) {
            t->Reset();
            ::operator delete(t);
        }
    }
    if (m_arr.m_pData) {
        delete[] m_arr.m_pData;
        m_arr.m_pData = NULL;
    }
    m_arr.m_nMaxSize = 0;
    m_arr.m_nSize = 0;
}

// @early-stop
// Block skeleton is aligned (60 blocks both sides). Residue is confined to the k-loop
// channel expressions: both addends share `pal[i].pe<C>` as a factor, and cl applies the
// distributive law (`c*inv + c*K*uu` -> `c*(inv + K*uu)`, one `fmul st,st(1)` per channel)
// where retail emits the two products and a `faddp`. Commutative respellings (operand
// order inside the product, addend order) are canonical here and byte-identical - the
// factorisation is not reachable by reassociating the source. The j-loop, whose addends
// have different leading operands, is byte-aligned. Re-measured: all 12 factor orders of
// the second product (x endPct / g_p01 / uu, both addend orders) are byte-identical, and
// five named-float-temp forms (two temps + a sum, one sum temp, a hoisted c, a hoisted
// scale) all score 3-12 points WORSE - the temps defeat the ternary's own CSE instead.
RVA(0x0014df40, 0x5f4)
CShadeTable*
CShadeTableCache::FlashTable(PALETTEENTRY* pal, i32 nA, i32 nB, i32 startPct, i32 endPct) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    i32 total = nA + nB;
    if (!t->Set(total << 8, 0)) {
        return NULL;
    }

    i32 oldSize = m_arr.m_nSize;
    i32 newSize = oldSize + 1;
    if (newSize == 0) {
        if (m_arr.m_pData) {
            delete[] m_arr.m_pData;
            m_arr.m_pData = NULL;
        }
        m_arr.m_nMaxSize = 0;
        m_arr.m_nSize = 0;
    } else if (m_arr.m_pData == NULL) {
        m_arr.m_pData = new CShadeTable*[newSize];
        ConstructElements<CShadeTable*>(m_arr.m_pData, newSize);
        m_arr.m_nMaxSize = newSize;
        m_arr.m_nSize = newSize;
    } else if (newSize <= m_arr.m_nMaxSize) {
        if (newSize > m_arr.m_nSize) {
            ConstructElements<CShadeTable*>(&m_arr.m_pData[m_arr.m_nSize], newSize - m_arr.m_nSize);
        }
        m_arr.m_nSize = newSize;
    } else {
        i32 grow = m_arr.m_nGrowBy;
        if (grow == 0) {
            grow = m_arr.m_nSize / 8;
            if (grow < 4) {
                grow = 4;
            } else if (grow > 0x400) {
                grow = 0x400;
            }
        }
        i32 newMax;
        if (newSize < m_arr.m_nMaxSize + grow) {
            newMax = m_arr.m_nMaxSize + grow;
        } else {
            newMax = newSize;
        }
        CShadeTable** data = new CShadeTable*[newMax];
        memcpy(data, m_arr.m_pData, m_arr.m_nSize * 4);
        ConstructElements<CShadeTable*>(&data[m_arr.m_nSize], newSize - m_arr.m_nSize);
        delete[] m_arr.m_pData;
        m_arr.m_pData = data;
        m_arr.m_nSize = newSize;
        m_arr.m_nMaxSize = newMax;
    }
    m_arr.m_pData[oldSize] = t;

    u8* data = t->m_data;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        u8* ramp = &data[i * total];

        for (i32 j = 0; j < nA; j++) {
            float tt = static_cast<float>(j) / static_cast<float>(nA);
            float inv = g_one - tt;
            u8 rn = static_cast<u8>(
                (static_cast<float>((startPct * static_cast<i32>(pal[i].peRed) / 100)) * inv
                 + static_cast<float>(pal[i].peRed) * tt)
                        < g_255
                    ? static_cast<float>((startPct * static_cast<i32>(pal[i].peRed) / 100)) * inv
                          + static_cast<float>(pal[i].peRed) * tt
                    : g_255
            );
            u8 gn = static_cast<u8>(
                (static_cast<float>((startPct * static_cast<i32>(pal[i].peGreen) / 100)) * inv
                 + static_cast<float>(pal[i].peGreen) * tt)
                        < g_255
                    ? static_cast<float>((startPct * static_cast<i32>(pal[i].peGreen) / 100)) * inv
                          + static_cast<float>(pal[i].peGreen) * tt
                    : g_255
            );
            u8 bn = static_cast<u8>(
                (static_cast<float>((startPct * static_cast<i32>(pal[i].peBlue) / 100)) * inv
                 + static_cast<float>(pal[i].peBlue) * tt)
                        < g_255
                    ? static_cast<float>((startPct * static_cast<i32>(pal[i].peBlue) / 100)) * inv
                          + static_cast<float>(pal[i].peBlue) * tt
                    : g_255
            );
            ramp[j] = static_cast<u8>(FindNearestColor(pal, rn, gn, bn));
        }

        i32 br = static_cast<i32>(pal[i].peRed) + 0x10;
        pal[i].peRed = static_cast<u8>((br < 0xff ? br : 0xff));
        i32 bg = static_cast<i32>(pal[i].peGreen) + 0x10;
        pal[i].peGreen = static_cast<u8>((bg < 0xff ? bg : 0xff));
        i32 bb = static_cast<i32>(pal[i].peBlue) + 0x10;
        pal[i].peBlue = static_cast<u8>((bb < 0xff ? bb : 0xff));

        for (i32 k = nA; k < total; k++) {
            float uu = static_cast<float>((k - nA)) / static_cast<float>(nB);
            float inv = g_one - uu;
            u8 rn = static_cast<u8>(
                (static_cast<float>(pal[i].peRed) * inv
                 + static_cast<float>(pal[i].peRed) * static_cast<float>(endPct) * g_p01 * uu)
                        < g_255
                    ? static_cast<float>(pal[i].peRed) * inv
                          + static_cast<float>(pal[i].peRed) * static_cast<float>(endPct) * g_p01
                                * uu
                    : g_255
            );
            u8 gn = static_cast<u8>(
                (static_cast<float>(pal[i].peGreen) * inv
                 + static_cast<float>(pal[i].peGreen) * static_cast<float>(endPct) * g_p01 * uu)
                        < g_255
                    ? static_cast<float>(pal[i].peGreen) * inv
                          + static_cast<float>(pal[i].peGreen) * static_cast<float>(endPct) * g_p01
                                * uu
                    : g_255
            );
            u8 bn = static_cast<u8>(
                (static_cast<float>(pal[i].peBlue) * inv
                 + static_cast<float>(pal[i].peBlue) * static_cast<float>(endPct) * g_p01 * uu)
                        < g_255
                    ? static_cast<float>(pal[i].peBlue) * inv
                          + static_cast<float>(pal[i].peBlue) * static_cast<float>(endPct) * g_p01
                                * uu
                    : g_255
            );
            ramp[k] = static_cast<u8>(FindNearestColor(pal, rn, gn, bn));
        }
    }
    return t;
}

// @early-stop
// Whole-function coloring: retail keeps the new table in ebp with edi as the
// zero scratch; cl swaps them and memory-homes the table pointer. Permuter
// exhausted at this shape.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0014e540, 0x2ea)
CShadeTable*
CShadeTableCache::HsvShiftTable(PALETTEENTRY* pal, i32 steps, i32 pct, i32 gamma, i32 baseArg) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(steps << 8, 0)) {
        return NULL;
    }

    CShadeTableArray& arr = m_arr;
    i32 idx = arr.m_nSize;
    arr.SetSizeGrow(idx + 1, -1);
    arr.m_pData[idx] = t;
    u8* data = t->m_data;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        for (i32 j = 0; j < steps; j++) {
            float luma = static_cast<float>(pal[i].peRed) * g_lumaR
                         + static_cast<float>(pal[i].peGreen) * g_lumaG
                         + static_cast<float>(pal[i].peBlue) * g_lumaB;
            i32 lumaByte = static_cast<i32>(luma) & 0xff;
            float x = g_one / (static_cast<float>(lumaByte) * g_inv255 - g_negone);
            float factor =
                static_cast<float>(pow(static_cast<double>(x), static_cast<double>(gamma)));
            float scale = static_cast<float>(j) / static_cast<float>(steps)
                              * (factor * static_cast<float>((pct - 100)) * g_p01)
                          - g_negone;
            u8 rn = static_cast<u8>(
                HSV_MIN(static_cast<float>(((baseArg & 0xff) + pal[i].peRed)) * scale, g_255)
            );
            u8 gn = static_cast<u8>(
                HSV_MIN(static_cast<float>(((baseArg & 0xff) + pal[i].peGreen)) * scale, g_255)
            );
            u8 bn = static_cast<u8>(
                HSV_MIN(static_cast<float>(((baseArg & 0xff) + pal[i].peBlue)) * scale, g_255)
            );
            data[i * steps + j] = FindNearestColor(pal, rn, gn, bn);
        }
    }
    return t;
}

// @early-stop
// The MFC array growth, loop structure, calls, and referents are complete; only
// one x87 scheduling choice remains TU-state-sensitive.
// docs/patterns/mfc-array-setsize-is-constructelements.md
RVA(0x0014e830, 0x1b9)
CShadeTable* CShadeTableCache::HueRampTable(PALETTEENTRY* pal, i32 steps, i32 packedColor) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(steps << 8, 0)) {
        return NULL;
    }

    CShadeTableArray& arr = m_arr;
    i32 idx = arr.m_nSize;
    arr.SetSizeGrow(idx + 1, -1);
    arr.m_pData[idx] = t;
    u8* data = t->m_data;
    u32 rgb = static_cast<u32>(packedColor);
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        PALETTEENTRY* p = &pal[i];
        for (i32 j = 0; j < steps; j++) {
            float t0 = g_one - static_cast<float>(j) / static_cast<float>(steps);
            float t1 = static_cast<float>(j) / static_cast<float>(steps);
            u8 bn = static_cast<u8>(
                (t0 * static_cast<float>(p->peBlue) + t1 * static_cast<float>(GetBValue(rgb)))
            );
            u8 gn = static_cast<u8>(
                (t0 * static_cast<float>(p->peGreen) + t1 * static_cast<float>(GetGValue(rgb)))
            );
            u8 rn = static_cast<u8>(
                (t0 * static_cast<float>(p->peRed) + t1 * static_cast<float>(GetRValue(rgb)))
            );
            data[i * steps + j] = static_cast<u8>(FindNearestColor(pal, rn, gn, bn));
        }
    }
    return t;
}

// @early-stop
// The `PALETTEENTRY* pr/pc` element-address locals are gone (docs/patterns/
// element-address-local-hides-the-memory-homed-index.md) - subscripting pal[i]
// and pal[j] at every field read is what retail does, and keeping either local
// scores between the extremes (pr only / pc only / neither, measured). The
// residue is the commutative-sum coin: retail evaluates each channel's row term
// first, cl the column term, and swap/split/nesting spellings all canonicalize.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0014e9f0, 0x208)
CShadeTable* CShadeTableCache::GammaTable(PALETTEENTRY* pal, i32 wRow, i32 wCol) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(0x10000, 0)) {
        return NULL;
    }

    CShadeTableArray& arr = m_arr;
    i32 idx = arr.m_nSize;
    arr.SetSizeGrow(idx + 1, -1);
    arr.m_pData[idx] = t;
    u8* data = t->m_data;
    i32 div = (wRow + wCol) / 100;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        for (i32 j = 0; j < PALETTE_ENTRY_COUNT; j++) {
            u8 r = static_cast<u8>((pal[j].peRed * wCol / 100 + pal[i].peRed * wRow / 100) / div);
            u8 g =
                static_cast<u8>((pal[j].peGreen * wCol / 100 + pal[i].peGreen * wRow / 100) / div);
            u8 b = static_cast<u8>((pal[j].peBlue * wCol / 100 + pal[i].peBlue * wRow / 100) / div);
            data[i * 0x100 + j] = static_cast<u8>(FindNearestColor(pal, r, g, b));
        }
    }
    return t;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0014ec00, 0x10f)
CShadeTable* CShadeTableCache::LumaSortTable(PALETTEENTRY* pal) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(0x200, 0)) {
        return NULL;
    }

    CShadeTableArray& arr = m_arr;
    i32 idx = arr.m_nSize;
    arr.SetSizeGrow(idx + 1, -1);
    arr.m_pData[idx] = t;
    u8* data = t->m_data;
    g_pal = pal;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        data[i] = static_cast<u8>(i);
    }
    qsort(data, 0x100, 1, CompareLuma);
    for (i32 c = 0; c < PALETTE_ENTRY_COUNT; c++) {
        for (i32 j = 0; j < PALETTE_ENTRY_COUNT; j++) {
            if (data[j] == c) {
                data[0x100 + c] = static_cast<u8>(j);
                break;
            }
        }
    }
    return t;
}

RVA(0x0014ed10, 0xcc)
i32 __cdecl CShadeTableCache::CompareLuma(const void* a, const void* b) {
    u8 ia = *static_cast<const u8*>(a);
    u8 ib = *static_cast<const u8*>(b);
    u8 la = static_cast<u8>(static_cast<i32>(
        (static_cast<float>(g_pal[ia].peBlue) * g_lumaB
         + static_cast<float>(g_pal[ia].peGreen) * g_lumaG
         + static_cast<float>(g_pal[ia].peRed) * g_lumaR)
    ));
    u8 lb = static_cast<u8>(static_cast<i32>(
        (static_cast<float>(g_pal[ib].peBlue) * g_lumaB
         + static_cast<float>(g_pal[ib].peGreen) * g_lumaG
         + static_cast<float>(g_pal[ib].peRed) * g_lumaR)
    ));
    if (lb > la) {
        return -1;
    }
    if (lb < la) {
        return 1;
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0014ede0, 0x10f)
CShadeTable* CShadeTableCache::HueSortTable(PALETTEENTRY* pal) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(0x200, 0)) {
        return NULL;
    }

    CShadeTableArray& arr = m_arr;
    i32 idx = arr.m_nSize;
    arr.SetSizeGrow(idx + 1, -1);
    arr.m_pData[idx] = t;
    u8* data = t->m_data;
    g_pal = pal;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        data[i] = static_cast<u8>(i);
    }
    qsort(data, 0x100, 1, CompareHue);
    for (i32 c = 0; c < PALETTE_ENTRY_COUNT; c++) {
        for (i32 j = 0; j < PALETTE_ENTRY_COUNT; j++) {
            if (data[j] == c) {
                data[0x100 + c] = static_cast<u8>(j);
                break;
            }
        }
    }
    return t;
}

// @early-stop
// Both arms' block skeletons and the arm-local epilogues match; the residue is
// the widen idiom - retail widens all three nibble terms with `movzx cx,cl`
// (u8 -> u16 in a byte register) where cl masks two of them 32-bit
// (`and edx,0xff` / `and eax,0xf`) and only the store-side term gets movzx.
// Every u16-typed spelling of the accumulator or the terms DOES emit the three
// movzx, but costs a fourth register (an extra ebx save) that retail does not
// have, so it scores 80-84 against 92.9 here; retail's loop lives in
// eax/ecx/edx/esi only. Applying the u16 shape to ONE arm scores 93.6, but
// retail's two arms are byte-identical, so an asymmetric source is fabricated
// and was rejected.
RVA(0x0014eef0, 0x183)
CShadeTable* CShadeTableCache::GreyTable() {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(0x20000, 0)) {
        t->Reset();
        ::operator delete(t);
        return NULL;
    }

    CShadeTableArray& arr = m_arr;
    i32 idx = arr.m_nSize;
    arr.SetSizeGrow(idx + 1, -1);
    arr.m_pData[idx] = t;
    u16* out = Pix16(t->m_data);
    if (PIXEL_FORMAT_IS_RGB555) {
        for (i32 v = 0; v < 0x10000; v++) {
            i32 acc = static_cast<u8>((v >> 0xb)) << 4;
            acc = (acc + static_cast<u8>((v >> 6) & 0xf)) << 4;
            *out++ = static_cast<u16>((acc + static_cast<u8>((v >> 1) & 0xf)));
        }
    } else {
        for (i32 v = 0; v < 0x10000; v++) {
            i32 acc = static_cast<u8>((v >> 0xc)) << 4;
            acc = (acc + static_cast<u8>((v >> 7) & 0xf)) << 4;
            *out++ = static_cast<u16>((acc + static_cast<u8>((v >> 1) & 0xf)));
        }
    }
    return t;
}

// @early-stop
// cl hoists the invariant (float)v * scale product to the v-loop preheader and
// keeps it on the x87 stack across the r/g/b nest (fstp at loop end); retail
// recomputes the whole f = v*scale*inv255-negone per pixel. Six spellings of f
// (inline-per-channel, split, pre-clamp) all CSE back to the hoist.
RVA(0x0014f080, 0x283)
CShadeTable* CShadeTableCache::AddTable(float scale) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(0x20000, 0)) {
        t->Reset();
        ::operator delete(t);
        return NULL;
    }

    CShadeTableArray& arr = m_arr;
    i32 idx = arr.m_nSize;
    arr.SetSizeGrow(idx + 1, -1);
    arr.m_pData[idx] = t;
    u16* out = Pix16(t->m_data);

    for (i32 v = 0; v < PALETTE_ENTRY_COUNT; v += 0x10) {
        i32 r = 8;
        for (i32 nr = 0x10; nr != 0; nr--) {
            i32 g = 8;
            for (i32 ng = 0x10; ng != 0; ng--) {
                i32 b = 8;
                for (i32 nb = 0x10; nb != 0; nb--) {
                    u8 rc = static_cast<u8>((r < 0xff ? r : 0xff));
                    u8 gc = static_cast<u8>((g < 0xff ? g : 0xff));
                    u8 bc = static_cast<u8>((b < 0xff ? b : 0xff));

                    float f = static_cast<float>(v) * scale * g_inv255 - g_negone;

                    float fr = static_cast<float>(rc) * f;
                    u8 rn = static_cast<u8>(static_cast<i32>((fr < g_255 ? fr : g_255)));
                    float fg = static_cast<float>(gc) * f;
                    u8 gn = static_cast<u8>(static_cast<i32>((fg < g_255 ? fg : g_255)));
                    float fb = static_cast<float>(bc) * f;
                    u8 bn = static_cast<u8>(static_cast<i32>((fb < g_255 ? fb : g_255)));
                    *out++ = static_cast<u16>(
                        ((static_cast<u8>((static_cast<u8>(rn) >> static_cast<u8>(g_rDown)))
                          << g_rUp)
                         | (static_cast<u8>((static_cast<u8>(gn) >> static_cast<u8>(g_gDown)))
                            << g_gUp)
                         | static_cast<u8>((static_cast<u8>(bn) >> static_cast<u8>(g_bDown))))
                    );
                    b += 0x10;
                }
                g += 0x10;
            }
            r += 0x10;
        }
    }
    return t;
}

RVA(0x0014f310, 0x297)
CShadeTable* CShadeTableCache::SubTable(i32 color) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(0x20000, 0)) {
        t->Reset();
        ::operator delete(t);
        return NULL;
    }

    CShadeTableArray& arr = m_arr;
    i32 idx = arr.m_nSize;
    arr.SetSizeGrow(idx + 1, -1);
    arr.m_pData[idx] = t;
    u16* out = Pix16(t->m_data);
    u32 rgb = static_cast<u32>(color);
    i32 cb = GetBValue(rgb);
    i32 cg = GetGValue(rgb);
    i32 cr = GetRValue(rgb);

    for (i32 fill = 0; fill < 0x10; fill++) {
        i32 subr = cr * fill / 0xf;
        i32 subg = cg * fill / 0xf;
        i32 subb = cb * fill / 0xf;
        i32 level = 0xf - fill;
        for (i32 r = 0; r < 0x10; r++) {
            u8 rn = static_cast<u8>((((r * level / 0xf) << 4) + subr));
            for (i32 g = 0; g < 0x10; g++) {
                u8 gn = static_cast<u8>((((g * level / 0xf) << 4) + subg));
                for (i32 b = 0; b < 0x10; b++) {
                    u8 bn = static_cast<u8>((((b * level / 0xf) << 4) + subb));
                    // Accumulated channel by channel: one OR expression lets cl
                    // reassociate the loop-invariant red|green pair and hoist it
                    // into a b-loop preheader retail does not have.
                    u16 v = static_cast<u8>(bn >> g_bDown);
                    v |= static_cast<u16>(static_cast<u8>(rn >> g_rDown) << g_rUp);
                    v |= static_cast<u16>(static_cast<u8>(gn >> g_gDown) << g_gUp);
                    *out++ = v;
                }
            }
        }
    }
    return t;
}

RVA(0x0014f5b0, 0x10a)
CShadeTable* CShadeTableCache::AlphaTable(PALETTEENTRY* pal) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(0x200, 0)) {
        t->Reset();
        ::operator delete(t);
        return NULL;
    }

    CShadeTableArray& arr = m_arr;
    i32 idx = arr.m_nSize;
    arr.SetSizeGrow(idx + 1, -1);
    arr.m_pData[idx] = t;
    u16* out = Pix16(t->m_data);
    PALETTEENTRY* p = pal;
    for (i32 i = 0x100; i != 0; i--) {
        u16 v = static_cast<u16>(
            ((static_cast<u8>((p->peRed >> static_cast<u8>(g_rDown))) << g_rUp)
             | (static_cast<u8>((p->peGreen >> static_cast<u8>(g_gDown))) << g_gUp)
             | static_cast<u8>((p->peBlue >> static_cast<u8>(g_bDown))))
        );
        *out++ = v;
        p++;
    }
    return t;
}

RVA(0x0014f6c0, 0x1e1)
CShadeTable* CShadeTableCache::AddFromArray(CString name) {
    CShadeTable* t = new CShadeTable;
    i32 oldSize = m_arr.m_nSize;
    i32 newSize = oldSize + 1;
    if (newSize == 0) {
        if (m_arr.m_pData) {
            delete[] m_arr.m_pData;
            m_arr.m_pData = NULL;
        }
        m_arr.m_nMaxSize = 0;
        m_arr.m_nSize = 0;
    } else if (m_arr.m_pData == NULL) {
        m_arr.m_pData = new CShadeTable*[newSize];
        ConstructElements<CShadeTable*>(m_arr.m_pData, newSize);
        m_arr.m_nMaxSize = newSize;
        m_arr.m_nSize = newSize;
    } else if (newSize <= m_arr.m_nMaxSize) {
        if (newSize > m_arr.m_nSize) {
            ConstructElements<CShadeTable*>(&m_arr.m_pData[m_arr.m_nSize], newSize - m_arr.m_nSize);
        }
        m_arr.m_nSize = newSize;
    } else {
        i32 grow = m_arr.m_nGrowBy;
        if (grow == 0) {
            grow = m_arr.m_nSize / 8;
            if (grow < 4) {
                grow = 4;
            } else if (grow > 0x400) {
                grow = 0x400;
            }
        }
        i32 newMax;
        if (newSize < m_arr.m_nMaxSize + grow) {
            newMax = m_arr.m_nMaxSize + grow;
        } else {
            newMax = newSize;
        }
        CShadeTable** data = new CShadeTable*[newMax];
        memcpy(data, m_arr.m_pData, m_arr.m_nSize * 4);
        ConstructElements<CShadeTable*>(&data[m_arr.m_nSize], newSize - m_arr.m_nSize);
        delete[] m_arr.m_pData;
        m_arr.m_pData = data;
        m_arr.m_nSize = newSize;
        m_arr.m_nMaxSize = newMax;
    }
    m_arr.m_pData[oldSize] = t;
    if (!t->LoadFromFile(name, 0)) {
        FindRemove(t);
        return NULL;
    }
    return t;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0014f8b0, 0x1b0)
CShadeTable* CShadeTableCache::AddFromBuffer(u8* data, i32 size) {
    CShadeTable* t = new CShadeTable;
    i32 oldSize = m_arr.m_nSize;
    i32 newSize = oldSize + 1;
    if (newSize == 0) {
        if (m_arr.m_pData) {
            delete[] m_arr.m_pData;
            m_arr.m_pData = NULL;
        }
        m_arr.m_nMaxSize = 0;
        m_arr.m_nSize = 0;
    } else if (m_arr.m_pData == NULL) {
        m_arr.m_pData = new CShadeTable*[newSize];
        ConstructElements<CShadeTable*>(m_arr.m_pData, newSize);
        m_arr.m_nMaxSize = newSize;
        m_arr.m_nSize = newSize;
    } else if (newSize <= m_arr.m_nMaxSize) {
        if (newSize > m_arr.m_nSize) {
            ConstructElements<CShadeTable*>(&m_arr.m_pData[m_arr.m_nSize], newSize - m_arr.m_nSize);
        }
        m_arr.m_nSize = newSize;
    } else {
        i32 grow = m_arr.m_nGrowBy;
        if (grow == 0) {
            grow = m_arr.m_nSize / 8;
            if (grow < 4) {
                grow = 4;
            } else if (grow > 0x400) {
                grow = 0x400;
            }
        }
        i32 newMax;
        if (newSize < m_arr.m_nMaxSize + grow) {
            newMax = m_arr.m_nMaxSize + grow;
        } else {
            newMax = newSize;
        }
        CShadeTable** data = new CShadeTable*[newMax];
        memcpy(data, m_arr.m_pData, m_arr.m_nSize * 4);
        ConstructElements<CShadeTable*>(&data[m_arr.m_nSize], newSize - m_arr.m_nSize);
        delete[] m_arr.m_pData;
        m_arr.m_pData = data;
        m_arr.m_nSize = newSize;
        m_arr.m_nMaxSize = newMax;
    }
    m_arr.m_pData[oldSize] = t;
    if (!t->LoadFromMem(data, size, 0)) {
        FindRemove(t);
        return NULL;
    }
    return t;
}

RVA(0x0014fa60, 0xd7)
i32 __cdecl CShadeTableCache::CompareHue(const void* a, const void* b) {
    u8 ia = *static_cast<const u8*>(a);
    u8 ib = *static_cast<const u8*>(b);
    ColorHSV ha, hb;
    ha = RgbToHsv((g_pal[ia].peBlue << 0x10) | (g_pal[ia].peGreen << 8) | g_pal[ia].peRed);
    hb = RgbToHsv((g_pal[ib].peBlue << 0x10) | (g_pal[ib].peGreen << 8) | g_pal[ib].peRed);
    if (ha.h < hb.h) {
        return -1;
    }
    if (ha.h > hb.h) {
        return 1;
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0014fb40, 0x3e)
CShadeTable* CShadeTableCache::FindByKey(i32 key) {
    for (i32 i = 0; i < m_arr.GetSize(); i++) {
        if (m_arr[i]->m_key == key) {
            return m_arr[i];
        }
    }
    return NULL;
}

RVA(0x0014fb80, 0x68)
void CShadeTableCache::FindRemove(CShadeTable* key) {
    i32 n = m_arr.m_nSize;
    for (i32 i = 0; i < n; i++) {
        if (m_arr.m_pData[i] == key) {
            m_arr.m_pData[i]->Free();
            CShadeTable* t = m_arr.m_pData[i];
            if (t) {
                t->Reset();
                ::operator delete(t);
            }
            i32 cnt = m_arr.m_nSize - i - 1;
            CShadeTable** dst = &m_arr.m_pData[i];
            if (cnt) {
                CShadeTable** src = &m_arr.m_pData[i + 1];
                memcpy(dst, src, cnt * sizeof(CShadeTable*));
            }
            m_arr.m_nSize--;
            return;
        }
    }
}

RVA(0x0014fbf0, 0xcb)
i32 __cdecl CShadeTableCache::FindNearestColor(PALETTEENTRY* pal, u8 r, u8 g, u8 b) {
    i32 gg = g;
    i32 bb = b;
    i32 best = 0;
    i32 rr = r;
    i32 dg = gg - pal->peGreen;
    i32 db = bb - pal->peBlue;
    i32 dr = rr - pal->peRed;
    i32 bestDist = dg * dg + db * db + dr * dr;
    for (i32 i = 1; i < PALETTE_ENTRY_COUNT; i++) {
        i32 dr2 = rr - pal[i].peRed;
        i32 dg2 = gg - pal[i].peGreen;
        i32 db2 = bb - pal[i].peBlue;
        i32 d = dr2 * dr2 + dg2 * dg2 + db2 * db2;
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

RVA(0x0014fcc0, 0x16d)
ColorHSV RgbToHsv(u32 color) {
    ColorHSV hsv;
    float v =
        static_cast<float>(HSV_MAX(HSV_MAX(GetRValue(color), GetGValue(color)), GetBValue(color)));
    float mn =
        static_cast<float>(HSV_MIN(HSV_MIN(GetRValue(color), GetGValue(color)), GetBValue(color)));
    float h;

    hsv.v = v;
    if (v == 0.0) {
        hsv.s = 0.0;
        hsv.h = 0.0;
    } else {
        float delta = v - mn;
        hsv.s = delta / v;
        if (delta == 0.0) {
            h = 0.0f;
        } else if (GetRValue(color) == v) {
            h = (GetGValue(color) - GetBValue(color)) / delta;
        } else if (GetGValue(color) == v) {
            h = (GetBValue(color) - GetRValue(color)) / delta - -2.0f;
        } else {
            h = (GetRValue(color) - GetGValue(color)) / delta - -4.0f;
        }
        h = h * 60.0f;
        if (h < 0.0) {
            h = h - -360.0f;
        }
        hsv.h = h;
    }
    return hsv;
}

RVA_COMPGEN(0x0014fe30, 0x51, ??1CShadeTableArray@@UAE@XZ)

RVA(0x0014fe90, 0x188)
void CShadeTableArray::Serialize(CArchive& arc) {
    if (arc.IsStoring()) {
        arc.WriteCount(m_nSize);
    } else {
        i32 n = arc.ReadCount();
        if (n == 0) {
            if (m_pData != NULL) {
                delete[] m_pData;
                m_pData = NULL;
            }
            m_nMaxSize = 0;
            m_nSize = 0;
        } else if (m_pData == NULL) {
            m_pData = new CShadeTable*[n];
            ConstructElements<CShadeTable*>(m_pData, n);
            m_nMaxSize = n;
            m_nSize = n;
        } else if (n <= m_nMaxSize) {
            if (n > m_nSize) {
                ConstructElements<CShadeTable*>(&m_pData[m_nSize], n - m_nSize);
            }
            m_nSize = n;
        } else {
            i32 grow = m_nGrowBy;
            if (grow == 0) {
                grow = m_nSize / 8;
                if (grow < 4) {
                    grow = 4;
                } else if (grow > 0x400) {
                    grow = 0x400;
                }
            }
            i32 newcap;
            if (n < m_nMaxSize + grow) {
                newcap = m_nMaxSize + grow;
            } else {
                newcap = n;
            }
            CShadeTable** nd = new CShadeTable*[newcap];
            memcpy(nd, m_pData, m_nSize * 4);
            ConstructElements<CShadeTable*>(&nd[m_nSize], n - m_nSize);
            delete[] m_pData;
            m_pData = nd;
            m_nSize = n;
            m_nMaxSize = newcap;
        }
    }

    SerializeElements<CShadeTable*>(arc, m_pData, m_nSize);
}

RVA_COMPGEN(0x00150020, 0x1e, ??_GCShadeTableArray@@UAEPAXI@Z)
RVA(0x00150040, 0x136)
void CShadeTableArray::SetSizeGrow(i32 nNewSize, i32 nGrowBy) {
    if (nGrowBy != -1) {
        m_nGrowBy = nGrowBy;
    }
    if (nNewSize == 0) {
        if (m_pData != NULL) {
            delete[] m_pData;
            m_pData = NULL;
        }
        m_nSize = m_nMaxSize = 0;
    } else if (m_pData == NULL) {
        m_pData = new CShadeTable*[nNewSize];
        ConstructElements<CShadeTable*>(m_pData, nNewSize);
        m_nSize = m_nMaxSize = nNewSize;
    } else if (nNewSize <= m_nMaxSize) {
        if (nNewSize > m_nSize) {
            ConstructElements<CShadeTable*>(&m_pData[m_nSize], nNewSize - m_nSize);
        }
        m_nSize = nNewSize;
    } else {
        i32 grow = m_nGrowBy;
        if (grow == 0) {
            grow = m_nSize / 8;
            if (grow < 4) {
                grow = 4;
            } else if (grow > 1024) {
                grow = 1024;
            }
        }
        i32 nNewMax;
        if (nNewSize < m_nMaxSize + grow) {
            nNewMax = m_nMaxSize + grow;
        } else {
            nNewMax = nNewSize;
        }
        CShadeTable** pNewData = new CShadeTable*[nNewMax];
        memcpy(pNewData, m_pData, m_nSize * sizeof(CShadeTable*));
        ConstructElements<CShadeTable*>(&pNewData[m_nSize], nNewSize - m_nSize);
        delete[] m_pData;
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }
}
