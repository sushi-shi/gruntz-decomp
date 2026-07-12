// StaticHazard.cpp - a static hazard game-object (C:\Proj\Gruntz).
//
// CStaticHazard : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Methods defined in ascending retail-RVA order:
//   ~CStaticHazard   @0x012b30 - the /GX leaf dtor (folds the CUserLogic teardown).
//   CStaticHazard    @0x0fb7a0 - the 1-arg ctor (CUserLogic leaf init + the
//                                static-hazard tail).
//   FireActivation   @0x0fbbf0 - the per-coordinate activation-registry dispatcher.
//   LoadAttributes2  @0x0fc0b0 - the time-gated pulse (the smaller tick variant).
//   LoadAttributes   @0x0fc1a0 - the full periodic tick/update.
//
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/HaznColl.h>        // shared coordinate/activation-registry collection
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Bute/ButeTree.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/StaticHazard.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialObjRef.h>  // SerialRef34()->Chain (0x8c00)
#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Bute/ButeMgr.h>         // CButeMgr (g_buteMgr GetIntDef), CButeTree (g_buteTree)
#include <Globals.h>

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190).
extern CButeTree g_buteTree;

// The running game clock (DAT_00645588; low 32 bits of the engine counter) and
// the draw-clock delta the per-frame animation re-target reads (DAT_006bf3bc).
extern "C" u32 g_645588;
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// A .data global the ctor copies into the bound object's +0x124 (DAT_0064553c).

// ---------------------------------------------------------------------------
// The bound game object is the inherited CUserLogic m_10/m_38 (both CGameObject*,
// both point at it); the static-hazard paths use them directly (the CTeleporter
// idiom - no per-TU view cast). CGameObject (<Gruntz/UserLogic.h>) models every
// field/method these paths touch. Re-read m_10/m_38 per access (never cache to a
// local) so each member load matches retail's reload.
//
// The one hazard-specific sub-object CGameObject does not model as a member is the
// +0x1a0 animation sub-object; it is reached via the (char*)m_38 + 0x1a0 byte-arith
// idiom (same as CTeleporter's CTeleAnimSink). Its +0x20/+0x28/+0x2c state flags
// gate the "animation finished -> revert to IDLE" branch; their exact roles are
// unproven, so they stay placeholders.

// The active-anim descriptor (m_38->m_1b4): the SetAnimEx idiom reads its first
// element's frame seed.
struct HazAnimElem {
    char m_pad00[0x14];
    i32 m_frameSeed; // +0x14
};
struct HazAnimDesc {
    char m_pad00[0x0c];
    HazAnimElem** m_elems; // +0x0c  element vector (first elem = *m_elems)
    i32 m_count;           // +0x10  element count (>0 gate)
};

// ---------------------------------------------------------------------------
// The game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A). Modeled here with
// the offsets the static-hazard paths touch.
// DISPOSITION: HazSwitchSrc/HazLookupEntry/HazSndCat/HazSndRoot/HazGrid are the
// SANCTIONED per-TU views of the 0x64556c multi-view singleton's void* sub-object
// slots (see <Gruntz/GameRegistry.h>: each TU casts the slot to its own concrete
// sub-object type at the deref site). The singleton is the CGruntzMgr view (classifier
// scope); these stay as the endorsed per-TU sub-object casts, not fabricated types.
// ---------------------------------------------------------------------------
struct HazSwitchSrc {
    char m_pad00[0x20];
    i32 m_levelKind; // +0x20  the level-kind tag the ctor switches on
};
struct HazLookupEntry {
    char m_pad00[0x24];
    i32 m_aniPadBias; // +0x24  the per-effect AniPad bias
};
// (The ex-`CMapStringToOb` view is DISSOLVED: an empty phantom aliasing the MFC library
// CMapStringToOb::Lookup @0x1b8438 - the member is the real map.)
struct HazSndCat {
    char m_pad00[0x10];
    CMapStringToOb m_map; // +0x10  the lookup map
};
struct HazSndRoot {
    char m_pad00[0x2c];
    HazSndCat* m_cat; // +0x2c
};
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// ===========================================================================
// FireActivation's per-coordinate activation registry (CStaticHazard's OWN
// instance @0x64e3d0) - the SAME archetype as CTimeBomb::FireActivation. A
// coordinate maps to an Entry* either directly (within [g_haznLo,g_haznHi]) or by
// a slow Find/rebuild. All globals are unnamed BSS (DATA-pinned so the loads
// reloc-mask); the collection methods are external/no-body.
// ===========================================================================
struct CHaznEntry;         // an entry: first dword is the registered handler
extern void* GetRetAddr(); // 0x16d990

