#include <Mfc.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/LeafCue.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/Boomerang.h> // CBoomerang::MovingSlot16 (@0xe08b0) is defined here, interleaved
#include <Gruntz/LightFx.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GameRegistry.h> // CGameRegistry singleton (pulls SoundCue.h + TileGrid.h)
#include <Gruntz/TriggerMgr.h>   // canonical CTriggerMgr (m_cmdGrid: LoadExplosionSprites @0x7b330)
#include <Gruntz/State.h>        // CState (reg->m_curState: the level-type descriptor)
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/TypeNameEntry.h>     // the shared type-name-registry record (CString m_name)
#include <Gruntz/StringNode.h>        // the shared type-name teardown slot (CStringNode::Free)
#include <Gruntz/ActReg.h>
#include <Bute/ButeMgr.h>   // CButeTree (the type-registry funnel)
#include <math.h>           // sin / cos (StepMotion's parabola)
#include <string.h>         // memset (1-arg spawn ctor's +0x1e0 zero-fill)
#include <rva.h>
#include <Globals.h>
#include <Wap32/ZVec.h>
#include <Gruntz/StatusBarUpdatersViews.h>
#include <Bute/ButeTree.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/HaznColl.h> // shared coordinate/activation-registry collection (CCoordColl)
#include <Gruntz/TimeBomb.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialArchive.h>     // CSerialArchive (the inherited CWapX::Chain arg)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // the anim registry (m_10 Lookup; ex SerialObjRef.h pull)
#include <DDrawMgr/DDrawSurfaceMgr.h> // obj->m_0c world root (ex SerialObjRef.h pull)
#include <Gruntz/ActName.h>       // CActName (shared)
#include <Gruntz/ActReg.h>        // CLogicActTable::ResolveEntry (0xade60 dispatcher's real table)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor::Setup (0x15c2d0) for the m_1a0 forwarder


#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
#include <Gruntz/SerialCounter.h> // g_serialCounter (SerializeMove's per-record bumps)
#include <Gruntz/AniElement.h> // CAniElement complete type (KeyOfValue's CObject* upcast)


VTBL(CTimeBomb, 0x001e771c);
VTBL(CProjectile, 0x001e798c);
DATA(0x001f04b0)
const double g_movingLogicMin = -2147483647.0;
DATA(0x001f04b8)
const double g_movingLogicMax = 2147483646.0;

CMovingLogic::~CMovingLogic() {}

// @confidence: high
// @source: rtti-vptr
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): body, all
// field offsets, the double init and the CPtrList construction are byte-identical;
// the only residue is this ctor's OWN __ehfuncinfo (the EH prologue pushes
// funcinfo+0x0 vs retail funcinfo+0xe, and the CPtrList state id is `mov
// [esp+0x18],1` vs retail 2). NOT resolvable by completing the projectile TU:
// the funcinfo is per-function, and the out-of-line CMovingLogic ctor (0x13940)
// cannot be emitted from this TU (the no-arg ctor must keep CMovingLogic() inline
// to fold it; forcing the standalone drops this ctor 99%->19.7%; nothing here
// calls CMovingLogic out-of-line). See the CAVEAT in the pattern doc. ~99% wall.
// @interleaver CProjectile - own-class out-of-line COMDAT pooled in the 0x12xxx leaf-
// ctor band (physical neighbours are other classes' COMDATs: CTimeBomb dtor @0x12a70,
// CMotionState ctor @0x136d0, CMovingLogic ctor @0x13940). RVA-placement artifact of
// the /INCREMENTAL COMDAT pool, not a conflation - kept in its own class file.
RVA(0x000126e0, 0x1fc)
CProjectile::CProjectile() {}

// CTimeBomb::~CTimeBomb @0x012a70 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CKitchenSlime
// @0x013100 / the established leaf dtors; the empty body is enough for cl.
// @interleaver CTimeBomb - own-class COMDAT-pooled leaf dtor in the 0x12xxx dtor pool
// (CTimeBomb shares this merged TU; kept here, RVA-placement artifact not conflation).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CTimeBomb() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00012a70, 0x44, ??1CTimeBomb@@UAE@XZ)

typedef void (CMovingLogic::*MovingCallback)();

RVA(0x00013c70, 0x47)
void CMovingLogic::FinalizeStep(i32) {
    if (m_deferredCallback != 0) {
        if (m_gatedCallback != 0 && reinterpret_cast<i32>(m_objAux->m_1c) == m_28) {
            (this->*reinterpret_cast<MovingCallback&>(m_gatedCallback))();
            m_gatedCallback = 0;
        }
        (this->*reinterpret_cast<MovingCallback&>(m_deferredCallback))();
        m_deferredCallback = 0;
        m_28 = 0x3e9;
    }
    MovingSlot16(); // virtual slot 16 (vtable offset 0x40) - CMovingLogic's one new virtual
}

DATA(0x001eaa88)
const double g_motionZScale = 0.0;
DATA(0x001eab00)
const double g_projPhase1 = 6.2831854; // 0x5eab00  2*pi phase wrap (m_phase > g_projPhase1)
DATA(0x001f04e8)
u32 g_defaultZ = 0;

