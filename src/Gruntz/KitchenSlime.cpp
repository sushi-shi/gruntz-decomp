#include <Mfc.h> // real MFC CString (direction-name match temp; reloc-masked)
#include <Image/CImage.h> // complete CImage: the CObArray-element downcasts are static (CImage : CWapObj : CObject)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Wap32/zBitVec.h> // GetRetAddr/g_projActCache/g_retAddrBreadcrumb
#include <Io/FileMem.h>    // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Gruntz/TypeKeyColl.h>
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype (CActRegPool<CKitchenSlime>::s_table)
#include <Bute/ButeTree.h>
#include <rva.h>
#include <math.h>   // floor (0x120580) / ceil (0x120480) / fabs (inline d9 e1)
#include <string.h> // inline strcmp for the ctor's direction-name match
#include <Bute/ButeMgr.h>
#include <Gruntz/StringNode.h> // the type-name teardown slot
#include <Gruntz/UserLogic.h> // CUserLogic base (CKitchenSlime : CUserLogic) + CGameObject::ApplyName (0x150540)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor::Advance (0x15c360) - the +0x1a0 sub-object
#include <Gruntz/Sprite.h>        // CDDrawWorker (frame-data value; the looked-up direction sprite)
#include <Gruntz/GameRegistry.h>  // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/TypeNameEntry.h> // the shared type-name-registry record (CString m_name)
#include <Gruntz/SerialArchive.h> // shared CFileMemBase stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)

#include <Gruntz/TriggerMgr.h> // canonical CTriggerMgr (FindGruntAt @0x75c60, CellDispatch)

#include <Gruntz/Grunt.h>
#include <Rez/FrameClock.h> // g_frameDelta/g_engineFrameDelta (frame-clock band)

VTBL(CKitchenSlime, 0x001e750c);

#include <Gruntz/KitchenSlime.h>

DATA(0x001ea3e0)
const double g_slimeSpeedNum = 32.0;

template<> DATA(0x00246228)
CActReg CActRegPool<CKitchenSlime>::s_table(2000, 2010);

static inline CKSlimeEntry* KSlimeLookup(i32 coord) {
    return reinterpret_cast<CKSlimeEntry*>(CActRegPool<CKitchenSlime>::s_table.ResolveEntry(coord));
}

// CKitchenSlime::~CKitchenSlime @0x013100 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to the established leaf
// dtors (UserLogic.cpp 0x10ab0 / 0x11540); the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CKitchenSlime() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00013100, 0x44, ??1CKitchenSlime@@UAE@XZ)

