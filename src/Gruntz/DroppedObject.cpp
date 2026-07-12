// DroppedObject.cpp - the dropped-object tile-logic TU (C:\Proj\Gruntz):
// CObjectDropper + CDroppedObject + CDroppedObjectShadow, their logic-worker
// pumps, activation registries and serialize round-trips - the ONE original obj
// at retail .text [0xc5360 .. 0xc7e90] (9 leading $E static-init frags + code +
// the trailing 9-frag run), plus the three low-band /GX leaf dtors.
//
// wave2-H merge, PROVEN one TU against the roster hypothesis by the
// private-globals oracle: the .data band 0x64be10..0x64c268 is one interleaved
// contribution - the 9 leading frag statics (0x64be20..88 + 0x64bec8..d0) WEAVE
// with this TU's registry singletons (g_dropperActReg 0x64be90, g_dropColl
// 0x64bed8, g_shadowActReg 0x64bf00), while MultiStartDlgRoster's private
// extent ends cleanly at 0x64bdcc. g_dropperActReg (ex "g_netBe90") is
// referenced ONLY by InitActReg (0xc5f00, ex roster's "NetConfigureBe90") +
// FireAct/RegisterActs here - zero roster references. Each class carries the
// identical {E-frag, Construct(0x15), atexit-thunk(0xe)} static-registry triple
// (0xc5ee0/0xc5f00/0xc5f30 dropper; 0xc6b30/0xc6b50/0xc6b80 dropped;
// 0xc76b0/0xc76d0/0xc7700 shadow). Merges the former droppedobject +
// objectdropper + droppedobjectshadow + droppedobjectserialize +
// objectlogicpump + actregsiblings(in-band fns) units + WapMisc's 0xc76d0 +
// the roster's 0xc5f00.
//
// IDENTITY RECOVERED: ActRegSiblings' "CSiblingActorA" facet IS CObjectDropper
// (its registry entry fires &CObjectDropper::Update) and "CSiblingActorB" IS
// CDroppedObjectShadow (its registry construct 0xc76d0 abuts the shadow ctor;
// its per-frame Advance spawns the "DroppedObject" sprite on the drop frame).
//
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/ObjectDropper.h>   // CObjectDropper : CUserLogic (ctor 0xc59f0)
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/DroppedObject.h>   // CDroppedObject : CUserLogic (ctor 0xc68b0)
#include <Gruntz/DroppedObjectShadow.h> // CDroppedObjectShadow : CUserLogic (ctor 0xc7490)
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TypeKeyColl.h>
#include <Bute/ButeTree.h>
#include <Gruntz/ActReg.h> // the shared activation-registrar archetype (CSiblingActReg)
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgr.h>    // canonical CTriggerMgr (m_cmdGrid; FindGruntAt @0x75c60)
#include <Gruntz/LightFxMgr.h>    // CLightFxMgr (g_gameReg->m_logicPump @+0x78; m_tables[])
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialObjRef.h>  // CSerialObjRef::Chain (0x8c00)
#include <Globals.h>

#include <string.h> // inline strcmp for the direction-name match

// ---------------------------------------------------------------------------
// Shared engine singletons/externs (reloc-masked).
// ---------------------------------------------------------------------------
// The global bute store (?g_buteTree@@3VCButeTree@@A @0x6bf620; Find 0x16d190 /
// Insert 0x16db90) + the bute manager (g_buteMgr); named symbols so the calls
// reloc-mask.
extern CButeTree g_buteTree;
extern CButeMgr g_buteMgr;

// The game-registry singleton (0x64556c; the SAME instance every gamemode unit
// binds as g_gameReg / g_gameReg). The dropper family reaches its facets
// through the reused per-mode slots (authentic downcasts, see CGameRegistry.h).
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// The per-frame game clock (g_645588) + frame delta (g_645584) + draw-clock
// delta (g_6bf3bc). C linkage so the symbols pair with the targets' _g_* names
// (the convention across the gamemode units).
extern "C" u32 g_645588; // 0x645588
extern "C" {
    DATA(0x00245584)
    extern u32 g_645584; // 0x645584
    DATA(0x002bf3bc)
    extern u32 g_6bf3bc; // 0x6bf3bc
}

extern void* GetRetAddr(); // 0x16d990
DATA(0x002bf464)
extern void* g_projActCache; // 0x2bf464 canonical (bound in GruntStartingPoint.cpp); the old
                             // g_actCache spelling was an unbound VA-typo alias of this global
extern void* g_retAddrBreadcrumb;

// The drop-motion .rdata FP constants (owner-TU defs; VA 0x5ea9f0/0x5eaa00).
DATA(0x001ea9f0)
const double g_objDropDiv = 32.0; // 0x5ea9f0  m_speed = g_objDropDiv / time
DATA(0x001eaa00)
double g_dropFallBias = -0.5; // 0x5eaa00  landed = m_fallY - g_dropFallBias

