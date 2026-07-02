// CStaticHazard.cpp - a static hazard game-object (C:\Proj\Gruntz).
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
#include <Gruntz/CHaznColl.h> // shared coordinate/activation-registry collection
#include <Gruntz/CStaticHazard.h>
#include <Gruntz/CGameRegistry.h>
#include <Bute/ButeMgr.h> // CButeMgr (g_buteMgr GetIntDef), CButeTree (g_buteTree)
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
// The bound game object (the inherited CUserLogic m_10/m_38 both point at it).
// The static-hazard reads hazard-specific offsets past what CGameObject models,
// so the inherited CGameObject* is cast to this TU-local view (the same idiom
// CTeleporter/CLightFx use for their bound object). Re-read m_10/m_38 per access
// (never cache to a local) so each member load matches retail's reload.
// ---------------------------------------------------------------------------
struct WwdAnimSub {
    i32 SetAnim(u32 mode); // 0x15c360 (re-target the active animation)
    char m_pad00[0x20];
    i32 m_20; // +0x20
    char m_pad24[0x28 - 0x24];
    i32 m_28; // +0x28
    i32 m_2c; // +0x2c
};

struct WwdGameObject {
    void ApplyLookupGeometry(const char* key, i32 flag); // 0x1505b0
    void ApplyLookupSprite(const char* key, i32 flag);   // 0x1504d0
    char m_pad00[0x08];
    i32 m_08; // +0x08  flag word
    char m_pad0c[0x5c - 0x0c];
    i32 m_5c; // +0x5c  screen X
    i32 m_60; // +0x60  screen Y
    char m_pad64[0x74 - 0x64];
    i32 m_74; // +0x74  z gate
    char m_pad78[0x118 - 0x78];
    i32 m_118; // +0x118  period base
    char m_pad11c[0x120 - 0x11c];
    i32 m_120; // +0x120  active gate
    i32 m_124; // +0x124
    i32 m_128; // +0x128
    char m_pad12c[0x144 - 0x12c];
    i32 m_144, m_148, m_14c, m_150; // +0x144..+0x150  bbox
    char m_pad154[0x164 - 0x154];
    i32 m_164, m_168; // +0x164/+0x168
    char m_pad16c[0x1a0 - 0x16c];
    WwdAnimSub m_1a0; // +0x1a0  animation sub-object
    char m_pad1a4[0x1b4 - 0x1a4];
    i32 m_1b4; // +0x1b4  active-anim descriptor pointer
};

// The active-anim descriptor (m_38->m_1b4): the SetAnimEx idiom reads its first
// element's m_14 as the sprite frame seed.
struct HazAnimElem {
    char m_pad00[0x14];
    i32 m_14; // +0x14
};
struct HazAnimDesc {
    char m_pad00[0x0c];
    HazAnimElem** m_c; // +0x0c  element vector (first elem = *m_c)
    i32 m_10;          // +0x10  element count (>0 gate)
};

