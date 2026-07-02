// ShadeTableCache.cpp - the DDrawMgr color/shade lookup-table cache (tracer
// placeholder ClassUnknown_2). A CGruntzMgr member: a polymorphic owner of a
// growable array of 0x10-byte CShadeTable buffers, each holding a 64KB RGB565 (or
// raw-RGB) color-conversion table built from the live screen RGB-format globals.
//
// Methods in ascending retail-RVA order. Field names are placeholders; offsets +
// code bytes are load-bearing. The array grow/element helpers (0x150040 /
// 0x150180 / 0x150190 / 0x1501a0 / 0x1503c0) and operator new/delete are
// external/reloc-masked.
#include <DDrawMgr/ShadeTableCache.h>

#include <math.h> // pow (__CIpow) in HsvShiftTable
#include <rva.h>
#include <stdlib.h> // qsort in the palette-sort builders
#include <string.h> // inlined memcpy (rep movsl) in FindRemove

// The live screen RGB-format shift/mask table at 0x683ea0..0x683eb4 - already
// named by CLightFxRender.cpp / CDDrawShadeBlit.cpp. The builders gate on the
// RGB565 magic state (rUp=10, gUp=5, rDown/gDown/bDown=3). Reloc-masked.
DATA(0x00283ea0)
extern i32 g_rUp; // 0x683ea0
DATA(0x00283ea4)
extern i32 g_gUp; // 0x683ea4
DATA(0x00283eac)
extern i32 g_rDown; // 0x683eac
DATA(0x00283eb0)
extern i32 g_gDown; // 0x683eb0
DATA(0x00283eb4)
extern i32 g_bDown; // 0x683eb4

// ALL-VTABLES phase: the array vtable (0x5efb28) + the CObject grand-base dtor
// vtable (0x5e8cb4) are now cl-emitted from the real CShadeArrayBase/CShadeTableArray
// polymorphic hierarchy - the manual g_shadeArrayVtbl / g_remusBaseDtorVtbl stamps
// are gone (cl auto-stamps in the array ctor + auto-resets in the array dtor).

// The working palette base (0x6bf224): the sort/remap builders stash the active
// palette pointer here for their __cdecl comparators. Reloc-masked DATA.
DATA(0x002bf224)
extern PalEntry* g_pal; // 0x6bf224

// A {h,s,v} float triple produced by RgbToHsv (0x14fcc0). The comparator and the
// HSV builder read .h (.s/.v populated but only .h drives the hue sort).
struct Hsv {
    float h, s, v;
};

// External helpers (sibling TU, reloc-masked). 0x14fcc0 converts a packed
// b<<16|g<<8|r color to its Hsv (returning out); 0x14fbf0 finds the nearest
// palette index for an (r,g,b) triple (sum-of-squares). 0x14ed10 is the sibling
// luma comparator referenced by address by the luma-sort builder.
extern "C" Hsv* RgbToHsv(Hsv* out, i32 packedRgb);                     // 0x14fcc0
extern "C" u8 NearestPaletteIndex(i32 r, PalEntry* pal, i32 g, i32 b); // 0x14fbf0
extern "C" i32 __cdecl CompareLuma(const void* a, const void* b);      // 0x14ed10

// Rez heap (reloc-masked): operator new is RezAlloc-backed; RezFree frees the
// element-array buffer in the inlined SetSize of the AddFrom* loaders.
extern "C" void* RezAlloc(u32 size); // 0x1b9b46
extern "C" void RezFree(void* p);    // 0x1b9b82

// The CString / CMemFile temps the AddFrom* loaders build (ctor/dtor external,
// reloc-masked). Modeled as tiny holders so the __thiscall ctor/dtor calls and
// the element load calls bind by shape.
struct CStr {
    char* m_p;
    CStr(const char* s); // 0x1b9ba3
    ~CStr();             // 0x1b9cde
};
struct CMemFile {
    char m_buf[0x2c];
    CMemFile(u8* data, i32 size); // 0x1cce4f (+ Attach 0x1cced7)
    ~CMemFile();                  // 0x1ccf14
};

// ===========================================================================
// CShadeTableArray - the embedded element-array subobject. Its inline ctor/dtor
// fold into the cache ctor/dtor: stamp the array vtable, zero/free m_pData.
// ===========================================================================
// Empty grand-base dtor: cl emits just the CObject vptr reset (0x5e8cb4) at the
// array dtor's tail.
inline CShadeArrayBase::~CShadeArrayBase() {}

inline CShadeTableArray::CShadeTableArray() {
    // cl auto-stamps ??_7CShadeTableArray (0x5efb28) here (was m_vtbl = &g_shadeArrayVtbl).
    m_pData = 0;
    m_nGrowBy = 0;
    m_nMaxSize = 0;
    m_nSize = 0;
}

