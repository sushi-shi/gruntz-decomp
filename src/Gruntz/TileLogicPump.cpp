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
#include <Gruntz/ActNameRegistry.h> // g_buteTree / s_codeA / g_typeCounter / g_typeColl* / ActNameLookup
#include <Gruntz/GameRegPtr.h>
#include <Gruntz/TypeKeyColl.h> // s_codeA/s_actKeyB registration keys
#include <Io/FileMem.h>         // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/ActReg.h>                // CActReg archetype
#include <Gruntz/TileTrigger.h>           // CTileTrigger + the 3 leaves (new-sites)
#include <Gruntz/TileTriggerSwitch.h>     // CTileTriggerSwitch (new-site)
#include <Gruntz/WarpStonePad.h>          // CWarpStonePad (new-site)
#include <Gruntz/CheckpointTrigger.h>     // CCheckpointTrigger
#include <Gruntz/TileTriggerTransition.h> // CTileTransitionController/State + default step
#include <Gruntz/CBrickz.h>               // CBrickz (ctor + leaf pool; LogicDispatchB new-site)
#include <Gruntz/AniElement.h>            // CAniElement (ApplyAnimation +0x1b4 anim descriptor)
#include <Gruntz/AniAdvanceCursor.h>      // CAniAdvanceCursor (Handler_110110 anim sub-object)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/SerialArchive.h>         // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Gruntz/GameRegistry.h>          // g_gameReg->m_134 (play sub-mode gate in the warp ctor)
#include <string.h>                       // memset (inlined rep stosd)
#include <rva.h>
#include <DDrawMgr/AniAdvance.h> // CAniDesc (the descriptor record)
#include <Image/CImage.h> // the +0x198 cached frame (ex CGameObjLayer view)

// The activation key "B" (0x60d1bc) CTileSecretTrigger's second registration interns;
// s_codeA/g_buteTree/g_typeCounter come from <Gruntz/ActNameRegistry.h>.
// @undefined-data: a char[] datum here is a STRING (or a run of them); its
// extent is not boundable from the named-symbol gaps (the unnamed $SG literals
// in between get swallowed). Inline the literal at its use site instead.

// The game registry singleton the warp ctor polls for the play sub-mode (m_134).

// --- The per-class activation-coordinate registry singletons + handler-entry views.
// Each ActReg is the shared <Gruntz/ActReg.h> CActReg archetype (distinct instance);
// each ActEntry's first dword is the per-frame handler PMF (4-byte code ptr). -------

// CWarpStonePad's registry (@0x64e6a0, range [2000,2010]); the ActEntry record
// lives with the class (<Gruntz/WarpStonePad.h>).
// to give this global a distinct type name. The variable name already makes the mangled
// symbol unique, and DATA() rebinds it - so the archetype IS the type.)
DATA(0x0024e6a0)
CActReg g_warpStonePadActReg; // 0x64e6a0

// CTileTriggerSwitch's registry (@0x64e798); ActEntry in <Gruntz/TileTriggerSwitch.h>.
DATA(0x0024e798)
CActReg g_tileTriggerSwitchActReg; // 0x64e798

// CCheckpointTrigger's registry (@0x64e7c0; entry record in <Gruntz/CheckpointTrigger.h>).
DATA(0x0024e7c0)
CActReg g_brickzActReg; // 0x64e7c0 (ex "g_checkpointActReg" - the act clusters here
                        // were shifted one class down; see the note at 0x10ea00)

// CTileTrigger / CTileSecretTrigger's registries (@0x64e810 / @0x64e7e8).
DATA(0x0024e810)
CActReg g_tileTriggerActReg; // 0x64e810
DATA(0x0024e7e8)
CActReg g_checkpointActReg; // 0x64e7e8 (ex "g_tileSecretTriggerActReg" - the shift)
// (TileTrigger/TileSecretTrigger ActEntry records live in <Gruntz/TileTrigger.h>.)

// --- CTileTriggerTransition (the tiletriggertransition stray, folded waveM-strays) -----------

// The per-frame draw-delta mirror (_g_6bf3bc); the value-load reloc-masks.
extern "C" u32 g_engineFrameDelta;