DATA(0x0024e3d0)
extern CCoordColl g_haznColl;

// ConstructHaznRange @0x0fbb70 - the static initializer that builds g_haznColl's fast
// [0x7d0, 0x7da] id range (CZDArrayDerived::Construct). Re-homed from
// src/Stub/BoundaryLowerThunks.cpp (was RegRangefbb70).
RVA(0x000fbb70, 0x15)
void ConstructHaznRange() {
    ((CZDArrayDerived*)&g_haznColl)->Construct(0x7d0, 0x7da);
}
DATA(0x002bf464)
extern void* g_projActCache;
extern void* g_retAddrBreadcrumb;

// The entry's first dword is a pointer-to-member-function of CStaticHazard
// (single inheritance -> 4-byte code pointer); FireActivation invokes it on
// `this`, emitting `mov ecx,this; call [entry]`.
typedef void (CStaticHazard::*HaznHandler)();
struct CHaznEntry {
    HaznHandler m_fn; // [entry]
};

// RegisterActs binds the two i32-returning handler PMFs (LoadAttributes2 /
// LoadAttributes); a distinct entry view so the store keeps the real signature.
typedef i32 (CStaticHazard::*HaznHandler2)();
struct CHaznEntry2 {
    HaznHandler2 m_fn;
};

// ---------------------------------------------------------------------------
// RegisterActs (0x0fbd50) interns the "A" and "B" activation keys into the shared
// bute store and records each in the shared name registry (@0x6bf650, the SAME
// instance CTimeBomb/CDroppedObject use), then resolves the id in CStaticHazard's
// OWN registry (HaznLookup) and stores the per-key handler PMF.
DATA(0x0021aea8)
extern i32 g_typeCounter;
DATA(0x0020a454)
extern char s_codeA[]; // "A"
DATA(0x0020d1bc)
extern char s_actKeyB[]; // "B"
class CTypeKeyColl;      // canonical g_typeColl @0x6bf650 (<Gruntz/TypeKeyColl.h>)
struct CTypeNameEntry;   // canonical g_typeCur slot record (<Gruntz/TypeNameEntry.h>)
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl; // 0x6bf650
DATA(0x002bf654)
extern CVariantSlot* g_typeColl2; // 0x6bf654
DATA(0x002bf658)
extern i32 g_typeLo;
DATA(0x002bf65c)
extern i32 g_typeHi;
DATA(0x002bf660)
extern char* g_typeBase;
DATA(0x002bf668)
extern i32 g_typeStride;
DATA(0x002bf664)
extern CTypeNameEntry* g_typeCur;
DATA(0x002bf66c)
extern void* g_typeNodes;
DATA(0x002bf670)
extern i32 g_typeCount;

// The CString in the resolved name slot: ~CString (0x1b9b93) frees the old list,
// operator= (0x1b9e74) assigns the new key. Modeled so the calls reloc-mask.
#include <Gruntz/ActName.h> // CActName (shared)

