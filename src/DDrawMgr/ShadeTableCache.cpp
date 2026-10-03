#include <StdAfx.h>

#include <Ints.h>

#include <DDrawMgr/ShadeTableCache.h>

#include <DDrawMgr/ColorHsv.h>
#include <DDrawMgr/ColorHsvMacros.h>
#include <DDrawMgr/PaletteColorInline.h>
#include <DDrawMgr/PaletteSize.h>
#include <DDrawMgr/PixelFormatMacros.h>
#include <DDrawMgr/PixelShift.h>
#include <Enums.h>
#include <Ints.h>
#include <Lith/BDefs.h>
#include <Pix16.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

PALETTEENTRY* g_pal = NULL;

const float g_one = 1.0f;

const float g_colorChannelMax = 255.0f;

const float g_percentScale = 0.01f;

const float g_lumaR = 0.5859375f;

const float g_lumaG = 0.296875f;

const float g_lumaB = 0.109375f;

const float g_inv255 = 0.003921568859368563f;

const float s_negone = -1.0f;

CShadeTableCache::CShadeTableCache() {
    m_initialized = false;
}

CShadeTableCache::~CShadeTableCache() {
    if (m_initialized) {
        FreeNodes();
    }
}

i32 CShadeTableCache::Init() {
    m_initialized = true;
    return 1;
}

void CShadeTableCache::FreeNodes() {
    for (i32 i = 0; i < static_cast<i32>(m_arr.size()); i++) {
        m_arr[i]->Free();
        CShadeTable* t = m_arr[i];
        if (t) {
            t->Reset();
            delete t;
        }
    }
    m_arr.clear();
}

CShadeTable* CShadeTableCache::FlashTable(
    PALETTEENTRY* pal,
    i32 darkRampSteps,
    i32 brightRampSteps,
    i32 startPct,
    i32 endPct
) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    i32 total = darkRampSteps + brightRampSteps;
    if (!t->Set(total << 8, 0)) {
        return NULL;
    }

    m_arr.push_back(t);

    u8* data = t->GetData();
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        u8* ramp = &data[i * total];

        for (i32 j = 0; j < darkRampSteps; j++) {
            float tt = static_cast<float>(j) / static_cast<float>(darkRampSteps);
            u8 rn = static_cast<u8>(
                min(INTERPOLATE(
                        static_cast<float>((startPct * static_cast<i32>(pal[i].peRed) / 100)),
                        static_cast<float>(pal[i].peRed),
                        tt
                    ),
                    g_colorChannelMax)
            );
            u8 gn = static_cast<u8>(
                min(INTERPOLATE(
                        static_cast<float>((startPct * static_cast<i32>(pal[i].peGreen) / 100)),
                        static_cast<float>(pal[i].peGreen),
                        tt
                    ),
                    g_colorChannelMax)
            );
            u8 bn = static_cast<u8>(
                min(INTERPOLATE(
                        static_cast<float>((startPct * static_cast<i32>(pal[i].peBlue) / 100)),
                        static_cast<float>(pal[i].peBlue),
                        tt
                    ),
                    g_colorChannelMax)
            );
            ramp[j] = static_cast<u8>(FindNearestColor(pal, rn, gn, bn));
        }

        i32 br = static_cast<i32>(pal[i].peRed) + FLASH_SHADE_CHANNEL_BOOST;
        pal[i].peRed = static_cast<u8>(min(br, FLASH_SHADE_CHANNEL_MAX));
        i32 bg = static_cast<i32>(pal[i].peGreen) + FLASH_SHADE_CHANNEL_BOOST;
        pal[i].peGreen = static_cast<u8>(min(bg, FLASH_SHADE_CHANNEL_MAX));
        i32 bb = static_cast<i32>(pal[i].peBlue) + FLASH_SHADE_CHANNEL_BOOST;
        pal[i].peBlue = static_cast<u8>(min(bb, FLASH_SHADE_CHANNEL_MAX));

        for (i32 k = darkRampSteps; k < total; k++) {
            float uu =
                static_cast<float>((k - darkRampSteps)) / static_cast<float>(brightRampSteps);
            u8 rn = static_cast<u8>(
                min(INTERPOLATE(
                        static_cast<float>(pal[i].peRed),
                        (static_cast<float>(endPct) * static_cast<float>(pal[i].peRed))
                            * g_percentScale,
                        uu
                    ),
                    g_colorChannelMax)
            );
            u8 gn = static_cast<u8>(
                min(INTERPOLATE(
                        static_cast<float>(pal[i].peGreen),
                        (static_cast<float>(endPct) * static_cast<float>(pal[i].peGreen))
                            * g_percentScale,
                        uu
                    ),
                    g_colorChannelMax)
            );
            u8 bn = static_cast<u8>(
                min(INTERPOLATE(
                        static_cast<float>(pal[i].peBlue),
                        (static_cast<float>(endPct) * static_cast<float>(pal[i].peBlue))
                            * g_percentScale,
                        uu
                    ),
                    g_colorChannelMax)
            );
            ramp[k] = static_cast<u8>(FindNearestColor(pal, rn, gn, bn));
        }
    }
    return t;
}

