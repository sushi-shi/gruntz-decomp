// Projectile.cpp - the CProjectile game-object (C:\Proj\Gruntz). Continues the
// CUserBase/CUserLogic/CMovingLogic hierarchy (see include/Gruntz/Projectile.h).
//
// CProjectile::CProjectile() (0x126e0) is the no-arg ctor: it folds the inline
// CMovingLogic init (the +0x38..+0x10c motion ints + the twelve default-bound
// doubles), constructs the +0x204 tracked-hit CObList, and stamps its vftable.
// Like the rest of the family it constructs a throwing CUserBaseLink (in the
// CUserLogic base) + a CObList, so MSVC emits the /GX EH frame -> built eh.
#include <Gruntz/Projectile.h>
#include <Bute/ButeMgr.h> // CButeTree (the type-registry funnel)
#include <math.h>         // sin / cos (StepMotion's parabola)
#include <rva.h>

// StepMotion's two motion-phase thresholds (.rdata doubles) + the int amplitude
// global it folds into the trajectory (loaded as a double via fild). DATA pins so
// the fcomp/mov loads reloc-mask against the named symbols.
extern const double g_projPhase0;
extern const double g_projPhase1;
DATA(0x00245584)
extern i32 g_645584;

// ---------------------------------------------------------------------------
// Externs the reconstructed projectile methods reference (reloc-masked).
// ---------------------------------------------------------------------------
// The global node free-list the dtor / hit-scan recycle tracked-hit nodes onto.
DATA(0x00245544)
extern void* g_freeList;
DATA(0x0024554c)
extern i32 g_freeListNodeBias;

// The draw-clock delta global passed to the render object's SetAnim on detach.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// The game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A). Modeled here with
// the offsets the projectile sound/hit-scan paths touch; the DATA pin reloc-masks
// the `mov ecx,ds:g_gameReg` load against the already-named symbol.
struct CProjSoundEntry; // map value: the per-effect sound table entry
struct CProjSoundInner; // reg->m_30->m_28: holds the CMapStringToOb at +0x10
struct CProjSoundCat {  // reg->m_30: the sound-category object
    char m_pad00[0x28];
    CProjSoundInner* m_28; // +0x28  -> the lookup map lives at (*m_28)+0x10
};
struct CGameReg {
    char m_pad00[0x10];
    i32 m_10; // +0x10  gate (must be non-null)
    char m_pad14[0x30 - 0x14];
    CProjSoundCat* m_30; // +0x30
    char m_pad34[0x68 - 0x34];
    void* m_68; // +0x68  the 15x15 grunt-grid base
    char m_pad6c[0x11c - 0x6c];
    i32 m_11c; // +0x11c  the sound-channel param
};
DATA(0x0024556c)
extern CGameReg* g_gameReg;

// A grunt in the hit-scan grid (g_gameReg->m_68 is a flat 15x15 cell table; each
// cell holds a grunt ptr). Only the offsets ScanTargets touches are modeled: the
// +0x10 owner (screen pos at +0x5c/+0x60), the +0x1fc live-projectile slot, the
// +0x170 alt-state gate, and the (+0x1ec,+0x1f0) spawn-cell key. The two hit
// handlers are out-of-line CGrunt methods (reloc-masked, reached via ILT thunks).
struct CGruntOwner {
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c  screen X
    i32 m_60; // +0x60  screen Y
};
struct CGruntTarget {
    void DeliverHit(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g, i32 h); // 0x4646b0 (ret 0x20)
    void SelfImpact(i32 a, i32 b, i32 c, i32 d);                             // 0x4dd50  (ret 0x10)
    char m_pad00[0x10];
    CGruntOwner* m_10; // +0x10
    char m_pad14[0x170 - 0x14];
    i32 m_170; // +0x170
    char m_pad174[0x1ec - 0x174];
    i32 m_1ec, m_1f0; // +0x1ec/+0x1f0  spawn-cell key
    char m_pad1f4[0x1fc - 0x1f4];
    i32 m_1fc; // +0x1fc  live-projectile slot
};

// A {x, y} cell-key node recycled through the global free-list (the same 2-int
// pair node BattlezMapConfig pulls). m_0 doubles as the free-list link.
struct CHitKey {
    i32 m_0;
    i32 m_4;
};

