// Fader.cpp - the CFader screen-transition family (C:\Proj\Gruntz): the CFader
// base + its concrete subtypes (CFaderSine / CFaderFlat / CFaderMesh /
// CFaderRadial / CFaderLight / CFaderShape) plus the flat body-views that host
// each subtype's ApplyInit (CFaderRadialApply / CFaderShapeApply / CFaderElem /
// CFaderTileRender + the FaderRun helper). One dev TU, formerly
// split across Fader.cpp + Fader{Run,Radial,ElemSetup,ShapeSetup,TileRender,
// 17e940Apply}.cpp + Obj5f0890Dtor.cpp (the ~CFaderShape dtor). Merged per
// docs/tu-topology-plan.md (Phase 1); FaderMgr.cpp stays its own TU. All in the
// 0x17d8f0-0x182610 engine band (one link cluster).
//
// waveM-mech absorbed the ex fxmodedesc unit (the CFxModeDesc/T1-T6 mode-descriptor
// ctors @0x17e7b0-0x17e910, woven between CFader::Set2c and CFaderMesh) + the ex
// lighteffectsetup unit (CFaderLightApply::Setup + CFaderLight::v2/v3/v4 @0x1804a0-
// 0x1816a0, woven around CFaderLight's ctor/dtor) - both frag-less but text-woven
// (Fader brackets each), so ONE original TU. The ex-fxmodedesc's out-of-interval
// MakeButeSectionKey@0xf9280 stays in FxModeDesc.cpp (a separate obj, ~0.44M RVA away).
//
// Unit flags eh: CFader's dtor + ~CFaderShape need the /GX EH frame; the run/
// build/render helpers (RunFade / ApplyInit / Apply / Build / Setup / RenderTile /
// RenderWarpTile) carry no destructible C++ object so /GX is a no-op for them
// (byte-verified under eh). Functions kept in ascending retail-RVA order at the
// per-original-TU section granularity (within-TU order is byte-neutral,
// docs/patterns/within-tu-order-vs-field-order.md); each section keeps its
// original file's provenance comment.
#include <Gruntz/Fader.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <Gruntz/FaderSubtypes.h> // the six concrete subtypes (declarations)
#include <Gruntz/FadeSink.h>      // IFadeSink (the CFader::RunFade fade-notify sink; P2)
#include <Ints.h>
#include <Mfc.h>                // superset of Win32.h; needed for CDDSurface (CPtrArray member)
#include <DDrawMgr/DDSurface.h> // the real CDDSurface (was the FrSurface view)
#include <DDrawMgr/DDrawPtrCollections.h> // the +0x2c overlay surface pool (CFaderLight v3/v4)
#include <Gruntz/FxModeT1.h>              // the CString-bearing CFxModeT1 (ex fxmodedesc)
#include <Gruntz/FxModeDesc.h>            // CFxModeDesc + T2-T6 (ex fxmodedesc)
#include <math.h>                         // acos/sin (fsin) / sqrt (fsqrt) intrinsics
#include <string.h>                       // rep-movs / memset element copies
#include <rva.h>

// ApplyInit's two .rdata FP constants. Kept as minimal externs (not a fat
// <Globals.h> include, which leaks regalloc into the sibling render helpers - see
// docs on globals consolidation). Owner-TU defs; the reference extern stays in
// <Globals.h>.
DATA(0x001f07ec)
float g_fxBias = -50.0f; // 0x5f07ec
DATA(0x001f07f4)
float g_fxEps = 1.0f; // 0x5f07f4

// ============================================================================
// section: Fader.cpp
// ============================================================================
// Fader.cpp - the CFaderMgr element base class + one concrete subtype's
// destructor (tracer placeholder MallocCtor_17fdb0, keyed off the 0x17fdb0 subtype
// ctor). CFader is the polymorphic base of the six screen-fader subtypes the
// CFaderMgr::Add factory allocates; each fader owns a CShadeTableCache color-table
// cache at +0x04 plus the manager-primed timing fields, and a GetTickCount
// busy-wait helper.
//
// Methods in ascending retail-RVA order. Field names are placeholders; offsets +
// code bytes are load-bearing. The CShadeTableCache ctor/dtor/FindRemove (0x14de30
// / 0x14de50 / 0x14fb80) and operator new/delete are external/reloc-masked; the
// fader subtype vftables are stamped as reloc-masked DIR32 data.

// ===========================================================================
// 0x17e450 - CFader::CFader(): build the cache subobject (0x14de30), stamp the
// fader vftable, zero m_table, and arm the teardown flag.
// ===========================================================================
RVA(0x0017e450, 0x23)
CFader::CFader() {
    m_table = 0;
    m_flag = 1;
}

// ===========================================================================
// 0x17e4a0 - CFader::~CFader(): restore the fader vftable, FindRemove the cached
// table from the cache when armed, then destruct the cache subobject (0x14de50).
// The destructible m_cache member forces the /GX EH frame.
// ===========================================================================
// Real polymorphic now: cl emits the implicit ??_7CFader vptr re-stamp in the
// ENTRY state (stamp-first, == retail), and the destructible m_cache member folds
// in to supply the /GX frame. (eh-dtor-implicit-vptr-stamp-first.md sub-case 1.)
RVA(0x0017e4a0, 0x69)
CFader::~CFader() {
    if (m_table && m_flag) {
        m_cache.FindRemove(m_table);
        m_table = 0;
    }
}

// ===========================================================================
// 0x17e510 - CFader::Wait(delay): spin on GetTickCount() until at least `delay`
// ms have elapsed since the call. __thiscall (ecx=this, unused by the body).
// ===========================================================================
RVA(0x0017e510, 0x23)
void CFader::Wait(i32 delay) {
    DWORD target = GetTickCount() + delay;
    while (GetTickCount() < target) {
    }
}

// CFader::SetTimers (0x17e760) - store the two timer values.
RVA(0x0017e760, 0x11)
void CFader::SetTimers(i32 a, i32 b) {
    m_timerA = a;
    m_timerB = b;
}

// CFader::Set2c (0x17e780) - store the +0x2c arg.
RVA(0x0017e780, 0xa)
void CFader::Set2c(i32 v) {
    m_set2cArg = v;
}

// ===========================================================================
// The mode/effect descriptor record family (ex fxmodedesc, woven here between
// Set2c and CFaderMesh). CFxModeDesc/T1-T6 come from <Gruntz/FxModeDesc.h> /
// <Gruntz/FxModeT1.h>; each ctor base-inits then stamps its type record.
// ===========================================================================

// 0x17e7b0 - CFxModeDesc(): zero the type discriminator.
RVA(0x0017e7b0, 0x9)
CFxModeDesc::CFxModeDesc() {
    m_type = 0;
}

// 0x17e7c0 - CFxModeT1(): the type-1 variant ctor (base + CString member ctor, stamp
// type=1/m_10=0x32/m_14=1/m_18=1, assign the empty string to the +0x24 CString). The
// destructible CString member forces the /GX frame.
extern "C" char g_emptyString[]; // 0x6293f4
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

// 0x17e880 - CFxModeT3(): base ctor, stamp the type-3 record.
RVA(0x0017e880, 0x28)
CFxModeT3::CFxModeT3() {
    m_type = 3;
    m_04 = 0;
    m_08 = 0;
    m_0c = 1;
    m_10 = 0xf;
}

// 0x17e8b0 - CFxModeT4(): base ctor, stamp the type-4 record (m_0c stored last).
RVA(0x0017e8b0, 0x27)
CFxModeT4::CFxModeT4() {
    m_type = 4;
    m_04 = 0;
    m_08 = 0;
    m_10 = 0;
    m_14 = 0;
    m_0c = 1;
}

// 0x17e8e0 - CFxModeT5(): base ctor, stamp the type-5 record (m_10 stored last).
RVA(0x0017e8e0, 0x27)
CFxModeT5::CFxModeT5() {
    m_type = 5;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_14 = 0;
    m_10 = 0x19;
}

// 0x17e910 - CFxModeT6(): base ctor, stamp the type-6 record.
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

// ===========================================================================
// CFaderMesh - a CFader subtype (ctor 0x17e940, size 0x6c) that embeds a nested
// polymorphic sub-object at +0x58 (its own vftable 0x5f07d8 + four zeroed fields).
// ATYPICAL vptr order: the member sub-object is constructed (its vptr + fields)
// BEFORE the subtype's own primary vftable 0x5f07c0 is stamped -- MSVC5's ctor
// order is base ctors, MEMBER ctors, own vptr, body (cf. CFader::CFader building
// m_cache before its own stamp). cl inlines the member ctor, so the member vptr +
// field zeros fall between the CFader base ctor call and the sunk own-vptr stamp.
// (Class declaration in <Gruntz/FaderSubtypes.h>.)
// ===========================================================================
RVA(0x0017e940, 0x27)
CFaderMesh::CFaderMesh() {}

