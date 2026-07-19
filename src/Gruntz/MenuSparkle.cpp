// MenuSparkle.cpp - the menu-sparkle eyecandy game-object (C:\Proj\Gruntz).
//
// This obj's ordinary .text contribution is the CONTIGUOUS run 0xadbe0..0xae32e:
// the ctor (0xadbe0 - the vtable-emission anchor), this leaf's per-class activation
// registrar (0xadde0 / 0xade60 / 0xadfc0), and the per-frame AdvanceAnim (0xae2a0).
// The /GX leaf dtor 0x101b0 is COMDAT-pooled (0x1xxxx pool), outside the run.
//
// The registrar trio was the former LogicActReg646010.cpp, whose @identity-TODO
// ("pin the owning leaf class" behind dispatch table 0x646010) is RESOLVED: the leaf
// is CMenuSparkle. Proof:
//   * ??_7CMenuSparkle@@6B@+0x10 (.rdata 0x1e82ec, via ILT thunk 0x19b0) points at
//     0xade60 - so that body is CMenuSparkle's own vtable slot 4, not some
//     "CProjActDispatcher"'s (that view is dissolved here).
//   * the trio is bracketed on BOTH sides by CMenuSparkle's own bodies inside this
//     run (ctor 0xadbe0 < 0xadde0/0xade60/0xadfc0 < SerializeMove 0xae1c0 <
//     AdvanceAnim 0xae2a0); a compiland's .text run is contiguous.
//   * ConstructLogicActRange_646010 (0xadde0) is called from the file-scope static
//     initializer at 0xadd58 - immediately after this ctor (0xadbe0+0x178), i.e.
//     inside this obj's own run.
//   * the archetype matches the sibling leaves exactly: CBehindCandyAni keeps its own
//     RegisterActs (0xad9b0) beside its AdvanceAnim (0xadbb0) in BehindCandyAni.cpp,
//     and CKitchenSlime::RegisterType is the cited ordering archetype.
// So g_logicActReg_646010 is CMenuSparkle's per-class activation table.
//
// The slot-1 SerializeMove (0xae1c0) still lives in MenuSparkleSerial.cpp under the
// Grunt.h-world serialize view (documented dual-model; never coexist in a TU) - it is
// inside this run and wants folding once that dual-model is retired.
// Only offsets / code bytes are load-bearing.
#include <Gruntz/MenuSparkle.h>
#include <Gruntz/AniAdvanceCursor.h> // the +0x1a0 anim sub-object (Advance)
#include <Bute/ButeTree.h>           // CButeTree (the "A" animset key store)
#include <Gruntz/ActNameRegistry.h>  // the shared action-name registry archetype
#include <Gruntz/ActReg.h>           // the shared activation-registrar archetype
#include <stdlib.h>                  // rand (0x11fee0; flicker-timer seed)

// The global bute store the ctor interns "A" in (?g_buteTree@@3VCButeTree@@A @0x6bf620).

// (The local CDDrawBlitParam facet view is GONE: Recompute_15c320 lives on the
// one CAniAdvanceCursor class - the ex dual-view is a single class now.)

// The frame delta / tick globals the sparkle handler drives (DATA-bound elsewhere:
// g_frameDelta in Attract.cpp, g_engineFrameDelta in the pump cluster); declared extern so the
// loads reloc-mask against the already-matched symbols.
extern "C" u32 g_frameDelta;       // 0x645584  per-frame time delta
extern "C" i32 g_engineFrameDelta; // 0x6bf3bc  frame tick

// --- CMenuSparkle (0x0adbe0), vptr 0x5e82dc --- the ctor anchors the ??_7CMenuSparkle
// vtable in this TU. Folds the inline CUserLogic(obj) base + the sparkle name/geometry
// setup, then seeds the random flicker delay.
RVA(0x000adbe0, 0x178)
CMenuSparkle::CMenuSparkle(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyName("MENU_SPARKLE");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("MENU_FORWARD100", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_objAux->m_130 = rand() % 0xfa1 + 0x3e8;
}

// --- CMenuSparkle::~CMenuSparkle (0x101b0) --- empty vtable-anchor dtor; folds the
// bare CUserLogic teardown (the destructible +0x18 link forces the /GX EH frame).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CMenuSparkle() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CMenuSparkle@@UAE@XZ 0x000101b0 0x44

// ===========================================================================
// CMenuSparkle's per-class activation registrar (dispatch table 0x646010), homed
// from the former LogicActReg646010.cpp - see the identity proof in the header.
// ===========================================================================

// This leaf's per-class activation dispatch table (.data, DATA-pinned).
DATA(0x00246010)
extern CLogicActTable g_logicActReg_646010; // 0x646010

// The class activation handler (ILT thunk 0x403c10 -> 0xad2a0).
extern "C" void LogicHandler_0ad2a0();

// The shared name-registry build (action key "A"), CKitchenSlime::RegisterType
// ordering: register the action name on first use, resolve its name-table slot, free
// the slot's old CString nodes, assign the key, bump the global counter; returns the
// (possibly newly-allocated) action id.
static inline i32 RegisterActionName() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        i32 key = g_typeCounter;
        id = key;
        char* slot = ActNameLookup(key);
        i32 cnt = g_typeColl.m_grown;
        void** nodes = reinterpret_cast<void**>(g_typeColl.m_alloc);
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    (reinterpret_cast<CString*>(nodes))->CString::~CString();
                }
                nodes++;
            } while (--cnt);
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    return id;
}