// The map value the launch-sound lookup returns: its +0x10 sub-object owns the
// sample factory (GetItem 0x135d70, __thiscall) the projectile clones from.
struct CProjSampleFactory {
    CProjSample* GetItem(); // 0x135d70 (__thiscall, 0 args; returns a new sample)
};
struct CProjSoundEntry {
    char m_pad00[0x10];
    CProjSampleFactory* m_10; // +0x10
};
// The CMapStringToOb the category exposes (its Lookup is the engine method
// 0x1b8438). Embedded inside reg->m_30->m_28 at +0x10.
struct CProjSoundMap {
    i32 Lookup(const char* key, CProjSoundEntry** out); // 0x1b8438 (ret 8)
};
struct CProjSoundInner {
    char m_pad00[0x10];
    CProjSoundMap m_10; // +0x10  the lookup map
};

// The shared default-bound doubles the CMovingLogic ctor copies into the twelve
// coordinate-bound members (retail .rdata 0x5f04b0 / 0x5f04b8). Defined here with
// DATA() pins so the ctor's dword loads reloc-mask against them.
DATA(0x001f04b0)
const double g_movingLogicMin = -2147483647.0;
DATA(0x001f04b8)
const double g_movingLogicMax = 2147483646.0;

// ---------------------------------------------------------------------------
// Out-of-line vtable anchors. Give CMovingLogic / CProjectile real vftables in
// this TU so the inline ctors emit their vptr stores. Bodies are not matched.
// ---------------------------------------------------------------------------
CMovingLogic::~CMovingLogic() {}
i32 CMovingLogic::MovingLogicVfunc() {
    return 0;
}
i32 CMovingLogic::MovingLogicVfunc2() {
    return 0;
}
i32 CMovingLogic::MovingLogicVfunc3() {
    return 0;
}

i32 CProjectile::ProjectileVfunc() {
    return 0;
}

// @confidence: high
// @source: rtti-vptr
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): body, all
// field offsets, the double init and the CObList construction are byte-identical;
// the only residue is this ctor's OWN __ehfuncinfo (the EH prologue pushes
// funcinfo+0x0 vs retail funcinfo+0xe, and the CObList state id is `mov
// [esp+0x18],1` vs retail 2). NOT resolvable by completing the projectile TU:
// the funcinfo is per-function, and the out-of-line CMovingLogic ctor (0x13940)
// cannot be emitted from this TU (the no-arg ctor must keep CMovingLogic() inline
// to fold it; forcing the standalone drops this ctor 99%->19.7%; nothing here
// calls CMovingLogic out-of-line). See the CAVEAT in the pattern doc. ~99% wall.
RVA(0x000126e0, 0x1fc)
CProjectile::CProjectile() {}

// ---------------------------------------------------------------------------
// CProjectile::ReleaseDeferred (0x13c70) - fire the two queued one-shot callbacks
// (m_08 first, gated on the recorded hit-handle still matching m_28; then m_04
// unconditionally), reset the handle to its default 0x3e9, then run the slot-16
// virtual. The callbacks are raw __thiscall code pointers cached in the inherited
// CUserLogic m_04/m_08 ints; expressed as pointer-to-member-functions so the
// `mov ecx,this; call ptr` falls out (MSVC5 reserves the __thiscall keyword).
// ---------------------------------------------------------------------------
typedef void (CProjectile::*ProjCallback)();

RVA(0x00013c70, 0x47)
void CProjectile::ReleaseDeferred(i32) {
    if (m_04 != 0) {
        if (m_08 != 0 && (i32)m_14->m_1c == m_28) {
            (this->*(ProjCallback&)m_08)();
            m_08 = 0;
        }
        (this->*(ProjCallback&)m_04)();
        m_04 = 0;
        m_28 = 0x3e9;
    }
    MovingLogicVfunc3(); // virtual slot 16 (vtable offset 0x40)
}

