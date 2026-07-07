// DroppedObject.cpp - a dropped game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CDroppedObject methods, defined in ascending retail-RVA
// order:
//   ~CDroppedObject   @0x0125b0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   FireActivation    @0x0c6bd0 - the per-coordinate activation-registry dispatcher.
//
// CDroppedObject : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/DroppedObject.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TypeKeyColl.h>
#include <Bute/ButeTree.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Globals.h>

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CDroppedObject::FireActivation
// (0x0c6bd0) dispatches through - the SAME archetype as CTimeBomb::FireActivation
// (0x0e1830), but CDroppedObject's OWN registry instance at 0x64bed8. A
// coordinate maps to an Entry* either directly (when within the fast
// [g_dropLo, g_dropHi] range) via g_dropBase + (coord-lo)*stride, or by a slow
// Find in the collection (0x16da80, __thiscall ret 8), which on miss rebuilds
// (GetRetAddr 0x16d990 -> g_actCache, Insert 0x16d850 __thiscall ret 0xc) and
// yields g_dropCur. The entry's first dword is a fn-ptr; a nonzero entry's
// handler is called __thiscall on `this`. All globals are unnamed BSS
// (DATA-pinned so the loads reloc-mask); the collection methods are
// external/no-body (the SAME shared engine functions every registry calls). The
// alloc-cache pair (g_actCache 0x6bf464 / g_retAddrBreadcrumb 0x6bf428) is the SAME
// shared global every registry writes (already named by KitchenSlime.cpp -
// re-declared here, address-pinned).
struct CDropEntry;         // an entry: first dword is the registered handler
extern void* GetRetAddr(); // 0x16d990

DATA(0x0024bed8)
extern CTypeKeyColl g_dropColl;
DATA(0x002bf464)
extern void* g_actCache;
extern void* g_retAddrBreadcrumb;

// ---------------------------------------------------------------------------
// RegisterActs (0x0c6d30) binds TWO activation keys ("A" and "B") into the
// dropped-object registry, each carrying its own per-frame handler. Each key is
// interned into the shared bute store (g_buteTree, named symbol so Find/Insert
// reloc-mask); a fresh id records the key in the shared name registry (@0x6bf650,
// the SAME shared instance CTimeBomb/CKitchenSlime use), then the id->Entry
// resolve in CDroppedObject's OWN registry (g_dropColl, via DropLookup) stores the
// handler code pointer. g_nextActId (0x61aea8) is the running id counter.
DATA(0x0021aea8)
extern i32 g_nextActId;
DATA(0x0020a454)
extern char s_actKeyA[]; // "A"
DATA(0x0020d1bc)
extern char s_actKeyB[]; // "B"
DATA(0x002bf650)
extern CTypeKeyColl g_nameReg; // 0x6bf650
DATA(0x002bf654)
extern CVariantSlot* g_nameReg2; // 0x6bf654
DATA(0x002bf658)
extern i32 g_nameRegLo;
DATA(0x002bf65c)
extern i32 g_nameRegHi;
DATA(0x002bf660)
extern char* g_nameRegBase;
DATA(0x002bf668)
extern i32 g_nameRegStride;
DATA(0x002bf664)
extern char* g_nameRegCur; // slow-path result slot
DATA(0x002bf66c)
extern void** g_nameRegCurList; // the slot's CString list base
DATA(0x002bf670)
extern i32 g_nameRegScratch; // zeroed first; doubles as the list count

// The shared bute store the keys intern into (?g_buteTree@@3VCButeTree@@A
// @0x6bf620; named symbol so Find/Insert reloc-mask).
extern CButeTree g_buteTree;

// The CString in the resolved name slot: ~CString (0x1b9b93) frees the old list,
// operator= (0x1b9e74) assigns the new key. Modeled so the calls reloc-mask.
#include <Gruntz/ActName.h> // CActName (shared)