inline CShadeTableArray::~CShadeTableArray() {
    // cl resets the vptr to 0x5efb28 (entry), runs the free, then chains
    // ~CShadeArrayBase which resets the CObject vtable (0x5e8cb4) - was the two
    // manual m_vtbl stamps.
    if (m_pData) {
        operator delete(m_pData);
    }
}

// ===========================================================================
// 0x14de30 - ctor: array subobject ctor (stamp vtable, zero fields), then the
// leading gate.
// ===========================================================================
RVA(0x0014de30, 0x1a)
CShadeTableCache::CShadeTableCache() {
    m_initialized = 0;
}

// ===========================================================================
// 0x14de50 - ~ : FreeNodes, then the inline array-subobject teardown. EH frame.
// ===========================================================================
RVA(0x0014de50, 0x6b)
CShadeTableCache::~CShadeTableCache() {
    if (m_initialized) {
        FreeNodes();
    }
}

// ===========================================================================
// 0x14dec0 - Init: mark the gate and report success.
// ===========================================================================
RVA(0x0014dec0, 0xc)
i32 CShadeTableCache::Init() {
    m_initialized = 1;
    return 1;
}

// ===========================================================================
// 0x14ded0 - FreeNodes: destroy + free every element, then drop the array.
// ===========================================================================
RVA(0x0014ded0, 0x64)
void CShadeTableCache::FreeNodes() {
    for (i32 i = 0; i < m_arr.m_nSize; i++) {
        m_arr.m_pData[i]->Destroy();
        CShadeTable* t = m_arr.m_pData[i];
        if (t) {
            t->Free();
            operator delete(t);
        }
    }
    if (m_arr.m_pData) {
        operator delete(m_arr.m_pData);
        m_arr.m_pData = 0;
    }
    m_arr.m_nMaxSize = 0;
    m_arr.m_nSize = 0;
}

// Luma-shift float constants at 0x5efb40..0x5efb5c (the gamma/luminance build).
// Reloc-masked .rdata literals; named so the operands pair.
extern float g_one;    // 1.0
extern float g_255;    // 255.0
extern float g_p01;    // 0.01
extern float g_lumaR;  // 0.5859375
extern float g_lumaG;  // 0.296875
extern float g_lumaB;  // 0.109375
extern float g_inv255; // 1/255
DATA(0x001efb5c)
extern float g_negone; // -1.0

