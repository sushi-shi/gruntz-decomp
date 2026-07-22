#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Image/CImage.h> // the +0x198 cached frame (ex CGameObjLayer view)
#include <Wap32/ZVec.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/BehindCandyAni.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)

VTBL(CBehindCandyAni, 0x001e838c);


RVA(0x00010050, 0x47)
i32 CBehindCandyAni::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CBehindCandyAni::~CBehindCandyAni @0x0100f0 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the
// embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CBehindCandyAni() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>
RVA_COMPGEN(0x000100f0, 0x44, ??1CBehindCandyAni@@UAE@XZ)

RVA(0x000ad540, 0x1f0)
CBehindCandyAni::CBehindCandyAni(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_38->m_1a0.m_14 == 0) {
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    if (m_object->m_sortKey != 0) {
        m_object->m_sortKey = 0;
        m_object->m_flags |= 0x20000;
    }
    if (m_object->m_layer != 0) {
        if (m_object->m_layer->m_width >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_height >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

RVA(0x000ad7d0, 0x15)
void CBehindCandyAni::InitActReg() {
    g_behindCandyActReg.Construct(2000, 2010);
}

RVA(0x000ad850, 0x102)
void CBehindCandyAni::FireActivation(i32 id) {
    CBehindCandyActEntry* e = reinterpret_cast<CBehindCandyActEntry*>(g_behindCandyActReg.ResolveEntry(id));
    if (e->m_fn != 0) {
        (this->*(reinterpret_cast<CBehindCandyActEntry*>(g_behindCandyActReg.ResolveEntry(id)))->m_fn)();
    }
}

// CBehindCandyAni::RegisterActs @0x0ad9b0 - bind the class's per-frame handler
// (AdvanceAnim @0x0adbb0) to the activation key "A" via the shared name registry.
// The SAME archetype as CSecretLevelTrigger::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000ad9b0, 0x18d)
void CBehindCandyAni::RegisterActs() {
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
    (reinterpret_cast<CBehindCandyActEntry*>(g_behindCandyActReg.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CBehindCandyAni::AdvanceAnim);
}

RVA(0x000adbb0, 0x17)
i32 CBehindCandyAni::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)

// g_behindCandyActReg (0x00245f98): CBehindCandyActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
DATA_SYMBOL(0x00245f98, 0x0, ?g_behindCandyActReg@@3UCBehindCandyActReg@@A)
