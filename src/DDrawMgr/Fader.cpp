#include <Gruntz/Fader.h>
#include <EmptyString.h>  // g_emptyString
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <DDrawMgr/ShadeTableCache.h>
#include <Gruntz/FaderSubtypes.h> // the six concrete subtypes (declarations)
#include <Gruntz/FadeSink.h>      // IFadeSink (the CFader::RunFade fade-notify sink; P2)
#include <Ints.h>
#include <Mfc.h>                // superset of Win32.h; needed for CDDSurface (CPtrArray member)
#include <ddraw.h> // IDirectDrawSurface::Unlock (the ex manual slot dispatch)
#include <DDrawMgr/DDSurface.h> // the real CDDSurface (was the Surf/FShadeSurf/TileSurf/FxBox views)
#include <DDrawMgr/DirectDrawMgr.h> // the real CDDPalette (its +0x0c m_cacheA is the
#include <DDrawMgr/DDrawPtrCollections.h> // the +0x2c overlay surface pool (CFaderLight BeginFade/EndFade)
#include <Gruntz/FxModeT1.h>              // the CString-bearing CFxModeT1 (ex fxmodedesc)
#include <Gruntz/FxModeDesc.h>            // CFxModeDesc + T2-T6 (ex fxmodedesc)
#include <math.h>                         // acos/sin (fsin) / sqrt (fsqrt) intrinsics
#include <string.h>                       // rep-movs / memset element copies
#include <rva.h>

DATA(0x001f07ec)
float g_fxBias = -50.0f; // 0x5f07ec
DATA(0x001f07f4)
float g_fxEps = 1.0f; // 0x5f07f4

RVA(0x0017e450, 0x23)
CFader::CFader() {
    m_table = 0;
    m_flag = 1;
}

RVA(0x0017e4a0, 0x69)
CFader::~CFader() {
    if (m_table && m_flag) {
        m_cache.FindRemove(m_table);
        m_table = 0;
    }
}

RVA(0x0017e510, 0x23)
void CFader::Wait(i32 delay) {
    DWORD target = GetTickCount() + delay;
    while (GetTickCount() < target) {
    }
}

RVA(0x0017e760, 0x11)
void CFader::SetTimers(i32 a, i32 b) {
    m_timerA = a;
    m_timerB = b;
}

RVA(0x0017e780, 0xa)
void CFader::Set2c(i32 v) {
    m_set2cArg = v;
}

RVA(0x0017e7b0, 0x9)
CFxModeDesc::CFxModeDesc() {
    m_type = 0;
}

RVA(0x0017e7c0, 0x7a)
CFxModeT1::CFxModeT1() {
    m_type = 1;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_10 = 0x32;
    m_14 = 1;
    m_18 = 1;
    m_1c = 0;
    m_20 = 0;
    m_24 = g_emptyString;
    m_28 = 0;
}

// 0x17e840 - CFxModeT2(): base ctor, stamp the type-2 record.
// @early-stop
// constant-materialization/store-scheduling wall (~72%): same stores/offsets/values as
// retail, but retail pre-loads 0x140->eax / 0xf0->ecx / 0->edx + stores in offset order,
// while cl keeps 0x140/0xf0 as immediates + groups the zero stores. Not source-steerable.
RVA(0x0017e840, 0x37)
CFxModeT2::CFxModeT2() {
    m_type = 2;
    m_04 = 0;
    m_08 = 0;
    m_10 = 1;
    m_14 = 0;
    m_18 = 0x140;
    m_1c = 0xf0;
    m_20 = 0;
}

RVA(0x0017e880, 0x28)
CFxModeT3::CFxModeT3() {
    m_type = 3;
    m_04 = 0;
    m_08 = 0;
    m_0c = 1;
    m_10 = 0xf;
}

RVA(0x0017e8b0, 0x27)
CFxModeT4::CFxModeT4() {
    m_type = 4;
    m_04 = 0;
    m_08 = 0;
    m_10 = 0;
    m_14 = 0;
    m_0c = 1;
}

RVA(0x0017e8e0, 0x27)
CFxModeT5::CFxModeT5() {
    m_type = 5;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_14 = 0;
    m_10 = 0x19;
}

RVA(0x0017e910, 0x29)
CFxModeT6::CFxModeT6() {
    m_type = 6;
    m_04 = 0;
    m_08 = 0;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    m_1c = 0;
    m_20 = 0;
}

RVA(0x0017e940, 0x27)
CFaderMesh::CFaderMesh() {}

// 0x17e990 - ~CFaderMesh (the real class dtor). Empty body:
// cl stamps ??_7CFaderMesh (masks 0x5f07c0), destroys the m_58 CRezBufferObject
// member, then chains ~CFader (0x17e4a0). /GX frame from the destructible member
// + base.
// @early-stop
// out-of-line member-dtor wall: retail INLINES ~CRezBufferObject's teardown here
// (stamp 0x5f07d8 into +0x58, free +0x5c, restamp ??_7CObject) - byte-shape
// `mov [edi],vt; mov eax,[edi+4]; test; push; call delete` - while our canonical
// ??1CRezBufferObject stays out-of-line (RezBufferObjectDtor.cpp @0x17f330, itself
// 100%-matched), so this dtor emits `lea ecx,[esi+0x58]; call ??1CRezBufferObject`
// instead. The CALL reloc binds to the real dtor (reloc-faithful); inlining it here
// would require the header-inline move that unbinds the 0x17f330 emission. A
// deliberate structure-over-% trade.
RVA(0x0017e990, 0x6b)
CFaderMesh::~CFaderMesh() {}

// ===========================================================================
// 0x17ef00 - CFaderMesh::RenderFrame(frame): the mesh-warp blit. Prime the dest surface
// (m_3c): Blt the m_40 source if set, else Clear it. Then for each of the m_58
// buffer's records (40 bytes = {srcRectA[4], dstRectB[4], _, _}): interpolate the
// A->B rect by t = frame/GetFrameCount(), clip it to the dest surface, and BltEx from the m_38
// source (srcRect = m_4c ? rectA : the clipped rectB). Finally Flip m_44.
// ===========================================================================
// @early-stop
// x87-fp-stack-schedule wall (docs/patterns/x87-fp-stack-schedule.md; sibling of
// ApplyInit @0x17ea00): the per-record t=frame/GetFrameCount() divide, the four
// A + (int)((B-A)*t) rect interpolations (fild/fmul/__ftol), the surface clip and the
// BltEx/Flip dispatch are byte-faithful in operation/offset, but retail keeps t on the
// x87 stack across the four __ftol calls and colours the record temporaries into a
// dense frame-slot layout cl doesn't reproduce; the frame/count int64->float loads
// also differ (fild qword vs dword). Not source-steerable.
RVA(0x0017ef00, 0x21c)
void CFaderMesh::RenderFrame(i32 frame) {
    CDDSurface* dst = m_dstSurface;
    if (m_primeSrc != 0) {
        dst->Blt(m_primeSrc);
    } else {
        dst->Clear(0);
    }
    if (m_meshBuf.m_nSize > 0) {
        float ff = static_cast<float>(frame);
        char* pData = reinterpret_cast<char*>(m_meshBuf.m_pData);
        for (i32 i = 0; i < m_meshBuf.m_nSize; i++) {
            i32* rec = reinterpret_cast<i32*>((pData + i * 0x28));
            i32 r0 = rec[0], r1 = rec[1], r2 = rec[2], r3 = rec[3];
            i32 r4 = rec[4], r5 = rec[5], r6 = rec[6], r7 = rec[7];
            float t = ff / static_cast<float>(GetFrameCount());

            i32 x0 = r0 + static_cast<i32>((static_cast<float>((r4 - r0)) * t));
            i32 y0 = r1 + static_cast<i32>((static_cast<float>((r5 - r1)) * t));
            i32 x1 = r2 + static_cast<i32>((static_cast<float>((r6 - r2)) * t));
            i32 y1 = r3 + static_cast<i32>((static_cast<float>((r7 - r3)) * t));

            i32 bx0 = r4, by0 = r5, bx1 = r6, by1 = r7;
            if (x0 < 0 && x1 > 0) {
                bx0 = r4 - x0;
                x0 = 0;
            } else if (x1 <= dst->m_width && x0 > dst->m_width) {
                bx1 = r6 - x1 + dst->m_width;
                x1 = dst->m_width - 1;
            }
            if (y0 < 0 && y1 > 0) {
                by0 = r5 - y0;
                y0 = 0;
            } else if (y1 <= dst->m_height && y0 > dst->m_height) {
                by1 = r7 - y1 + dst->m_height;
                y1 = dst->m_height - 1;
            }

            i32 dstRect[4] = {x0, y0, x1, y1};
            i32 srcRect[4];
            if (m_recOrderFlag != 0) {
                srcRect[0] = r0;
                srcRect[1] = r1;
                srcRect[2] = r2;
                srcRect[3] = r3;
            } else {
                srcRect[0] = bx0;
                srcRect[1] = by0;
                srcRect[2] = bx1;
                srcRect[3] = by1;
            }
            dst->BltEx(dstRect, m_bltSrc, srcRect, 0x1000000, 0);
        }
    }
    m_flipTarget->Flip(0);
}