// ===========================================================================
// 0x14df40 - FlashTable: a 256 x (nA+nB)-byte per-palette brightness-pulse ramp.
// Each palette color gets an (nA+nB)-byte ramp: phase 1 fades the channel IN from
// startPct% brightness to full over nA steps; the palette entry is then brightened
// +16 (clamped, in place); phase 2 fades the (brightened) channel OUT toward
// endPct% over nB steps. Every ramp byte is the nearest palette index of the
// (r,g,b) triple. The element-array grow is inlined (RezAlloc/RezFree, the MFC
// SetSize(nSize+1) algorithm), like AddFrom*. EH frame + x87.
// ===========================================================================
// @early-stop
// EH-frame wall (rezalloc-placement-new-no-eh-frame.md) + dense x87 schedule
// (x87-fp-stack-schedule.md): the new+throwing-ctor /GX frame is absent on MSVC5
// (element ctor 0x150180 external), and retail's fld/fxch/fmul ordering across the
// per-channel lerps does not reproduce from C. Phase 1 (fade-in from
// startPct%->full), the in-place +16 palette highlight, and the FindNearestColor
// remap are recovered byte-for-structure; the phase-2 fade-out's exact channel
// expression is the wall's residual (re-derive in the final sweep). Logic complete.
RVA(0x0014df40, 0x5f4)
CShadeTable* CShadeTableCache::FlashTable(PalEntry* pal, i32 nA, i32 nB, i32 startPct, i32 endPct) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return 0;
    }
    i32 total = nA + nB;
    if (!t->Alloc(total << 8, 0)) {
        return 0;
    }

    // Inline the element-array grow by one (MFC SetSize(nSize+1)).
    i32 oldSize = m_arr.m_nSize;
    i32 newSize = oldSize + 1;
    if (newSize == 0) {
        if (m_arr.m_pData) {
            RezFree(m_arr.m_pData);
            m_arr.m_pData = 0;
        }
        m_arr.m_nMaxSize = 0;
        m_arr.m_nSize = 0;
    } else if (m_arr.m_pData == 0) {
        m_arr.m_pData = (CShadeTable**)RezAlloc(newSize * 4);
        memset(m_arr.m_pData, 0, newSize * 4);
        m_arr.m_nMaxSize = newSize;
        m_arr.m_nSize = newSize;
    } else if (newSize <= m_arr.m_nMaxSize) {
        if (newSize > m_arr.m_nSize) {
            memset(&m_arr.m_pData[m_arr.m_nSize], 0, (newSize - m_arr.m_nSize) * 4);
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
        i32 newMax = m_arr.m_nMaxSize + grow;
        if (newSize > newMax) {
            newMax = newSize;
        }
        CShadeTable** data = (CShadeTable**)RezAlloc(newMax * 4);
        memcpy(data, m_arr.m_pData, m_arr.m_nSize * 4);
        memset(&data[m_arr.m_nSize], 0, (newSize - m_arr.m_nSize) * 4);
        RezFree(m_arr.m_pData);
        m_arr.m_pData = data;
        m_arr.m_nSize = newSize;
        m_arr.m_nMaxSize = newMax;
    }
    m_arr.m_pData[oldSize] = t;

    u8* data = t->m_data;
    for (i32 i = 0; i < 0x100; i++) {
        PalEntry* p = &pal[i];
        u8* ramp = &data[i * total];
        // Phase 1: fade in from startPct% brightness to full over nA steps.
        for (i32 j = 0; j < nA; j++) {
            float tt = (float)j / (float)nA;
            float inv = g_one - tt;
            float fr = (float)(startPct * (i32)p->r / 100) * inv + (float)(i32)p->r * tt;
            i32 rn = (i32)(fr < g_255 ? fr : g_255);
            float fg = (float)(startPct * (i32)p->g / 100) * inv + (float)(i32)p->g * tt;
            i32 gn = (i32)(fg < g_255 ? fg : g_255);
            float fb = (float)(startPct * (i32)p->b / 100) * inv + (float)(i32)p->b * tt;
            i32 bn = (i32)(fb < g_255 ? fb : g_255);
            ramp[j] = NearestPaletteIndex(rn, pal, gn, bn);
        }
        // Brighten the palette entry +16 (clamped) in place for the fade-out.
        i32 br = (i32)p->r + 0x10;
        p->r = (u8)(br < 0xff ? br : 0xff);
        i32 bg = (i32)p->g + 0x10;
        p->g = (u8)(bg < 0xff ? bg : 0xff);
        i32 bb = (i32)p->b + 0x10;
        p->b = (u8)(bb < 0xff ? bb : 0xff);
        // Phase 2: fade the brightened channel out toward endPct% over nB steps.
        for (i32 k = 0; k < nB; k++) {
            float uu = (float)k / (float)nB;
            float inv = g_one - uu;
            float fr = (float)(i32)p->r * inv + (float)(i32)p->r * (float)endPct * g_p01 * uu;
            i32 rn = (i32)(fr < g_255 ? fr : g_255);
            float fg = (float)(i32)p->g * inv + (float)(i32)p->g * (float)endPct * g_p01 * uu;
            i32 gn = (i32)(fg < g_255 ? fg : g_255);
            float fb = (float)(i32)p->b * inv + (float)(i32)p->b * (float)endPct * g_p01 * uu;
            i32 bn = (i32)(fb < g_255 ? fb : g_255);
            ramp[nA + k] = NearestPaletteIndex(rn, pal, gn, bn);
        }
    }
    return t;
}

// ===========================================================================
// 0x14e540 - HsvShiftTable: a (256 x steps)-byte luma-gamma shift table. For each
// palette color and each step, compute the luminance (0.586 R + 0.297 G +
// 0.109 B), drive a pow()-based gamma factor from the per-step level, scale each
// channel by it with a 255 clamp, and map back to the nearest palette index.
// EH frame + x87 (pow via __CIpow, __ftol). @early-stop
// ===========================================================================
// @early-stop
// EH-frame wall (rezalloc-placement-new-no-eh-frame.md) + dense x87 schedule
// (x87-fp-stack-schedule.md): the luma/pow/per-channel gamma is reconstructed but
// retail's fld/fxch/faddp ordering across the luma sum, the __CIpow call, and the
// three clamped channel scales does not reproduce from C; ~50-60%.
RVA(0x0014e540, 0x2ea)
CShadeTable* CShadeTableCache::HsvShiftTable(PalEntry* pal, i32 steps, i32 packedColor) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return 0;
    }
    if (!t->Alloc(steps << 8, 0)) {
        t->Free();
        operator delete(t);
        return 0;
    }
    i32 idx = m_arr.m_nSize;
    m_arr.SetSizeGrow(idx + 1, -1);
    m_arr.m_pData[idx] = t;
    u8* data = t->m_data;
    float gamma = (float)((packedColor & 0xff)) * g_p01;
    for (i32 i = 0; i < 0x100; i++) {
        PalEntry* p = &pal[i];
        float fr = (float)(i32)p->r;
        float fg = (float)(i32)p->g;
        float fb = (float)(i32)p->b;
        float luma = fr * g_lumaR + fg * g_lumaG + fb * g_lumaB;
        for (i32 j = 0; j < steps; j++) {
            float level = (float)j / (float)steps;
            float factor = (float)pow((double)(luma * g_inv255), (double)(level * gamma));
            i32 rn = (i32)(fr * factor < g_255 ? fr * factor : g_255);
            i32 gn = (i32)(fg * factor < g_255 ? fg * factor : g_255);
            i32 bn = (i32)(fb * factor < g_255 ? fb * factor : g_255);
            data[i * steps + j] = NearestPaletteIndex(rn, pal, gn, bn);
        }
    }
    return t;
}

