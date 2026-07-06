// FortressFlag.cpp - the fortress-flag game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CFortressFlag methods, defined in ascending retail-RVA
// order:
//   ~CFortressFlag    @0x010e90 - the /GX leaf dtor (folds the CUserLogic teardown).
//   Serialize         @0x046410 - the Serialize override (chain + the tag-8 fixup).
//
// CFortressFlag : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/FortressFlag.h>
#include <Gruntz/SerialObjRef.h>   // the shared serialized-object-reference (Chain @0x8c00)
#include <Gruntz/SpriteRefTable.h> // the shared CSpriteRefTable (g_gameReg->m_74->GetSel)
#include <Gruntz/Enums.h> // Warlord - the m_124 flag-owner roster (KING/NAPOLEAN/PATTON/VIKING)
#include <Gruntz/AnimSink.h>
#include <Gruntz/WwdGameReg.h> // the canonical WwdGameReg singleton (g_gameReg)

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class).
typedef i32 (CFortressFlag::*FortressFlagHandler)();
struct CFortressFlagActEntry {
    FortressFlagHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x644638), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). Was a per-file
// duplicate of the <Gruntz/ActReg.h> CActReg archetype (layout + ResolveEntry); now
// derives from it, keeping its own placeholder name so the DATA-pinned global is
// unchanged.
struct CFortressFlagActReg : public CActReg {};
DATA(0x00244638)
extern CFortressFlagActReg g_fortressFlagActReg; // 0x644638

// The per-frame draw-delta mirror (_g_6bf3bc); the value-load reloc-masks.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// The bound sprite/game-object is the inherited CUserLogic m_10 (a CGameObject*):
// the tag-8 fixup reads its +0x124 sprite-selector key and re-seeds the
// +0x4c/+0x50/+0x58 state trio directly (all modeled on CGameObject).

// The level sprite-ref table (g_gameReg->m_74). GetSel(i, bAlt) (0xe23c0) returns
// the selected sprite handle for ref-row i; the body lives in
// src/Gruntz/SpriteRefTable.cpp (reloc-masked). CSpriteRefTable is the shared
// <Gruntz/SpriteRefTable.h> shape.

// One ref-index array slot: an 8-byte entry whose first dword is the ref-row
// index. The fixup indexes the array at g_gameReg+0x158 by (selector-row * 71)
// (retail: lea+shl+sub = *71, then the *8 element stride folds into the [...*8]
// addressing-mode scale), so the row multiply is materialized but the *8 is free.
struct WwdRefSlot {
    i32 m_idx; // +0x00  ref-row index (passed to GetSel)
    i32 m_04;  // +0x04
};

// The global game registry (canonical <Gruntz/WwdGameReg.h>, RVA 0x24556c; wwdfile
// owns the DATA label). The tag-8 fixup reads the level sprite-ref table at +0x74
// (m_74) and the ref-index array in the m_options block at +0x158 (raw offset - the
// established 0x150-region idiom; WwdRefSlot is this TU's element view).
DATA(0x0024556c)
extern WwdGameReg* g_gameReg;

// CFortressFlag::GetTypeTag @0x010e40 - vtable slot 2: the class's logic-type id
// (0x427), the 6-byte `mov eax,<id>; ret` accessor archetype. Regular method (the
// fat CUserLogic base slot 2 carries a placeholder signature; the leaf vtable is
// not a diffed symbol, so a plain method reproduces the slot bytes exactly).
RVA(0x00010e40, 0x6)
LogicTypeId CFortressFlag::GetTypeTag() {
    return LOGIC_FORTRESSFLAG; // 0x427
}

// CFortressFlag::~CFortressFlag @0x010e90 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to
// ~CSecretTeleporterTrigger @0x010ab0; the empty body is enough for cl.
RVA(0x00010e90, 0x44)
CFortressFlag::~CFortressFlag() {}

// CFortressFlag::HandleFortConquered @0x03f5f0 (1318 B) - the per-frame fort-
// conquest check (re-homed from src/Stub/Backlog.cpp; Ghidra symbol cluster proves
// CFortressFlag ownership: adjacent to NotifyFortUnderAttack @0x45270 / the
// CFortressFlag ctor/dtor / BuildFortSplashParticles @0x44f80). Structure decoded:
// the +0x1a0 sub-clock tick, the g_mgrSettings->m_134 mode gate, HitTestCell + dedup
// vs owner->m_124, the 5-CString "<A> was conquered by <B>!" HUD message, the config
// re-tag, the two handler-type re-home list walks + a g_freeList pop, and a
// per-object GAME_EXPLOSION3 eye-candy spawn.
// @early-stop
// >512B /GX regalloc wall: the full-body reconstruction BUILDS but scores 0.0% (below
// the empty-stub baseline) - retail pins `this` to ebp with a 0x24 frame + canonical
// /GX prologue while the /O2 recompile pins `this` to ebx with a 0x20 frame + a
// hoisted `mov eax,fs:0`, a this-register + frame-slot divergence cascading through
// all 1318 B. Kept as the empty (highest-%) stub per the >512B REVERT rule; final-
// sweep leaf-first redo needs the callee set + a matching regalloc modeled first.
RVA(0x0003f5f0, 0x526)
void CFortressFlag::HandleFortConquered() {}

