#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Wap32/ZVec.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/SingleAnimation.h>
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)

VTBL(CSingleAnimation, 0x001e745c);
template<> DATA(0x00245f70)
CActReg CActRegPool<CSingleAnimation>::s_table(2000, 2010);

RVA(0x000104a0, 0x47)
i32 CSingleAnimation::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CSingleAnimation::~CSingleAnimation @0x010540 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the
// embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CSingleAnimation() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>
RVA_COMPGEN(0x00010540, 0x44, ??1CSingleAnimation@@UAE@XZ)

RVA(0x000ae7f0, 0x13d)
CSingleAnimation::CSingleAnimation(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 2;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
}

RVA(0x000aea20, 0x102)
void CSingleAnimation::FireActivation(i32 id) {
    CSingleAnimActEntry* e = reinterpret_cast<CSingleAnimActEntry*>(
        CActRegPool<CSingleAnimation>::s_table.ResolveEntry(id)
    );
    if (e->m_fn != 0) {
        (this
             ->*(reinterpret_cast<CSingleAnimActEntry*>(
                 CActRegPool<CSingleAnimation>::s_table.ResolveEntry(id)
             ))
             ->m_fn)();
    }
}

// CSingleAnimation::RegisterActs @0x0aeb80 - bind the class's per-frame handler
// (AdvanceAnim @0x0aed80) to the activation key "A" via the shared name registry.
// The SAME archetype as CBehindCandyAni::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000aeb80, 0x18d)
void CSingleAnimation::RegisterActs() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", reinterpret_cast<void*>(id));
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    (reinterpret_cast<CSingleAnimActEntry*>(
         CActRegPool<CSingleAnimation>::s_table.ResolveEntry(id)
     ))
        ->m_fn = static_cast<i32 (CUserLogic::*)()>(&CSingleAnimation::AdvanceAnim);
}

RVA(0x000aed80, 0x39)
i32 CSingleAnimation::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    if (m_38->m_1a0.m_28 != 0 && m_38->m_1a0.m_20 == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