// ===========================================================================
// 0x14e830 - HueRampTable: a (256 x steps)-byte gradient table. For each palette
// color i and each step j the channel is linearly blended toward the target
// color (arg2): out = (1 - j/steps)*pal[i] + (j/steps)*color, then mapped to the
// nearest palette index. Channels processed b,g,r. EH frame + x87. @early-stop
// ===========================================================================
// @early-stop
// EH-frame wall (rezalloc-placement-new-no-eh-frame.md) + x87 fxch schedule
// (x87-fp-stack-schedule.md): body reconstructed (the per-channel lerp toward
// arg2 by t=j/steps), but the /GX frame is absent and retail's fld/fxch/fmul
// ordering of the three interleaved channel lerps diverges from the recompile.
RVA(0x0014e830, 0x1b9)
CShadeTable* CShadeTableCache::HueRampTable(PalEntry* pal, i32 steps, i32 packedColor) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return 0;
    }
    if (!t->Alloc(steps << 8, 0)) {
        return 0;
    }
    i32 idx = m_arr.m_nSize;
    m_arr.SetSizeGrow(idx + 1, -1);
    m_arr.m_pData[idx] = t;
    u8* data = t->m_data;
    i32 cr = packedColor & 0xff;
    i32 cg = (packedColor >> 8) & 0xff;
    i32 cb = (packedColor >> 0x10) & 0xff;
    for (i32 i = 0; i < 0x100; i++) {
        PalEntry* p = &pal[i];
        for (i32 j = 0; j < steps; j++) {
            float t1 = (float)j / (float)steps;
            float t0 = 1.0f - t1;
            i32 bn = (i32)(t0 * (float)(i32)p->b + t1 * (float)cb);
            i32 gn = (i32)(t0 * (float)(i32)p->g + t1 * (float)cg);
            i32 rn = (i32)(t0 * (float)(i32)p->r + t1 * (float)cr);
            data[i * steps + j] = NearestPaletteIndex(rn, pal, gn, bn);
        }
    }
    return t;
}

// ===========================================================================
// 0x14e9f0 - GammaTable: a 0x10000-byte (256x256) two-palette cross-blend table.
// For each (row i, col j) the per-channel value is the weighted blend
// (pal[j].ch*wCol/100 + pal[i].ch*wRow/100) / ((wRow+wCol)/100), mapped back to
// the nearest palette index. The /100 lowers to imul 0x51eb851f;sar 5; the final
// /div is a runtime idiv. EH frame. @early-stop
// ===========================================================================
// @early-stop
// EH-frame wall (rezalloc-placement-new-no-eh-frame.md) + div-by-100 strength
// reduction + runtime idiv scheduling: body reconstructed (256x256, two /100
// blends summed then /div per channel), but the /GX frame is absent and MSVC
// orders the strength-reduced accumulators differently than retail.
RVA(0x0014e9f0, 0x208)
CShadeTable* CShadeTableCache::GammaTable(PalEntry* pal, i32 wRow, i32 wCol) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return 0;
    }
    if (!t->Alloc(0x10000, 0)) {
        return 0;
    }
    i32 idx = m_arr.m_nSize;
    m_arr.SetSizeGrow(idx + 1, -1);
    m_arr.m_pData[idx] = t;
    u8* data = t->m_data;
    i32 div = (wRow + wCol) / 100;
    for (i32 i = 0; i < 0x100; i++) {
        PalEntry* pr = &pal[i];
        for (i32 j = 0; j < 0x100; j++) {
            PalEntry* pc = &pal[j];
            i32 r = (pc->r * wCol / 100 + pr->r * wRow / 100) / div;
            i32 g = (pc->g * wCol / 100 + pr->g * wRow / 100) / div;
            i32 b = (pc->b * wCol / 100 + pr->b * wRow / 100) / div;
            data[i * 0x100 + j] = NearestPaletteIndex(r, pal, g, b);
        }
    }
    return t;
}

