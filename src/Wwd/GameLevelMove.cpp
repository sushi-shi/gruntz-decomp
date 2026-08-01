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
//   CWwdGridShell::OnFound                 0x00168060
//   CWwdSpatialMgr::Init                   0x00168080
//
// The class definitions stay canonical (<Gruntz/GameLevel.h> / <Gruntz/UserLogic.h> /
// <Wwd/WwdSpatialMgr.h>); this TU only hosts the bodies (strictly RVA-ascending). The
// tile-probe macro + tile-code defines are duplicated file-local from GameLevel.cpp
// (both TUs inline the same retail copy-paste probe).
#include <Wwd/WwdGridShell.h> // concrete 0x44 grid; dtor lives in WwdGrid.cpp
#include <Wwd/WwdSpatialMgr.h> // canonical CWwdSpatialMgr (the plane grid/scroll worker; Init 0x168080)
#include <Gruntz/WwdGrid.h> // abstract grid base and Setup implementation
#include <Mfc.h>
#include <Gruntz/GameLevel.h>
#include <Wap32/Object.h>             // CObject grand-base (CWwdGridShell's base)
#include <Gruntz/UserLogic.h>         // canonical CGameObject (the movement target)
#include <DDrawMgr/DDrawSurfaceMgr.h> // the m_0c world root (the chain owner)
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup (the object chain)
#include <rva.h>

VTBL(CWwdGridShell, 0x001f0310); // ??_7CWwdGridShell (was g_subVtbl_5f0310)
static const i32 AXIS_UNSET = static_cast<i32>(0x80000000);

// The mode-1..2 sub-dispatch is CGameLevel::MoveKindDispatch12 (@0x1671c0,
// __thiscall), reconstructed below. ApplyMove's call to it reloc-masks to
// the same address regardless of convention; modeling it as this __stdcall leaf
// gives ApplyMove's surrounding code a closer byte match (94.78%) than the
// literal method-call form (92.61%) - see ApplyMove's @early-stop note.

// ---------------------------------------------------------------------------
// ApplyMove: drive the +0xe4 edit-state machine. editKind <= 0: nothing; kind 7
// commits the new scroll x/y directly; kinds 1..2 fan to MoveKindDispatch12. Then
// fold flag bits into the result and tag 0x400000 when the scroll did not move.
//
// @early-stop
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

// MoveStepXHi - 0x167260. Fixed X = high edge (x + axisMid); sweep Y, scan X down.
// The sweep is a natural `while (cur <= limit)`, not the `if (>) goto helper; looptop:`
// goto skeleton: the goto form put the loop-exit on the FALLTHROUGH and emitted
// `jg helper / jmp looptop` where retail has `jle looptop` and falls through to the
// helper (jcc_sieve POLARITY #17, all four of these). The equal-coord early return is
// spelled inline for the same reason - the `goto done_eq` label got laid out ahead of
// the helper block. Branch sequences agree now.
// @early-stop
RVA(0x00167260, 0x1ef)
i32 CGameLevel::MoveStepXHi(CGameObject* t, i32 x, i32 y, i32* px, i32 flags) {
    i32 xEnd = x + t->m_extent.right;
    i32 yHi = t->m_extent.bottom + y;
    i32 yLo = t->m_extent.top + y;
    i32 state = 0;
    while (yLo <= yHi) {
        i32 result;
        {
            i32 cx = xEnd;
            if (cx < 0) {
                cx = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cx >= pc->m_wrapW) {
                    cx = pc->m_wrapW - 1;
                }
            }
            i32 cy = yLo;
            if (cy < 0) {
                cy = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cy >= pc->m_wrapH) {
                    cy = pc->m_wrapH - 1;
                }
            }
            CDDrawWorkerHost* pl = m_mainPlane;
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
                CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
                result = set->GetCollisionAt(subX, subY);
            }
        }
        if (result == kTileSoft2 && (t->m_flags & 0x400)) {
            result = kTilePassable;
        }
        if (result == kTileSoft || result == kTileSoft2) {
            i32 lo = t->m_screenX + t->m_extent.right;
            i32 j = xEnd - 1;
            state |= 0x60000;
            for (; j > lo; j--) {
                if (AxisProbe(j, yLo) == kTilePassable) {
                    j -= t->m_extent.right;
                    goto have_x;
                }
            }
            j = t->m_screenX;
        have_x:
            x = j;
            if (j == t->m_screenX) {
                *px = t->m_screenX;
                return state;
            }
        }
        if (yLo == yHi) {
            yLo++;
        } else {
            yLo += t->m_strideY;
            if (yLo > yHi) {
                yLo = yHi;
            }
        }
    }
    if (BroadPhase(t, x, y) != 0) {
        *px = t->m_screenX;
        return state | 0x22000000;
    }
    *px = x;
    return state;
}