// @confidence: med
// @source: rtti-vptr / disasm
// @early-stop
// The 1-arg spawn ctor CProjectile(owner), 0.63% stub -> ~55%. Reconstructed
// ADDITIVELY (new overloaded ctors CProjectile(owner)/CMovingLogic(owner); the
// byte-exact no-arg ctor 0x126e0 + its inline CMovingLogic are UNTOUCHED - verified
// still 99.05%). The bounds/SetCoords/CProjectile-body region is byte-shaped.
// DOMINANT wall (asm-level, llvm-objdump -dr base vs target): retail CALLS the
// CUserLogic(owner) base init OUT-OF-LINE (`call 0x58cd0`, ~5 B), but MSVC here
// INLINES the whole CUserLogic init (~160 B: CUserBase vptr, CUserBaseLink ctor,
// EngStr temp, RegisterLogicTypesOnce, AddLogicHit/Attack/Bump). Cause: the
// CUserLogic(owner) modeled in UserLogic.h is a SUBSET of retail's real init, so
// it fits MSVC's inline budget when folded into this large ctor, whereas retail's
// fuller init exceeds it and is emitted out-of-line. #pragma inline_depth(1) has
// no effect on MSVC5's mem-init base-ctor inlining. Forcing it out-of-line needs
// either the full CUserLogic(owner) body (a separate CUserLogic reconstruction,
// risks the matched tile-trigger leaves that inline it) or making +0x38 a member-
// with-ctor (would regress the banked 99% no-arg ctor - task-forbidden).
// Secondary residues: +0x38 Init (0x136d0) emits after the CMovingLogic vptr
// stamp vs retail's member-init position (EH state 0); the EH-state numbering
// (retail 0/1/2/3 over +0x38 + CPtrList) and the m_hitList-vs-body order differ.
RVA(0x000dec60, 0x255)
CProjectile::CProjectile(CGameObject* owner) : CMovingLogic(owner) {
    // The band init - THIS ctor's own copy (its grunt sibling @0x47a10 carries a
    // drifted copy: g_gruntSpawnScale for the step + a SetZ call for the Z seed).
    Motion()->Init();
    // Each bound: 0 => the shared MIN/MAX double copied dword-wise; else the int
    // widened via fild (if/else, not ?:, so the constant branch stays a mov/mov copy).
    i32 lo0 = m_objAux->m_2c;
    if (lo0 == 0) {
        m_a8 = g_movingLogicMin;
    } else {
        m_a8 = static_cast<double>(lo0);
    }
    i32 lo1 = m_objAux->m_34;
    if (lo1 == 0) {
        m_b0 = g_movingLogicMin;
    } else {
        m_b0 = static_cast<double>(lo1);
    }
    i32 hi0 = m_objAux->m_30;
    if (hi0 == 0) {
        m_c0 = g_movingLogicMax;
    } else {
        m_c0 = static_cast<double>(hi0);
    }
    i32 hi1 = m_objAux->m_38;
    if (hi1 == 0) {
        m_c8 = g_movingLogicMax;
    } else {
        m_c8 = static_cast<double>(hi1);
    }
    Motion()->SetParams(
        static_cast<double>(m_object->m_screenX),
        static_cast<double>(m_object->m_screenY),
        0.0,
        static_cast<double>(m_object->m_164),
        static_cast<double>(m_object->m_168),
        0.0,
        0.0,
        0.0,
        0.0,
        static_cast<double>(g_frameTime) * g_motionZScale,
        0.0
    );
    m_110 = m_118 = m_120 = static_cast<double>(g_defaultZ);
    m_148 = 0;
    m_14c = 0;
    m_object->m_moveMode = 7;
    // The base per-frame update fired once at spawn - qualified = direct @0x16ea90
    // (was the fake free-fn alias "Fn16ea90").
    CMovingLogic::MovingSlot16();
    // Seed the CWapX base's back-pointers (the +0x150 second base; <Gruntz/UserLogic.h>).
    // ASSIGNED IN THE BODY, not via a `CWapX(owner)` init-list base ctor: retail puts
    // these three stores AFTER m_148/m_14c/m_moveMode/Fn16ea90, which a base ctor could
    // not do (it must run before the body). So CWapX's ctor is trivial and every derived
    // ctor assigns the inherited fields itself - see the ordering note in UserLogic.h.
    // Typing them on the base also retires three `(i32)`/`(CGameObject*)` casts.
    m_34 = owner;
    m_38 = static_cast<CWwdGameObjectA*>(owner); // the bound owner IS the A-kind sprite
    m_3c = owner->m_7c;
    m_38->m_flags |= 0x2000002;
    m_38->m_stateFlags |= 1;
    if (m_object->m_sortKey != 0xcf850) {
        m_object->m_sortKey = 0xcf850;
        m_object->m_flags |= 0x20000;
    }
    memset(&m_frames[0], 0, 0x1c); // zero the seven +0x1e0..+0x1fb sprite-frame slots
    m_sound = 0;
    m_shadow = 0;
}