// ConstructLogicActRange_646010 @0x0adde0 - the static initializer that builds this
// leaf's dispatch table's fast [0x7d0, 0x7da] id range. Called from the file-scope
// static-init at 0xadd58, immediately after this TU's ctor.
RVA(0x000adde0, 0x15)
void ConstructLogicActRange_646010() {
    g_logicActReg_646010.Construct(0x7d0, 0x7da);
}

// CMenuSparkle::Dispatch @0x0ade60 - per-coordinate activation dispatch over this
// leaf's table. Resolves the activation entry for `coord` (ResolveEntry, inlined
// twice); if the entry's leading handler slot is non-null, re-resolves and invokes it
// __thiscall on this object.
//
// This IS CMenuSparkle's vtable slot 4 (??_7CMenuSparkle@@6B@+0x10 -> 0xade60 via ILT
// thunk 0x19b0; vtable_hierarchy tags slot 4 `override`, origin CUserLogic). It is
// declared a PLAIN method rather than the OVERRIDE, because the campaign-wide
// CUserLogic base models slot 4 with a no-arg `UserLogicVfunc2()` placeholder while
// the real slot is int-arg - retail's own base body (thunk 0x246e -> 0x8b70) and this
// override both `ret 4`. That is the same documented workaround ~40 sibling leaves use
// for their RunAct/FireActivation slot-4 bodies (see Grunt.h, Projectile.h,
// PathHazard.cpp, ActionArea.h); fixing the base arity is a tree-wide change and would
// let every one of them become a real OVERRIDE and delete its placeholder virtual.
// The former CProjActDispatcher .cpp-local view is DISSOLVED onto CMenuSparkle here.

// The entry's leading slot is a __thiscall handler taking this object; MSVC5 rejects
// the __thiscall keyword, so model it as a single-inheritance member pointer (a bare
// 4-byte code address) reinterpreted from the entry word.
typedef void (CUserLogic::*MenuSparkleActHandler)();

RVA(0x000ade60, 0x102)
void CMenuSparkle::FireActivation(i32 coord) {
    char* e = g_logicActReg_646010.ResolveEntry(coord);
    if (*reinterpret_cast<void**>(e) != 0) {
        char* e2 = g_logicActReg_646010.ResolveEntry(coord);
        MenuSparkleActHandler h = *reinterpret_cast<MenuSparkleActHandler*>(e2);
        (this->*h)();
    }
}

// RegisterXLogic_646010 @0x0adfc0 - bind this leaf to its activation handler. Same
// archetype/wall as 0x03a710.
// @early-stop
// register-pinning wall (see CursorSnapActReg.cpp): logic byte-faithful, residual is
// the action-id register coloring + count-down induction. Deferred.
RVA(0x000adfc0, 0x18d)
void RegisterXLogic_646010() {
    i32 id = RegisterActionName();
    *reinterpret_cast<void**>(g_logicActReg_646010.ResolveEntry(id)) = static_cast<void*>(&LogicHandler_0ad2a0);
}

// CMenuSparkle::AdvanceAnim @0x0ae2a0 - the sparkle's per-frame handler. Tick down
// the aux flicker countdown (m_objAux->m_130, seeded random in the ctor); when it
// reaches 0 advance the +0x1a0 anim; then, while the object is active (m_38->m_1a0.m_28)
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
    u32 delta = g_frameDelta;
    if (delta >= m_objAux->m_130) {
        m_objAux->m_130 = 0;
    } else {
        m_objAux->m_130 -= delta;
    }
    if (m_objAux->m_130 == 0) {
        m_38->m_1a0.Advance(g_engineFrameDelta);
    }
    CAniAdvanceCursor* anim = &m_38->m_1a0;
    i32 active = m_38->m_1a0.m_28;
    if (active != 0 && anim->m_20 == 0) {
        if (anim != 0) {
            anim->Recompute_15c320(1);
        }
        *reinterpret_cast<i32*>((reinterpret_cast<char*>(m_3c) + 0x20)) = rand() % 0xfa1 + 0x3e8;
    }
    return 0;
}

#include <rva.h>
