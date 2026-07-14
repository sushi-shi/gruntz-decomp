// TileTriggerTransition.h - the CTileTriggerTransition tile-trigger state object
// and its state-machine pump (C:\Proj\Gruntz).
//
// CTileTriggerTransition is a CUserLogic leaf (vftable 0x5e7db4); its ctor/dtor
// live with the rest of the CUserLogic family in src/Gruntz/UserLogic.cpp. This
// header adds the three leaf methods + the controller's state pump that the
// trace lumped under the class but that live in their own retail TU:
//
//   0x011730  GetTypeTag      -> the per-class logic-type id (0x405)
//   0x110070  ApplyAnimation  -> seed geometry/sprite + swap in the "A" bute node
//   0x10d150  StepController   -> the aux-controller state pump (free __cdecl)
//
// The leaf/controller/state-object fields are named from usage; only the OFFSETS
// + the emitted code bytes are load-bearing (campaign doctrine).
#ifndef GRUNTZ_TILETRIGGERTRANSITION_H
#define GRUNTZ_TILETRIGGERTRANSITION_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/XferArchive.h> // the real 0x16e4f0 = ProjTypeXfer(CXferArchive*)

// CButeTree + g_buteTree come via UserLogic.h (pulls <Bute/ButeMgr.h>); the leaf
// data layout (CGameObject/AnimWorkerObj/CUserLogic) is modeled there too.
#include <Gruntz/UserLogic.h>

// The +0x1b4 animation-descriptor collection CGameObject hangs (ApplyAnimation
// walks it, same shape CGrunt's anim resolvers use - see <Gruntz/Grunt.h>):
// element[0]'s +0x14 is the seed frame.
struct CAnimElem {
    char m_pad00[0x14];
    i32 m_14; // +0x14  seed frame
};

// (The former CTileTransitionController/CTileTransitionState pump views are
// DISSOLVED (2026-07-13 worker fold): the controller IS the canonical
// AnimWorkerObj (m_state == m_logic, m_stateId == m_1c) and the state object a
// 16-slot lens of the real CUserLogic vtable (Activate slot 6, the 0x1d/0x1e/
// 0x50..0x53 states = UserLogicVfunc9/8/C/B/A/D at slots 11/10/14/13/12/15).
// The pumps now dispatch the real classes; the default step calls ProjTypeXfer
// (0x16e4f0) directly.)

// ---------------------------------------------------------------------------
// CTileTriggerTransition : CUserLogic (vftable 0x5e7db4) - the CUserLogic
// tile-logic leaf the aux state machine (StepController @0x10d150) builds. Layout
// is plain CUserLogic (0x40) + the leaf tail; the definition lives HERE (not the
// .cpp) so it is a real shared class, not a per-TU view.
//
// vtable slot map (python -m gruntz.analysis.vtable_hierarchy --class
// CTileTriggerTransition; RTTI vtbl 0x1e7db4, 16 slots: 0 new / 4 override / 12
// inherited - transcribed mechanically, NO padding):
//   [0] override  ~dtor (scalar-deleting)            0x0117f0
//   [1] override  SerializeMove                      0x011750
//   [2] override  GetTypeTag                         0x011730
//   [4] override  FireActivation (base slot-4 shape) 0x10fd10
//   the remaining 12 slots are inherited from CUserLogic -> declared NOTHING.
// The int-arg FireActivation cannot spell OVERRIDE against the fat base's no-arg
// slot-4 placeholder (UserLogicVfunc2), so slot 4 is filled by the base-shaped
// UserLogicVfunc2() OVERRIDE + FireActivation stays a plain method (the same
// compromise CTileTrigger / CSpotLight / CLightFx use for their slot-4 body).
class CTileTriggerTransition : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CTileTriggerTransition(CGameObject* obj); // 0x10faf0
    virtual ~CTileTriggerTransition() OVERRIDE;

    // per-class logic-type id (0x405); body out-of-line at 0x011730 in the leaf pool.
    virtual LogicTypeId GetTypeTag() OVERRIDE;
    void Register_10fc90();         // 0x10fc90
    void FireActivation(i32 coord); // 0x10fd10 (vtable slot 4: per-coord PMF dispatch)
    static void RegisterActs();     // 0x10fe70  intern "A", bind Handler (static: no this)
    i32 ApplyAnimation(char* sprite, char* geom); // 0x110070
    i32 Handler_110110();                         // 0x110110  the per-frame handler bound here

    // Leaf fields: CUserLogic ends at +0x40, the leaf object is 0x54 (the size the
    // state pump's `operator new(0x54)` allocates). m_activeAnimDesc caches the
    // +0x1b4 animation descriptor.
    i32 m_activeAnimDesc;      // +0x40
    char m_pad44[0x54 - 0x44]; // +0x44..+0x53
};
VTBL(CTileTriggerTransition, 0x1e7db4);
SIZE_UNKNOWN(CTileTriggerTransition);

// The per-class registry entry: its first dword receives the per-frame handler PMF
// (a 4-byte code pointer on this complete single-inheritance class).
typedef i32 (CTileTriggerTransition::*TileActHandler)();
struct TileActEntry {
    TileActHandler m_fn;
};
SIZE_UNKNOWN(TileActEntry);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_TILETRIGGERTRANSITION_H
