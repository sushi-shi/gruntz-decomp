// GameLevelMove.cpp - the movement/collision-resolution module.
// original TU: filename unknown (@identity-TODO - movement/collision module between
// ImageSet and WwdSpatialMgr; candidates Move.cpp/Collide.cpp)
//
// Carved out of the GameLevel god-TU by retail .text birth position: these 12
// functions form their own contiguous original TU at [0x167130 .. 0x168276]
// (bracketed by the ImageSet TU before and CWwdSpatialMgr after) - CGameLevel's
// methods span SEVERAL original files, and this is the movement/collision one:
//   ApplyMove (free __stdcall)             0x00167130
//   CGameLevel::MoveKindDispatch12         0x001671c0
//   CGameLevel::MoveStepXHi/XLo/YHi/YLo    0x00167260/450/640/830
//   CGameLevel::ResolveRightX/LeftX        0x00167a20/b40
//   CGameLevel::ResolveBottomY/TopY        0x00167c60/d80
//   CGameLevel::BroadPhase                 0x00167ea0
//   CWwdSpatialMgr::Init                   0x00168080
//
// The class definitions stay canonical (<Gruntz/GameLevel.h> / <Gruntz/UserLogic.h> /
// <Wwd/WwdSpatialMgr.h>); this TU only hosts the bodies (strictly RVA-ascending). The
// tile-probe macro + tile-code defines are duplicated file-local from GameLevel.cpp
// (both TUs inline the same retail copy-paste probe).
#include <Wwd/SubWidget168080.h> // the 0x44 grid alloc-view (transient vtable; dtor in WwdGrid.cpp)
#include <Wwd/WwdSpatialMgr.h> // canonical CWwdSpatialMgr (the plane grid/scroll worker; Init 0x168080)
#include <Gruntz/WwdGrid.h> // CWwdGrid (the grids' final type; Setup == CWwdGrid::CWwdGrid @0x1915c0)
#include <Mfc.h>
#include <Gruntz/GameLevel.h>
#include <Wap32/Object.h>     // CObject grand-base (SubWidget_168080's base)
#include <Gruntz/UserLogic.h> // canonical CGameObject (the movement target) + world chain types
#include <rva.h>

// The collision-relevant tile codes are the typed enum TileCollision in
// <Gruntz/GameLevel.h> (consolidated from here + GameLevel.cpp).

// LookupTile's empty-cell sentinels: 0xeeeeeeee is the uninitialized-heap fill
// (no tile placed); -1 is the explicit "clear" marker.

// The +0x134 axis-low bracket's "unset" sentinel (INT_MIN): BroadPhase tests it
// before treating an object's AABB as a live box.
static const i32 AXIS_UNSET = (i32)0x80000000;

// The mode-1..2 sub-dispatch is CGameLevel::MoveKindDispatch12 (@0x1671c0,
// __thiscall), reconstructed below. ApplyMove's call to it reloc-masks to
// the same address regardless of convention; modeling it as this __stdcall leaf
// gives ApplyMove's surrounding code a closer byte match (94.78%) than the
// literal method-call form (92.61%) - see ApplyMove's @early-stop note.
extern "C" i32 __stdcall MoveSubDispatch12(CGameObject* obj, i32 a, i32 b, i32 c); // @0x1671c0

// ---------------------------------------------------------------------------
// ApplyMove: drive the +0xe4 edit-state machine. editKind <= 0: nothing; kind 7
// commits the new scroll x/y directly; kinds 1..2 fan to MoveKindDispatch12. Then
// fold flag bits into the result and tag 0x400000 when the scroll did not move.
//
// @early-stop
// call-arg-materialization entropy (~95%): logic/offsets/CFG/__stdcall conv are exact;
// the only residue is the MoveKindDispatch12 call setup - retail interleaves a reload
// between the arg pushes (2 regs), MSVC pre-loads all three (eax/ecx/edx). See
// docs/patterns/pin-local-for-callee-saved-reg.md. Entropy tail; deferred.
RVA(0x00167130, 0x83)
i32 __stdcall ApplyMove(CGameObject* obj, i32 a, i32 b, i32 c) {
    CGameObject* s = obj;
    i32 eax = 0;
    i32 prevX = s->m_screenX;
    i32 prevY = s->m_screenY;
    i32 kind = s->m_moveMode;

    if (kind > 0) {
        if (kind > 2) {
            if (kind == 7) {
                s->m_screenX = a;
                s->m_screenY = b;
            }
        } else {
            eax = MoveSubDispatch12(s, a, b, c);
        }
    }

    if (eax & 0x20000) {
        eax |= 0x10000;
    }
    u32 f = s->m_flags;
    if (f & 0x400000) {
        eax |= 0x100000;
    }
    if (f & 0x10) {
        eax |= 0x200000;
    }
    if (s->m_screenX == prevX && s->m_screenY == prevY) {
        eax |= 0x400000;
    }
    return eax;
}