RVA(0x0017fdb0, 0x1a)
CFaderSine::CFaderSine() {
    m_elemCount = 0;
    m_frameCount = 0;
}

RVA(0x0017fdf0, 0xb)
CFaderSine::~CFaderSine() {}

DATA(0x002c279c)
u8 g_fxRandSeeded; // 0x6c279c  seed-init flag (bit 0)
DATA(0x002c27a8)
i32 g_fxRandSeed; // 0x6c27a8  LCG seed

static __inline i32 FxRand(i32 range) {
    u32 x;
    if (!(g_fxRandSeeded & 1)) {
        g_fxRandSeeded |= 1;
        x = ::timeGetTime();
    } else {
        x = g_fxRandSeed;
    }
    g_fxRandSeed = x * 214013 + 2531011;
    return ((static_cast<i32>(g_fxRandSeed) >> 16) & 0x7fff) % range;
}

DATA(0x001f085c)
extern const float g_faderScale_5f085c = 0.01f; // 0x5f085c  intensity->magnitude scale
void ScatterSamples(i32* arr, i32, i32, i32);   // 0x182940 ?ScatterSamples@@YAXPAHHHH@Z

// @early-stop
// regalloc coin-flip wall (73.5% fuzzy). Full body is byte-shape-identical to retail;
// the residual is a callee-saved coloring swap that touches every ModRM byte: retail
// colors this->ebx and the reused const-0/count->edi, while cl picks this->edi /
// const-0->ebx (a symmetric loop-weight tie broken the other way). Verified via
// llvm-objdump -dr base vs target - the only mnemonic-level diffs are the ebx/edi
// register columns.
RVA(0x0017fe00, 0x12d)
i32 CFaderSine::ApplyInit(CFxModeDesc* desc) {
    i32 w;
    i32 p;
    i32 i;
    m_20 = 0;
    m_boxParam = desc->m_0c;
    FaderSrc* src = reinterpret_cast<FaderSrc*>(desc->m_04);
    if (!src) {
        src = reinterpret_cast<FaderSrc*>(m_timerA);
    }
    m_srcBox = src;
    i32 alt = desc->m_08;
    if (!alt) {
        alt = m_timerB;
    }
    m_dstBox = reinterpret_cast<FaderSrc*>(alt);
    if (!m_srcBox) {
        goto fail;
    }
    if (!m_dstBox) {
        m_boxParam = 1;
    }
    m_elemCount = m_srcBox->m_count;
    w = m_srcBox->m_frameCount;
    m_frameCount = w;
    p = desc->m_10;
    if (p < 0) {
        goto fail;
    }
    if (p > 100) {
        goto fail;
    }
    m_intensity = p;
    m_scaledMag = static_cast<i32>((static_cast<float>(p) * g_faderScale_5f085c * w));
    for (i = 0; i < 2000; i++) {
        m_arr0[i] = 0;
        m_arr2[i] = 0;
        m_arr3[i] = 0;
        m_arr1[i] = FxRand(m_elemCount);
    }
    ScatterSamples(m_arr3, 0, m_elemCount, 1);
    return 1;
fail:
    return 0;
}

RVA(0x0017f530, 0x19)
CFaderFlat::CFaderFlat() {
    m_frames = 0;
}

RVA(0x00180410, 0x19)
CFaderLight::CFaderLight() {
    m_overlay = 0;
}

RVA(0x00180450, 0x4f)
CFaderLight::~CFaderLight() {
    SubFree180630();
}

DATA(0x001f0888)
extern const double g_faderPowK = 2.0; // 2.0

SIZE(CShadeTable, 0x10);      // array-element stride (0x10-byte buffer wrapper)
SIZE(CShadeTableArray, 0x14); // MFC CObArray-shaped subobject (cache 0x18 - 0x04)
SIZE(PalEntry, 0x4);          // 4-byte palette record (256-entry array stride)
SIZE(CShadeTableCache, 0x18); // RE'd heap-alloc size (CGruntzMgr +0x50)

// CFaderLight::ApplyInit (0x1804a0): capture the descriptor's surface/palette/centre,
// clip the centre to the surface rect (early-out if outside), fill the per-scanline span
// tables, resolve the hue-ramp shade table into the base's m_table (m_flag = "we own it",
// so ~CFader FindRemoves it from the cache).
// @early-stop
// 92% - /O2 regalloc entropy tail: the descriptor field loads + the m_3c/m_surface
// conditional reuse eax in retail but the recompile distributes them across ecx/edx/eax;
// same instruction selection + scheduling, only the register names differ. Final sweep.
RVA(0x0001804a0, 0x182)
i32 CFaderLight::ApplyInit(CFxModeDesc* desc) {
    CFxModeT2* d = static_cast<CFxModeT2*>(desc); // fader type 1 -> the id-2 mode record
    m_20 = 0;
    CDDSurface* s = reinterpret_cast<CDDSurface*>(d->m_04);
    if (s == 0) {
        s = reinterpret_cast<CDDSurface*>(m_timerA); // the base's default-surface dword
    }
    m_surface = s;
    CDDSurface* b = reinterpret_cast<CDDSurface*>(d->m_08); // desc dword holds a surface
    if (b == 0) {
        m_dstSurface = reinterpret_cast<CDDSurface*>(m_timerB);
    } else {
        m_dstSurface = b;
    }
    m_lightGate = d->m_10;
    m_centerX = d->m_18;
    m_centerY = d->m_1c;
    CDDPalette* pal = reinterpret_cast<CDDPalette*>(d->m_0c);
    m_palette = pal;
    i32 cnt = d->m_14;
    m_spanCount = cnt;
    if (cnt > 0 && d->m_20 == 0 && pal == 0) {
        return 0;
    }
    if (m_surface == 0) {
        return 0;
    }
    if (m_dstSurface == 0 && m_lightGate == 0) {
        return 0;
    }
    RECT rect;
    rect.right = m_surface->m_width; // +0x1c
    m_surfWidth = rect.right;
    rect.bottom = m_surface->m_height; // +0x18
    m_surfHeight = rect.bottom;
    rect.left = 0;
    rect.top = 0;
    POINT pt;
    pt.x = m_centerX;
    pt.y = m_centerY;
    if (::PtInRect(&rect, pt) == 0) {
        return 0;
    }
    if (m_lightGate != 0) {
        i32 i = 0;
        if (m_surfHeight > 0) {
            do {
                m_spanStarts[i] = 0;
                m_spanEnds[i] = m_surfWidth;
                i++;
            } while (i < m_surfHeight);
        }
    } else {
        i32 i = 0;
        if (m_surfHeight > 0) {
            do {
                m_spanStarts[i] = m_centerX;
                m_spanEnds[i] = m_centerX;
                i++;
            } while (i < m_surfHeight);
        }
    }
    if (m_spanCount > 0) {
        if (d->m_20 == 0) {
            m_table = m_cache.HueRampTable(reinterpret_cast<PalEntry*>(m_palette->m_cacheA), m_spanCount, 0);
            m_flag = 1;
            return 1;
        }
        m_table = reinterpret_cast<CShadeTable*>(d->m_20);
    }
    return 1;
}