// MoveStepXLo - 0x167450. Fixed X = low edge (x + axisLoA); sweep Y, scan X up.
// The sweep is a natural `while (cur <= limit)`, not the `if (>) goto helper; looptop:`
// goto skeleton: the goto form put the loop-exit on the FALLTHROUGH and emitted
// `jg helper / jmp looptop` where retail has `jle looptop` and falls through to the
// helper (jcc_sieve POLARITY #17, all four of these). The equal-coord early return is
// spelled inline for the same reason - the `goto done_eq` label got laid out ahead of
// the helper block. Branch sequences agree now.
// @early-stop
RVA(0x00167450, 0x1ef)
i32 CGameLevel::MoveStepXLo(CGameObject* t, i32 x, i32 y, i32* px, i32 flags) {
    i32 xEnd = x + t->m_extent.left;
    i32 yHi = t->m_extent.bottom + y;
    i32 yLo = t->m_extent.top + y;
    i32 state = 0;
    while (yLo <= yHi) {
        i32 result;
        {
            i32 cx = xEnd;
            if (cx < 0) {
                cx = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cx >= pc->m_wrapW) {
                    cx = pc->m_wrapW - 1;
                }
            }
            i32 cy = yLo;
            if (cy < 0) {
                cy = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cy >= pc->m_wrapH) {
                    cy = pc->m_wrapH - 1;
                }
            }
            CDDrawWorkerHost* pl = m_mainPlane;
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
                CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
                result = set->GetCollisionAt(subX, subY);
            }
        }
        if (result == kTileSoft2 && (t->m_flags & 0x400)) {
            result = kTilePassable;
        }
        if (result == kTileSoft || result == kTileSoft2) {
            i32 lo = t->m_screenX + t->m_extent.left;
            i32 j = xEnd + 1;
            state |= 0xa0000;
            for (; j < lo; j++) {
                if (AxisProbe(j, yLo) == kTilePassable) {
                    j -= t->m_extent.left;
                    goto have_x;
                }
            }
            j = t->m_screenX;
        have_x:
            x = j;
            if (j == t->m_screenX) {
                *px = t->m_screenX;
                return state;
            }
        }
        if (yLo == yHi) {
            yLo++;
        } else {
            yLo += t->m_strideY;
            if (yLo > yHi) {
                yLo = yHi;
            }
        }
    }
    if (BroadPhase(t, x, y) != 0) {
        *px = t->m_screenX;
        return state | 0x82000000;
    }
    *px = x;
    return state;
}