// 0x17e990 - ~CFaderMesh (the real class dtor; was the C17e990 placeholder trio
// EmbedBase17e990/EmbedSub17e990/C17e990, three RELOC_VTBL fake views). Empty body:
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
// 0x17ef00 - CFaderMesh::v1(frame): the mesh-warp blit. Prime the dest surface
// (m_3c): Blt the m_40 source if set, else Clear it. Then for each of the m_58
// buffer's records (40 bytes = {srcRectA[4], dstRectB[4], _, _}): interpolate the
// A->B rect by t = frame/v2(), clip it to the dest surface, and BltEx from the m_38
// source (srcRect = m_4c ? rectA : the clipped rectB). Finally Flip m_44.
// ===========================================================================
// @early-stop
// x87-fp-stack-schedule wall (docs/patterns/x87-fp-stack-schedule.md; sibling of
// ApplyInit @0x17ea00): the per-record t=frame/v2() divide, the four
// A + (int)((B-A)*t) rect interpolations (fild/fmul/__ftol), the surface clip and the
// BltEx/Flip dispatch are byte-faithful in operation/offset, but retail keeps t on the
// x87 stack across the four __ftol calls and colours the record temporaries into a
// dense frame-slot layout cl doesn't reproduce; the frame/count int64->float loads
// also differ (fild qword vs dword). Not source-steerable.
RVA(0x0017ef00, 0x21c)
void CFaderMesh::v1(i32 frame) {
    CDDSurface* dst = (CDDSurface*)m_3c;
    if (m_40 != 0) {
        dst->Blt((CDDSurface*)m_40);
    } else {
        dst->Clear(0);
    }
    if (m_58.m_nSize > 0) {
        float ff = (float)frame;
        char* pData = (char*)m_58.m_pData;
        for (i32 i = 0; i < m_58.m_nSize; i++) {
            i32* rec = (i32*)(pData + i * 0x28);
            i32 r0 = rec[0], r1 = rec[1], r2 = rec[2], r3 = rec[3];
            i32 r4 = rec[4], r5 = rec[5], r6 = rec[6], r7 = rec[7];
            float t = ff / (float)v2();

            i32 x0 = r0 + (i32)((float)(r4 - r0) * t);
            i32 y0 = r1 + (i32)((float)(r5 - r1) * t);
            i32 x1 = r2 + (i32)((float)(r6 - r2) * t);
            i32 y1 = r3 + (i32)((float)(r7 - r3) * t);

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
            if (m_4c != 0) {
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
            dst->BltEx(dstRect, (CDDSurface*)m_38, srcRect, 0x1000000, 0);
        }
    }
    ((CDDSurface*)m_44)->Flip(0);
}

// ===========================================================================
// CFaderSine - the case-"3" / jump-index-2 fader subtype (CFaderMgr::Add allocates
// 0x7d5c bytes for it). Its motion virtuals (0x17ff30 / 0x180400) override the two
// CFader pure virtuals (slots 1/2); slots 3/4 are inherited. Real polymorphic now:
// the empty ~CFaderSine stamps ??_7CFaderSine then tail-calls ~CFader, and cl emits
// ??_7CFaderSine (slots reloc-mask the 0x5f0848 target). Name is descriptive.
// ===========================================================================
// (Class declaration in <Gruntz/FaderSubtypes.h>.)
// ===========================================================================
// 0x17fdb0 - CFaderSine(): chain CFader::CFader, stamp ??_7CFaderSine, zero the
// two subtype fields. cl auto-emits the base ctor call + the implicit vptr stamp
// (reloc-masked vs 0x5f0848); the subtype virtuals are declared (not defined
// here) only to make the class concrete + size its vtable.
// ===========================================================================
RVA(0x0017fdb0, 0x1a)
CFaderSine::CFaderSine() {
    m_50 = 0;
    m_4c = 0;
}

// ===========================================================================
// 0x17fdf0 - ~CFaderSine(): stamp the subtype vftable, then tail-call ~CFader().
// ===========================================================================
RVA(0x0017fdf0, 0xb)
CFaderSine::~CFaderSine() {}

// ===========================================================================
// 0x17fe00 - CFaderSine::ApplyInit(desc): copy the source-box geometry out of the
// descriptor (falling back to the shared timer fields the manager primed), range-
// check the 0..100 intensity, compute the scaled magnitude via the FP pipeline, then
// fill four parallel 2000-int arrays (three zeroed, one seeded with rand()%count) and
// scatter the last one via ScatterSamples (0x182940, ScatterSamples.cpp).
// ===========================================================================
// The inlined game RNG (a THIRD LCG instance: own seed-flag + state, distinct from
// the 0x6c127d/0x6c1288 and 0x6c2798 pairs), seeded lazily from timeGetTime.
extern u8 g_fxRandSeeded;                 // 0x6c279c  seed-init flag (bit 0)
extern i32 g_fxRandSeed;                  // 0x6c27a8  LCG seed

static __inline i32 FxRand(i32 range) {
    u32 x;
    if (!(g_fxRandSeeded & 1)) {
        g_fxRandSeeded |= 1;
        x = ::timeGetTime();
    } else {
        x = g_fxRandSeed;
    }
    g_fxRandSeed = x * 214013 + 2531011;
    return (((i32)g_fxRandSeed >> 16) & 0x7fff) % range;
}

extern const float g_faderScale_5f085c;       // 0x5f085c  intensity->magnitude scale
void ScatterSamples(i32* arr, i32, i32, i32); // 0x182940 ?ScatterSamples@@YAXPAHHHH@Z

// @early-stop
// regalloc coin-flip wall (73.5% fuzzy). Full body is byte-shape-identical to retail;
// the residual is a callee-saved coloring swap that touches every ModRM byte: retail
// colors this->ebx and the reused const-0/count->edi, while cl picks this->edi /
// const-0->ebx (a symmetric loop-weight tie broken the other way). Verified via
// llvm-objdump -dr base vs target - the only mnemonic-level diffs are the ebx/edi
// register columns.
RVA(0x0017fe00, 0x12d)
i32 CFaderSine::ApplyInit(CFaderInit* desc) {
    i32 w;
    i32 p;
    i32 i;
    m_20 = 0;
    m_40 = desc->m_0c;
    FaderSrc* src = (FaderSrc*)desc->m_04;
    if (!src) {
        src = (FaderSrc*)m_timerA;
    }
    m_38 = src;
    i32 alt = desc->m_08;
    if (!alt) {
        alt = m_timerB;
    }
    m_3c = (FaderSrc*)alt;
    if (!m_38) {
        goto fail;
    }
    if (!m_3c) {
        m_40 = 1;
    }
    m_50 = m_38->m_1c;
    w = m_38->m_frameCount;
    m_4c = w;
    p = desc->m_10;
    if (p < 0) {
        goto fail;
    }
    if (p > 100) {
        goto fail;
    }
    m_58 = p;
    m_54 = (i32)((float)p * g_faderScale_5f085c * w);
    for (i = 0; i < 2000; i++) {
        m_arr0[i] = 0;
        m_arr2[i] = 0;
        m_arr3[i] = 0;
        m_arr1[i] = FxRand(m_50);
    }
    ScatterSamples(m_arr3, 0, m_50, 1);
    return 1;
fail:
    return 0;
}

// ===========================================================================
// CFaderFlat - the fader subtype whose ctor (0x17f530) only clears m_4c. Its
// vftable is 0x5f07f8. Same modeling as CFaderSine.
// ===========================================================================
// (Class declaration in <Gruntz/FaderSubtypes.h>.)
// ===========================================================================
RVA(0x0017f530, 0x19)
CFaderFlat::CFaderFlat() {
    m_4c = 0;
}

// ===========================================================================
// CFaderLight - subtype ctor 0x180410: clears m_40. vftable 0x5f0870.
// ===========================================================================
// (Class declaration in <Gruntz/FaderSubtypes.h>; size 0x206c pinned from the
// CFaderMgr::Add new(0x206c) allocation.)
// ===========================================================================
RVA(0x00180410, 0x19)
CFaderLight::CFaderLight() {
    m_40 = 0;
}

// 0x180450 - ~CFaderLight() (re-homed/folded from src/Stub/BoundaryUpperEh.cpp, the
// C180450 placeholder): stamp the subtype vftable, run the member teardown (SubFree
// @0x180630), then tail-call ~CFader. cl emits the /GX base-subobject unwind frame.
RVA(0x00180450, 0x4f)
CFaderLight::~CFaderLight() {
    SubFree180630();
}

// ===========================================================================
// CFaderLight's shade/lighting effect setup pass (ex lighteffectsetup). Modeled as
// the flat body-view CFaderLightApply (CFaderLight : public CFader reads the base-
// region slots repurposed as surface/palette; fold onto CFaderLight is a follow-up).
// ===========================================================================

// The corner-distance exponent (retail .rdata double @0x5f0888 = 2.0); passed to the
// __CIpow intrinsic so the squarings emit `fild x; fld K; call pow`.
DATA(0x001f0888)
extern const double g_faderPowK; // 2.0

// SIZE annotations for the ShadeTableCache.h classes are hosted here (a neutral
// includer) rather than in the header: shadetablecache.cpp reschedules under any
// header-injected typedef.
SIZE(CShadeTable, 0x10);      // array-element stride (0x10-byte buffer wrapper)
SIZE(CShadeTableArray, 0x14); // MFC CObArray-shaped subobject (cache 0x18 - 0x04)
SIZE(PalEntry, 0x4);          // 4-byte palette record (256-entry array stride)
SIZE(CShadeTableCache, 0x18); // RE'd heap-alloc size (CGruntzMgr +0x50)

// PtInRect reached through a game-owned function pointer (ff 15).
DATA(0x002c456c)

struct Surf {
    char m_pad00[0x18];
    i32 m_height; // +0x18 height
    i32 m_width;  // +0x1c width
};
SIZE_UNKNOWN(Surf);
struct PalHolder {
    char m_pad00[0xc];
    PalEntry* m_palette; // +0x0c palette
};
SIZE_UNKNOWN(PalHolder);
struct LightDesc {
    char m_pad00[0x4];
    Surf* m_surface;              // +0x04
    i32 m_8;                      // +0x08
    PalHolder* m_paletteHolder;   // +0x0c
    i32 m_10;                     // +0x10
    i32 m_spanCount;              // +0x14 span count
    i32 m_centerX;                // +0x18 centre x
    i32 m_centerY;                // +0x1c centre y
    CShadeTable* m_overrideTable; // +0x20 override table (pointer)
};
SIZE_UNKNOWN(LightDesc);

class CFaderLightApply {
public:
    i32 Setup(LightDesc* d);

    i32 m_00;                  // +0x00
    CShadeTableCache m_cache;  // +0x04  embedded shade-table cache (0x18 B -> +0x1c)
    CShadeTable* m_shadeTable; // +0x1c shade table
    i32 m_20;                  // +0x20
    Surf* m_defaultSurface;    // +0x24 default surface
    i32 m_28;                  // +0x28
    char m_pad2c[0x30 - 0x2c];
    i32 m_30; // +0x30
    char m_pad34[0x38 - 0x34];
    Surf* m_activeSurface; // +0x38 active surface
    i32 m_3c;              // +0x3c
    char m_pad40[0x44 - 0x40];
    PalHolder* m_paletteHolder; // +0x44
    i32 m_48;                   // +0x48
    i32 m_centerX;              // +0x4c centre x
    i32 m_centerY;              // +0x50 centre y
    char m_pad54[0x5c - 0x54];
    i32 m_5c;               // +0x5c  frame count = max light->corner distance (v2 output)
    i32 m_spanStarts[1024]; // +0x60   span starts
    i32 m_spanEnds[1024];   // +0x1060 span ends
    i32 m_spanCount;        // +0x2060 span count
    i32 m_surfaceWidth;     // +0x2064 surface width
    i32 m_surfaceHeight;    // +0x2068 surface height
};
SIZE_UNKNOWN(CFaderLightApply);

// CFaderLightApply::Setup (0x1804a0) = CFaderLight::ApplyInit: capture a descriptor's
// surface/colour parameters, clip the centre to the surface rect (early-out if outside),
// fill the per-scan span tables, resolve the hue-ramp shade table.
// @early-stop
// 92% - /O2 regalloc entropy tail: the descriptor field loads + the m_3c/m_activeSurface
// conditional reuse eax in retail but the recompile distributes them across ecx/edx/eax;
// same instruction selection + scheduling, only the register names differ. Final sweep.
RVA(0x0001804a0, 0x182)
i32 CFaderLightApply::Setup(LightDesc* d) {
    m_20 = 0;
    Surf* s = d->m_surface;
    if (s == 0) {
        s = m_defaultSurface;
    }
    m_activeSurface = s;
    i32 b = d->m_8;
    if (b == 0) {
        m_3c = m_28;
    } else {
        m_3c = b;
    }
    m_48 = d->m_10;
    m_centerX = d->m_centerX;
    m_centerY = d->m_centerY;
    PalHolder* pal = d->m_paletteHolder;
    m_paletteHolder = pal;
    i32 cnt = d->m_spanCount;
    m_spanCount = cnt;
    if (cnt > 0 && d->m_overrideTable == 0 && pal == 0) {
        return 0;
    }
    if (m_activeSurface == 0) {
        return 0;
    }
    if (m_3c == 0 && m_48 == 0) {
        return 0;
    }
    RECT rect;
    rect.right = m_activeSurface->m_width;
    m_surfaceWidth = rect.right;
    rect.bottom = m_activeSurface->m_height;
    m_surfaceHeight = rect.bottom;
    rect.left = 0;
    rect.top = 0;
    POINT pt;
    pt.x = m_centerX;
    pt.y = m_centerY;
    if (::PtInRect(&rect, pt) == 0) {
        return 0;
    }
    if (m_48 != 0) {
        i32 i = 0;
        if (m_surfaceHeight > 0) {
            do {
                m_spanStarts[i] = 0;
                m_spanEnds[i] = m_surfaceWidth;
                i++;
            } while (i < m_surfaceHeight);
        }
    } else {
        i32 i = 0;
        if (m_surfaceHeight > 0) {
            do {
                m_spanStarts[i] = m_centerX;
                m_spanEnds[i] = m_centerX;
                i++;
            } while (i < m_surfaceHeight);
        }
    }
    if (m_spanCount > 0) {
        if (d->m_overrideTable == 0) {
            m_shadeTable = m_cache.HueRampTable(m_paletteHolder->m_palette, m_spanCount, 0);
            m_30 = 1;
            return 1;
        }
        m_shadeTable = d->m_overrideTable;
    }
    return 1;
}

// 0x180630 - CFaderLight::SubFree180630: the empty member-teardown ~CFaderLight calls,
// a bare `ret`. The __fpclear FID row that owned 0x180630 was a false positive (that
// 1-byte-`ret` fingerprint matched ~35 rvas at LOW confidence, incl. CFader::v3/v4
// @0x17e790/0x17e7a0); the retail dtor calls it thiscall (ecx=this), so it is the game
// member, not a CRT stub. Reconstructed here (library label removed) - the dtor's call
// binds. reloc-fidelity (R45).
RVA(0x00180630, 0x1)
void CFaderLight::SubFree180630() {}

// @early-stop
// 0x180640 (2412 B) = a large light/circle-shade blit worker; homed pending leaf-first
// reconstruction (>512 B).
RVA(0x00180640, 0x96c)
i32 Gap_180640(void) {
    return 0;
}

// CFaderLight::v2 (0x1814f0, vtable slot 2) - the fade frame count = the maximum
// distance from the light centre to any active-surface corner (each squaring is
// pow(x, 2.0), the largest hypotenuse __ftol'd into m_5c).
// @early-stop
// x87-fp-stack-schedule wall (docs/patterns/x87-fp-stack-schedule.md): the four pow/sqrt
// corner distances + the running-max are byte-faithful in operation/operand, but retail
// interleaves the pow calls with a dense fxch/fld juggle + open-codes the max as an fcomp
// tree; cl serialises them + lowers the max as fcom/branch pairs. Not source-steerable.
RVA(0x001814f0, 0x16d)
i32 CFaderLight::v2() {
    CFaderLightApply* self = (CFaderLightApply*)this;
    i32 cx = self->m_centerX;
    i32 cy = self->m_centerY;
    i32 w = self->m_activeSurface->m_width;
    i32 h = self->m_activeSurface->m_height;

    double pA = pow((double)cx, g_faderPowK);
    double pB = pow((double)cy, g_faderPowK);
    double pH = pow((double)(h - cy), g_faderPowK);
    double pW = pow((double)(w - cx), g_faderPowK);

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
    i32 r = (i32)m;
    self->m_5c = r;
    return r;
}

// CFaderLight::v3 (0x181660, vtable slot 3) - AddItem: when active, acquire a fresh
// overlay surface from the +0x2c pool (MakeAndAddB), stash it in m_40, blit it onto m_38.
RVA(0x00181660, 0x40)
void CFaderLight::v3() {
    if (m_2060 > 0 && m_48 != 0) {
        CDDrawPtrCollections* pool = (CDDrawPtrCollections*)m_set2cArg; // +0x2c dual-role pool slot
        CDDSurface* h = pool->MakeAndAddB(m_2064, m_2068, 0, 0, -1);
        m_40 = h;
        h->Blt(m_38);
    }
}

// CFaderLight::v4 (0x1816a0, vtable slot 4) - DropItem: if an overlay surface is held,
// release it back to the +0x2c pool (RemoveItemA) and clear the slot.
RVA(0x001816a0, 0x1c)
void CFaderLight::v4() {
    if (m_40) {
        CDDrawPtrCollections* pool = (CDDrawPtrCollections*)m_set2cArg;
        pool->RemoveItemA(m_40);
        m_40 = 0;
    }
}

// ===========================================================================
// CFaderRadial - subtype ctor 0x17f9a0: m_44/m_40/m_50 = 0, m_48 = 1. vftable
// 0x5f0810.
// ===========================================================================
// (Class declaration in <Gruntz/FaderSubtypes.h>; size 0x5c pinned from the
// CFaderMgr::Add new(0x5c) allocation.)
// ===========================================================================
RVA(0x0017f9a0, 0x24)
CFaderRadial::CFaderRadial() {
    m_44 = 0;
    m_40 = 0;
    m_50 = 0;
    m_48 = 1;
}

// 0x17f9f0 - ~CFaderRadial() (re-homed/folded from src/Stub/BoundaryUpperEh.cpp, the
// C17f9f0 placeholder): stamp the subtype vftable, free m_50 (FreeBuffer17fc40:
// `if(m_50) RezFree(m_50)`), then tail-call ~CFader. cl emits the /GX unwind frame.
RVA(0x0017f9f0, 0x4f)
CFaderRadial::~CFaderRadial() {
    FreeBuffer17fc40();
}

// 0x17fc40 - CFaderRadial::FreeBuffer17fc40: release the owned +0x50 buffer (no
// zero-out) via ::operator delete (0x1b9b82, reloc-masked). Re-homed from matcher-5's
// BoundaryUpper re-attack; its target file (BoundaryUpperEh.cpp) was deleted by the
// parallel fader-dtor fold, so the body lands here on the real class (the
// ~CFaderRadial self-call proved the identity).
RVA(0x0017fc40, 0x11)
void CFaderRadial::FreeBuffer17fc40() {
    if (m_50) {
        ::operator delete((void*)m_50);
    }
}

// ===========================================================================
// CFaderShape - subtype ctor 0x1816c0 (size 0x494): zeroes m_478/m_44/m_48/m_4c/
// m_488/m_48c and the CFader base field m_20. vftable 0x5f0890.
// ===========================================================================
// (Class declaration in <Gruntz/FaderSubtypes.h>.)
// ===========================================================================
RVA(0x001816c0, 0x32)
CFaderShape::CFaderShape() {
    m_478 = 0;
    m_44 = 0;
    m_48 = 0;
    m_4c = 0;
    m_488 = 0;
    m_48c = 0;
    m_20 = 0;
}

// ===========================================================================
// 0x17e540 - CFader::RunFadeStepped(step, lead, notify): the stepped counterpart
// of RunFade. Primes frame 0, busy-waits the lead-in, then renders every `step`-th
// frame from 1..count back-to-back (no elapsed/duration mapping), poking the
// m_set2cArg fade sink + v1(frame) each step. Finalizes v1(count)/v4() and records
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
    i32 count = v2();
    if (count < 1) {
        return;
    }
    v3();
    v1(0);
    Wait(lead);
    DWORD startTick = GetTickCount();
    i32 loops = 0;
    i32 frame = 1;
    if (count >= 1) {
        do {
            if (notify && m_set2cArg) {
                IFadeSink* o = *(IFadeSink**)m_set2cArg;
                o->FadeNotify(1, 0);
            }
            v1(frame);
            loops++;
            frame += step;
        } while (frame <= count);
    }
    if (frame != count) {
        v1(count);
        loops++;
    }
    float fLoops = (float)loops;
    DWORD elapsed = GetTickCount() - startTick;
    m_34 = (i32)(fLoops / ((float)elapsed * 0.001f));
    v4();
}