// ---------------------------------------------------------------------------
// The game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A). Modeled here with
// the offsets the static-hazard paths touch.
// ---------------------------------------------------------------------------
struct HazSwitchSrc {
    char m_pad00[0x20];
    i32 m_20; // +0x20  the level-kind tag the ctor switches on
};
struct HazLookupEntry {
    char m_pad00[0x24];
    i32 m_24; // +0x24  the per-effect AniPad bias
};
struct HazStrMap {
    i32 Lookup(const char* key, HazLookupEntry** out); // 0x1b8438 (ret 8)
};
struct HazSndCat {
    char m_pad00[0x10];
    HazStrMap m_10; // +0x10  the lookup map
};
struct HazSndRoot {
    char m_pad00[0x2c];
    HazSndCat* m_2c; // +0x2c
};
struct HazGrid {
    char m_pad00[0x08];
    char** m_8; // +0x08  row table (m_8[row] -> cell row base; cells are 0x1c B)
    i32 m_c;    // +0x0c  width  (col bound)
    i32 m_10;   // +0x10  height (row bound)
};
struct HazGridMgr {
    i32 ScreenToCell(i32 x, i32 y, i32* outA, i32* outB, i32 z); // 0x35f3 thunk
    void MarkCell(i32 a, i32 b, i32 id, i32 flag);               // 0x2e96 thunk
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// ===========================================================================
// FireActivation's per-coordinate activation registry (CStaticHazard's OWN
// instance @0x64e3d0) - the SAME archetype as CTimeBomb::FireActivation. A
// coordinate maps to an Entry* either directly (within [g_haznLo,g_haznHi]) or by
// a slow Find/rebuild. All globals are unnamed BSS (DATA-pinned so the loads
// reloc-mask); the collection methods are external/no-body.
// ===========================================================================
struct CHaznEntry; // an entry: first dword is the registered handler
struct CHaznColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
extern "C" i32 ActAlloc(); // 0x16d990

DATA(0x0024e3d0)
extern CHaznColl g_haznColl;
DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

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
extern i32 g_nextActId;
DATA(0x0020a454)
extern char s_actKeyA[]; // "A"
DATA(0x0020d1bc)
extern char s_actKeyB[]; // "B"
DATA(0x002bf650)
extern CHaznColl g_nameReg; // 0x6bf650
DATA(0x002bf654)
extern CHaznColl2* g_nameReg2; // 0x6bf654
DATA(0x002bf658)
extern i32 g_nameRegLo;
DATA(0x002bf65c)
extern i32 g_nameRegHi;
DATA(0x002bf660)
extern char* g_nameRegBase;
DATA(0x002bf668)
extern i32 g_nameRegStride;
DATA(0x002bf664)
extern char* g_nameRegCur;
DATA(0x002bf66c)
extern void** g_nameRegCurList;
DATA(0x002bf670)
extern i32 g_nameRegScratch;

// The CString in the resolved name slot: ~CString (0x1b9b93) frees the old list,
// operator= (0x1b9e74) assigns the new key. Modeled so the calls reloc-mask.
#include <Gruntz/CActName.h> // CActName (shared)

// The id->name-slot resolve (fast range path + slow Find/ActAlloc/Insert rebuild).
static inline char* ActNameLookup(i32 id) {
    g_nameRegScratch = 0;
    if (id >= g_nameRegLo && id <= g_nameRegHi) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    if (g_nameReg.Find(id, 0)) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_nameReg2->Insert(&g_nameReg, item, 0xc);
    return g_nameRegCur;
}

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CHaznEntry* HaznLookup(i32 coord) {
    g_haznScratch = 0;
    if (coord >= g_haznLo && coord <= g_haznHi) {
        return (CHaznEntry*)(g_haznBase + (coord - g_haznLo) * g_haznStride);
    }
    if (g_haznColl.Find(coord, 0)) {
        return (CHaznEntry*)(g_haznBase + (coord - g_haznLo) * g_haznStride);
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_haznColl2->Insert(&g_haznColl, item, 0xc);
    return g_haznCur;
}

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
    // re-arm the IDLE geometry + STATICHAZARD sprite (SetAnimEx idiom).
    m_40 = ((WwdGameObject*)m_38)->m_1b4;
    ((WwdGameObject*)m_38)->ApplyLookupGeometry("LEVEL_STATICHAZARDIDLE", 0);
    {
        HazAnimDesc* d = (HazAnimDesc*)((WwdGameObject*)m_38)->m_1b4;
        HazAnimElem* e = d->m_10 > 0 ? *d->m_c : 0;
        ((WwdGameObject*)m_38)->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_14);
    }
    // snap the bound object's screen position to tile center.
    ((WwdGameObject*)m_10)->m_5c = (((WwdGameObject*)m_10)->m_5c & ~0x1f) + 0x10;
    ((WwdGameObject*)m_10)->m_60 = (((WwdGameObject*)m_10)->m_60 & ~0x1f) + 0x10;
    if (((WwdGameObject*)m_10)->m_74 != 0) {
        ((WwdGameObject*)m_10)->m_74 = 0;
        ((WwdGameObject*)m_10)->m_08 |= 0x20000;
    }
    m_64 = ((WwdGameObject*)m_10)->m_5c >> 5;
    m_68 = ((WwdGameObject*)m_10)->m_60 >> 5;
    ((WwdGameObject*)m_10)->m_128 = 0;
    switch (((HazSwitchSrc*)g_gameReg->m_2c)->m_20) {
        case 3:
        case 4:
        case 7:
        case 8:
            ((WwdGameObject*)m_10)->m_128 = ((WwdGameObject*)m_10)->m_60 + 0x186b0;
            break;
        default:
            break;
    }
    ((WwdGameObject*)m_10)->m_144 = ((WwdGameObject*)m_10)->m_5c - 7;
    ((WwdGameObject*)m_10)->m_14c = ((WwdGameObject*)m_10)->m_144 + 14;
    ((WwdGameObject*)m_10)->m_148 = ((WwdGameObject*)m_10)->m_60 - 7;
    ((WwdGameObject*)m_10)->m_150 = ((WwdGameObject*)m_10)->m_148 + 14;
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    ((WwdGameObject*)m_38)->m_08 |= 0x2000002;
    ((WwdGameObject*)m_10)->m_1a0.m_2c = 0;
    ((WwdGameObject*)m_10)->m_124 = g_64553c;
    m_58 = 0;
    m_5c = ((WwdGameObject*)m_10)->m_120;
    m_54 = g_645588;
    HazLookupEntry* entry = 0;
    ((HazSndRoot*)g_gameReg->m_30)->m_2c->m_10.Lookup("LEVEL_STATICHAZARDGO", &entry);
    if (entry != 0) {
        m_58 = g_buteMgr.GetIntDef("Hazardz", "AniPad", 0x64) + entry->m_24;
    } else {
        g_gameReg->EmitEvent(0x8009, 0x461);
    }
    if (((WwdGameObject*)m_10)->m_120 == 0) {
        m_5c = m_58;
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
    ((CHaznEntry2*)HaznLookup(id))->m_fn = &CStaticHazard::LoadAttributes2;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        id2 = g_nextActId;
        g_buteTree.Insert(s_actKeyB, (void*)id2);
        char* slot = ActNameLookup(id2);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyB);
        g_nextActId++;
    }
    ((CHaznEntry2*)HaznLookup(id2))->m_fn = &CStaticHazard::LoadAttributes;
}