// The id->name-slot resolve (fast range path + slow Find/GetRetAddr/Insert rebuild).
static inline char* ActNameLookup(i32 id) {
    g_nameRegScratch = 0;
    if (id >= g_nameRegLo && id <= g_nameRegHi) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    if ((i32)((_zvec*)&g_nameReg)->GrowTo(id, 0)) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    void* item = g_actCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_nameReg2->Set(&g_nameReg, (i32)item, 0xc);
    return g_nameRegCur;
}

// The two per-frame handlers bound into the registry slots (referenced by address
// so the DIR32 store operands reloc-mask). 0xc7090 binds to "A", 0xc7be0 to "B".
extern i32 DropActA_c7090();
extern i32 DropActB_c7be0();

// The entry's first dword is a pointer-to-member-function of CDroppedObject
// (single inheritance -> 4-byte code pointer); FireActivation invokes it on
// `this`, emitting `mov ecx,this; call [entry]`. CDroppedObject is defined
// COMPLETE in the header above this typedef so the PMF stays 4 bytes
// (pmf-complete-class-4byte).
typedef void (CDroppedObject::*DropHandler)();
struct CDropEntry {
    DropHandler m_fn; // [entry]
};

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CDropEntry* DropLookup(i32 coord) {
    g_dropScratch = 0;
    if (coord >= g_dropLo && coord <= g_dropHi) {
        return (CDropEntry*)(g_dropBase + (coord - g_dropLo) * g_dropStride);
    }
    if ((i32)((_zvec*)&g_dropColl)->GrowTo(coord, 0)) {
        return (CDropEntry*)(g_dropBase + (coord - g_dropLo) * g_dropStride);
    }
    void* item = g_actCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_dropColl2->Set(&g_dropColl, (i32)item, 0xc);
    return g_dropCur;
}

// CDroppedObject::~CDroppedObject @0x0125b0 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the
// embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to
// ~CTimeBomb @0x012a70; the empty body is enough for cl.
RVA(0x000125b0, 0x44)
CDroppedObject::~CDroppedObject() {}

#include <Bute/ButeMgr.h> // CButeMgr (g_buteMgr GetIntDef/GetDwordDef)
extern CButeMgr g_buteMgr;