// CKitchenSlime::CKitchenSlime @0x0b23a0 - fold the shared CUserLogic(obj) init,
// snap the bound object to the tile grid (m_posX/m_posY doubles + m_74 layer key +
// the m_tileX/m_tileY tile coords), scale the raw target tile (m_164/m_168) to pixels
// and compute the travel window (min/max of the start and target), match the
// slime's direction name (LEVEL_KITCHENSLIME_{NORTH,EAST,SOUTH,WEST}) into the
// direction id, then run LoadSprites for the first leg, bind the "A" bute node +
// cycle geometry, and clear the bound sprite's rect.
//
// @early-stop
// inline-strcmp + min/max-polarity + eh wall (docs/patterns/strcmp-eq-bool-local-setcc.md,
// zero-register-pinning.md, eh-ctor-vptr-restamp-position.md): body byte-faithful
// (the four unrolled inline-strcmp loops + CString temp EH, the min/max window
// clamp, the LoadSprites/bute/geometry tail). Residual is the strcmp result-reg
// alloc, the cmov-vs-branch min/max selection polarity, and the /GX leaf-vptr
// re-stamp position. Not source-steerable (global regalloc/EH numbering).
RVA(0x000b23a0, 0x3f8)
CKitchenSlime::CKitchenSlime(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 0x2000002;

    CWwdGameObjectA* o = m_object;
    i32 snapX = (o->m_screenX & ~0x1f) + 0x10;
    i32 snapY = (o->m_screenY & ~0x1f) + 0x10;
    o->m_screenX = snapX;
    m_posX = static_cast<double>(snapX);
    o->m_screenY = snapY;
    m_posY = static_cast<double>(snapY);
    if (o->m_sortKey != 0x13) {
        o->m_sortKey = 0x13;
        o->m_flags |= 0x20000;
    }
    m_tileY = snapY;
    m_tileX = snapX;

    o->m_164 = (o->m_164 << 5) + 0x10;
    o->m_168 = (o->m_168 << 5) + 0x10;
    if (o->m_screenX == o->m_164 && o->m_screenY == o->m_168) {
        m_38->m_flags |= 0x10000;
        return;
    }
    o->m_extent.left = (o->m_screenX < o->m_164) ? o->m_screenX : o->m_164;
    o->m_extent.right = (o->m_screenX <= o->m_164) ? o->m_164 : o->m_screenX;
    o->m_extent.top = (o->m_screenY >= o->m_168) ? o->m_168 : o->m_screenY;
    o->m_extent.bottom = (o->m_screenY <= o->m_168) ? o->m_168 : o->m_screenY;

    CWwdGameObjectA* obj38 = Anim();
    if (obj38->m_194 != 0) {
        CString name;
        name = obj38->m_194 + 0x24;
        const char* s = static_cast<LPCTSTR>(name);
        if (strcmp(s, "LEVEL_KITCHENSLIME_NORTH") == 0) {
            o->m_124 = 1;
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_EAST") == 0) {
            o->m_124 = 2;
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_SOUTH") == 0) {
            o->m_124 = 3;
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_WEST") == 0) {
            o->m_124 = 4;
        }
    }

    m_stepMag = 0;
    m_stepMagHi = 0;
    if (LoadSprites() == 0) {
        m_38->m_flags |= 0x10000;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    o->m_area.left = 0;
    o->m_area.right = 0;
    o->m_area.top = 0;
    o->m_area.bottom = 0;
}

DATA(0x0021aea8)
i32 g_typeCounter = 2000;

static inline CTypeNameEntry* TypeLookup(i32 key) {
    g_typeColl.m_grown = 0;
    if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
        return reinterpret_cast<CTypeNameEntry*>(
            (g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride)
        );
    }
    if (reinterpret_cast<i32>((static_cast<_zvec*>(&g_typeColl))->GrowTo(key, 0))) {
        return reinterpret_cast<CTypeNameEntry*>(
            (g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride)
        );
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, reinterpret_cast<i32>(item), 0xc);
    return reinterpret_cast<CTypeNameEntry*>(
        g_typeColl.m_spare
    ); // m_spare is the i32-typed slow-path slot
}

// CKitchenSlime::RegisterType @0x0b2aa0 - the level-load class registrar. Assign
// the slime class a type-id via the global bute-tree (registering its name on
// first use), record the name into the shared type-name table, then store the
// slime's activation handler (0x40180c) into the per-class activation table at
// that id. A static initializer (no `this`); same archetype as CProjectile's.
// @early-stop
// ~91%: every operation/offset/string/call is byte-correct; the residual is pure
// regalloc + induction-variable coloring - retail pins the type-id in esi (mine
// edi), reads the node count into ebp via the `ecx=cnt; eax=cnt-1; lea ebp,[eax+1]`
// count-down idiom (mine a plain --cnt), and orders the `id=key` store before the
// scratch=0. Not source-steerable (regalloc/strength-reduction wall); deferred.
RVA_COMPGEN(0x000b28a0, 0xa, _$E731296)
RVA_COMPGEN(0x000b28c0, 0x15, _$E731328)
RVA_COMPGEN(0x000b28f0, 0xe, _$E731376)
RVA_COMPGEN(0x000b2910, 0x1f, _$E731408)