// ---------------------------------------------------------------------------
// CProjectile::~CProjectile (0xdef60) - the most-derived dtor. Stop+rewind the
// launch sample, recycle each tracked-hit node back onto the global free-list,
// RemoveAll the list, then the compiler auto-destructs the CObList member and the
// CMovingLogic/CUserLogic/link base subobjects (the throwing link forces the /GX
// frame). Field names are placeholders; the offsets are load-bearing.
//
// @early-stop
// EH-state-numbering / base-dtor-inlining wall (docs/patterns/eh-state-numbering-base.md):
// the body is byte-identical through `m_204.RemoveAll()` (the m_200 stop, the
// free-list recycle walk, the AddTail/RemoveAll). The residue is the base-dtor
// tail: retail INLINES the whole CMovingLogic/CUserLogic/CUserBaseLink chain (vptr
// restamp 0x5e705c, ~EngStr on +0x18, vptr 0x5e70b4) and numbers the EH states
// 2/1/3 in a `sub esp,8` frame; our recompile emits an out-of-line `~CMovingLogic`
// call with states 1/0/-1 in a `push ecx` frame. Inlining the chain is not
// reachable from this TU without un-emitting the standalone base dtors (matched
// elsewhere) - the same per-function funcinfo wall the no-arg ctor hits. ~90%.
// ---------------------------------------------------------------------------
RVA(0x000def60, 0xbc)
CProjectile::~CProjectile() {
    if (m_200 != 0) {
        m_200->StopAndRewind();
        m_200 = 0;
    }
    for (POSITION pos = m_204.GetHeadPosition(); pos != NULL;) {
        CObject* data = m_204.GetNext(pos);
        if (data != 0) {
            void** node = (void**)((char*)data - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_204.RemoveAll();
}

// ===========================================================================
// CProjectile level-load registration (the same dual-registry archetype as
// CKitchenSlime::RegisterType): a per-coordinate activation table (R2, the
// projectile's own collection at @0x64c758) and the shared game-object type-name
// table (R1, @0x6bf650) keyed by the per-type id the global bute-tree assigns to
// the class name ("A"). All globals are BSS / DATA-pinned (reloc-masked); the
// collection / CString helpers are external/no-body.
// ===========================================================================

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 / Insert 0x16db90).
extern CButeTree g_buteTree;

// The activation-collection methods (shared with the per-class registries):
//   Find  0x16da80 (__thiscall ret 8), Insert 0x16d850 (__thiscall ret 0xc),
//   ActAlloc 0x16d990, RegisterRange 0x408710 (via 0x3742 thunk).
struct CProjColl {
    i32 Find(i32 coord, i32 z);         // 0x16da80
    void RegisterRange(i32 lo, i32 hi); // 0x408710 (0x0df920 callee)
};
struct CProjColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850
};
extern "C" i32 ProjActAlloc(); // 0x16d990
DATA(0x002bf464)
extern void* g_projActCache; // 0x6bf464 (shared alloc cache)
DATA(0x002bf428)
extern void* g_projActAllocResult; // 0x6bf428

// R1 - the shared type-name table (@0x6bf650).
struct CProjTypeEntry;
DATA(0x002bf658)
extern i32 g_projTypeLo;
DATA(0x002bf65c)
extern i32 g_projTypeHi;
DATA(0x002bf660)
extern char* g_projTypeBase;
DATA(0x002bf668)
extern i32 g_projTypeStride;
DATA(0x002bf664)
extern CProjTypeEntry* g_projTypeCur;
DATA(0x002bf670)
extern i32 g_projTypeCount;
DATA(0x002bf650)
extern CProjColl g_projTypeColl;
DATA(0x002bf654)
extern CProjColl2* g_projTypeColl2;
DATA(0x002bf66c)
extern void* g_projTypeNodes;
DATA(0x0021aea8)
extern i32 g_projTypeCounter; // 0x61aea8 (global type counter)

// R2 - the projectile's per-coordinate activation table (@0x64c758).
struct CProjActEntry;
extern i32 g_projActLo;
extern i32 g_projActHi;
extern char* g_projActBase;
extern i32 g_projActStride;
extern CProjActEntry* g_projActCur;
extern i32 g_projActScratch;
DATA(0x0024c758)
extern CProjColl g_projActColl;
extern CProjColl2* g_projActColl2;

// The CString slot teardown (0x1b9b93 __thiscall) + name assign (0x1b9e74).
struct CProjStringNode {
    void* m_0;
    void Free(); // 0x1b9b93
};
struct CProjTypeEntryView {
    void Assign(const char* name); // 0x1b9e74
};

// The projectile's activation handler (LAB_00403896, an ILT thunk).
extern "C" void ProjActivationHandler(); // 0x403896

// R2 lookup (projectile activation table).
static inline CProjActEntry* ProjActLookup(i32 coord) {
    g_projActScratch = 0;
    if (coord >= g_projActLo && coord <= g_projActHi) {
        return (CProjActEntry*)(g_projActBase + (coord - g_projActLo) * g_projActStride);
    }
    if (g_projActColl.Find(coord, 0)) {
        return (CProjActEntry*)(g_projActBase + (coord - g_projActLo) * g_projActStride);
    }
    void* item = g_projActCache;
    g_projActAllocResult = (void*)ProjActAlloc();
    g_projActColl2->Insert(&g_projActColl, item, 0xc);
    return g_projActCur;
}

// R1 lookup (shared type-name table).
static inline CProjTypeEntry* ProjTypeLookup(i32 key) {
    g_projTypeCount = 0;
    if (key >= g_projTypeLo && key <= g_projTypeHi) {
        return (CProjTypeEntry*)(g_projTypeBase + (key - g_projTypeLo) * g_projTypeStride);
    }
    if (g_projTypeColl.Find(key, 0)) {
        return (CProjTypeEntry*)(g_projTypeBase + (key - g_projTypeLo) * g_projTypeStride);
    }
    void* item = g_projActCache;
    g_projActAllocResult = (void*)ProjActAlloc();
    g_projTypeColl2->Insert(&g_projTypeColl, item, 0xc);
    return g_projTypeCur;
}

// CProjectile::RegisterRange @0x0df920 - seed the projectile's activation table's
// fast-range bounds (RegisterRange(0x7d0, 0x7da)). A static initializer.
RVA(0x000df920, 0x15)
void CProjectile::RegisterRange() {
    g_projActColl.RegisterRange(0x7d0, 0x7da);
}

// CProjectile::RegisterType @0x0dfb00 - the level-load class registrar (same
// archetype as CKitchenSlime::RegisterType): assign the class a type-id via the
// global bute-tree, record the name into the shared type-name table, then store
// the projectile's activation handler (0x403896) into the per-class table.
// @early-stop
// ~91%: byte-correct operations/offsets/strings/calls; the residual is the same
// regalloc + count-down induction wall RegisterType carries (type-id register
// coloring + the `ecx=cnt; eax=cnt-1; lea ebp,[eax+1]` node-free loop idiom). Not
// source-steerable; deferred to the final sweep.
RVA(0x000dfb00, 0x18d)
void CProjectile::RegisterType() {
    i32 id = (i32)g_buteTree.Find("A");
    if (id == 0) {
        g_buteTree.Insert("A", (void*)g_projTypeCounter);
        i32 key = g_projTypeCounter;
        id = key;
        CProjTypeEntry* slot = ProjTypeLookup(key);
        i32 cnt = g_projTypeCount;
        CProjStringNode* nodes = (CProjStringNode*)g_projTypeNodes;
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    nodes->Free();
                }
                nodes++;
            } while (--cnt);
        }
        ((CProjTypeEntryView*)slot)->Assign("A");
        g_projTypeCounter++;
    }
    *(void**)ProjActLookup(id) = (void*)&ProjActivationHandler;
}

