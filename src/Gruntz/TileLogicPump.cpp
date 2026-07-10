// TileLogicPump.cpp - the tile-logic worker-pump family (C:\Proj\Gruntz).
//
// The six free __cdecl /GX state pumps (0x10cb10..0x10d510), one per tile-logic
// leaf, in ascending retail-RVA order. Each is byte-identical to
// CTileTriggerTransition's StepController (0x10d150, src/Gruntz/TileTrigger
// Transition.cpp) bar the leaf TYPE `new`d on state 0: read the controller at
// obj->m_7c, dispatch on its state id, build the leaf on state 0 and dispatch to
// the state object's vtable slots otherwise. Shared via the TILE_LOGIC_WORKER_PUMP
// macro; only the leaf + its `new`-size differ. Re-homed out of the UserLogic
// god-TU; the leaf classes come from their canonical headers.
#include <Gruntz/TileTrigger.h>            // CTileTrigger + the 3 leaves (new-sites)
#include <Gruntz/TileTriggerSwitch.h>      // CTileTriggerSwitch (new-site)
#include <Gruntz/WarpStonePad.h>           // CWarpStonePad (new-site)
#include <Gruntz/TileTriggerTransition.h>  // CTileTransitionController/State + default step
#include <rva.h>

// ---------------------------------------------------------------------------
// Each pump reads the per-object state machine (CTileTransitionController) that
// lives in the bound object's aux sub-object (obj->m_7c) and dispatches on the
// state id: state 0 builds the leaf (operator new(0x54)), Activates it and installs
// it; the 0x1d/0x1e/0x50..0x53 states dispatch to the state object's vtable slots;
// 0x3e8 is idle; anything else runs the default engine step.
// ---------------------------------------------------------------------------
#define TILE_LOGIC_WORKER_PUMP(LEAF)                                                                \
    CTileTransitionController* ctl = (CTileTransitionController*)obj->m_7c;                          \
    switch (ctl->m_stateId) {                                                                        \
        case 0: {                                                                                    \
            ctl->m_stateId = 0x3e8;                                                                  \
            LEAF* t = new LEAF(obj);                                                                 \
            ((CTileTransitionState*)t)->Activate();                                                  \
            ctl->m_state = (CTileTransitionState*)t;                                                 \
            break;                                                                                   \
        }                                                                                            \
        case 0x1d:                                                                                   \
            ctl->m_state->Vfunc2C();                                                                 \
            break;                                                                                   \
        case 0x1e:                                                                                   \
            ctl->m_state->Vfunc28();                                                                 \
            break;                                                                                   \
        case 0x50:                                                                                   \
            ctl->m_state->Vfunc38();                                                                 \
            break;                                                                                   \
        case 0x51:                                                                                   \
            ctl->m_state->Vfunc34();                                                                 \
            break;                                                                                   \
        case 0x52:                                                                                   \
            ctl->m_state->Vfunc30();                                                                 \
            break;                                                                                   \
        case 0x53:                                                                                   \
            ctl->m_state->Vfunc3C();                                                                 \
            break;                                                                                   \
        case 0x3e8:                                                                                  \
            break;                                                                                   \
        default:                                                                                     \
            TileTransitionDefaultStep(ctl->m_state);                                                 \
            break;                                                                                   \
    }                                                                                                \
    return 1;

RVA(0x0010cb10, 0xf1)
i32 TileTriggerStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTrigger)}

RVA(0x0010cc50, 0xf1)
i32 TileTriggerSwitchStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTriggerSwitch)}

RVA(0x0010cd90, 0xf1)
i32 TileSecretTriggerStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileSecretTrigger)}

RVA(0x0010ced0, 0xf1)
i32 GiantRockStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CGiantRock)}

RVA(0x0010d010, 0xf1)
i32 CoveredPowerupStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CCoveredPowerup)}

RVA(0x0010d510, 0xf1)
i32 WarpStonePadStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CWarpStonePad)}