// CFortressFlag::CFortressFlag @0x045d30 - fold the shared CUserLogic(obj) init,
// run the eyecandy z-clamp, pick the flag's faction name (a 4-way switch on the
// sprite-selector m_124: KING/NAPOLEAN/PATTON/VIKING - any other selector hides
// the sub-object and bails), name + cycle-geometry the bound object, bind the "A"
// bute node, then resolve the selected sprite handle from g_gameReg's ref-index
// array and stamp it back (+0x58/+0x50/+0x4c). GetByIndex (0x4165) is the engine
// routine GetSel thunks to, so the reused GetSel call reloc-masks.
//
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical (incl. the m_124 jump table); residual is the /GX leaf-vptr
// re-stamp position + EH-state ids.
RVA(0x00045d30, 0x203)
CFortressFlag::CFortressFlag(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    CGameObject* o = m_object;
    i32 v = o->m_layer->m_1c + o->m_screenY + 0x186a0;
    if (o->m_latchedAnimId != v) {
        o->m_latchedAnimId = v;
        o->m_flags |= 0x20000;
    }
    const char* name;
    switch (m_object->m_124) {
        case WARLORD_KING:
            name = "GAME_FORTRESSFLAGZ_KING";
            break;
        case WARLORD_NAPOLEAN:
            name = "GAME_FORTRESSFLAGZ_NAPOLEAN";
            break;
        case WARLORD_PATTON:
            name = "GAME_FORTRESSFLAGZ_PATTON";
            break;
        case WARLORD_VIKING:
            name = "GAME_FORTRESSFLAGZ_VIKING";
            break;
        default:
            m_38->m_flags |= 0x10000;
            return;
    }
    m_38->ApplyName(name);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(s_actKeyA);
    m_prevAnimNode = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_38->m_flags |= 3;
    i32 idx = ((WwdRefSlot*)((char*)g_gameReg + 0x158))[m_object->m_124 * 71].m_idx;
    i32 sel = g_gameReg->m_74->GetSel(idx, 0);
    CGameObject* spr = m_object;
    spr->m_drawActive = 1;
    spr->m_drawFillCmd = 0xa;
    spr->m_drawFillArg = sel;
}

// CFortressFlag::InitActReg @0x046000 - construct the class's activation-
// coordinate registry singleton (g_fortressFlagActReg @0x644638) over the fixed
// range [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x00046000, 0x15)
void CFortressFlag::InitActReg() {
    ((CZDArrayDerived*)&g_fortressFlagActReg)->Construct(2000, 2010);
}

// CFortressFlag::RegisterActs @0x0461e0 - bind the per-frame handler (AdvanceAnim
// @0x0463e0) to the activation key "A" via the shared name registry. The SAME
// archetype as CBehindCandyAni::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000461e0, 0x18d)
void CFortressFlag::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((CFortressFlagActEntry*)g_fortressFlagActReg.ResolveEntry(id))->m_fn =
        &CFortressFlag::AdvanceAnim;
}

// CFortressFlag::AdvanceAnim @0x0463e0 - re-target the bound object's animation
// sub-object (m_38 + 0x1a0) to the current draw-delta (g_6bf3bc) and return 0.
// Same archetype as CGruntCreationPoint::AdvanceAnim (0x03ecc0).
RVA(0x000463e0, 0x17)
i32 CFortressFlag::AdvanceAnim() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_6bf3bc);
    return 0;
}

// CFortressFlag::Serialize @0x046410 - chain the shared CUserLogic serialize
// helper on `this`, and (only on success) the +0x34 sub-object's chain; both run
// the same (ar, tag, c, d) tuple. On the post-load tag (tag == 8), look the
// flag's sprite selector (m_object->m_124 * 71) up in g_gameReg's ref-index array,
// resolve it through the level sprite-ref table, and re-seed the bound sprite's
// state trio. Always returns 1 once the two chains succeed.
RVA(0x00046410, 0x92)
i32 CFortressFlag::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    if (!((CSerialObjRef*)((char*)this + 0x34))
             ->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d)) {
        return 0;
    }
    if (tag == 8) {
        CGameObject* spr = m_object;
        i32 idx = ((WwdRefSlot*)((char*)g_gameReg + 0x158))[spr->m_124 * 71].m_idx;
        i32 sel = g_gameReg->m_74->GetSel(idx, 0);
        spr = m_object;
        spr->m_drawActive = 1;
        spr->m_drawFillCmd = 0xa;
        spr->m_drawFillArg = sel;
    }
    return 1;
}

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
SIZE_UNKNOWN(CFortressFlag);
SIZE_UNKNOWN(CFortressFlagActEntry);
SIZE_UNKNOWN(CFortressFlagActReg);
SIZE_UNKNOWN(WwdRefSlot);