// ---------------------------------------------------------------------------
// The three per-class activation-coordinate registries (one per leaf class; all
// the shared <Gruntz/ActReg.h> CActReg archetype, each built over the fixed
// [2000, 2010] act-id range by its class's InitActReg static-init triple):
//   g_dropperActReg @0x64be90 (ex "g_netBe90" - a net-parking misnomer)
//   g_dropColl      @0x64bed8 (CDroppedObject's, reached via the DropLookup inline)
//   g_shadowActReg  @0x64bf00 (ex "g_64bf00")
// ---------------------------------------------------------------------------
DATA(0x0024be90)
CSiblingActReg g_dropperActReg; // 0x64be90 (owner TU: real definition; interior
                                // fields 0x24be94..0x24beb0 are this object's members)
DATA(0x0024bed8)
extern CTypeKeyColl g_dropColl; // 0x64bed8
DATA(0x0024bf00)
CSiblingActReg g_shadowActReg; // 0x64bf00 (owner TU: real definition; interior
                               // fields 0x24bf04..0x24bf20 are this object's members)

// The registered-handler entries (first dword = the handler PMF; single
// inheritance -> 4-byte code pointers; the classes are COMPLETE above so each
// PMF stays 4 bytes - pmf-complete-class-4byte).
struct CDropperActEntry {
    i32 (CObjectDropper::*m_fn)();
};
struct CShadowActEntry {
    i32 (CDroppedObjectShadow::*m_fn)();
};

// ---------------------------------------------------------------------------
// The shared activation-NAME registry (@0x6bf650, the SAME shared instance
// CTimeBomb/CKitchenSlime use) + the running id counter and the two key strings.
// ---------------------------------------------------------------------------
DATA(0x0021aea8)
extern i32 g_typeCounter;
DATA(0x0020a454)
extern char s_codeA[]; // "A"
DATA(0x0020d1bc)
extern char s_actKeyB[]; // "B"
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
extern CTypeNameEntry* g_typeCur; // slow-path result slot
DATA(0x002bf66c)
extern void* g_typeNodes; // the slot's CString list base
DATA(0x002bf670)
extern i32 g_typeCount; // zeroed first; doubles as the list count

// The CString in the resolved name slot: ~CString (0x1b9b93) frees the old list,
// operator= (0x1b9e74) assigns the new key. Modeled so the calls reloc-mask.
#include <Gruntz/ActName.h> // CActName (shared)