// MoveStepYHi - 0x167640. Fixed Y = high edge (y + axisHi); sweep X, scan Y down.
// The sweep is a natural `while (cur <= limit)`, not the `if (>) goto helper; looptop:`
// goto skeleton: the goto form put the loop-exit on the FALLTHROUGH and emitted
// `jg helper / jmp looptop` where retail has `jle looptop` and falls through to the
// helper (jcc_sieve POLARITY #17, all four of these). The equal-coord early return is
// spelled inline for the same reason - the `goto done_eq` label got laid out ahead of
// the helper block. Branch sequences agree now.
// @early-stop
RVA(0x00167640, 0x1eb)
i32 CGameLevel::MoveStepYHi(CGameObject* t, i32 x, i32 y, i32* py, i32 flags) {
    i32 colHi = t->m_extent.right + x;
    i32 fixedY = y + t->m_extent.bottom;
    i32 col = t->m_extent.left + x;
    i32 state = 0;
    while (col <= colHi) {
        i32 result;
        {
            i32 cx = col;
            if (cx < 0) {
                cx = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cx >= pc->m_wrapW) {
                    cx = pc->m_wrapW - 1;
                }
            }
            i32 cy = fixedY;
            if (cy < 0) {
                cy = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cy >= pc->m_wrapH) {
                    cy = pc->m_wrapH - 1;
                }
            }
            CDDrawWorkerHost* pl = m_mainPlane;
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
                CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
                result = set->GetCollisionAt(subX, subY);
            }
        }
        if (result == kTileSoft2 && (t->m_flags & 0x400)) {
            result = kTilePassable;
        }
        if (result == kTileSoft || result == kTileSoft2) {
            i32 lo = t->m_screenY + t->m_extent.bottom;
            i32 j = fixedY - 1;
            state |= 0x1020000;
            for (; j > lo; j--) {
                if (AxisProbe(col, j) == kTilePassable) {
                    j -= t->m_extent.bottom;
                    goto have_y;
                }
            }
            j = t->m_screenY;
        have_y:
            y = j;
            if (j == t->m_screenY) {
                *py = t->m_screenY;
                return state;
            }
        }
        if (col == colHi) {
            col++;
        } else {
            col += t->m_strideX;
            if (col > colHi) {
                col = colHi;
            }
        }
    }
    if (BroadPhase(t, x, y) != 0) {
        *py = t->m_screenY;
        return state | 0x42000000;
    }
    *py = y;
    return state;
}

// MoveStepYLo - 0x167830. Fixed Y = low edge (y + axisLoB); sweep X, scan Y up.
// The sweep is a natural `while (cur <= limit)`, not the `if (>) goto helper; looptop:`
// goto skeleton: the goto form put the loop-exit on the FALLTHROUGH and emitted
// `jg helper / jmp looptop` where retail has `jle looptop` and falls through to the
// helper (jcc_sieve POLARITY #17, all four of these). The equal-coord early return is
// spelled inline for the same reason - the `goto done_eq` label got laid out ahead of
// the helper block. Branch sequences agree now.
// @early-stop
RVA(0x00167830, 0x1eb)
i32 CGameLevel::MoveStepYLo(CGameObject* t, i32 x, i32 y, i32* py, i32 flags) {
    i32 colHi = t->m_extent.right + x;
    i32 fixedY = y + t->m_extent.top;
    i32 col = t->m_extent.left + x;
    i32 state = 0;
    while (col <= colHi) {
        i32 result;
        {
            i32 cx = col;
            if (cx < 0) {
                cx = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cx >= pc->m_wrapW) {
                    cx = pc->m_wrapW - 1;
                }
            }
            i32 cy = fixedY;
            if (cy < 0) {
                cy = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cy >= pc->m_wrapH) {
                    cy = pc->m_wrapH - 1;
                }
            }
            CDDrawWorkerHost* pl = m_mainPlane;
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
                CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
                result = set->GetCollisionAt(subX, subY);
            }
        }
        if (result == kTileSoft2 && (t->m_flags & 0x400)) {
            result = kTilePassable;
        }
        if (result == kTileSoft || result == kTileSoft2) {
            i32 lo = t->m_screenY + t->m_extent.top;
            i32 j = fixedY + 1;
            state |= 0x820000;
            for (; j < lo; j++) {
                if (AxisProbe(col, j) == kTilePassable) {
                    j -= t->m_extent.top;
                    goto have_y;
                }
            }
            j = t->m_screenY;
        have_y:
            y = j;
            if (j == t->m_screenY) {
                *py = t->m_screenY;
                return state;
            }
        }
        if (col == colHi) {
            col++;
        } else {
            col += t->m_strideX;
            if (col > colHi) {
                col = colHi;
            }
        }
    }
    if (BroadPhase(t, x, y) != 0) {
        *py = t->m_screenY;
        return state | 0x12000000;
    }
    *py = y;
    return state;
}