// ---------------------------------------------------------------------------
// CProjectile::~CProjectile (0xdef60) - the most-derived dtor. Stop+rewind the
// launch sample, recycle each tracked-hit node back onto the global free-list,
// RemoveAll the list, then the compiler auto-destructs the CPtrList member and the
// CMovingLogic/CUserLogic/link base subobjects (the throwing link forces the /GX
// frame). Field names are placeholders; the offsets are load-bearing.
//
// @early-stop
// EH-state-numbering / base-dtor-inlining wall (docs/patterns/eh-state-numbering-base.md):
// the body is byte-identical through `m_hitList.RemoveAll()` (the m_sound stop, the
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
    if (m_sound != 0) {
        m_sound->StopAndRewind();
        m_sound = 0;
    }
    for (POSITION pos = m_hitList.GetHeadPosition(); pos != NULL;) {
        void* data = m_hitList.GetNext(pos);
        if (data != 0) {
            // authentic: freelist recycle - bias the node back to its list-link header
            CoordPoolNode* node = g_coordPool.NodeOf(data);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_hitList.RemoveAll();
}

enum ProjectileKind {
    PROJ_BOOMERANG = 2, // GRUNTZ_BOOMERANGGRUNT
    PROJ_GUNHAT = 9,    // GRUNTZ_GUNHATGRUNT
    PROJ_NERFGUN = 10,  // GRUNTZ_NERFGUNGRUNT
    PROJ_ROCK = 11,     // GRUNTZ_ROCKGRUNT
    PROJ_WELDER = 21,   // GRUNTZ_WELDERGRUNT
    PROJ_WINGZ = 22,    // GRUNTZ_WINGZGRUNT
};

// @early-stop
// x87-scheduling + EH-frame-size wall (~58%, docs/patterns, same family as
// StepMotion ~70% and gx-scoped-local-eh-frame-size): the prologue, the tile-snap
// + id stores, the dense jump-table switch (kind 2/9/10/11/21/22 -> the six sprite
// bases, tail-merged arc-flag commons), the seven CString `base + "N"` concat +
// geometry-map Lookups, the frame-0 Setup and CacheFirstFrame are reproduced. Two
// residues: (1) the fxch-laden trajectory-normalisation block (0xdf4db..0xdf6ae:
// the sqrt / fdiv / fdivr chain, the two sign-of-component fcomp ladders and their
// interleaved qword stores) whose x87 stack ordering is not steerable from C; its
// double temps also miss the retail /GX frame by a dword (cl `sub esp,0x18` vs
// retail `sub esp,0x1c` - tried 0x18/0x1c/0x20 local combos, none lands 0x1c),
// which shifts EVERY front-half [esp+N] displacement by 4 (opcodes match, disp
// bytes differ). (2) engine-call/string relocs (operator+/Lookup/CreateSprite/
// GetDwordDef/CacheFirstFrame reached direct where retail uses ILT thunks). Logic
// complete; parked for the final sweep.
RVA(0x000df050, 0x6ba)
i32 CProjectile::LoadProjectileSprites(i32 kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1) {
    CString key;
    m_srcRow = a;
    m_targetX = (sx & ~0x1f) + 0x10;
    m_srcCol = b;
    m_kind = kind;
    m_targetY = (sy & ~0x1f) + 0x10;
    m_targetId = t0;
    m_ownerId = t1;

    CWwdGameObjectA* owner = m_object;
    double dx = static_cast<double>((m_targetX - owner->m_screenX));
    double dy = static_cast<double>((m_targetY - owner->m_screenY));
    i32 count = 1;

    switch (kind) {
        case PROJ_ROCK: // ROCK
            key = "GRUNTZ_ROCKGRUNT_PROJECTILE";
            m_timePerTile = g_buteMgr.GetDwordDef("Projectile", "RockProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PROJ_GUNHAT: // GUNHAT
            key = "GRUNTZ_GUNHATGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "GunhatProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PROJ_BOOMERANG: // BOOMERANG
            key = "GRUNTZ_BOOMERANGGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "BoomerangProjectileTimePerTile", 0xbb8);
            m_isArcing = 0;
            break;
        case PROJ_NERFGUN: // NERFGUN
            key = "GRUNTZ_NERFGUNGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "NerfGunProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PROJ_WELDER: // WELDER
            key = "GRUNTZ_WELDERGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "WelderProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PROJ_WINGZ: { // WINGZ
            key = "GRUNTZ_WINGZGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "WingzProjectileTimePerTile", 0xbb8);
            LaunchSound("GRUNTZ_WINGZGRUNT_WINGZGRUNTLOOP");
            m_isArcing = 0;
            i32 ddx = (m_targetX >> 5) - (owner->m_screenX >> 5);
            if (ddx < 0) {
                ddx = -ddx;
            }
            i32 ddy = (m_targetY >> 5) - (owner->m_screenY >> 5);
            if (ddy < 0) {
                ddy = -ddy;
            }
            count = ddx;
            if (ddx <= ddy) {
                count = ddy;
            }
            break;
        }
        default:
            return 0;
    }

    // Resolve the six numbered frame sprites; frame "1" is required.
    CMapStringToPtr& map =
        m_38->OwnerMgr()->m_animRegistry->m_10; // Lookup 0x1b8438 -> void& out-param
    void* out;
    out = 0;
    map.Lookup(key + "1", out);
    m_frames[0] = static_cast<CAniElement*>(out); // (the Ptr map is void*-valued; container-edge cast)
    if (m_frames[0] == 0) {
        return 0;
    }
    out = 0;
    map.Lookup(key + "2", out);
    m_frames[1] = static_cast<CAniElement*>(out); // (the Ptr map is void*-valued; container-edge cast)
    out = 0;
    map.Lookup(key + "3", out);
    m_frames[2] = static_cast<CAniElement*>(out); // (the Ptr map is void*-valued; container-edge cast)
    out = 0;
    map.Lookup(key + "4", out);
    m_frames[3] = static_cast<CAniElement*>(out); // (the Ptr map is void*-valued; container-edge cast)
    out = 0;
    map.Lookup(key + "5", out);
    m_frames[4] = static_cast<CAniElement*>(out); // (the Ptr map is void*-valued; container-edge cast)
    out = 0;
    map.Lookup(key + "IMPACT", out);
    m_frames[PF_IMPACT] = static_cast<CAniElement*>(out);
    out = 0;
    map.Lookup(key + "FALL", out);
    m_frames[PF_FALL] = static_cast<CAniElement*>(out);

    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(m_frames[0]);
    m_38->ApplyName(key + "_OBJECT");

    // Normalise the launch trajectory into the per-frame velocity + sign vectors.
    u32 totalTime = static_cast<u32>((count * m_timePerTile));
    double len = sqrt(dx * dx + dy * dy);
    double t = static_cast<double>(totalTime);
    double vx = dx / len;
    m_flightDist = len;
    m_velScale = len / t;
    m_posX = vx;
    m_posY = dy / len;
    m_velX = vx;
    m_velY = dy / len;
    // sign(dx): +0.5 / 0.0 / -0.5 stored as a double {lo=0, hi=+-0x3fe00000}
    m_roundXLo = 0;
    if (vx > 0.0) {
        m_roundXHi = 0x3fe00000;
    } else if (vx < 0.0) {
        m_roundXHi = static_cast<i32>(0xbfe00000);
    } else {
        m_roundXHi = 0;
    }
    m_roundYLo = 0;
    if (dy > 0.0) {
        m_roundYHi = 0x3fe00000;
    } else if (dy < 0.0) {
        m_roundYHi = static_cast<i32>(0xbfe00000);
    } else {
        m_roundYHi = 0;
    }
    m_flightDist = len < 0.0 ? -len : len;
    m_curX = owner->m_screenX;
    m_curY = owner->m_screenY;
    m_arrived = 0;

    // Spawn the LightFx shadow companion + activate its two frames.
    CDDrawChildGroup* factory = g_gameReg->m_world->m_childGroup;
    m_shadow =
        (factory
            ->CreateSprite(0, owner->m_screenX, owner->m_screenY, 0xcf84f, "LightFx", 0x2040003));
    if (m_shadow != 0) {
        m_shadow->m_7c->m_notify(m_shadow);
        (static_cast<CLightFx*>(m_shadow->m_7c->m_logic))
            ->Activate(reinterpret_cast<i32>(static_cast<const char*>((key + "_SHADOW"))), reinterpret_cast<i32>(static_cast<const char*>((key + "1"))), 5, 1);
    }

    // Latch the class act key ("A"): save the old registry node, then re-point it.
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    return 1;
}

#include <Gruntz/TypeKeyColl.h>

DATA_SYMBOL(0x0024c758, 0x24, ?g_projActColl@@3UCActReg@@A)


static inline CProjActEntry* ProjActLookup(i32 coord) {
    return reinterpret_cast<CProjActEntry*>(g_projActColl.ResolveEntry(coord));
}

static inline CTypeNameEntry* ProjTypeLookup(i32 key) {
    g_typeColl.m_grown = 0;
    if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
        return reinterpret_cast<CTypeNameEntry*>((g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride));
    }
    if (reinterpret_cast<i32>((static_cast<_zvec*>(&g_typeColl))->GrowTo(key, 0))) {
        return reinterpret_cast<CTypeNameEntry*>((g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride));
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, reinterpret_cast<i32>(item), 0xc);
    return reinterpret_cast<CTypeNameEntry*>(g_typeColl.m_spare); // m_spare is the i32-typed slow-path slot
}

RVA(0x000df920, 0x15)
void CProjectile::RegisterRange() {
    g_projActColl.Construct(0x7d0, 0x7da);
}

RVA(0x000df9a0, 0x102)
void CProjectile::FireActivation(i32 coord) {
    CProjActEntry* e = ProjActLookup(coord);
    if (e->m_fn != 0) {
        (this->*(ProjActLookup(coord)->m_fn))();
    }
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
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        i32 key = g_typeCounter;
        id = key;
        CTypeNameEntry* slot = ProjTypeLookup(key);
        i32 cnt = g_typeColl.m_grown;
        CStringNode* nodes = reinterpret_cast<CStringNode*>(g_typeColl.m_alloc);
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    (reinterpret_cast<CString*>(nodes))->~CString();
                }
                nodes++;
            } while (--cnt);
        }
        slot->m_name = "A";
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(ProjActLookup(id)) = static_cast<void*>(&ProjActivationHandler);
}