// ---------------------------------------------------------------------------
// MoveKindDispatch12 (@0x1671c0): drive both axes toward (x,y). For the X axis,
// if x is above/below the target's current scrollX call the matching hi/lo stepper
// (which clamps x in place through &x); same for Y; OR the two results. Finally
// commit the (possibly stepped) screen x/y (+0x5c/+0x60) back into the target and
// return the accumulated flag word. this=CGameLevel; `t` is the moved CGameObject
// (verified: CMovingLogic::Update loads m_object->owner->m_level as this and passes
// m_object as the target - see the IDENTITY note above).
RVA(0x001671c0, 0x97)
i32 CGameLevel::MoveKindDispatch12(CGameObject* t, i32 x, i32 y, i32 flags) {
    i32 result = 0;
    i32 curX = t->m_screenX;
    if (x > curX) {
        result = MoveStepXHi(t, x, y, &x, flags);
    } else if (x < curX) {
        result = MoveStepXLo(t, x, y, &x, flags);
    }
    i32 curY = t->m_screenY;
    if (y > curY) {
        result |= MoveStepYHi(t, x, y, &y, flags);
    } else if (y < curY) {
        result |= MoveStepYLo(t, x, y, &y, flags);
    }
    t->m_screenX = x;
    t->m_screenY = y;
    return result;
}

// ===========================================================================
// The four axis steppers (MoveStepXHi/XLo/YHi/YLo @0x167260/450/640/830).
// All __thiscall (this=level), ret 0x14. Each sweeps the OTHER axis across a tile
// region [t->axis brackets], inlining the per-tile probe (== AxisProbe). When a
// probed tile reports a blocking kind (1/2), it scans the resolved axis inward via
// AxisProbe to find the first clear coord, accumulating a state-flag word. Finally
// it tails into BroadPhase to test moving objects, writing the resolved scroll coord
// back through the out-pointer and returning the state word.
//   X-variants resolve X (out = &x, scroll +0x5c, step +0xfc, outer Y over +138..+140);
//   Y-variants resolve Y (out = &y, scroll +0x60, step +0xf8, outer X over +134..+13c).
//   Hi-variants fix the high edge (+0x13c / +0x140) and scan down; Lo-variants fix the
//   low edge (+0x134 / +0x138) and scan up. Field names via EditTarget (file-scope).
// ===========================================================================

