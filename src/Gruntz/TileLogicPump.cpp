// original TU: filename unknown (@identity-TODO tile-trigger logic module)
//
// waveM-mech merged the tile-trigger logic obj: the 0x10cb10-0x10fac0 .text (plus
// the 0x10f20-0x116c0 low-RVA COMDAT-pool leaves the same obj emits) is ONE original
// TU. Evidence: the CRT $E init-frag run i143-i152 interleaves warpstonepad /
// tiletriggerswitch / tiletrigger / checkpointtrigger / tiletriggertransition
// (impossible across objs at first link), and the six free /GX state pumps + the
// trigger-class bodies + their exiled leaf virtuals all sit in this contiguous band.
//
// Absorbed the ex tilelogicpump (the 6 pumps) + warpstonepad + tiletriggerswitch +
// tiletrigger + checkpointtrigger + tiletriggertransition + cbrickz + logicrecorddispatch's
// LogicDispatchB units. The exiled COMDAT leaves (each class's small slot-1 SerializeMove /
// dtor / no-arg ctor at ~0x10f20-0x117f0) move WITH their class bodies (they are emitted by
// this obj's vtable instantiation) and are laid out first here so the file stays strictly
// RVA-ascending (leaf pool ~0x10f20-0x117f0 all sit below the class band ~0x10cb10-0x110149).
//
// waveM-strays folded the last three frag-woven strays into this obj (their bodies sit
// interleaved WITHIN this obj's contiguous first-link .text block [0x10cb10,0x110149), which
// is impossible across objs at first link -> same obj):
//   - tiletriggertransition (StepController@0x10d150 + CTileTriggerTransition ctor@0x10faf0 +
//     its 0x110110-tail): $E init-frag-proven this obj (runs i143 AND i150 interleave it).
//   - cbrickz (CBrickz ctor@0x10e800 + leaf pool): its ctor sits inside the block; its state
//     pump is LogicDispatchB.
//   - logicrecorddispatch's LogicDispatchB@0x10d3d0: CBrickz's state pump (its state-0 news a
//     CBrickz via ILT thunk 0x3701 -> ctor 0x10e800). Modeled on the real CBrickz (the ex
//
// Only offsets / code bytes are load-bearing; names are placeholders.
#include <Gruntz/GameObjectFactory.h> // C linkage for the definitions below (inherited, not restated)
#include <Gruntz/ActNameRegistry.h> // g_buteTree / s_codeA / g_typeCounter / g_typeColl* / ActNameLookup
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TypeKeyColl.h> // s_codeA/s_actKeyB registration keys
#include <Io/FileMem.h>         // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h>            // CActReg archetype
#include <Gruntz/TileTrigger.h>       // CTileTrigger + the 3 leaves (new-sites)
#include <Gruntz/TileTriggerSwitch.h> // CTileTriggerSwitch (new-site)
#include <Gruntz/WarpStonePad.h>      // CWarpStonePad (new-site)
#include <Gruntz/CheckpointTrigger.h> // CCheckpointTrigger
#include <Gruntz/Play.h> // CPlay (m_curState's real state; m_beginMarker/m_frameMarker)
#include <Gruntz/TileTriggerContainer.h>   // CTileTriggerContainer::FindChild
#include <Gruntz/TileTriggerSwitchLogic.h> // CTileTriggerSwitchLogic (m_linkGate/m_08/m_key0c)
#include <Gruntz/TriggerMgr.h>             // CTriggerMgr::m_grid (the placed-object cells)
#include <Gruntz/MapMgr.h>                 // CMapMgr row table (the packed owner word)
#include <Gruntz/Grunt.h>                  // CGrunt (the grid cell)
#include <Gruntz/Timer.h>                  // CTimer::AddTime
#include <Gruntz/LeafCue.h>                // LeafCue::PlayIfElapsed
#include <Gruntz/GruntSpawnConfig.h>       // CGruntSpawnConfig::SpawnVoiceDriver
#include <Gruntz/GameLevel.h>              // CGameLevel::m_mainPlane
#include <DDrawMgr/DDrawWorkerHost.h>      // CDDrawWorkerHost::m_viewRect
#include <DDrawMgr/DDrawSubMgrLeafScan.h>  // CDDrawSubMgrLeafScan::Lookup
#include <Gruntz/Random.h>                 // g_randSeed / g_randSeeded (the seeded LCG)
#include <Gruntz/SoundState.h>             // g_sndCueTag
#include <Gruntz/Brickz.h>                 // BrickzCell complete (the 0x1c grid cell)
#include <Gruntz/TileTriggerTransition.h>  // CTileTransitionController/State + default step
#include <Gruntz/CBrickz.h>                // CBrickz (ctor + leaf pool; LogicDispatchB new-site)
#include <Gruntz/AniElement.h>             // CAniElement (ApplyAnimation +0x1b4 anim descriptor)
#include <Gruntz/AniAdvanceCursor.h>       // CAniAdvanceCursor (TransitionAct anim sub-object)
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/SerialArchive.h> // CFileMemBase (Read @+0x2c / Write @+0x30)
#include <Gruntz/GameRegistry.h>  // g_gameReg->m_134 (play sub-mode gate in the warp ctor)
#include <string.h>               // memset (inlined rep stosd)
#include <rva.h>
#include <DDrawMgr/AniAdvance.h>  // CAniDesc (the descriptor record)
#include <Image/CImage.h>         // the +0x198 cached frame (ex CGameObjLayer view)
#include <Gruntz/TileLogicPump.h> // CActRegPool<CBrickz>::s_table decl