// The scan reuses the incoming coordinate as the loop counter (retail's
// `mov ebx,[esp+N]; dec ebx` inc-in-place form; a fresh `col` local hoists a
// lea instead and caps the quartet at 93-97%).
//
// EXACT since 2026-07-29. The last two bytes were the `limit` sum's LOAD ORDER: cl
// CANONICALISES `a + b` when both sides are plain member loads (it emits the higher
// offset first REGARDLESS of source order - swapping the operands is a byte-identical
// no-op, measured both ways). The lever is a named local for the FIRST operand: that
// makes its load a statement of its own and it is emitted first, which is retail's
// `mov eax,[t+0x5c]; mov ebx,[t+0x13c]`. A local on the SECOND operand does nothing.
RVA(0x00167a20, 0x11b)
i32 CGameLevel::ResolveRightX(CGameObject* t, i32 x, i32 y) {
    i32 sx = t->m_screenX;
    i32 limit = sx + t->m_extent.right;
    for (x--; x > limit; x--) {
        i32 result;
        PROBE_TILE(this, x, y, result);
        if (result == kTilePassable) {
            return x - t->m_extent.right;
        }
    }
    return t->m_screenX;
}

RVA(0x00167b40, 0x11b)
i32 CGameLevel::ResolveLeftX(CGameObject* t, i32 x, i32 y) {
    i32 limit = t->m_screenX;
    limit += t->m_extent.left;
    for (x++; x < limit; x++) {
        i32 result;
        PROBE_TILE(this, x, y, result);
        if (result == kTilePassable) {
            return x - t->m_extent.left;
        }
    }
    return t->m_screenX;
}

RVA(0x00167c60, 0x11b)
i32 CGameLevel::ResolveBottomY(CGameObject* t, i32 x, i32 y) {
    i32 sy = t->m_screenY;
    i32 limit = sy + t->m_extent.bottom;
    for (y--; y > limit; y--) {
        i32 result;
        PROBE_TILE(this, x, y, result);
        if (result == kTilePassable) {
            return y - t->m_extent.bottom;
        }
    }
    return t->m_screenY;
}

RVA(0x00167d80, 0x11b)
i32 CGameLevel::ResolveTopY(CGameObject* t, i32 x, i32 y) {
    i32 sy = t->m_screenY;
    i32 e = t->m_extent.top;
    i32 limit = sy + e;
    for (y++; y < limit; y++) {
        i32 result;
        PROBE_TILE(this, x, y, result);
        if (result == kTilePassable) {
            return y - t->m_extent.top;
        }
    }
    return t->m_screenY;
}

