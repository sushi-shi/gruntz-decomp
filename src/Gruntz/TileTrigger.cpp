// TileTrigger.cpp - the CTileTrigger tile-logic game-object family (C:\Proj\Gruntz).
//
// The whole CTileTrigger band - the base's no-arg/1-arg ctors (vtable-emission
// anchors), the slot-1 SerializeMove, the base + three leaf /GX dtors, the two
// per-class activation registries and their Init/Fire/Register methods, and the
// three leaf 1-arg ctors - defined in ascending retail-RVA order. Re-homed out of
// the UserLogic god-TU; the leaf views are dissolved onto the canonical
// <Gruntz/TileTrigger.h> (real leaf classes deriving from CTileTrigger). The state
// pumps that `new` these leaves live in src/Gruntz/TileLogicPump.cpp.
#include <Gruntz/ActNameRegistry.h> // shared activation-name registry (g_buteTree/s_actKeyA/ActNameLookup)
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/TileTrigger.h>
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip

// The activation key "B" (0x60d1bc) CTileSecretTrigger's second registration
// interns; s_actKeyA comes from <Gruntz/ActNameRegistry.h>.
DATA(0x0020d1bc)
extern char s_actKeyB[]; // "B"

// The shared per-leaf activation-coordinate registry singletons each RegisterActs
// binds its id->handler entry in - same [2000,2010] range shape but a distinct
// per-class instance. CLeafActReg is the shared <Gruntz/ActReg.h> CActReg archetype;
// each keeps its own placeholder name so the DATA-pinned globals below are unchanged.
SIZE_UNKNOWN(CLeafActReg);
struct CLeafActReg : public CActReg {};
DATA(0x0024e810)
extern CLeafActReg g_tileTriggerActReg; // 0x64e810
DATA(0x0024e7e8)
extern CLeafActReg g_tileSecretTriggerActReg; // 0x64e7e8

// Each leaf's handler entry: its first dword receives the per-frame handler PMF (a
// 4-byte code ptr on the single-inheritance class), invoked __thiscall on the trigger.
typedef i32 (CTileTrigger::*TileTriggerHandler)();
SIZE_UNKNOWN(CTileTriggerActEntry);
struct CTileTriggerActEntry {
    TileTriggerHandler m_fn;
};
typedef i32 (CTileSecretTrigger::*TileSecretTriggerHandler)();
SIZE_UNKNOWN(CTileSecretTriggerActEntry);
struct CTileSecretTriggerActEntry {
    TileSecretTriggerHandler m_fn;
};

// ===========================================================================
// Definitions in ascending-RVA order.
// ===========================================================================

RVA(0x00011160, 0x4b)
CTileTrigger::CTileTrigger() {}

// --- CTileTrigger::SerializeMove (0x111f0), vtable slot 1 ---
// Base impl shared (inherited) by CGiantRock/CCoveredPowerup/CTileSecretTrigger
// (their slot-1 vtable entries all point here - no leaf override).
RVA(0x000111f0, 0x47)
i32 CTileTrigger::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (!SerializeChain((i32)ar, mode, a3, a4)) {
        return 0;
    }
    return SerialRef34()->Chain((CSerialArchive*)ar, mode, a3, (CSerialObj*)a4) != 0;
}

// --- CTileTrigger / leaf destructors (0x011290 / 0x011540 / 0x011600 / 0x0116c0) ---
// All four are the SAME folded CUserLogic teardown (store CUserLogic vptr,
// inline-destruct the +0x18 link via ~EngStr 0x16d2a0, store CUserBase vptr; the
// destructible link forces the /GX EH frame; leaf vptr store dead-eliminated).
// ~CTileTrigger is inline (header) so it folds into the three leaf dtors instead
// of being called. MSVC still emits one out-of-line COMDAT copy of ~CTileTrigger
// (called by its scalar-deleting dtor); it lands at 0x011290. An inline-defined
// dtor can't hang an RVA() (the attribute would also tag the synthesized ??_G ->
// duplicate-RVA), so it is pinned by mangled name here:
// @rva-symbol: ??1CTileTrigger@@UAE@XZ 0x00011290 0x44
RVA(0x00011540, 0x44)
CTileSecretTrigger::~CTileSecretTrigger() {}
RVA(0x00011600, 0x44)
CGiantRock::~CGiantRock() {}
RVA(0x000116c0, 0x44)
CCoveredPowerup::~CCoveredPowerup() {}

// --- CTileTrigger 1-arg (0x10e220), vptr 0x5e7f14 ---
RVA(0x0010e220, 0x17d)
CTileTrigger::CTileTrigger(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    m_38->m_stateFlags |= 1;
    m_object->m_164 = m_object->m_screenX >> 5;
    m_object->m_168 = m_object->m_screenY >> 5;
    m_object->m_04 = (m_object->m_164 << 8) + m_object->m_168;
}

