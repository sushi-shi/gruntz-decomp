// LevelTileValidation.cpp - the level-load tile/trigger validation pass
// (C:\Proj\Gruntz). The single biggest function in the backlog (7652 B): a /GX
// EH-framed __thiscall on the level/world object that walks the level's
// tile-logic object list and, per object, recovers the object's leaf class
// (identified by the constant address sitting in its +0x7c identity sub-object's
// vtable slot +0x10) and routes it to a per-class validator.
//
// The dominant class (identity 0x401799) clamps the object's world coords to the
// tile grid, looks up the underlying tile-type id (LookupTileType, inlined), and
// on a 16-way switch over the RESOLVED tile-type id registers a
// CTileTriggerSwitchLogic via this->m_2e4 (the trigger registrar, method 0x115f60)
// with a per-case tag {1..8} + isMatch predicate, or on failure emits a
// "Bad <kind> switch at: x=%d, y=%d" diagnostic through g_gameReg. Tile-type 0x21
// runs a 3x3 neighbour scan (LookupKind tag 0x16) to re-resolve; tile-types
// 0x1e/0x1f/0x22/0x23 (toys) re-resolve through a tag-0x1a record. Two sibling
// classes (0x403bfc / 0x4037b0) repeat the scan; the remaining ~8 identities run
// short bounds/free-list/cell-grid pokes.
//
// CARCASS doctrine: only the member OFFSETS and the per-object call/branch
// STRUCTURE are load-bearing. Field names are placeholders (m_<hexoffset>);
// engine callees are external no-body fns (reloc-masked `call rel32`/virtual).
//
// @early-stop  (16.9% -> 52.7% -> 54.2%: fixed the node layout [pNext@+0/data@+8], the
// 2-load direct-field identity read [was a wrong 3-load vtable model], the grid
// path [m_playMgr->m_24, was renderer->m_10], and rebuilt the switch to dispatch
// on the RESOLVED tile-type with all 8 arms + the 0x21 neighbour scan + the toy
// re-resolve, so ~half the body now aligns; the TriggerRegistrar/
// PlayfieldMgr views dissolved and the 0x401b09/0x40288d/0x402a68/0x4019bf arms
// re-targeted to their thunk-proven callees.) Residual is the documented big /GX
// megafunction wall: (a) a 4-byte frame-size delta (0x38 vs retail 0x34) shifts
// every [esp+X] spill slot; (b) retail homes `this` in edx and spills/reloads it
// where our cl keeps it enregistered - a systematic register rename across 2358
// instructions; (c) the physical switch-arm ordering + rect-copy scheduling + the
// EH-state numbering across this size are not steerable from C source
// (docs/patterns/INDEX.md, topic:wall; big-seh-fuzzy-desync.md).

#include <Mfc.h> // CString (Format / ctor / dtor), RECT
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TileTriggerContainer.h> // CTileTriggerContainer (m_beginMarker: AddSwitchLogic/
#include <Gruntz/TileTriggerLogic.h> // CTileTriggerLogic/CGiantRockLogic (the FindInLists12 hits)
#include <Gruntz/StatusBarMgr.h>     // CStatusBarMgr::InsertPtr @0x108410 (m_guts)

#include <Gruntz/ChatBoxOwner.h>      // CChatBoxOwner (this->m_2e0; Configure @0x20530)
#include <Gruntz/Play.h>              // ::CPlay - the REAL class of this TU's CLevelValidator view
#include <Gruntz/GruntzMgr.h>         // ::CGruntzMgr - the RTTI-true *0x24556c singleton AND
#include <Gruntz/UserLogic.h>         // CGameObject + AnimWorkerObj - the objects being validated
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup + CDDrawGroupNode - the live-object list
#include <Gruntz/GameLevel.h> // CGameLevel - the +0x24 level (image sets @+0x48, plane @+0x5c)
#include <Gruntz/ImageSets.h> // CImageSet1 - the tile-attrib class (GetCollisionAt, slot 8)
#include <Wwd/WwdFile.h>      // CDDrawWorkerHost - the canonical plane (tile grid + transform)
#include <rva.h>