RVA(0x00180630, 0x1)
void CFaderLight::SubFree180630() {}

// @early-stop
// CFaderLight::RenderFrame (0x180640, vtable slot 1, 2412 B): the light fader's per-frame
// light/circle-shade blit. Identity recovered by data-ref: 0x180640 is ??_7CFaderLight
// @@6B@+0x4 (vtable slot 1), the RenderFrame(i32 frame) the class already declares. Defined here
// as the real virtual (was the mis-homed Gap_180640 free-fn stub) so ??_7CFaderLight
// slot 1 binds to its own body; body parked (>512 B leaf-first reconstruction).
RVA(0x00180640, 0x96c)
void CFaderLight::RenderFrame(i32 frame) {}

// CFaderLight::GetFrameCount (0x1814f0, vtable slot 2) - the fade frame count = the maximum
// distance from the light centre to any active-surface corner (each squaring is
// pow(x, 2.0), the largest hypotenuse __ftol'd into m_5c).
// @early-stop
// x87-fp-stack-schedule wall (docs/patterns/x87-fp-stack-schedule.md): the four pow/sqrt
// corner distances + the running-max are byte-faithful in operation/operand, but retail
// interleaves the pow calls with a dense fxch/fld juggle + open-codes the max as an fcomp
// tree; cl serialises them + lowers the max as fcom/branch pairs. Not source-steerable.
RVA(0x001814f0, 0x16d)
i32 CFaderLight::GetFrameCount() {
    i32 cx = m_centerX;
    i32 cy = m_centerY;
    i32 w = m_surface->m_width;
    i32 h = m_surface->m_height;

    double pA = pow(static_cast<double>(cx), g_faderPowK);
    double pB = pow(static_cast<double>(cy), g_faderPowK);
    double pH = pow(static_cast<double>((h - cy)), g_faderPowK);
    double pW = pow(static_cast<double>((w - cx)), g_faderPowK);

    double d0 = sqrt(pA + pB);
    double d1 = sqrt(pW + pB);
    double d2 = sqrt(pA + pH);
    double d3 = sqrt(pW + pH);

    double m = d0;
    if (d1 > m) {
        m = d1;
    }
    if (d2 > m) {
        m = d2;
    }
    if (d3 > m) {
        m = d3;
    }
    i32 r = static_cast<i32>(m);
    m_frameCount = r;
    return r;
}

RVA(0x00181660, 0x40)
void CFaderLight::BeginFade() {
    if (m_spanCount > 0 && m_lightGate != 0) {
        CDDrawPtrCollections* pool = reinterpret_cast<CDDrawPtrCollections*>(m_set2cArg); // +0x2c dual-role pool slot
        CDDSurface* h = pool->MakeAndAddB(m_surfWidth, m_surfHeight, 0, 0, -1);
        m_overlay = h;
        h->Blt(m_surface);
    }
}

RVA(0x001816a0, 0x1c)
void CFaderLight::EndFade() {
    if (m_overlay) {
        CDDrawPtrCollections* pool = reinterpret_cast<CDDrawPtrCollections*>(m_set2cArg);
        pool->RemoveItemA(m_overlay);
        m_overlay = 0;
    }
}

RVA(0x0017f9a0, 0x24)
CFaderRadial::CFaderRadial() {
    m_maxRadius = 0;
    m_40 = 0;
    m_cells = 0;
    m_48 = 1;
}

RVA(0x0017f9f0, 0x4f)
CFaderRadial::~CFaderRadial() {
    FreeBuffer17fc40();
}

RVA(0x0017fc40, 0x11)
void CFaderRadial::FreeBuffer17fc40() {
    if (m_cells) {
        ::operator delete(m_cells);
    }
}

RVA(0x001816c0, 0x32)
CFaderShape::CFaderShape() {
    m_warpTable = 0;
    m_rowOfsA = 0;
    m_rowOfsB = 0;
    m_rowOfsC = 0;
    m_lineBuf = 0;
    m_shadeRamp = 0;
    m_20 = 0;
}

// ===========================================================================
// 0x17e540 - CFader::RunFadeStepped(step, lead, notify): the stepped counterpart
// of RunFade. Primes frame 0, busy-waits the lead-in, then renders every `step`-th
// frame from 1..count back-to-back (no elapsed/duration mapping), poking the
// m_set2cArg fade sink + RenderFrame(frame) each step. Finalizes RenderFrame(count)/EndFade() and records
// the achieved frame rate in m_34. NON-EH (base /O2) frame.
// ===========================================================================
// @early-stop
// Complete + correct (~87%). Same x87-schedule + regalloc wall as the sibling
// RunFade (0x17e620): retail pins `count` in ebx and `loops` in ebp, cl swaps them
// (count->ebp, loops->ebx) - a callee-saved recolor changing every ModRM through the
// body; and retail spills `(float)loops` to memory BEFORE the GetTickCount call
// (fild/fstp [esp+0x20], then fdivrs memory), while cl schedules the fild after the
// call and keeps it on the x87 stack (fdivrp register). The loop, the sink COM-call,
// all guards, and the m_34 frame-rate store are byte-exact; the reg-swap + fp-spill
// schedule are not source-steerable (docs/patterns/x87-fp-stack-schedule.md).
RVA(0x0017e540, 0xd8)
void CFader::RunFadeStepped(i32 step, i32 lead, i32 notify) {
    i32 count = GetFrameCount();
    if (count < 1) {
        return;
    }
    BeginFade();
    RenderFrame(0);
    Wait(lead);
    DWORD startTick = GetTickCount();
    i32 loops = 0;
    i32 frame = 1;
    if (count >= 1) {
        do {
            if (notify && m_set2cArg) {
                IFadeSink* o = *reinterpret_cast<IFadeSink**>(m_set2cArg);
                o->FadeNotify(1, 0);
            }
            RenderFrame(frame);
            loops++;
            frame += step;
        } while (frame <= count);
    }
    if (frame != count) {
        RenderFrame(count);
        loops++;
    }
    float fLoops = static_cast<float>(loops);
    DWORD elapsed = GetTickCount() - startTick;
    m_34 = static_cast<i32>((fLoops / (static_cast<float>(elapsed) * 0.001f)));
    EndFade();
}

// @early-stop
// Complete + correct (~86%). Wall = the x87 loop-invariant conversion block: retail
// batches (float)startTick + (float)dur + (float)count as fild QWORD x2 + fild DWORD
// then a single `fxch st(2)` + 3 fstp, and pins fStart@0x2c / fDur@0x10 / fCount@0x14;
// MSVC5 here schedules the three (unsigned/int)->float conversions separately and
// assigns different stack slots, which cascades the fsub/fdiv/fmul memory operands
// (same FP-schedule/local-slot class as the blit reassociation walls). No source
// ordering of the three float decls pins the fxch batching. Loop body, guards, sink
// COM-call and the m_34 frame-rate store (0.001f const) are byte-exact.
RVA(0x0017e620, 0x13b)
void CFader::RunFade(u32 dur, i32 lead, i32 notify) {
    i32 prev = 0;
    i32 frame = 0;
    i32 count = GetFrameCount();
    if (count < 1) {
        return;
    }
    BeginFade();
    RenderFrame(0);
    Wait(lead);
    i32 loops = 0;
    DWORD startTick = GetTickCount();
    float fStart = static_cast<float>(startTick);
    float fDur = static_cast<float>(dur);
    float fCount = static_cast<float>(count);
    if (count >= 0) {
        do {
            frame = static_cast<i32>(((static_cast<float>(GetTickCount()) - fStart) / fDur * fCount));
            if (prev != frame && frame <= count && frame > 0) {
                if (notify && m_set2cArg) {
                    IFadeSink* o = *reinterpret_cast<IFadeSink**>(m_set2cArg);
                    o->FadeNotify(1, 0);
                }
                RenderFrame(frame);
                loops++;
            }
            prev = frame;
        } while (frame <= count);
    }
    if (frame != count) {
        RenderFrame(count);
        loops++;
    }
    float fLoops = static_cast<float>(loops);
    DWORD elapsed = GetTickCount() - startTick;
    m_34 = static_cast<i32>((fLoops / (static_cast<float>(elapsed) * 0.001f)));
    EndFade();
}

