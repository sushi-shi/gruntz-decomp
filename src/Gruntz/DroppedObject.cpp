#include <Gruntz/ObjectDropper.h> // CObjectDropper : CUserLogic (ctor 0xc59f0)
#include <Rez/FrameClock.h>        // frame-clock band (g_frameDelta/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Wap32/zBitVec.h>        // GetRetAddr/g_projActCache/g_retAddrBreadcrumb
#include <Io/FileMem.h>           // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/DroppedObject.h> // CDroppedObject : CUserLogic (ctor 0xc68b0)
#include <Gruntz/DroppedObjectShadow.h> // CDroppedObjectShadow : CUserLogic (ctor 0xc7490)
#include <Wap32/ZVec.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GameLevel.h> // CGameLevel (holder->m_24) + CLevelPlane (its m_mainPlane wrap extent)
#include <Gruntz/TypeKeyColl.h>
#include <Bute/ButeTree.h>
#include <Gruntz/ActReg.h> // the shared activation-registrar archetype (CSiblingActReg)
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgr.h>        // canonical CTriggerMgr (m_cmdGrid; FindGruntAt @0x75c60)
#include <Gruntz/LightFxMgr.h>        // CLightFxMgr (g_gameReg->m_logicPump @+0x78; m_tables[])
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/SerialArchive.h>     // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Gruntz/UserLogic.h>         // canonical CGameObject / CGameObjLayer (the bound object)
#include <Image/CImage.h> // the +0x198 cached frame (ex CGameObjLayer view)
#include <Gruntz/Brickz.h>            // canonical BrickzCell (the 0x1c-byte tile-grid cell)
#include <Gruntz/State.h> // canonical CState (g_gameReg->m_curState; m_levelType @+0x20)
#include <Globals.h>

#include <string.h> // inline strcmp for the direction-name match


DATA(0x001ea9f0)
const double g_objDropDiv = 32.0; // 0x5ea9f0  m_speed = g_objDropDiv / time
DATA(0x001eaa00)
double g_dropFallBias = -0.5; // 0x5eaa00  landed = m_fallY - g_dropFallBias

DATA(0x0024be90)
extern CSiblingActReg g_dropperActReg; // 0x64be90 (owner TU: real definition; interior
DATA(0x0024bed8)
extern CSiblingActReg g_dropColl; // 0x64bed8 (owner TU: real definition; interior fields
DATA(0x0024bf00)
extern CSiblingActReg g_shadowActReg; // 0x64bf00 (owner TU: real definition; interior

struct CTypeNameEntry; // canonical g_typeColl.m_spare slot record (<Gruntz/TypeNameEntry.h>)

#include <Gruntz/ActName.h> // CActName (shared)

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

extern i32 DropActA_c7090();
extern i32 DropActB_c7be0();

static inline CDropEntry* DropLookup(i32 coord) {
    return reinterpret_cast<CDropEntry*>(g_dropColl.ResolveEntry(coord));
}

#include <Gruntz/XferArchive.h>

typedef enum DropperDir {
    DROPDIR_NORTH = 1, // "LEVEL_OBJECTDROPPER_NORTH", (dx,dy) = ( 0,-1)
    DROPDIR_EAST = 2,  // "LEVEL_OBJECTDROPPER_EAST",  (dx,dy) = ( 1, 0)
    DROPDIR_SOUTH = 3, // "LEVEL_OBJECTDROPPER_SOUTH", (dx,dy) = ( 0, 1)
    DROPDIR_WEST = 4   // "LEVEL_OBJECTDROPPER_WEST",  (dx,dy) = (-1, 0)
} DropperDir;

// ===========================================================================
// The three low-band /GX leaf dtors (the 0x124f0..0x126b4 ctor-band pocket).
// Each folds the bare CUserLogic teardown: store the CUserLogic vptr (0x5e705c),
// inline-destruct the +0x18 link (the embedded ~EngStr call 0x16d2a0), store the
// CUserBase vptr (0x5e70b4). The destructible link forces the /GX EH frame; the
// leaf vptr store is dead-eliminated. Byte-identical in shape to ~CTimeBomb
// @0x012a70; the empty bodies are enough for cl.
// ===========================================================================
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CObjectDropper() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>
RVA_COMPGEN(0x000124f0, 0x44, ??1CObjectDropper@@UAE@XZ)

// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CDroppedObject() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x000125b0, 0x44, ??1CDroppedObject@@UAE@XZ)

// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CDroppedObjectShadow() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00012670, 0x44, ??1CDroppedObjectShadow@@UAE@XZ)