// ============================================================================
// section: FaderRun.cpp
// ============================================================================
// FaderRun.cpp - the CFader "run timed fade" driver (0x17e620), a CFader-subtype
// method the ApiCaller stub misfiled as winapi_17e620_GetTickCount. It drives the
// whole fade: primes frame 0, busy-waits the lead-in (Wait), then spins on
// GetTickCount mapping elapsed/duration onto the [0..count] frame index (v2 = frame
// count, v1 = render frame N). Each newly-reached frame optionally pokes a COM-style
// sink (m_2c) then renders; at the end it records the achieved frame rate in m_34
// and finalizes via v4. Field NAMES are placeholders; offsets + bytes load-bearing.
//
// `this` is modelled as a standalone CFader-subtype shape (vtable + fields to +0x38)
// rather than deriving CFader, because the recovered CFader.h fixes v1/v2 as void()
// while this driver needs v1(int)/v2()->int; the vtable SLOTS + call bytes are what
// match (reloc-masked), not the names. NON-EH (base /O2) frame.

// The busy-wait spinner (0x17e510): spins until GetTickCount() >= now + delay.
// Reloc-masked rel32 callee, __thiscall (ignores its this).

// The COM-style fade sink IFadeSink (P2 placeholder, <Gruntz/FadeSink.h>) is reached
// through m_set2cArg (an IFadeSink** stored as a dword via Set2c): *sink is the
// interface whose slot 0x58 is poked __stdcall(this, 1, 0) once per newly-reached frame.
// (Was the standalone FaderRun view; dissolved onto the real CFader base - RunFade is a
// CFader method @0x17e620, Wait/v1/v2/v3/v4 are the base's, m_2c==m_set2cArg, m_34 is the
// base's trailing frame-rate field. The v1(i32)/i32 v2() slot signatures the driver needs
// are now the CFader canonical, fixing the old void-signature mismodel.)

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
    i32 count = v2();
    if (count < 1) {
        return;
    }
    v3();
    v1(0);
    Wait(lead);
    i32 loops = 0;
    DWORD startTick = GetTickCount();
    float fStart = (float)startTick;
    float fDur = (float)dur;
    float fCount = (float)count;
    if (count >= 0) {
        do {
            frame = (i32)(((float)GetTickCount() - fStart) / fDur * fCount);
            if (prev != frame && frame <= count && frame > 0) {
                if (notify && m_set2cArg) {
                    IFadeSink* o = *(IFadeSink**)m_set2cArg;
                    o->FadeNotify(1, 0);
                }
                v1(frame);
                loops++;
            }
            prev = frame;
        } while (frame <= count);
    }
    if (frame != count) {
        v1(count);
        loops++;
    }
    float fLoops = (float)loops;
    DWORD elapsed = GetTickCount() - startTick;
    m_34 = (i32)(fLoops / ((float)elapsed * 0.001f));
    v4();
}