// ===========================================================================
// CProjectile::MovingSlot16 (0xdfd00) - per-frame trajectory advance +
// impact-effect select. Runs each frame until the projectile reaches its target
// tile (m_curX/m_curY catch up to m_targetX/m_targetY): integrate the parabola into the
// render position (m_posX/m_posY), clamp the muzzle-tracked position against the
// target, and for the arc kinds (m_isArcing) select one of five impact-effect sprite
// tiers by the fractional distance-to-target. On arrival it stops the loop
// sound, runs a final hit-scan, then - by the destination terrain flags - spills
// the water / level death splash effect or installs the IMPACT/FALL sprite. The
// one-shot m_arrived latch (0 while in flight; set on arrival) gates re-entry.
// (WINGZ, kind 0x16, additionally loops its flight sound while over the level.)
//
// @early-stop
// x87-scheduling + EH-frame family wall (same as LoadProjectileSprites ~58% /
// StepMotion ~70%; docs/patterns): the control flow, the WINGZ sound gate, the
// reached-destination hit-scan + terrain-flag switch (water/death/tier), the
// LEVEL_DEATHSPLASH/GAME_WATER spawns and the five distance-tier sprite installs
// are reconstructed. The residue is the dense fxch/fcompp FP schedule of the
// parabola integration (0xdfeb6..0xdff37) and the distance/sqrt tier ladder
// (0xdffd7..0xe00e9) - MSVC5 keeps `dist` live on the x87 stack across the eight
// tier comparisons and pre-computes m_flightDist*0.9 interleaved with the fsqrt, which
// is not steerable from C source - plus the effect-spawn regalloc (ecx vs edx for
// reg->m_world) and the unnamed engine-call relocs. Logic complete; parked.
// ===========================================================================
RVA(0x000dfd00, 0x6f5)
void CProjectile::MovingSlot16() {
    if (m_arrived != 0) {
        return;
    }

    if (m_kind == 0x16) { // WINGZ: loop the flight sound while over the level
        CWwdGameObjectA* owner = m_object;
        CGruntzMgr* reg = g_gameReg;
        if (owner->m_screenX < reg->m_viewOriginR && owner->m_screenX >= reg->m_viewOriginL
            && owner->m_screenY < reg->m_viewOriginB && owner->m_screenY >= reg->m_viewOriginT) {
            LaunchSound("GRUNTZ_WINGZGRUNT_PROJECTILELOOP");
        } else if (m_sound != 0) {
            m_sound->StopAndRewind();
            m_sound = 0;
        }
    }

    if (m_curX != m_targetX || m_curY != m_targetY) {
        // -- in flight: integrate one frame + select the impact tier ----------
        if (m_kind == 0x16) {
            ScanTargets(0);
        }
        m_posX = m_posX + static_cast<double>(static_cast<u32>(g_frameDelta)) * m_velX * m_velScale;
        m_posY = m_posY + static_cast<double>(static_cast<u32>(g_frameDelta)) * m_velY * m_velScale;
        i32 xRes = static_cast<i32>((*reinterpret_cast<double*>(&m_roundXLo) + m_posX));
        i32 yRes = static_cast<i32>((*reinterpret_cast<double*>(&m_roundYLo) + m_posY));
        i32 localX = xRes;
        if (m_velX > 0.0) {
            if (xRes > m_targetX) {
                localX = m_targetX;
                xRes = m_targetX;
            }
        } else if (m_velX < 0.0) {
            if (xRes < m_targetX) {
                localX = m_targetX;
                xRes = m_targetX;
            }
        }
        if (m_velY > 0.0) {
            if (yRes > m_targetY) {
                yRes = m_targetY;
            }
        } else if (m_velY < 0.0) {
            if (yRes < m_targetY) {
                yRes = m_targetY;
            }
        }
        m_curX = xRes;
        m_curY = yRes;
        i32 offX = 0;
        i32 offY = 0;
        if (m_isArcing != 0) {
            double dx = fabs(static_cast<double>(m_targetX) - m_posX);
            double dy = fabs(static_cast<double>(m_targetY) - m_posY);
            double dist = sqrt(dx * dx + dy * dy);
            double mag = m_flightDist;
            if (dist >= mag * 0.9 || dist < mag * 0.1) {
                offX = 0x4;
                offY = -0x4;
                if (m_38->m_1a0.m_14 != m_frames[0]) {
                    m_value = m_38->m_1a0.m_14;
                    m_38->m_1a0.Setup(m_frames[0]);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.Setup(m_frames[0]);
                    }
                }
            } else if (dist >= mag * 0.8 || dist < mag * 0.2) {
                offX = 0x8;
                offY = -0x8;
                if (m_38->m_1a0.m_14 != m_frames[1]) {
                    m_value = m_38->m_1a0.m_14;
                    m_38->m_1a0.Setup(m_frames[1]);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.Setup(m_frames[1]);
                    }
                }
            } else if (dist >= mag * 0.7 || dist < mag * 0.3) {
                offX = 0xc;
                offY = -0xc;
                if (m_38->m_1a0.m_14 != m_frames[2]) {
                    m_value = m_38->m_1a0.m_14;
                    m_38->m_1a0.Setup(m_frames[2]);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.Setup(m_frames[2]);
                    }
                }
            } else if (dist >= mag * 0.6 || dist < mag * 0.4) {
                offX = 0x10;
                offY = -0x10;
                if (m_38->m_1a0.m_14 != m_frames[3]) {
                    m_value = m_38->m_1a0.m_14;
                    m_38->m_1a0.Setup(m_frames[3]);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.Setup(m_frames[3]);
                    }
                }
            } else {
                offX = 0x14;
                offY = -0x14;
                if (m_38->m_1a0.m_14 != m_frames[4]) {
                    m_value = m_38->m_1a0.m_14;
                    m_38->m_1a0.Setup(m_frames[4]);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.Setup(m_frames[4]);
                    }
                }
            }
        }
        m_object->m_screenX = offX + m_curX;
        m_object->m_screenY = offY + m_curY;
        if (m_shadow != 0) {
            m_shadow->m_screenX = localX;
            m_shadow->m_screenY = yRes;
        }
        return;
    }

    // -- arrived at the target tile ------------------------------------------
    if (m_sound != 0) {
        m_sound->StopAndRewind();
        m_sound = 0;
    }
    ScanTargets(0);
    if (m_shadow != 0) {
        m_shadow->m_flags |= 0x10000;
        m_shadow = 0;
    }
    m_arrived = 1;
    i32 tier = 0;
    if (m_kind != 0x16) {
        CGruntzMgr* reg = g_gameReg;
        CTileGrid* plane = reg->m_tileGrid;
        i32 tileX = m_targetX >> 5;
        i32 tileY = m_targetY >> 5;
        u32 flags;
        if (static_cast<u32>(tileX) >= static_cast<u32>(plane->m_width)
            || static_cast<u32>(tileY) >= static_cast<u32>(plane->m_height)) {
            flags = 1;
        } else {
            flags = plane->m_rowInts[tileY][tileX * 7]; // cell dword 0 = terrain flags
        }
        if (flags & 0x900) {
            // water tile: spill a splash then hide the projectile
            if (m_targetX < reg->m_viewOriginR && m_targetX >= reg->m_viewOriginL
                && m_targetY < reg->m_viewOriginB && m_targetY >= reg->m_viewOriginT) {
                CWwdGameObjectA* fx =
                    reg->m_world->m_childGroup
                        ->CreateSprite(0, m_targetX, m_targetY, 0xcf84f, "Particlez", 0x40003);
                if (fx != 0) {
                    fx->ApplyName("GAME_WATER");
                    fx->ApplyLookupGeometry("GAME_WATER", 0);
                }
            }
            m_38->m_flags |= 0x10000;
            return;
        }
        if (flags & 0x2) {
            if (flags & 0x40) {
                tier = 1;
            } else {
                switch (reg->m_curState->m_levelType) {
                    case 4:
                    case 5:
                    case 8:
                        tier = 1;
                        break;
                    case 6:
                        break;
                    default:
                        // level death tile: spill the death-splash then hide
                        if (m_targetX < reg->m_viewOriginR && m_targetX >= reg->m_viewOriginL
                            && m_targetY < reg->m_viewOriginB && m_targetY >= reg->m_viewOriginT) {
                            CWwdGameObjectA* fx = reg->m_world->m_childGroup->CreateSprite(
                                0,
                                m_targetX,
                                m_targetY,
                                0xcf84f,
                                "Particlez",
                                0x40003
                            );
                            if (fx != 0) {
                                fx->ApplyName("LEVEL_DEATHSPLASH");
                                fx->ApplyLookupGeometry("LEVEL_DEATHSPLASH", 0);
                            }
                        }
                        m_38->m_flags |= 0x10000;
                        return;
                }
            }
        }
    }
    CAniElement* sprite = (tier != 0) ? m_frames[PF_FALL] : m_frames[PF_IMPACT];
    if (sprite == 0) {
        m_38->m_flags |= 0x10000;
        return;
    }
    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(sprite);
}