// ---------------------------------------------------------------------------
// CStaticHazard::LoadAttributes2 @0x0fc0b0 - the time-gated pulse: bail when the
// registry is in the gated state; compute the running phase modulo the window;
// on a hit latch m_60, re-arm the GO geometry/STATICHAZARD sprite (SetAnimEx
// idiom), and re-resolve the "B" anim-set node through the global bute tree.
//
// @early-stop
// store-vs-load scheduling wall (docs/patterns/statement-schedule-faithful.md):
// logic 100% correct, all reloc operands named. Retail emits `mov [m_60],1` before
// reloading m_38; MSVC5 here hoists the m_38 load above the store. The m_38 reload
// is load-bearing (re-read per call), so the store/load pair can't be pinned.
// ~98%. Parked for the final sweep.
RVA(0x000fc0b0, 0xb2)
i32 CStaticHazard::LoadAttributes2() {
    CGameRegistry* reg = g_gameReg;
    if (reg->m_118 != 0 && reg->m_134 == 1) {
        return 0;
    }
    u32 phase = g_645588 - m_54;
    u32 base = (u32)((WwdGameObject*)m_10)->m_118;
    if (phase <= base) {
        return 0;
    }
    phase -= base;
    u32 span = m_5c + m_58;
    if (phase % span > (u32)m_58) {
        return 0;
    }
    m_60 = 1;
    m_40 = ((WwdGameObject*)m_38)->m_1b4;
    ((WwdGameObject*)m_38)->ApplyLookupGeometry("LEVEL_STATICHAZARDGO", 0);
    {
        HazAnimDesc* d = (HazAnimDesc*)((WwdGameObject*)m_38)->m_1b4;
        HazAnimElem* e = d->m_10 > 0 ? *d->m_c : 0;
        ((WwdGameObject*)m_38)->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_14);
    }
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("B");
    return 0;
}

// ---------------------------------------------------------------------------
// CStaticHazard::LoadAttributes @0x0fc1a0 - the full periodic tick. Compute the
// running phase modulo the on/off window; depending on the window half + the
// fired flag (m_60) + the bound object's gate (m_120), re-arm the GO or IDLE
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
    u32 phase = (g_645588 - m_54) - (u32)((WwdGameObject*)m_10)->m_118;
    u32 rem = phase % (u32)(m_5c + m_58);
    if (rem > (u32)m_58) {
        // idle window
        if (m_60 == 0) {
            goto dispatch;
        }
        if (((WwdGameObject*)m_10)->m_120 != 0) {
            // re-arm IDLE (cache the anim-set node first)
            m_30 = m_14->m_1c;
            m_14->m_1c = g_buteTree.Find("A");
            m_40 = ((WwdGameObject*)m_38)->m_1b4;
            ((WwdGameObject*)m_38)->ApplyLookupGeometry("LEVEL_STATICHAZARDIDLE", 0);
            {
                HazAnimDesc* d = (HazAnimDesc*)((WwdGameObject*)m_38)->m_1b4;
                HazAnimElem* e = d->m_10 > 0 ? *d->m_c : 0;
                ((WwdGameObject*)m_38)->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_14);
            }
            if (((WwdGameObject*)m_10)->m_74 != 0) {
                ((WwdGameObject*)m_10)->m_74 = 0;
                ((WwdGameObject*)m_10)->m_08 |= 0x20000;
            }
            // clear the hazard cell's bit-0x8000000
            HazGrid* grid = (HazGrid*)g_gameReg->m_70;
            if ((u32)m_64 < (u32)grid->m_c && (u32)m_68 < (u32)grid->m_10) {
                *(i32*)(grid->m_8[m_68] + m_64 * 0x1c) &= 0xf7ffffff;
            }
            return 0;
        }
        // m_120 == 0: re-arm GO + clear the fired flag
        m_40 = ((WwdGameObject*)m_38)->m_1b4;
        ((WwdGameObject*)m_38)->ApplyLookupGeometry("LEVEL_STATICHAZARDGO", 0);
        {
            HazAnimDesc* d = (HazAnimDesc*)((WwdGameObject*)m_38)->m_1b4;
            HazAnimElem* e = d->m_10 > 0 ? *d->m_c : 0;
            ((WwdGameObject*)m_38)->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_14);
        }
        if (((WwdGameObject*)m_10)->m_74 != 0) {
            ((WwdGameObject*)m_10)->m_74 = 0;
            ((WwdGameObject*)m_10)->m_08 |= 0x20000;
        }
        m_60 = 0;
        return 0;
    } else {
        // active window
        if (m_60 != 0) {
            goto dispatch;
        }
        if (((WwdGameObject*)m_10)->m_120 != 0) {
            goto dispatch;
        }
        // turn on: re-arm GO, latch the fired flag
        m_40 = ((WwdGameObject*)m_38)->m_1b4;
        ((WwdGameObject*)m_38)->ApplyLookupGeometry("LEVEL_STATICHAZARDGO", 0);
        {
            HazAnimDesc* d = (HazAnimDesc*)((WwdGameObject*)m_38)->m_1b4;
            HazAnimElem* e = d->m_10 > 0 ? *d->m_c : 0;
            ((WwdGameObject*)m_38)->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_14);
        }
        if (((WwdGameObject*)m_10)->m_74 != 0) {
            ((WwdGameObject*)m_10)->m_74 = 0;
            ((WwdGameObject*)m_10)->m_08 |= 0x20000;
        }
        m_60 = 1;
        return 0;
    }

