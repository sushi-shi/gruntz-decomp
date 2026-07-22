#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Wap32/ZVec.h>
#include <Bute/ButeTree.h>
#include <Gruntz/ToobSpikez.h>
#include <Gruntz/XferArchive.h> // the real 0x16e4f0 = ProjTypeXfer(CXferArchive*)
#include <Gruntz/ActReg.h>      // CActReg (g_toobColl); ResolveEntry + GetRetAddr/g_projActCache
#include <Gruntz/ActNameRegistry.h> // the shared name registry: g_typeColl/g_typeCounter/s_codeA/ActNameLookup/g_buteTree

RVA_COMPGEN(0x00012c60, 0x44, ??1CToobSpikez@@UAE@XZ)

RVA(0x00114480, 0xf1)
i32 ToobSpikezLogic(CGameObject* obj) {
    AnimWorkerObj* rec = obj->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CToobSpikez* inst = new CToobSpikez(obj);
            inst->Activate(); // slot 6
            rec->m_logic = inst;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9(); // slot 11
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8(); // slot 10
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC(); // slot 14
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB(); // slot 13
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA(); // slot 12
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD(); // slot 15
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(rec->m_logic);
            break;
    }
    return 1;
}

VTBL(CToobSpikez, 0x001e7774);
DATA_SYMBOL(0x0024e978, 0x24, ?g_toobColl@@3UCActReg@@A)

typedef void (CUserLogic::*ToobHandler)();


RVA(0x001145c0, 0x18e)
CToobSpikez::CToobSpikez(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 2);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_object->m_164 = m_object->m_screenX >> 5;
    m_object->m_168 = m_object->m_screenY >> 5;
    if (m_object->m_sortKey != 0xc) {
        m_object->m_sortKey = 0xc;
        m_object->m_flags |= 0x20000;
    }
}

RVA(0x001147e0, 0x15)
void CToobSpikez::Register() {
    g_toobColl.Construct(0x7d0, 0x7da);
}

RVA(0x00012bc0, 0x47)
i32 CToobSpikez::SerializeMove(CGruntArchive* a, i32 b, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CToobSpikez::~CToobSpikez @0x012c60 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70;
// the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CToobSpikez() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>

RVA(0x00114860, 0x102)
void CToobSpikez::FireActivation(i32 coord) {
    ToobHandler* e = reinterpret_cast<ToobHandler*>(g_toobColl.ResolveEntry(coord));
    if (*e != 0) {
        ToobHandler* e2 = reinterpret_cast<ToobHandler*>(g_toobColl.ResolveEntry(coord));
        (this->*(*e2))();
    }
}

// CToobSpikez::RegisterActs @0x1149c0 - bind the logic handler to the activation
// key "A" in the toob-spikez's OWN registry (g_toobColl). See the registration
// commentary above. The SAME archetype as CParticlez::RegisterActs.
//
// @early-stop
// zvec/name-vec IndexToPtr regalloc wall (docs/patterns/zero-register-pinning.md +
// the documented ZVec family): logic + the bute find/insert + the fn-ptr store are
// byte-faithful; cl pins the index/this/base across the grow branches differently
// than retail. Not source-steerable; the SAME plateau as CParticlez::RegisterActs.
RVA(0x001149c0, 0x18d)
void CToobSpikez::RegisterActs() {
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
    *reinterpret_cast<void**>(g_toobColl.ResolveEntry(id)) = static_cast<void*>(&ToobLogic_114bc0);
}

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
