// ExitTrigger.cpp - the level-exit trigger tile-logic game-object (C:\Proj\Gruntz).
//
// Two CExitTrigger methods, defined in ascending retail-RVA order:
//   GetTypeTag     @0x010870 - the 6-byte per-class logic-type id accessor (0x3f7).
//   ~CExitTrigger  @0x0108c0 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CExitTrigger : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/ExitTrigger.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/GameRegistry.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0; +0x48 GruntObjMap)
#include <Gruntz/SerialObjRef.h>  // SerialRef34()->Chain (0x8c00) + CSerialArchive Read/Write
#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @+0x2c / Write @+0x30)

// The per-serialize round counter the archive helpers bump (DAT_00629ad0); the
// DATA label is owned by spriteloaders - reference-only here.
extern i32 g_serialCounter; // 0x629ad0

// CExitTrigger::GetTypeTag (0x00010870) is now an inline member in the class header.

// CExitTrigger::~CExitTrigger @0x0108c0 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70;
// the empty body is enough for cl.
RVA(0x000108c0, 0x44)
CExitTrigger::~CExitTrigger() {}

// The global bute store the leaf interns "A" into (g_buteTree @0x6bf620; Find
// 0x16d190 __thiscall ret 4). The "A" key (0x60a454).
#include <Bute/ButeMgr.h>
extern char s_codeA[]; // "A"

// The active-area index (DAT_00644c54): the exit trigger pins the focused warlord
// HUD only for the active area.
extern "C" i32 g_curPlayer;

// The level-exit "Warlord" entity is a fresh CSpriteFactory::CreateSprite result
// (the canonical 0x1597b0 factory entry on g_gameReg->m_world->m_8; the former
// "Probe" reading was a mislabel - it CREATES the warlord head sprite at the bound
// screen pos). The created instance is the shared CGameObject: +0x7c AnimWorkerObj
// carries the Init driver (+0x10, the finalize fn-ptr here) and the per-class
// setup slot m_18 the ctor snapshots raw as the warlord id; m_124 the area/owner
// index, m_188 the object id stored back into the focus slot.

// The focused-warlord cue slot is CFocusSlot, the g_gameReg->m_focusSlots[]
// element (<Gruntz/GameRegistry.h>), indexed by the bound object's area index
// m_124: m_20 the live gate, m_0c the stored id, m_220/m_224 the snapped position.

// The cue receiver (g_gameReg->m_68): +0x2a0 holds the active warlord id.
struct CExitCueSink {
    char m_pad00[0x2a0];
    i32 m_2a0; // +0x2a0
};
SIZE_UNKNOWN(CExitCueSink);

// The game registry singleton (g_gameReg @0x64556c): +0x30 the probe-sink
// holder, +0x68 the cue receiver, the per-area focus slots at +0x150.
SIZE_UNKNOWN(CGameRegistry);
extern "C" CGameRegistry* g_gameReg; // *0x64556c canonical singleton (def: GruntzMgr.cpp)

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
CExitTrigger::CExitTrigger(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->m_flags |= 2;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(s_codeA);
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    if (m_object->m_latchedAnimId != 0x124f8) {
        m_object->m_latchedAnimId = 0x124f8;
        m_object->m_flags |= 0x20000;
    }
    m_object->m_areaL = 1;
    m_object->m_areaR = 1;
    m_object->m_areaT = 1;
    m_object->m_areaB = 1;
    m_savedGeoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_warlordLogic = 0;
    CFocusSlot* slot = &g_gameReg->m_focusSlots[m_object->m_124];
    if (slot->m_20 == 0) {
        m_resolved = 0;
        return;
    }
    slot->m_220 = m_object->m_screenX;
    slot->m_224 = m_object->m_screenY;
    CGameObject* e =
        g_gameReg->m_world->m_8
            ->CreateSprite(0, m_object->m_screenX, m_object->m_screenY, 0, "Warlord", 0x40003);
    if (e != 0) {
        e->m_124 = m_object->m_124;
        e->m_7c->m_notify(e);
        // snapshot the warlord's bound logic (obj->m_7c->m_logic); the cue sink keeps
        // it as a raw DWORD (authentic pointer-as-dword storage)
        m_warlordLogic = e->m_7c->m_logic;
        if (m_object->m_124 == g_curPlayer) {
            ((CExitCueSink*)g_gameReg->m_cmdGrid)->m_2a0 = (i32)m_warlordLogic;
        }
        CFocusSlot* slot2 = &g_gameReg->m_focusSlots[m_object->m_124];
        if (slot2 != 0) {
            slot2->m_0c = e->m_188;
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
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)((i32)ar), mode, a3, a4)) {
        return 0;
    }
    CSerialArchive* arc = (CSerialArchive*)ar;
    if (!SerialRef34()->Chain(arc, mode, a3, (CSerialObj*)a4)) {
        return 0;
    }

    CSpriteFactoryHolder* holder = g_gameReg->m_world;
    switch (mode) {
        case 7: {
            arc->Read(&m_resolved, 4);
            i32 key = 0;
            arc->Read(&key, 4);
            if (key != 0) {
                CGameObject* found = 0;
                CGameObject* obj = holder->m_8->m_objMap.Lookup((void*)key, found) ? found : 0;
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