// ---------------------------------------------------------------------------
// CProjectile::DetachRenderObj (0xe05e0) - clear the render object's bit-0 flag,
// re-target its animation to the current draw-delta, and (when the object is in
// the "active but un-anchored" state) raise its hide bit. Returns 0.
// ---------------------------------------------------------------------------
RVA(0x000e05e0, 0x4e)
i32 CProjectile::DetachRenderObj() {
    m_154->m_40 &= ~1u;
    m_154->m_1a0.SetAnim(g_6bf3bc);
    CProjRenderObj* r = m_154;
    if (r->m_1c8 != 0 && r->m_1c0 == 0) {
        r->m_08 |= 0x10000;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CProjectile::StepMotion (0xe08b0) - advance the projectile one frame. On the
// launch frame it snaps the render objects to the muzzle (m_17c/m_180); once past
// the second phase threshold it expires (scan for the terminal impact, raise the
// hide bit on the shadow + owner). Otherwise it integrates the sin/cos parabola
// into the render position and rounds it into the render objects' screen coords.
//
// @early-stop
// x87-scheduling wall: the sin/cos parabola block (0xe0929..0xe09e1) is a dense
// fxch-laden FP stack schedule MSVC5 emits from the trajectory expression; the
// control flow, the muzzle-snap, the expire path and all __ftol rounds match, but
// the FP body's stack ordering is not steerable from C source. ~70% plateau.
// ---------------------------------------------------------------------------
RVA(0x000e08b0, 0x1de)
void CProjectile::StepMotion() {
    i32 impact = 0;
    if (m_258 == 0) {
        if (m_250 > g_projPhase0) {
            // launch frame: snap render objects to the muzzle, mark launched.
            m_10->m_5c = m_17c;
            m_10->m_60 = m_180;
            if (m_1fc != 0) {
                m_1fc->m_5c = m_17c;
                m_1fc->m_60 = m_180;
            }
            m_258 = 1;
            goto step;
        }
    } else if (m_250 > g_projPhase1) {
        // past the terminal threshold: deliver the impact scan + expire.
        ScanTargets(1);
        if (m_1fc != 0) {
            m_1fc->m_08 |= 0x10000;
            m_1fc = 0;
        }
        m_154->m_08 |= 0x10000;
        return;
    }
step:
    ScanTargets(impact);
    // integrate the sin/cos parabola into the render position.
    double s = sin(m_250);
    double c = cos(m_250);
    double amp = (double)g_645584;
    double vx = -m_230;
    double vy = m_238;
    double px = m_240 + vy * m_198 * s - vx * amp * c + m_250;
    double py = m_248 + vy * amp * c + vx * m_198 * s;
    m_1a0 = px;
    m_1a8 = py;
    m_250 = px;
    m_10->m_5c = (i32)m_1a0;
    m_10->m_60 = (i32)m_1a8;
    if (m_1fc != 0) {
        m_1fc->m_5c = (i32)m_1a0;
        m_1fc->m_60 = (i32)m_1a8;
    }
}

// ---------------------------------------------------------------------------
// CProjectile::ScanTargets (0xe0b10) - sweep the 15x15 grunt grid centered on the
// projectile, AABB-test each grunt's 14x14 footprint against the projectile's
// 0x20x0x20 box, and for each fresh overlap (not self, not already tracked) record
// the grunt's spawn-cell key in the tracked-hit list and deliver the hit. The
// self cell, when `impact` is set, triggers the grunt's self-impact handler.
//
// @early-stop
// Logic byte-exact end to end (the grid scan, the AABB test, the GetNext list
// walk, the free-list pull/AddTail, the deliver/self-impact branches, the void
// epilogue). Residue is a register-coloring difference in the bbox slot pair
// (projXhi/projYlo land in swapped [esp+0x20]/[esp+0x24] vs retail) and the
// 8-arg DeliverHit push temps (edx/eax/ecx vs retail eax/ecx/edx) - same values,
// same push order, different temp registers - plus the three unnamed engine-call
// relocs (AddTail/DeliverHit/SelfImpact at instantiation-specific RVAs not yet in
// symbol_names.csv). ~94%, not steerable from C source.
// ---------------------------------------------------------------------------
RVA(0x000e0b10, 0x1bd)
void CProjectile::ScanTargets(i32 impact) {
    i32 tileY = 0;                   // [esp+0x10]  outer (row) counter
    i32 projXlo = m_10->m_5c - 0x10; // [esp+0x1c]  m_10 = owner CGameObject
    i32 projYlo = m_10->m_60 - 0x10; // [esp+0x20]
    i32 projXhi = projXlo + 0x20;    // [esp+0x24]
    i32 projYhi = projYlo + 0x20;    // [esp+0x28]
    i32 rowBase = 0x1c;              // [esp+0x18]  row byte stride base
    i32 colOff;                      // [esp+0x14]
    i32 col;                         // ebp
    do {
        col = 0;
        colOff = rowBase;
        for (; col < 0xf; col++, colOff += 4) {
            CGruntTarget* g = *(CGruntTarget**)((char*)g_gameReg->m_68 + colOff);
            if (g == 0) {
                continue;
            }
            if (g->m_1fc == 0) {
                continue;
            }
            i32 gx = g->m_10->m_5c - 7;
            i32 gy = g->m_10->m_60 - 7;
            i32 gxhi = gx + 0xe;
            i32 gyhi = gy + 0xe;
            if (projXlo > gxhi) {
                continue;
            }
            if (projXhi < gx) {
                continue;
            }
            if (projYlo > gyhi) {
                continue;
            }
            if (projYhi < gy) {
                continue;
            }
            if (m_174 == tileY && m_178 == col) {
                // self cell
                if (impact != 0 && g->m_1fc != 0 && g->m_170 == 0) {
                    g->SelfImpact(2, 1, 0, 0);
                }
                return;
            }
            // already-tracked? walk the hit list for this grunt's cell key.
            i32 keyX = g->m_1ec;
            i32 keyY = g->m_1f0;
            for (POSITION pos = m_204.GetHeadPosition(); pos != NULL;) {
                CHitKey* k = (CHitKey*)m_204.GetNext(pos);
                if (k->m_0 == keyX && k->m_4 == keyY) {
                    return;
                }
            }
            // fresh hit: pull a key node off the free-list, record + deliver.
            CHitKey* slot = 0;
            CHitKey* p = (CHitKey*)g_freeList;
            if (p->m_0 != 0) {
                slot = (CHitKey*)&p->m_4;
                slot->m_0 = keyX;
                slot->m_4 = keyY;
                g_freeList = (void*)p->m_0;
            }
            m_204.AddTail((CObject*)slot);
            g->DeliverHit(m_170, 1, m_174, m_178, m_220, m_224, 1, 0);
        }
        rowBase += 0x3c;
        tileY++;
    } while (rowBase < 0x10c);
}

// ---------------------------------------------------------------------------
// CProjectile::LaunchSound (0xe2190) - lazily create + play the launch sound the
// first time. Look the effect up in the game-registry sound map by name, clone a
// sample off the matched entry, store it at m_200, and start it on the configured
// channel. Returns 1 on success, 0 if already launched / any lookup gate fails.
//
// @early-stop
// Code bytes byte-exact vs retail (verified by full llvm-objdump compare); the
// residue is purely the reloc-naming artifact (docs/patterns, objdiff-reloc-scoring
// memory): the four engine callees - CMapStringToOb::Lookup (0x1b8438), the sample
// factory GetItem (0x135d70) and CSample Play/StopAndRewind (0x136300) - are not
// yet named in symbol_names.csv, so their REL32 relocs stay fuzzy against the
// target's FUN_ names. g_gameReg IS named (CGameReg). Flips to exact once those
// engine functions get RVA-annotated stubs. ~44% scoring artifact, logic complete.
// ---------------------------------------------------------------------------
RVA(0x000e2190, 0x83)
i32 CProjectile::LaunchSound(const char* key) {
    if (m_200 != 0) {
        return 0;
    }
    CGameReg* reg = g_gameReg;
    if (reg->m_10 == 0) {
        return 0;
    }
    CProjSoundCat* cat = reg->m_30;
    CProjSoundEntry* entry = 0;
    cat->m_28->m_10.Lookup(key, &entry);
    if (entry == 0) {
        return 0;
    }
    if (entry->m_10 == 0) {
        return 0;
    }
    m_200 = entry->m_10->GetItem();
    if (m_200 == 0) {
        return 0;
    }
    m_200->Play(g_gameReg->m_11c, 0, 0, 1);
    return 1;
}

// ===========================================================================
// 0x0ade60 - per-coordinate projectile-action dispatch.  Resolves the activation
// entry for `coord` (R2 lookup, inlined); if the entry's leading handler slot is
// non-null, re-resolves the entry and invokes the handler (__thiscall) on this
// dispatcher object.  Same global-table-driven shape as ProjActLookup's callers.
// ===========================================================================
class CProjActDispatcher {
public:
    void Dispatch(i32 coord); // 0x0ade60
};

// The entry's leading slot is a __thiscall handler taking this dispatcher; MSVC5
// rejects the __thiscall keyword, so model it as a single-inheritance member
// pointer (a bare 4-byte code address) reinterpreted from the entry word.
typedef void (CProjActDispatcher::*ProjActHandler)();

RVA(0x000ade60, 0x102)
void CProjActDispatcher::Dispatch(i32 coord) {
    CProjActEntry* e = ProjActLookup(coord);
    if (*(void**)e != 0) {
        CProjActEntry* e2 = ProjActLookup(coord);
        ProjActHandler h = *(ProjActHandler*)e2;
        (this->*h)();
    }
}
