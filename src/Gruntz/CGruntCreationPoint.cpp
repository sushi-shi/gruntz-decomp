// CGruntCreationPoint.cpp - the grunt creation-point game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CGruntCreationPoint methods, defined in ascending
// retail-RVA order:
//   ~CGruntCreationPoint @0x010730 - the /GX leaf dtor (folds the CUserLogic teardown).
//   AdvanceAnim          @0x03ecc0 - the per-frame animation-advance (ret 0).
//
// CGruntCreationPoint : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CGruntCreationPoint.h>

// The animation sub-object embedded at CGameObject+0x1a0 (the bound object is
// CUserLogic::m_38). Its setter (0x15c360, __thiscall, 1 arg) re-targets the
// active animation to the draw-delta passed in; modeled NO-body so the call
// reloc-masks (the body lives in the engine sub-mgr TU). The SAME engine method
// CSimpleAnimation::AdvanceAnim / CGruntPuddleSink::Notify call.
struct CAnimSink {
    i32 SetAnim(u32 ctx); // 0x15c360
};

// The global the advance hands the sink (_g_6bf3bc; the per-frame draw-delta
// mirror). Defined in SpriteResource.cpp/Projectile.cpp; declared extern "C"
// here so the value-load reloc-masks against the already-matched symbol.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// CGruntCreationPoint::~CGruntCreationPoint @0x010730 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
RVA(0x00010730, 0x44)
CGruntCreationPoint::~CGruntCreationPoint() {}

// CGruntCreationPoint::AdvanceAnim @0x03ecc0 - re-target the bound object's
// animation sub-object (m_38 + 0x1a0) to the current draw-delta (g_6bf3bc) and
// return 0. Same archetype as CSimpleAnimation::AdvanceAnim (0x0abf70).
RVA(0x0003ecc0, 0x17)
i32 CGruntCreationPoint::AdvanceAnim() {
    ((CAnimSink*)((char*)m_38 + 0x1a0))->SetAnim(g_6bf3bc);
    return 0;
}