RVA(0x000e05e0, 0x4e)
i32 CProjectile::DetachRenderObj() {
    m_38->m_stateFlags &= ~1u;
    // The +0x1a0 anim sub-object IS a CAniAdvanceCursor (Advance @0x15c360); call
    // it directly (retail's DetachRenderObj rel32s straight to 0x15c360, not a forwarder),
    // matching the cast-at-use pattern of the sibling site below (~line 1322).
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CWwdGameObjectA* r = m_38;
    if (r->m_1a0.m_28 != 0 && r->m_1a0.m_20 == 0) {
        r->m_flags |= 0x10000;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CBoomerang::MovingSlot16 (0xe08b0) - the boomerang's per-frame motion step
// (vtable slot 16, override of CMovingLogic's Update; the retail slot holds thunk
// 0x317f -> 0xe08b0). Formerly mis-homed as CProjectile::StepMotion: it reads the
// return-trajectory fields (m_dirX/m_originX/m_phase/m_launched at +0x230..+0x258)
// which live in CBoomerang (sizeof 0x260), NOT CProjectile (sizeof 0x228). It stays
// defined here (interleaved in the CProjectile .text band, sharing g_frameDelta /
// g_projPhase* with CProjectile::MovingSlot16); CBoomerang inherits ScanTargets and
// the render/motion members it also touches.
//
// On the launch frame it snaps the render objects to the muzzle (m_targetX/m_targetY);
// once past the second phase threshold it expires (scan for the terminal impact, raise
// the hide bit on the shadow + owner). Otherwise it integrates the sin/cos parabola
// into the render position and rounds it into the render objects' screen coords.
//
// @early-stop
// x87-scheduling wall: the sin/cos parabola block (0xe0929..0xe09e1) is a dense
// fxch-laden FP stack schedule MSVC5 emits from the trajectory expression; the
// control flow, the muzzle-snap, the expire path and all __ftol rounds match, but
// the FP body's stack ordering is not steerable from C source. ~70% plateau.
// ---------------------------------------------------------------------------
RVA(0x000e08b0, 0x1de)
void CBoomerang::MovingSlot16() {
    i32 impact = 0;
    if (m_launched == 0) {
        if (m_phase > g_projPhase0) {
            // launch frame: snap render objects to the muzzle, mark launched.
            m_object->m_screenX = m_targetX;
            m_object->m_screenY = m_targetY;
            if (m_shadow != 0) {
                m_shadow->m_screenX = m_targetX;
                m_shadow->m_screenY = m_targetY;
            }
            m_launched = 1;
            goto step;
        }
    } else if (m_phase > g_projPhase1) {
        // past the terminal threshold: deliver the impact scan + expire.
        ScanTargets(1);
        if (m_shadow != 0) {
            m_shadow->m_flags |= 0x10000;
            m_shadow = 0;
        }
        m_38->m_flags |= 0x10000;
        return;
    }
step:
    ScanTargets(impact);
    // integrate the sin/cos parabola into the render position.
    double s = sin(m_phase);
    double c = cos(m_phase);
    double amp = static_cast<double>(g_frameDelta);
    double vx = -m_dirX;
    double vy = m_dirY;
    double px = m_originX + vy * m_velScale * s - vx * amp * c + m_phase;
    double py = m_originY + vy * amp * c + vx * m_velScale * s;
    m_posX = px;
    m_posY = py;
    m_phase = px;
    m_object->m_screenX = static_cast<i32>(m_posX);
    m_object->m_screenY = static_cast<i32>(m_posY);
    if (m_shadow != 0) {
        m_shadow->m_screenX = static_cast<i32>(m_posX);
        m_shadow->m_screenY = static_cast<i32>(m_posY);
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
    i32 tileY = 0;                            // [esp+0x10]  outer (row) counter
    i32 projXlo = m_object->m_screenX - 0x10; // [esp+0x1c]  m_10 = owner CGameObject
    i32 projYlo = m_object->m_screenY - 0x10; // [esp+0x20]
    i32 projXhi = projXlo + 0x20;             // [esp+0x24]
    i32 projYhi = projYlo + 0x20;             // [esp+0x28]
    i32 rowBase = 0x1c;                       // [esp+0x18]  row byte stride base
    i32 colOff;                               // [esp+0x14]
    i32 col;                                  // ebp
    do {
        col = 0;
        colOff = rowBase;
        for (; col < 0xf; col++, colOff += 4) {
            // authentic: sliding-window grid access - 0x1c row-stride overlaps the
            // 4-byte column pitch, so it is raw byte arithmetic, not a 2D pointer array.
            CGrunt* g = *reinterpret_cast<CGrunt**>((reinterpret_cast<char*>(g_gameReg->m_cmdGrid) + colOff));
            if (g == 0) {
                continue;
            }
            if (g->m_entranceCommitted == 0) {
                continue;
            }
            i32 gx = g->m_object->m_screenX - 7;
            i32 gy = g->m_object->m_screenY - 7;
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
            if (m_srcRow == tileY && m_srcCol == col) {
                // self cell
                if (impact != 0 && g->m_entranceCommitted != 0 && g->m_entranceReason == 0) {
                    g->LoadGruntTypeTable(2, 1, 0, 0);
                }
                return;
            }
            // already-tracked? walk the hit list for this grunt's cell key.
            i32 keyX = g->m_tileOwnerHi;
            i32 keyY = g->m_tileOwnerLo;
            for (POSITION pos = m_hitList.GetHeadPosition(); pos != NULL;) {
                // MFC CPtrList stores the raw Coord payloads; GetNext returns void*
                Coord* k = reinterpret_cast<Coord*>(m_hitList.GetNext(pos));
                if (k->m_x == keyX && k->m_y == keyY) {
                    return;
                }
            }
            // fresh hit: pull a key node off the free-list, record + deliver.
            Coord* slot = 0;
            CoordPoolNode* p = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead); // freelist head is void*
            if (p->m_next != 0) {
                slot = &p->m_coord; // the {x,y} payload overlays +4 (past the link word)
                slot->m_x = keyX;
                slot->m_y = keyY;
                g_coordPool.m_freeHead = p->m_next;
            }
            m_hitList.AddTail(slot);
            g->StepCombatReaction(m_kind, 1, m_srcRow, m_srcCol, m_targetId, m_ownerId, 1, 0);
        }
        rowBase += 0x3c;
        tileY++;
    } while (rowBase < 0x10c);
}

// ---------------------------------------------------------------------------
// 0xe0d40 - CProjectile::SerializeMove (slot 1; the vtable 0x1e798c[1] body).
// FOLDED from the CProjLoadRec view (its own header carried the proof:
// vtable_hierarchy resolves slot 1's ILT thunk 0x0034b3 here, and the view's
// layout mirrored this class field-for-field). Dual-mode: 7 = read the
// trajectory block + the 7 sprite frames by registry key + the shadow by object
// id + the hit list via the coord pool; 4 = the write mirror. Then the
// CMovingLogic base chain (the view's fabricated "ChainLoad") + the CWapX
// record tail (m_34/m_38/m_3c/m_value/m_blob).
// a4 carries the bound CGameObject* through the family's i32 slot-1 arg -
// reinterpreted at use exactly like CUserLogic::SerializeMove does.
// ---------------------------------------------------------------------------
// @early-stop
// scratch-slot scheduling tail (same family as CTriggerLoadRec/CEventLoadRec/
// CGruntStateRec): the dual-mode switch, every Read/Write field+size, the
// 7-entry name-ref loop, the type-code-gated map lookup, the g_coordPool
// m_freeHead splice + AddTail, the inline strlen/strcpy KeyOfValue temps, the
// g_serialCounter bumps, the base tail-chain and the embedded CWapX record are
// byte-faithful; residual is the MSVC5 scratch-buffer slot assignment +
// outparam zero-init store positions. Not source-steerable.
RVA(0x000e0d40, 0x6c2)
i32 CProjectile::SerializeMove(CGruntArchive* s, i32 mode, i32 a2, i32 a4) {
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];

    switch (mode) {
        case 7: {
            m_sound = 0;
            s->Read(&m_kind, 4);
            s->Read(&m_srcRow, 4);
            s->Read(&m_srcCol, 4);
            s->Read(&m_targetX, 4);
            s->Read(&m_targetY, 4);
            s->Read(&m_flightDist, 8);
            s->Read(&m_timePerTile, 4);
            s->Read(&m_velScale, 8);
            s->Read(&m_posX, 8);
            s->Read(&m_posY, 8);
            s->Read(&m_velX, 8);
            s->Read(&m_velY, 8);
            s->Read(&m_roundXLo, 8);
            s->Read(&m_roundYLo, 8);
            s->Read(&m_curX, 4);
            s->Read(&m_curY, 4);
            s->Read(&m_isArcing, 4);
            s->Read(&m_arrived, 4);
            s->Read(&m_targetId, 4);
            s->Read(&m_ownerId, 4);

            for (i32 ni = 0; ni < 7; ni++) {
                g_serialCounter++;
                s->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    void* out = 0; // CMapStringToPtr::Lookup (0x1b8438) takes a void&
                    reg->m_animRegistry->m_10.Lookup(buf, out);
                    m_frames[ni] = static_cast<CAniElement*>(out);
                } else {
                    m_frames[ni] = 0;
                }
            }

            g_serialCounter++;
            i32 key;
            s->Read(&key, 4);
            CGameObject* found = 0;
            i32 r;
            if (reg->m_childGroup->m_map48.Lookup(reinterpret_cast<void*>(key), reinterpret_cast<void*&>(found)) == 0) {
                r = 0;
            } else if (found == 0) {
                r = 0;
            } else {
                r = (found->GetClassId() == CLASSID_SERIALREF) ? reinterpret_cast<i32>(found) : 0;
            }
            m_shadow = reinterpret_cast<CWwdGameObjectA*>(r);
            if (m_shadow == 0 && key != 0) {
                return 0;
            }

            i32 cnt;
            s->Read(&cnt, 4);
            for (i32 ci = 0; ci < cnt; ci++) {
                CoordPoolNode* node = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
                void* payload = 0;
                if (node->m_next != 0) {
                    g_coordPool.m_freeHead = node->m_next;
                    payload = &node->m_coord;
                }
                s->Read(payload, 8);
                m_hitList.AddTail(payload);
            }
            break;
        }

        case 4: {
            s->Write(&m_kind, 4);
            s->Write(&m_srcRow, 4);
            s->Write(&m_srcCol, 4);
            s->Write(&m_targetX, 4);
            s->Write(&m_targetY, 4);
            s->Write(&m_flightDist, 8);
            s->Write(&m_timePerTile, 4);
            s->Write(&m_velScale, 8);
            s->Write(&m_posX, 8);
            s->Write(&m_posY, 8);
            s->Write(&m_velX, 8);
            s->Write(&m_velY, 8);
            s->Write(&m_roundXLo, 8);
            s->Write(&m_roundYLo, 8);
            s->Write(&m_curX, 4);
            s->Write(&m_curY, 4);
            s->Write(&m_isArcing, 4);
            s->Write(&m_arrived, 4);
            s->Write(&m_targetId, 4);
            s->Write(&m_ownerId, 4);

            for (i32 wi = 0; wi < 7; wi++) {
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_frames[wi] != 0) {
                    CString nm = reg->m_animRegistry->KeyOfValue(m_frames[wi]);
                    strcpy(buf, nm);
                }
                s->Write(buf, 0x80);
            }

            g_serialCounter++;
            i32 v = 0;
            if (m_shadow != 0) {
                v = m_shadow->m_188;
            }
            s->Write(&v, 4);

            i32 v2 = m_hitList.GetCount();
            s->Write(&v2, 4);

            for (CoordNode* n = reinterpret_cast<CoordNode*>(m_hitList.GetHeadPosition()); n != 0;
                 n = n->m_next) {
                s->Write(n->m_coord, 8);
            }
            break;
        }
    }

    if (CMovingLogic::SerializeMove(s, mode, a2, a4) == 0) {
        return 0;
    }
    if (s == 0) {
        return 0;
    }

    if (mode == 4) {
        char blob[0x80];
        memset(blob, 0, sizeof(blob));
        if (m_value != 0) {
            CString nm = m_3c->m_0c->m_animRegistry->KeyOfValue(m_value);
            strcpy(blob, nm);
        }
        s->Write(blob, 0x80);
        s->Write(m_blob, 0x10);
        return 1;
    }
    if (mode != 7) {
        return 1;
    }

    s->Read(buf, 0x80);
    s->Read(m_blob, 0x10);
    CGameObject* obj = reinterpret_cast<CGameObject*>(a4);
    m_34 = obj;
    m_38 = static_cast<CWwdGameObjectA*>(obj); // the bound obj IS the created A-kind sprite
    m_3c = obj->m_7c;
    if (strlen(buf) == 0) {
        m_value = 0;
        return 1;
    }
    void* out = 0; // CMapStringToPtr::Lookup (0x1b8438) takes a void&
    m_3c->m_0c->m_animRegistry->m_10.Lookup(buf, out);
    m_value = static_cast<CAniElement*>(out);
    return 1;
}