// ============================================================================
// section: Fader17e940Apply.cpp
// ============================================================================
// Fader17e940Apply.cpp - CFaderMesh::ApplyInit (0x17ea00), the type-6
// screen-fader's "apply the transition descriptor" method (the mesh builder).
//
// DE-VIEW NOTE (was mislabeled CFxModeT3::Build): Ghidra RTTI names 0x17ea00
// ?Build@CFxModeT3@@QAEHPAUFxConfig@@@Z, but that attribution is WRONG. This
// method's `this` (ecx) is the 0x6c-byte CFaderMesh (ctor 0x17e940, own vftable
// 0x5f07c0, growable mesh buffer at +0x58), NOT the 0x14-byte CFxModeT3 mode
// descriptor (ctor 0x17e880, fields only 0x00..0x10). Proof: 0x17e880 writes just
// 0x14 bytes, whereas this method reads/writes +0x24/+0x28/+0x3c..+0x54 and inits a
// buffer at +0x58, so 0x58+0x14 == 0x6c == the CFaderMesh allocation size. Its
// only caller is CFaderMgr::Add's case-6 arm (RezAlloc(0x6c) -> ctor 0x17e940 ->
// this method), and the descriptor argument it consumes is the CFxMode-family record
// CFaderMgr builds via CFxModeT6::CFxModeT6 (0x17e910) / CFaderInit::BuildDefaultInit5.
// So the real owner is CFaderMesh (canonical decl in <Gruntz/FaderSubtypes.h>),
// the real method name is ApplyInit, and CFxModeT3 the descriptor stays modeled at
// its true 0x14 size in <Gruntz/FxModeDesc.h>.
//
// It latches the descriptor into this, sizes the source/dest boxes, derives the
// per-cell step (dx = box->h / m_50, dy = box->w / m_54) and the cell radius
// (sqrt(dx*dx+dy*dy)), then walks an (m_54 x m_50) grid emitting one projected mesh
// point per cell into the growable buffer at this+0x58. Each point ellipse-projects
// the cell center: the radial distance v = sqrt((row-halfH)^2 + (col-halfW)^2)
// drives a normalized (x,y) direction, scaled by the cell radius and accumulated via
// OffsetRect onto two sub-rects (pt48 = src cell, pt64 = dst cell), assembled (order
// gated on m_4c) into the 40-byte mesh record. Field names are placeholders; offsets
// + code bytes are load-bearing. The point conversions go through the CRT __ftol; the
// buffer grow is the inlined MFC CArray::SetAtGrow(GetSize(),&pt) (RezAlloc/RezFree).

// A box whose +0x18 / +0x1c are its width / height (read by the param derive). The
// active src/dst box pointers live in CFaderMesh::m_3c / m_38 (stored as dwords).
struct FxBox {
    char pad[0x18];
    i32 m_width;  // +0x18
    i32 m_height; // +0x1c
};

// The CFxMode transition descriptor (the ApplyInit argument): the mode record
// CFaderMgr::Add fills (CFxModeT6::CFxModeT6 / BuildDefaultInit5) or passes through.
// CFaderInit models the same bytes as an opaque blob + a trailing CString; this view
// names the int parameter fields ApplyInit reads. +0x00 is the type discriminator.
struct FxTransDesc {
    i32 m_00;
    FxBox* m_srcBox; // +0x04 src box override (else this->m_timerA)
    FxBox* m_dstBox; // +0x08 dst box override (else this->m_timerB)
    i32 m_0c;
    i32 m_10;          // gate: 0 -> bail
    i32 m_recordOrder; // +0x14
    i32 m_18;
    i32 m_columns; // +0x1c
    i32 m_rows;    // +0x20
};

// The 40-byte (10-dword) mesh point pushed into the buffer.
struct FxPoint {
    i32 v[10];
};

// A typed view of the growable buffer sub-object embedded at CFaderMesh::m_58
// (the CRezBufferObject / MFC CArray<FxPoint>): vptr + pData/size/max/growby.
struct E40 {
    char m_b[40];
};
// CArray layout: a CObject-style 4-byte head (+0x00, untouched by SetSize), then
// m_pData/m_nSize/m_nMaxSize/m_nGrowBy.
struct CArrayE40 {
    void* m_head;   // +0x00
    E40* m_pData;   // +0x04
    i32 m_nSize;    // +0x08
    i32 m_nMaxSize; // +0x0c
    i32 m_nGrowBy;  // +0x10
    void SetSize(i32 nNewSize, i32 nGrowBy);
};

// The fade mesh buffer IS a CArrayE40 (Init @0x17f390 = CArrayE40::SetSize); cast at the call.
struct FxMeshBuffer {
    char _vft0[4];           // +0x00 foreign/base object vptr (reduced view; not owned/dispatched)
    FxPoint* m_pData;        // +0x04 (+0x5c)
    i32 m_nSize;             // +0x08 (+0x60)
    i32 m_nMaxSize;          // +0x0c (+0x64)
    i32 m_nGrowBy;           // +0x10 (+0x68)
    void Init(i32 a, i32 b); // 0x17f390 (external, reloc-masked)
};

// The OffsetRect import (reached via the global function pointer at 0x6c4490) and
// the two .rdata float constants the projection compares/biases against.
DATA(0x002c4490)