// MoveStepXHi - 0x167260. Fixed X = high edge (x + axisMid); sweep Y, scan X down.
// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167260, 0x1ef)
i32 CGameLevel::MoveStepXHi(CGameObject* t, i32 x, i32 y, i32* px, i32 flags) {
    i32 xEnd = x + t->m_extentR;
    i32 yHi = t->m_extentB + y;
    i32 yLo = t->m_extentT + y;
    i32 state = 0;
    if (yLo > yHi) {
        goto helper;
    }
looptop: {
    i32 result;
    {
        i32 cx = xEnd;
        if (cx < 0) {
            cx = 0;
        } else {
            CLevelPlane* pc = m_mainPlane;
            if (cx >= pc->m_wrapW) {
                cx = pc->m_wrapW - 1;
            }
        }
        i32 cy = yLo;
        if (cy < 0) {
            cy = 0;
        } else {
            CLevelPlane* pc = m_mainPlane;
            if (cy >= pc->m_wrapH) {
                cy = pc->m_wrapH - 1;
            }
        }
        CLevelPlane* pl = m_mainPlane;
        i32 qx = cx >> pl->m_shiftX;
        i32 qy = cy >> pl->m_shiftY;
        i32 col = qx;
        i32 subX = cx - (qx << pl->m_shiftX);
        i32 idx = pl->m_colOffsets[qy] + col;
        i32 subY = cy - (qy << pl->m_shiftY);
        i32 tile = pl->m_tileGrid[idx];
        if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
            result = kTilePassable;
        } else {
            CTileImageSet* set = (CTileImageSet*)m_imageSets[tile & 0xffff];
            result = set->GetCollisionAt(subX, subY);
        }
    }
    if (result == kTileSoft2 && (t->m_flags & 0x400)) {
        result = kTilePassable;
    }
    if (result == kTileSoft || result == kTileSoft2) {
        i32 lo = t->m_screenX + t->m_extentR;
        i32 j = xEnd - 1;
        state |= 0x60000;
        for (; j > lo; j--) {
            if (AxisProbe(j, yLo) == kTilePassable) {
                j -= t->m_extentR;
                goto have_x;
            }
        }
        j = t->m_screenX;
    have_x:
        x = j;
        if (j == t->m_screenX) {
            goto done_eq;
        }
    }
    if (yLo == yHi) {
        yLo++;
    } else {
        yLo += t->m_strideY;
        if (yLo <= yHi) {
            goto looptop;
        }
        yLo = yHi;
    }
    if (yLo <= yHi) {
        goto looptop;
    }
}
helper:
    if (BroadPhase(t, x, y) != 0) {
        *px = t->m_screenX;
        return state | 0x22000000;
    }
    *px = x;
    return state;
done_eq:
    *px = t->m_screenX;
    return state;
}

// MoveStepXLo - 0x167450. Fixed X = low edge (x + axisLoA); sweep Y, scan X up.
// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167450, 0x1ef)
i32 CGameLevel::MoveStepXLo(CGameObject* t, i32 x, i32 y, i32* px, i32 flags) {
    i32 xEnd = x + t->m_extentL;
    i32 yHi = t->m_extentB + y;
    i32 yLo = t->m_extentT + y;
    i32 state = 0;
    if (yLo > yHi) {
        goto helper;
    }
looptop: {
    i32 result;
    {
        i32 cx = xEnd;
        if (cx < 0) {
            cx = 0;
        } else {
            CLevelPlane* pc = m_mainPlane;
            if (cx >= pc->m_wrapW) {
                cx = pc->m_wrapW - 1;
            }
        }
        i32 cy = yLo;
        if (cy < 0) {
            cy = 0;
        } else {
            CLevelPlane* pc = m_mainPlane;
            if (cy >= pc->m_wrapH) {
                cy = pc->m_wrapH - 1;
            }
        }
        CLevelPlane* pl = m_mainPlane;
        i32 qx = cx >> pl->m_shiftX;
        i32 qy = cy >> pl->m_shiftY;
        i32 col = qx;
        i32 subX = cx - (qx << pl->m_shiftX);
        i32 idx = pl->m_colOffsets[qy] + col;
        i32 subY = cy - (qy << pl->m_shiftY);
        i32 tile = pl->m_tileGrid[idx];
        if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
            result = kTilePassable;
        } else {
            CTileImageSet* set = (CTileImageSet*)m_imageSets[tile & 0xffff];
            result = set->GetCollisionAt(subX, subY);
        }
    }
    if (result == kTileSoft2 && (t->m_flags & 0x400)) {
        result = kTilePassable;
    }
    if (result == kTileSoft || result == kTileSoft2) {
        i32 lo = t->m_screenX + t->m_extentL;
        i32 j = xEnd + 1;
        state |= 0xa0000;
        for (; j < lo; j++) {
            if (AxisProbe(j, yLo) == kTilePassable) {
                j -= t->m_extentL;
                goto have_x;
            }
        }
        j = t->m_screenX;
    have_x:
        x = j;
        if (j == t->m_screenX) {
            goto done_eq;
        }
    }
    if (yLo == yHi) {
        yLo++;
    } else {
        yLo += t->m_strideY;
        if (yLo <= yHi) {
            goto looptop;
        }
        yLo = yHi;
    }
    if (yLo <= yHi) {
        goto looptop;
    }
}
helper:
    if (BroadPhase(t, x, y) != 0) {
        *px = t->m_screenX;
        return state | 0x82000000;
    }
    *px = x;
    return state;
