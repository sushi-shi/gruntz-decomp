#include <rva.h>
#include <Wap32/CObject.h> // Wap::CObject - the shared engine grand-base (sub-widget CObject prefix)
// ApiHiCallers.cpp - reconstructed game API-caller methods, the HIGH-RVA half
// (RVA >= 0x0e0000). Each was classified as GAME (not CRT/MFC-library) and
// reconstructed against a best-guess
// class shape: placeholder m_<hexoffset> field names, only the OFFSETS + emitted
// code bytes are load-bearing (campaign doctrine). Plain /O2 /MT frameless leaves
// (no SEH/EH). External engine callees are modeled with NO body so their rel32/
// DIR32 references reloc-mask.
#include <Win32.h>

// The inlined game RNG used by the 0x17fe00 fader-noise init. This is a THIRD
// LCG instance (its own seed-flag + state globals, distinct from the 0x6c127d/
// 0x6c1288 and 0x6c2798 pairs), seeded lazily from timeGetTime.
extern "C" u32(WINAPI* g_pTimeGetTime)(); // 0x6c4650
extern u8 g_fxRandSeeded;                 // 0x6c279c  seed-init flag (bit 0)
extern i32 g_fxRandSeed;                  // 0x6c27a8  LCG seed

static __inline i32 FxRand(i32 range) {
    u32 x;
    if (!(g_fxRandSeeded & 1)) {
        g_fxRandSeeded |= 1;
        x = g_pTimeGetTime();
    } else {
        x = g_fxRandSeed;
    }
    g_fxRandSeed = x * 214013 + 2531011;
    return (((i32)g_fxRandSeed >> 16) & 0x7fff) % range;
}

// ===========================================================================
// 0x0017fe00 (301B) - a fader/noise-effect initializer (proximity: CFaderSine).
// __thiscall(desc): copy geometry out of the descriptor + its source object,
// range-check the intensity (0..100), compute a scaled magnitude via the FP
// pipeline, then fill four parallel 2000-int arrays (three zeroed, one seeded
// with rand()%count) and hand the last one to the tick registrar 0x182940.
// ===========================================================================
extern const float g_faderScale_5f085c;       // 0x5f085c  intensity->magnitude scale
void ScatterSamples(i32* arr, i32, i32, i32); // 0x182940 ?ScatterSamples@@YAXPAHHHH@Z

// @early-stop
// regalloc coin-flip wall (73.5% fuzzy, was 1.96% stub; the goto-fail single exit
// already recovered the shared `return 0` tail). Full body is byte-shape-identical
// to retail; the residual is a callee-saved coloring swap that touches every
// ModRM byte: retail colors this->ebx and the reused const-0/count->edi, while cl
// picks this->edi / const-0->ebx (a symmetric loop-weight tie broken the other
// way). Verified via llvm-objdump -dr base vs target - the only mnemonic-level
// diffs are the ebx/edi register columns.

struct FxDesc_17fe00 {
    char m_pad0[4];
    i32 m_4;  // +0x04
    i32 m_8;  // +0x08
    i32 m_c;  // +0x0c
    i32 m_10; // +0x10
};
struct FxSrc_17fe00 {
    char m_pad0[0x18];
    i32 m_18; // +0x18
    i32 m_1c; // +0x1c
};
struct Fx_17fe00 {
    char m_pad0[0x20];
    i32 m_20;           // +0x20
    FxSrc_17fe00* m_24; // +0x24 fallback source
    FxSrc_17fe00* m_28; // +0x28 fallback source
    char m_pad2c[0x38 - 0x2c];
    FxSrc_17fe00* m_38; // +0x38
    FxSrc_17fe00* m_3c; // +0x3c
    i32 m_40;           // +0x40
    char m_pad44[0x4c - 0x44];
    i32 m_4c;         // +0x4c
    i32 m_50;         // +0x50 element count
    i32 m_54;         // +0x54
    i32 m_58;         // +0x58 intensity
    i32 m_arr0[2000]; // +0x5c
    i32 m_arr1[2000]; // +0x1f9c
    i32 m_arr2[2000]; // +0x3edc
    i32 m_arr3[2000]; // +0x5e1c
    i32 Init(FxDesc_17fe00* desc);
};