template<> DATA(0x0024e6a0)
CActReg CActRegPool<CWarpStonePad>::s_table(2000, 2010);
template<> DATA(0x0024e798)
CActReg CActRegPool<CTileTriggerSwitch>::s_table(2000, 2010);
template<> DATA(0x0024e810)
CActReg CActRegPool<CTileTrigger>::s_table(2000, 2010);
template<> DATA(0x0024e7c0)
CActReg CActRegPool<CBrickz>::s_table(2000, 2010);
template<> DATA(0x0024e7e8)
CActReg CActRegPool<CCheckpointTrigger>::s_table(2000, 2010);

VTBL(CWarpStonePad, 0x001e71ac); // vtable_names -> code (RTTI game class)
VTBL(CBrickz, 0x001e7c54);
VTBL(CGiantRock, 0x001e7d5c); // vtable_names -> code (RTTI game class)
VTBL(CTileTriggerTransition, 0x001e7db4);
VTBL(CCoveredPowerup, 0x001e7e0c);    // vtable_names -> code (RTTI game class)
VTBL(CTileSecretTrigger, 0x001e7e64); // vtable_names -> code (RTTI game class)
VTBL(CCheckpointTrigger, 0x001e7ebc);
VTBL(CTileTrigger, 0x001e7f14);
VTBL(CTileTriggerSwitch, 0x001e7f6c); // vtable_names -> code (RTTI game class)

template<> DATA(0x0024e720)
CActReg CActRegPool<CTileTriggerTransition>::s_table(2000, 2010);

#define TILE_LOGIC_WORKER_PUMP(LEAF)                                                               \
    AnimWorkerObj* ctl = obj->m_7c;                                                                \
    switch (static_cast<u32>(ctl->ActKey())) {                                                     \
        case 0: {                                                                                  \
            ctl->SetActKey(0x3e8);                                                                 \
            LEAF* t = new LEAF(obj);                                                               \
            t->Activate();                                                                         \
            ctl->m_logic = t;                                                                      \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            ctl->m_logic->UserLogicVfunc9();                                                       \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            ctl->m_logic->UserLogicVfunc8();                                                       \
            break;                                                                                 \
        case 0x50:                                                                                 \
            ctl->m_logic->UserLogicVfuncC();                                                       \
            break;                                                                                 \
        case 0x51:                                                                                 \
            ctl->m_logic->UserLogicVfuncB();                                                       \
            break;                                                                                 \
        case 0x52:                                                                                 \
            ctl->m_logic->UserLogicVfuncA();                                                       \
            break;                                                                                 \
        case 0x53:                                                                                 \
            ctl->m_logic->UserLogicVfuncD();                                                       \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            ProjTypeXfer(ctl->m_logic);                                                            \
            break;                                                                                 \
    }                                                                                              \
    return 1;

RVA(0x00010f20, 0x47)
i32 CWarpStonePad::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}

// CWarpStonePad::~CWarpStonePad @0x10fc0 - empty vtable-anchor dtor; folds the bare
// CUserLogic teardown (the destructible +0x18 link forces the /GX EH frame).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CWarpStonePad() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00010f90, 0x1e, ??_GCWarpStonePad@@UAEPAXI@Z)
RVA_COMPGEN(0x00010fc0, 0x44, ??1CWarpStonePad@@UAE@XZ)

RVA(0x00011030, 0x6)
LogicTypeId CTileTriggerSwitch::GetTypeTag() {
    return LOGIC_TILETRIGGERSWITCH;
}