CShadeTable*
CShadeTableCache::HsvShiftTable(PALETTEENTRY* pal, i32 steps, i32 pct, i32 gamma, i32 baseArg) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(steps << 8, 0)) {
        return NULL;
    }

    m_arr.push_back(t);
    u8* data = t->GetData();
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        for (i32 j = 0; j < steps; j++) {
            i32 green = pal[i].peGreen;
            i32 red = pal[i].peRed;
            i32 blue = pal[i].peBlue;
            float luma = static_cast<float>(red) * g_lumaR + static_cast<float>(green) * g_lumaG
                         + static_cast<float>(blue) * g_lumaB;
            i32 lumaByte = static_cast<i32>(luma) & PIXEL_BYTE_MASK;
            float x = g_one / (static_cast<float>(lumaByte) * g_inv255 - s_negone);
            float factor =
                static_cast<float>(pow(static_cast<double>(x), static_cast<double>(gamma)));
            float scale = static_cast<float>(j) / static_cast<float>(steps)
                              * ((static_cast<float>((pct - 100)) * factor) * g_percentScale)
                          - s_negone;
            u8 rn = static_cast<u8>(
                min(static_cast<float>(((baseArg & PIXEL_BYTE_MASK) + pal[i].peRed)) * scale,
                    g_colorChannelMax)
            );
            u8 gn = static_cast<u8>(
                min(static_cast<float>(((baseArg & PIXEL_BYTE_MASK) + pal[i].peGreen)) * scale,
                    g_colorChannelMax)
            );
            u8 bn = static_cast<u8>(
                min(static_cast<float>(((baseArg & PIXEL_BYTE_MASK) + pal[i].peBlue)) * scale,
                    g_colorChannelMax)
            );
            data[i * steps + j] = FindNearestColor(pal, rn, gn, bn);
        }
    }
    return t;
}

CShadeTable* CShadeTableCache::HueRampTable(PALETTEENTRY* pal, i32 steps, i32 packedColor) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(steps << 8, 0)) {
        return NULL;
    }

    m_arr.push_back(t);
    u8* data = t->GetData();
    u32 rgb = static_cast<u32>(packedColor);
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        for (i32 j = 0; j < steps; j++) {
            float t1 = static_cast<float>(j) / static_cast<float>(steps);
            u8 rn = static_cast<u8>(INTERPOLATE(
                static_cast<float>(pal[i].peRed),
                static_cast<float>(GetRValue(rgb)),
                t1
            ));
            u8 gn = static_cast<u8>(INTERPOLATE(
                static_cast<float>(pal[i].peGreen),
                static_cast<float>(GetGValue(rgb)),
                t1
            ));
            u8 bn = static_cast<u8>(INTERPOLATE(
                static_cast<float>(pal[i].peBlue),
                static_cast<float>(GetBValue(rgb)),
                t1
            ));
            data[i * steps + j] = static_cast<u8>(FindNearestColor(pal, rn, gn, bn));
        }
    }
    return t;
}

