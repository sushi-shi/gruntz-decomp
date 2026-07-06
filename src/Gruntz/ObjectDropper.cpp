// ObjectDropper.cpp - the object-dropper tile-logic object (C:\Proj\Gruntz), a
// CUserLogic leaf. The /GX leaf dtor + the 1-arg ctor (shared CUserLogic(obj)
// prologue + the per-class drop setup).
#include <Gruntz/ObjectDropper.h>

#include <string.h> // inline strcmp for the direction-name match
#include <Globals.h>
#include <Gruntz/GameRegistry.h>  // canonical *0x24556c singleton + CTileGrid terrain plane
#include <Gruntz/LightFxMgr.h>    // CLightFxMgr (g_gameReg->m_logicPump @+0x78; m_tables[])
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190) + the bute manager
// (g_buteMgr.GetDwordDef 0x1721e0); declared extern so the calls reloc-mask.
extern CButeTree g_buteTree;
extern CButeMgr g_buteMgr;

// The direction-name match builds a real MFC CString temp (default-ctor 0x1b9b93,
// operator=(LPCSTR) 0x1b9e74, dtor 0x1b9cde - all reloc-masked, /GX temp-cleanup
// state). Uses the real <Mfc.h> CString (via <Gruntz/String.h>), no local view.

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
// The world/level bounds (reg->m_mgr->m_level->m_world): tile width @0x30, height @0x34.
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
// The world tile map ((DropperMap*)g_gameReg->m_68, the reused +0x68 slot's
// dropper facet): picks a random reachable destination tile in the wander box and
// returns the object it lands on. FindDest @0x475c60, __thiscall (0x32ce ILT thunk).
SIZE_UNKNOWN(DropperMap);
namespace m4 {
    struct HitGrunt;
    struct HitTileRect;
    class GruntHitMgr {
    public:
        HitGrunt* FindGruntAt(i32 x, i32 y, HitTileRect* r, i32* a, i32* b, struct tagRECT* rect);
    };
} // namespace m4
struct DropperMap {
    // FindDest IS m4::GruntHitMgr::FindGruntAt; cast at the call.
};
// One terrain-plane cell of the registry's tile grid (g_gameReg->m_tileGrid, the
// canonical CTileGrid): a 0x1c-byte (7-dword) record; dword 0 holds the flags.
// Reached as ((DropperTile*)grid->m_8[row])[col] - the authentic CTileGrid cell
// idiom (grid->m_8 is the row-pointer table, cells 0x1c B apart; see CTileGrid.h).
SIZE_UNKNOWN(DropperTile);
struct DropperTile {
    u32 m_flags; // +0x0 terrain flags (bit 1 = blocked)
    char m_pad04[0x1c - 0x4];
};

// The global game registry is the one canonical CGameRegistry singleton (RVA
// 0x24556c). The dropper reaches its own facets through the reused per-mode slots
// (authentic downcasts, see CGameRegistry.h): m_world -> DropperMgr (sprite
// factory + level/world bounds), m_68 -> DropperMap (destination probe), m_tileGrid
// -> the terrain plane (CTileGrid), m_78 -> the level selector table (i32*). The
// scroll/hazard gates are m_134 (mode discriminator) and m_isEasyMode (edit gate).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// The bound object's +0x1a0 per-frame animator (Advance_15c360 @0x55c360).
SIZE_UNKNOWN(DropperAnim);
// DropperAnim::Advance @0x15c360 IS CAniAdvanceCursor::Advance_15c360 (header-less); local decl.
class CAniAdvanceCursor {
public:
    i32 Advance_15c360(i32 clock);
};
struct DropperAnim {
    // Advance @0x15c360 IS CAniAdvanceCursor::Advance_15c360; cast at the call.
};

// The per-frame game clock (g_645588) + scroll delta (g_645584); read unsigned so
// the zero-extended 64-bit timer subtraction / the fild-qword drift conversion
// fall out. The draw-clock delta (g_6bf3bc) the anim advancer consumes. All are
// DATA-bound by other TUs; extern-only here so the loads reloc-mask.
extern "C" u32 g_645588; // 0x645588
extern "C" u32 g_645584; // 0x645584
extern "C" u32 g_6bf3bc; // 0x6bf3bc

// 1000.0 (the per-tile-time -> per-frame-speed reciprocal numerator), VA 0x5ea9f0.

// CObjectDropper::GetTypeTag (0x124a0) - vtable slot 2: the class's logic-type id
// (0x40f), the 6-byte `mov eax,<id>; ret` accessor archetype. Regular method (the
// fat CUserLogic base slot 2 carries a placeholder signature; the leaf vtable is
// not a diffed symbol, so a plain method reproduces the slot bytes exactly).
RVA(0x000124a0, 0x6)
LogicTypeId CObjectDropper::GetTypeTag() {
    return LOGIC_OBJECTDROPPER; // 0x40f
}

// CObjectDropper::~CObjectDropper (0x124f0) - the /GX leaf dtor folds the bare
// CUserLogic teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the
// +0x18 link (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr
// (0x5e70b4). The leaf vptr store is dead-eliminated.
RVA(0x000124f0, 0x44)
CObjectDropper::~CObjectDropper() {}

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
            DropperFound* found = (DropperFound*)((m4::GruntHitMgr*)g_gameReg->m_cmdGrid)
                                      ->FindGruntAt(
                                          o->m_screenX,
                                          o->m_screenY,
                                          (m4::HitTileRect*)&o->m_144,
                                          &tx,
                                          &ty,
                                          (struct tagRECT*)&box
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