RVA(0x00011050, 0x47)
i32 CTileTriggerSwitch::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}

// CTileTriggerSwitch::~CTileTriggerSwitch @0x110f0 - the 0x44 folded CUserLogic teardown.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CTileTriggerSwitch() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x000110c0, 0x1e, ??_GCTileTriggerSwitch@@UAEPAXI@Z)
RVA_COMPGEN(0x000110f0, 0x44, ??1CTileTriggerSwitch@@UAE@XZ)

RVA(0x00011160, 0x4b)
CTileTrigger::CTileTrigger() {}

RVA(0x000111d0, 0x6)
LogicTypeId CTileTrigger::GetTypeTag() {
    return LOGIC_TILETRIGGER;
}

RVA(0x000111f0, 0x47)
i32 CTileTrigger::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}

// ~CTileTrigger is inline (header) so it folds into the three leaf dtors instead of
// being called; MSVC still emits one out-of-line COMDAT copy (called by its scalar-
// deleting dtor) at 0x011290, pinned by mangled name (an inline-defined dtor can't
// hang an RVA()):
RVA_COMPGEN(0x00011260, 0x1e, ??_GCTileTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011290, 0x44, ??1CTileTrigger@@UAE@XZ)

RVA(0x00011320, 0x47)
i32 CBrickz::SerializeMove(CFileMemBase* a, i32 b, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, d) != 0;
}

// --- CBrickz leaf pool (the cbrickz stray, folded waveM-strays) --- ~CBrickz is
// IMPLICIT (retail 0x113c0 is COMPILER-GENERATED). Identity proven by the vtable-owner
// probe (see <Gruntz/MapLogic.h>: ??_7CBrickz @0x1e7c54 slot 0 -> sdd 0x11390 -> 0x113c0;
// the ex-CMapLogic view binding). This TU emits CBrickz vtable/??_G -> the ??1 COMDAT.
// CBrickz::GetTypeTag @0x011300 is header-inline (in <Gruntz/CBrickz.h>).
RVA_COMPGEN(0x00011390, 0x1e, ??_GCBrickz@@UAEPAXI@Z)
RVA_COMPGEN(0x000113c0, 0x44, ??1CBrickz@@UAE@XZ)

// ~CCheckpointTrigger @0x011480 - the bare folded CUserLogic teardown (store the
// CUserLogic vptr, inline-destruct the +0x18 link via ~EngStr, store CUserBase vptr;
// the destructible link forces the /GX EH frame).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CCheckpointTrigger() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA(0x00011430, 0x6)
LogicTypeId CCheckpointTrigger::GetTypeTag() {
    return LOGIC_CHECKPOINTTRIGGER;
}

RVA_COMPGEN(0x00011450, 0x1e, ??_GCCheckpointTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011480, 0x44, ??1CCheckpointTrigger@@UAE@XZ)

// --- CTileTrigger leaf destructors (0x011540 / 0x011600 / 0x0116c0) --- the SAME
// folded CUserLogic teardown (leaf vptr store dead-eliminated).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CTileSecretTrigger() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA(0x000114f0, 0x6)
LogicTypeId CTileSecretTrigger::GetTypeTag() {
    return LOGIC_TILESECRETTRIGGER;
}

RVA_COMPGEN(0x00011510, 0x1e, ??_GCTileSecretTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011540, 0x44, ??1CTileSecretTrigger@@UAE@XZ)

// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGiantRock() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA(0x000115b0, 0x6)
LogicTypeId CGiantRock::GetTypeTag() {
    return LOGIC_GIANTROCK;
}

RVA_COMPGEN(0x000115d0, 0x1e, ??_GCGiantRock@@UAEPAXI@Z)
RVA_COMPGEN(0x00011600, 0x44, ??1CGiantRock@@UAE@XZ)
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CCoveredPowerup() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA(0x00011670, 0x6)
LogicTypeId CCoveredPowerup::GetTypeTag() {
    return LOGIC_COVEREDPOWERUP;
}

RVA_COMPGEN(0x00011690, 0x1e, ??_GCCoveredPowerup@@UAEPAXI@Z)
RVA_COMPGEN(0x000116c0, 0x44, ??1CCoveredPowerup@@UAE@XZ)

// --- CTileTriggerTransition leaf pool (the tiletriggertransition stray, folded waveM-strays)
// --- ~CTileTriggerTransition is IMPLICIT (retail is COMPILER-GENERATED - a user `{}`
// would emit the leaf-vptr restamp now that the CWapX base EH state blocks the old
// dead-store elision). Its out-of-line COMDAT is the 0x117f0 RVA_COMPGEN pin below.