done_eq:
    *px = t->m_screenX;
    return state;
}

// MoveStepYHi - 0x167640. Fixed Y = high edge (y + axisHi); sweep X, scan Y down.
// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167640, 0x1eb)
i32 CGameLevel::MoveStepYHi(CGameObject* t, i32 x, i32 y, i32* py, i32 flags) {
    i32 colHi = t->m_extentR + x;
    i32 fixedY = y + t->m_extentB;
    i32 col = t->m_extentL + x;
    i32 state = 0;
    if (col > colHi) {
        goto helper;
    }
looptop: {
    i32 result;
    {
        i32 cx = col;
        if (cx < 0) {
            cx = 0;
        } else {
            CLevelPlane* pc = m_mainPlane;
            if (cx >= pc->m_wrapW) {
                cx = pc->m_wrapW - 1;
            }
        }
        i32 cy = fixedY;
        if (cy < 0) {
            cy = 0;
        } else {
            CLevelPlane* pc = m_mainPlane;
            if (cy >= pc->m_wrapH) {
                cy = pc->m_wrapH - 1;
            }
        }
        CLevelPlane* pl = m_mainPlane;
        i32 qx = cx >> pl->m_shiftX;
        i32 qy = cy >> pl->m_shiftY;
        i32 c = qx;
        i32 subX = cx - (qx << pl->m_shiftX);
        i32 idx = pl->m_colOffsets[qy] + c;
        i32 subY = cy - (qy << pl->m_shiftY);
        i32 tile = pl->m_tileGrid[idx];
        if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
            result = kTilePassable;
        } else {
            CTileImageSet* set = (CTileImageSet*)m_imageSets[tile & 0xffff];
            result = set->GetCollisionAt(subX, subY);
        }
    }
    if (result == kTileSoft2 && (t->m_flags & 0x400)) {
        result = kTilePassable;
    }
    if (result == kTileSoft || result == kTileSoft2) {
        i32 lo = t->m_screenY + t->m_extentB;
        i32 j = fixedY - 1;
        state |= 0x1020000;
        for (; j > lo; j--) {
            if (AxisProbe(col, j) == kTilePassable) {
                j -= t->m_extentB;
                goto have_y;
            }
        }
        j = t->m_screenY;
    have_y:
        y = j;
        if (j == t->m_screenY) {
            goto done_eq;
        }
    }
    if (col == colHi) {
        col++;
    } else {
        col += t->m_strideX;
        if (col <= colHi) {
            goto looptop;
        }
        col = colHi;
    }
    if (col <= colHi) {
        goto looptop;
    }
}
helper:
    if (BroadPhase(t, x, y) != 0) {
        *py = t->m_screenY;
        return state | 0x42000000;
    }
    *py = y;
    return state;
done_eq:
    *py = t->m_screenY;
    return state;
}

// MoveStepYLo - 0x167830. Fixed Y = low edge (y + axisLoB); sweep X, scan Y up.
// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167830, 0x1eb)
i32 CGameLevel::MoveStepYLo(CGameObject* t, i32 x, i32 y, i32* py, i32 flags) {
    i32 colHi = t->m_extentR + x;
    i32 fixedY = y + t->m_extentT;
    i32 col = t->m_extentL + x;
    i32 state = 0;
    if (col > colHi) {
        goto helper;
    }
looptop: {
    i32 result;
    {
        i32 cx = col;
        if (cx < 0) {
            cx = 0;
        } else {
            CLevelPlane* pc = m_mainPlane;
            if (cx >= pc->m_wrapW) {
                cx = pc->m_wrapW - 1;
            }
        }
        i32 cy = fixedY;
        if (cy < 0) {
            cy = 0;
        } else {
            CLevelPlane* pc = m_mainPlane;
            if (cy >= pc->m_wrapH) {
                cy = pc->m_wrapH - 1;
            }
        }
        CLevelPlane* pl = m_mainPlane;
        i32 qx = cx >> pl->m_shiftX;
        i32 qy = cy >> pl->m_shiftY;
        i32 c = qx;
        i32 subX = cx - (qx << pl->m_shiftX);
        i32 idx = pl->m_colOffsets[qy] + c;
        i32 subY = cy - (qy << pl->m_shiftY);
        i32 tile = pl->m_tileGrid[idx];
        if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
            result = kTilePassable;
        } else {
            CTileImageSet* set = (CTileImageSet*)m_imageSets[tile & 0xffff];
            result = set->GetCollisionAt(subX, subY);
        }
    }
    if (result == kTileSoft2 && (t->m_flags & 0x400)) {
        result = kTilePassable;
    }
    if (result == kTileSoft || result == kTileSoft2) {
        i32 lo = t->m_screenY + t->m_extentT;
        i32 j = fixedY + 1;
        state |= 0x820000;
        for (; j < lo; j++) {
            if (AxisProbe(col, j) == kTilePassable) {
                j -= t->m_extentT;
                goto have_y;
            }
        }
        j = t->m_screenY;
    have_y:
        y = j;
        if (j == t->m_screenY) {
            goto done_eq;
        }
    }
    if (col == colHi) {
        col++;
    } else {
        col += t->m_strideX;
        if (col <= colHi) {
            goto looptop;
        }
        col = colHi;
    }
    if (col <= colHi) {
        goto looptop;
    }
}
helper:
    if (BroadPhase(t, x, y) != 0) {
        *py = t->m_screenY;
        return state | 0x12000000;
    }
    *py = y;
    return state;