// The CTileTriggerTransition activation-coordinate registry @0x64e720: the fixed
// [0x7d0, 0x7da] (== [2000, 2010]) range built by the shared registry ctor (0x408710).
// TileActReg is the shared <Gruntz/ActReg.h> CActReg archetype; it keeps its own placeholder
// name so the DATA-pinned global symbol is unchanged.
DATA(0x0024e720)
CActReg g_tileActReg;

// CTileTriggerTransition (the CUserLogic leaf the state machine builds), its
// vtable slot map, and its TileActEntry PMF holder now live in the shared header
// <Gruntz/TileTriggerTransition.h> (included above) - it is a real class, not a
// per-TU view. GruntVoice/StatusBarSpriteActs already include that header (they
// don't use the class, so it emits no code there - a class decl is matching-neutral).

// ---------------------------------------------------------------------------
// The per-object tile-logic state pump: read the per-object state machine
// (CTileTransitionController) at obj->m_7c and dispatch on its state id - state 0
// builds the leaf (operator new(0x54)), Activates it and installs it; the 0x1d/0x1e/
// 0x50..0x53 states dispatch to the state object's vtable slots; 0x3e8 is idle;
// anything else runs the default engine step.
// ---------------------------------------------------------------------------
#define TILE_LOGIC_WORKER_PUMP(LEAF)                                                               \
    AnimWorkerObj* ctl = obj->m_7c;                                                                \
    switch (reinterpret_cast<u32>(ctl->m_1c)) {                                                                      \
        case 0: {                                                                                  \
            ctl->m_1c = reinterpret_cast<void*>(0x3e8);                                                              \
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
            ProjTypeXfer(reinterpret_cast<CXferArchive*>(ctl->m_logic));                                             \
            break;                                                                                 \
    }                                                                                              \
    return 1;

// ===========================================================================
// Exiled COMDAT-pool leaves (each class's small slot-1 SerializeMove / no-arg ctor /
// dtor), ascending in the ~0x10f20-0x116c0 pool region - laid out first so the whole
// file stays RVA-ascending (they all sit below the 0x10cb10 class band).
// ===========================================================================

// CWarpStonePad::SerializeMove @0x10f20, vtable slot 1 - chain the shared serialize
// helper on `this`, then (only on success) the +0x34 CSerialObjRef sub-object.
RVA(0x00010f20, 0x47)
i32 CWarpStonePad::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (!CUserLogic::SerializeMove(reinterpret_cast<CSerialArchive*>((reinterpret_cast<i32>(ar))), mode, a3, a4)) {
        return 0;
    }
    return Chain(static_cast<CSerialArchive*>(ar), mode, a3, reinterpret_cast<CGameObject*>(a4)) != 0;
}

// CWarpStonePad::~CWarpStonePad @0x10fc0 - empty vtable-anchor dtor; folds the bare
// CUserLogic teardown (the destructible +0x18 link forces the /GX EH frame).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CWarpStonePad() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CWarpStonePad@@UAE@XZ 0x00010fc0 0x44

// CTileTriggerSwitch::SerializeMove @0x11050, vtable slot 1.
RVA(0x00011050, 0x47)
i32 CTileTriggerSwitch::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (!CUserLogic::SerializeMove(reinterpret_cast<CSerialArchive*>((reinterpret_cast<i32>(ar))), mode, a3, a4)) {
        return 0;
    }
    return Chain(static_cast<CSerialArchive*>(ar), mode, a3, reinterpret_cast<CGameObject*>(a4)) != 0;
}

// CTileTriggerSwitch::~CTileTriggerSwitch @0x110f0 - the 0x44 folded CUserLogic teardown.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CTileTriggerSwitch() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CTileTriggerSwitch@@UAE@XZ 0x000110f0 0x44

// @interleaver CTileTrigger ctor in tilelogicpump's own 0x11xxx block - KEEP (correctly placed)
// (REHOME D10: flag_outliers marks 0x11160 a lone "misplaced" outlier (home_n=1, weak), but
// that is a dtor/virtual-exclusion ARTIFACT: this ctor is surrounded by tilelogicpump on BOTH
// sides - ~CTileTriggerSwitch @0x110f0 (before) + CTileTrigger::SerializeMove @0x111f0 (after),
// all tilelogicpump. It heads this TU's OWN low tile-trigger-leaf ctor/dtor/serialize block,
// linker-separated from the 0x10cb10 pump-logic block. Own-unit, not foreign; leave in place.)
RVA(0x00011160, 0x4b)
CTileTrigger::CTileTrigger() {}