// The id->name-slot resolve (fast range path + slow Find/GetRetAddr/Insert rebuild).
static inline char* ActNameLookup(i32 id) {
    g_typeCount = 0;
    if (id >= g_typeLo && id <= g_typeHi) {
        return g_typeBase + (id - g_typeLo) * g_typeStride;
    }
    if ((i32)((_zvec*)&g_typeColl)->GrowTo(id, 0)) {
        return g_typeBase + (id - g_typeLo) * g_typeStride;
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl2->Set(&g_typeColl, (i32)item, 0xc);
    return (char*)g_typeCur;
}

// The two per-frame handlers bound into CDroppedObject's registry slots
// (referenced by address so the DIR32 store operands reloc-mask). 0xc7090 binds
// to "A" (== CDroppedObject::ActA below), 0xc7be0 to "B" (unreconstructed).
extern i32 DropActA_c7090();
extern i32 DropActB_c7be0();

// CDroppedObject's entry type + the inlined coordinate->Entry* lookup its
// FireActivation folds in twice.
typedef void (CDroppedObject::*DropHandler)();
struct CDropEntry {
    DropHandler m_fn; // [entry]
};
// g_drop* registry-field globals (referenced only from this TU): real
// definitions DATA-pinned here; the single extern is in <Globals.h>.
DATA(0x0024bedc)
CVariantSlot* g_dropColl2;
DATA(0x0024bee0)
i32 g_dropLo;
DATA(0x0024bee4)
i32 g_dropHi;
DATA(0x0024bee8)
char* g_dropBase;
DATA(0x0024beec)
CDropEntry* g_dropCur;
DATA(0x0024bef0)
i32 g_dropStride;
DATA(0x0024bef8)
i32 g_dropScratch;

static inline CDropEntry* DropLookup(i32 coord) {
    g_dropScratch = 0;
    if (coord >= g_dropLo && coord <= g_dropHi) {
        return (CDropEntry*)(g_dropBase + (coord - g_dropLo) * g_dropStride);
    }
    if ((i32)((_zvec*)&g_dropColl)->GrowTo(coord, 0)) {
        return (CDropEntry*)(g_dropBase + (coord - g_dropLo) * g_dropStride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_dropColl2->Set(&g_dropColl, (i32)item, 0xc);
    return g_dropCur;
}

// The default case's shared type-keyed record serializer (0x16e4f0, owned +
// matched in TypeKeyColl.cpp = ?ProjTypeXfer@@YAHPAUCXferArchive@@@Z); the active
// logic leaf is the record arg, reinterpreted as the archive record it drives.
#include <Gruntz/XferArchive.h>

// ---------------------------------------------------------------------------
// ObjectDropper.cpp's local views of the bound object + registry facets (only
// the touched offsets are modeled; see each note).
// ---------------------------------------------------------------------------

// The bound object's +0x198 geometry/footprint descriptor: the per-frame Update
// polls its half-extents (+0x18, +0x1c) to build the wander box.
SIZE_UNKNOWN(DropperLayer);
struct DropperLayer {
    char m_pad00[0x18];
    i32 m_halfWidth;  // +0x18 half-width  (tiles)
    i32 m_halfHeight; // +0x1c half-height (tiles)
};

// The bound CGameObject viewed by the ctor (m_10 == m_38). A divergent local
// model of CGameObject (it has a name record at +0x194 the base header treats as
// padding), so it can't fold onto <Gruntz/UserLogic.h>'s CGameObject without
// editing that shared base - kept as a local view. Only touched offsets modeled.
SIZE_UNKNOWN(CObjDropObj);
struct CObjDropObj {
    char m_pad00[0x08];
    i32 m_flags; // +0x08 flags
    char m_pad0c[0x4c - 0x0c];
    i32 m_spriteRef; // +0x4c sprite-ref handle
    i32 m_state;     // +0x50 state
    char m_pad54[0x58 - 0x54];
    i32 m_active;  // +0x58 active flag
    i32 m_screenX; // +0x5c screen X
    i32 m_screenY; // +0x60 screen Y
    char m_pad64[0x74 - 0x64];
    i32 m_layerKey; // +0x74 layer key
    char m_pad78[0x12c - 0x78];
    i32 m_travelDir; // +0x12c travel direction (1..4)
    char m_pad130[0x144 - 0x130];
    i32 m_144; // +0x144 rect base (probe RECT; L/T/R/B ordering unproven)
    i32 m_148; // +0x148
    i32 m_14c; // +0x14c
    i32 m_150; // +0x150
    char m_pad154[0x194 - 0x154];
    char*
        m_nameRec; // +0x194 the sprite/name record (dir name at +0x24); char* like CGameObject::m_194
    DropperLayer* m_footprint; // +0x198 footprint descriptor (wander-box half-extents)
    char m_pad19c[0x1b4 - 0x19c];
    i32 m_cycleGeomId; // +0x1b4 cycle-geometry id
};

// The dropper's facet of the registry resource holder (g_gameReg->m_world, an
// authentic per-mode downcast of the reused +0x30 slot; see CGameRegistry.h):
// the HUD sprite factory (m_spriteFactory->CreateSprite @0x1597b0, __thiscall) +
// the level/world tile bounds (m_level->m_world width @0x30 / height @0x34).
SIZE_UNKNOWN(DropperWorld);
struct DropperWorld {
    char m_pad00[0x30];
    i32 m_widthTiles;  // +0x30 world width  (tiles)
    i32 m_heightTiles; // +0x34 world height (tiles)
};
SIZE_UNKNOWN(DropperLevel);
struct DropperLevel {
    char m_pad00[0x5c];
    DropperWorld* m_world; // +0x5c
};
SIZE_UNKNOWN(DropperMgr);
struct DropperMgr { // (DropperMgr*)g_gameReg->m_world
    char m_pad00[0x08];
    CSpriteFactory* m_spriteFactory; // +0x8  HUD sprite factory (canonical, @0x1597b0)
    char m_pad0c[0x24 - 0xc];
    DropperLevel* m_level; // +0x24 level bounds
};
// The wander RECT the destination probe searches {left, top, right, bottom}.
SIZE_UNKNOWN(DropperBox);
struct DropperBox {
    i32 left;
    i32 top;
    i32 right;
    i32 bottom;
};
// The probe result (a tile-logic object): +0x10 -> its bound render object.
SIZE_UNKNOWN(DropperFound);
struct DropperFound {
    char m_pad00[0x10];
    CObjDropObj* m_obj; // +0x10
};
// The world tile map is g_gameReg->m_cmdGrid (the +0x68 CTriggerMgr slot): the drop
// destination probe IS CTriggerMgr::FindGruntAt @0x75c60 (__thiscall, 0x32ce ILT
// thunk) - it picks the reachable destination tile in the wander box and returns the
// CTmCell it lands on. Called cast-free on the real CTriggerMgr (<Gruntz/TriggerMgr.h>).
// One terrain-plane cell of the registry's tile grid (g_gameReg->m_tileGrid, the
// canonical CTileGrid): a 0x1c-byte (7-dword) record; dword 0 holds the flags.
// Reached as ((DropperTile*)grid->m_8[row])[col] - the authentic CTileGrid cell
// idiom (grid->m_8 is the row-pointer table, cells 0x1c B apart; see CTileGrid.h).
SIZE_UNKNOWN(DropperTile);
struct DropperTile {
    u32 m_flags; // +0x0 terrain flags (bit 1 = blocked)
    char m_pad04[0x1c - 0x4];
};

// The bound object's +0x1a0 per-frame animator (Advance_15c360 @0x55c360).
SIZE_UNKNOWN(DropperAnim);
struct DropperAnim {
    // Advance @0x15c360 IS CAniAdvanceCursor::Advance_15c360; cast at the call.
};

// The +0x20 fx-mode selector facet of g_gameReg->m_curState (the "A" handler's
// splash switch key).
struct DropReg2c { // g_gameReg->m_curState
    char m_pad00[0x20];
    i32 m_20; // +0x20  fx-mode selector (the splash switch key)
};

// ===========================================================================
// The three low-band /GX leaf dtors (the 0x124f0..0x126b4 ctor-band pocket).
// Each folds the bare CUserLogic teardown: store the CUserLogic vptr (0x5e705c),
// inline-destruct the +0x18 link (the embedded ~EngStr call 0x16d2a0), store the
// CUserBase vptr (0x5e70b4). The destructible link forces the /GX EH frame; the
// leaf vptr store is dead-eliminated. Byte-identical in shape to ~CTimeBomb
// @0x012a70; the empty bodies are enough for cl.
// ===========================================================================
RVA(0x000124f0, 0x44)
CObjectDropper::~CObjectDropper() {}

RVA(0x000125b0, 0x44)
CDroppedObject::~CDroppedObject() {}

RVA(0x00012670, 0x44)
CDroppedObjectShadow::~CDroppedObjectShadow() {}

// ===========================================================================
// The three logic-worker pumps (0xc5630/0xc5770/0xc58b0, ex ObjectLogicPump.cpp)
// - the per-frame state/message pumps for the three leaves. Each is the SAME
// archetype as StateDispatch.cpp's CLevelTime pump (0x9b770): a
// __cdecl(CGameObject*) that reads the object's +0x7c aux state id (+0x1c) and,
// on id 0, operator-new's + constructs its leaf (id then latched to 0x3e8),
// Activates it (vtable slot 6) and installs it into aux->m_logic (+0x18); routes
// ids 0x1d/0x1e/0x50..0x53 to the active handler's matching CUserLogic virtual
// slot; and (default) hands the handler to the shared type-keyed serializer.
// Always returns 1.
// ===========================================================================

// EXACT since the merge: the old "throwing-operator-new /GX frame wall" (~32%
// under the base-flags ObjectLogicPump.cpp parking) dissolved - this TU's true
// /GX profile raises the operator-delete-on-ctor-throw frame retail has - and
// the UNSIGNED switch key emits the retail ja/jbe range checks
// (docs/patterns/switch-key-unsigned-ja-vs-jg.md).
RVA(0x000c5630, 0xf4)
i32 ObjectDropperPump(CGameObject* obj) {
    CGameObjAux* aux = obj->m_7c;
    switch ((u32)aux->m_1c) {
        case 0: {
            aux->m_1c = (void*)0x3e8;
            CObjectDropper* h = new CObjectDropper(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case 0x1d:
            aux->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            aux->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            aux->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            aux->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            aux->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            aux->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer((CXferArchive*)aux->m_logic);
            break;
    }
    return 1;
}

RVA(0x000c5770, 0xf1)
i32 DroppedObjectPump(CGameObject* obj) {
    CGameObjAux* aux = obj->m_7c;
    switch ((u32)aux->m_1c) {
        case 0: {
            aux->m_1c = (void*)0x3e8;
            CDroppedObject* h = new CDroppedObject(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case 0x1d:
            aux->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            aux->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            aux->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            aux->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            aux->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            aux->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer((CXferArchive*)aux->m_logic);
            break;
    }
    return 1;
}

RVA(0x000c58b0, 0xf1)
i32 DroppedObjectShadowPump(CGameObject* obj) {
    CGameObjAux* aux = obj->m_7c;
    switch ((u32)aux->m_1c) {
        case 0: {
            aux->m_1c = (void*)0x3e8;
            CDroppedObjectShadow* h = new CDroppedObjectShadow(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case 0x1d:
            aux->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            aux->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            aux->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            aux->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            aux->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            aux->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer((CXferArchive*)aux->m_logic);
            break;
    }
    return 1;
}

// ===========================================================================
// CObjectDropper::CObjectDropper @0xc59f0 - fold the shared CUserLogic(obj) init,
// bind the cycle geometry + "A" bute node, snap the bound object to the tile grid,
// match the dropper's direction name (LEVEL_OBJECTDROPPER_{NORTH,EAST,SOUTH,WEST})
// to seed the travel vector + direction id, then read the per-tile time
// (ObjectDropperTimePerTile) into the per-frame speed and seed the bound sprite's
// draw state.
//
// @early-stop
// inline-strcmp + register-pinning + eh wall (docs/patterns/strcmp-eq-bool-local-setcc.md,
// zero-register-pinning.md, eh-ctor-vptr-restamp-position.md): body byte-faithful
// (the four unrolled inline-strcmp loops, the CString temp lifecycle + EH states,
// the snap/bute/time-divide all match retail). Residual is the strcmp result-reg
// alloc (ebx pinned to 1 across the first loop), the /GX leaf-vptr re-stamp position
// + the shared dy=0 store fold. Not source-steerable (global regalloc/EH numbering).
RVA(0x000c59f0, 0x3e3)
CObjectDropper::CObjectDropper(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_lastDropTime = 0;
    m_dropInterval = 0;
    m_geomId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("LEVEL_OBJECTDROPPER", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 0x2000002;

    CObjDropObj* o = (CObjDropObj*)m_object;
    i32 snapX = (o->m_screenX & ~0x1f) + 0x10;
    i32 snapY = (o->m_screenY & ~0x1f) + 0x10;
    o->m_screenX = snapX;
    m_posX = (double)snapX;
    o->m_screenY = snapY;
    m_posY = (double)snapY;
    if (o->m_layerKey != 0xcf851) {
        o->m_layerKey = 0xcf851;
        o->m_flags |= 0x20000;
    }

    CObjDropObj* obj38 = (CObjDropObj*)m_38;
    if (obj38->m_nameRec != 0) {
        CString name;
        name = obj38->m_nameRec + 0x24;
        const char* s = name;
        if (strcmp(s, "LEVEL_OBJECTDROPPER_NORTH") == 0) {
            o->m_travelDir = 1;
            m_travelDx = 0;
            m_travelDy = -1;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_EAST") == 0) {
            o->m_travelDir = 2;
            m_travelDx = 1;
            m_travelDy = 0;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_SOUTH") == 0) {
            o->m_travelDir = 3;
            m_travelDx = 0;
            m_travelDy = 1;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_WEST") == 0) {
            o->m_travelDir = 4;
            m_travelDx = -1;
            m_travelDy = 0;
        }
    }

    i32 time = g_buteMgr.GetDwordDef("Hazardz", "ObjectDropperTimePerTile", 1000);
    m_scrollMode = 0;
    m_lastDropTileX = -1;
    m_lastDropTileY = -1;
    m_speed = g_objDropDiv / (double)(i64)(u32)time;
    if (g_gameReg->m_134 == 1) {
        m_scrollMode = 1;
    }
    i32 sel = (i32)g_gameReg->m_logicPump->m_tables[5];
    o->m_active = 1;
    o->m_state = 7;
    o->m_spriteRef = sel;
    m_lastDropTime = 0;
    m_dropInterval = 0;
    o->m_144 = 1;
    o->m_14c = 1;
    o->m_148 = 1;
    o->m_150 = 1;
}

// CObjectDropper::InitActReg @0xc5f00 (ex the roster-parked "NetConfigureBe90") -
// construct the class's activation-coordinate registry singleton over [2000,
// 2010]; the Construct body of the class's static-registry triple (its $E frag
// @0xc5ee0 calls this, its atexit dtor thunk is 0xc5f30/0xc5f50). Free init
// thunk; reloc-masked.
RVA(0x000c5f00, 0x15)
void CObjectDropper::InitActReg() {
    ((CZDArrayDerived*)&g_dropperActReg)->Construct(0x7d0, 0x7da);
}

// CObjectDropper::FireAct (0xc5f80): the runtime side of the registry - resolve the
// handler for act `actId` and, if bound, fire it on `this`. ResolveEntry has side
// effects (m_scratch reset + GrowTo-on-miss), so it is re-run for the actual call
// rather than cached; the null-slot path just returns (eax = the entry ptr).
RVA(0x000c5f80, 0x102)
void CObjectDropper::FireAct(i32 actId) {
    if (((CDropperActEntry*)g_dropperActReg.ResolveEntry(actId))->m_fn != 0) {
        (this->*(((CDropperActEntry*)g_dropperActReg.ResolveEntry(actId))->m_fn))();
    }
}

// CObjectDropper::RegisterActs (0xc60e0): register "A" in the shared name registry
// (first caller only), then bind Update into the class registry slot.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): every
// call/immediate/branch/offset + the `mov [entry],offset` handler store is
// byte-faithful; the residual is the slot-vs-id callee-saved register choice that
// cascades into the free-loop trip-count materialization (`ecx=cnt; eax=cnt-1; lea
// ebp,[eax+1]`). Identical to CAniCycle::RegisterActs. Deferred to the final sweep.
RVA(0x000c60e0, 0x18d)
void CObjectDropper::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        g_buteTree.Insert(s_codeA, (void*)g_typeCounter);
        id = g_typeCounter;
        char* slot = ActNameLookup(id);
        i32 cnt = g_typeCount;
        void** list = (void**)g_typeNodes;
        if (cnt != 0) {
            do {
                if (list != 0) {
                    ((CString*)list)->CString::~CString();
                }
                list++;
            } while (--cnt);
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CDropperActEntry*)g_dropperActReg.ResolveEntry(id))->m_fn = &CObjectDropper::Update;
}

// CObjectDropper::Update @0xc62e0 - the per-frame tick (re-homed from the
// CDroppedObjectShadow::LoadAttributes trace mis-attribution). When the 64-bit
// re-arm timer expires (and the game isn't paused in edit mode), probe a random
// reachable destination tile inside the wander box, and if it lands on a new,
// unblocked tile spawn a "DroppedObjectShadow" there and re-arm the timer from
// the "Hazardz/ObjectDropperDelay" bute. Every frame: advance the bound sprite's
// animator, x87-drift the double position by g_645584 * m_speed along the travel
// vector (wrapping at the world tile bounds), and write the rounded coords back
// to the bound object's screen position.
RVA(0x000c62e0, 0x2dd)
i32 CObjectDropper::Update() {
    if ((i64)g_645588 - m_lastDropTime >= m_dropInterval) {
        if (g_gameReg->m_isEasyMode == 0 || g_gameReg->m_134 != 1) {
            CObjDropObj* o = (CObjDropObj*)m_object;
            DropperBox box;
            box.left = o->m_screenX - o->m_footprint->m_halfWidth + 7;
            box.right = o->m_screenX + o->m_footprint->m_halfWidth - 7;
            box.top = o->m_screenY - o->m_footprint->m_halfHeight + 7;
            box.bottom = o->m_screenY + o->m_footprint->m_halfHeight - 7;
            i32 tx;
            i32 ty;
            DropperFound* found = (DropperFound*)g_gameReg->m_cmdGrid->FindGruntAt(
                o->m_screenX,
                o->m_screenY,
                (RECT*)&o->m_144,
                &tx,
                &ty,
                (RECT*)&box
            );
            if (found != 0) {
                if (m_lastDropTileX != tx || m_lastDropTileY != ty) {
                    if (m_scrollMode == 0 || tx == 0) {
                        CObjDropObj* fo = found->m_obj;
                        i32 fx = fo->m_screenX;
                        i32 fy = fo->m_screenY;
                        CTileGrid* plane = g_gameReg->m_tileGrid;
                        i32 cx = fx >> 5;
                        i32 cy = fy >> 5;
                        u32 flags;
                        if ((u32)cx >= (u32)plane->m_c || (u32)cy >= (u32)plane->m_10) {
                            flags = 1;
                        } else {
                            flags = ((DropperTile*)plane->m_8[cy])[cx].m_flags;
                        }
                        if ((flags & 2) == 0) {
                            ((DropperMgr*)g_gameReg->m_world)
                                ->m_spriteFactory
                                ->CreateSprite(0, fx, fy, 0, "DroppedObjectShadow", 0x40003);
                            m_lastDropTileX = tx;
                            m_lastDropTileY = ty;
                            m_dropInterval =
                                g_buteMgr.GetDwordDef("Hazardz", "ObjectDropperDelay", 1000);
                            m_lastDropTime = g_645588;
                        }
                    }
                }
            }
        }
    }

    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360((i32)g_6bf3bc);

    double drift = (double)g_645584 * m_speed;
    if (m_travelDx > 0) {
        m_posX += drift;
        if (m_posX >= (double)((DropperMgr*)g_gameReg->m_world)->m_level->m_world->m_widthTiles) {
            m_posX = 0.0;
            m_lastDropTileX = -1;
            m_lastDropTileY = -1;
        }
    } else if (m_travelDx < 0) {
        m_posX -= drift;
        if (m_posX < 0.0) {
            m_posX =
                (double)(((DropperMgr*)g_gameReg->m_world)->m_level->m_world->m_widthTiles - 1);
            m_lastDropTileX = -1;
            m_lastDropTileY = -1;
        }
    }
    if (m_travelDy > 0) {
        m_posY += drift;
        if (m_posY > (double)((DropperMgr*)g_gameReg->m_world)->m_level->m_world->m_heightTiles) {
            m_posY = 0.0;
            m_lastDropTileX = -1;
            m_lastDropTileY = -1;
        }
    } else if (m_travelDy < 0) {
        m_posY -= drift;
        if (m_posY < 0.0) {
            m_posY =
                (double)(((DropperMgr*)g_gameReg->m_world)->m_level->m_world->m_heightTiles - 1);
            m_lastDropTileX = -1;
            m_lastDropTileY = -1;
        }
    }

    m_object->m_screenX = (i32)m_posX;
    m_object->m_screenY = (i32)m_posY;
    return 0;
}

// CObjectDropper::Serialize (0xc6680): the base/chain gate, then the +0x88/+0x90 drop-
// timing i64 pair, then the +0x58..+0x80 move/state fields; mode 8 instead seeds a
// draw-fill command on the bound object from the light-FX table set.
RVA(0x000c6680, 0x1b4)
i32 CObjectDropper::Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)((i32)ar), tag, c, d)) {
        return 0;
    }
    if (!SerialRef34()->Chain(ar, tag, c, (CSerialObj*)d)) {
        return 0;
    }

    // The drop-timing i64 pair (+0x88/+0x90), streamed through a walking pointer.
    char* p = (char*)this + 0x88;
    switch (tag) {
        case 4:
            ar->Write(p, 8);
            p += 8;
            ar->Write(p, 8);
            break;
        case 7:
            ar->Read(p, 8);
            p += 8;
            ar->Read(p, 8);
            break;
    }

    // The move/state field list (+0x58..+0x80); mode 8 seeds a draw-fill instead.
    switch (tag) {
        case 4:
            ar->Write(&m_speed, 8);
            ar->Write(&m_posX, 8);
            ar->Write(&m_posY, 8);
            ar->Write(&m_travelDx, 4);
            ar->Write(&m_travelDy, 4);
            ar->Write(&m_lastDropTileX, 4);
            ar->Write(&m_lastDropTileY, 4);
            ar->Write(&m_scrollMode, 4);
            break;
        case 7:
            ar->Read(&m_speed, 8);
            ar->Read(&m_posX, 8);
            ar->Read(&m_posY, 8);
            ar->Read(&m_travelDx, 4);
            ar->Read(&m_travelDy, 4);
            ar->Read(&m_lastDropTileX, 4);
            ar->Read(&m_lastDropTileY, 4);
            ar->Read(&m_scrollMode, 4);
            break;
        case 8: {
            i32 fill = (i32)g_gameReg->m_logicPump->m_tables[5];
            CGameObject* o = m_object;
            o->m_drawActive = 1;
            o->m_drawFillArg = fill;
            o->m_drawFillCmd = 7;
            break;
        }
    }
    return 1;
}

// ===========================================================================
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
CDroppedObject::CDroppedObject(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
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
// (RegisterRange(0x7d0, 0x7da), 0x408710 through the 0x3742 ILT thunk). The
// Construct body of this class's static-registry triple ($E frag @0xc6b30,
// atexit thunk 0xc6b80); same archetype as CProjectile::RegisterRange (0x0df920).
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
    *(void**)DropLookup(id) = (void*)&DropActA_c7090;

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
    *(void**)DropLookup(id2) = (void*)&DropActB_c7be0;
}

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

// CDroppedObject::UserLogicVfunc5 (0xc7350), vtable slot 7 - the per-frame draw-
// cursor step: advance the bound object's +0x1a0 anim cursor by the frame draw-
// delta, then latch the object's dirty bit (0x10000) when its anim sub-mgr is
// active (m_1c8) but idle (m_1c0 == 0).
RVA(0x000c7350, 0x39)
i32 CDroppedObject::UserLogicVfunc5() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_6bf3bc);
    if (m_38->m_1c8 != 0 && m_38->m_1c0 == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

// CDroppedObject::Serialize (0xc73a0): base/chain gate, then the +0x58/+0x60 doubles
// and the +0x68 landing row.
RVA(0x000c73a0, 0xb5)
i32 CDroppedObject::Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)((i32)ar), tag, c, d)) {
        return 0;
    }
    if (!SerialRef34()->Chain(ar, tag, c, (CSerialObj*)d)) {
        return 0;
    }
    switch (tag) {
        case 4:
            ar->Write(&m_timePerTile, 8);
            ar->Write(&m_fallY, 8);
            ar->Write(&m_landY, 4);
            break;
        case 7:
            ar->Read(&m_timePerTile, 8);
            ar->Read(&m_fallY, 8);
            ar->Read(&m_landY, 4);
            break;
    }
    return 1;
}