RVA(0x00011730, 0x6)
LogicTypeId CTileTriggerTransition::GetTypeTag() {
    return LOGIC_TILETRIGGERTRANSITION;
}

RVA(0x00011750, 0x47)
i32 CTileTriggerTransition::SerializeMove(
    CFileMemBase* ar,
    i32 mode,
    i32 typeId,
    CGameObject* pObj
) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}

// ~CTileTriggerTransition (0x0117f0) - THIS class's own out-of-line dtor COMDAT, not the
// base's. IDENTITY PROVEN from the binary (vtable-slot chase): the class vtable
// ??_7CTileTriggerTransition @0x1e7db4 holds, at slot 0, an ILT thunk to the scalar-
// deleting dtor 0x117c0, which calls 0x117f0; slot 1 = 0x11750 (SerializeMove, matched
// here) and slot 2 = 0x11730 (GetTypeTag -> LOGIC_TILETRIGGERTRANSITION, matched here).
// So 0x117f0 is ??1CTileTriggerTransition. (It was misbound as ??1CUserLogic; the REAL
// ??1CUserLogic is 0x8860 - ??_7CUserLogic @0x1e705c slot 0 -> sdd 0x8a10 -> 0x8860,
// bound in WorldSoundSet.cpp. MSVC5 keeps ONE COMDAT per name, so the many byte-identical
// empty leaf dtors CANNOT be copies of one ~CUserLogic: each is its own class's dtor.)
// An inline-defined dtor can't hang an RVA() (it would also tag the synthesized ??_G ->
// duplicate-RVA), so it is pinned by mangled name:
RVA_COMPGEN(0x000117c0, 0x1e, ??_GCTileTriggerTransition@@UAEPAXI@Z)
RVA_COMPGEN(0x000117f0, 0x44, ??1CTileTriggerTransition@@UAE@XZ)

RVA(0x0010cb10, 0xf1)
i32 CreateTileTrigger(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTrigger)}

RVA(0x0010cc50, 0xf1)
i32 CreateTileTriggerSwitch(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTriggerSwitch)}

RVA(0x0010cd90, 0xf1)
i32 CreateTileSecretTrigger(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileSecretTrigger)}

RVA(0x0010ced0, 0xf1)
i32 CreateGiantRock(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CGiantRock)}

RVA(0x0010d010, 0xf1)
i32 CreateCoveredPowerup(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CCoveredPowerup)}

RVA(0x0010d150, 0xf1)
i32 StepController(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTriggerTransition)}

RVA(0x0010d290, 0xf4)
i32 CreateCheckpointTrigger(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_7c;
    switch (static_cast<u32>(ctl->ActKey())) {
        case 0: {
            ctl->SetActKey(0x3e8);
            CCheckpointTrigger* t = new CCheckpointTrigger(obj);
            t->Activate();
            ctl->m_logic = t;
            break;
        }
        case 0x1d:
            ctl->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            ctl->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            ctl->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            ctl->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            ctl->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            ctl->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(ctl->m_logic);
            break;
    }
    return 1;
}

RVA(0x0010d3d0, 0xf1)
i32 LogicDispatchB(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CBrickz)}

RVA(0x0010d510, 0xf1)
i32 CreateWarpStonePad(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CWarpStonePad)}

// @early-stop
// eh-ctor-vptr-restamp-position wall, all-inline-base variant (99.65%) - the shared cause of the
// whole CUserLogic+CWapX leaf-ctor family; mechanism + the full list of spellings that do NOT move
// it is on CWayPoint::CWayPoint (src/Gruntz/WayPoint.cpp) and in
// docs/patterns/eh-ctor-vptr-restamp-position.md. One adjacent transposition: cl hoists the body's
// `mov eax,[esi+0x38]` one slot over the leaf vptr stamp; everything else (incl. the folded
// `|= 3`, the g_gameReg->m_134 arm and the "A" re-latch) is byte-identical.
RVA(0x0010d650, 0x16c)
CWarpStonePad::CWarpStonePad(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    if (g_gameReg->m_134 == 1) {
        m_38->m_stateFlags |= 1;
        m_38->m_flags |= 0x10000;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
}

RVA(0x0010d8c0, 0x102)
void CWarpStonePad::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CWarpStonePad>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CWarpStonePad>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

// CWarpStonePad::RegisterActs @0x10da20 - bind AdvanceAnim to key "A".
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
RVA(0x0010da20, 0x18d)
void CWarpStonePad::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CWarpStonePad>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CWarpStonePad::AdvanceAnim);
}

