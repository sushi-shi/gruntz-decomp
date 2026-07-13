// MenuSparkle.cpp - the menu-sparkle eyecandy game-object (C:\Proj\Gruntz).
//
// The canonical CMenuSparkle band (ctor 0xadbe0 - the vtable-emission anchor - the
// /GX leaf dtor 0x101b0, and the per-frame AdvanceAnim 0xae2a0), re-homed out of the
// UserLogic god-TU. The slot-1 SerializeMove (0xae1c0) lives in MenuSparkleSerial.cpp
// under the Grunt.h-world serialize view (documented dual-model; never coexist in a
// TU). Only offsets / code bytes are load-bearing.
#include <Gruntz/MenuSparkle.h>
#include <Gruntz/AniAdvanceCursor.h> // the +0x1a0 anim sub-object (Advance_15c360)
#include <Bute/ButeTree.h>           // CButeTree (the "A" animset key store)
#include <stdlib.h>                  // rand (0x11fee0; flicker-timer seed)

// The global bute store the ctor interns "A" in (?g_buteTree@@3VCButeTree@@A @0x6bf620).
extern CButeTree g_buteTree;

// The +0x1a0 anim sub-object's blit-param facet (Recompute @0x15c320, __thiscall) -
// the SAME embedded object CAniAdvanceCursor advances, viewed as CDDrawBlitParam for
// the recompute call (the documented per-leaf +0x1a0 dual-view; call reloc-masks).
class CDDrawBlitParam {
public:
    void Recompute_15c320(i32 x); // 0x15c320
};

// The frame delta / tick globals the sparkle handler drives (DATA-bound elsewhere:
// g_645584 in Attract.cpp, g_6bf3bc in the pump cluster); declared extern so the
// loads reloc-mask against the already-matched symbols.
extern "C" u32 g_645584; // 0x645584  per-frame time delta
extern "C" i32 g_6bf3bc; // 0x6bf3bc  frame tick

// --- CMenuSparkle (0x0adbe0), vptr 0x5e82dc --- the ctor anchors the ??_7CMenuSparkle
// vtable in this TU. Folds the inline CUserLogic(obj) base + the sparkle name/geometry
// setup, then seeds the random flicker delay.
RVA(0x000adbe0, 0x178)
CMenuSparkle::CMenuSparkle(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->ApplyName("MENU_SPARKLE");
    m_40 = m_38->m_geoId;
    m_38->ApplyLookupGeometry("MENU_FORWARD100", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_objAux->m_130 = rand() % 0xfa1 + 0x3e8;
}

// --- CMenuSparkle::~CMenuSparkle (0x101b0) --- empty vtable-anchor dtor; folds the
// bare CUserLogic teardown (the destructible +0x18 link forces the /GX EH frame).
RVA(0x000101b0, 0x44)
CMenuSparkle::~CMenuSparkle() {}

// CMenuSparkle::AdvanceAnim @0x0ae2a0 - the sparkle's per-frame handler. Tick down
// the aux flicker countdown (m_objAux->m_130, seeded random in the ctor); when it
// reaches 0 advance the +0x1a0 anim; then, while the object is active (m_38->m_1c8)
// and the anim is idle (m_20 == 0), recompute the blit param and re-arm the random
// flicker delay (rand()%0xfa1 + 0x3e8, the same range the ctor seeds).
// @early-stop
// scheduling/regalloc coin-flip (~95%, topic:scheduling): logic byte-faithful (the
// countdown, the conditional Advance, the m_1c8/m_20 gate, the Recompute + rand
// re-arm all match retail). Sole residual: retail schedules `lea ecx,[m_38+0x1a0]`
// (anim) BEFORE `mov eax,[m_38+0x1c8]` (m_1c8, reusing eax), whereas MSVC5 loads
// m_1c8 into edx first (keeping m_38 in eax) then the lea - a 2-instruction reorder
// + reg choice not steerable from C (tried o-local, active-local, direct m_38).
RVA(0x000ae2a0, 0x8e)
i32 CMenuSparkle::AdvanceAnim() {
    u32 delta = g_645584;
    if (delta >= m_objAux->m_130) {
        m_objAux->m_130 = 0;
    } else {
        m_objAux->m_130 -= delta;
    }
    if (m_objAux->m_130 == 0) {
        ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_6bf3bc);
    }
    CAniAdvanceCursor* anim = (CAniAdvanceCursor*)((char*)m_38 + 0x1a0);
    i32 active = m_38->m_1c8;
    if (active != 0 && anim->m_20 == 0) {
        if (anim != 0) {
            ((CDDrawBlitParam*)anim)->Recompute_15c320(1);
        }
        *(i32*)((char*)m_3c + 0x20) = rand() % 0xfa1 + 0x3e8;
    }
    return 0;
}

#include <rva.h>
SIZE_UNKNOWN(CDDrawBlitParam);