// The id->name-slot resolve (fast range path + slow Find/GetRetAddr/Insert rebuild).
static inline char* ActNameLookup(i32 id) {
    g_typeCount = 0;
    if (id >= g_typeLo && id <= g_typeHi) {
        return g_typeBase + (id - g_typeLo) * g_typeStride;
    }
    if ((i32)((_zvec*)&g_typeColl)->GrowTo(id, 0)) { // slow lookup == _zvec::GrowTo @0x16da80
        return g_typeBase + (id - g_typeLo) * g_typeStride;
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl2->Set(&g_typeColl, (i32)item, 0xc);
    return (char*)g_typeCur;
}

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
// g_hazn* registry-field globals (referenced only from this TU): real
// definitions DATA-pinned here; the single extern is in <Globals.h>.
DATA(0x0024e3d4)
CVariantSlot* g_haznColl2;
DATA(0x0024e3d8)
i32 g_haznLo;
DATA(0x0024e3dc)
i32 g_haznHi;
DATA(0x0024e3e0)
char* g_haznBase;
DATA(0x0024e3e4)
CHaznEntry* g_haznCur;
DATA(0x0024e3e8)
i32 g_haznStride;
DATA(0x0024e3f0)
i32 g_haznScratch;

static inline CHaznEntry* HaznLookup(i32 coord) {
    g_haznScratch = 0;
    if (coord >= g_haznLo && coord <= g_haznHi) {
        return (CHaznEntry*)(g_haznBase + (coord - g_haznLo) * g_haznStride);
    }
    if ((i32)((_zvec*)&g_haznColl)->GrowTo(coord, 0)) { // slow lookup == _zvec::GrowTo @0x16da80
        return (CHaznEntry*)(g_haznBase + (coord - g_haznLo) * g_haznStride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_haznColl2->Set(&g_haznColl, (i32)item, 0xc);
    return g_haznCur;
}

// CStaticHazard::GetTypeTag (0x00012ae0) is now an inline member in the class header.

// ---------------------------------------------------------------------------
// CStaticHazard::~CStaticHazard @0x012b30 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown; the
// destructible +0x18 link forces the /GX EH frame. Byte-identical in shape to
// ~CTimeBomb @0x012a70; the empty body is enough for cl.
RVA(0x00012b30, 0x44)
CStaticHazard::~CStaticHazard() {}

// ---------------------------------------------------------------------------
// CStaticHazard::CStaticHazard @0x0fb7a0 - the 1-arg ctor. Chains the standard
// CUserLogic(CGameObject*) leaf init (folded inline), then the static-hazard
// tail: re-arm the IDLE animation, snap the bound object to tile center, seed the
// pulse window from the "Hazardz/AniPad" bute int, cache the anim-set node.
//
// @early-stop
// jump-table + bound-object spill wall: logic byte-correct (the CUserLogic init,
// the IDLE re-arm via the SetAnimEx idiom, the tile snap (`and al,0xe0`), the
// 4-case level-kind jump table, the bbox/period seeding, the sound-map Lookup and
// the Hazardz/AniPad GetIntDef), but the dense bound-object field stores spill
// against retail's stack-slot schedule and the jump-table data region scores as
// the jumptable-data-overlap artifact. Parked for the final sweep.
RVA(0x000fb7a0, 0x2d4)
CStaticHazard::CStaticHazard(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    // re-arm the IDLE geometry + STATICHAZARD sprite (SetAnimEx idiom).
    m_prevAnimNode = m_38->m_geoId;
    m_38->ApplyLookupGeometry("LEVEL_STATICHAZARDIDLE", 0);
    {
        HazAnimDesc* d = (HazAnimDesc*)m_38->m_geoId;
        HazAnimElem* e = d->m_count > 0 ? *d->m_elems : 0;
        m_38->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_frameSeed);
    }
    // snap the bound object's screen position to tile center.
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    if (m_object->m_latchedAnimId != 0) {
        m_object->m_latchedAnimId = 0;
        m_object->m_flags |= 0x20000;
    }
    m_tileCol = m_object->m_screenX >> 5;
    m_tileRow = m_object->m_screenY >> 5;
    m_object->m_placeMode = 0;
    switch (((HazSwitchSrc*)g_gameReg->m_curState)->m_levelKind) {
        case 3:
        case 4:
        case 7:
        case 8:
            m_object->m_placeMode = m_object->m_screenY + 0x186b0;
            break;
        default:
            break;
    }
    m_object->m_areaL = m_object->m_screenX - 7;
    m_object->m_areaR = m_object->m_areaL + 14;
    m_object->m_areaT = m_object->m_screenY - 7;
    m_object->m_areaB = m_object->m_areaT + 14;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 0x2000002;
    ((CAniAdvanceCursor*)((char*)m_object + 0x1a0))->m_2c = 0;
    m_object->m_124 = g_64553c;
    m_activeWindow = 0;
    m_idleWindow = m_object->m_120;
    m_pulseEpoch = g_645588;
    HazLookupEntry* entry = 0;
    ((CMapStringToOb*)&((HazSndRoot*)g_gameReg->m_world)->m_cat->m_map)
        ->Lookup("LEVEL_STATICHAZARDGO", (CObject*&)entry);
    if (entry != 0) {
        m_activeWindow = g_buteMgr.GetIntDef("Hazardz", "AniPad", 0x64) + entry->m_aniPadBias;
    } else {
        g_gameReg->EmitEvent(0x8009, 0x461);
    }
    if (m_object->m_120 == 0) {
        m_idleWindow = m_activeWindow;
    }
}

// ---------------------------------------------------------------------------
// CStaticHazard::FireActivation @0x0fbbf0 - look the activation coordinate up in
// the registry; if the entry has a registered handler, look it up again and
// dispatch it __thiscall on this.
RVA(0x000fbbf0, 0x102)
void CStaticHazard::FireActivation(i32 coord) {
    CHaznEntry* e = HaznLookup(coord);
    if (e->m_fn != 0) {
        CHaznEntry* e2 = HaznLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CStaticHazard::RegisterActs @0x0fbd50 - intern "A" and "B" and bind each to its
// handler PMF (LoadAttributes2 @0xfc0b0 for "A", LoadAttributes @0xfc1a0 for "B")
// in the hazard registry. Two back-to-back single-key registrations; the SAME
// archetype as CTimeBomb::RegisterActs done twice.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (both intern/name-resolve blocks + the OWN-registry resolves + the
// `mov [entry],offset handler` stores match retail); residual is the slot-vs-id
// callee-saved register choice cascading into the free-loop counts. Deferred.
RVA(0x000fbd50, 0x2ac)
void CStaticHazard::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert(s_codeA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeCount;
        void** list = (void**)g_typeNodes;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CHaznEntry2*)HaznLookup(id))->m_fn = &CStaticHazard::LoadAttributes2;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        id2 = g_typeCounter;
        g_buteTree.Insert(s_actKeyB, (void*)id2);
        char* slot = ActNameLookup(id2);
        i32 n = g_typeCount;
        void** list = (void**)g_typeNodes;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyB);
        g_typeCounter++;
    }
    ((CHaznEntry2*)HaznLookup(id2))->m_fn = &CStaticHazard::LoadAttributes;
}