#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540

static char s_BadSwitch[] = "Bad switch at: x=%d, y=%d\n";
static char s_BadMulti[] = "Bad multi switch at: x=%d, y=%d\n";

static char s_CouldNotAdd[] = "Could not add Grunt: Player=%d, x=%d, y=%d";

static inline CGameLevel* LevelOf(CDDrawSurfaceMgr* holder) {
    return holder->m_level;
}

static inline i32 LookupTileType(CGameLevel* level, i32 x, i32 y) {
    CDDrawWorkerHost* g = level->m_mainPlane; // +0x5c (CDDrawWorkerHost == CDDrawWorkerHost)
    if (x < 0) {
        x = 0;
    } else if (x >= g->m_wrapW) {
        x = g->m_wrapW - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= g->m_wrapH) {
        y = g->m_wrapH - 1;
    }
    i32 tx = x >> g->m_shiftX;
    i32 ty = y >> g->m_shiftY;
    i32 subX = x - (tx << g->m_shiftX);
    i32 subY = y - (ty << g->m_shiftY);
    i32 cell = g->GetTileHandle(tx, ty);
    if (cell == static_cast<i32>(0xeeeeeeee) || cell == -1) {
        return 0;
    }
    // +0x4c is m_imageSets' data pointer (the CObArray sits at +0x48). The array holds the
    // CImageSet1/2/3 variants; all three declare GetCollisionAt at slot 8, so the dispatch
    // (`call [eax+0x20]`) is the same whichever kind the cell names.
    CImageSet1* tc = static_cast<CImageSet1*>(level->m_imageSets.GetAt(cell & 0xffff));
    return tc->GetCollisionAt(subX, subY);
}

// ===========================================================================
// CPlay::PlaceStartGruntz  (0x0d2b20) - /GX EH method. Walk the placed-
// object list (renderer+0x10): for each "start grunt" object (identity 0x4024a5)
// PlaceObject it into the trigger grid at its snapped tile centre, OR'ing in the
// 0x10000 "placed" flag; on PlaceObject failure build a "Could not add Grunt..."
// CString and log it through g_gameReg, returning 0. For "player marker" objects
// (identity 0x4017e4) in a non-active game, EnqueueSingle one op per player up to
// the player's start cap (g_gameReg+0x150[magic].m_228). ret 1.
// ===========================================================================
// @early-stop
// /GX-EH + regalloc wall (~65%): every instruction/branch and the reloc-masked externs
// (PlaceObject/EnqueueSingle/LogTileError/CString Format+ctor+dtor) reproduce, incl. the
// two vacuous address-null guards and the stride-568 player-slot `lea` - but retail homes
// `this` in ebp (re-reading m_gameReg + spilling counter/flag14 to the EH frame) while our cl
// keeps `this` in ecx and enregisters counter/flag14, a systematic register rename across
// the whole body. Same EH/regalloc wall ValidateLevelTiles (this TU) hits. topic:wall.
RVA(0x000d2b20, 0x21f)
i32 CPlay::PlaceStartGruntz() {
    // Retail lea's the live-object CObList embedded at manager+0x10 (the real m_list
    // member) and null-tests it (a vacuous guard), then reads its head (+4).
    CObList* list = &m_world->m_childGroup->m_list;
    if (list == 0) {
        return 0;
    }
    CGruntzMgr* reg = m_mgr;
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(list->GetHeadPosition());
    i32 result = 1;
    i32 counter = 0;
    i32 flag14 = 0;
    if (reg->m_134 == 1) {
        flag14 = 1;
    }
    if (node == 0) {
        return result;
    }
    do {
        CGameObject* obj = node->m_obj;
        CDDrawGroupNode* next = node->m_next;
        if (obj != 0) {
            AnimWorkerObj* aux = obj->m_7c;
            void* who = static_cast<void*>(aux->m_notify); // +0x10: WHICH leaf class built this object
            if (who == reinterpret_cast<void*>(0x4024a5)) {
                i32 idx = reg->m_cmdGrid->PlaceObject(
                    obj->m_124,
                    (obj->m_screenX & ~0x1f) + 0x10,
                    (obj->m_screenY & ~0x1f) + 0x10,
                    100000,
                    flag14,
                    obj->m_114,
                    obj->m_11c,
                    obj->m_120,
                    obj->m_118,
                    obj->m_12c,
                    aux->m_2c,
                    aux->m_30,
                    reinterpret_cast<i32>(&obj->m_extent.left)
                );
                if (idx == -1) {
                    CString s;
                    s.Format(
                        s_CouldNotAdd,
                        obj->m_124,
                        (obj->m_screenX & ~0x1f) + 0x10,
                        (obj->m_screenY & ~0x1f) + 0x10
                    );
                    g_gameReg->EnterModalUI(static_cast<const char*>(static_cast<LPCSTR>(s))); // 0x8ef10
                    return 0;
                }
                obj->m_flags |= 0x10000;
            } else if (g_gameReg->m_134 != 1 && who == reinterpret_cast<void*>(0x4017e4)
                       && obj->m_124 == g_tileKindMagic) {
                // The per-player start record: m_options[k] (+0x150, stride 0x238); +0x228 is
                // the cap on start gruntz for that player (the roster's m_comboSel field).
                GruntzPlayer* e = &g_gameReg->m_options[g_tileKindMagic];
                if (e != 0 && counter < e->m_comboSel) {
                    reg->m_cmdSubMgr->EnqueueSingle(
                        result,
                        static_cast<char>(obj->m_124),
                        0,
                        0,
                        (obj->m_screenX & ~0x1f) + 0x10,
                        (obj->m_screenY & ~0x1f) + 0x10,
                        0,
                        0
                    );
                    counter++;
                }
            }
        }
        node = next;
    } while (node != 0);
    return result;
}