RVA(0x0010dc20, 0x3)
i32 CWarpStonePad::AdvanceAnim() {
    return 0;
}

RVA(0x0010dc40, 0x154)
CTileTriggerSwitch::CTileTriggerSwitch(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    // TWO separate read-modify-writes, not a folded `|= 3`: retail re-reads m_38 and
    // emits `or <r>,ebp(=2)` then `or <r>,ebx(=1)` - the 2 is CSE'd with m_2c = 2.
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    m_38->m_stateFlags |= 1;
}

RVA(0x0010dea0, 0x102)
void CTileTriggerSwitch::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTileTriggerSwitch>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CTileTriggerSwitch>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

// CTileTriggerSwitch::RegisterActs @0x10e000 - bind AdvanceAnim to key "A".
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
RVA(0x0010e000, 0x18d)
void CTileTriggerSwitch::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CTileTriggerSwitch>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CTileTriggerSwitch::AdvanceAnim);
}

RVA(0x0010e200, 0x3)
i32 CTileTriggerSwitch::AdvanceAnim() {
    return 0;
}

RVA(0x0010e220, 0x17d)
CTileTrigger::CTileTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    m_38->m_stateFlags |= 1;
    // the coord seed runs off ONE bound object pointer: that (and only that, out of ~30
    // tail spellings) reproduces retail's callee-saved triple for the three m_38 RMWs
    // above (ecx/ebp/edi with the late `pop edi`) and its tileY-in-edx tail. The shifted
    // coords go through LOCALS - retail reuses them for m_id (`shl eax,8; add eax,edx`).
    CWwdGameObjectA* o = m_object;
    i32 tileX = o->m_screenX >> 5;
    i32 tileY = o->m_screenY >> 5;
    o->m_164 = tileX;
    o->m_168 = tileY;
    o->m_id = (tileX << 8) + tileY;
}

RVA(0x0010e4a0, 0x102)
void CTileTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTileTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CTileTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

// CTileTrigger::RegisterActs (0x10e600) - bind AdvanceAnim to key "A".
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
RVA(0x0010e600, 0x18d)
void CTileTrigger::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CTileTrigger>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CTileTrigger::AdvanceAnim);
}

// CBrickz::CBrickz @0x10e800 (the cbrickz stray, folded waveM-strays) - the 1-arg leaf ctor:
// the standard CUserLogic(obj) init plus the Brickz tail (cache the anim-set node off the "A"
// bute key, raise the logic/collision flag bits, seed the tile-coordinate fields).
// @early-stop
// regalloc: the three m_38 read-modify-writes land in a rotated callee-saved triple
// (retail ecx/ebp/edi with a late `pop edi`, cl edi/edx/ecx with an early one), which
// then flips tileY between edx and ecx in the coord tail. ~30 tail spellings measured;
// only an `o = m_object` local reproduces retail's triple, and that elides the two
// +0x10 reloads retail keeps. Identical body to CTileTrigger @0x10e220.
RVA(0x0010e800, 0x17d)
CBrickz::CBrickz(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    m_38->m_stateFlags |= 1;
    // the coord seed runs off ONE bound object pointer: that (and only that, out of ~30
    // tail spellings) reproduces retail's callee-saved triple for the three m_38 RMWs
    // above (ecx/ebp/edi with the late `pop edi`) and its tileY-in-edx tail. The shifted
    // coords go through LOCALS - retail reuses them for m_id (`shl eax,8; add eax,edx`).
    CWwdGameObjectA* o = m_object;
    i32 tileX = o->m_screenX >> 5;
    i32 tileY = o->m_screenY >> 5;
    o->m_164 = tileX;
    o->m_168 = tileY;
    o->m_id = (tileX << 8) + tileY;
}

// RE-ATTRIBUTED (the ex @identity-TODO shift-by-one, executed): this cluster
// (InitActReg 0x10ea00 / FireActivation 0x10ea80 / RegisterActs 0x10ebe0 +
// CActRegPool<CBrickz>::s_table + Trigger 0x10ede0) is CBRICKZ's, and the NEXT
// cluster (0x10f160/0x10f1e0/0x10f340) is CCHECKPOINTTRIGGER's. Retail proof, read
// two independent ways that agree:
//   vtable_hierarchy (RTTI):  CBrickz[4] override -> 0x0012b2 ; CCheckpointTrigger[4]
//                             override -> 0x001366 ; CTileSecretTrigger[4] INHERITED
//                             -> 0x0034fe (origin CUserLogic)
//   sema xref (jmp graph):    0x0012b2 -> jmp 0x10ea80 ; 0x001366 -> jmp 0x10f1e0 ;
//                             0x0034fe -> jmp 0x10e4a0 (CTileTrigger::FireActivation)
// So CTileSecretTrigger has NO own slot-4 body at all. MSVC5 has no /OPT:ICF, so each
// body has exactly one owner.
//
// CBrickz::InitActReg @0x10ea00 - construct CActRegPool<CBrickz>::s_table over [2000,2010].

