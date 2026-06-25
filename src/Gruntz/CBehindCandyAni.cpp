// CBehindCandyAni.cpp - a behind-candy eyecandy animation game-object
// (C:\Proj\Gruntz).
//
// Two trace-discovered CBehindCandyAni methods, defined in ascending retail-RVA
// order:
//   ~CBehindCandyAni @0x0100f0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   AdvanceAnim      @0x0adbb0 - the per-frame animation-advance (ret 0).
//
// CBehindCandyAni : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CBehindCandyAni.h>

// The animation sub-object embedded at CGameObject+0x1a0 (the bound object is
// CUserLogic::m_38). Its setter (0x15c360, __thiscall, 1 arg) re-targets the
// active animation to the draw-delta passed in; modeled NO-body so the call
// reloc-masks. The SAME engine method CSimpleAnimation / CGruntPuddle / the
// projectile render-obj all call.
struct CAnimSink {
    i32 SetAnim(u32 ctx); // 0x15c360
};

// The global the advance hands the sink (_g_6bf3bc; the per-frame draw-delta
// mirror). Declared extern "C" here so the value-load reloc-masks against the
// already-matched symbol.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// CBehindCandyAni::~CBehindCandyAni @0x0100f0 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the
// embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
RVA(0x000100f0, 0x44)
CBehindCandyAni::~CBehindCandyAni() {}

// CBehindCandyAni::AdvanceAnim @0x0adbb0 - re-target the bound object's
// animation sub-object (m_38 + 0x1a0) to the current draw-delta (g_6bf3bc) and
// return 0. Byte-identical to CSimpleAnimation::AdvanceAnim save the call
// displacement.
RVA(0x000adbb0, 0x17)
i32 CBehindCandyAni::AdvanceAnim() {
    ((CAnimSink*)((char*)m_38 + 0x1a0))->SetAnim(g_6bf3bc);
    return 0;
}