done_eq:
    *py = t->m_screenY;
    return state;
}

// ===========================================================================
// The four wall-slide coordinate resolvers (@0x167a20..0x167d80). Each scans one
// axis in one direction from a desired coord toward the object's per-side extent
// limit for the nearest passable (0) tile, returning that tile's coord converted
// back to the object's screen space (coord - extent) - or the object's current
// screen coord when no passable tile is found before the limit. A tight single
// inlined-PROBE_TILE loop.
// ===========================================================================

// @early-stop
// ~93%: the `for` form fixed the exit-block order (jg loop + two distinct screenX
// exits). Residual is two byte-level codegen picks, NOT reloc: (1) the scheduler
// hoists the `x-1` init above the limit computation and materializes col via
// `lea ebx,[edx-1]` (x kept live) instead of retail's `mov ebx,[esp+N]; dec ebx`;
// (2) the PROBE_TILE Y-clamp reads m_mainPlane into eax (our cl) vs ecx (retail) -
// a free-register pick for a dead temp (the X-clamp matches at eax in both). Neither
// is source-steerable. Deferred to the final sweep.
RVA(0x00167a20, 0x11b)
i32 CGameLevel::ResolveRightX(CGameObject* t, i32 x, i32 y) {
    i32 limit = t->m_screenX + t->m_extentR;
    for (i32 col = x - 1; col > limit; col--) {
        i32 result;
        PROBE_TILE(this, col, y, result);
        if (result == kTilePassable) {
            return col - t->m_extentR;
        }
    }
    return t->m_screenX;
}

// @early-stop
// ~93%: `for`-form fixed exit-block order; residual = the x+1 init hoisted above the
// limit compute + the Y-clamp mainPlane-temp register (eax vs ecx). See ResolveRightX.
RVA(0x00167b40, 0x11b)
i32 CGameLevel::ResolveLeftX(CGameObject* t, i32 x, i32 y) {
    i32 limit = t->m_screenX + t->m_extentL;
    for (i32 col = x + 1; col < limit; col++) {
        i32 result;
        PROBE_TILE(this, col, y, result);
        if (result == kTilePassable) {
            return col - t->m_extentL;
        }
    }
    return t->m_screenX;
}

// @early-stop
// ~96.8%: `for`-form fixed exit-block order; residual = the y-1 init hoist + the
// Y-clamp mainPlane-temp register (eax vs ecx). See ResolveRightX.
RVA(0x00167c60, 0x11b)
i32 CGameLevel::ResolveBottomY(CGameObject* t, i32 x, i32 y) {
    i32 limit = t->m_screenY + t->m_extentB;
    for (i32 row = y - 1; row > limit; row--) {
        i32 result;
        PROBE_TILE(this, x, row, result);
        if (result == kTilePassable) {
            return row - t->m_extentB;
        }
    }
    return t->m_screenY;
}

