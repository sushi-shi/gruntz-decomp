#include <rva.h>
#include <Wap32/Object.h> // CObject - the shared engine grand-base (sub-widget CObject prefix)
// ApiHiCallers.cpp - reconstructed game API-caller methods, the HIGH-RVA half
// (RVA >= 0x0e0000). Each was classified as GAME (not CRT/MFC-library) and
// reconstructed against a best-guess
// class shape: placeholder m_<hexoffset> field names, only the OFFSETS + emitted
// code bytes are load-bearing (campaign doctrine). Plain /O2 /MT frameless leaves
// (no SEH/EH). External engine callees are modeled with NO body so their rel32/
// DIR32 references reloc-mask.
#include <Win32.h>

// (0x0017fe00 fader-noise init re-homed to src/Gruntz/Fader.cpp as CFaderSine::
// ApplyInit - the CFaderSine subtype's default-init apply; the FxDesc/FxSrc views
// folded onto the canonical CFaderInit/FaderSrc, and the third LCG RNG + its seed
// globals + the ScatterSamples extern moved alongside it.)

// ===========================================================================
// 0x00168080 (502B) - a compound-widget builder (proximity: CGameLevel/CWwdGrid).
// __thiscall(a1, rc, p3, p4, p5, p6, p7, p8): allocate three same-typed sub-
// widgets (operator new + vtable stamp), lay each out over the RECT *rc with its
// own size-pair, then derive the widget's own extents (min-1 + half-centers) from
// three more size-pairs and finish with a SetRect over rc's corners.
//
// ATTRIBUTION (proven, sema xref + disasm): this IS WwdPlaneRender::Init - the
// 0xb8-byte plane-render worker's init, and its ONLY caller is WwdFile::RebuildPlanes
// (@0x1628f0, src/Wwd/WwdFile.cpp) which `new`s the 0xb8 worker (stamps the +0x70
// CObList vtable 0x5f02a8) then calls this at 0x168080. Builder_168080 is a partial
// (+0x00..+0x6c) view of WwdPlaneRender; SubWidget_168080 (0x44 B, vtable 0x5f0310)
// is the sub-render-object it allocates x3.
//   DEFERRED (not moved this batch): the real Init takes 8 args (this ret 0x20) -
//   src + a RECT* (arg2, a 4-int region: `Setup(*rc)` + `SetRect(rc->l..b)`) + 6 Pt*.
//   But RebuildPlanes' @early-stop reconstruction passes only 7 args (`Init(src,
//   p0..p5)`) and builds its 6 pairs with a SIMPLIFIED geo mapping, whereas retail
//   interleaves 12 header ints (hdr+0xb0..0xdc) into a RECT + 6 pairs via a SCRAMBLED
//   stack layout (disasm 0x1629b1..0x162a69: pairs mix non-adjacent geo, e.g.
//   [esp+0x18]=geo@0xdc with [esp+0x1c]=geo@0xd0). Homing Init as an 8-arg
//   WwdPlaneRender::Init forces rewriting RebuildPlanes' call + its scrambled arg-
//   build, which risks regressing that @early-stop match; that arg-build recovery is
//   the prerequisite move. Left here with the attribution recorded for the final sweep.
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
// (0x1bef01 / 0x0028ec / 0x00106e / 0x004034) come from the CObject grand-
// base; slot 1 is the class's own 0x168280 scalar dtor, slot 5 the 0x168060 new
// virtual - all declared-only -> reloc-masked.
struct SubWidget_168080 : public CObject {
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
// @orphan: `this` is a STACK-LOCAL command block - CGruntzMgr::ChangeState_8fab0
// (0x8fab0) builds it on its own stack and Init()s it (`lea ecx,[esp+0x94]; call
// 0x17c3f0`), so there is no persistent owning class to home it onto; the 0x520-byte
// command-block layout + its ObjA2/ObjA3 interfaces are the transient's own shape.
// ===========================================================================
struct ObjA2_17c3f0 { // real polymorphic; fn5 is slot 5 (+0x14)
    virtual void Slot0();
    virtual void Slot1();
    virtual void Slot2();
    virtual void Slot3();
    virtual void Slot4();
    virtual i32 __stdcall fn5(i32, i32*, i32*, i32); // slot 5 (+0x14)
};
struct ObjA3_17c3f0 { // real polymorphic; fn31 is slot 31 (+0x7c)
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void Slot10();
    virtual void Slot11();
    virtual void Slot12();
    virtual void Slot13();
    virtual void Slot14();
    virtual void Slot15();
    virtual void Slot16();
    virtual void Slot17();
    virtual void Slot18();
    virtual void Slot19();
    virtual void Slot20();
    virtual void Slot21();
    virtual void Slot22();
    virtual void Slot23();
    virtual void Slot24();
    virtual void Slot25();
    virtual void Slot26();
    virtual void Slot27();
    virtual void Slot28();
    virtual void Slot29();
    virtual void Slot30();
    virtual void __stdcall fn31(i32); // slot 31 (+0x7c)
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
        if (m_14->fn5(4, &m_108, &m_2c, 0)) {
            M_17cc80();
            return 0;
        }
        m_1c->fn31(m_2c);
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
SIZE_UNKNOWN(Handler_17c3f0);
SIZE_UNKNOWN(ObjA2_17c3f0);
SIZE_UNKNOWN(ObjA3_17c3f0);
SIZE_UNKNOWN(Pt_168080);
SIZE(SubWidget_168080, 0x44);
SIZE_UNKNOWN(VtblA2_17c3f0);
SIZE_UNKNOWN(VtblA3_17c3f0);
VTBL(SubWidget_168080, 0x001f0310); // ??_7SubWidget_168080 (was g_subVtbl_5f0310)

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