// ===========================================================================
// 0x17ea00 - CFaderMesh::ApplyInit(desc): build the (m_54 x m_50) ellipse mesh
// into this->m_58 from the transition descriptor. Returns 0 on a null gate, else 1.
// ===========================================================================
// @early-stop
// regalloc this-pin + x87-schedule wall (proven with llvm-objdump -dr base vs
// target): the descriptor latch is instruction-for-instruction identical
// (same loads [eax+N], same stores [this+N], same tests/branches, same offsets),
// but retail pins `this` in EBP while cl colours it ESI - a callee-saved recolor
// that changes the ModRM encoding of every this-relative access through the whole
// body (this is the docs/patterns/zero-register-pinning.md family), and the frame
// is 0x9c vs cl's 0x90 (retail reserves 12 B more x87 stack-temps). On top of that
// the radius + per-cell projection is a dense x87 expression whose fild/fsqrt/fdiv/
// fmul + __ftol ordering is not source-steerable (docs/patterns/x87-fp-stack-
// schedule.md, ~60-75% band), and the open-coded MFC buffer grow (RezAlloc/RezFree
// + rep-movs) diverges likewise. Grid topology, the sqrt distance projection and the
// field assembly are all correct; the this-recolor + FP schedule park it (~68.4%,
// up from ~65.6% pre-de-view). Final-sweep candidate. The de-view (CFxModeT3::Build
// -> CFaderMesh::ApplyInit) is byte-neutral/positive: only the mangled symbol name
// changes (paired by RVA), not the algorithm.
RVA(0x0017ea00, 0x4fc)
i32 CFaderMesh::ApplyInit(CFxModeDesc* descOpaque) {
    // The arg is the type-6 CFxMode transition descriptor; m_58 is this fader's real
    // growable mesh buffer (CRezBufferObject).
    CFxModeT6* cfg = static_cast<CFxModeT6*>(descOpaque);
    CRezBufferObject* mesh = &m_meshBuf;

    m_dstSurface = cfg->m_04 ? reinterpret_cast<CDDSurface*>(cfg->m_04) : reinterpret_cast<CDDSurface*>(m_timerA);
    m_bltSrc = cfg->m_08 ? reinterpret_cast<CDDSurface*>(cfg->m_08) : reinterpret_cast<CDDSurface*>(m_timerB);
    if (cfg->m_10 == 0) {
        return 0;
    }
    m_primeSrc = reinterpret_cast<CDDSurface*>(cfg->m_0c);
    m_flipTarget = reinterpret_cast<CDDSurface*>(cfg->m_10);
    m_48 = cfg->m_18;
    m_recOrderFlag = cfg->m_14;
    m_cols = cfg->m_1c;
    m_rows = cfg->m_20;

    mesh->SetSize(0, -1);

    // The two "boxes" are surfaces: the geometry comes out of their DDSURFACEDESC cache
    // (+0x18 dwHeight, +0x1c dwWidth). Offsets preserved from the old FxBox view, whose
    // width/height names were swapped relative to the canonical descriptor.
    i32 halfW = m_dstSurface->m_height / 2;
    i32 halfH = m_dstSurface->m_width / 2;
    i32 dx = m_bltSrc->m_width / m_cols;
    i32 dy = m_bltSrc->m_height / m_rows;
    float radius = static_cast<float>(sqrt(static_cast<double>((dx * dx + dy * dy))));
    if (m_rows <= 0) {
        return 1;
    }

    for (i32 row = 0; row < m_rows; row++) {
        i32 cellW2 = halfW * halfW;
        i32 cellD = halfW * halfW + halfH * halfH;
        float cellR = static_cast<float>(sqrt(static_cast<double>(cellD))) + radius - g_fxBias;
        if (m_cols <= 0) {
            continue;
        }
        for (i32 col = 0; col < m_cols; col++) {
            i32 d2 = halfH * halfH + cellW2;
            float v = static_cast<float>(sqrt(static_cast<double>(d2)));
            float normX, normY;
            if (v > g_fxEps) {
                normY = static_cast<float>((row - halfH)) / v;
                normX = static_cast<float>((col - halfW)) / v;
            } else {
                normY = 0.0f;
                normX = 1.0f;
            }

            RECT pt48;
            pt48.left = 0;
            pt48.top = 0;
            pt48.right = dx;
            pt48.bottom = dy;
            OffsetRect(&pt48, row, col);
            i32 ox = static_cast<i32>((cellR * normX));
            i32 oy = static_cast<i32>((cellR * normY));
            OffsetRect(&pt48, oy, ox);

            RECT pt64;
            pt64.left = 0;
            pt64.top = 0;
            pt64.right = d2;
            pt64.bottom = dy;
            OffsetRect(&pt64, row, col);

            i32 pt[10]; // the 40-byte mesh record (a RezElem40 slot)
            if (m_recOrderFlag) {
                pt[0] = pt64.right;
                pt[1] = pt64.bottom;
                pt[2] = pt64.left;
                pt[3] = pt64.top;
                pt[4] = pt48.left;
                pt[5] = pt48.top;
                pt[6] = pt48.right;
                pt[7] = pt48.bottom;
            } else {
                pt[0] = pt48.left;
                pt[1] = pt48.top;
                pt[2] = pt48.right;
                pt[3] = pt48.bottom;
                pt[4] = pt64.right;
                pt[5] = pt64.bottom;
                pt[6] = pt64.left;
                pt[7] = pt64.top;
            }
            pt[8] = 0;
            pt[9] = 0x3f800000;

            // Inlined MFC CArray::SetAtGrow(GetSize(), &pt).
            i32 idx = mesh->m_nSize;
            i32 newSize = idx + 1;
            if (newSize == 0) {
                if (mesh->m_pData) {
                    RezFree(mesh->m_pData);
                    mesh->m_pData = 0;
                }
                mesh->m_nMaxSize = 0;
                mesh->m_nSize = 0;
            } else if (mesh->m_pData == 0) {
                mesh->m_pData = static_cast<RezElem40*>(RezAlloc(newSize * sizeof(RezElem40)));
                memset(mesh->m_pData, 0, newSize * sizeof(RezElem40));
                mesh->m_nMaxSize = newSize;
                mesh->m_nSize = newSize;
            } else if (newSize <= mesh->m_nMaxSize) {
                if (newSize > idx) {
                    memset(&mesh->m_pData[idx], 0, (newSize - idx) * sizeof(RezElem40));
                }
                mesh->m_nSize = newSize;
            } else {
                i32 grow = mesh->m_nGrowBy;
                if (grow == 0) {
                    grow = idx / 8;
                    if (grow < 4) {
                        grow = 4;
                    } else if (grow > 0x400) {
                        grow = 0x400;
                    }
                }
                i32 newMax = mesh->m_nMaxSize + grow;
                if (newSize > newMax) {
                    newMax = newSize;
                }
                RezElem40* nd = static_cast<RezElem40*>(RezAlloc(newMax * sizeof(RezElem40)));
                memcpy(nd, mesh->m_pData, mesh->m_nSize * sizeof(RezElem40));
                memset(&nd[mesh->m_nSize], 0, (newSize - mesh->m_nSize) * sizeof(RezElem40));
                RezFree(mesh->m_pData);
                mesh->m_pData = nd;
                mesh->m_nSize = newSize;
                mesh->m_nMaxSize = newMax;
            }
            memcpy(&mesh->m_pData[idx], pt, sizeof(RezElem40));
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CRezBufferObject::SetSize (0x17f390) - the out-of-line MFC CArray<RezElem40>::SetSize
// (CFaderMesh::ApplyInit @0x17ea00 calls it at 0x17ea79 on the buffer embedded at
// CFaderMesh+0x58). sema xref: that is its ONLY caller, and +0x58 IS a CRezBufferObject,
// so the method belongs to that class - it was never a free-standing "CArrayE40". The
// element is POD here (ConstructElements = zero-fill), so grow/shrink inline memset/memcpy
// around the engine allocator (RezAlloc 0x1b9b46 / RezFree 0x1b9b82).
//
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md, topic:wall
// topic:regalloc): ~96%, every operation/offset/immediate/branch + the store
// ordering (m_nSize=m_nMaxSize=0) is byte-faithful; the sole residual is the long-
// lived "0"/null register choice - retail pins it in esi (then reloads esi as the
// memcpy src), cl pins it in edi, which cascades into the edx/ecx scratch picks in
// the realloc lea chains. Not source-steerable. Deferred to the final sweep.
RVA(0x0017f390, 0x164)
void CRezBufferObject::SetSize(i32 nNewSize, i32 nGrowBy) {
    if (nGrowBy != -1) {
        m_nGrowBy = nGrowBy;
    }
    if (nNewSize == 0) {
        if (m_pData != 0) {
            RezFree(m_pData);
            m_pData = 0;
        }
        m_nSize = m_nMaxSize = 0;
    } else if (m_pData == 0) {
        m_pData = static_cast<RezElem40*>(RezAlloc(nNewSize * sizeof(RezElem40)));
        memset(m_pData, 0, nNewSize * sizeof(RezElem40));
        m_nSize = m_nMaxSize = nNewSize;
    } else if (nNewSize <= m_nMaxSize) {
        if (nNewSize > m_nSize) {
            memset(&m_pData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(RezElem40));
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
        RezElem40* pNewData = static_cast<RezElem40*>(RezAlloc(nNewMax * sizeof(RezElem40)));
        memcpy(pNewData, m_pData, m_nSize * sizeof(RezElem40));
        memset(&pNewData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(RezElem40));
        RezFree(m_pData);
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }
}

// @early-stop
// regalloc/scheduling tie (~90%): logic byte-exact; retail's ecx/edx assignment
// for the m_src reload + s->m_10 store schedule differs from this cl's.
RVA(0x0017f5e0, 0x7d)
i32 CFaderFlat::ApplyInit(CFxModeDesc* desc) {
    CFxModeT5* s = static_cast<CFxModeT5*>(desc);
    i32 a = s->m_04;
    if (!a) {
        a = m_timerA; // the base's default dword
    }
    m_desc04 = a;
    if (s->m_08) {
        m_src = reinterpret_cast<FaderSrc*>(s->m_08);
    } else {
        m_src = reinterpret_cast<FaderSrc*>(m_timerB);
    }
    m_desc0c = s->m_0c;
    m_percent = s->m_10;
    m_20 = 0;
    m_desc14 = s->m_14;
    m_frames = static_cast<i32*>(RezAlloc(m_src->m_frameCount << 2));
    for (i32 i = 0; i < m_src->m_frameCount; i++) {
        m_frames[i] = 0;
    }
    return 1;
}

RVA(0x0017f950, 0x24)
i32 CFaderFlat::GetFrameCount() {
    i32 n = m_src->m_frameCount;
    return n + (m_percent * n) / 100;
}

extern "C" int __ftol(double v); // 0x11f570 (CRT double->long, x87 fstcw/fldcw)

DATA(0x001f0828)
extern const float g_faderHalf = 0.5f; // 0.5
DATA(0x001f0830)
extern const double g_faderScale = 10000.0; // 10000.0
DATA(0x001f0838)
extern const double g_faderBiasR = -1.0; // -1.0  (r - K == r + 1.0)
DATA(0x001f0840)
extern const float g_faderBiasFade = -1.0f; // -1.0  (fade - K == fade + 1.0)
DATA(0x001f0844)
extern const float g_faderOne = 1.0f; // 1.0  (per-cell render threshold: fade - frame > 1.0)

// @early-stop
// Re-reconstructed 61.64%->73.70% by fixing three structural bugs the prior model
// carried (it was NOT an x87 wall): (1) m_maxRadius was `ftol(cx^2+cy^2)` - the real
// formula is `ftol(sqrt(cx^2+cy^2) * 10000)`; (2) the per-cell dy was `0-centerY`
// (the y term dropped) - it is `y - centerY`; (3) the pixel read used m_pixels(=the
// m_8 COM surface) as the byte base and a phantom GetRowBase(row) - the real code
// Lock()s the surface (0x13e6d0 returns the locked lpSurface base) then reads
// `base[colStride*x + pitch*y]` and UnlockThunk()s. The K constants (0.5/10000/-1.0/
// -1.0) are the reloc-masked .rdata doubles/floats. Residual is a callee-saved regalloc
// coloring (retail pins this->esi/zero->ebp; our cl picks edi/ebx) + the x87 temp-slot
// frame (sub esp,0x18 vs 0xc) - genuine MSVC5 scheduling, not logic.
// (The cells store FLOAT vx/vy/fade - RenderFrame @0x17fc60 reads them with fld/fcomp - so the
// de-view also drops the three (i32) truncations the old FrCell forced.)
RVA(0x0017fa40, 0x1f3)
i32 CFaderRadial::ApplyInit(CFxModeDesc* desc) {
    CFxModeT4* cfg = static_cast<CFxModeT4*>(desc);
    if (cfg->m_04 == 0) {
        m_dstSurface = reinterpret_cast<CDDSurface*>(m_timerA);
    } else {
        m_dstSurface = reinterpret_cast<CDDSurface*>(cfg->m_04);
    }

    if (cfg->m_08 == 0) {
        m_srcSurface = reinterpret_cast<CDDSurface*>(m_timerB);
    } else {
        m_srcSurface = reinterpret_cast<CDDSurface*>(cfg->m_08);
    }

    if (cfg->m_14 == 0) {
        // build the fade shade table from the descriptor's palette (m_cache is the
        // CFader base's embedded CShadeTableCache at this+0x04)
        CDDPalette* pal = reinterpret_cast<CDDPalette*>(cfg->m_10);
        m_table = m_cache.HueRampTable(reinterpret_cast<PalEntry*>(pal->m_cacheA), 0x10, 0);
        m_flag = 1; // we own it: ~CFader will FindRemove it
    } else {
        m_table = reinterpret_cast<CShadeTable*>(cfg->m_14);
        m_flag = 0;
    }
    if (m_table == 0) {
        return 0;
    }

    // The source surface is the real CDDSurface: width @+0x1c, height @+0x18, pitch
    // @+0x20, column stride @+0xb0, held IDirectDrawSurface @m_8. Lock() (0x13e6d0)
    // returns the locked pixel base; UnlockThunk() is the m_8->vtbl[0x80] COM unlock.
    CDDSurface* s = m_srcSurface;
    m_fadeDivisor = static_cast<float>(s->m_width) * g_faderHalf; // width * 0.5
    m_centerX = s->m_width / 2;
    m_centerY = s->m_height / 2;
    m_cells = static_cast<CFaderRadialCell*>(::operator new(s->m_height * s->m_width * 16));

    i32 cx = m_centerX;
    i32 cy = m_centerY;
    m_maxRadius = static_cast<i32>((sqrt(static_cast<double>((cx * cx + cy * cy))) * g_faderScale));

    for (i32 y = 0; y < m_srcSurface->m_height; y++) {
        for (i32 x = 0; x < m_srcSurface->m_width; x++) {
            i32 dx = x - m_centerX;
            i32 dy = y - m_centerY;
            float r = static_cast<float>((static_cast<double>(m_maxRadius) - sqrt(static_cast<double>((dx * dx + dy * dy))) * g_faderScale
                              - g_faderBiasR));
            float fade = r / m_fadeDivisor - g_faderBiasFade;
            float vx = static_cast<float>(dx) * fade;
            float vy = static_cast<float>(dy) * fade;
            u8 pix;
            i32 base = m_srcSurface->Lock(0);
            if (base != 0) {
                pix = *reinterpret_cast<u8*>((base + m_srcSurface->m_bytesPerPixel * x + m_srcSurface->m_pitch * y));
                m_srcSurface->UnlockThunk();
            } else {
                pix = 0;
            }
            CFaderRadialCell* cell = &m_cells[y * m_srcSurface->m_width + x];
            cell->m_vx = vx;
            cell->m_vy = vy;
            cell->m_fade = fade;
            cell->m_pixel = pix;
        }
    }
    return 1;
}

// ===========================================================================
// 0x17fc60 - CFaderRadial::RenderFrame(frame) (the vtable slot-1 render, hosted on the
// CFaderRadialApply flat view): plot the precomputed radial-fade cells whose fade
// threshold still exceeds `frame` into the m_3c dest surface. Alloc a per-width Rez
// scratch (retail allocates it but leaves it unused, freed at the end), Clear + Lock
// the dest, Lock the source (its base unused; the cells were precomputed by Build),
// then per cell: if (fade - frame) > 1.0, displace the cell (centerX + vx/scaledFade,
// centerY - vy/scaledFade) and, if in bounds, write the cell pixel into the locked
// dest at [py*pitch + px]. Unlock both COM surfaces (inlined m_8->vtbl[0x80]).
// ===========================================================================
// @early-stop
// x87-fp-stack-schedule wall (docs/patterns/x87-fp-stack-schedule.md; same family as
// the sibling ApplyInit @0x17fa40): the per-cell (fade-frame)/divisor + vx/sf, vy/sf
// __ftol chain, the fcoms threshold and the bounds test + plot are byte-faithful, but
// cl schedules the frame-float on the x87 stack and colours edi/ebx/ebp differently
// than retail (which pins this->esi/cellptr->ebx/index->ebp). Not source-steerable.
RVA(0x0017fc60, 0x136)
void CFaderRadial::RenderFrame(i32 frame) {
    CDDSurface* dst = m_dstSurface;         // +0x3c
    void* scratch = RezAlloc(dst->m_width); // per-width scratch (alloc'd, unused)
    dst->Clear(0);
    m_srcSurface->Lock(0);      // lock source (base unused here)
    i32 base = dst->Lock(0);    // locked dest pixel base
    if (m_table->m_data == 0) { // gate: is the shade table's buffer present?
        return;                 // retail bails w/o unlock/free (matched)
    }

    i32 total = m_srcSurface->m_width * m_srcSurface->m_height;
    float ff = static_cast<float>(frame);
    for (i32 i = 0; i < total; i++) {
        CFaderRadialCell* c = &m_cells[i];
        float d = c->m_fade - ff;
        if (d > g_faderOne) {
            float sf = d / m_fadeDivisor - g_faderBiasFade;
            i32 px = m_centerX + static_cast<i32>((c->m_vx / sf));
            i32 py = m_centerY - static_cast<i32>((c->m_vy / sf));
            if (px > 0 && px < dst->m_width && py > 0 && py < dst->m_height) {
                (reinterpret_cast<u8*>(base))[py * dst->m_pitch + px] = static_cast<u8>(c->m_pixel);
            }
        }
    }

    // Inlined UnlockThunk: IDirectDrawSurface::Unlock(NULL) on both surfaces (COM slot
    // 32, byte +0x80 - the ex void**-element "+0x20" spelling of the same slot).
    m_srcSurface->m_ddSurface->Unlock(0);
    dst->m_ddSurface->Unlock(0);
    RezFree(scratch);
}

void __cdecl operator delete(void* p); // ??3@YAXPAX@Z (0x1b9b82)

RVA(0x00181720, 0xb3)
CFaderShape::~CFaderShape() {
    if (m_warpTable) {
        operator delete(m_warpTable);
    }
    if (m_rowOfsA) {
        operator delete(m_rowOfsA);
    }
    if (m_rowOfsB) {
        operator delete(m_rowOfsB);
    }
    if (m_rowOfsC) {
        operator delete(m_rowOfsC);
    }
    if (m_lineBuf) {
        operator delete(m_lineBuf);
    }
    if (m_shadeRamp) {
        operator delete(m_shadeRamp);
    }
}

extern "C" int _access(const char* path, int mode); // 0x193900 CRT

// @early-stop
// x87 scheduling wall (~40-50%): the surface resolution, the equal-dimension
// validation, the per-row offset tables (m_rowOfsA/B/C) and the m_lineBuf scratch
// allocation match, but the two transcendental ramp loops are not source-steerable
// - retail's fild/fxch/fstp stack-slot reuse over `acos((i-r)/r)*r` (m_warpTable) and
// `0x10 - sin(i/r*PI)*-32` (m_shadeRamp), plus the MFC CString temp the file-name
// AddFromArray path builds, diverge. Same family as CFaderRadial::ApplyInit (0x17fa40).
RVA(0x001817e0, 0x315)
i32 CFaderShape::ApplyInit(CFxModeDesc* desc) {
    CFxModeT1* pInit = static_cast<CFxModeT1*>(desc);
    i32 i;
    m_20 = 0;
    if (pInit == 0) {
        return 0;
    }

    m_surfA = pInit->m_04 ? reinterpret_cast<CDDSurface*>(pInit->m_04) : reinterpret_cast<CDDSurface*>(m_timerA);
    m_surfB = pInit->m_08 ? reinterpret_cast<CDDSurface*>(pInit->m_08) : reinterpret_cast<CDDSurface*>(m_timerB);
    if (m_surfA == 0) {
        return 0;
    }
    if (m_surfB == 0) {
        return 0;
    }
    m_surfC = pInit->m_0c ? reinterpret_cast<CDDSurface*>(pInit->m_0c) : m_surfB;

    if (!m_cache.Init()) {
        return 0;
    }

    // +0x60/+0x64 (etc) take the surface's +0x1c (dwWidth) then +0x18 (dwHeight).
    m_span = m_surfA->m_width;
    m_rowCount = m_surfA->m_height;
    m_spanB = m_surfB->m_width;
    m_rowCountB = m_surfB->m_height;
    m_spanC = m_surfC->m_width;
    m_rowCountC = m_surfC->m_height;
    if (m_span != m_spanB) {
        return 0;
    }
    if (m_rowCount != m_rowCountB) {
        return 0;
    }
    if (m_span != m_spanC) {
        return 0;
    }
    if (m_rowCount != m_rowCountC) {
        return 0;
    }
    if (m_spanC != m_spanB) {
        return 0;
    }
    if (m_rowCountC != m_rowCountB) {
        return 0;
    }

    if (pInit->m_14 == 0) {
        return 0;
    }
    if (static_cast<u32>(pInit->m_14) >= 4) {
        return 0;
    }
    m_mode = pInit->m_14;
    m_stripCopy = pInit->m_18;
    m_halfWidth = pInit->m_10;

    if (m_mode == 1 || m_mode == 2) {
        if (m_span < static_cast<i32>((static_cast<double>(m_halfWidth) * 3.141592653589793))) {
            return 0;
        }
    }

    m_warpTable = static_cast<i32*>(RezAlloc(m_halfWidth * 8));
    for (i = 0; i < 2 * m_halfWidth; i++) {
        m_warpTable[i] =
            static_cast<i32>((acos((static_cast<float>(i) - static_cast<float>(m_halfWidth)) / static_cast<float>(m_halfWidth)) * static_cast<float>(m_halfWidth)));
    }

    m_useLut = pInit->m_1c;
    if (m_surfA->m_bitDepth != 8) {
        m_useLut = 0;
    }

    if (m_useLut != 0) {
        if (pInit->m_20) {
            m_flag = 0; // a caller-supplied table: ~CFader must NOT FindRemove it
            m_table = reinterpret_cast<CShadeTable*>(pInit->m_20);
        } else if (_access(pInit->m_24, 0) == 0) {
            m_table = m_cache.AddFromArray(pInit->m_24);
            if (m_table == 0) {
                m_useLut = 0;
            }
        } else {
            CDDPalette* pal = reinterpret_cast<CDDPalette*>(pInit->m_28);
            m_table = m_cache.FlashTable(reinterpret_cast<PalEntry*>(pal->m_cacheA), 0x20, 0x20, 0x32, 0xc8);
        }

        i32 m = m_halfWidth << 1;
        m_shadeRamp = static_cast<u8*>(RezAlloc(m));
        for (i = 0; i < m; i++) {
            i32 t = static_cast<i32>((sin(static_cast<float>(i) / static_cast<float>(m_halfWidth) * 3.14f) * -32.0));
            m_shadeRamp[i] = static_cast<u8>((0x10 - t));
        }
    }

    m_rowOfsA = static_cast<i32*>(RezAlloc(m_rowCount * 4));
    m_rowOfsB = static_cast<i32*>(RezAlloc(m_rowCountB * 4));
    m_rowOfsC = static_cast<i32*>(RezAlloc(m_rowCountC * 4));
    for (i = 0; i < m_rowCount; i++) {
        m_rowOfsA[i] = m_surfA->m_pitch * i;
        m_rowOfsB[i] = m_surfB->m_pitch * i;
        m_rowOfsC[i] = m_surfC->m_pitch * i;
    }

    i32 mx = (m_rowCount > m_span) ? m_rowCount : m_span;
    m_lineBuf = static_cast<u8*>(RezAlloc(m_surfA->m_bytesPerPixel * mx));
    return 1;
}

// ===========================================================================
// 0x182610 - RenderTile: assemble + write back one (2*m_halfWidth)-wide line per row.
// ===========================================================================
// @early-stop
// Regalloc / loop-scheduling wall: this is a deep nested gather with ~16 live
// member bases, four per-bpp inner variants and two byte-copy tails. The logic
// (column loop, the m_useLut LUT gather vs the 1/2/3-byte straight copies, the m_stripCopy
// copy/zero strip, the scratch write-back) is reconstructed faithfully, but MSVC5
// reloads each member base per use and threads the dual src/dest pointers through
// a spill schedule that this C spelling does not reproduce instruction-for-
// instruction. Inner addressing, the *bpp scaling and the LUT keying are correct;
// the spill/reload schedule parks it. Final-sweep candidate.
RVA(0x00182610, 0x2eb)
void CFaderShape::RenderTile(i32 arg0, i32 arg1) {
    if (arg1 <= 0) {
        return;
    }
    i32 stride = m_halfWidth * 2; // inner pixel count
    i32 rowBytes = stride + arg1;
    i32 bpp = m_surfA->m_bytesPerPixel;

    i32 x0;
    u8* src2base;
    u8* destBase;
    if (m_mode == 1) {
        src2base = m_lineBuf;
        x0 = arg1;
        destBase = m_straightBase + (arg0 - arg1) * bpp;
    } else if (m_mode == 2) {
        src2base = m_lineBuf + bpp * stride;
        x0 = 0;
        destBase = m_straightBase + (arg0 + stride) * bpp;
    } else {
        return;
    }

    u8* srcA = m_dstBase + (arg0 - x0) * bpp;
    u8* srcB = m_gatherBase + (arg0 - x0) * bpp;
    if (m_rowCount <= 0) {
        return;
    }

    for (i32 j = 0; j < m_rowCount; j++) {
        u8* rowSrcA = srcA + m_rowOfsA[j];
        u8* rowSrcB = srcB + m_rowOfsC[j];

        if (m_useLut) {
            u8* lut = m_table->m_data;
            for (i32 k = 0; k < stride; k++) {
                u8 b = rowSrcB[m_warpTable[k]];
                m_lineBuf[x0 + k] = lut[(b << 6) + m_shadeRamp[k]];
            }
        } else if (bpp == 1) {
            for (i32 k = 0; k < stride; k++) {
                m_lineBuf[x0 + k] = rowSrcB[m_warpTable[k]];
            }
        } else if (bpp == 2) {
            for (i32 k = 0; k < stride; k++) {
                u8* s = rowSrcB + m_warpTable[k] * 2;
                u8* d = m_lineBuf + (x0 + k) * 2;
                d[0] = s[0];
                d[1] = s[1];
            }
        } else if (bpp == 3) {
            for (i32 k = 0; k < stride; k++) {
                u8* s = rowSrcB + m_warpTable[k] * 3;
                u8* d = m_lineBuf + (x0 + k) * 3;
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
            }
        }

        if (m_stripCopy) {
            u8* s = destBase + m_rowOfsB[j];
            u8* d = src2base;
            for (i32 n = bpp * arg1; n > 0; n--) {
                *d++ = *s++;
            }
        } else {
            memset(src2base, 0, bpp * arg1);
        }

        u8* s = m_lineBuf;
        u8* d = rowSrcA;
        for (i32 n = bpp * rowBytes; n > 0; n--) {
            *d++ = *s++;
        }
    }
}

// ===========================================================================
// 0x181e50 - RenderWarpTile: the PI-scaled counterpart of RenderTile. Computes a
// per-tile column split point from a circular (m_halfWidth * PI) arc scaling, then for
// each of m_rowCount columns gathers a (2*m_halfWidth) line (straight bytes + the m_warpTable-tapped
// remainder, or the m_useLut LUT path) and writes it back, with the m_stripCopy copy/zero
// dest strip. param_2 = base pixel row, param_3 = leading width.
// ===========================================================================
// @early-stop
// Dual wall: (1) the same deep-loop / many-live-base regalloc schedule that parks
// the sibling RenderTile, and (2) the x87 arc/scale block (fild/fmul PI/fidiv/fimul
// /__ftol) whose fp-stack scheduling cl reorders. Logic (the two condition-gated
// scroll halves, the per-bpp 1/2/3 gather, the LUT remap, the copy/zero strip and
// the scratch write-back) is reconstructed faithfully; FP scheduling + spill order
// park it. Final-sweep candidate.
RVA(0x00181e50, 0x7b9)
void CFaderShape::RenderWarpTile(i32 arg0, i32 arg1) {
    i32 stride = m_halfWidth * 2;
    if (arg1 <= 0) {
        return;
    }
    i32 arc = static_cast<i32>((static_cast<double>(m_halfWidth) * 3.14159));
    i32 bpp = m_surfA->m_bytesPerPixel;

    i32 colBase;
    if ((m_mode == 1 && m_stripCopy != 0) || (m_mode == 2 && m_stripCopy == 0)) {
        colBase = stride - static_cast<i32>((static_cast<double>(stride) / (arc - m_halfWidth) * (m_span - arg0 - stride)));
    } else {
        colBase = arg0;
    }
    if ((m_mode == 1 && m_stripCopy == 0) || (m_mode == 2 && m_stripCopy != 0)) {
        colBase = static_cast<i32>((static_cast<double>(stride) / (arc - m_halfWidth) * arg0));
    }

    if ((m_mode == 1 && m_stripCopy != 0) || (m_mode == 2 && m_stripCopy == 0)) {
        i32 col = 0;
        if (m_rowCount > 0) {
            i32 base = bpp * arg0;
            do {
                u8* dstLine = m_rowOfsA[col] + base + m_dstBase;
                u8* gsrc = m_rowOfsC[col] + base + m_gatherBase;
                u8* ssrc = m_rowOfsB[col] + base + m_straightBase;
                if (m_useLut == 0) {
                    if (bpp == 1) {
                        i32 i = 0;
                        i32 t = colBase;
                        if (colBase > 0) {
                            do {
                                m_lineBuf[i] = ssrc[i];
                                i++;
                            } while (i < colBase);
                        }
                        for (; t < stride; t++) {
                            m_lineBuf[t] = gsrc[m_warpTable[t]];
                        }
                    } else if (bpp == 2) {
                        i32 i = 0;
                        i32 t = colBase;
                        if (colBase > 0) {
                            do {
                                i32 o = i * 2;
                                m_lineBuf[o] = ssrc[o];
                                m_lineBuf[o + 1] = ssrc[o + 1];
                                i++;
                            } while (i < colBase);
                        }
                        while (t < stride) {
                            i32 e = t + 1;
                            m_lineBuf[e * 2 - 2] = gsrc[m_warpTable[t] * 2];
                            m_lineBuf[e * 2 - 1] = gsrc[m_warpTable[t] * 2 + 1];
                            t = e;
                        }
                    } else if (bpp == 3) {
                        if (colBase > 0) {
                            i32 d = 0;
                            u8* sp = ssrc + 2;
                            i32 c = colBase;
                            do {
                                m_lineBuf[d] = sp[-2];
                                m_lineBuf[d + 1] = sp[-1];
                                m_lineBuf[d + 2] = *sp;
                                d += 3;
                                c--;
                                sp += 3;
                            } while (c != 0);
                        }
                        if (colBase < stride) {
                            i32 d = colBase * 3;
                            for (i32 t = colBase; t < stride; t++) {
                                m_lineBuf[d] = gsrc[m_warpTable[t] * 3];
                                m_lineBuf[d + 1] = gsrc[m_warpTable[t] * 3 + 1];
                                m_lineBuf[d + 2] = gsrc[m_warpTable[t] * 3 + 2];
                                d += 3;
                            }
                        }
                    }
                } else {
                    u8* lut = m_table->m_data;
                    i32 i = 0;
                    i32 t = colBase;
                    if (colBase > 0) {
                        do {
                            m_lineBuf[i] = ssrc[i];
                            i++;
                        } while (i < colBase);
                    }
                    for (; t < stride; t++) {
                        m_lineBuf[t] = lut[static_cast<u32>(m_shadeRamp[t]) + static_cast<u32>(gsrc[m_warpTable[t]]) * 0x40];
                    }
                }
                u8* sp = m_lineBuf;
                i32 cnt = bpp * stride;
                u8* dp = dstLine;
                i32 n = cnt;
                if (cnt > 0) {
                    do {
                        *dp = *sp;
                        sp++;
                        n--;
                        dp++;
                    } while (n != 0);
                }
                if (m_stripCopy == 0) {
                    if (bpp * arg1 > 0) {
                        memset(dstLine + cnt, 0, bpp * arg1);
                    }
                } else {
                    i32 c2 = bpp * arg1;
                    dstLine -= c2;
                    u8* s2 = (arg0 - arg1) * bpp + m_rowOfsB[col] + m_straightBase;
                    if (c2 > 0) {
                        do {
                            *dstLine = *s2;
                            dstLine++;
                            s2++;
                            c2--;
                        } while (c2 != 0);
                    }
                }
                col++;
            } while (col < m_rowCount);
        }
    } else if (((m_mode == 1 && m_stripCopy == 0) || (m_mode == 2 && m_stripCopy != 0))
               && m_rowCount > 0) {
        i32 col = 0;
        i32 base = bpp * arg0;
        do {
            u8* dstLine = m_rowOfsA[col] + base + m_dstBase;
            u8* gsrc = m_rowOfsC[col] + base + m_gatherBase;
            u8* ssrc = m_rowOfsB[col] + base + m_straightBase;
            if (m_useLut == 0) {
                if (bpp == 1) {
                    i32 i = 0;
                    i32 t = colBase;
                    i32 e;
                    if (colBase > 0) {
                        do {
                            e = i + 1;
                            m_lineBuf[i] = gsrc[m_warpTable[i]];
                            i = e;
                        } while (e < colBase);
                    }
                    for (; t < stride; t++) {
                        m_lineBuf[t] = ssrc[t];
                    }
                } else if (bpp == 2) {
                    i32 i = 0;
                    i32 t = colBase;
                    if (colBase > 0) {
                        do {
                            i32 o = i * 4;
                            i++;
                            m_lineBuf[i * 2 - 2] = gsrc[m_warpTable[o / 4] * 2];
                            m_lineBuf[i * 2 - 1] = gsrc[m_warpTable[i - 1] * 2 + 1];
                        } while (i < colBase);
                    }
                    for (; t < stride; t++) {
                        i32 o = t * 2;
                        m_lineBuf[o] = ssrc[o];
                        m_lineBuf[o + 1] = ssrc[o + 1];
                    }
                } else if (bpp == 3) {
                    i32 k = 0;
                    if (colBase > 0) {
                        i32 d = 0;
                        do {
                            m_lineBuf[d] = gsrc[m_warpTable[k] * 3];
                            m_lineBuf[d + 1] = gsrc[m_warpTable[k] * 3 + 1];
                            m_lineBuf[d + 2] = gsrc[m_warpTable[k] * 3 + 2];
                            k++;
                            d += 3;
                        } while (k < colBase);
                    }
                    if (colBase < stride) {
                        i32 d = colBase * 3;
                        i32 c = stride - colBase;
                        u8* sp = ssrc + 2 + d;
                        do {
                            m_lineBuf[d] = sp[-2];
                            m_lineBuf[d + 1] = sp[-1];
                            m_lineBuf[d + 2] = *sp;
                            d += 3;
                            c--;
                            sp += 3;
                        } while (c != 0);
                    }
                }
            } else {
                u8* lut = m_table->m_data;
                i32 i = 0;
                i32 t = colBase;
                i32 e;
                if (colBase > 0) {
                    do {
                        e = i + 1;
                        m_lineBuf[i] = lut[static_cast<u32>(m_shadeRamp[i]) + static_cast<u32>(gsrc[m_warpTable[i]]) * 0x40];
                        i = e;
                    } while (e < colBase);
                }
                for (; t < stride; t++) {
                    m_lineBuf[t] = ssrc[t];
                }
            }
            u8* sp = m_lineBuf;
            i32 cnt = bpp * stride;
            u8* dp = dstLine;
            i32 n = cnt;
            if (cnt > 0) {
                do {
                    *dp = *sp;
                    sp++;
                    n--;
                    dp++;
                } while (n != 0);
            }
            if (m_stripCopy == 0) {
                if (bpp * arg1 > 0) {
                    memset(dstLine - bpp * arg1, 0, bpp * arg1);
                }
            } else {
                i32 c2 = bpp * arg1;
                u8* s2 = (arg0 + stride) * bpp + m_rowOfsB[col] + m_straightBase;
                dstLine += cnt;
                if (c2 > 0) {
                    do {
                        *dstLine = *s2;
                        dstLine++;
                        s2++;
                        c2--;
                    } while (c2 != 0);
                }
            }
            col++;
        } while (col < m_rowCount);
    }
}

RVA(0x00182900, 0x35)
i32 CFaderShape::GetFrameCount() {
    i32 mode = m_mode;
    if (mode == 1 || mode == 2) {
        return m_span - m_halfWidth * 2;
    }
    if (mode == 3) {
        return (m_span - m_halfWidth * 4) / 2;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// The slot-1 render virtuals (RenderFrame(i32 frame)) of three CFader subtypes. Identity
// recovered by data-ref: each RVA is its class's ??_7CFader<Sub>@@6B@+0x4 (vtable slot
// 1), the RenderFrame the class already declares in FaderSubtypes.h. Defined here as the real
// virtuals (were mis-homed Gap_* free-fn stubs from GapFunctions.cpp, matcher-5) so each
// ??_7 slot 1 binds to its own body; bodies parked (>512 B leaf-first reconstructions).
// ---------------------------------------------------------------------------
// @early-stop
// CFaderFlat::RenderFrame (0x17f660, vtable slot 1, 742 B): the flat fader's per-frame render.
RVA(0x0017f660, 0x2e6)
void CFaderFlat::RenderFrame(i32 frame) {}

// @early-stop
// CFaderSine::RenderFrame (0x17ff30, vtable slot 1, 1218 B): the sine fader's per-frame render.
RVA(0x0017ff30, 0x4c2)
void CFaderSine::RenderFrame(i32 frame) {}

// @early-stop
// CFaderShape::RenderFrame (0x181b00, vtable slot 1, 847 B): the shape fader's per-frame render.
RVA(0x00181b00, 0x34f)
void CFaderShape::RenderFrame(i32 frame) {}