// ===========================================================================
// 0x14ec00 - LumaSortTable: build a 0x200-byte palette-remap table. Fill the
// first 256 bytes with the identity, qsort them by luma (CompareLuma, 0x14ed10),
// then write the inverse permutation into bytes [0x100..0x1ff]. The active
// palette is published to g_pal for the comparator. EH frame.
// ===========================================================================
// @early-stop
// EH-frame wall (rezalloc-placement-new-no-eh-frame.md): the new+throwing-ctor
// /GX frame is absent on MSVC5 (element ctor 0x150180 external); body + the two
// remap loops byte-exact otherwise.
RVA(0x0014ec00, 0x10f)
CShadeTable* CShadeTableCache::LumaSortTable(PalEntry* pal) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return 0;
    }
    if (!t->Alloc(0x200, 0)) {
        return 0;
    }
    i32 idx = m_arr.m_nSize;
    m_arr.SetSizeGrow(idx + 1, -1);
    m_arr.m_pData[idx] = t;
    u8* data = t->m_data;
    g_pal = pal;
    for (i32 i = 0; i < 0x100; i++) {
        data[i] = (u8)i;
    }
    qsort(data, 0x100, 1, CompareLuma);
    for (i32 c = 0; c < 0x100; c++) {
        for (i32 j = 0; j < 0x100; j++) {
            if (data[j] == c) {
                data[0x100 + c] = (u8)j;
                break;
            }
        }
    }
    return t;
}

// ===========================================================================
// 0x14ede0 - HueSortTable: identical to LumaSortTable but the qsort comparator
// is CompareHue (0x14fa60), so the palette is remapped in hue order. EH frame.
// ===========================================================================
// @early-stop
// EH-frame wall (rezalloc-placement-new-no-eh-frame.md): see LumaSortTable.
RVA(0x0014ede0, 0x10f)
CShadeTable* CShadeTableCache::HueSortTable(PalEntry* pal) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return 0;
    }
    if (!t->Alloc(0x200, 0)) {
        return 0;
    }
    i32 idx = m_arr.m_nSize;
    m_arr.SetSizeGrow(idx + 1, -1);
    m_arr.m_pData[idx] = t;
    u8* data = t->m_data;
    g_pal = pal;
    for (i32 i = 0; i < 0x100; i++) {
        data[i] = (u8)i;
    }
    qsort(data, 0x100, 1, CompareHue);
    for (i32 c = 0; c < 0x100; c++) {
        for (i32 j = 0; j < 0x100; j++) {
            if (data[j] == c) {
                data[0x100 + c] = (u8)j;
                break;
            }
        }
    }
    return t;
}

// ===========================================================================
// 0x14eef0 - GreyTable: allocate a 0x20000-byte (64K-entry u16) identity remap.
// Two loop variants gated on the RGB565 screen format. EH frame (documented wall:
// the RezAlloc + external element ctor emits no /GX frame on MSVC5). @early-stop
// ===========================================================================
// @early-stop
// EH-frame wall: retail's `new`+throwing-element-ctor carries a /GX frame this
// recompile can't emit (element ctor is external ClassUnknown_4_150180); body
// byte-exact, frame+epilogue cascade absent. See rezalloc-placement-new-no-eh-frame.md
RVA(0x0014eef0, 0x183)
CShadeTable* CShadeTableCache::GreyTable() {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return 0;
    }
    if (!t->Alloc(0x20000, 0)) {
        t->Free();
        operator delete(t);
        return 0;
    }
    i32 idx = m_arr.m_nSize;
    m_arr.SetSizeGrow(idx + 1, -1);
    m_arr.m_pData[idx] = t;
    u16* out = (u16*)t->m_data;
    if (g_rDown == 3 && g_gDown == 3 && g_bDown == 3 && g_rUp == 0xa && g_gUp == 5) {
        for (i32 v = 0; v < 0x10000; v++) {
            *out++ = (u16)(((((u8)(v >> 0xb) << 4) + ((v >> 6) & 0xf)) << 4) + ((v >> 1) & 0xf));
        }
    } else {
        for (i32 v = 0; v < 0x10000; v++) {
            *out++ = (u16)(((((u8)(v >> 0xc) << 4) + ((v >> 7) & 0xf)) << 4) + ((v >> 1) & 0xf));
        }
    }
    return t;
}

