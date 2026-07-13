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
//   Builder_168080::Init                   0x00168080
//
// The class definitions stay canonical (<Gruntz/GameLevel.h> / <Gruntz/UserLogic.h>);
// this TU only hosts the bodies (strictly RVA-ascending). The tile-probe macro +
// tile-code defines are duplicated file-local from GameLevel.cpp (both TUs inline
// the same retail copy-paste probe).
#include <Wwd/SubWidget168080.h> // the 0x44 sub-widget (dtor in WwdGrid.cpp)
#include <Mfc.h>
#include <Gruntz/GameLevel.h>
#include <Wap32/Object.h>     // CObject grand-base (SubWidget_168080's base)
#include <Gruntz/UserLogic.h> // canonical CGameObject (the movement target) + world chain types
#include <rva.h>

// The collision-relevant tile codes (see GameLevel.cpp for the full derivation
// note). #define, not an enum, for the same no-AST-footprint reason.
#define kTilePassable 0 // empty tile / any non-colliding code
#define kTileSoft 1     // soft-blocking (triggers the inward axis re-scan)
#define kTileSoft2 2    // soft-blocking; 0x400-flag downgradeable, and blocks a fall
#define kTileHard 3     // hard-blocking (the axis gates' `== kTileHard` stop code)
#define kTileSpecial 4  // special (folds the target's 0x400000 flag)

// LookupTile's empty-cell sentinels: 0xeeeeeeee is the uninitialized-heap fill
// (no tile placed); -1 is the explicit "clear" marker.
static const i32 TILE_UNINIT = (i32)0xeeeeeeee;
static const i32 TILE_CLEAR = -1;

// The +0x134 axis-low bracket's "unset" sentinel (INT_MIN): BroadPhase tests it
// before treating an object's AABB as a live box.
static const i32 AXIS_UNSET = (i32)0x80000000;

// The mode-1..2 sub-dispatch is CGameLevel::MoveKindDispatch12 (@0x1671c0,
// __thiscall), reconstructed below. ApplyMove's call to it reloc-masks to
// the same address regardless of convention; modeling it as this __stdcall leaf
// gives ApplyMove's surrounding code a closer byte match (94.78%) than the
// literal method-call form (92.61%) - see ApplyMove's @early-stop note.
extern "C" i32 __stdcall MoveSubDispatch12(CGameObject* obj, i32 a, i32 b, i32 c); // @0x1671c0