// --- CTileTrigger::SerializeMove (0x111f0), vtable slot 1 --- base impl shared
// (inherited) by CGiantRock/CCoveredPowerup/CTileSecretTrigger (no leaf override).
RVA(0x000111f0, 0x47)
i32 CTileTrigger::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (!CUserLogic::SerializeMove(reinterpret_cast<CSerialArchive*>((reinterpret_cast<i32>(ar))), mode, a3, a4)) {
        return 0;
    }
    return Chain(static_cast<CSerialArchive*>(ar), mode, a3, reinterpret_cast<CGameObject*>(a4)) != 0;
}

// ~CTileTrigger is inline (header) so it folds into the three leaf dtors instead of
// being called; MSVC still emits one out-of-line COMDAT copy (called by its scalar-
// deleting dtor) at 0x011290, pinned by mangled name (an inline-defined dtor can't
// hang an RVA()):
// @rva-symbol: ??1CTileTrigger@@UAE@XZ 0x00011290 0x44

// --- CBrickz leaf pool (the cbrickz stray, folded waveM-strays) --- ~CBrickz is
// IMPLICIT (retail 0x113c0 is COMPILER-GENERATED). Identity proven by the vtable-owner
// probe (see <Gruntz/MapLogic.h>: ??_7CBrickz @0x1e7c54 slot 0 -> sdd 0x11390 -> 0x113c0;
// the ex-CMapLogic view binding). This TU emits CBrickz vtable/??_G -> the ??1 COMDAT.
// CBrickz::GetTypeTag @0x011300 is header-inline (in <Gruntz/CBrickz.h>).
// @rva-symbol: ??1CBrickz@@UAE@XZ 0x000113c0 0x44

// CBrickz::Serialize @0x011320 - vtable slot 1: chain the shared CUserLogic serialize
// helper on `this`, then (on success) the +0x34 sub-object's chain, normalized to a
// strict bool. Byte-identical to CSecretTeleporterTrigger::Serialize.
RVA(0x00011320, 0x47)
i32 CBrickz::SerializeMove(CGruntArchive* a, i32 b, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// ~CCheckpointTrigger @0x011480 - the bare folded CUserLogic teardown (store the
// CUserLogic vptr, inline-destruct the +0x18 link via ~EngStr, store CUserBase vptr;
// the destructible link forces the /GX EH frame).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CCheckpointTrigger() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CCheckpointTrigger@@UAE@XZ 0x00011480 0x44

// --- CTileTrigger leaf destructors (0x011540 / 0x011600 / 0x0116c0) --- the SAME
// folded CUserLogic teardown (leaf vptr store dead-eliminated).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CTileSecretTrigger() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CTileSecretTrigger@@UAE@XZ 0x00011540 0x44
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGiantRock() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CGiantRock@@UAE@XZ 0x00011600 0x44
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CCoveredPowerup() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CCoveredPowerup@@UAE@XZ 0x000116c0 0x44

// --- CTileTriggerTransition leaf pool (the tiletriggertransition stray, folded waveM-strays)
// --- ~CTileTriggerTransition is IMPLICIT (retail is COMPILER-GENERATED - a user `{}`
// would emit the leaf-vptr restamp now that the CWapX base EH state blocks the old
// dead-store elision). Its out-of-line COMDAT is the 0x117f0 @rva-symbol pin below.

// CTileTriggerTransition::GetTypeTag (0x011730) - per-class logic-type id (0x405). Out-of-line
// here so its RVA lands in leaf-pool order (one deduped COMDAT copy in retail).
RVA(0x00011730, 0x6)
LogicTypeId CTileTriggerTransition::GetTypeTag() {
    return LOGIC_TILETRIGGERTRANSITION;
}

// CTileTriggerTransition::SerializeMove (0x11750), vtable slot 1 - the
// CSecretTeleporterTrigger::Serialize archetype (chain + +0x34 CSerialObjRef gate).
RVA(0x00011750, 0x47)
i32 CTileTriggerTransition::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (!CUserLogic::SerializeMove(reinterpret_cast<CSerialArchive*>((reinterpret_cast<i32>(ar))), mode, a3, a4)) {
        return 0;
    }
    return Chain(static_cast<CSerialArchive*>(ar), mode, a3, reinterpret_cast<CGameObject*>(a4)) != 0;
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
// @rva-symbol: ??1CTileTriggerTransition@@UAE@XZ 0x000117f0 0x44

// ===========================================================================
// The class band, ascending in the 0x10cb10-0x110149 region.
// ===========================================================================

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

// StepController @0x10d150 (the tiletriggertransition stray, folded waveM-strays) - the aux
// state pump that builds CTileTriggerTransition (0x54) for state 0 then dispatches by state
// id to the state object's vtable slots. Same pump shape as the sibling steps.
RVA(0x0010d150, 0xf1)
i32 StepController(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTriggerTransition)}