// ===========================================================================
// 0x14f080 - AddTable: a 0x20000-byte additive/glow blend table. For each alpha
// level v (0..240 step 16) and each quantized source color (r/g/b = 8..248 step
// 16), brighten the channel by f = 1 + (v*scale)/255 and pack to RGB565. The
// channels go through __ftol after a 255 ceiling clamp. EH frame. @early-stop
// ===========================================================================
// @early-stop
// EH-frame wall (rezalloc-placement-new-no-eh-frame.md) + x87 fxch schedule
// (x87-fp-stack-schedule.md): body reconstructed (4-deep v/r/g/b loop, the factor
// recomputed per innermost iter to match retail's structure), but the /GX
// ctor-in-flight frame is absent on MSVC5 and retail fuses the first (r) channel
// into the factor build via fxch (uniform fild;fmul here); ~60%.
RVA(0x0014f080, 0x283)
CShadeTable* CShadeTableCache::AddTable(float scale) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return 0;
    }
    if (!t->Alloc(0x20000, 0)) {
        t->Free();
        operator delete(t);
        return 0;
    }
    i32 idx = m_arr.m_nSize;
    m_arr.SetSizeGrow(idx + 1, -1);
    m_arr.m_pData[idx] = t;
    u16* out = (u16*)t->m_data;
    for (i32 v = 0; v < 0x100; v += 0x10) {
        for (i32 r = 8; r < 0x100; r += 0x10) {
            for (i32 g = 8; g < 0x100; g += 0x10) {
                for (i32 b = 8; b < 0x100; b += 0x10) {
                    u8 rc = (u8)(r < 0xff ? r : 0xff);
                    u8 gc = (u8)(g < 0xff ? g : 0xff);
                    u8 bc = (u8)(b < 0xff ? b : 0xff);
                    float f = (float)v * scale * (1.0f / 255.0f) + 1.0f;
                    float fr = (float)(i32)rc * f;
                    i32 rn = (i32)(fr < 255.0f ? fr : 255.0f);
                    float fg = (float)(i32)gc * f;
                    i32 gn = (i32)(fg < 255.0f ? fg : 255.0f);
                    float fb = (float)(i32)bc * f;
                    i32 bn = (i32)(fb < 255.0f ? fb : 255.0f);
                    *out++ = (u16)(((u8)((u8)rn >> (u8)g_rDown) << g_rUp)
                                   | ((u8)((u8)gn >> (u8)g_gDown) << g_gUp)
                                   | (u8)((u8)bn >> (u8)g_bDown));
                }
            }
        }
    }
    return t;
}

// ===========================================================================
// 0x14f310 - SubTable: a 0x20000-byte subtractive/darken blend table keyed by a
// packed RGB color arg. For each level (15..0) and each quantized source color,
// quantize src/15 and add the per-level color contribution color*level/15, then
// pack to RGB565. Pure integer (the /15 lowers to imul 0x88888889; sar 3). EH
// frame. @early-stop
// ===========================================================================
// @early-stop
// EH-frame wall (rezalloc-placement-new-no-eh-frame.md) + div-by-15 strength
// reduction scheduling: body reconstructed (level 15..0 x r/g/b, color split into
// cr/cg/cb, /15 via imul 0x88888889;sar 3), but the /GX frame is absent and MSVC
// strength-reduces the per-level color accumulators with a different init/step
// schedule than retail; ~61%.
RVA(0x0014f310, 0x297)
CShadeTable* CShadeTableCache::SubTable(i32 color) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return 0;
    }
    if (!t->Alloc(0x20000, 0)) {
        t->Free();
        operator delete(t);
        return 0;
    }
    i32 idx = m_arr.m_nSize;
    m_arr.SetSizeGrow(idx + 1, -1);
    m_arr.m_pData[idx] = t;
    u16* out = (u16*)t->m_data;
    i32 cr = color & 0xff;
    i32 cg = (color >> 8) & 0xff;
    i32 cb = (color >> 0x10) & 0xff;
    for (i32 level = 0xf; level >= 0; level--) {
        i32 subr = cr * level / 0xf;
        i32 subg = cg * level / 0xf;
        i32 subb = cb * level / 0xf;
        for (i32 r = 0; r < 0x10; r++) {
            u8 rn = (u8)(((r * level / 0xf) << 4) + subr);
            for (i32 g = 0; g < 0x10; g++) {
                u8 gn = (u8)(((g * level / 0xf) << 4) + subg);
                for (i32 b = 0; b < 0x10; b++) {
                    u8 bn = (u8)(((b * level / 0xf) << 4) + subb);
                    *out++ =
                        (u16)(((u8)(bn >> (u8)g_bDown)) | ((u8)((u8)rn >> (u8)g_rDown) << g_rUp)
                              | ((u8)((u8)gn >> (u8)g_gDown) << g_gUp));
                }
            }
        }
    }
    return t;
}

// ===========================================================================
// 0x14f5b0 - AlphaTable: allocate a 0x200-byte (256-entry u16) table and fill it
// with the RGB565 conversion of a 256-color RGBA palette (arg). EH frame.
// ===========================================================================
// @early-stop
// EH-frame wall (rezalloc-placement-new-no-eh-frame.md): body byte-exact, the
// /GX ctor-in-flight frame is absent on MSVC5; ~66%.
RVA(0x0014f5b0, 0x10a)
CShadeTable* CShadeTableCache::AlphaTable(u8* pal) {
    CShadeTable* t = new CShadeTable;
    if (!t) {
        return 0;
    }
    if (!t->Alloc(0x200, 0)) {
        t->Free();
        operator delete(t);
        return 0;
    }
    i32 idx = m_arr.m_nSize;
    m_arr.SetSizeGrow(idx + 1, -1);
    m_arr.m_pData[idx] = t;
    u16* out = (u16*)t->m_data;
    u8* p = pal;
    for (i32 i = 0x100; i != 0; i--) {
        u16 v = (u16)(((u8)(p[0] >> (u8)g_rDown) << g_rUp) | ((u8)(p[1] >> (u8)g_gDown) << g_gUp)
                      | (u8)(p[2] >> (u8)g_bDown));
        *out++ = v;
        p += 4;
    }
    return t;
}