CShadeTable* CShadeTableCache::GammaTable(PALETTEENTRY* pal, i32 wRow, i32 wCol) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(PALETTE_ENTRY_COUNT * PALETTE_ENTRY_COUNT, 0)) {
        return NULL;
    }

    m_arr.push_back(t);
    u8* data = t->GetData();
    i32 div = (wRow + wCol) / 100;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        for (i32 j = 0; j < PALETTE_ENTRY_COUNT; j++) {
            u8 r = WeightedPaletteChannel(pal[i].peRed, pal[j].peRed, wRow, wCol, div);
            u8 g = WeightedPaletteChannel(pal[i].peGreen, pal[j].peGreen, wRow, wCol, div);
            u8 b = WeightedPaletteChannel(pal[i].peBlue, pal[j].peBlue, wRow, wCol, div);
            data[i * PALETTE_ENTRY_COUNT + j] = static_cast<u8>(FindNearestColor(pal, r, g, b));
        }
    }
    return t;
}

CShadeTable* CShadeTableCache::LumaSortTable(PALETTEENTRY* pal) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(PALETTE_ENTRY_COUNT * 2, 0)) {
        return NULL;
    }

    m_arr.push_back(t);
    u8* data = t->GetData();
    g_pal = pal;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        data[i] = static_cast<u8>(i);
    }
    qsort(data, PALETTE_ENTRY_COUNT, sizeof(u8), CompareLuma);
    for (i32 c = 0; c < PALETTE_ENTRY_COUNT; c++) {
        for (i32 j = 0; j < PALETTE_ENTRY_COUNT; j++) {
            if (data[j] == c) {
                data[PALETTE_ENTRY_COUNT + c] = static_cast<u8>(j);
                break;
            }
        }
    }
    return t;
}

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

CShadeTable* CShadeTableCache::HueSortTable(PALETTEENTRY* pal) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(PALETTE_ENTRY_COUNT * 2, 0)) {
        return NULL;
    }

    m_arr.push_back(t);
    u8* data = t->GetData();
    g_pal = pal;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        data[i] = static_cast<u8>(i);
    }
    qsort(data, PALETTE_ENTRY_COUNT, sizeof(u8), CompareHue);
    for (i32 c = 0; c < PALETTE_ENTRY_COUNT; c++) {
        for (i32 j = 0; j < PALETTE_ENTRY_COUNT; j++) {
            if (data[j] == c) {
                data[PALETTE_ENTRY_COUNT + c] = static_cast<u8>(j);
                break;
            }
        }
    }
    return t;
}

CShadeTable* CShadeTableCache::GreyTable() {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(PIXEL16_VALUE_COUNT * sizeof(u16), 0)) {
        t->Reset();
        delete t;
        return NULL;
    }

    m_arr.push_back(t);
    if (PIXEL_FORMAT_IS_RGB555) {
        u16* out = t->Lut16();
        for (i32 v = 0; v < PIXEL16_VALUE_COUNT; v++) {
            u8 r = static_cast<u8>(v >> RGB555_RED_TO_4_SHIFT);
            u8 g = static_cast<u8>((v >> RGB555_GREEN_TO_4_SHIFT) & PIXEL_NIBBLE_MASK);
            u8 b = static_cast<u8>((v >> RGB16_BLUE_TO_4_SHIFT) & PIXEL_NIBBLE_MASK);
            *out++ = static_cast<u16>(
                (((static_cast<u16>(r) << PIXEL_NIBBLE_BITS) + static_cast<u16>(g))
                 << PIXEL_NIBBLE_BITS)
                + static_cast<u16>(b)
            );
        }
    } else {
        u16* out = t->Lut16();
        for (i32 v = 0; v < PIXEL16_VALUE_COUNT; v++) {
            u8 r = static_cast<u8>(v >> RGB565_RED_TO_4_SHIFT);
            u8 g = static_cast<u8>((v >> RGB565_GREEN_TO_4_SHIFT) & PIXEL_NIBBLE_MASK);
            u8 b = static_cast<u8>((v >> RGB16_BLUE_TO_4_SHIFT) & PIXEL_NIBBLE_MASK);
            *out++ = static_cast<u16>(
                (((static_cast<u16>(r) << PIXEL_NIBBLE_BITS) + static_cast<u16>(g))
                 << PIXEL_NIBBLE_BITS)
                + static_cast<u16>(b)
            );
        }
    }
    return t;
}