RVA(0x000d2dd0, 0x1de4)
i32 CPlay::ValidateLevelTiles() {
    i32 validCount = 0; // [esp+0x10]  count of objects validated
    i32 counts[4];      // per-kind pressure-pad tallies (0x40164f arm)
    counts[0] = 0;
    counts[1] = 0;
    counts[2] = 0;
    counts[3] = 0;

    // The live-object list (the CObList embedded at manager+0x10; head node 191880x14).
    CObList* list = &m_world->m_childGroup->m_list;
    if (list == 0) {
        return 0;
    }
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(list->GetHeadPosition());
    if (node == 0) {
        return 1;
    }

    i32 ok = 1;
    do {
        CGameObject* obj = node->m_obj; // GetNext: data @+0x08
        node = node->m_next;                //          pNext @+0x00
        if (obj == 0) {
            continue;
        }

        // 2-load leaf identity: [obj+0x7c] -> +0x10 == AnimWorkerObj::Init (the per-leaf
        // post-create driver fn-ptr). The constants are those leaves' Init thunks.
        void* who = static_cast<void*>(obj->m_7c->m_notify);

        if (who == reinterpret_cast<void*>(0x401799)) {
            CGameLevel* grid = LevelOf(m_world); // recomputed per-arm (retail spills it)
            i32 type = LookupTileType(LevelOf(m_world), obj->m_screenX, obj->m_screenY);
            if (type == 0x21) {
                // 3x3 neighbour scan around the tile: find an adjacent registered
                // switch (LookupKind tag 0x16), then re-resolve the tile-class type
                // at the hit cell (rel index into hit+0xac).
                void* hit = 0;
                i32 col = obj->m_164 - 1;
                i32 colOff = col << 8;
                i32 row = obj->m_168 - 1;
                while (col < obj->m_164 + 2) {
                    row = obj->m_168 - 1;
                    if (hit != 0) {
                        break;
                    }
                    while (row < obj->m_168 + 2) {
                        void* r = m_beginMarker->FindInLists12(row + colOff, TRIGID_GIANT_ROCK_22);
                        if (r != 0) {
                            hit = r;
                        }
                        if (hit != 0) {
                            break;
                        }
                        row++;
                    }
                    if (hit != 0) {
                        break;
                    }
                    col++;
                    colOff += 0x100;
                }
                if (hit == 0) {
                    return 0;
                }
                i32 rel = (obj->m_168 - row) * 3 - col + obj->m_164;
                // the tag-0x16 hit IS a CGiantRockLogic; +0xac = m_matrix[rel + 4]
                i32 tcidx = (static_cast<CGiantRockLogic*>(hit))->m_matrix[rel + 4];
                if (tcidx == 0) {
                    return 0;
                }
                type = (static_cast<CImageSet1*>(grid->m_imageSets.GetAt(tcidx)))->GetCollisionAt(0, 0);
            }
            if (type == 0x1e || type == 0x1f || type == 0x22 || type == 0x23) {
                // toy tile: re-resolve the underlying tile-class type through the
                // trigger registrar's tag-0x1a record (+0x34 = tile-class index).
                CTileTriggerLogic* r = m_beginMarker->FindInLists12(obj->m_id, TRIGID_COVERED_POWERUP_26);
                if (r == 0) {
                    return 0;
                }
                i32 tcidx = r->m_tileToken;
                if (tcidx == 0) {
                    return 0;
                }
                type = (static_cast<CImageSet1*>(grid->m_imageSets.GetAt(tcidx)))->GetCollisionAt(0, 0);
            }
            switch (type - 0x33) {
                case 4: // 0x37
                case 5: // 0x38
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_MULTI_SWITCH_3,
                            obj->m_164,
                            obj->m_168,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_7c->m_switchRectA,
                            obj->m_7c->m_switchRectB,
                            type == 0x38,
                            obj->m_120,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadSwitch, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case 8: // 0x3b
                case 9: // 0x3c
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_EXCLUSIVE_SWITCH_4,
                            obj->m_164,
                            obj->m_168,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_7c->m_switchRectA,
                            obj->m_7c->m_switchRectB,
                            type == 0x3c,
                            obj->m_120,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadSwitch, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case 0xa: // 0x3d (retail also increments (*0x64556c)->m_7c->+0x3c here)
                case 0xb: // 0x3e
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_SECRET_SWITCH_6,
                            obj->m_164,
                            obj->m_168,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_7c->m_switchRectA,
                            obj->m_7c->m_switchRectB,
                            type == 0x3e,
                            obj->m_120,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadSwitch, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case 0xc: // 0x3f
                case 0xd: // 0x40
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_TIME_SWITCH_7,
                            obj->m_164,
                            obj->m_168,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_7c->m_switchRectA,
                            obj->m_7c->m_switchRectB,
                            type == 0x40,
                            obj->m_120,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadSwitch, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case 0xe: // 0x41
                case 0xf: // 0x42
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_CHECKPOINT_SWITCH_8,
                            obj->m_164,
                            obj->m_168,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_7c->m_switchRectA,
                            obj->m_7c->m_switchRectB,
                            type == 0x42,
                            obj->m_120,
                            obj->m_124
                        )) {
                        CString s;
                        s.Format(s_BadSwitch, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case 0: // 0x33
                case 1: // 0x34
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_SWITCH_1,
                            obj->m_164,
                            obj->m_168,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_7c->m_switchRectA,
                            obj->m_7c->m_switchRectB,
                            type == 0x34 || type == 0x36 || type == 0x3a || type == 0x3e,
                            obj->m_120,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadMulti, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case 2: // 0x35
                case 3: // 0x36
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_SWITCH_2,
                            obj->m_164,
                            obj->m_168,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_7c->m_switchRectA,
                            obj->m_7c->m_switchRectB,
                            type == 0x34 || type == 0x36 || type == 0x3a || type == 0x3e,
                            obj->m_120,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadMulti, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case 6: // 0x39
                case 7: // 0x3a
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_SWITCH_5,
                            obj->m_164,
                            obj->m_168,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_7c->m_switchRectA,
                            obj->m_7c->m_switchRectB,
                            type == 0x34 || type == 0x36 || type == 0x3a || type == 0x3e,
                            obj->m_120,
                            0
                        )) {
                        CString s;
                        s.Format(s_BadMulti, obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                default:
                    break;
            }
        } else if (who == reinterpret_cast<void*>(0x403bfc)) {
            i32 type = LookupTileType(LevelOf(m_world), obj->m_screenX, obj->m_screenY);
            static_cast<void>(type);
            obj->m_flags |= 0x10000;
        } else if (who == reinterpret_cast<void*>(0x4037b0)) {
            i32 type = LookupTileType(LevelOf(m_world), obj->m_screenX, obj->m_screenY);
            static_cast<void>(type);
            obj->m_flags |= 0x10000;
        } else if (who == reinterpret_cast<void*>(0x401b09)) {
            // seed the on-screen level timer from the marker's (min,sec) pair; the
            // callee is ?SetTime@CTimer@@ @0x9c090 (thunk 0x2de7) on m_frameMarker,
            // gated on the manager's m_134 mode (retail: cmp [this->m_4+0x134],2).
            if (m_frameMarker != 0 && m_mgr->m_134 != 2 && g_gameReg->m_isEasyMode != 0
                && g_gameReg->m_134 == ok) {
                i32 a = obj->m_118;
                i32 b = obj->m_114;
                a += a;
                b += b;
                if (a > 0x3b) {
                    b++;
                    a -= 0x3c;
                }
                m_frameMarker->SetTime(b, a);
            }
            obj->m_flags |= 0x10000;
        } else if (who == reinterpret_cast<void*>(0x40288d)) {
            if (obj->m_124 == 0x32) {
                // ?InsertPtr@CStatusBarMgr@@ @0x108410 (thunk 0x1d2f) on m_guts
                m_guts->InsertPtr(obj->m_118, obj->m_114);
            }
        } else if (who == reinterpret_cast<void*>(0x4017e4)) {
            if (obj->m_124 == g_tileKindMagic) {
                CoordPoolNode* cell = g_coordPool.m_freeHead;
                void* slot = 0;
                if (cell->m_next != 0) {
                    slot = &cell->m_coord;
                    g_coordPool.m_freeHead = cell->m_next;
                }
                if (slot != 0) {
                    (static_cast<i32*>(slot))[0] = (obj->m_screenX & ~0x1f) + 0x10;
                    (static_cast<i32*>(slot))[1] = (obj->m_screenY & ~0x1f) + 0x10;
                }
            }
        } else if (who == reinterpret_cast<void*>(0x4019bf)) {
            // resolve the raw tile handle at the object's grid cell (inlined
            // GetTileHandle - no collision query); tile ids 0x12f..0x149 register
            // a tile-action event with the extent rect (AddToList3 @0x116a40,
            // thunk 0x3580, on m_beginMarker - the ex-"SpawnPuddle" alias).
            CDDrawWorkerHost* pl = m_world->m_level->m_mainPlane;
            i32 tile = pl->m_tileGrid[pl->m_colOffsets[obj->m_168] + obj->m_164];
            if (tile >= 0x12f && tile <= 0x149) {
                if (m_beginMarker->AddToList3(
                        tile,
                        obj->m_164,
                        obj->m_168,
                        obj->m_id,
                        obj->m_extent.left,
                        obj->m_extent.top,
                        obj->m_extent.right,
                        obj->m_extent.bottom
                    )
                    != 0) {
                    validCount++;
                    obj->m_flags |= 0x10000;
                }
            }
        } else if (who == reinterpret_cast<void*>(0x402a68)) {
            // ?PlacePuddle@CTriggerMgr@@ @0x7a240 (thunk 0x35fd) on the command grid
            m_mgr->m_cmdGrid->PlacePuddle(obj, 0);
        } else if (who == reinterpret_cast<void*>(0x40164f)) {
            // 3x3 coarse-grid pressure-pad stamp into g_gameReg->m_tileGrid: for each
            // of the 3 rows and 3 columns around the object's coarse cell, bounds-
            // check against the registry grid, tally the per-kind counter, and OR
            // a per-kind flag bit (0x100000 << kind) into the grid cell.
            i32 col = obj->m_screenX >> 5;
            i32 rowBase = obj->m_screenY >> 5;
            i32 stride = (col << 3) - col; // col*7
            i32 ebp = stride * 4 - 0x1c;
            for (i32 dy = -1; dy < 2; dy++, ebp += 0x1c) {
                i32 row = rowBase;
                i32 ofs = rowBase * 4 - 4;
                for (i32 k = 3; k != 0; k--, ofs += 4, row++) {
                    i32 gx = dy + col;
                    i32 gyy = row - 1;
                    CGruntzMapMgr* gg = g_gameReg->m_tileGrid;
                    if (static_cast<u32>(gx) >= gg->m_width || static_cast<u32>(gyy) >= gg->m_height) {
                        continue;
                    }
                    i32 kind = obj->m_124;
                    i32 bit;
                    if (static_cast<u32>(kind) > 3) {
                        bit = 0; // fall through with last ebx (matches retail)
                    } else {
                        switch (kind) {
                            case 0:
                                bit = 0x100000;
                                break;
                            case 1:
                                bit = 0x200000;
                                break;
                            case 2:
                                bit = 0x400000;
                                break;
                            default:
                                bit = 0x800000;
                                break;
                        }
                    }
                    counts[kind]++;
                    gg = g_gameReg->m_tileGrid;
                    if (static_cast<u32>(gx) >= gg->m_width || static_cast<u32>(gyy) >= gg->m_height) {
                        continue;
                    }
                    i32* cellRow = reinterpret_cast<i32*>((reinterpret_cast<char*>(gg->m_rows[0]) + ofs));
                    *reinterpret_cast<i32*>((reinterpret_cast<char*>(cellRow) + ebp)) |= bit;
                }
            }
        } else if (who == reinterpret_cast<void*>(0x40182a)) {
            CGruntzMapMgr* gg = g_gameReg->m_tileGrid;
            i32 cy = obj->m_screenX >> 5;
            i32 cx = obj->m_screenY >> 5;
            if (static_cast<u32>(cy) < gg->m_width && static_cast<u32>(cx) < gg->m_height) {
                // poke the cell
            }
        } else if (who == reinterpret_cast<void*>(0x401f0a)) {
            if (g_gameReg->m_134 != ok) {
                CoordPoolNode* cell = g_coordPool.m_freeHead;
                if (cell->m_next != 0) {
                    g_coordPool.m_freeHead = cell->m_next;
                }
            }
        }
    } while (node != 0);

    return ok;
}