// ---------------------------------------------------------------------------
// CStaticHazard::LoadAttributes2 @0x0fc0b0 - the time-gated pulse: bail when the
// registry is in the gated state; compute the running phase modulo the window;
// on a hit latch m_fired, re-arm the GO geometry/STATICHAZARD sprite (SetAnimEx
// idiom), and re-resolve the "B" anim-set node through the global bute tree.
//
// @early-stop
// store-vs-load scheduling wall (docs/patterns/statement-schedule-faithful.md):
// logic 100% correct, all reloc operands named. Retail emits `mov [m_fired],1` before
// reloading m_38; MSVC5 here hoists the m_38 load above the store. The m_38 reload
// is load-bearing (re-read per call), so the store/load pair can't be pinned.
// ~98%. Parked for the final sweep.
RVA(0x000fc0b0, 0xb2)
i32 CStaticHazard::LoadAttributes2() {
    CGameRegistry* reg = g_gameReg;
    if (reg->m_isEasyMode != 0 && reg->m_134 == 1) {
        return 0;
    }
    u32 phase = g_645588 - m_pulseEpoch;
    u32 base = (u32)m_object->m_118;
    if (phase <= base) {
        return 0;
    }
    phase -= base;
    u32 span = m_idleWindow + m_activeWindow;
    if (phase % span > (u32)m_activeWindow) {
        return 0;
    }
    m_fired = 1;
    m_prevAnimNode = m_38->m_geoId;
    m_38->ApplyLookupGeometry("LEVEL_STATICHAZARDGO", 0);
    {
        HazAnimDesc* d = (HazAnimDesc*)m_38->m_geoId;
        HazAnimElem* e = d->m_count > 0 ? *d->m_elems : 0;
        m_38->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_frameSeed);
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("B");
    return 0;
}