CShadeTable* CShadeTableCache::AddTable(float scale) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(PIXEL16_VALUE_COUNT * sizeof(u16), 0)) {
        t->Reset();
        delete t;
        return NULL;
    }

    m_arr.push_back(t);
    u16* out = t->Lut16();

    for (i32 v = 0; v < PALETTE_ENTRY_COUNT; v += PIXEL_NIBBLE_VALUE_COUNT) {
        i32 r = PIXEL_NIBBLE_MIDPOINT;
        for (i32 nr = PIXEL_NIBBLE_VALUE_COUNT; nr != 0; nr--) {
            i32 g = PIXEL_NIBBLE_MIDPOINT;
            for (i32 ng = PIXEL_NIBBLE_VALUE_COUNT; ng != 0; ng--) {
                i32 b = PIXEL_NIBBLE_MIDPOINT;
                for (i32 nb = PIXEL_NIBBLE_VALUE_COUNT; nb != 0; nb--) {
                    u8 rc = static_cast<u8>(min(r, PIXEL_BYTE_MASK));
                    u8 gc = static_cast<u8>(min(g, PIXEL_BYTE_MASK));
                    u8 bc = static_cast<u8>(min(b, PIXEL_BYTE_MASK));

                    float f = static_cast<float>(v) * (scale * g_inv255) - s_negone;

                    u8 rn = static_cast<u8>(min(static_cast<float>(rc) * f, g_colorChannelMax));
                    u8 gn = static_cast<u8>(min(static_cast<float>(gc) * f, g_colorChannelMax));
                    u8 bn = static_cast<u8>(min(static_cast<float>(bc) * f, g_colorChannelMax));
                    *out++ = static_cast<u16>(
                        ((static_cast<u8>((static_cast<u8>(rn) >> static_cast<u8>(g_rDown)))
                          << g_rUp)
                         | (static_cast<u8>((static_cast<u8>(gn) >> static_cast<u8>(g_gDown)))
                            << g_gUp)
                         | static_cast<u8>((static_cast<u8>(bn) >> static_cast<u8>(g_bDown))))
                    );
                    b += PIXEL_NIBBLE_VALUE_COUNT;
                }
                g += PIXEL_NIBBLE_VALUE_COUNT;
            }
            r += PIXEL_NIBBLE_VALUE_COUNT;
        }
    }
    return t;
}

CShadeTable* CShadeTableCache::SubTable(i32 color) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(PIXEL16_VALUE_COUNT * sizeof(u16), 0)) {
        t->Reset();
        delete t;
        return NULL;
    }

    m_arr.push_back(t);
    u16* out = t->Lut16();
    i32 subb = 0;
    i32 subg = 0;
    i32 subr = 0;
    u32 rgb = static_cast<u32>(color);
    i32 cb = GetBValue(rgb);
    i32 cg = GetGValue(rgb);
    i32 cr = GetRValue(rgb);

    for (i32 level = PIXEL_NIBBLE_MASK; level > -1; level--) {
        for (i32 r = 0; r < PIXEL_NIBBLE_VALUE_COUNT; r++) {
            u8 rn = static_cast<u8>(
                ((r * level / PIXEL_NIBBLE_MASK) << PIXEL_NIBBLE_BITS) + subr / PIXEL_NIBBLE_MASK
            );
            for (i32 g = 0; g < PIXEL_NIBBLE_VALUE_COUNT; g++) {
                u8 gn = static_cast<u8>(
                    ((g * level / PIXEL_NIBBLE_MASK) << PIXEL_NIBBLE_BITS)
                    + subg / PIXEL_NIBBLE_MASK
                );
                for (i32 b = 0; b < PIXEL_NIBBLE_VALUE_COUNT; b++) {
                    u8 bn = static_cast<u8>(
                        ((b * level / PIXEL_NIBBLE_MASK) << PIXEL_NIBBLE_BITS)
                        + subb / PIXEL_NIBBLE_MASK
                    );
                    *out++ = PackPixel16(rn, gn, bn);
                }
            }
        }
        subr += cr;
        subg += cg;
        subb += cb;
    }
    return t;
}