// CheckpointTriggerStep @0x10d290 - byte-identical to StepController bar the leaf TYPE
// (CCheckpointTrigger is 0x94, so `new` pushes 0x94 imm32 - the 3-byte-longer body).
RVA(0x0010d290, 0xf4)
i32 CheckpointTriggerStep(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_7c;
    switch (reinterpret_cast<u32>(ctl->m_1c)) {
        case 0: {
            ctl->m_1c = reinterpret_cast<void*>(0x3e8);
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
            ProjTypeXfer(reinterpret_cast<CXferArchive*>(ctl->m_logic));
            break;
    }
    return 1;
}

// LogicDispatchB @0x10d3d0 (the logicrecorddispatch stray, folded waveM-strays) - CBrickz's
// state pump: state 0 news a CBrickz (0x54; retail calls it through ILT thunk 0x3701 ->
// onto the real CBrickz. Was ?LogicDispatchB@@YAHPAULogicDispatchOwner@@@Z (the placeholder
// owner view); its owner is a CGameObject like every other pump.
RVA(0x0010d3d0, 0xf1)
i32 LogicDispatchB(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CBrickz)}

RVA(0x0010d510, 0xf1)
i32 WarpStonePadStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CWarpStonePad)}

// --- CWarpStonePad (0x10d650), vptr 0x5e71ac --- the ctor anchors GetTypeTag @0x10f00
// + the ??_7CWarpStonePad vtable in this TU. Folds the inline CUserLogic(obj) base.
RVA(0x0010d650, 0x16c)
CWarpStonePad::CWarpStonePad(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    if (g_gameReg->m_134 == 1) {
        m_38->m_stateFlags |= 1;
        m_38->m_flags |= 0x10000;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
}

// CWarpStonePad::InitActReg @0x10d840 - construct g_warpStonePadActReg over [2000,2010].
RVA(0x0010d840, 0x15)
void CWarpStonePad::InitActReg() {
    (reinterpret_cast<CZDArrayDerived*>(&g_warpStonePadActReg))->Construct(2000, 2010);
}

// CWarpStonePad::FireWarp @0x10d8c0, vtable slot 4 - resolve the coordinate; if the
// entry carries a handler PMF, re-resolve and dispatch it __thiscall on this.
RVA(0x0010d8c0, 0x102)
void CWarpStonePad::FireActivation(i32 coord) {
    CWarpStonePadActEntry* e = reinterpret_cast<CWarpStonePadActEntry*>(g_warpStonePadActReg.ResolveEntry(coord));
    if (e->m_fn != 0) {
        CWarpStonePadActEntry* e2 =
            reinterpret_cast<CWarpStonePadActEntry*>(g_warpStonePadActReg.ResolveEntry(coord));
        (this->*(e2->m_fn))();
    }
}

// CWarpStonePad::RegisterActs @0x10da20 - bind AdvanceAnim to key "A".
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful; residual is the slot-vs-id callee-saved register choice cascading
// into the free-loop count materialization. Deferred.
RVA(0x0010da20, 0x18d)
void CWarpStonePad::RegisterActs() {
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
    (reinterpret_cast<CWarpStonePadActEntry*>(g_warpStonePadActReg.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CWarpStonePad::AdvanceAnim);
}

// --- CTileTriggerSwitch (0x10dc40), vptr 0x5e7f6c --- ctor anchors the vtable.
RVA(0x0010dc40, 0x154)
CTileTriggerSwitch::CTileTriggerSwitch(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    m_38->m_stateFlags |= 1;
}

// CTileTriggerSwitch::InitActReg @0x10de20 - construct g_tileTriggerSwitchActReg.
RVA(0x0010de20, 0x15)
void CTileTriggerSwitch::InitActReg() {
    (reinterpret_cast<CZDArrayDerived*>(&g_tileTriggerSwitchActReg))->Construct(2000, 2010);
}

// CTileTriggerSwitch::FireActivation @0x10dea0, vtable slot 4.
RVA(0x0010dea0, 0x102)
void CTileTriggerSwitch::FireActivation(i32 coord) {
    CTileTriggerSwitchActEntry* e =
        reinterpret_cast<CTileTriggerSwitchActEntry*>(g_tileTriggerSwitchActReg.ResolveEntry(coord));
    if (e->m_fn != 0) {
        CTileTriggerSwitchActEntry* e2 =
            reinterpret_cast<CTileTriggerSwitchActEntry*>(g_tileTriggerSwitchActReg.ResolveEntry(coord));
        (this->*(e2->m_fn))();
    }
}

// CTileTriggerSwitch::RegisterActs @0x10e000 - bind AdvanceAnim to key "A".
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful; residual is the slot-vs-id callee-saved register choice. Deferred.
RVA(0x0010e000, 0x18d)
void CTileTriggerSwitch::RegisterActs() {
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
    (reinterpret_cast<CTileTriggerSwitchActEntry*>(g_tileTriggerSwitchActReg.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CTileTriggerSwitch::AdvanceAnim);
}

// --- CTileTrigger 1-arg (0x10e220), vptr 0x5e7f14 ---
RVA(0x0010e220, 0x17d)
CTileTrigger::CTileTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    m_38->m_stateFlags |= 1;
    m_object->m_164 = m_object->m_screenX >> 5;
    m_object->m_168 = m_object->m_screenY >> 5;
    m_object->m_04 = (m_object->m_164 << 8) + m_object->m_168;
}

// CTileTrigger::InitActReg (0x10e420) - construct g_tileTriggerActReg over [2000,2010].
RVA(0x0010e420, 0x15)
void CTileTrigger::InitActReg() {
    (reinterpret_cast<CZDArrayDerived*>(&g_tileTriggerActReg))->Construct(2000, 2010);
}

// CTileTrigger::FireActivation (0x10e4a0), vtable slot 4.
RVA(0x0010e4a0, 0x102)
void CTileTrigger::FireActivation(i32 coord) {
    CTileTriggerActEntry* e = reinterpret_cast<CTileTriggerActEntry*>(g_tileTriggerActReg.ResolveEntry(coord));
    if (e->m_fn != 0) {
        CTileTriggerActEntry* e2 = reinterpret_cast<CTileTriggerActEntry*>(g_tileTriggerActReg.ResolveEntry(coord));
        (this->*(e2->m_fn))();
    }
}

// CTileTrigger::RegisterActs (0x10e600) - bind AdvanceAnim to key "A".
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful; residual is the slot-vs-id callee-saved register choice. Deferred.
RVA(0x0010e600, 0x18d)
void CTileTrigger::RegisterActs() {
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
    (reinterpret_cast<CTileTriggerActEntry*>(g_tileTriggerActReg.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CTileTrigger::AdvanceAnim);
}

// CBrickz::CBrickz @0x10e800 (the cbrickz stray, folded waveM-strays) - the 1-arg leaf ctor:
// the standard CUserLogic(obj) init plus the Brickz tail (cache the anim-set node off the "A"
// bute key, raise the logic/collision flag bits, seed the tile-coordinate fields).
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-identical (the CUserLogic init, the "A" anim-set cache, the two separate m_38->m_08
// RMW + the m_38->m_40 bit, the m_164/m_168/m_04 tile-coord seed); the residue is this ctor's
// own __ehfuncinfo + a 1-slot pop-edi scheduling delta in the tail. Not source-steerable; ~88%.
RVA(0x0010e800, 0x17d)
CBrickz::CBrickz(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    m_38->m_stateFlags |= 1;
    m_object->m_164 = m_object->m_screenX >> 5;
    m_object->m_168 = m_object->m_screenY >> 5;
    m_object->m_04 = (m_object->m_164 << 8) + m_object->m_168;
}

// RE-ATTRIBUTED (the ex @identity-TODO shift-by-one, executed): this cluster
// (InitActReg 0x10ea00 / FireActivation 0x10ea80 / RegisterActs 0x10ebe0 +
// g_brickzActReg + CBrickzActEntry + Trigger 0x10ede0) is CBRICKZ's, and the NEXT
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
// CBrickz::InitActReg @0x10ea00 - construct g_brickzActReg over [2000,2010].
RVA(0x0010ea00, 0x15)
void CBrickz::InitActReg() {
    (reinterpret_cast<CZDArrayDerived*>(&g_brickzActReg))->Construct(2000, 2010);
}

// CBrickz::FireActivation (0x10ea80), vtable slot 4 - the double-ResolveEntry
// + PMF-fire archetype (ResolveEntry has side effects, so re-run for the actual call).
RVA(0x0010ea80, 0x102)
void CBrickz::FireActivation(i32 coord) {
    CBrickzActEntry* e = reinterpret_cast<CBrickzActEntry*>(g_brickzActReg.ResolveEntry(coord));
    if (e->m_fn != 0) {
        CBrickzActEntry* e2 = reinterpret_cast<CBrickzActEntry*>(g_brickzActReg.ResolveEntry(coord));
        (this->*(e2->m_fn))();
    }
}

// CBrickz::RegisterActs (0x10ebe0) - the register-"A"-then-bind archetype.
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md): logic + every byte faithful; only
// the regalloc/free-loop-count materialization diverges. Deferred.
RVA(0x0010ebe0, 0x18d)
void CBrickz::RegisterActs() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        id = g_typeCounter;
        char* slot = ActNameLookup(id);
        i32 cnt = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        if (cnt != 0) {
            do {
                if (list != 0) {
                    (reinterpret_cast<CString*>(list))->CString::~CString();
                }
                list++;
            } while (--cnt);
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    (reinterpret_cast<CBrickzActEntry*>(g_brickzActReg.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CBrickz::Trigger);
}

// CCheckpointTrigger::CCheckpointTrigger(CGameObject*) @0x10ee20 - the 1-arg leaf ctor:
// the standard CUserLogic(obj) init plus the checkpoint tail (leaf vftable stamp, "A"
// cache, two logic bits, z-key recompute, then capture the 15-dword checkpoint state).
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-faithful; the residue is this ctor's own __ehfuncinfo state numbering + the
// zero-register-pinning callee-saved choice (the shared CUserLogic-init wall). Deferred.
RVA(0x0010ee20, 0x27d)
CCheckpointTrigger::CCheckpointTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    i32 zk = m_object->m_layer->m_anchorY + m_object->m_screenY + 0x186a0;
    if (m_object->m_latchedAnimId != zk) {
        m_object->m_latchedAnimId = zk;
        m_object->m_flags |= 0x20000;
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
    for (m_firstEmpty = 0; m_firstEmpty < 15; m_firstEmpty++) {
        if (m_state[m_firstEmpty] == 0) {
            break;
        }
    }
}

// CCheckpointTrigger::InitActReg (0x10f160) - construct g_checkpointActReg.
RVA(0x0010f160, 0x15)
void CCheckpointTrigger::InitActReg() {
    (reinterpret_cast<CZDArrayDerived*>(&g_checkpointActReg))->Construct(2000, 2010);
}

// CCheckpointTrigger::FireActivation (0x10f1e0), vtable slot 4 (RTTI: its slot 4 is
// ILT 0x001366 -> jmp here; the 0x10ea80 body it used to claim is CBrickz's).
RVA(0x0010f1e0, 0x102)
void CCheckpointTrigger::FireActivation(i32 coord) {
    CCheckpointActEntry* e = reinterpret_cast<CCheckpointActEntry*>(g_checkpointActReg.ResolveEntry(coord));
    if (e->m_fn != 0) {
        CCheckpointActEntry* e2 = reinterpret_cast<CCheckpointActEntry*>(g_checkpointActReg.ResolveEntry(coord));
        (this->*(e2->m_fn))();
    }
}

// CCheckpointTrigger::RegisterActs (0x10f340) - intern "A" and "B", bind each handler.
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (both intern/name-resolve blocks + the OWN-registry resolves + the
// handler stores match retail); residual is the slot-vs-id callee-saved register
// choice cascading into the free-loop counts. Deferred.
RVA(0x0010f340, 0x2ac)
void CCheckpointTrigger::RegisterActs() {
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
    (reinterpret_cast<CCheckpointActEntry*>(g_checkpointActReg.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CCheckpointTrigger::Act_10f6a0);

    i32 id2 = reinterpret_cast<i32>(g_buteTree.Find("B"));
    if (id2 == 0) {
        id2 = g_typeCounter;
        g_buteTree.Insert("B", reinterpret_cast<void*>(id2));
        char* slot = ActNameLookup(id2);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("B");
        g_typeCounter++;
    }
    (reinterpret_cast<CCheckpointActEntry*>(g_checkpointActReg.ResolveEntry(id2)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CCheckpointTrigger::Act_10f970);
}

// @confidence: high
// @source: vtable-slot+pmf (RegisterActs binds this RVA as the "A" handler)
// @stub
// 0x10f6a0 (565 B) = CCheckpointTrigger's per-frame "A" activation handler (ex the
// free Gap_10f6a0, homed from src/Stub/GapFunctions.cpp by matcher-5). Operates on
// the 0x94 checkpoint layout; ~10 callees (FindChild, CButeTree::Find,
// ApplyLookupGeometry, LeafCue::PlayIfElapsed, OnCheckpointReached, SpawnVoiceDriver
// + inline rand + a level sprite-ref hit-test). Pending leaf-first reconstruction
// (>512 B); the empty body keeps the PMF wired to the real method symbol.
RVA(0x0010f6a0, 0x235)
i32 CCheckpointTrigger::Act_10f6a0() {
    return 0;
}

// CCheckpointTrigger::SerializeMove @0x10f9a0 - vtable slot 1. Read/Write the captured
// checkpoint state (the 15-dword m_state block @+0x54 + the m_firstEmpty index @+0x90)
// through the archive's mode-keyed slots, then chain the shared serialize helper + the
// +0x34 CSerialObjRef sub-object's Chain.
RVA(0x0010f9a0, 0x8f)
i32 CCheckpointTrigger::SerializeMove(CGruntArchive* arc, i32 mode, i32 a3, i32 a4) {
    CSerialArchive* sa = static_cast<CSerialArchive*>(arc);
    if (mode == 4) {
        sa->Write(m_state, 0x3c);
        sa->Write(&m_firstEmpty, 4);
    } else if (mode == 7) {
        sa->Read(m_state, 0x3c);
        sa->Read(&m_firstEmpty, 4);
    }
    if (!CUserLogic::SerializeMove(reinterpret_cast<CSerialArchive*>((reinterpret_cast<i32>(arc))), mode, a3, a4)) {
        return 0;
    }
    return Chain(sa, mode, a3, reinterpret_cast<CGameObject*>(a4)) ? 1 : 0;
}

// --- The three CTileTrigger leaves' 1-arg ctors (0x10fa60/90/c0) --- each chains
// CTileTrigger(obj) then the leaf vptr auto-stamps. vptrs: CTileSecretTrigger
// 0x5e7e64, CGiantRock 0x5e7d5c, CCoveredPowerup 0x5e7e0c.
RVA(0x0010fa60, 0x19)
CTileSecretTrigger::CTileSecretTrigger(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fa90, 0x19)
CGiantRock::CGiantRock(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fac0, 0x19)
CCoveredPowerup::CCoveredPowerup(CGameObject* obj) : CTileTrigger(obj) {}

// ===========================================================================
// CTileTriggerTransition class band (the tiletriggertransition stray, folded waveM-strays):
// the leaf's ctor + own methods, ascending 0x10faf0-0x110149.
// ===========================================================================

// CTileTriggerTransition::CTileTriggerTransition (0x10faf0) - the 1-arg leaf ctor: fold the
// shared CUserLogic(obj) init then stamp the leaf vptr (0x5e7db4) + the +0x1000000 object
// flag / +0x74 type write the original tail does.
// @early-stop
// EH-ctor vptr-restamp wall (94.9%): the leaf vptr re-stamp lands in EH state 0 + the EH
// scope-cookie initializes to 8 not 0 - see docs/patterns/eh-ctor-vptr-restamp-position.md
// (non-steerable EH-state machine ordering). Body byte-identical otherwise.
RVA(0x0010faf0, 0x128)
CTileTriggerTransition::CTileTriggerTransition(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 0x1000000;
    if (m_object->m_latchedAnimId != 0) {
        m_object->m_latchedAnimId = 0;
        m_object->m_flags |= 0x20000;
    }
}

// Register_10fc90 (0x10fc90) - reserve this class's activation coordinate range [0x7d0, 0x7da]
// in the global tile-trigger activation registry (ecx/this unused).
RVA(0x0010fc90, 0x15)
void CTileTriggerTransition::Register_10fc90() {
    (reinterpret_cast<CZDArrayDerived*>(&g_tileActReg))->Construct(0x7d0, 0x7da);
}

// FireActivation (0x10fd10), vtable slot 4 - look the activation coordinate up in the class
// registry (g_tileActReg); if the resolved entry carries a registered handler PMF, resolve it
// again and dispatch it __thiscall on `this`. Same double-ResolveEntry + PMF-fire archetype.
RVA(0x0010fd10, 0x102)
void CTileTriggerTransition::FireActivation(i32 coord) {
    TileActEntry* e = reinterpret_cast<TileActEntry*>(g_tileActReg.ResolveEntry(coord));
    if (e->m_fn != 0) {
        TileActEntry* e2 = reinterpret_cast<TileActEntry*>(g_tileActReg.ResolveEntry(coord));
        (this->*(e2->m_fn))();
    }
}

// RegisterActs (0x10fe70) - intern this class's activation key "A" into the shared bute-tree
// name map, then bind that id to this class's per-frame handler (Handler_110110).
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset Handler` store
// match retail); residual is the slot-vs-id callee-saved register choice cascading into the
// name-list free-loop count materialization - identical wall to CSecretLevelTrigger::RegisterActs.
RVA(0x0010fe70, 0x18d)
void CTileTriggerTransition::RegisterActs() {
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
    (reinterpret_cast<TileActEntry*>(g_tileActReg.ResolveEntry(id)))->m_fn = static_cast<i32 (CUserLogic::*)()>(&CTileTriggerTransition::Handler_110110);
}

// ApplyAnimation (0x110070) - reads the object's +0x1b4 animation descriptor, applies the
// geometry token, picks the seed frame from element[0], applies the sprite, then swaps the
// aux's bute node for the "A" node (caching the old one).
RVA(0x00110070, 0x71)
i32 CTileTriggerTransition::ApplyAnimation(char* sprite, char* geom) {
    m_value = m_38->m_1a0.m_14;
    if (m_38->ApplyLookupGeometry(geom, 0) == 0) {
        return 0;
    }
    CAniElement* desc = m_38->m_1a0.m_14;
    CAniDesc* elem = desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
    m_38->ApplyLookupSprite(sprite, elem->m_param);
    m_prevAnimSetNode = m_objAux->m_1c; // save the prev anim-set node (CUserLogic base field)
    m_objAux->m_1c = g_buteTree.Find("A");
    return 1;
}

// Handler_110110 (0x110110) - the per-frame handler bound by RegisterActs. Advance the bound
// object's +0x1a0 anim sub-object to the current draw-delta (g_engineFrameDelta); if the sub-mgr is
// active (m_1c8 != 0) but not idle (m_1c0 == 0), mark the object stalled/handled this frame.
RVA(0x00110110, 0x39)
i32 CTileTriggerTransition::Handler_110110() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    if (m_38->m_1a0.m_28 != 0 && m_38->m_1a0.m_20 == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

SIZE_UNKNOWN(CWarpStonePadActEntry);
SIZE_UNKNOWN(CTileTriggerSwitchActEntry);
SIZE_UNKNOWN(CCheckpointActEntry);
SIZE_UNKNOWN(CTileTriggerActEntry);
SIZE_UNKNOWN(CBrickzActEntry);
// (TileActEntry's SIZE_UNKNOWN moved to <Gruntz/TileTriggerTransition.h> with the class.)