// Rez heap for the buffer grow (reloc-masked). Param unified to i32 across the
// merged sections (extern "C" cannot overload; 32-bit push is width-neutral).
extern "C" void* RezAlloc(i32 n); // 0x1b9b46
extern "C" void RezFree(void* p); // 0x1b9b82

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
i32 CFaderMesh::ApplyInit(CFaderInit* descOpaque) {
    // The arg is the CFxMode transition descriptor; m_58 is this fader's growable
    // mesh buffer. Both are typed views onto the real (opaque) shapes.
    FxTransDesc* cfg = (FxTransDesc*)descOpaque;
    FxMeshBuffer* mesh = (FxMeshBuffer*)&m_58;

    m_3c = cfg->m_srcBox ? (i32)cfg->m_srcBox : m_timerA;
    m_38 = cfg->m_dstBox ? (i32)cfg->m_dstBox : m_timerB;
    if (cfg->m_10 == 0) {
        return 0;
    }
    m_40 = cfg->m_0c;
    m_44 = cfg->m_10;
    m_48 = cfg->m_18;
    m_4c = cfg->m_recordOrder;
    m_50 = cfg->m_columns;
    m_54 = cfg->m_rows;

    ((CArrayE40*)mesh)->SetSize(0, -1);

    i32 halfW = ((FxBox*)m_3c)->m_width / 2;
    i32 halfH = ((FxBox*)m_3c)->m_height / 2;
    i32 dx = ((FxBox*)m_38)->m_height / m_50;
    i32 dy = ((FxBox*)m_38)->m_width / m_54;
    float radius = (float)sqrt((double)(dx * dx + dy * dy));
    if (m_54 <= 0) {
        return 1;
    }

    for (i32 row = 0; row < m_54; row++) {
        i32 cellW2 = halfW * halfW;
        i32 cellD = halfW * halfW + halfH * halfH;
        float cellR = (float)sqrt((double)cellD) + radius - g_fxBias;
        if (m_50 <= 0) {
            continue;
        }
        for (i32 col = 0; col < m_50; col++) {
            i32 d2 = halfH * halfH + cellW2;
            float v = (float)sqrt((double)d2);
            float normX, normY;
            if (v > g_fxEps) {
                normY = (float)(row - halfH) / v;
                normX = (float)(col - halfW) / v;
            } else {
                normY = 0.0f;
                normX = 1.0f;
            }

            RECT pt48;
            pt48.left = 0;
            pt48.top = 0;
            pt48.right = dx;
            pt48.bottom = dy;
            ::OffsetRect(&pt48, row, col);
            i32 ox = (i32)(cellR * normX);
            i32 oy = (i32)(cellR * normY);
            ::OffsetRect(&pt48, oy, ox);

            RECT pt64;
            pt64.left = 0;
            pt64.top = 0;
            pt64.right = d2;
            pt64.bottom = dy;
            ::OffsetRect(&pt64, row, col);

            FxPoint pt;
            if (m_4c) {
                pt.v[0] = pt64.right;
                pt.v[1] = pt64.bottom;
                pt.v[2] = pt64.left;
                pt.v[3] = pt64.top;
                pt.v[4] = pt48.left;
                pt.v[5] = pt48.top;
                pt.v[6] = pt48.right;
                pt.v[7] = pt48.bottom;
            } else {
                pt.v[0] = pt48.left;
                pt.v[1] = pt48.top;
                pt.v[2] = pt48.right;
                pt.v[3] = pt48.bottom;
                pt.v[4] = pt64.right;
                pt.v[5] = pt64.bottom;
                pt.v[6] = pt64.left;
                pt.v[7] = pt64.top;
            }
            pt.v[8] = 0;
            pt.v[9] = 0x3f800000;

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
                mesh->m_pData = (FxPoint*)RezAlloc(newSize * sizeof(FxPoint));
                memset(mesh->m_pData, 0, newSize * sizeof(FxPoint));
                mesh->m_nMaxSize = newSize;
                mesh->m_nSize = newSize;
            } else if (newSize <= mesh->m_nMaxSize) {
                if (newSize > idx) {
                    memset(&mesh->m_pData[idx], 0, (newSize - idx) * sizeof(FxPoint));
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
                FxPoint* nd = (FxPoint*)RezAlloc(newMax * sizeof(FxPoint));
                memcpy(nd, mesh->m_pData, mesh->m_nSize * sizeof(FxPoint));
                memset(&nd[mesh->m_nSize], 0, (newSize - mesh->m_nSize) * sizeof(FxPoint));
                RezFree(mesh->m_pData);
                mesh->m_pData = nd;
                mesh->m_nSize = newSize;
                mesh->m_nMaxSize = newMax;
            }
            mesh->m_pData[idx] = pt;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CArrayE40::SetSize (0x17f390) - the out-of-line MFC CArray<40-byte>::SetSize for
// CFader17e940's 40-byte-element mesh array (ApplyInit @0x17ea00 calls it at
// 0x17ea79). Re-homed here from the ArrayE40.cpp fragment: the owner is CFader17e940
// (sema xref: SetSize's only caller is CFader17e940::ApplyInit). The element is POD
// (ConstructElements = zero-fill), so grow/shrink inline memset/memcpy around the
// engine operator new/delete (RezAlloc 0x1b9b46 / RezFree 0x1b9b82).
//
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md, topic:wall
// topic:regalloc): ~96%, every operation/offset/immediate/branch + the store
// ordering (m_nSize=m_nMaxSize=0) is byte-faithful; the sole residual is the long-
// lived "0"/null register choice - retail pins it in esi (then reloads esi as the
// memcpy src), cl pins it in edi, which cascades into the edx/ecx scratch picks in
// the realloc lea chains. Not source-steerable. Deferred to the final sweep.
RVA(0x0017f390, 0x164)
void CArrayE40::SetSize(i32 nNewSize, i32 nGrowBy) {
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
        m_pData = (E40*)RezAlloc(nNewSize * sizeof(E40));
        memset(m_pData, 0, nNewSize * sizeof(E40));
        m_nSize = m_nMaxSize = nNewSize;
    } else if (nNewSize <= m_nMaxSize) {
        if (nNewSize > m_nSize) {
            memset(&m_pData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(E40));
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
        E40* pNewData = (E40*)RezAlloc(nNewMax * sizeof(E40));
        memcpy(pNewData, m_pData, m_nSize * sizeof(E40));
        memset(&pNewData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(E40));
        RezFree(m_pData);
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }
}

SIZE_UNKNOWN(CArrayE40);
SIZE_UNKNOWN(E40);

SIZE_UNKNOWN(FxBox);
SIZE_UNKNOWN(FxTransDesc);
SIZE_UNKNOWN(FxPoint);
SIZE_UNKNOWN(FxMeshBuffer);

// ============================================================================
// section: FaderElemSetup.cpp
// ============================================================================
// FaderElemSetup.cpp - CFaderElem::Apply (0x17f5e0), a fader-element setup method
// in the CFaderMgr family (caller CFaderMgr::Add 0x17d9c0).  Copies a config
// source's fields into the element and allocates its per-frame work array.

extern "C" void* RezAlloc(i32 n); // 0x1b9b46

// FaderSrc is now the canonical <Gruntz/FaderSubtypes.h> struct (frameCount @+0x18).

struct FaderArg {
    char pad00[4];
    i32 m_04;        // +0x04
    FaderSrc* m_src; // +0x08
    i32 m_0c;        // +0x0c
    i32 m_10;        // +0x10
    i32 m_14;        // +0x14
};

struct CFaderElem {
    char pad00[0x20];
    i32 m_20;               // +0x20
    i32 m_24;               // +0x24
    FaderSrc* m_defaultSrc; // +0x28
    char pad2c[0x38 - 0x2c];
    i32 m_38;        // +0x38
    FaderSrc* m_src; // +0x3c
    i32 m_40;        // +0x40
    i32 m_44;        // +0x44
    i32 m_48;        // +0x48
    i32* m_frames;   // +0x4c per-frame array

    i32 Apply(FaderArg* s); // 0x17f5e0
};

// @early-stop
// regalloc/scheduling tie (~90%): logic byte-exact; retail's ecx/edx assignment
// for the m_src reload + s->m_10 store schedule differs from this cl's.
RVA(0x0017f5e0, 0x7d)
i32 CFaderElem::Apply(FaderArg* s) {
    i32 a = s->m_04;
    if (!a) {
        a = m_24;
    }
    m_38 = a;
    if (s->m_src) {
        m_src = s->m_src;
    } else {
        m_src = m_defaultSrc;
    }
    m_40 = s->m_0c;
    m_44 = s->m_10;
    m_20 = 0;
    m_48 = s->m_14;
    m_frames = (i32*)RezAlloc(m_src->m_frameCount << 2);
    for (i32 i = 0; i < m_src->m_frameCount; i++) {
        m_frames[i] = 0;
    }
    return 1;
}

// 0x17f950 - CFaderFlat::v2 (vtable slot 2): the flat fader's total duration - the
// source frame count scaled up by m_percent% (signed /100 via the 0x51eb851f
// reciprocal-multiply idiom), plus the base frame count.
RVA(0x0017f950, 0x24)
i32 CFaderFlat::v2() {
    i32 n = m_src->m_frameCount;
    return n + (m_percent * n) / 100;
}

SIZE_UNKNOWN(FaderArg);
SIZE_UNKNOWN(CFaderElem);

// ============================================================================
// section: FaderRadial.cpp
// ============================================================================
// FaderRadial.cpp - the radial distance-field fader init (0x17fa40). __thiscall.
// Resolves the source surface + palette from the config arg, allocates a
// width*height*16 cell buffer, computes the center + max radius, then fills each
// cell with sqrt(dx^2+dy^2)-derived fade values (sampling the source pixel
// through the surface's GetRowBase + post-plot notifier when in bounds). The
// embedded radial math is heavy x87 (fild/fsqrt + the constant pool at
// 0x5f0828/30/38/40). Field names are placeholders; offsets + code bytes are the
// load-bearing fact.

extern "C" int __ftol(double v); // 0x11f570 (CRT double->long, x87 fstcw/fldcw)

// The radial-fade FP constants (retail .rdata): fade divisor half, the sqrt->units
// scale, and the two -1.0 bias subtractions the per-cell fade formula applies.
DATA(0x001f0828)
extern const float g_faderHalf; // 0.5
DATA(0x001f0830)
extern const double g_faderScale; // 10000.0
DATA(0x001f0838)
extern const double g_faderBiasR; // -1.0  (r - K == r + 1.0)
DATA(0x001f0840)
extern const float g_faderBiasFade; // -1.0  (fade - K == fade + 1.0)
DATA(0x001f0844)
extern const float g_faderOne; // 1.0  (per-cell render threshold: fade - frame > 1.0)

// The image-source descriptor reached via FrConfig::m_imageSrc: dimension/handle
// at +0x0c that seeds BuildSurface.
struct FrImageSrc {
    char p0[0xc];
    i32 m_0c; // +0x0c  surface dimension/handle
};

// The source config arg.
struct FrConfig {
    char p0[0x4];
    void* m_paletteArg;       // +0x04
    CDDSurface* m_surfaceArg; // +0x08  (was the FrSurface view; real CDDSurface)
    char p0c[0x10 - 0xc];
    FrImageSrc* m_imageSrc;  // +0x10  (-> m_0c surface)
    void* m_providedSurface; // +0x14
};

// The 16-byte fade cell.
struct FrCell {
    i32 vx, vy, fade, pixel;
};

// Flat body-view of CFaderRadial::ApplyInit (0x17fa40): models the whole 0x5c-byte
// object flat because Build reads CFader base-region slots (m_table/m_timerA/m_timerB/
// m_flag @+0x1c/+0x24/+0x28/+0x30) repurposed as surface/palette/owns-flag. Folding
// into CFaderRadial (: public CFader) needs those base slots reinterpreted, not a
// pure rename - left as a subordinate view; fold is a follow-up matcher.
struct CFaderRadialApply {
    i32 Build(FrConfig* cfg);                // 0x17fa40
    void* BuildSurface(i32 a, i32 b, i32 c); // 0x14e830  (this+0x4 helper)
    char p0[0x1c];
    void* m_workSurface; // +0x1c  resolved surface handle
    char p20[0x24 - 0x20];
    void* m_defaultPalette;       // +0x24  default palette
    CDDSurface* m_defaultSurface; // +0x28  default surface (real CDDSurface)
    char p2c[0x30 - 0x2c];
    i32 m_ownsSurface; // +0x30  owns-surface flag
    char p34[0x38 - 0x34];
    CDDSurface* m_srcSurface; // +0x38  source surface (real CDDSurface)
    void* m_palette;          // +0x3c  resolved palette
    char p40[0x44 - 0x40];
    i32 m_maxRadius; // +0x44  max-radius scale
    char p48[0x4c - 0x48];
    float m_fadeDivisor; // +0x4c  radius->fade divisor
    FrCell* m_cells;     // +0x50  cell buffer
    i32 m_centerX;       // +0x54  center x
    i32 m_centerY;       // +0x58  center y
};

// @early-stop
// Re-reconstructed 61.64%->73.70% by fixing three structural bugs the prior model
// carried (it was NOT an x87 wall): (1) m_maxRadius was `ftol(cx^2+cy^2)` - the real
// formula is `ftol(sqrt(cx^2+cy^2) * 10000)`; (2) the per-cell dy was `0-centerY`
// (the y term dropped) - it is `y - centerY`; (3) the pixel read used m_pixels(=the
// m_8 COM surface) as the byte base and a phantom GetRowBase(row) - the real code
// Lock()s the surface (0x13e6d0 returns the locked lpSurface base) then reads
// `base[colStride*x + pitch*y]` and UnlockThunk()s. The FrSurface view is now the
// real CDDSurface; the manual m_8->vtbl[0x80] unlock is the real UnlockThunk. The
// K constants (0.5/10000/-1.0/-1.0) are the reloc-masked .rdata doubles/floats.
// Residual is a callee-saved regalloc coloring (retail pins this->esi/zero->ebp;
// our cl picks edi/ebx) + the x87 temp-slot frame (sub esp,0x18 vs 0xc) - genuine
// MSVC5 scheduling, not logic. Accepted per the cleanup-over-% mandate.
RVA(0x0017fa40, 0x1f3)
i32 CFaderRadialApply::Build(FrConfig* cfg) {
    CFaderRadialApply* self = this;
    if (cfg->m_paletteArg == 0) {
        self->m_palette = self->m_defaultPalette;
    } else {
        self->m_palette = cfg->m_paletteArg;
    }

    if (cfg->m_surfaceArg == 0) {
        self->m_srcSurface = self->m_defaultSurface;
    } else {
        self->m_srcSurface = cfg->m_surfaceArg;
    }

    if (cfg->m_providedSurface == 0) {
        self->m_workSurface = self->BuildSurface(cfg->m_imageSrc->m_0c, 0x10, 0);
        self->m_ownsSurface = 1;
    } else {
        self->m_workSurface = cfg->m_providedSurface;
        self->m_ownsSurface = 0;
    }
    if (self->m_workSurface == 0) {
        return 0;
    }

    // The source surface is the real CDDSurface: width @+0x1c, height @+0x18, pitch
    // @+0x20, column stride @+0xb0, held IDirectDrawSurface @m_8. Lock() (0x13e6d0)
    // returns the locked pixel base; UnlockThunk() is the m_8->vtbl[0x80] COM unlock.
    CDDSurface* s = self->m_srcSurface;
    self->m_fadeDivisor = (float)s->m_width * g_faderHalf; // width * 0.5
    self->m_centerX = s->m_width / 2;
    self->m_centerY = s->m_height / 2;
    self->m_cells = (FrCell*)::operator new(s->m_height * s->m_width * 16);

    i32 cx = self->m_centerX;
    i32 cy = self->m_centerY;
    self->m_maxRadius = (i32)(sqrt((double)(cx * cx + cy * cy)) * g_faderScale);

    for (i32 y = 0; y < self->m_srcSurface->m_height; y++) {
        for (i32 x = 0; x < self->m_srcSurface->m_width; x++) {
            i32 dx = x - self->m_centerX;
            i32 dy = y - self->m_centerY;
            float r = (float)((double)self->m_maxRadius
                              - sqrt((double)(dx * dx + dy * dy)) * g_faderScale - g_faderBiasR);
            float fade = r / self->m_fadeDivisor - g_faderBiasFade;
            i32 vx = (i32)((float)dx * fade);
            i32 vy = (i32)((float)dy * fade);
            u8 pix;
            i32 base = self->m_srcSurface->Lock(0);
            if (base != 0) {
                pix = *(u8*)(base + self->m_srcSurface->m_b0 * x + self->m_srcSurface->m_pitch * y);
                self->m_srcSurface->UnlockThunk();
            } else {
                pix = 0;
            }
            FrCell* cell = &self->m_cells[y * self->m_srcSurface->m_width + x];
            cell->vx = vx;
            cell->vy = vy;
            cell->fade = (i32)fade;
            cell->pixel = pix;
        }
    }
    return 1;
}

// ===========================================================================
// 0x17fc60 - CFaderRadial::v1(frame) (the vtable slot-1 render, hosted on the
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
// the sibling Build @0x17fa40): the per-cell (fade-frame)/divisor + vx/sf, vy/sf
// __ftol chain, the fcoms threshold and the bounds test + plot are byte-faithful, but
// cl schedules the frame-float on the x87 stack and colours edi/ebx/ebp differently
// than retail (which pins this->esi/cellptr->ebx/index->ebp). The cells hold FLOAT
// vx/vy/fade (Build's (i32) casts are the mismatch that parks Build); read here as
// floats. Not source-steerable. Defined as CFaderRadial::v1 (not a second
// CFaderRadialApply method) so the delinker doesn't pack it with Build @0x17fa40.
RVA(0x0017fc60, 0x136)
void CFaderRadial::v1(i32 frame) {
    CFaderRadialApply* self = (CFaderRadialApply*)this;
    CDDSurface* dst = (CDDSurface*)self->m_palette; // +0x3c (dest surface in the v1 path)
    void* scratch = RezAlloc(dst->m_width);         // per-width scratch (alloc'd, unused)
    dst->Clear(0);
    self->m_srcSurface->Lock(0);                        // lock source (base unused here)
    i32 base = dst->Lock(0);                            // locked dest pixel base
    if (((CDDSurface*)self->m_workSurface)->m_8 == 0) { // gate: work-surface COM present?
        return;                                         // retail bails w/o unlock/free (matched)
    }

    i32 total = self->m_srcSurface->m_width * self->m_srcSurface->m_height;
    float ff = (float)frame;
    for (i32 i = 0; i < total; i++) {
        float* c = (float*)&self->m_cells[i]; // vx=c[0], vy=c[1], fade=c[2]
        float d = c[2] - ff;
        if (d > g_faderOne) {
            float sf = d / self->m_fadeDivisor - g_faderBiasFade;
            i32 px = self->m_centerX + (i32)(c[0] / sf);
            i32 py = self->m_centerY - (i32)(c[1] / sf);
            if (px > 0 && px < dst->m_width && py > 0 && py < dst->m_height) {
                ((u8*)base)[py * dst->m_pitch + px] = ((u8*)&self->m_cells[i])[0xc];
            }
        }
    }

    // Inlined UnlockThunk: m_8->vtbl[0x80](m_8, 0) on both surfaces (no ddraw.h needed
    // in this unit; forward-declared IDirectDrawSurface* dispatched by slot).
    void* s8 = self->m_srcSurface->m_8;
    (*(void(__stdcall**)(void*, i32))(*(void***)s8 + 0x20))(s8, 0);
    void* d8 = dst->m_8;
    (*(void(__stdcall**)(void*, i32))(*(void***)d8 + 0x20))(d8, 0);
    RezFree(scratch);
}

SIZE_UNKNOWN(FrImageSrc);
SIZE_UNKNOWN(FrConfig);
SIZE_UNKNOWN(FrCell);
SIZE_UNKNOWN(CFaderRadialApply);

// ============================================================================
// section: Obj5f0890Dtor.cpp
// ============================================================================
// Obj5f0890Dtor.cpp - 0x181720, CFaderShape::~CFaderShape: the virtual destructor
// of the screen-fader subtype whose vftable is at 0x5f0890 (== ??_7CFaderShape@@6B@,
// confirmed by the entry vptr-stamp reloc). Stamp-first vptr, then free the six owned
// heap buffers (m_478, m_44, m_48, m_4c, m_488, m_48c) with operator delete, then chain
// to the CFader base destructor at 0x17e4a0. The CFader base subobject's non-trivial
// virtual dtor gives the /GX unwind frame (state 0 -> -1 around the base-dtor call). The
// implicit vptr stamp reloc-masks against the retail 0x5f0890 vtable; operator delete and
// the base dtor are reloc-masked externs.
//
// (Was a standalone placeholder CObj5f0890 : CObj5f0890Base; folded onto the real
// CFaderShape - identical field layout, and CObj5f0890Base was CFader (both base dtor
// 0x17e4a0). The dtor now carries the correct ??_7CFaderShape name.)

void __cdecl operator delete(void* p); // ??3@YAXPAX@Z (0x1b9b82)

// ---------------------------------------------------------------------------
RVA(0x00181720, 0xb3)
CFaderShape::~CFaderShape() {
    if (m_478) {
        operator delete((void*)m_478);
    }
    if (m_44) {
        operator delete((void*)m_44);
    }
    if (m_48) {
        operator delete((void*)m_48);
    }
    if (m_4c) {
        operator delete((void*)m_4c);
    }
    if (m_488) {
        operator delete((void*)m_488);
    }
    if (m_48c) {
        operator delete((void*)m_48c);
    }
}

// ============================================================================
// section: FaderShapeSetup.cpp
// ============================================================================
// FaderShapeSetup.cpp - a CFaderMgr-family screen-fader subclass init (0x1817e0).
// __thiscall, called by CFaderMgr::Add (0x17d9c0). Resolves three equal-sized
// source surfaces (m_surfA/m_surfB/m_surfC) from the config arg (defaulting to the fader's
// own m_defaultSurfA/m_defaultSurfB), validates their dimensions match, builds an acos warp table
// (m_warpTable) and a sin highlight ramp (m_highlightRamp), resolves the shade table via the
// embedded CShadeTableCache (file/array/flash), then allocates per-column pitch
// tables (m_pitchA/m_pitchB/m_pitchC) and a scratch line (m_scratchLine). Field names are placeholders;
// offsets + code bytes are the load-bearing fact.

extern "C" void* RezAlloc(i32 n);                   // 0x1b9b46
extern "C" int _access(const char* path, int mode); // 0x193900 CRT

// The 0x10-byte shade-table buffer the cache builders return (full def in
// <DDrawMgr/ShadeTableCache.h>); referenced only by pointer here.
struct CShadeTable;

// The embedded shade-table cache subobject (CFader base, at this+0x04).

// A source surface: dims at +0x18/+0x1c, row pitch +0x20, format gate +0xa8,
// column stride +0xb0.
struct FShadeSurf {
    char p00[0x18];
    i32 m_width;  // +0x18 width
    i32 m_height; // +0x1c height
    i32 m_pitch;  // +0x20 row pitch
    char p24[0xa8 - 0x24];
    i32 m_format; // +0xa8 format
    char pac[0xb0 - 0xac];
    i32 m_colStride; // +0xb0 column stride
};

// The +0x28 config arg's palette holder.
struct FInitPal {
    char p00[0xc];
    void* m_palBase; // +0x0c palette base
};

// The config arg (CFaderMgr::Add's pInit).
struct FInit {
    char p00[0x4];
    FShadeSurf* m_surfA;          // +0x04 surface A override
    FShadeSurf* m_surfB;          // +0x08 surface B override
    FShadeSurf* m_surfC;          // +0x0c surface C override
    i32 m_rampSize;               // +0x10 ramp size
    i32 m_mode;                   // +0x14 mode (1..3)
    i32 m_18;                     // +0x18
    i32 m_gate;                   // +0x1c gate
    CShadeTable* m_prebuiltTable; // +0x20 prebuilt shade table
    char* m_tableName;            // +0x24 table file/array name
    FInitPal* m_flashPal;         // +0x28 flash palette source
};

// Flat body-view of CFaderShape::ApplyInit (0x1817e0): models the whole 0x494-byte
// object flat (Setup reuses the CFader base-region slots). Fold into CFaderShape
// (: public CFader) is a follow-up matcher; kept as a subordinate view for now.
struct CFaderShapeApply {
    char p00[0x4];
    CShadeTableCache m_cache;   // +0x04 cache subobject (0x18)
    CShadeTable* m_shadeTable;  // +0x1c resolved shade table
    i32 m_20;                   // +0x20
    FShadeSurf* m_defaultSurfA; // +0x24 default surface A
    FShadeSurf* m_defaultSurfB; // +0x28 default surface B
    char p2c[0x30 - 0x2c];
    i32 m_30; // +0x30
    char p34[0x38 - 0x34];
    FShadeSurf* m_surfA; // +0x38 surface A
    FShadeSurf* m_surfB; // +0x3c surface B
    FShadeSurf* m_surfC; // +0x40 surface C
    i32* m_pitchA;       // +0x44 A column pitch table
    i32* m_pitchB;       // +0x48 B column pitch table
    i32* m_pitchC;       // +0x4c C column pitch table
    i32 m_mode;          // +0x50 mode
    i32 m_54;            // +0x54
    i32 m_rampSize;      // +0x58 ramp size
    i32 m_active;        // +0x5c active flag
    i32 m_heightA;       // +0x60 A height
    i32 m_widthA;        // +0x64 A width
    i32 m_heightB;       // +0x68 B height
    i32 m_widthB;        // +0x6c B width
    i32 m_heightC;       // +0x70 C height
    i32 m_widthC;        // +0x74 C width
    char p78[0x478 - 0x78];
    i32* m_warpTable; // +0x478 acos warp table
    char p47c[0x488 - 0x47c];
    void* m_scratchLine; // +0x488 scratch line
    u8* m_highlightRamp; // +0x48c sin highlight ramp

    i32 Setup(FInit* pInit); // 0x1817e0
};

// @early-stop
// x87 scheduling wall (~40-50%): the surface resolution, the equal-dimension
// validation, the per-column pitch tables (m_pitchA/m_pitchB/m_pitchC) and the m_scratchLine scratch
// allocation match, but the two transcendental ramp loops are not source-steerable
// - retail's fild/fxch/fstp stack-slot reuse over `acos((i-r)/r)*r` (m_warpTable) and
// `0x10 - sin(i/r*PI)*-32` (m_highlightRamp), plus the MFC CString temp the file-name
// AddFromArray path builds, diverge. Same family as CFaderRadial::Build (0x17fa40).
RVA(0x001817e0, 0x315)
i32 CFaderShapeApply::Setup(FInit* pInit) {
    i32 i;
    this->m_20 = 0;
    if (pInit == 0) {
        return 0;
    }

    this->m_surfA = pInit->m_surfA ? pInit->m_surfA : this->m_defaultSurfA;
    this->m_surfB = pInit->m_surfB ? pInit->m_surfB : this->m_defaultSurfB;
    if (this->m_surfA == 0) {
        return 0;
    }
    if (this->m_surfB == 0) {
        return 0;
    }
    this->m_surfC = pInit->m_surfC ? pInit->m_surfC : this->m_surfB;

    if (!this->m_cache.Init()) {
        return 0;
    }

    this->m_heightA = this->m_surfA->m_height;
    this->m_widthA = this->m_surfA->m_width;
    this->m_heightB = this->m_surfB->m_height;
    this->m_widthB = this->m_surfB->m_width;
    this->m_heightC = this->m_surfC->m_height;
    this->m_widthC = this->m_surfC->m_width;
    if (this->m_heightA != this->m_heightB) {
        return 0;
    }
    if (this->m_widthA != this->m_widthB) {
        return 0;
    }
    if (this->m_heightA != this->m_heightC) {
        return 0;
    }
    if (this->m_widthA != this->m_widthC) {
        return 0;
    }
    if (this->m_heightC != this->m_heightB) {
        return 0;
    }
    if (this->m_widthC != this->m_widthB) {
        return 0;
    }

    if (pInit->m_mode == 0) {
        return 0;
    }
    if ((u32)pInit->m_mode >= 4) {
        return 0;
    }
    this->m_mode = pInit->m_mode;
    this->m_54 = pInit->m_18;
    this->m_rampSize = pInit->m_rampSize;

    if (this->m_mode == 1 || this->m_mode == 2) {
        if (this->m_heightA < (i32)((double)this->m_rampSize * 3.141592653589793)) {
            return 0;
        }
    }

    this->m_warpTable = (i32*)RezAlloc(this->m_rampSize * 8);
    for (i = 0; i < 2 * this->m_rampSize; i++) {
        this->m_warpTable[i] =
            (i32)(acos(((float)i - (float)this->m_rampSize) / (float)this->m_rampSize)
                  * (float)this->m_rampSize);
    }

    this->m_active = pInit->m_gate;
    if (this->m_surfA->m_format != 8) {
        this->m_active = 0;
    }

    if (this->m_active != 0) {
        if (pInit->m_prebuiltTable) {
            this->m_30 = 0;
            this->m_shadeTable = pInit->m_prebuiltTable;
        } else if (_access(pInit->m_tableName, 0) == 0) {
            this->m_shadeTable = this->m_cache.AddFromArray(pInit->m_tableName);
            if (this->m_shadeTable == 0) {
                this->m_active = 0;
            }
        } else {
            this->m_shadeTable =
                this->m_cache
                    .FlashTable((PalEntry*)pInit->m_flashPal->m_palBase, 0x20, 0x20, 0x32, 0xc8);
        }

        i32 m = this->m_rampSize << 1;
        this->m_highlightRamp = (u8*)RezAlloc(m);
        for (i = 0; i < m; i++) {
            i32 t = (i32)(sin((float)i / (float)this->m_rampSize * 3.14f) * -32.0);
            this->m_highlightRamp[i] = (u8)(0x10 - t);
        }
    }

    this->m_pitchA = (i32*)RezAlloc(this->m_widthA * 4);
    this->m_pitchB = (i32*)RezAlloc(this->m_widthB * 4);
    this->m_pitchC = (i32*)RezAlloc(this->m_widthC * 4);
    for (i = 0; i < this->m_widthA; i++) {
        this->m_pitchA[i] = this->m_surfA->m_pitch * i;
        this->m_pitchB[i] = this->m_surfB->m_pitch * i;
        this->m_pitchC[i] = this->m_surfC->m_pitch * i;
    }

    i32 mx = (this->m_widthA > this->m_heightA) ? this->m_widthA : this->m_heightA;
    this->m_scratchLine = RezAlloc(this->m_surfA->m_colStride * mx);
    return 1;
}

SIZE_UNKNOWN(CShadeTableCache);
SIZE_UNKNOWN(FShadeSurf);
SIZE_UNKNOWN(FInitPal);
SIZE_UNKNOWN(FInit);
SIZE_UNKNOWN(CFaderShapeApply);

// ============================================================================
// section: FaderTileRender.cpp
// ============================================================================
// FaderTileRender.cpp - the per-column tile gather/remap scanline compositor of
// the big CFader subtype (the 0x7d5c-byte fader allocated by CFaderMgr::Add case
// 2; its Apply path 0x1817e0 calls this). __thiscall, two args (arg0 = base
// pixel row, arg1 = a leading X offset). For each of m_colCount columns it gathers a
// (2*m_halfWidth)-pixel source line into the scratch line m_lineBuf - either straight
// (per-bpp 1/2/3) or through a 64-wide 2D LUT (m_lut->table, keyed by the gathered
// byte and the per-pixel selector m_shadeIndices) when m_useLut is set - then optionally
// copies/zeros a destination strip (gated on m_stripCopy) and finally writes the scratch
// line back into the destination buffer. The gather source (m_gatherBase, tapped
// via m_tapTable) and the straight/edge-strip source (m_straightBase) are read; the
// assembled line is written to m_dstBase - all addressed in pixel*bpp units. The
// per-column row offsets come from m_gatherRowOffsets / m_straightRowOffsets /
// m_dstRowOffsets respectively. Offsets + code bytes are load-bearing.

// m_surf's type: a surface descriptor whose +0xb0 is the bytes-per-pixel (1/2/3).
struct TileSurf {
    char pad[0xb0];
    i32 bytesPerPixel; // bytes per pixel
};
SIZE_UNKNOWN(TileSurf);

// m_lut's type: a palette/LUT descriptor whose +0x8 is the 2D remap table base.
struct TileLut {
    char pad[0x8];
    u8* table;
};
SIZE_UNKNOWN(TileLut);

class CFaderTileRender {
public:
    void RenderTile(i32 arg0, i32 arg1);     // 0x182610
    void RenderWarpTile(i32 arg0, i32 arg1); // 0x181e50

    char m_00[0x1c];
    TileLut* m_lut; // +0x1c remap-table descriptor
    char m_20[0x38 - 0x20];
    TileSurf* m_surf; // +0x38 surface (->bytesPerPixel = bytes/pixel)
    char m_3c[0x44 - 0x3c];
    i32* m_dstRowOffsets;      // +0x44 per-column write-back row offset (into m_dstBase)
    i32* m_straightRowOffsets; // +0x48 per-column straight/strip-source row offset
    i32* m_gatherRowOffsets;   // +0x4c per-column gather-source row offset (into m_gatherBase)
    i32 m_placement;           // +0x50 placement mode (1 = leading edge, 2 = trailing edge)
    i32 m_stripCopy;           // +0x54 edge strip: nonzero = copy underlying pixels, 0 = zero-fill
    i32 m_halfWidth; // +0x58 half line-width (line is 2*m_halfWidth px; arc = PI*m_halfWidth)
    i32 m_useLut;    // +0x5c shade-LUT gather flag
    i32 m_span;      // +0x60 total wrap span (used by the PI-scaled warp)
    i32 m_colCount;  // +0x64 column count
    char m_68[0x478 - 0x68];
    i32* m_tapTable;    // +0x478 per-pixel source-tap table (pixel indices into the gather source)
    u8* m_dstBase;      // +0x47c write-back destination line base
    u8* m_straightBase; // +0x480 straight-copy / edge-strip source base
    u8* m_gatherBase;   // +0x484 tap-sampled gather source base
    u8* m_lineBuf;      // +0x488 scratch line assembled before write-back
    u8* m_shadeIndices; // +0x48c per-pixel shade selector (low 6 bits of the LUT index)
};
SIZE(CFaderTileRender, 0x7d5c);

// ===========================================================================
// 0x182610 - RenderTile: assemble + write back one (2*m_halfWidth)-wide line per column.
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
void CFaderTileRender::RenderTile(i32 arg0, i32 arg1) {
    if (arg1 <= 0) {
        return;
    }
    i32 stride = m_halfWidth * 2; // inner pixel count
    i32 rowBytes = stride + arg1;
    i32 bpp = m_surf->bytesPerPixel;

    i32 x0;
    u8* src2base;
    u8* destBase;
    if (m_placement == 1) {
        src2base = m_lineBuf;
        x0 = arg1;
        destBase = m_straightBase + (arg0 - arg1) * bpp;
    } else if (m_placement == 2) {
        src2base = m_lineBuf + bpp * stride;
        x0 = 0;
        destBase = m_straightBase + (arg0 + stride) * bpp;
    } else {
        return;
    }

    u8* srcA = m_dstBase + (arg0 - x0) * bpp;
    u8* srcB = m_gatherBase + (arg0 - x0) * bpp;
    if (m_colCount <= 0) {
        return;
    }

    for (i32 j = 0; j < m_colCount; j++) {
        u8* rowSrcA = srcA + m_dstRowOffsets[j];
        u8* rowSrcB = srcB + m_gatherRowOffsets[j];

        if (m_useLut) {
            u8* lut = m_lut->table;
            for (i32 k = 0; k < stride; k++) {
                u8 b = rowSrcB[m_tapTable[k]];
                m_lineBuf[x0 + k] = lut[(b << 6) + m_shadeIndices[k]];
            }
        } else if (bpp == 1) {
            for (i32 k = 0; k < stride; k++) {
                m_lineBuf[x0 + k] = rowSrcB[m_tapTable[k]];
            }
        } else if (bpp == 2) {
            for (i32 k = 0; k < stride; k++) {
                u8* s = rowSrcB + m_tapTable[k] * 2;
                u8* d = m_lineBuf + (x0 + k) * 2;
                d[0] = s[0];
                d[1] = s[1];
            }
        } else if (bpp == 3) {
            for (i32 k = 0; k < stride; k++) {
                u8* s = rowSrcB + m_tapTable[k] * 3;
                u8* d = m_lineBuf + (x0 + k) * 3;
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
            }
        }

        if (m_stripCopy) {
            u8* s = destBase + m_straightRowOffsets[j];
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
// each of m_colCount columns gathers a (2*m_halfWidth) line (straight bytes + the m_tapTable-tapped
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
void CFaderTileRender::RenderWarpTile(i32 arg0, i32 arg1) {
    i32 stride = m_halfWidth * 2;
    if (arg1 <= 0) {
        return;
    }
    i32 arc = (i32)((double)m_halfWidth * 3.14159);
    i32 bpp = m_surf->bytesPerPixel;

    i32 colBase;
    if ((m_placement == 1 && m_stripCopy != 0) || (m_placement == 2 && m_stripCopy == 0)) {
        colBase = stride - (i32)((double)stride / (arc - m_halfWidth) * (m_span - arg0 - stride));
    } else {
        colBase = arg0;
    }
    if ((m_placement == 1 && m_stripCopy == 0) || (m_placement == 2 && m_stripCopy != 0)) {
        colBase = (i32)((double)stride / (arc - m_halfWidth) * arg0);
    }

    if ((m_placement == 1 && m_stripCopy != 0) || (m_placement == 2 && m_stripCopy == 0)) {
        i32 col = 0;
        if (m_colCount > 0) {
            i32 base = bpp * arg0;
            do {
                u8* dstLine = m_dstRowOffsets[col] + base + m_dstBase;
                u8* gsrc = m_gatherRowOffsets[col] + base + m_gatherBase;
                u8* ssrc = m_straightRowOffsets[col] + base + m_straightBase;
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
                            m_lineBuf[t] = gsrc[m_tapTable[t]];
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
                            m_lineBuf[e * 2 - 2] = gsrc[m_tapTable[t] * 2];
                            m_lineBuf[e * 2 - 1] = gsrc[m_tapTable[t] * 2 + 1];
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
                                m_lineBuf[d] = gsrc[m_tapTable[t] * 3];
                                m_lineBuf[d + 1] = gsrc[m_tapTable[t] * 3 + 1];
                                m_lineBuf[d + 2] = gsrc[m_tapTable[t] * 3 + 2];
                                d += 3;
                            }
                        }
                    }
                } else {
                    u8* lut = m_lut->table;
                    i32 i = 0;
                    i32 t = colBase;
                    if (colBase > 0) {
                        do {
                            m_lineBuf[i] = ssrc[i];
                            i++;
                        } while (i < colBase);
                    }
                    for (; t < stride; t++) {
                        m_lineBuf[t] =
                            lut[(u32)m_shadeIndices[t] + (u32)gsrc[m_tapTable[t]] * 0x40];
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
                    u8* s2 = (arg0 - arg1) * bpp + m_straightRowOffsets[col] + m_straightBase;
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
            } while (col < m_colCount);
        }
    } else if (((m_placement == 1 && m_stripCopy == 0) || (m_placement == 2 && m_stripCopy != 0))
               && m_colCount > 0) {
        i32 col = 0;
        i32 base = bpp * arg0;
        do {
            u8* dstLine = m_dstRowOffsets[col] + base + m_dstBase;
            u8* gsrc = m_gatherRowOffsets[col] + base + m_gatherBase;
            u8* ssrc = m_straightRowOffsets[col] + base + m_straightBase;
            if (m_useLut == 0) {
                if (bpp == 1) {
                    i32 i = 0;
                    i32 t = colBase;
                    i32 e;
                    if (colBase > 0) {
                        do {
                            e = i + 1;
                            m_lineBuf[i] = gsrc[m_tapTable[i]];
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
                            m_lineBuf[i * 2 - 2] = gsrc[m_tapTable[o / 4] * 2];
                            m_lineBuf[i * 2 - 1] = gsrc[m_tapTable[i - 1] * 2 + 1];
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
                            m_lineBuf[d] = gsrc[m_tapTable[k] * 3];
                            m_lineBuf[d + 1] = gsrc[m_tapTable[k] * 3 + 1];
                            m_lineBuf[d + 2] = gsrc[m_tapTable[k] * 3 + 2];
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
                u8* lut = m_lut->table;
                i32 i = 0;
                i32 t = colBase;
                i32 e;
                if (colBase > 0) {
                    do {
                        e = i + 1;
                        m_lineBuf[i] =
                            lut[(u32)m_shadeIndices[i] + (u32)gsrc[m_tapTable[i]] * 0x40];
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
                u8* s2 = (arg0 + stride) * bpp + m_straightRowOffsets[col] + m_straightBase;
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
        } while (col < m_colCount);
    }
}

// 0x182900 - CFaderShape::v2 (vtable slot 2): total frame count for the shape
// transition. Modes 1/2 (box) run m_60-2*m_58 frames; mode 3 (diamond) halves the
// m_60-4*m_58 span. Any other mode is a zero-length (instant) transition.
RVA(0x00182900, 0x35)
i32 CFaderShape::v2() {
    i32 mode = m_50;
    if (mode == 1 || mode == 2) {
        return m_60 - m_58 * 2;
    }
    if (mode == 3) {
        return (m_60 - m_58 * 4) / 2;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Homed from src/Stub/GapFunctions.cpp (matcher-5): three Fader TU leaves, homed
// by RVA neighbourhood (all inside Fader's .text block).
// ---------------------------------------------------------------------------
// @early-stop
// 0x17f660 (742 B) - a Fader worker; homed pending leaf-first reconstruction (>512 B).
RVA(0x0017f660, 0x2e6)
i32 Gap_17f660(void) {
    return 0;
}

// @early-stop
// 0x17ff30 (1218 B) - a large Fader worker; homed pending leaf-first reconstruction (>512 B).
RVA(0x0017ff30, 0x4c2)
i32 Gap_17ff30(void) {
    return 0;
}

// @early-stop
// 0x181b00 (847 B) - a Fader worker; homed pending leaf-first reconstruction (>512 B).
RVA(0x00181b00, 0x34f)
i32 Gap_181b00(void) {
    return 0;
}