// ===========================================================================
// CPlay::PositionBridgeToggle (0x0d5b20) - position the game-timer HUD widget at a
// fixed inset from the screen size, with the toggle mode and the X inset selected by
// `mode` (0 / 1 / other). If the timer (+0x3f4) is null, only the mode is set. Then,
// if a goal object is active (m_4->m_cmdGrid->m_goal), flag it released, detach it,
// and run the timeline goal-tail.
//
// CLevelValidator IS CPlay:
//   - CPlay::ResetPlayState @0x0d60b0 calls the sibling PlaceStartGruntz with
//     `mov ecx,esi`, the SAME `this` it writes at +0x4f8 (`gruntz sema xref --tree`);
//   - both views type +0x2e0 as CChatBoxOwner*, +0x2dc as the guts/UI subsystem;
//   - <Gruntz/Play.h> ALREADY claimed this very RVA on CPlay, under the fake name
//     `SetState(cur, prev)` - declared-only, so SBI_RectOnly's call emitted
//     ?SetState@CPlay@@QAEHHH@Z, a symbol NOTHING defines (unbound reloc -> link fail).
// The former per-TU views it used are all fake views of classes we already model, so the
// body is now CAST-FREE on its own members:
//   LvWorld       IS ::CGruntzMgr   (+0x8c/+0x90 = m_modeW/m_modeH, +0x68 = m_cmdGrid)
//   LvBridgePoint IS ::CTimer       (its {x,y} at +0x0/+0x4 ARE CTimer::m_baseX/m_baseY -
//                                    this positions the on-screen timer, hence the name)
//   this->m_gameReg IS CState::m_4, this->m_2e0 IS CPlay::m_hitTest,
//   this->m_bridgePoint IS CPlay::m_frameMarker.
// The goal tail still walks the LvWorld view (LvTimeline IS ::CTriggerMgr, LvGoal IS
// the CWwdGameObjectA goal sprite, ex-CTmGoal - same +0x23c goal + 0x10000 "released" bit TriggerMgr.cpp:ResetAll sets);
// folding it needs GoalTail's real CTriggerMgr name, which this lane did not prove -
// left as-is so no reference is re-bound on a guess. (@identity-TODO, see report.)
// ===========================================================================
// @early-stop
// ~91%: control flow + offsets byte-identical. Residual is three documented
// codegen-idiom/regalloc nits: (a) MSVC5 emits `sub edi,K` where retail emits
// `add edi,-K` for the two X-inset decrements (non-steerable add/sub coin-flip);
// (b) the tail's m_4 reload lands in eax (ours) vs ecx (retail) - a free-list
// pick; (c) retail keeps a redundant consecutive `test;je` on the goal pointer
// that MSVC5 collapses in the nested form here (redundant-sibling-guard-retest.md;
// no intervening call to pin the flag, so de-nesting doesn't apply). Deferred.
RVA(0x000d5b20, 0xbb)
i32 CPlay::PositionBridgeToggle(i32 mode, i32) {
    CGruntzMgr* w = m_mgr;
    i32 ex = w->m_modeW;
    i32 ey = w->m_modeH;
    CTimer* pt;
    if (mode == 1) {
        m_hitTest->Configure(2);
        pt = m_frameMarker;
        if (pt == 0) {
            goto done;
        }
        ex -= 0x37;
    } else if (mode == 0) {
        m_hitTest->Configure(1);
        pt = m_frameMarker;
        if (pt == 0) {
            goto done;
        }
        ex -= 0xd7;
    } else {
        m_hitTest->Configure(3);
        pt = m_frameMarker;
        if (pt == 0) {
            goto done;
        }
        ex -= 0x37;
    }
    ey -= 0x16;
    pt->m_baseX = ex;
    pt->m_baseY = ey;
done:
    // The goal tail is now CAST-FREE on the real classes: LvWorld WAS ::CGruntzMgr, its
    // +0x68 IS ::CTriggerMgr and its +0x23c IS CTriggerMgr::m_goal. The former
    // `LvWorld::LvTimeline::GoalTail()` was a fake method on a fake nested view: it emitted
    // ?GoalTail@LvTimeline@LvWorld@@QAEXXZ, a symbol NOTHING defines (unbound -> link fail).
    // Its real target is thunk 0x3d1e -> 0x78960 == CTriggerMgr::LoadCameraSprite (which
    // lazily re-creates the "DoNothing" camera sprite into the m_goal slot this just cleared
    // - the two halves finally read as one operation).
    CTriggerMgr* g = m_mgr->m_cmdGrid;
    CWwdGameObjectA* goal = g->m_goal;
    if (goal != 0) {
        if (goal != 0) {
            goal->m_flags |= 0x10000;
            g->m_goal = 0;
        }
        m_mgr->m_cmdGrid->LoadCameraSprite();
    }
    return 1;
}