// The chain walk is a natural `while`, not `if (pos == 0) return 0; do {} while (pos)`:
// the do-while spelling let cl TAIL-MERGE the loop-exit `return 0` onto the two guard
// epilogues (2 rets where retail has 3) and inverted the back edge (`je exit` where
// retail has `jne looptop`). jcc_sieve flagged it as POLARITY #17; the branch sequences
// agree now.
// @early-stop
RVA(0x00167ea0, 0x1b9)
i32 CGameLevel::BroadPhase(CGameObject* t, i32 candX, i32 candY) {
    if (!(t->m_flags & 0x100)) {
        return 0;
    }
    CObList& chain = OwnerMgr()->m_childGroup->m_list;
    POSITION pos = chain.GetHeadPosition();
    while (pos != 0) {
        CGameObject* obj = static_cast<CGameObject*>(chain.GetNext(pos));
        if (obj != t && (obj->m_flags & 0x100) && (t->m_collMask & obj->m_collCategory)
            && t->m_extent.left != AXIS_UNSET && obj->m_extent.left != AXIS_UNSET) {
            i32 tLeft = t->m_extent.left + t->m_screenX;
            i32 tBot = t->m_extent.top + t->m_screenY;
            i32 tRight = t->m_screenX + t->m_extent.right;
            i32 tTop = t->m_extent.bottom + t->m_screenY;
            i32 oLeft = obj->m_screenX + obj->m_extent.left;
            i32 oBot = obj->m_extent.top + obj->m_screenY;
            i32 oTop = obj->m_screenY + obj->m_extent.bottom;
            i32 oRight = obj->m_screenX + obj->m_extent.right;
            if (tLeft > oRight || tRight < oLeft || tBot > oTop || tTop < oBot) {
                i32 cLeft = candX + t->m_extent.left;
                i32 cRight = t->m_extent.right + candX;
                i32 cBot = t->m_extent.top + candY;
                i32 cTop = t->m_extent.bottom + candY;
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
    }
    return 0;
}

// ===========================================================================
// CWwdGridShell::OnFound (0x168060) - the concrete impl of the slot the abstract
// CWwdGrid leaves __purecall: hand the found region's game object (+0x18) to the
// world's broadcast child-group for sorted (re)insertion, addToMaps=1. The world
// hop is the CLoadable-family int owner handle (OwnerMgr()).
RVA(0x00168060, 0x18)
void CWwdGridShell::OnFound(WwdRegion* r) {
    CGameObject* obj = r->m_object;
    obj->OwnerMgr()->m_childGroup->InsertSorted(obj, 1);
}

// ===========================================================================
// CWwdSpatialMgr::Init (0x168080, __thiscall, ret 0x20 = 8 args): bring up the
// 0xb8-byte plane grid/scroll worker. Allocate three concrete CWwdGridShells,
// initialize their inherited CWwdGrid storage with Setup(*rc, cellW, cellH),
// then seed each plane's dimensions and scroll origin. `owner` becomes m_mgr.
//
// 1:1 - m_0=m_mgr, m_4/8/c=grids, rects @0x10/0x30/0x20, origins @0x40/0x48/0x50, bbox
// @0x58, scroll @0x68]; the fake `?Init@Builder_168080@@` name also left RebuildPlanes'
// Init call unresolved. `Pt_168080` was a plain i32[2] size pair.)
//
RVA(0x00168080, 0x1f6)
RVA_COMPGEN(0x00168280, 0x1e, ??_GCWwdGridShell@@UAEPAXI@Z)
// The six geometry pairs come straight off CGameLevel in the one caller,
// CDDrawWorkerHost::RebuildPlanes @0x1628f0: cellA/B/C are its m_pairA/m_pairB/m_pairC
// (each a {cellW, cellH} pair for one grid's Setup) and sizeA/B/C its
// m_rectA/m_rectB/m_rectC {w, h} (each becomes a plane's 0..n-1 rect and its n/2 origin).
i32 CWwdSpatialMgr::Init(
    void* owner,
    RECT* rc,
    i32* cellA,
    i32* cellB,
    i32* cellC,
    i32* sizeA,
    i32* sizeB,
    i32* sizeC
) {
    if (owner) {
        m_grid0 = new CWwdGridShell;
        m_grid1 = new CWwdGridShell;
        m_grid2 = new CWwdGridShell;
        if (m_grid0 && m_grid1 && m_grid2 && m_grid0->Setup(*rc, cellA[0], cellA[1])
            && m_grid1->Setup(*rc, cellB[0], cellB[1]) && m_grid2->Setup(*rc, cellC[0], cellC[1])) {
            m_rect0.left = 0;
            m_rect0.top = 0;
            m_rect0.right = sizeA[0] - 1;
            m_rect0.bottom = sizeA[1] - 1;
            m_org0x = sizeA[0] / 2;
            m_org0y = sizeA[1] / 2;
            m_rect1.left = 0;
            m_rect1.top = 0;
            m_rect1.right = sizeB[0] - 1;
            m_rect1.bottom = sizeB[1] - 1;
            m_org1x = sizeB[0] / 2;
            m_org1y = sizeB[1] / 2;
            m_rect2.left = 0;
            m_rect2.top = 0;
            m_rect2.right = sizeC[0] - 1;
            m_rect2.bottom = sizeC[1] - 1;
            m_org2x = sizeC[0] / 2;
            m_org2y = sizeC[1] / 2;
            m_mgr = static_cast<CDDrawChildGroup*>(owner);
            SetRect(&m_bounds, rc->left, rc->top, rc->right, rc->bottom);
            m_scrollX = static_cast<i32>(0xffffa932);
            m_scrollY = static_cast<i32>(0xffffa932);
            return 1;
        }
    }
    return 0;
}

#undef kTilePassable
#undef kTileSoft
#undef kTileSoft2
#undef kTileHard
#undef kTileSpecial
#undef PROBE_TILE