// ===========================================================================
// 0x14f6c0 - AddFromArray: grow the element array by one (the MFC SetSize(n+1)
// algorithm is inlined here, not the out-of-line SetSizeGrow other builders
// call), allocate + push a CShadeTable, build a CString from the name arg, load
// the table from it, and FindRemove it on failure. EH frame + inlined MFC array
// growth + a CString temp. @early-stop
// ===========================================================================
// @early-stop
// EH-frame + inlined-MFC-SetSize wall: the array-grow algorithm is open-coded
// (the realloc/capacity-bump/rep-movs copy + RezAlloc/RezFree) inline rather than
// the out-of-line SetSizeGrow, and the /GX frame + CString temp teardown
// (1b9ba3/1b9cde) don't reproduce; logic complete, scheduling parks it.
RVA(0x0014f6c0, 0x1e1)
CShadeTable* CShadeTableCache::AddFromArray(const char* name) {
    CShadeTable* t = new CShadeTable;
    i32 oldSize = m_arr.m_nSize;
    i32 newSize = oldSize + 1;
    if (newSize == 0) {
        if (m_arr.m_pData) {
            RezFree(m_arr.m_pData);
            m_arr.m_pData = 0;
        }
        m_arr.m_nMaxSize = 0;
        m_arr.m_nSize = 0;
    } else if (m_arr.m_pData == 0) {
        m_arr.m_pData = (CShadeTable**)RezAlloc(newSize * 4);
        memset(m_arr.m_pData, 0, newSize * 4);
        m_arr.m_nMaxSize = newSize;
        m_arr.m_nSize = newSize;
    } else if (newSize <= m_arr.m_nMaxSize) {
        if (newSize > m_arr.m_nSize) {
            memset(&m_arr.m_pData[m_arr.m_nSize], 0, (newSize - m_arr.m_nSize) * 4);
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
        i32 newMax = m_arr.m_nMaxSize + grow;
        if (newSize > newMax) {
            newMax = newSize;
        }
        CShadeTable** data = (CShadeTable**)RezAlloc(newMax * 4);
        memcpy(data, m_arr.m_pData, m_arr.m_nSize * 4);
        memset(&data[m_arr.m_nSize], 0, (newSize - m_arr.m_nSize) * 4);
        RezFree(m_arr.m_pData);
        m_arr.m_pData = data;
        m_arr.m_nSize = newSize;
        m_arr.m_nMaxSize = newMax;
    }
    m_arr.m_pData[oldSize] = t;
    CStr cstr(name);
    if (!t->Load(cstr, 0)) {
        FindRemove(t);
        return 0;
    }
    return t;
}

// ===========================================================================
// 0x14f8b0 - AddFromFile: as AddFromArray, but load the table from a CMemFile
// over the (buffer,size) args via the element's file loader (0x150330). The
// element-array growth is again inlined. EH frame. @early-stop
// ===========================================================================
// @early-stop
// EH-frame + inlined-MFC-SetSize wall (see AddFromArray): plus a CMemFile temp
// instead of CString; logic complete, scheduling parks it.
RVA(0x0014f8b0, 0x1b0)
CShadeTable* CShadeTableCache::AddFromFile(const char* name, i32 size) {
    CShadeTable* t = new CShadeTable;
    i32 oldSize = m_arr.m_nSize;
    i32 newSize = oldSize + 1;
    if (newSize == 0) {
        if (m_arr.m_pData) {
            RezFree(m_arr.m_pData);
            m_arr.m_pData = 0;
        }
        m_arr.m_nMaxSize = 0;
        m_arr.m_nSize = 0;
    } else if (m_arr.m_pData == 0) {
        m_arr.m_pData = (CShadeTable**)RezAlloc(newSize * 4);
        memset(m_arr.m_pData, 0, newSize * 4);
        m_arr.m_nMaxSize = newSize;
        m_arr.m_nSize = newSize;
    } else if (newSize <= m_arr.m_nMaxSize) {
        if (newSize > m_arr.m_nSize) {
            memset(&m_arr.m_pData[m_arr.m_nSize], 0, (newSize - m_arr.m_nSize) * 4);
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
        i32 newMax = m_arr.m_nMaxSize + grow;
        if (newSize > newMax) {
            newMax = newSize;
        }
        CShadeTable** data = (CShadeTable**)RezAlloc(newMax * 4);
        memcpy(data, m_arr.m_pData, m_arr.m_nSize * 4);
        memset(&data[m_arr.m_nSize], 0, (newSize - m_arr.m_nSize) * 4);
        RezFree(m_arr.m_pData);
        m_arr.m_pData = data;
        m_arr.m_nSize = newSize;
        m_arr.m_nMaxSize = newMax;
    }
    m_arr.m_pData[oldSize] = t;
    CMemFile mf((u8*)name, size);
    if (!t->LoadFile(&mf, size, 0)) {
        FindRemove(t);
        return 0;
    }
    return t;
}

// ===========================================================================
// 0x14fa60 - CompareHue: __cdecl qsort comparator. Converts the two palette
// entries (indexed by the byte each arg points at) to HSV via RgbToHsv and
// orders them by hue. Plain frame (no /GX): the Hsv locals are trivial.
// ===========================================================================
// @early-stop
// regalloc wall: body + frame (sub esp,0x24, the *a/*b spill-reload, the two
// fcomp/fnstsw return canonicalizations) byte-exact; retail reads arg a before
// arg b and pins g_pal in edx (index in ecx), the recompile reads b first and
// swaps the pair - pure register-allocation noise, ~90%.
RVA(0x0014fa60, 0xd7)
i32 __cdecl CShadeTableCache::CompareHue(const void* a, const void* b) {
    u8 ia = *(const u8*)a;
    u8 ib = *(const u8*)b;
    Hsv ha, hb, tmp;
    PalEntry* pa = &g_pal[ia];
    ha = *RgbToHsv(&tmp, (pa->b << 0x10) | (pa->g << 8) | pa->r);
    PalEntry* pb = &g_pal[ib];
    hb = *RgbToHsv(&tmp, (pb->b << 0x10) | (pb->g << 8) | pb->r);
    if (ha.h < hb.h) {
        return -1;
    }
    if (ha.h > hb.h) {
        return 1;
    }
    return 0;
}

// ===========================================================================
// 0x14fb80 - FindRemove: locate the table whose +0 matches the key, destroy it,
// and memmove the tail down one slot. __thiscall, one arg.
// ===========================================================================
RVA(0x0014fb80, 0x68)
void CShadeTableCache::FindRemove(CShadeTable* key) {
    i32 n = m_arr.m_nSize;
    i32 i = 0;
    if (n > 0) {
        CShadeTable** w = m_arr.m_pData;
        for (;;) {
            if (*w == key) {
                break;
            }
            i++;
            w++;
            if (i >= n) {
                return;
            }
        }
        m_arr.m_pData[i]->Destroy();
        CShadeTable* t = m_arr.m_pData[i];
        if (t) {
            t->Free();
            operator delete(t);
        }
        i32 cnt = m_arr.m_nSize - i - 1;
        CShadeTable** dst = &m_arr.m_pData[i];
        if (cnt) {
            CShadeTable** src = &m_arr.m_pData[i + 1];
            memcpy(dst, src, cnt * sizeof(CShadeTable*));
        }
        m_arr.m_nSize--;
    }
}

// ===========================================================================
// 0x14fbf0 - FindNearestColor: nearest-palette-color search. Mask the target to
// bytes, seed the running best with entry 0's squared (r,g,b) distance, then scan
// entries 1..255 keeping the minimum; return its index.
// ===========================================================================
// @early-stop
// regalloc wall (~64%): logic byte-for-byte correct, but retail spills r/b/i/best
// to stack (keeps only g in a reg) and reserves the extra slot via `push ecx`,
// whereas MSVC keeps r/g/b all in registers here (4 pushes, reuses the dead arg
// slot). Pure register-allocation/stack-layout divergence; instruction selection
// matches. Cascades register names through the whole body. Final-sweep candidate.
RVA(0x0014fbf0, 0xcb)
i32 __cdecl CShadeTableCache::FindNearestColor(PalEntry* pal, i32 r, i32 g, i32 b) {
    r &= 0xff;
    g &= 0xff;
    b &= 0xff;
    i32 dg = g - pal->g;
    i32 db = b - pal->b;
    i32 dr = r - pal->r;
    i32 bestDist = dg * dg + db * db + dr * dr;
    i32 best = 0;
    for (i32 i = 1; i < 256; i++) {
        i32 dr2 = r - pal[i].r;
        i32 dg2 = g - pal[i].g;
        i32 db2 = b - pal[i].b;
        i32 d = dr2 * dr2 + dg2 * dg2 + db2 * db2;
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

// SIZE tracking for this TU's modeling-view locals (placed at EOF: any
// mid-file typedef reschedules the /O2 codegen of CompareHue/GammaTable).
SIZE_UNKNOWN(Hsv);
SIZE_UNKNOWN(CStr);
SIZE_UNKNOWN(CMemFile); // MFC-library modeling view (opaque holder)