// ---------------------------------------------------------------------------
// CDroppedObject::CDroppedObject(CGameObject*) @0xc68b0 - the 1-arg leaf ctor:
// the standard CUserLogic(obj) init (folded inline) plus the dropped-object tail
// - cache the anim-set node off the "A" bute key, snapshot m_38->m_1b4, apply the
// dropped-object sprite/geometry, raise the bound object's logic/collision bits,
// snap the bound object's screen position to the tile grid, then bias its Y by the
// bute "DroppedObjectYOffset" (storing the result as a double) and seed the
// per-tile time as 32.0 / bute "DroppedObjectTimePerTile". Constructs a throwing
// CUserBaseLink, so MSVC emits the /GX EH frame.
//
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-faithful to retail (the CUserLogic init, the anim-set cache, the
// ApplyName/ApplyLookupGeometry pair, the tile snap, both bute reads + the int->
// double conversions); the residue is this ctor's own __ehfuncinfo state numbering,
// the constant-enregistration coin-flip, and the `and al,0xe0` vs `and eax,~0x1f`
// byte-AND codegen pick. The SAME plateau as CBrickz / the other bute ctors; not
// source-steerable. Parked for the final sweep.
RVA(0x000c68b0, 0x1f5)
CDroppedObject::CDroppedObject(CGameObject* obj) : CTileLogic(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->ApplyName("LEVEL_OBJECTDROPPER_OBJECT");
    m_savedGeoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("LEVEL_DROPPEDOBJECT", 0);
    m_38->m_flags |= 0x2000002;
    i32 adjY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_landY = adjY;
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = adjY - g_buteMgr.GetIntDef("Hazardz", "DroppedObjectYOffset", 0x140);
    m_fallY = (double)m_object->m_screenY;
    if (m_object->m_latchedAnimId != 0xcf851) {
        m_object->m_latchedAnimId = 0xcf851;
        m_object->m_flags |= 0x20000;
    }
    m_timePerTile =
        32.0 / (double)(u32)g_buteMgr.GetDwordDef("Hazardz", "DroppedObjectTimePerTile", 0x3e8);
}

// CDroppedObject::RegisterRange @0x0c6b50 - seed the dropped-object activation
// table's fast-range bounds via the shared zDArray registry ctor
// (RegisterRange(0x7d0, 0x7da), 0x408710 through the 0x3742 ILT thunk). A static
// initializer; same archetype as CProjectile::RegisterRange (0x0df920).
RVA(0x000c6b50, 0x15)
void CDroppedObject::RegisterRange() {
    ((CZDArrayDerived*)&g_dropColl)->Construct(0x7d0, 0x7da);
}

// CDroppedObject::FireActivation @0x0c6bd0 - look the activation coordinate up
// in the registry; if the entry has a registered handler, look it up again and
// dispatch it __thiscall on this. Same archetype as CTimeBomb::FireActivation
// (0x0e1830).
RVA(0x000c6bd0, 0x102)
void CDroppedObject::FireActivation(i32 coord) {
    CDropEntry* e = DropLookup(coord);
    if (e->m_fn != 0) {
        CDropEntry* e2 = DropLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CDroppedObject::RegisterActs @0x0c6d30 - intern the "A" and "B" activation keys
// and bind each to its per-frame handler (0xc7090 / 0xc7be0) in the dropped-object
// registry. Two back-to-back single-key registrations; the SAME archetype as
// CTimeBomb::RegisterActs done twice.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (both intern/name-resolve blocks + the OWN-registry resolves + the
// `mov [entry],offset handler` stores match retail); residual is the slot-vs-id
// callee-saved register choice cascading into the free-loop counts. Deferred.
RVA(0x000c6d30, 0x2ac)
void CDroppedObject::RegisterActs() {
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
    *(void**)DropLookup(id) = (void*)&DropActA_c7090;

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
    *(void**)DropLookup(id2) = (void*)&DropActB_c7be0;
}

// ---------------------------------------------------------------------------
// The game-registry singleton (0x64556c; the SAME instance every gamemode unit
// binds as g_gameReg / g_mgrSettings). Only the fields the "A" handler touches are
// modeled: the fx-mode selector (m_2c->m_20), the sprite factory (m_30->m_08), the
// tile-event sink (m_68), the collision grid (m_70), and the on-screen bounds
// (m_13c..m_148). Address-pinned so the ds:g_gameReg loads reloc-mask.
// The 0x1c-byte-cell collision grid is g_gameReg->m_tileGrid, already typed
// CTileGrid* on the canonical CGameRegistry (row table m_8, width m_c, height m_10)
// - no local grid view; ActA reads a cell through it directly.
// g_gameReg->m_world->m_8 is the canonical CSpriteFactory (shared
// <Gruntz/SpriteFactory.h>); CreateSprite @0x1597b0 returns the created CGameObject.
struct DropReg2c { // g_gameReg->m_curState
    char m_pad00[0x20];
    i32 m_20; // +0x20  fx-mode selector (the splash switch key)
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// The fall-timer inputs: the frame-delta accumulator (g_645584, u32) scaled by the
// per-tile time m_58, and the 0x5eaa00 double bias subtracted before the >m_68
// landing test. The (i32) truncation lowers to __ftol (0x11f570).
DATA(0x00245584)
extern u32 g_645584;
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// The +0x1a0 animation sub-mgr advanced each draw-delta (Advance 0x15c360, the SAME
// engine sink CTimeBomb's per-frame step drives).

// CDroppedObject::ActA @0x0c7090 - the per-frame "A" activation handler (bound into
// the registry by RegisterActs via the DropActA_c7090 address alias). Advance the
// fall animation, integrate the drop by the frame delta, and once the object has
// fallen past its landing row, look up the grid cell it lands on: over deep water
// (cell & 0x900) spawn a GAME_WATER ripple; over shallow/hazard water (cell & 2, not
// the 0x40 solid) spawn a LEVEL_DEATHSPLASH (gated by the fx-mode selector), then in
// all landed cases apply the LEVEL_DROPPEDOBJECTHIT geometry, intern the "B"
// activation key, and post the tile-hit event to the registry's tile-manager.
//
// @early-stop
// callee-saved-register-assignment coin-flip (~92.5%, docs/patterns/zero-register-pinning.md,
// topic:wall topic:regalloc): the whole body is byte-faithful (verified base vs
// target with llvm-objdump -dr) - the fall integration + __ftol, the >m_68 landing
// inversion, the grid-cell lookup, the (cell&0x900)/(cell&2)/==0x40 split, the
// fx-mode splash jump table, both CreateSprite/ApplyName/ApplyLookupGeometry splash
// paths, and the hit/bute/CombatCue tail all match. The sole residual is which
// callee-saved register holds the long-lived screen-X vs the short-lived grid
// pointer: retail pins X->edi, grid->ebx; cl pins X->ebx, grid->edi, cascading the
// modrm register field through the landing block. Not source-steerable (tried
// reordering the x/grid declarations - identical codegen). Parked for the final
// sweep.
RVA(0x000c7090, 0x21b)
i32 CDroppedObject::ActA() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_6bf3bc);
    m_fallY = (double)g_645584 * m_timePerTile + m_fallY;
    i32 landed = (i32)(m_fallY - g_dropFallBias);
    if (landed > m_landY) {
        i32 x = m_object->m_screenX;
        CTileGrid* g = g_gameReg->m_tileGrid;
        i32 cell;
        {
            i32 cx = x >> 5;
            i32 cy = m_landY >> 5;
            if ((u32)cx < (u32)g->m_c && (u32)cy < (u32)g->m_10) {
                cell = *(i32*)((char*)g->m_8[cy] + cx * 0x1c);
            } else {
                cell = 1;
            }
        }
        if ((cell & 0x900) == 0) {
            if (cell & 2) {
                if (cell == 0x40) {
                    m_38->m_flags |= 0x10000;
                } else {
                    switch (((DropReg2c*)g_gameReg->m_curState)->m_20) {
                        case 4:
                        case 5:
                        case 8:
                            m_38->m_flags |= 0x10000;
                            // fall through
                        case 7:
                        default:
                            if (x < g_gameReg->m_viewOriginR && x >= g_gameReg->m_viewOriginL
                                && m_landY < g_gameReg->m_viewOriginB
                                && m_landY >= g_gameReg->m_viewOriginT) {
                                CGameObject* s = g_gameReg->m_world->m_8->CreateSprite(
                                    0,
                                    x,
                                    m_landY,
                                    0xcf84f,
                                    "Particlez",
                                    0x40003
                                );
                                if (s != 0) {
                                    s->ApplyName("LEVEL_DEATHSPLASH");
                                    s->ApplyLookupGeometry("LEVEL_DEATHSPLASH", 0);
                                }
                            }
                            break;
                        case 6:
                            break;
                    }
                }
            }
        } else {
            if (x < g_gameReg->m_viewOriginR && x >= g_gameReg->m_viewOriginL
                && m_landY < g_gameReg->m_viewOriginB && m_landY >= g_gameReg->m_viewOriginT) {
                CGameObject* s = g_gameReg->m_world->m_8
                                     ->CreateSprite(0, x, m_landY, 0xcf84f, "Particlez", 0x40003);
                if (s != 0) {
                    s->ApplyName("GAME_WATER");
                    s->ApplyLookupGeometry("GAME_WATER", 0);
                }
            }
        }
        m_savedGeoId = m_38->m_geoId;
        m_38->ApplyLookupGeometry("LEVEL_DROPPEDOBJECTHIT", 0);
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find(s_actKeyB);
        ((CGruntTileMgr*)g_gameReg->m_cmdGrid)->CombatCue(m_object->m_screenX, m_landY, 1, 7, -1);
        return 0;
    }
    m_object->m_screenY = landed;
    return 0;
}

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
SIZE_UNKNOWN(CDropEntry);
SIZE_UNKNOWN(CDroppedObject);
SIZE_UNKNOWN(DropAnimSink);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(DropReg2c);