// ---------------------------------------------------------------------------
// CStaticHazard::LoadAttributes @0x0fc1a0 - the full periodic tick. Compute the
// running phase modulo the on/off window; depending on the window half + the
// fired flag (m_fired) + the bound object's gate (m_120), re-arm the GO or IDLE
// animation, drive the per-frame anim sub-object (SetAnim==2 => place + mark the
// hazard grid cell), and set/clear the cell's bit-0x8000000.
//
// @early-stop
// branch-heavy bound-object + grid-cell spill wall: logic byte-correct (the phase
// gate, the four re-arm paths, the SetAnim dispatch with the ScreenToCell/MarkCell
// calls, the grid set/clear-bit cell math), but the dense field reloads + the four
// grid-cell op sites spill against retail's stack-slot schedule. Parked for sweep.
RVA(0x000fc1a0, 0x33b)
i32 CStaticHazard::LoadAttributes() {
    u32 phase = (g_645588 - m_pulseEpoch) - (u32)m_object->m_118;
    u32 rem = phase % (u32)(m_idleWindow + m_activeWindow);
    if (rem > (u32)m_activeWindow) {
        // idle window
        if (m_fired == 0) {
            goto dispatch;
        }
        if (m_object->m_120 != 0) {
            // re-arm IDLE (cache the anim-set node first)
            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = g_buteTree.Find("A");
            m_prevAnimNode = m_38->m_geoId;
            m_38->ApplyLookupGeometry("LEVEL_STATICHAZARDIDLE", 0);
            {
                HazAnimDesc* d = (HazAnimDesc*)m_38->m_geoId;
                HazAnimElem* e = d->m_count > 0 ? *d->m_elems : 0;
                m_38->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_frameSeed);
            }
            if (m_object->m_latchedAnimId != 0) {
                m_object->m_latchedAnimId = 0;
                m_object->m_flags |= 0x20000;
            }
            // clear the hazard cell's bit-0x8000000
            CTileGrid* grid = g_gameReg->m_tileGrid;
            if ((u32)m_tileCol < (u32)grid->m_c && (u32)m_tileRow < (u32)grid->m_10) {
                grid->m_8[m_tileRow][m_tileCol * 7] &= 0xf7ffffff;
            }
            return 0;
        }
        // m_120 == 0: re-arm GO + clear the fired flag
        m_prevAnimNode = m_38->m_geoId;
        m_38->ApplyLookupGeometry("LEVEL_STATICHAZARDGO", 0);
        {
            HazAnimDesc* d = (HazAnimDesc*)m_38->m_geoId;
            HazAnimElem* e = d->m_count > 0 ? *d->m_elems : 0;
            m_38->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_frameSeed);
        }
        if (m_object->m_latchedAnimId != 0) {
            m_object->m_latchedAnimId = 0;
            m_object->m_flags |= 0x20000;
        }
        m_fired = 0;
        return 0;
    } else {
        // active window
        if (m_fired != 0) {
            goto dispatch;
        }
        if (m_object->m_120 != 0) {
            goto dispatch;
        }
        // turn on: re-arm GO, latch the fired flag
        m_prevAnimNode = m_38->m_geoId;
        m_38->ApplyLookupGeometry("LEVEL_STATICHAZARDGO", 0);
        {
            HazAnimDesc* d = (HazAnimDesc*)m_38->m_geoId;
            HazAnimElem* e = d->m_count > 0 ? *d->m_elems : 0;
            m_38->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_frameSeed);
        }
        if (m_object->m_latchedAnimId != 0) {
            m_object->m_latchedAnimId = 0;
            m_object->m_flags |= 0x20000;
        }
        m_fired = 1;
        return 0;
    }