// @early-stop
// ~96.8%: `for`-form fixed exit-block order; residual = the y+1 init hoist + the
// Y-clamp mainPlane-temp register (eax vs ecx). See ResolveRightX.
RVA(0x00167d80, 0x11b)
i32 CGameLevel::ResolveTopY(CGameObject* t, i32 x, i32 y) {
    i32 limit = t->m_screenY + t->m_extentT;
    for (i32 row = y + 1; row < limit; row++) {
        i32 result;
        PROBE_TILE(this, x, row, result);
        if (result == kTilePassable) {
            return row - t->m_extentT;
        }
    }
    return t->m_screenY;
}

// ===========================================================================
// BroadPhase - 0x167ea0 (__thiscall, ret 0xc). The AABB broad-phase the steppers
// tail into. `t` is the moving CGameObject; it walks the world's object chain
// (this level's m_0c owner -> m_objChain) and, for every other collision-active
// object whose category matches t's mask and whose extents are set (m_extentL !=
// sentinel), tests whether t currently overlaps it. If NOT (a separation on any
// axis) but t's CANDIDATE box (at candX, candY) WOULD overlap, it stores the
// other party in t->m_hitOther and fires t's worker m_notify; on a
// nonzero reply it fires the object's own notify (masks permitting), returns 1.
// (The `(CGameObjWorld*)m_0c` cast is language-forced: the CLoadable base stores
// the owning context as a generic i32 across the whole family.)
// ===========================================================================

// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167ea0, 0x1b9)
i32 CGameLevel::BroadPhase(CGameObject* t, i32 candX, i32 candY) {
    if (!(t->m_flags & 0x100)) {
        return 0;
    }
    CGameObjNode* node = ((CGameObjWorld*)m_0c)->m_objChain->m_list.head;
    if (node == 0) {
        return 0;
    }
    do {
        CGameObjNode* nx = node->next;
        CGameObject* obj = node->obj;
        if (obj != t && (obj->m_flags & 0x100) && (t->m_collMask & obj->m_collCategory)
            && t->m_extentL != AXIS_UNSET && obj->m_extentL != AXIS_UNSET) {
            i32 tLeft = t->m_extentL + t->m_screenX;
            i32 tBot = t->m_extentT + t->m_screenY;
            i32 tRight = t->m_screenX + t->m_extentR;
            i32 tTop = t->m_extentB + t->m_screenY;
            i32 oLeft = obj->m_screenX + obj->m_extentL;
            i32 oBot = obj->m_extentT + obj->m_screenY;
            i32 oTop = obj->m_screenY + obj->m_extentB;
            i32 oRight = obj->m_screenX + obj->m_extentR;
            if (tLeft > oRight || tRight < oLeft || tBot > oTop || tTop < oBot) {
                i32 cLeft = candX + t->m_extentL;
                i32 cRight = t->m_extentR + candX;
                i32 cBot = t->m_extentT + candY;
                i32 cTop = t->m_extentB + candY;
                if (cLeft <= oRight && cRight >= oLeft && cBot <= oTop && cTop >= oBot) {
                    i32 fire;
                    if (t->m_collideWorker != 0) {
                        t->m_hitOther = obj;
                        fire = t->m_collideWorker->m_notify(t);
                    } else {
                        fire = 1;
                    }
                    if (fire != 0) {
                        if (t->m_collMask & obj->m_collCategory) {
                            if (obj->m_collideWorker != 0) {
                                obj->m_hitOther = t;
                                obj->m_collideWorker->m_notify(obj);
                            }
                        }
                        return 1;
                    }
                }
            }
        }
        node = nx;
    } while (node != 0);
    return 0;
}