RVA(0x0010ea80, 0x102)
void CBrickz::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CBrickz>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CBrickz>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

// CBrickz::RegisterActs (0x10ebe0) - the register-"A"-then-bind archetype.
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
RVA(0x0010ebe0, 0x18d)
void CBrickz::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 cnt = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (cnt-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CBrickz>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CBrickz::Trigger);
}

// CCheckpointTrigger::CCheckpointTrigger(CGameObject*) @0x10ee20 - the 1-arg leaf ctor:
// the standard CUserLogic(obj) init plus the checkpoint tail (leaf vftable stamp, "A"
// cache, two logic bits, z-key recompute, then capture the 15-dword checkpoint state).
RVA(0x0010ede0, 0x3)
i32 CBrickz::Trigger() {
    return 0;
}

RVA(0x0010ee00, 0x3)
i32 CTileTrigger::AdvanceAnim() {
    return 0;
}

// @early-stop
// 94.23 -> 99.87. Three real corrections: the first-empty scan is a flag-terminated
// `while` (not `for`+`break`), its NON-empty arm owns the fallthrough, and the z-key
// block runs off one bound object pointer. The last 3 rows are the two-member
// commutative add - cl canonicalises `A + B` to load the LOWER member offset into the
// accumulator (proven with a standalone cl A/B over 9 spellings; only an intervening
// aliasing STORE defeats it), and retail's is the other way round. Same wall as
// ProbeColumn/ProbeFeetKind/ProbeHeadSoft/HoldMove/SumWeighted.
RVA(0x0010ee20, 0x27d)
CCheckpointTrigger::CCheckpointTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    // the z-key block runs off ONE bound object pointer (retail keeps it in eax across
    // the compare, the store and the flag RMW); re-reading m_object per statement made
    // cl reload +0x10 for the flag and pick the memory-form `or [eax+8],imm`.
    CWwdGameObjectA* o = m_object;
    i32 zk = o->m_layer->m_anchorY + o->m_screenY + 0x186a0;
    if (o->m_sortKey != zk) {
        o->m_sortKey = zk;
        o->m_flags |= 0x20000;
    }
    memset(m_state, 0, sizeof(m_state));
    if (m_object->m_extent.left == 0x80000000) {
        m_object->m_extent.left = 0;
    }
    if (m_object->m_area.left == 0x80000000) {
        m_object->m_area.left = 0;
    }
    if (m_object->m_switchRect.left == 0x80000000) {
        m_object->m_switchRect.left = 0;
    }
    if (m_object->m_clip.left == 0x80000000) {
        m_object->m_clip.left = 0;
    }
    m_state[0] = m_object->m_extent.left;
    m_state[1] = m_object->m_extent.top;
    m_state[2] = m_object->m_extent.right;
    m_state[3] = m_object->m_extent.bottom;
    m_state[4] = m_object->m_area.left;
    m_state[5] = m_object->m_area.top;
    m_state[6] = m_object->m_area.right;
    m_state[7] = m_object->m_area.bottom;
    m_state[8] = m_object->m_switchRect.left;
    m_state[9] = m_object->m_switchRect.top;
    m_state[10] = m_object->m_switchRect.right;
    m_state[11] = m_object->m_switchRect.bottom;
    m_state[12] = m_object->m_clip.left;
    m_state[13] = m_object->m_clip.top;
    m_state[14] = m_object->m_clip.right;
    // the scan is a flag-terminated `while`, not a `for`+`break`: retail tests the
    // bound at the TOP (`jge` out) and the flag on the BACK EDGE (`cmp ecx,0 / je top`),
    // which is what cl emits for `while (found == 0 && i < 15)` once it proves found==0
    // on entry. The `for`/`break` spelling folded both tests into the bottom.
    i32 found = 0;
    m_firstEmpty = 0;
    while (found == 0 && m_firstEmpty < 15) {
        // the NON-empty arm owns the fallthrough (retail `je <found> / inc`), so the
        // test is spelled `!= 0`, not `== 0` with an else.
        if (m_state[m_firstEmpty] != 0) {
            m_firstEmpty++;
        } else {
            found = 1;
        }
    }
}