RVA(0x0017fe00, 0x12d)
i32 Fx_17fe00::Init(FxDesc_17fe00* desc) {
    i32 w;
    i32 p;
    i32 i;
    m_20 = 0;
    m_40 = desc->m_c;
    FxSrc_17fe00* src = (FxSrc_17fe00*)desc->m_4;
    if (!src) {
        src = m_24;
    }
    m_38 = src;
    i32 alt = desc->m_8;
    if (!alt) {
        alt = (i32)m_28;
    }
    m_3c = (FxSrc_17fe00*)alt;
    if (!m_38) {
        goto fail;
    }
    if (!m_3c) {
        m_40 = 1;
    }
    m_50 = m_38->m_1c;
    w = m_38->m_18;
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
// 0x00168080 (502B) - a compound-widget builder (proximity: CGameLevel/CWwdGrid).
// __thiscall(a1, rc, p3, p4, p5, p6, p7, p8): allocate three same-typed sub-
// widgets (operator new + vtable stamp), lay each out over the RECT *rc with its
// own size-pair, then derive the widget's own extents (min-1 + half-centers) from
// three more size-pairs and finish with a SetRect over rc's corners.
// ===========================================================================
extern "C" int(WINAPI* g_pSetRect_6c44b8)(RECT*, int, int, int, int);

struct Pt_168080 {
    i32 m_0; // +0x00
    i32 m_4; // +0x04
};
// REAL-POLYMORPHIC: the 6-slot sub-widget vtable @0x5f0310 (was g_subVtbl_5f0310)
// is now cl-emitted (??_7SubWidget_168080@@6B@, bound via VTBL below); the INLINE
// ctor AUTO-stamps the vptr, so `new SubWidget_168080` keeps the retail
// `operator new(0x44); if (p) { stamp vptr; m_4=0 }` shape. Slots 0/2/3/4
// (0x1bef01 / 0x0028ec / 0x00106e / 0x004034) come from the Wap::CObject grand-
// base; slot 1 is the class's own 0x168280 scalar dtor, slot 5 the 0x168060 new
// virtual - all declared-only -> reloc-masked.
struct SubWidget_168080 : public Wap::CObject {
    virtual ~SubWidget_168080() OVERRIDE; // [1] +0x04 0x168280 scalar-deleting dtor
    virtual void s14();                   // [5] 0x168060
    i32 m_4;                              // +0x04
    char m_pad8[0x44 - 8];
    i32 Setup(RECT rc, i32 a, i32 b); // 0x1915c0 (reloc-masked)
    SubWidget_168080() {
        m_4 = 0; // cl auto-stamps &??_7SubWidget_168080 first
    }
};
struct Builder_168080 {
    void* m_0;             // +0x00
    SubWidget_168080* m_4; // +0x04
    SubWidget_168080* m_8; // +0x08
    SubWidget_168080* m_c; // +0x0c
    i32 m_10;              // +0x10
    i32 m_14;              // +0x14
    i32 m_18;              // +0x18
    i32 m_1c;              // +0x1c
    i32 m_20;              // +0x20
    i32 m_24;              // +0x24
    i32 m_28;              // +0x28
    i32 m_2c;              // +0x2c
    i32 m_30;              // +0x30
    i32 m_34;              // +0x34
    i32 m_38;              // +0x38
    i32 m_3c;              // +0x3c
    i32 m_40;              // +0x40
    i32 m_44;              // +0x44
    i32 m_48;              // +0x48
    i32 m_4c;              // +0x4c
    i32 m_50;              // +0x50
    i32 m_54;              // +0x54
    RECT m_58;             // +0x58
    i32 m_68;              // +0x68
    i32 m_6c;              // +0x6c
    i32 Init(
        void* a1,
        RECT* rc,
        Pt_168080* p3,
        Pt_168080* p4,
        Pt_168080* p5,
        Pt_168080* p6,
        Pt_168080* p7,
        Pt_168080* p8
    );
};

RVA(0x00168080, 0x1f6)
i32 Builder_168080::Init(
    void* a1,
    RECT* rc,
    Pt_168080* p3,
    Pt_168080* p4,
    Pt_168080* p5,
    Pt_168080* p6,
    Pt_168080* p7,
    Pt_168080* p8
) {
    if (a1) {
        m_4 = new SubWidget_168080;
        m_8 = new SubWidget_168080;
        m_c = new SubWidget_168080;
        if (m_4 && m_8 && m_c && m_4->Setup(*rc, p3->m_0, p3->m_4)
            && m_8->Setup(*rc, p4->m_0, p4->m_4) && m_c->Setup(*rc, p5->m_0, p5->m_4)) {
            m_10 = 0;
            m_14 = 0;
            m_18 = p6->m_0 - 1;
            m_1c = p6->m_4 - 1;
            m_40 = p6->m_0 / 2;
            m_44 = p6->m_4 / 2;
            m_30 = 0;
            m_34 = 0;
            m_38 = p7->m_0 - 1;
            m_3c = p7->m_4 - 1;
            m_48 = p7->m_0 / 2;
            m_4c = p7->m_4 / 2;
            m_20 = 0;
            m_24 = 0;
            m_28 = p8->m_0 - 1;
            m_2c = p8->m_4 - 1;
            m_50 = p8->m_0 / 2;
            m_54 = p8->m_4 / 2;
            m_0 = a1;
            g_pSetRect_6c44b8(&m_58, rc->left, rc->top, rc->right, rc->bottom);
            m_68 = 0xffffa932;
            m_6c = 0xffffa932;
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// 0x0017c3f0 (287B) - a page/cursor command handler (proximity: CDDPageMgr).
// __thiscall(a1, src, dst, ..., kind, ..., a31): wire up the command, dispatch on
// `kind` (8 = blit src->dst via the two hand-rolled vtable interfaces; 0x18 =
// abort; 0x10 = validate), then commit the command block + hide the cursor. `src`
// / `dst` carry hand-rolled __stdcall function tables (object passed explicitly);
// the callee cleans, so they are modeled as __stdcall fn-ptr vtables.
// ===========================================================================
struct ObjA2_17c3f0;
struct VtblA2_17c3f0 {
    void* s0[5];
    i32(__stdcall* fn5)(ObjA2_17c3f0*, i32, i32*, i32*, i32); // slot +0x14
};
struct ObjA2_17c3f0 {
    VtblA2_17c3f0* vptr;
};
struct ObjA3_17c3f0;
struct VtblA3_17c3f0 {
    void* s0[31];
    void(__stdcall* fn31)(ObjA3_17c3f0*, i32); // slot +0x7c
};
struct ObjA3_17c3f0 {
    VtblA3_17c3f0* vptr;
};
extern "C" int(WINAPI* g_pShowCursor_6c44c4)(int); // 0x6c44c4

struct Handler_17c3f0 {
    void* m_0; // +0x00
    i32 m_4;   // +0x04
    void* m_8; // +0x08
    i32 m_c;   // +0x0c
    char m_pad10[0x14 - 0x10];
    ObjA2_17c3f0* m_14; // +0x14
    char m_pad18[0x1c - 0x18];
    ObjA3_17c3f0* m_1c; // +0x1c
    char m_pad20[0x24 - 0x20];
    i32 m_24; // +0x24
    i32 m_28; // +0x28
    i32 m_2c; // +0x2c
    char m_pad30[0x108 - 0x30];
    i32 m_108; // +0x108
    char m_pad10c[0x508 - 0x10c];
    i32 m_508; // +0x508
    char m_pad50c[0x510 - 0x50c];
    i32 m_510; // +0x510
    char m_pad514[0x518 - 0x514];
    i32 m_518; // +0x518
    i32 m_51c; // +0x51c
    i32 m_520; // +0x520
    void M_17cd90(void* a1);
    void M_17cc80();
    i32 M_17d2b0();
    void M_17d6b0();
    i32 Init(
        void* a1,
        ObjA2_17c3f0* a2,
        ObjA3_17c3f0* a3,
        i32 p4,
        i32 p5,
        i32 a6,
        i32 a7,
        i32 p8,
        i32 p9,
        i32 p10,
        i32 p11,
        i32 p12,
        i32 p13,
        i32 p14,
        i32 p15,
        i32 p16,
        i32 p17,
        i32 p18,
        i32 p19,
        i32 p20,
        i32 p21,
        i32 p22,
        i32 p23,
        i32 p24,
        i32 kind,
        i32 p26,
        i32 p27,
        i32 p28,
        i32 p29,
        i32 p30,
        i32 a31
    );
};

RVA(0x0017c3f0, 0x14e)
i32 Handler_17c3f0::Init(
    void* a1,
    ObjA2_17c3f0* a2,
    ObjA3_17c3f0* a3,
    i32 p4,
    i32 p5,
    i32 a6,
    i32 a7,
    i32 p8,
    i32 p9,
    i32 p10,
    i32 p11,
    i32 p12,
    i32 p13,
    i32 p14,
    i32 p15,
    i32 p16,
    i32 p17,
    i32 p18,
    i32 p19,
    i32 p20,
    i32 p21,
    i32 p22,
    i32 p23,
    i32 p24,
    i32 kind,
    i32 p26,
    i32 p27,
    i32 p28,
    i32 p29,
    i32 p30,
    i32 a31
) {
    if (!a1 || !a2 || !a3) {
        return 0;
    }
    m_14 = a2;
    m_c = 1;
    m_1c = a3;
    M_17cd90(a1);
    if (kind == 8) {
        if (m_14->vptr->fn5(m_14, 4, &m_108, &m_2c, 0)) {
            M_17cc80();
            return 0;
        }
        m_1c->vptr->fn31(m_1c, m_2c);
        m_510 = 0;
    }
    if (kind == 0x18) {
        M_17cc80();
        return 0;
    }
    if (kind == 0x10) {
        if (!M_17d2b0()) {
            M_17cc80();
            return 0;
        }
    }
    m_518 = a7;
    m_51c = a6;
    m_520 = kind;
    m_0 = a1;
    m_8 = 0;
    m_24 = 0;
    m_28 = 0;
    m_508 = a31;
    g_pShowCursor_6c44c4(0);
    m_4 = 1;
    M_17d6b0();
    return 1;
}

SIZE_UNKNOWN(Builder_168080);
SIZE_UNKNOWN(FxDesc_17fe00);
SIZE_UNKNOWN(FxSrc_17fe00);
SIZE_UNKNOWN(Fx_17fe00);
SIZE_UNKNOWN(Handler_17c3f0);
SIZE_UNKNOWN(ObjA2_17c3f0);
SIZE_UNKNOWN(ObjA3_17c3f0);
SIZE_UNKNOWN(Pt_168080);
SIZE(SubWidget_168080, 0x44);
SIZE_UNKNOWN(VtblA2_17c3f0);
SIZE_UNKNOWN(VtblA3_17c3f0);
VTBL(SubWidget_168080, 0x001f0310); // ??_7SubWidget_168080 (was g_subVtbl_5f0310)