dispatch:
    if (((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_6bf3bc) == 2) {
        i32 a = 0, b = 0;
        if (g_gameReg->m_cmdGrid->HitTestCell(m_object->m_screenX, m_object->m_screenY, &a, &b, 0)
            != 0) {
            g_gameReg->m_cmdGrid->CellDispatch(a, b, m_object->m_124, -1);
        }
        if (m_object->m_latchedAnimId != m_object->m_placeMode) {
            m_object->m_latchedAnimId = m_object->m_placeMode;
            m_object->m_flags |= 0x20000;
        }
        CTileGrid* grid = g_gameReg->m_tileGrid;
        if ((u32)m_tileCol < (u32)grid->m_c && (u32)m_tileRow < (u32)grid->m_10) {
            grid->m_8[m_tileRow][m_tileCol * 7] |= 0x8000000;
        }
    } else {
        CTileGrid* grid = g_gameReg->m_tileGrid;
        if ((u32)m_tileCol < (u32)grid->m_c && (u32)m_tileRow < (u32)grid->m_10) {
            grid->m_8[m_tileRow][m_tileCol * 7] &= 0xf7ffffff;
        }
        if (m_object->m_latchedAnimId != 0) {
            m_object->m_latchedAnimId = 0;
            m_object->m_flags |= 0x20000;
        }
    }
    {
        CAniAdvanceCursor* sub = (CAniAdvanceCursor*)((char*)m_38 + 0x1a0);
        if (sub->m_28 != 0 && sub->m_20 == 0) {
            m_prevAnimNode = m_38->m_geoId;
            m_38->ApplyLookupGeometry("LEVEL_STATICHAZARDIDLE", 0);
            {
                HazAnimDesc* d = (HazAnimDesc*)m_38->m_geoId;
                HazAnimElem* e = d->m_count > 0 ? *d->m_elems : 0;
                m_38->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_frameSeed);
            }
            CTileGrid* grid = g_gameReg->m_tileGrid;
            if ((u32)m_tileCol < (u32)grid->m_c && (u32)m_tileRow < (u32)grid->m_10) {
                grid->m_8[m_tileRow][m_tileCol * 7] &= 0xf7ffffff;
            }
        }
    }
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
SIZE_UNKNOWN(CHaznEntry);
SIZE_UNKNOWN(CHaznEntry2);
SIZE_UNKNOWN(CStaticHazard);
SIZE_UNKNOWN(HazAnimDesc);
SIZE_UNKNOWN(HazAnimElem);
SIZE_UNKNOWN(HazGrid);
SIZE_UNKNOWN(HazGridMgr);
SIZE_UNKNOWN(HazLookupEntry);
SIZE_UNKNOWN(HazSndCat);
SIZE_UNKNOWN(HazSndRoot);
SIZE_UNKNOWN(HazSwitchSrc);
// Tree-wide SIZE anchor for the unified CCoordColl coordinate/activation-registry
// archetype (<Gruntz/HaznColl.h>; the former CTBombColl/CHaznColl views, used across
// TimeBomb/StaticHazard). Moved here from the deleted src/Stub/BoundaryLowerThunks.cpp.
SIZE_UNKNOWN(CCoordColl);
SIZE_UNKNOWN(WwdAnimSub);

// CStaticHazard::SerializeMove (0x0fc5b0), vtable slot 1 - stream the leaf pulse
// state (m_54..m_68, six DWORDs) through the archive first, THEN chain the shared
// serialize helper on `this` (gate) + the +0x34 CSerialObjRef sub-object; normalize
// the ref result to a strict bool. The RollingBall::Serialize field-streaming
// archetype, but with the field block emitted before the chain gates.
RVA(0x000fc5b0, 0xf5)
i32 CStaticHazard::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    CSerialArchive* arc = (CSerialArchive*)ar;
    switch (mode) {
        case 4:
            arc->Write(&m_pulseEpoch, 4);
            arc->Write(&m_activeWindow, 4);
            arc->Write(&m_idleWindow, 4);
            arc->Write(&m_fired, 4);
            arc->Write(&m_tileCol, 4);
            arc->Write(&m_tileRow, 4);
            break;
        case 7:
            arc->Read(&m_pulseEpoch, 4);
            arc->Read(&m_activeWindow, 4);
            arc->Read(&m_idleWindow, 4);
            arc->Read(&m_fired, 4);
            arc->Read(&m_tileCol, 4);
            arc->Read(&m_tileRow, 4);
            break;
    }
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)((i32)ar), mode, a3, a4)) {
        return 0;
    }
    return SerialRef34()->Chain(arc, mode, a3, (CSerialObj*)a4) != 0;
}