// ===========================================================================
// CWwdSpatialMgr::Init (0x168080, __thiscall, ret 0x20 = 8 args): bring up the
// 0xb8-byte plane grid/scroll worker. Allocate the three per-plane grids and
// two-phase-construct each - a raw SubWidget_168080 alloc (its inline default ctor
// stamps a transient ??_7SubWidget_168080 @0x1f0310 + zeroes m_4), then Setup ==
// CWwdGrid::CWwdGrid(rc.left, rc.top, rc.right, rc.bottom, cellW, cellH) @0x1915c0,
// which re-stamps ??_7CWwdGrid @0x1f0328 and fills the grid over `*rc` with p3/p4/p5's
// cell sizes. Then seed each grid's world rect (0,0,dim-1) + scroll origin (dim/2) from
// p6/p7/p8, the world bbox from `rc`, and park the cached scroll at -22222. `a1`->m_mgr.
//
// (Former per-TU views dissolved: `Builder_168080` was CWwdSpatialMgr [+0x00..+0x6c map
// 1:1 - m_0=m_mgr, m_4/8/c=grids, rects @0x10/0x30/0x20, origins @0x40/0x48/0x50, bbox
// @0x58, scroll @0x68]; the fake `?Init@Builder_168080@@` name also left RebuildPlanes'
// Init call unresolved. `Pt_168080` was a plain i32[2] size pair.)
//
// The grids: SubWidget_168080 is the CONCRETE grid actually allocated (its own vtable
// ??_7SubWidget_168080 @0x1f0310 implements the pure OnFound @0x168060 that the ABSTRACT
// CWwdGrid @0x1f0328 leaves __purecall). The two are byte-faithfully modeled as
// layout-sharing siblings [both :CObject - ~SubWidget @0x1682a0 does NOT emit CWwdGrid's
// 0x1f0328 base stamp, proving it is not a CWwdGrid subobject], so storing the concrete
// grid into the polymorphic CWwdGrid* field WwdSpatialMgr.cpp drives is a reinterpret of
// two layout-identical siblings - the same honest residue as ~SubWidget's FreeBuckets
// call. Setup @0x1915c0 (== CWwdGrid::CWwdGrid run as a re-init on the raw object; the
// two-phase construction is not expressible in clean MSVC5 C++ - see WwdGrid.cpp 0x191770).
//
// @early-stop
// regalloc residue (~92%, was 99.51% as the `Builder_168080` view): the view kept the
// grids in fields it re-read through a SubWidget_168080-view cast (a fake view of the
// CWwdGrid* fields); modeling the concrete grid honestly with typed locals keeps cl's
// grid pointers in registers where retail re-reads them from [this+4/8/c]. A register-
// vs-memory scheduling coin-flip on the null/Setup `this`, not source-steerable without
// re-introducing the fake-view field cast. Logic + offsets + the 8-arg ABI byte-exact.
RVA(0x00168080, 0x1f6)
i32 CWwdSpatialMgr::Init(void* a1, RECT* rc, i32* p3, i32* p4, i32* p5, i32* p6, i32* p7, i32* p8) {
    if (a1) {
        SubWidget_168080* g0 = new SubWidget_168080;
        m_grid0 = (CWwdGrid*)g0;
        SubWidget_168080* g1 = new SubWidget_168080;
        m_grid1 = (CWwdGrid*)g1;
        SubWidget_168080* g2 = new SubWidget_168080;
        m_grid2 = (CWwdGrid*)g2;
        if (g0 && g1 && g2 && g0->Setup(*rc, p3[0], p3[1]) && g1->Setup(*rc, p4[0], p4[1])
            && g2->Setup(*rc, p5[0], p5[1])) {
            m_rect0Left = 0;
            m_rect0Top = 0;
            m_rect0Right = p6[0] - 1;
            m_rect0Bottom = p6[1] - 1;
            m_org0x = p6[0] / 2;
            m_org0y = p6[1] / 2;
            m_rect1Left = 0;
            m_rect1Top = 0;
            m_rect1Right = p7[0] - 1;
            m_rect1Bottom = p7[1] - 1;
            m_org1x = p7[0] / 2;
            m_org1y = p7[1] / 2;
            m_rect2Left = 0;
            m_rect2Top = 0;
            m_rect2Right = p8[0] - 1;
            m_rect2Bottom = p8[1] - 1;
            m_org2x = p8[0] / 2;
            m_org2y = p8[1] / 2;
            m_mgr = (CWwdObjMgr*)a1;
            SetRect((RECT*)&m_bbMinX, rc->left, rc->top, rc->right, rc->bottom);
            m_scrollX = (i32)0xffffa932;
            m_scrollY = (i32)0xffffa932;
            return 1;
        }
    }
    return 0;
}

// tile-collision code names (file-local; see the block near the top of this TU).
#undef kTilePassable
#undef kTileSoft
#undef kTileSoft2
#undef kTileHard
#undef kTileSpecial
#undef PROBE_TILE
