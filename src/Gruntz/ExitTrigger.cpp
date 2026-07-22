#include <Gruntz/ExitTrigger.h>
#include <Gruntz/Grunt.h> // complete CGrunt: the CUserLogic downcast is static
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/CurPlayer.h>     // g_curPlayer
#include <Gruntz/SerialCounter.h> // g_serialCounter
#include <Gruntz/TypeKeyColl.h>   // s_codeA/s_actKeyB registration keys
#include <Io/FileMem.h>           // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/GameRegistry.h>
#include <Gruntz/LogicTypeId.h>
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0; +0x48 GruntObjMap)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @+0x2c / Write @+0x30)

// CExitTrigger::~CExitTrigger @0x0108c0 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70;
// the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CExitTrigger() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>
RVA_COMPGEN(0x000108c0, 0x44, ??1CExitTrigger@@UAE@XZ)

#include <Bute/ButeMgr.h>

#include <Gruntz/TriggerMgr.h>

VTBL(CExitTrigger, 0x001e822c);

// CExitTrigger::CExitTrigger(CGameObject*) @0x03ecf0 - the 1-arg leaf ctor: the
// standard CUserLogic(obj) init (folded inline) plus the exit tail - cl emits the
// implicit leaf vftable (??_7CExitTrigger @0x5e822c) stamp, then raise the bound
// object's logic bit, cache the "A" bute node, snap its screen position, set its
// z-gate (0x124f8) + the four unit bounds, apply the GAME_CYCLE100 geometry, then
// resolve the warlord into the per-area focus slot: when the slot is live, store the
// trigger position, probe the "Warlord" entity, run its finalize fn-ptr, snapshot
// its id, and bind it to the active area + cue. Constructs a throwing CUserBaseLink,
// so MSVC emits the /GX EH frame.
//
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-faithful (the CUserLogic init, the implicit leaf vptr stamp, the "A" cache, the tile
// snaps, the z-gate, the four `=1` bounds, the geometry apply, the focus-slot
// resolve + warlord probe + finalize); the residue is this ctor's own __ehfuncinfo
// state numbering + the zero-register-pinning callee-saved choice (the shared
// CUserLogic-init wall) + the `and al,0xe0` byte-AND codegen pick. The SAME plateau
// as CTimeBomb / the other bute ctors; not source-steerable. Parked for the final sweep.
RVA(0x0003ecf0, 0x292)
CExitTrigger::CExitTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 2;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    if (m_object->m_sortKey != 0x124f8) {
        m_object->m_sortKey = 0x124f8;
        m_object->m_flags |= 0x20000;
    }
    m_object->m_area.left = 1;
    m_object->m_area.right = 1;
    m_object->m_area.top = 1;
    m_object->m_area.bottom = 1;
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_warlordLogic = 0;
    GruntzPlayer* slot = &g_gameReg->m_options[m_object->m_124];
    if (slot->m_liveGate == 0) {
        m_resolved = 0;
        return;
    }
    slot->m_focusX = m_object->m_screenX;
    slot->m_focusY = m_object->m_screenY;
    CGameObject* e =
        g_gameReg->m_world->m_childGroup
            ->CreateSprite(0, m_object->m_screenX, m_object->m_screenY, 0, "Warlord", 0x40003);
    if (e != 0) {
        e->m_124 = m_object->m_124;
        e->m_7c->m_notify(e);
        // snapshot the warlord's bound logic (obj->m_7c->m_logic)
        m_warlordLogic = e->m_7c->m_logic;
        if (m_object->m_124 == g_curPlayer) {
            // track the local player's warlord in the trigger mgr's pending-fx
            // grunt slot (CTmCell == CGrunt; the warlord logic is a grunt leaf)
            g_gameReg->m_cmdGrid->m_pendingFx = static_cast<CTmCell*>(m_warlordLogic);
        }
        GruntzPlayer* slot2 = &g_gameReg->m_options[m_object->m_124];
        if (slot2 != 0) {
            slot2->m_00c = e->m_188;
        }
    }
    m_resolved = 1;
}

// CExitTrigger::SerializeMove (0x3f040), vtable slot 1 - chain the shared serialize
// helper + the +0x34 CSerialObjRef (both gate), then stream the exit state: the
// resolved gate (m_resolved) directly, and the warlord as a persistent id that
// round-trips through the sprite factory's key->object map. Write persists the
// bound warlord's object id (0 if unbound); Read resolves the id back to a live
// object and re-binds its logic. g_serialCounter is bumped each id write.
//
// @early-stop
// ~93.7%: body byte-faithful (both chain gates, the mode-4/7 switch layout, the
// m_resolved stream, the write-side id/counter path, the read-side Lookup + aux
// deref + m_54 store). Residual is MSVC's branchless lowering of the read-side
// `Lookup(key,found) ? found : 0` ternary (retail spells it branchy: test/je/
// mov eax,found) + the mirrored key/found stack-slot assignment (esp+0x1c<->0x20).
// A branch-vs-branchless codegen coin-flip - the permuter found no operand-order
// spelling that flips it (topic:wall topic:regalloc). Deferred to the final sweep.
RVA(0x0003f040, 0x147)
i32 CExitTrigger::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (!CUserLogic::SerializeMove(reinterpret_cast<CSerialArchive*>((reinterpret_cast<i32>(ar))), mode, a3, a4)) {
        return 0;
    }
    CSerialArchive* arc = static_cast<CSerialArchive*>(ar);
    if (!Chain(arc, mode, a3, reinterpret_cast<CGameObject*>(a4))) {
        return 0;
    }

    CDDrawSurfaceMgr* holder = g_gameReg->m_world;
    switch (mode) {
        case 7: {
            arc->Read(&m_resolved, 4);
            i32 key = 0;
            arc->Read(&key, 4);
            if (key != 0) {
                CGameObject* found = 0;
                CGameObject* obj =
                    holder->m_childGroup->m_map48.Lookup(reinterpret_cast<void*>(key), reinterpret_cast<void*&>(found)) ? found : 0;
                m_warlordLogic = obj->m_7c->m_logic;
                if (m_warlordLogic == 0) {
                    return 0;
                }
            } else {
                m_warlordLogic = 0;
            }
            break;
        }
        case 4: {
            arc->Write(&m_resolved, 4);
            if (m_warlordLogic == 0) {
                g_serialCounter++;
                i32 id = 0;
                arc->Write(&id, 4);
            } else {
                g_serialCounter++;
                i32 id = 0;
                if (m_warlordLogic->m_object != 0) {
                    id = m_warlordLogic->m_object->m_188;
                }
                arc->Write(&id, 4);
            }
            break;
        }
    }
    return 1;
}