DATA_SYMBOL(0x0024c780, 0x24, ?g_tbombColl@@3UCCoordColl@@A)

RVA(0x000e17b0, 0x15)
void ConstructTBombRange() {
    g_tbombColl.Construct(0x7d0, 0x7da);
}

static inline CTBombEntry* TBombLookup(i32 coord) {
    return reinterpret_cast<CTBombEntry*>(g_tbombColl.ResolveEntry(coord));
}

#include <Gruntz/TypeKeyColl.h> // the REAL class at 0x6bf650 (its fields were the shredded g_type* globals)

static inline char* ActNameLookup(i32 id) {
    g_typeColl.m_grown = 0;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        return reinterpret_cast<char*>((g_typeColl.m_base + (id - g_typeColl.m_lo) * g_typeColl.m_stride));
    }
    if (reinterpret_cast<i32>((static_cast<_zvec*>(&g_typeColl))->GrowTo(id, 0))) {
        return reinterpret_cast<char*>((g_typeColl.m_base + (id - g_typeColl.m_lo) * g_typeColl.m_stride));
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, reinterpret_cast<i32>(item), 0xc);
    return reinterpret_cast<char*>(g_typeColl.m_spare);
}



RVA(0x000e1830, 0x102)
void CTimeBomb::FireActivation(i32 coord) {
    CTBombEntry* e = TBombLookup(coord);
    if (e->m_fn != 0) {
        CTBombEntry* e2 = TBombLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CTimeBomb::RegisterActs @0x0e1990 - bind the per-frame logic handler to the
// activation key "A" in the timebomb's OWN registry (g_tbombColl). See the
// registration commentary above. The SAME archetype as CParticlez::RegisterActs /
// RegisterSimpleAnimLogic.
//
// @early-stop
// zvec/name-vec IndexToPtr regalloc wall (docs/patterns/zero-register-pinning.md +
// the documented ZVec family): logic + the bute find/insert + the fn-ptr store are
// byte-faithful; cl pins the index/this/base across the grow branches differently
// than retail (the slot-vs-id callee-saved choice cascading into the free-loop
// count). Not source-steerable; the SAME plateau as CParticlez::RegisterActs.
RVA(0x000e1990, 0x18d)
void CTimeBomb::RegisterActs() {
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
    *reinterpret_cast<void**>(TBombLookup(id)) = static_cast<void*>(&TBombLogic_e1e60);
}

// ---------------------------------------------------------------------------
// CTimeBomb::CTimeBomb(CGameObject*) @0xe1b90 - the 1-arg leaf ctor: the standard
// CUserLogic(obj) init (folded inline) plus the timebomb tail - raise the bound
// object's logic/collision bits, set its z gate, apply the GAME_TIMEBOMB sprite,
// cache the anim-set node off the "A" bute key, snapshot m_38->m_1a0.m_14, then pick
// the FAST (per-tile-time>0) or SLOW (bute TimeBombSlowTime) geometry + running
// clock, mark the collision-grid cell at the bound object's tile, and arm the
// bound object's per-tile-time gate (-1). Constructs a throwing CUserBaseLink, so
// MSVC emits the /GX EH frame.
//
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-identical to retail (the CUserLogic init, the two flag RMWs, the
// ApplyName/anim-cache, the FAST/SLOW branch, the >>5 grid-cell mark, the m_124
// arm); the residue is this ctor's own __ehfuncinfo state numbering + the 1-slot
// callee-saved scheduling delta MSVC coin-flips. The SAME plateau as
// CBrickz::CBrickz / CStaticHazard::CStaticHazard; not source-steerable. Parked
// for the final sweep.
RVA(0x000e1b90, 0x23d)
CTimeBomb::CTimeBomb(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 0x2000002;
    if (m_object->m_sortKey != 0xf) {
        m_object->m_sortKey = 0xf;
        m_object->m_flags |= 0x20000;
    }
    m_38->ApplyName("GAME_TIMEBOMB");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_value = m_38->m_1a0.m_14;
    if (m_object->m_120 > 0) {
        m_38->ApplyLookupGeometry("GAME_TIMEBOMBFAST", 0);
        m_duration = static_cast<u32>(m_object->m_120);
        m_startTime = static_cast<u32>(g_frameTime);
        m_fastPhase = 1;
    } else {
        m_38->ApplyLookupGeometry("GAME_TIMEBOMBSLOW", 0);
        m_duration = static_cast<u32>(static_cast<i32>(g_buteMgr.GetDwordDef("Projectile", "TimeBombSlowTime", 0xfa0)));
        m_startTime = static_cast<u32>(g_frameTime);
        m_fastPhase = 0;
    }
    i32 cx = m_object->m_screenX >> 5;
    i32 cy = m_object->m_screenY >> 5;
    CTileGrid* g = g_gameReg->m_tileGrid;
    if (cx < g->m_width && cy < g->m_height) {
        char* row = reinterpret_cast<char*>(g->m_rows[cy]);
        *reinterpret_cast<i32*>((row + cx * 0x1c)) |= 0x1000000;
    }
    m_object->m_124 = -1;
}


static inline i32 TBombGridCell(CGameObject* obj) {
    CTileGrid* g = g_gameReg->m_tileGrid;
    i32 cx = obj->m_screenX >> 5;
    i32 cy = obj->m_screenY >> 5;
    if (static_cast<u32>(cx) < static_cast<u32>(g->m_width)
        && static_cast<u32>(cy) < static_cast<u32>(g->m_height)) {
        char* row = reinterpret_cast<char*>(g->m_rows[cy]);
        return *reinterpret_cast<i32*>((row + cx * 0x1c));
    }
    return 1;
}
static inline void TBombGridClear(CGameObject* obj) {
    CTileGrid* g = g_gameReg->m_tileGrid;
    i32 cx = obj->m_screenX >> 5;
    i32 cy = obj->m_screenY >> 5;
    if (static_cast<u32>(cx) < static_cast<u32>(g->m_width)
        && static_cast<u32>(cy) < static_cast<u32>(g->m_height)) {
        char* row = reinterpret_cast<char*>(g->m_rows[cy]);
        *reinterpret_cast<i32*>((row + cx * 0x1c)) &= ~0x1000000;
    }
}

// CTimeBomb::LoadAttributes @0x0e1e60 - the bomb's per-frame logic step. Read the
// bomb's tile state; if it sits under a blocking/hazard cell, raise m_38's pending
// flag, clear the cell's owner bit, and bail. Otherwise advance the anim sink and
// gate on the 64-bit running-clock timer (m_startTimeLo/m_startTimeHi base vs m_durationLo/m_durationHi duration);
// once it expires, either re-arm the FAST phase (m_fastPhase==0 -> apply GAME_TIMEBOMBFAST,
// reload the Projectile/TimeBombFastTime duration) or, on the second expiry
// (m_fastPhase!=0), detonate: raise m_38's flag, clear the cell, and post the tile event
// to the registry's tile-manager (NotifyMoveAt). Returns 0 on every path.
//
// @early-stop
// regalloc/scheduling wall (~90.5%, docs/patterns/zero-register-pinning.md +
// reread-member-view-pointer.md): the body is byte-faithful - the grid lookup,
// the `(cell&0x939)||(cell&2)` split test, the 64-bit clock-elapsed compare, the
// FAST re-arm bute read, and both detonate blocks all match. The residue is the
// coin-flip that pins the bound object (m_object) in eax vs ecx, which cascades into
// the screen-coord load order at the three inlined grid sites; plus the i64
// jl/jg branch-emission order and the NotifyMoveAt arg-push schedule. Not
// source-steerable (cy-first reordering regressed it). Parked for the final sweep.
RVA(0x000e1e60, 0x1ac)
i32 CTimeBomb::LoadAttributes() {
    i32 cell = TBombGridCell(m_object);
    if ((cell & 0x939) || (cell & 2)) {
        m_38->m_flags |= 0x10000;
        TBombGridClear(m_object);
        return 0;
    }
    m_38->m_1a0.Advance(g_engineFrameDelta);
    if (static_cast<i64>(g_frameTime) - m_startTime < m_duration) {
        return 0;
    }
    if (m_fastPhase == 0) {
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("GAME_TIMEBOMBFAST", 0);
        m_duration = static_cast<u32>(static_cast<i32>(g_buteMgr.GetDwordDef("Projectile", "TimeBombFastTime", 0x3e8)));
        m_startTime = static_cast<u32>(static_cast<i32>(g_frameTime));
        m_fastPhase = 1;
        return 0;
    }
    m_38->m_flags |= 0x10000;
    TBombGridClear(m_object);
    g_gameReg->m_cmdGrid
        ->LoadExplosionSprites(m_object->m_screenX, m_object->m_screenY, m_object->m_124, 1);
    return 0;
}

// CTimeBomb::SerializeMove @0xe2080 - vtable slot 1. Bail unless the resource
// manager is loaded (g_gameReg->m_world); round-trip the 64-bit phase-start clock
// (m_startTime) + the phase duration (m_duration) + the fast/slow phase flag
// (m_fastPhase) through the archive stream (mode 4 = Write @+0x30, mode 7 = Read
// @+0x2c), then chain the shared CUserLogic serialize helper (SerializeMove,
// 0x16e7f0) and the +0x34 CSerialObjRef sub-object's Chain (0x8c00). Same two-chain
// archetype as CGruntPuddle::Serialize.
// @early-stop
// regalloc/hoist wall (~79%, docs/patterns/zero-register-pinning.md): logic is
// byte-correct (the m_world gate, the m_58/m_60/m_54 round-trip, the SerializeMove
// + CSerialObjRef Chain tail). Residue: retail pins `this` in ebx and hoists
// `lea edi,[this+0x58]` above the mode branches (reusing edi via `add edi,8` for the
// consecutive 8-byte fields), where cl keeps `this` in edi and recomputes each
// field address - a callee-saved-register coloring choice not steerable from C.
RVA(0x000e2080, 0xc1)
i32 CTimeBomb::SerializeMove(CGruntArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    CSerialArchive* sa = static_cast<CSerialArchive*>(arc);
    if (mode == 4) {
        sa->Write(&m_startTime, 8);
        sa->Write(&m_duration, 8);
    } else if (mode == 7) {
        sa->Read(&m_startTime, 8);
        sa->Read(&m_duration, 8);
    }
    if (mode == 4) {
        sa->Write(&m_fastPhase, 4);
    } else if (mode == 7) {
        sa->Read(&m_fastPhase, 4);
    }
    if (!CUserLogic::SerializeMove(reinterpret_cast<CSerialArchive*>((reinterpret_cast<i32>(arc))), mode, a3, a4)) {
        return 0;
    }
    return Chain(sa, mode, a3, reinterpret_cast<CGameObject*>(a4)) ? 1 : 0;
}

// ---------------------------------------------------------------------------
// CProjectile::LaunchSound (0xe2190) - lazily create + play the launch sound the
// first time. Look the effect up in the game-registry sound map by name, clone a
// sample off the matched entry, store it at m_sound, and start it on the configured
// channel. Returns 1 on success, 0 if already launched / any lookup gate fails.
//
// CProjectile::LaunchSound: retail shares ONE `return 0` epilogue (0xe220c) that
// all five gates jump to; a per-gate `if (...) return 0` made cl emit the epilogue
// inline per gate (the OLD note's "byte-exact reloc-artifact" claim was wrong - it
// was a tail-merge structural miss). A shared `goto fail` reproduces the merged tail.
// @early-stop
// residual: cl still inlines the LAST gate's return-0 (jne play vs retail je fail)
// and schedules the reg->m_world load mid-setup vs retail's hoist-first - MSVC5 /O2
// block-layout coin-flips on the shared-tail's final arm; not source-steerable.
RVA(0x000e2190, 0x83)
i32 CProjectile::LaunchSound(const char* key) {
    CGruntzMgr* reg;
    void* entry_ob;
    LeafCue* entry;
    if (m_sound != 0) {
        goto fail;
    }
    reg = g_gameReg;
    if (reg->m_soundEnabled == 0) {
        goto fail;
    }
    entry_ob = 0;
    reg->m_world->m_soundRegistry->m_10.Lookup(key, entry_ob);
    entry = static_cast<LeafCue*>(entry_ob);
    if (entry == 0) {
        goto fail;
    }
    if (entry->m_10 == 0) {
        goto fail;
    }
    // GetItem returns the pooled DirectSound buffer (DirectSoundMgr in the cue-mgr
    // view); the projectile owns the same buffer as its m_sound sound sample.
    m_sound = static_cast<DirectSoundMgr*>(entry->m_10->GetItem());
    if (m_sound == 0) {
        goto fail;
    }
    m_sound->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
    return 1;
fail:
    return 0;
}