// ===========================================================================
// CDroppedObjectShadow::CDroppedObjectShadow(CGameObject*) @0xc7490 - the 1-arg
// leaf ctor: the standard CUserLogic(obj) init (folded inline) plus the shadow
// tail - cache the anim-set node off the "A" bute key, snapshot m_38->m_1b4,
// apply the shadow sprite/geometry to the bound object, raise its logic/collision
// flag bits, and seed the bound object's render state (m_4c from the game
// registry, m_50=7/m_58=1, the 0xcf84f tile-key + its dirty bit). Constructs a
// throwing CUserBaseLink, so MSVC emits the /GX EH frame.
//
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-identical to retail (the CUserLogic init, the "A" anim-set cache, the
// ApplyName/ApplyLookupGeometry pair, the m_38->m_08 RMW, the m_10 render-state
// seed); the residue is this ctor's own __ehfuncinfo state numbering + the
// 1-slot callee-saved scheduling delta MSVC coin-flips. The SAME plateau as
// CBrickz::CBrickz (~92%); not source-steerable. Parked for the final sweep.
RVA(0x000c7490, 0x1a6)
CDroppedObjectShadow::CDroppedObjectShadow(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->ApplyName("LEVEL_OBJECTDROPPER_SHADOW");
    m_savedGeoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("LEVEL_DROPPEDOBJECTSHADOW", 0);
    m_38->m_flags |= 0x2000002;
    m_object->m_drawFillArg = (i32)g_gameReg->m_logicPump->m_tables[5];
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 7;
    if (m_object->m_latchedAnimId != 0xcf84f) {
        m_object->m_latchedAnimId = 0xcf84f;
        m_object->m_flags |= 0x20000;
    }
}

// CDroppedObjectShadow::InitActReg @0xc76d0 (ex WapMisc's "Unmatched_c76d0") -
// construct the class's activation-coordinate registry singleton over [2000,
// 2010]; the Construct body of the class's static-registry triple ($E frag
// @0xc76b0, atexit thunk 0xc7700). Free init thunk; reloc-masked.
RVA(0x000c76d0, 0x15)
void CDroppedObjectShadow::InitActReg() {
    ((CZDArrayDerived*)&g_shadowActReg)->Construct(0x7d0, 0x7da);
}

// CDroppedObjectShadow::FireActivation (0xc7750): runtime dispatch for the
// class registry @0x64bf00 - same double-ResolveEntry + PMF-fire archetype as
// CObjectDropper::FireAct.
RVA(0x000c7750, 0x102)
void CDroppedObjectShadow::FireActivation(i32 coord) {
    if (((CShadowActEntry*)g_shadowActReg.ResolveEntry(coord))->m_fn != 0) {
        (this->*(((CShadowActEntry*)g_shadowActReg.ResolveEntry(coord))->m_fn))();
    }
}

// CDroppedObjectShadow::RegisterActs (0xc78b0): same archetype as
// CObjectDropper::RegisterActs, binding Advance into the class registry.
//
// @early-stop
// same register-pinning wall as CObjectDropper::RegisterActs above (logic + every
// byte byte-faithful; only the regalloc/free-loop-count materialization diverges).
RVA(0x000c78b0, 0x18d)
void CDroppedObjectShadow::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        g_buteTree.Insert(s_codeA, (void*)g_typeCounter);
        id = g_typeCounter;
        char* slot = ActNameLookup(id);
        i32 cnt = g_typeCount;
        void** list = (void**)g_typeNodes;
        if (cnt != 0) {
            do {
                if (list != 0) {
                    ((CString*)list)->CString::~CString();
                }
                list++;
            } while (--cnt);
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CShadowActEntry*)g_shadowActReg.ResolveEntry(id))->m_fn = &CDroppedObjectShadow::Advance;
}