RVA(0x0010f1e0, 0x102)
void CCheckpointTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CCheckpointTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CCheckpointTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

// CCheckpointTrigger::RegisterActs (0x10f340) - intern "A" and "B", bind each handler.
// Two-key registrar: cl5 spends its inline budget from the outside in, so only the
// SECOND key's name lookup expands the grow-fail report; the other three lookups keep
// it as the out-of-line zErrHandling::Report call.
// docs/patterns/act-registrar-report-outline-budget.md
RVA(0x0010f340, 0x2ac)
void CCheckpointTrigger::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CCheckpointTrigger>::s_table.ResolveEntryCallReport(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CCheckpointTrigger::Act);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "B";
        g_typeCounter++;
    }
    (*((CActRegPool<CCheckpointTrigger>::s_table.ResolveEntryCallReport(id2)))) =
        static_cast<i32 (CUserLogic::*)()>(&CCheckpointTrigger::Act_10f970);
}

// 0x10f6a0 (565 B) = CCheckpointTrigger's "A" activation handler: the CHECKPOINT
// REACHED sequence. Gate on every recorded switch in m_state being present and
// linked, re-latch the flag's anim-set + geometry to the raised-flag set, credit the
// level timer with the object's bonus time, play the GAME_FLAGRISE cue, tell the
// manager a checkpoint was reached, then pick one of the recorded switches AT RANDOM
// and - if the grunt standing on it is on-screen - fire its 0x334 voice line.
// @confidence: high
// @source: vtable-slot+pmf (RegisterActs binds this RVA as the "A" handler) +
//   full-disasm-decode (every callee named)
// @early-stop
// 97.3% (from a 1.1% stub). Control flow, all ten callees, the tile-grid *7-int walk,
// the m_grid row*15+col index, the inlined seeded-LCG coin flip and the four
// view-rect gates are byte-faithful. Two residues: (a) the "B" act key - retail
// references the .data array at 0x60d1bc (?s_actKeyB@@3PADA), which has no definition
// anywhere in the tree, so this uses the literal and one reloc TARGET NAME differs;
// (b) a two-instruction scheduling slip where cl sinks the `mov ecx,g_gameReg` for the
// Rand() call past the m_firstEmpty load.
RVA(0x0010f6a0, 0x235)
i32 CCheckpointTrigger::Act() {
    CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);

    for (i32 i = 0; i < m_firstEmpty; i++) {
        i32 key = m_state[i];
        if (key == 0) {
            return 0;
        }
        CTileTriggerSwitchLogic* child = play->m_beginMarker->FindChild(key, 8);
        if (child == 0) {
            g_gameReg->ReportError(0x80dd, 0x44c);
            return 0;
        }
        if (child->m_linkGate == 0) {
            return 0;
        }
    }

    m_prevAnimSetNode = m_objAux->m_1c;
    // retail references the .data act-key array at 0x60d1bc (?s_actKeyB@@3PADA),
    // which has no definition in the tree yet - the literal is byte-equivalent apart
    // from that one reloc target name.
    m_objAux->m_1c = ActFindId("B");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CHECKPOINTFLAGSET", 0);

    if (play->m_frameMarker != 0) {
        i32 a = m_object->m_114;
        i32 b = m_object->m_118;
        if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
            b += b;
            a += a;
            if (b > 0x3b) {
                a++;
                b -= 0x3c;
            }
        }
        play->m_frameMarker->AddTime(a, b);
    }

    CObject* cue = m_38->OwnerMgr()->m_soundRegistry->Lookup("GAME_FLAGRISE");
    if (cue != 0) {
        static_cast<LeafCue*>(cue)->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
    }
    g_gameReg->OnCheckpointReached();

    // Pick one recorded switch uniformly. Retail hand-inlines the seeded LCG for the
    // empty-span arm (the CRandomAmbientSound::Init2 idiom): coin-flip the endpoints.
    i32 hi = m_firstEmpty - 1;
    // the manager goes through a local: retail's `mov ecx,g_gameReg` for Rand() sits
    // ABOVE the span test (reading the global inside the else arm sinks it past it)
    CGruntzMgr* reg = g_gameReg;
    i32 span = hi + 1;
    i32 pick;
    if (span == 0) {
        i32 seed;
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_randSeed;
        }
        g_randSeed = seed * 214013 + 2531011;
        if (g_randSeed & 0x10000) {
            pick = 0;
        } else {
            pick = hi;
        }
    } else {
        pick = reg->Rand() % span;
    }

    CTileTriggerSwitchLogic* pad = play->m_beginMarker->FindChild(m_state[pick], 8);
    if (pad == 0) {
        g_gameReg->ReportError(0x80dd, 0x44c);
        return 0;
    }

    i32 gy = pad->m_key0c;
    i32 gx = pad->m_08;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    i32 owner;
    if (static_cast<u32>(gx) < grid->m_width && static_cast<u32>(gy) < grid->m_height) {
        owner = grid->m_rows[gy][gx].m_4;
    } else {
        owner = -1;
    }
    if (owner == -1) {
        return 0;
    }

    // the row is masked IN PLACE (retail `mov cl,ah` then `and eax,0xff`); masking into a
    // second local makes cl copy `owner` aside first and copy the column back afterwards
    i32 ownerCol = (owner >> 8) & 0xff;
    owner &= 0xff;
    CGrunt* g = g_gameReg->m_cmdGrid->m_grid[ownerCol * TM_GRID_COLS + owner];
    if (g == 0) {
        return 0;
    }

    i32 sy = g->m_object->m_screenY;
    i32 sx = g->m_object->m_screenX;
    RECT* view = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
    if (sx >= view->right) {
        return 0;
    }
    if (sx < view->left) {
        return 0;
    }
    if (sy >= view->bottom) {
        return 0;
    }
    if (sy < view->top) {
        return 0;
    }
    g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x334, -1, 0, -1, -1);
    return 0;
}