CShadeTable* CShadeTableCache::AlphaTable(PALETTEENTRY* pal) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return NULL;
    }
    if (!t->Set(PALETTE_ENTRY_COUNT * 2, 0)) {
        t->Reset();
        delete t;
        return NULL;
    }

    m_arr.push_back(t);
    u16* out = t->Lut16();
    PALETTEENTRY* p = pal;
    for (i32 i = PALETTE_ENTRY_COUNT; i != 0; i--) {
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

CShadeTable* CShadeTableCache::AddFromArray(std::string name) {
    CShadeTable* t = new CShadeTable;
    m_arr.push_back(t);
    if (!t->LoadFromFile(name, 0)) {
        FindRemove(t);
        return NULL;
    }
    return t;
}

CShadeTable* CShadeTableCache::AddFromBuffer(u8* data, i32 size) {
    CShadeTable* t = new CShadeTable;
    m_arr.push_back(t);
    if (!t->LoadFromMem(data, size, 0)) {
        FindRemove(t);
        return NULL;
    }
    return t;
}

i32 __cdecl CShadeTableCache::CompareHue(const void* a, const void* b) {
    u8 ia = *static_cast<const u8*>(a);
    u8 ib = *static_cast<const u8*>(b);
    ColorHSV ha, hb;
    ha = RgbToHsv(RGB(g_pal[ia].peRed, g_pal[ia].peGreen, g_pal[ia].peBlue));
    hb = RgbToHsv(RGB(g_pal[ib].peRed, g_pal[ib].peGreen, g_pal[ib].peBlue));
    if (ha.m_h < hb.m_h) {
        return -1;
    }
    if (ha.m_h > hb.m_h) {
        return 1;
    }
    return 0;
}

CShadeTable* CShadeTableCache::FindByKey(i32 key) {
    for (i32 i = 0; i < static_cast<i32>(m_arr.size()); i++) {
        if (m_arr[i]->m_key == key) {
            return m_arr[i];
        }
    }
    return NULL;
}

void CShadeTableCache::FindRemove(CShadeTable* key) {
    i32 n = static_cast<i32>(m_arr.size());
    for (i32 i = 0; i < n; i++) {
        if (m_arr[i] == key) {
            m_arr[i]->Free();
            CShadeTable* t = m_arr[i];
            if (t) {
                t->Reset();
                delete t;
            }
            m_arr.erase(m_arr.begin() + i);
            return;
        }
    }
}

i32 __cdecl CShadeTableCache::FindNearestColor(PALETTEENTRY* pal, u8 r, u8 g, u8 b) {
    i32 best = 0;
    i32 bestDist = SQR(r - pal->peRed) + SQR(g - pal->peGreen) + SQR(b - pal->peBlue);
    for (i32 i = 1; i < PALETTE_ENTRY_COUNT; i++) {
        i32 d = SQR(r - pal[i].peRed) + SQR(g - pal[i].peGreen) + SQR(b - pal[i].peBlue);
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

ColorHSV RgbToHsv(u32 color) {
    ColorHSV hsv;
    float v = static_cast<float>(max(max(GetRValue(color), GetGValue(color)), GetBValue(color)));
    float mn = static_cast<float>(min(min(GetRValue(color), GetGValue(color)), GetBValue(color)));
    float h;

    hsv.m_v = v;
    if (v == 0.0) {
        hsv.m_s = 0.0;
        hsv.m_h = 0.0;
    } else {
        float delta = v - mn;
        hsv.m_s = delta / v;
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
        hsv.m_h = h;
    }
    return hsv;
}