dispatch:
    if (((WwdAnimSub*)((char*)m_38 + 0x1a0))->SetAnim(g_6bf3bc) == 2) {
        i32 a = 0, b = 0;
        if (((HazGridMgr*)g_gameReg->m_68)
                ->ScreenToCell(
                    ((WwdGameObject*)m_10)->m_5c,
                    ((WwdGameObject*)m_10)->m_60,
                    &a,
                    &b,
                    0
                )
            != 0) {
            ((HazGridMgr*)g_gameReg->m_68)->MarkCell(a, b, ((WwdGameObject*)m_10)->m_124, -1);
        }
        if (((WwdGameObject*)m_10)->m_74 != ((WwdGameObject*)m_10)->m_128) {
            ((WwdGameObject*)m_10)->m_74 = ((WwdGameObject*)m_10)->m_128;
            ((WwdGameObject*)m_10)->m_08 |= 0x20000;
        }
        HazGrid* grid = (HazGrid*)g_gameReg->m_70;
        if ((u32)m_64 < (u32)grid->m_c && (u32)m_68 < (u32)grid->m_10) {
            *(i32*)(grid->m_8[m_68] + m_64 * 0x1c) |= 0x8000000;
        }
    } else {
        HazGrid* grid = (HazGrid*)g_gameReg->m_70;
        if ((u32)m_64 < (u32)grid->m_c && (u32)m_68 < (u32)grid->m_10) {
            *(i32*)(grid->m_8[m_68] + m_64 * 0x1c) &= 0xf7ffffff;
        }
        if (((WwdGameObject*)m_10)->m_74 != 0) {
            ((WwdGameObject*)m_10)->m_74 = 0;
            ((WwdGameObject*)m_10)->m_08 |= 0x20000;
        }
    }
    {
        WwdAnimSub* sub = (WwdAnimSub*)((char*)m_38 + 0x1a0);
        if (sub->m_28 != 0 && sub->m_20 == 0) {
            m_40 = ((WwdGameObject*)m_38)->m_1b4;
            ((WwdGameObject*)m_38)->ApplyLookupGeometry("LEVEL_STATICHAZARDIDLE", 0);
            {
                HazAnimDesc* d = (HazAnimDesc*)((WwdGameObject*)m_38)->m_1b4;
                HazAnimElem* e = d->m_10 > 0 ? *d->m_c : 0;
                ((WwdGameObject*)m_38)->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_14);
            }
            HazGrid* grid = (HazGrid*)g_gameReg->m_70;
            if ((u32)m_64 < (u32)grid->m_c && (u32)m_68 < (u32)grid->m_10) {
                *(i32*)(grid->m_8[m_68] + m_64 * 0x1c) &= 0xf7ffffff;
            }
        }
    }
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
SIZE_UNKNOWN(CHaznColl2);
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
SIZE_UNKNOWN(HazStrMap);
SIZE_UNKNOWN(HazSwitchSrc);
SIZE_UNKNOWN(WwdAnimSub);
SIZE_UNKNOWN(WwdGameObject);