RVA(0x000c5630, 0xf4)
i32 ObjectDropperPump(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_7c;
    switch (reinterpret_cast<u32>(aux->m_1c)) {
        case 0: {
            aux->m_1c = reinterpret_cast<void*>(0x3e8);
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
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}

RVA(0x000c5770, 0xf1)
i32 DroppedObjectPump(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_7c;
    switch (reinterpret_cast<u32>(aux->m_1c)) {
        case 0: {
            aux->m_1c = reinterpret_cast<void*>(0x3e8);
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
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}

RVA(0x000c58b0, 0xf1)
i32 DroppedObjectShadowPump(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_7c;
    switch (reinterpret_cast<u32>(aux->m_1c)) {
        case 0: {
            aux->m_1c = reinterpret_cast<void*>(0x3e8);
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
            ProjTypeXfer(aux->m_logic);
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
CObjectDropper::CObjectDropper(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_lastDropTime = 0;
    m_dropInterval = 0;
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_OBJECTDROPPER", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 0x2000002;

    CWwdGameObjectA* o = m_object;
    i32 snapX = (o->m_screenX & ~0x1f) + 0x10;
    i32 snapY = (o->m_screenY & ~0x1f) + 0x10;
    o->m_screenX = snapX;
    m_posX = static_cast<double>(snapX);
    o->m_screenY = snapY;
    m_posY = static_cast<double>(snapY);
    if (o->m_sortKey != 0xcf851) {
        o->m_sortKey = 0xcf851;
        o->m_flags |= 0x20000;
    }

    CWwdGameObjectA* obj38 = m_38;
    if (obj38->m_194 != 0) {
        CString name;
        name = obj38->m_194 + 0x24;
        const char* s = name;
        if (strcmp(s, "LEVEL_OBJECTDROPPER_NORTH") == 0) {
            o->m_12c = DROPDIR_NORTH;
            m_travelDx = 0;
            m_travelDy = -1;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_EAST") == 0) {
            o->m_12c = DROPDIR_EAST;
            m_travelDx = 1;
            m_travelDy = 0;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_SOUTH") == 0) {
            o->m_12c = DROPDIR_SOUTH;
            m_travelDx = 0;
            m_travelDy = 1;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_WEST") == 0) {
            o->m_12c = DROPDIR_WEST;
            m_travelDx = -1;
            m_travelDy = 0;
        }
    }

    i32 time = g_buteMgr.GetDwordDef("Hazardz", "ObjectDropperTimePerTile", 1000);
    m_scrollMode = 0;
    m_lastDropTileX = -1;
    m_lastDropTileY = -1;
    m_speed = g_objDropDiv / static_cast<double>(static_cast<i64>(static_cast<u32>(time)));
    if (g_gameReg->m_134 == 1) {
        m_scrollMode = 1;
    }
    i32 sel = reinterpret_cast<i32>(g_gameReg->m_logicPump->m_tables[5]);
    o->m_drawActive = 1;
    o->m_drawFillCmd = 7;
    o->m_drawFillArg = sel;
    m_lastDropTime = 0;
    m_dropInterval = 0;
    o->m_area.left = 1;
    o->m_area.right = 1;
    o->m_area.top = 1;
    o->m_area.bottom = 1;
}

RVA(0x000c5f00, 0x15)
void CObjectDropper::InitActReg() {
    g_dropperActReg.Construct(0x7d0, 0x7da);
}

RVA(0x000c5f80, 0x102)
void CObjectDropper::FireActivation(i32 actId) {
    if ((reinterpret_cast<CDropperActEntry*>(g_dropperActReg.ResolveEntry(actId)))->m_fn != 0) {
        (this->*((reinterpret_cast<CDropperActEntry*>(g_dropperActReg.ResolveEntry(actId)))->m_fn))();
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
    (reinterpret_cast<CDropperActEntry*>(g_dropperActReg.ResolveEntry(id)))->m_fn = static_cast<i32 (CUserLogic::*)()>(&CObjectDropper::Update);
}

RVA(0x000c62e0, 0x2dd)
i32 CObjectDropper::Update() {
    if (static_cast<i64>(g_frameTime) - m_lastDropTime >= m_dropInterval) {
        if (g_gameReg->m_isEasyMode == 0 || g_gameReg->m_134 != 1) {
            CWwdGameObjectA* o = m_object;
            RECT box;
            box.left = o->m_screenX - o->m_layer->m_anchorX + 7;
            box.right = o->m_screenX + o->m_layer->m_anchorX - 7;
            box.top = o->m_screenY - o->m_layer->m_anchorY + 7;
            box.bottom = o->m_screenY + o->m_layer->m_anchorY - 7;
            i32 tx;
            i32 ty;
            CTmCell* found =
                g_gameReg->m_cmdGrid
                    ->FindGruntAt(o->m_screenX, o->m_screenY, &o->m_area, &tx, &ty, &box);
            if (found != 0) {
                if (m_lastDropTileX != tx || m_lastDropTileY != ty) {
                    if (m_scrollMode == 0 || tx == 0) {
                        CGameObject* fo = found->m_object;
                        i32 fx = fo->m_screenX;
                        i32 fy = fo->m_screenY;
                        CTileGrid* plane = g_gameReg->m_tileGrid;
                        i32 cx = fx >> 5;
                        i32 cy = fy >> 5;
                        u32 flags;
                        if (static_cast<u32>(cx) >= static_cast<u32>(plane->m_width)
                            || static_cast<u32>(cy) >= static_cast<u32>(plane->m_height)) {
                            flags = 1;
                        } else {
                            // the row table is typed i32** on CMapMgr; the row's cells are
                            // the canonical 0x1c-byte BrickzCell (its m_0 = packed terrain
                            // flags). @fold-TODO in MapMgr.h tracks retyping m_8 to
                            // BrickzCell** tree-wide.
                            flags = static_cast<u32>((reinterpret_cast<BrickzCell*>(plane->m_rows[cy]))[cx].m_0);
                        }
                        if ((flags & 2) == 0) {
                            g_gameReg->m_world->m_childGroup
                                ->CreateSprite(0, fx, fy, 0, "DroppedObjectShadow", 0x40003);
                            m_lastDropTileX = tx;
                            m_lastDropTileY = ty;
                            m_dropInterval =
                                g_buteMgr.GetDwordDef("Hazardz", "ObjectDropperDelay", 1000);
                            m_lastDropTime = g_frameTime;
                        }
                    }
                }
            }
        }
    }

    m_38->m_1a0.Advance(static_cast<i32>(g_engineFrameDelta));

    double drift = static_cast<double>(static_cast<u32>(g_frameDelta)) * m_speed;
    if (m_travelDx > 0) {
        m_posX += drift;
        if (m_posX >= static_cast<double>(g_gameReg->m_world->m_level->m_mainPlane->m_wrapW)) {
            m_posX = 0.0;
            m_lastDropTileX = -1;
            m_lastDropTileY = -1;
        }
    } else if (m_travelDx < 0) {
        m_posX -= drift;
        if (m_posX < 0.0) {
            m_posX = static_cast<double>((g_gameReg->m_world->m_level->m_mainPlane->m_wrapW - 1));
            m_lastDropTileX = -1;
            m_lastDropTileY = -1;
        }
    }
    if (m_travelDy > 0) {
        m_posY += drift;
        if (m_posY > static_cast<double>(g_gameReg->m_world->m_level->m_mainPlane->m_wrapH)) {
            m_posY = 0.0;
            m_lastDropTileX = -1;
            m_lastDropTileY = -1;
        }
    } else if (m_travelDy < 0) {
        m_posY -= drift;
        if (m_posY < 0.0) {
            m_posY = static_cast<double>((g_gameReg->m_world->m_level->m_mainPlane->m_wrapH - 1));
            m_lastDropTileX = -1;
            m_lastDropTileY = -1;
        }
    }

    m_object->m_screenX = static_cast<i32>(m_posX);
    m_object->m_screenY = static_cast<i32>(m_posY);
    return 0;
}

RVA(0x000c6680, 0x1b4)
i32 CObjectDropper::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d))) {
        return 0;
    }

    // The drop-timing i64 pair (m_lastDropTime/m_dropInterval), streamed through a
    // walking typed cursor.
    i64* p = &m_lastDropTime;
    switch (tag) {
        case 4:
            ar->Write(p, 8);
            p += 1;
            ar->Write(p, 8);
            break;
        case 7:
            ar->Read(p, 8);
            p += 1;
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
            i32 fill = reinterpret_cast<i32>(g_gameReg->m_logicPump->m_tables[5]);
            CWwdGameObjectA* o = m_object;
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
// - cache the anim-set node off the "A" bute key, snapshot m_38->m_1a0.m_14, apply the
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
CDroppedObject::CDroppedObject(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->ApplyName("LEVEL_OBJECTDROPPER_OBJECT");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_DROPPEDOBJECT", 0);
    m_38->m_flags |= 0x2000002;
    i32 adjY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_landY = adjY;
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = adjY - g_buteMgr.GetIntDef("Hazardz", "DroppedObjectYOffset", 0x140);
    m_fallY = static_cast<double>(m_object->m_screenY);
    if (m_object->m_sortKey != 0xcf851) {
        m_object->m_sortKey = 0xcf851;
        m_object->m_flags |= 0x20000;
    }
    m_timePerTile =
        32.0
        / static_cast<double>(
            static_cast<u32>(g_buteMgr.GetDwordDef("Hazardz", "DroppedObjectTimePerTile", 0x3e8))
        );
}

RVA(0x000c6b50, 0x15)
void CDroppedObject::RegisterRange() {
    g_dropColl.Construct(0x7d0, 0x7da);
}

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
    *reinterpret_cast<void**>(DropLookup(id)) = static_cast<void*>(&DropActA_c7090);

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
    *reinterpret_cast<void**>(DropLookup(id2)) = static_cast<void*>(&DropActB_c7be0);
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
    m_38->m_1a0.Advance(g_engineFrameDelta);
    m_fallY = static_cast<double>(static_cast<u32>(g_frameDelta)) * m_timePerTile + m_fallY;
    i32 landed = static_cast<i32>((m_fallY - g_dropFallBias));
    if (landed > m_landY) {
        i32 x = m_object->m_screenX;
        CTileGrid* g = g_gameReg->m_tileGrid;
        i32 cell;
        {
            i32 cx = x >> 5;
            i32 cy = m_landY >> 5;
            if (static_cast<u32>(cx) < static_cast<u32>(g->m_width)
                && static_cast<u32>(cy) < static_cast<u32>(g->m_height)) {
                cell = *reinterpret_cast<i32*>((reinterpret_cast<char*>(g->m_rows[cy]) + cx * 0x1c));
            } else {
                cell = 1;
            }
        }
        if ((cell & 0x900) == 0) {
            if (cell & 2) {
                if (cell == 0x40) {
                    m_38->m_flags |= 0x10000;
                } else {
                    switch (g_gameReg->m_curState->m_levelType) {
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
                                CWwdGameObjectA* s = g_gameReg->m_world->m_childGroup->CreateSprite(
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
                CWwdGameObjectA* s = g_gameReg->m_world->m_childGroup
                                     ->CreateSprite(0, x, m_landY, 0xcf84f, "Particlez", 0x40003);
                if (s != 0) {
                    s->ApplyName("GAME_WATER");
                    s->ApplyLookupGeometry("GAME_WATER", 0);
                }
            }
        }
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("LEVEL_DROPPEDOBJECTHIT", 0);
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find("B");
        g_gameReg->m_cmdGrid->CombatCue(m_object->m_screenX, m_landY, 1, 7, -1);
        return 0;
    }
    m_object->m_screenY = landed;
    return 0;
}

RVA(0x000c7350, 0x39)
i32 CDroppedObject::UserLogicVfunc5() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    if (m_38->m_1a0.m_28 != 0 && m_38->m_1a0.m_20 == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

RVA(0x000c73a0, 0xb5)
i32 CDroppedObject::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d))) {
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
// tail - cache the anim-set node off the "A" bute key, snapshot m_38->m_1a0.m_14,
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
CDroppedObjectShadow::CDroppedObjectShadow(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->ApplyName("LEVEL_OBJECTDROPPER_SHADOW");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_DROPPEDOBJECTSHADOW", 0);
    m_38->m_flags |= 0x2000002;
    m_object->m_drawFillArg = reinterpret_cast<i32>(g_gameReg->m_logicPump->m_tables[5]);
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 7;
    if (m_object->m_sortKey != 0xcf84f) {
        m_object->m_sortKey = 0xcf84f;
        m_object->m_flags |= 0x20000;
    }
}

RVA(0x000c76d0, 0x15)
void CDroppedObjectShadow::InitActReg() {
    g_shadowActReg.Construct(0x7d0, 0x7da);
}

RVA(0x000c7750, 0x102)
void CDroppedObjectShadow::FireActivation(i32 coord) {
    if ((reinterpret_cast<CShadowActEntry*>(g_shadowActReg.ResolveEntry(coord)))->m_fn != 0) {
        (this->*((reinterpret_cast<CShadowActEntry*>(g_shadowActReg.ResolveEntry(coord)))->m_fn))();
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
    (reinterpret_cast<CShadowActEntry*>(g_shadowActReg.ResolveEntry(id)))->m_fn = static_cast<i32 (CUserLogic::*)()>(&CDroppedObjectShadow::Advance);
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
    if (m_38->m_1a0.Advance(g_engineFrameDelta) == 2) {
        CWwdGameObjectA* o = m_object;
        g_gameReg->m_world->m_childGroup
            ->CreateSprite(0, o->m_screenX, o->m_screenY, 0, "DroppedObject", 0x40003);
    }
    if (m_38->m_1a0.m_28 != 0 && m_38->m_1a0.m_20 == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

RVA(0x000c7b40, 0x76)
i32 CDroppedObjectShadow::SerializeMove(CGruntArchive* ar, i32 mode, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(reinterpret_cast<CSerialArchive*>((reinterpret_cast<i32>(ar))), mode, c, d)) {
        return 0;
    }
    if (!Chain(static_cast<CSerialArchive*>(ar), mode, c, reinterpret_cast<CGameObject*>(d))) {
        return 0;
    }
    if (mode == 8) {
        i32 fill = reinterpret_cast<i32>(g_gameReg->m_logicPump->m_tables[5]);
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 7;
        o->m_drawFillArg = fill;
    }
    return 1;
}

#include <rva.h>