RVA(0x000b2aa0, 0x18d)
void CKitchenSlime::RegisterType() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        i32 key = g_typeCounter;
        id = key;
        CTypeNameEntry* slot = TypeLookup(key);
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
    *reinterpret_cast<void**>(KSlimeLookup(id)) = static_cast<void*>(&KSlimeActivationHandler);
}

RVA(0x000b2940, 0x102)
void CKitchenSlime::FireActivation(i32 coord) {
    CKSlimeEntry* e = KSlimeLookup(coord);
    if (e->m_fn != 0) {
        CKSlimeEntry* e2 = KSlimeLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CKitchenSlime::Tick @0x0b2ca0 - the per-frame driver. Advances the anim
// sub-mgr, runs the on-screen visibility/scroll gate (unless the registry is in
// the no-scroll mode), and if the slime has reached its destination tile asks
// LoadSprites for the next leg; otherwise integrates the sub-pixel movement
// vector (m_dirX/m_dirY unit signs * the per-frame step) into m_posX/m_posY, snapping to
// the target tile on overshoot and writing the new grid position back to m_10.
// The integer scaffolding + visibility/already-arrived blocks are byte-exact.
// @early-stop
// x87 FP movement-integrator wall (docs/patterns/x87-fp-stack-schedule.md): the
// residual is a stack-slot swap (MSVC parks `step` at [esp+8] vs retail's
// [esp+0x10], swapping the per-iter temp) plus the dead x-clamp redundant-jump
// schedule. Logic byte-for-byte correct; ~95%, above the documented 60-75% range.
RVA(0x000b2ca0, 0x29c)
i32 CKitchenSlime::Tick() {
    m_38->m_1a0.Advance(static_cast<i32>(g_engineFrameDelta));

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode == 0 || reg->m_134 != 1) {
        CGameObject* lvl = Level();
        i32 outX, outY;
        CGrunt* ent = static_cast<CGrunt*>(reg->m_cmdGrid->FindGruntAt(
            lvl->m_screenX,
            lvl->m_screenY,
            &lvl->m_area,
            &outY,
            &outX,
            static_cast<RECT*>(0)
        ));
        if (ent && ent->m_gruntKind != 0x38) {
            (static_cast<CTriggerMgr*>(g_gameReg->m_cmdGrid))->CellDispatch(outY, outX, 5, -1);
        }
    }

    CGameObject* lvl = Level();
    if (lvl->m_screenX == m_tileX && lvl->m_screenY == m_tileY && LoadSprites() == 0) {
        m_38->m_flags |= 0x10000;
        return 0;
    }

    double step =
        static_cast<double>(static_cast<i64>(static_cast<u64>(static_cast<u32>(g_frameDelta))))
        * m_speed;
    double* m88d = reinterpret_cast<double*>(&m_stepMag);

    i32 newX;
    if (m_dirX > 0.0) {
        double t = (m_posX = m_posX + step);
        newX = static_cast<i32>(floor(t));
        i32 tx = m_tileX;
        *m88d = fabs(m_posX - static_cast<double>(tx));
        // The X axis never clamps (unlike Y), but retail still emits the compare
        // (a min/max fold whose result equals the input); the empty-body test
        // reproduces the cmp + m_tileX stack-spill shared with the fabs.
        if (newX > tx) {
            newX = newX;
        }
    } else if (m_dirX < 0.0) {
        double t = (m_posX = m_posX - step);
        newX = static_cast<i32>(ceil(t));
        i32 tx = m_tileX;
        *m88d = fabs(m_posX - static_cast<double>(tx));
        if (newX < tx) {
            newX = newX;
        }
    } else {
        newX = static_cast<i32>(floor(m_posX));
    }

    i32 newY;
    if (m_dirY > 0.0) {
        double t = (m_posY = m_posY + step);
        newY = static_cast<i32>(floor(t));
        i32 ty = m_tileY;
        *m88d = fabs(m_posY - static_cast<double>(ty));
        if (newY > ty) {
            Level()->m_screenX = newX;
            Level()->m_screenY = ty;
            return 0;
        }
    } else if (m_dirY < 0.0) {
        double t = (m_posY = m_posY - step);
        newY = static_cast<i32>(ceil(t));
        i32 ty = m_tileY;
        *m88d = fabs(m_posY - static_cast<double>(ty));
        if (newY < ty) {
            Level()->m_screenX = newX;
            Level()->m_screenY = ty;
            return 0;
        }
    } else {
        newY = static_cast<i32>(floor(m_posY));
    }

    Level()->m_screenX = newX;
    Level()->m_screenY = newY;
    return 0;
}

RVA(0x000b2ff0, 0x11b)
i32 CKitchenSlime::SerializeMove(CFileMemBase* stream, i32 tag, i32 c, i32 d) {
    char* B = reinterpret_cast<char*>(this);
    CFileMemBase* s = stream;
    // Written as `if (tag != 4) { if (tag == 7) Read... } else Transfer...` so
    // MSVC lays the tag-7 (Read) block physically first (cmp 4/je else; cmp 7/jne;
    // Read; jmp; else: Transfer) - the retail dispatch order.
    if (tag != 4) {
        if (tag == 7) {
            s->Read(B + 0x58, 8);
            s->Read(B + 0x60, 8);
            s->Read(B + 0x68, 8);
            s->Read(B + 0x70, 8);
            s->Read(B + 0x78, 8);
            s->Read(B + 0x80, 8);
            s->Read(B + 0x88, 8);
        }
    } else {
        s->Write(B + 0x58, 8);
        s->Write(B + 0x60, 8);
        s->Write(B + 0x68, 8);
        s->Write(B + 0x70, 8);
        s->Write(B + 0x78, 8);
        s->Write(B + 0x80, 8);
        s->Write(B + 0x88, 8);
    }
    if (CUserLogic::SerializeMove(stream, tag, c, d) == 0) {
        return 0;
    }
    return Chain(stream, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// @early-stop
// Returns int (1 on success, 0 when no walkable tile was found) - the true
// signature, needed by Tick's `LoadSprites() == 0` test. Residual is the same FP
// /jump-table stack-frame schedule wall it has carried (retail reserves 0x1c vs
// our 0x14 - an extra direction-magnitude stack temp). ~69%, logic exact.
RVA(0x000b3160, 0x339)
i32 CKitchenSlime::LoadSprites() {
    i32 savedDir = Level()->m_124;

    i32 tileX, tileY;
    i32 found = 0;
    for (i32 i = 0; i <= 4;) {
        CGameObject* lvl = Level();
        i32 sw = lvl->m_124 - 1;
        switch (sw) {
            case 0:
                tileX = m_tileX;
                tileY = m_tileY - 0x20;
                break; // north
            case 1:
                tileX = m_tileX + 0x20;
                tileY = m_tileY;
                break; // east
            case 2:
                tileX = m_tileX;
                tileY = m_tileY + 0x20;
                break; // south
            case 3:
                tileX = m_tileX - 0x20;
                tileY = m_tileY;
                break; // west
        }

        i32 gx = tileX >> 5;
        i32 gy = tileY >> 5;
        i32 tileFlags;
        CMapMgr* map = g_gameReg->m_tileGrid;
        if (static_cast<u32>(gx) >= static_cast<u32>(map->m_width)
            || static_cast<u32>(gy) >= static_cast<u32>(map->m_height)) {
            tileFlags = 1;
        } else {
            tileFlags = ((map->m_rowInts[gy]))[gx * 7];
        }

        if (tileY >= lvl->m_extent.top && tileX <= lvl->m_extent.right
            && tileY <= lvl->m_extent.bottom && tileX >= lvl->m_extent.left && !(tileFlags & 0x939)
            && !(tileFlags & 2)) {
            found = 1;
            break;
        }

        if (++i > 4) {
            return 0;
        }

        if (lvl->m_12c == 1) {
            lvl->m_124 = sw;
            if (Level()->m_124 <= 0) {
                Level()->m_124 = 4;
            }
        } else {
            lvl->m_124++;
            if (Level()->m_124 > 4) {
                Level()->m_124 = 1;
            }
        }
    }
    if (!found) {
        return 0;
    }

    m_posX = 0;
    m_posY = 0;
    i32 changed = (Level()->m_124 != savedDir);
    switch (Level()->m_124 - 1) {
        case 0: // north
            m_posY = -static_cast<double>(*(&m_stepMag));
            m_dirX = 0;
            m_dirY = 0;
            *reinterpret_cast<i32*>(&m_dirY) = 0;
            *(reinterpret_cast<i32*>(&m_dirX) + 1) = 0;
            *(reinterpret_cast<i32*>(&m_dirY) + 1) = 0xbff00000;
            if (changed) {
                Anim()->ApplyName("LEVEL_KITCHENSLIME_NORTH");
            }
            break;
        case 1: // east
            *reinterpret_cast<i32*>(&m_posX) = m_stepMag;
            *(reinterpret_cast<i32*>(&m_posX) + 1) = *((&m_stepMag) + 1);
            m_dirX = 0;
            m_dirY = 0;
            *(reinterpret_cast<i32*>(&m_dirX) + 1) = 0x3ff00000;
            *(reinterpret_cast<i32*>(&m_dirY) + 1) = 0;
            if (changed) {
                Anim()->ApplyName("LEVEL_KITCHENSLIME_EAST");
            }
            break;
        case 2: // south
            *reinterpret_cast<i32*>(&m_posY) = m_stepMag;
            *(reinterpret_cast<i32*>(&m_posY) + 1) = *((&m_stepMag) + 1);
            m_dirX = 0;
            m_dirY = 0;
            *(reinterpret_cast<i32*>(&m_dirY) + 1) = 0x3ff00000;
            *(reinterpret_cast<i32*>(&m_dirX) + 1) = 0;
            if (changed) {
                Anim()->ApplyName("LEVEL_KITCHENSLIME_SOUTH");
            }
            break;
        case 3: // west
            m_posX = -static_cast<double>(*(&m_stepMag));
            m_dirX = 0;
            m_dirY = 0;
            *(reinterpret_cast<i32*>(&m_dirX) + 1) = 0xbff00000;
            *(reinterpret_cast<i32*>(&m_dirY) + 1) = 0;
            if (changed) {
                Anim()->ApplyName("LEVEL_KITCHENSLIME_WEST");
            }
            break;
    }

    m_posX = static_cast<double>(Level()->m_screenX) + m_posX;
    m_posY = static_cast<double>(Level()->m_screenY) + m_posY;

    u32 time;
    if (Level()->m_7c->m_bc != 0) {
        time = Level()->m_7c->m_bc;
    } else {
        time = g_buteMgr.GetDwordDef("Hazardz", "KitchenSlimeTimePerTile", 1000);
    }

    m_speed = g_slimeSpeedNum / static_cast<double>(static_cast<i64>(static_cast<u64>(time)));
    m_tileX = tileX;
    m_tileY = tileY;

    // The direction sprite + first-frame cache is the CGameObject frame-cache
    // role-union: +0x194 (m_sprite) is the cached CDDrawWorker*, +0x198 (m_layer) doubles
    // as the first-frame pointer - the same union access CGameObject's own ApplyName
    // does.
    CWwdGameObjectA* player = Anim();
    CDDrawWorker* spr = player->m_sprite;
    if (changed != 0 && spr != 0) {
        if (spr->m_minIndex <= 1 && spr->m_maxIndex >= 1) {
            player->m_190 = 1;
            player->m_layer = static_cast<CImage*>(spr->m_items.GetAt(1));
            m_stepMag = 0;
            m_stepMagHi = 0;
            return 1;
        }
        player->m_190 = 1;
        player->m_layer = 0;
        m_stepMag = 0;
        m_stepMagHi = 0;
        return 1;
    }
    m_stepMag = 0;
    m_stepMagHi = 0;
    return 1;
}