// CDroppedObjectShadow::Advance (0xc7ab0): the per-frame act handler - advance
// the bound object's +0x1a0 anim sub-mgr, and on the drop frame (Advance == 2)
// spawn the "DroppedObject" sprite at the bound object's screen position (the
// shadow heralds the drop); then raise the object's redraw bit if the anim
// latched active while idle-clear (same idle tail as CDroppedObject::UserLogicVfunc5).
// @early-stop
// tail regalloc coin-flip (98.48%): the whole body is byte-faithful (the Advance
// call, the ==2 gate, the CreateSprite spawn, the idle check). The sole residue is
// the m_38 reload in the idle tail: with the CreateSprite branch in front, `this`
// is dead there and retail REUSES esi (`mov esi,[esi+0x38]`), while cl keeps `this`
// in esi and loads m_38 into eax. The identical tail matched EXACT in
// CDroppedObject::UserLogicVfunc5 (no preceding branch); not source-steerable here
// (local/inline m_object + permute all identical). Parked for the final sweep.
RVA(0x000c7ab0, 0x67)
i32 CDroppedObjectShadow::Advance() {
    if (((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_6bf3bc) == 2) {
        CGameObject* o = m_object;
        g_gameReg->m_world->m_8
            ->CreateSprite(0, o->m_screenX, o->m_screenY, 0, "DroppedObject", 0x40003);
    }
    if (m_38->m_1c8 != 0 && m_38->m_1c0 == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

// CDroppedObjectShadow::SerializeMove (0xc7b40), vtable slot 1 - chain the shared
// serialize helper + the +0x34 CSerialObjRef gate (both early-return 0 on failure);
// adds no streamed leaf fields. Mode 8 instead re-seeds the bound object's draw-fill
// render state (same shade-table source as the ctor). The CObjectDropper::Serialize
// mode-8 archetype.
RVA(0x000c7b40, 0x76)
i32 CDroppedObjectShadow::SerializeMove(CGruntArchive* ar, i32 mode, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)((i32)ar), mode, c, d)) {
        return 0;
    }
    if (!SerialRef34()->Chain((CSerialArchive*)ar, mode, c, (CSerialObj*)d)) {
        return 0;
    }
    if (mode == 8) {
        i32 fill = (i32)g_gameReg->m_logicPump->m_tables[5];
        CGameObject* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 7;
        o->m_drawFillArg = fill;
    }
    return 1;
}

#include <rva.h>
SIZE_UNKNOWN(CDropEntry);
SIZE_UNKNOWN(CDropperActEntry);
SIZE_UNKNOWN(CShadowActEntry);
SIZE_UNKNOWN(CDroppedObject);
SIZE_UNKNOWN(DropAnimSink);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(DropReg2c);