RVA(0x0010f970, 0x17)
i32 CCheckpointTrigger::Act_10f970() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x0010f9a0, 0x8f)
i32 CCheckpointTrigger::SerializeMove(CFileMemBase* arc, i32 mode, i32 typeId, CGameObject* pObj) {
    CFileMemBase* sa = static_cast<CFileMemBase*>(arc);
    switch (mode) {
        case 7:
            sa->Read(m_state, 0x3c);
            sa->Read(&m_firstEmpty, 4);
            break;
        case 4:
            sa->Write(m_state, 0x3c);
            sa->Write(&m_firstEmpty, 4);
            break;
    }
    if (!CUserLogic::SerializeMove(arc, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(sa, mode, typeId, pObj) ? 1 : 0;
}

RVA(0x0010fa60, 0x19)
CTileSecretTrigger::CTileSecretTrigger(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fa90, 0x19)
CGiantRock::CGiantRock(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fac0, 0x19)
CCoveredPowerup::CCoveredPowerup(CGameObject* obj) : CTileTrigger(obj) {}

// CTileTriggerTransition::CTileTriggerTransition (0x10faf0) - the 1-arg leaf ctor: fold the
// shared CUserLogic(obj) init then stamp the leaf vptr (0x5e7db4) + the +0x1000000 object
// flag / +0x74 type write the original tail does.
RVA(0x0010faf0, 0x128)
CTileTriggerTransition::CTileTriggerTransition(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 0x1000000;
    // ONE m_object load into a local: that is what lets cl hoist the m_flags read above
    // the m_sortKey store (`mov ecx,[eax+8]` / `or ecx,imm` / `mov [eax+8],ecx`).
    CGameObject* o = m_object;
    if (o->m_sortKey != 0) {
        o->m_sortKey = 0;
        o->m_flags |= 0x20000;
    }
}

RVA(0x0010fd10, 0x102)
void CTileTriggerTransition::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTileTriggerTransition>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CTileTriggerTransition>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

// RegisterActs (0x10fe70) - intern this class's activation key "A" into the shared bute-tree
// name map, then bind that id to this class's per-frame handler (TransitionAct).
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
RVA(0x0010fe70, 0x18d)
void CTileTriggerTransition::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CTileTriggerTransition>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CTileTriggerTransition::TransitionAct);
}

RVA(0x00110070, 0x71)
i32 CTileTriggerTransition::ApplyAnimation(char* sprite, char* geom) {
    m_value = m_38->m_1a0.m_14;
    if (m_38->ApplyLookupGeometry(geom, 0) == 0) {
        return 0;
    }
    CAniElement* desc = m_38->m_1a0.m_14;
    CAniDesc* elem =
        desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
    m_38->ApplyLookupSprite(sprite, elem->m_param);
    m_prevAnimSetNode = m_objAux->m_1c; // save the prev anim-set node (CUserLogic base field)
    m_objAux->m_1c = ActFindId("A");
    return 1;
}

RVA(0x00110110, 0x39)
i32 CTileTriggerTransition::TransitionAct() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    if (m_38->m_1a0.m_28 != 0 && m_38->m_1a0.m_20 == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}