// --- CTileTrigger::InitActReg (0x10e420) ---
// Construct the class's activation-coordinate registry singleton
// (g_tileTriggerActReg @0x64e810) over the fixed range [2000, 2010] via the
// shared registry ctor (0x408710). Free init thunk; reloc-masked.
RVA(0x0010e420, 0x15)
void CTileTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_tileTriggerActReg)->Construct(2000, 2010);
}

// --- CTileTrigger::FireActivation (0x10e4a0), vtable slot 4 ---
// Look the activation coordinate up in the class registry (g_tileTriggerActReg); if the
// resolved entry carries a registered handler PMF, resolve it again and dispatch it
// __thiscall on `this`. Same archetype as CWarpStonePad::FireWarp.
RVA(0x0010e4a0, 0x102)
void CTileTrigger::FireActivation(i32 coord) {
    CTileTriggerActEntry* e = (CTileTriggerActEntry*)g_tileTriggerActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CTileTriggerActEntry* e2 = (CTileTriggerActEntry*)g_tileTriggerActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
}

// --- CTileTrigger::RegisterActs (0x10e600) ---
// Bind the per-frame handler (AdvanceAnim @0x10ee00) to the activation key "A"
// via the shared name registry + the class's coordinate registry
// (g_tileTriggerActReg). SAME archetype as CSecretTeleporterTrigger::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the handler store match
// retail); residual is the slot-vs-id callee-saved register choice cascading into
// the free-loop count materialization. Deferred.
RVA(0x0010e600, 0x18d)
void CTileTrigger::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    ((CTileTriggerActEntry*)g_tileTriggerActReg.ResolveEntry(id))->m_fn =
        &CTileTrigger::AdvanceAnim;
}

// --- CTileSecretTrigger::InitActReg (0x10f160) ---
// Construct the class's activation-coordinate registry singleton
// (g_tileSecretTriggerActReg @0x64e7e8) over the fixed range [2000, 2010] via the
// shared registry ctor (0x408710). Free init thunk; reloc-masked.
RVA(0x0010f160, 0x15)
void CTileSecretTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_tileSecretTriggerActReg)->Construct(2000, 2010);
}

// --- CTileSecretTrigger::FireActivation (0x10f1e0), vtable slot 4 ---
// Look the activation coordinate up in the class registry (g_tileSecretTriggerActReg);
// if the resolved entry carries a registered handler PMF, resolve it again and dispatch
// it __thiscall on `this`. Same archetype as CWarpStonePad::FireWarp.
RVA(0x0010f1e0, 0x102)
void CTileSecretTrigger::FireActivation(i32 coord) {
    CTileSecretTriggerActEntry* e =
        (CTileSecretTriggerActEntry*)g_tileSecretTriggerActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CTileSecretTriggerActEntry* e2 =
            (CTileSecretTriggerActEntry*)g_tileSecretTriggerActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
}

// --- CTileSecretTrigger::RegisterActs (0x10f340) ---
// Intern "A" and "B" and bind each to its per-frame handler (0x10f6a0 / 0x10f970)
// in the class's coordinate registry (g_tileSecretTriggerActReg). Two back-to-back
// single-key registrations; the SAME archetype as CTileTrigger::RegisterActs done
// twice.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (both intern/name-resolve blocks + the OWN-registry resolves + the
// `mov [entry],offset handler` stores match retail); residual is the slot-vs-id
// callee-saved register choice cascading into the free-loop counts. Deferred.
RVA(0x0010f340, 0x2ac)
void CTileSecretTrigger::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    ((CTileSecretTriggerActEntry*)g_tileSecretTriggerActReg.ResolveEntry(id))->m_fn =
        &CTileSecretTrigger::Act_10f6a0;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        id2 = g_nextActId;
        g_buteTree.Insert(s_actKeyB, (void*)id2);
        char* slot = ActNameLookup(id2);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyB);
        g_nextActId++;
    }
    ((CTileSecretTriggerActEntry*)g_tileSecretTriggerActReg.ResolveEntry(id2))->m_fn =
        &CTileSecretTrigger::Act_10f970;
}

// --- The three CTileTrigger leaves' 1-arg ctors (0x10fa60/90/c0) ---
// Each just chains CTileTrigger(obj) (out-of-line call) then the leaf vptr
// auto-stamps. vptrs: CTileSecretTrigger 0x5e7e64, CGiantRock 0x5e7d5c,
// CCoveredPowerup 0x5e7e0c.
RVA(0x0010fa60, 0x19)
CTileSecretTrigger::CTileSecretTrigger(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fa90, 0x19)
CGiantRock::CGiantRock(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fac0, 0x19)
CCoveredPowerup::CCoveredPowerup(CGameObject* obj) : CTileTrigger(obj) {}