// PROBE_TILE - the inlined per-coord tile probe (== AxisProbe @0x161270, defined in
// GameLevel.cpp). Written as a do/while macro so each of the (up to four) copies in a
// single function schedules locally, exactly like the retail copy-paste
// (docs/patterns/x87-copypaste-vs-inline-fp-block.md). Duplicated file-local from
// GameLevel.cpp (the steppers below inline the same probe textually).
#define PROBE_TILE(LVL, X, Y, RESULT)                                                              \
    do {                                                                                           \
        i32 px_ = (X);                                                                             \
        i32 py_ = (Y);                                                                             \
        if (px_ < 0) {                                                                             \
            px_ = 0;                                                                               \
        } else {                                                                                   \
            CLevelPlane* pc_ = (LVL)->m_mainPlane;                                                 \
            if (px_ >= pc_->m_wrapW) {                                                             \
                px_ = pc_->m_wrapW - 1;                                                            \
            }                                                                                      \
        }                                                                                          \
        if (py_ < 0) {                                                                             \
            py_ = 0;                                                                               \
        } else {                                                                                   \
            CLevelPlane* pc_ = (LVL)->m_mainPlane;                                                 \
            if (py_ >= pc_->m_wrapH) {                                                             \
                py_ = pc_->m_wrapH - 1;                                                            \
            }                                                                                      \
        }                                                                                          \
        CLevelPlane* pl_ = (LVL)->m_mainPlane;                                                     \
        i32 qx_ = px_ >> pl_->m_shiftX;                                                            \
        i32 qy_ = py_ >> pl_->m_shiftY;                                                            \
        i32 col_ = qx_;                                                                            \
        i32 subX_ = px_ - (qx_ << pl_->m_shiftX);                                                  \
        i32 idx_ = pl_->m_colOffsets[qy_] + col_;                                                  \
        i32 subY_ = py_ - (qy_ << pl_->m_shiftY);                                                  \
        i32 tile_ = pl_->m_tileGrid[idx_];                                                         \
        if (tile_ == TILE_UNINIT || tile_ == TILE_CLEAR) {                                         \
            (RESULT) = kTilePassable;                                                              \
        } else {                                                                                   \
            CTileImageSet* set_ = (CTileImageSet*)m_imageSets[tile_ & 0xffff];                     \
            (RESULT) = set_->GetCollisionAt(subX_, subY_);                                         \
        }                                                                                          \
    } while (0)

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
// other party in t->m_hitOther and fires t's worker m_collideNotify; on a
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
                        fire = t->m_collideWorker->m_collideNotify(t);
                    } else {
                        fire = 1;
                    }
                    if (fire != 0) {
                        if (t->m_collMask & obj->m_collCategory) {
                            if (obj->m_collideWorker != 0) {
                                obj->m_hitOther = t;
                                obj->m_collideWorker->m_collideNotify(obj);
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
// 0x168080 - a compound-widget builder (RVA-adjacent to this TU). __thiscall(a1, rc,
// p3..p8): allocate three same-typed sub-widgets (operator new + vtable stamp), lay
// each out over the RECT *rc with its own size-pair, then derive the widget's own
// extents (min-1 + half-centers) from three more size-pairs and finish with a SetRect
// over rc's corners.
//
// ATTRIBUTION (proven, sema xref + disasm): this IS WwdPlaneRender::Init - the 0xb8-byte
// plane-render worker's init, and its ONLY caller is WwdFile::RebuildPlanes (@0x1628f0,
// src/Wwd/WwdFile.cpp). Builder_168080 (+0x00..+0x6c) is a partial view of
// WwdPlaneRender; SubWidget_168080 (0x44 B, vtable 0x5f0310) is the sub-render-object it
// allocates x3. DEFERRED fold onto WwdPlaneRender::Init: the real Init takes 8 args
// (this ret 0x20) with a RECT* + 6 Pt*, but RebuildPlanes' @early-stop reconstruction
// passes 7 args with a simplified geo mapping - folding would force rewriting
// RebuildPlanes' scrambled arg-build (risks regressing its match). Left as the placeholder
// view here (spatial home). Re-homed from src/Stub/ApiHiCallers.cpp.
extern "C" int(WINAPI* g_pSetRect_6c44b8)(RECT*, int, int, int, int);

struct Pt_168080 {
    i32 m_0; // +0x00
    i32 m_4; // +0x04
};
SIZE_UNKNOWN(Pt_168080);
// SubWidget_168080 is the shared canonical <Wwd/SubWidget168080.h> (its dtor body lives
// in the WwdGrid obj - the vtable-owner probe binds ??_7 @0x1f0310 slot 1 to 0x1682a0).
struct Builder_168080 {
    void* m_0;             // +0x00
    SubWidget_168080* m_4; // +0x04
    SubWidget_168080* m_8; // +0x08
    SubWidget_168080* m_c; // +0x0c
    i32 m_10;              // +0x10
    i32 m_14;              // +0x14
    i32 m_18;              // +0x18
    i32 m_1c;              // +0x1c
    i32 m_20;              // +0x20
    i32 m_24;              // +0x24
    i32 m_28;              // +0x28
    i32 m_2c;              // +0x2c
    i32 m_30;              // +0x30
    i32 m_34;              // +0x34
    i32 m_38;              // +0x38
    i32 m_3c;              // +0x3c
    i32 m_40;              // +0x40
    i32 m_44;              // +0x44
    i32 m_48;              // +0x48
    i32 m_4c;              // +0x4c
    i32 m_50;              // +0x50
    i32 m_54;              // +0x54
    RECT m_58;             // +0x58
    i32 m_68;              // +0x68
    i32 m_6c;              // +0x6c
    i32 Init(
        void* a1,
        RECT* rc,
        Pt_168080* p3,
        Pt_168080* p4,
        Pt_168080* p5,
        Pt_168080* p6,
        Pt_168080* p7,
        Pt_168080* p8
    );
};
SIZE_UNKNOWN(Builder_168080);
RVA(0x00168080, 0x1f6)
i32 Builder_168080::Init(
    void* a1,
    RECT* rc,
    Pt_168080* p3,
    Pt_168080* p4,
    Pt_168080* p5,
    Pt_168080* p6,
    Pt_168080* p7,
    Pt_168080* p8
) {
    if (a1) {
        m_4 = new SubWidget_168080;
        m_8 = new SubWidget_168080;
        m_c = new SubWidget_168080;
        if (m_4 && m_8 && m_c && m_4->Setup(*rc, p3->m_0, p3->m_4)
            && m_8->Setup(*rc, p4->m_0, p4->m_4) && m_c->Setup(*rc, p5->m_0, p5->m_4)) {
            m_10 = 0;
            m_14 = 0;
            m_18 = p6->m_0 - 1;
            m_1c = p6->m_4 - 1;
            m_40 = p6->m_0 / 2;
            m_44 = p6->m_4 / 2;
            m_30 = 0;
            m_34 = 0;
            m_38 = p7->m_0 - 1;
            m_3c = p7->m_4 - 1;
            m_48 = p7->m_0 / 2;
            m_4c = p7->m_4 / 2;
            m_20 = 0;
            m_24 = 0;
            m_28 = p8->m_0 - 1;
            m_2c = p8->m_4 - 1;
            m_50 = p8->m_0 / 2;
            m_54 = p8->m_4 / 2;
            m_0 = a1;
            g_pSetRect_6c44b8(&m_58, rc->left, rc->top, rc->right, rc->bottom);
            m_68 = 0xffffa932;
            m_6c = 0xffffa932;
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
