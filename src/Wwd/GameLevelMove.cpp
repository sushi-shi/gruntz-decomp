#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGrid.h>
#include <MakeRect.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Object.h>
#include <Wwd/MoveFlags.h>
#include <Wwd/MoveMode.h>
#include <Wwd/WwdGridShell.h>
#include <Wwd/WwdSpatialMgr.h>

RVA(0x00167130, 0x83)
i32 CGameLevel::ApplyMove(CGameObject* target, i32 destX, i32 destY, i32 moveFlags) {
    i32 result = 0;
    Coord previousPosition = target->ScreenPos();
    MoveMode moveMode = target->m_moveMode;

    if (moveMode > MOVE_NONE) {
        if (moveMode > MOVE_GROUNDED_LAST) {
            if (moveMode == MOVE_DIRECT) {
                target->SetScreenPos(destX, destY);
            }
        } else {
            result = MoveAxisAligned(target, destX, destY, moveFlags);
        }
    }

    if (result & IDX(MOVE_RESULT_AXIS_BLOCKED)) {
        result |= IDX(MOVE_RESULT_TILE_COLLISION);
    }
    u32 objectFlags = target->m_flags;
    if (objectFlags & IDX(WWD_GAME_OBJECT_FLAG_TOUCHED_DEATH_TILE)) {
        result |= IDX(MOVE_RESULT_DEATH_TILE);
    }
    if (objectFlags & IDX(WWD_GAME_OBJECT_FLAG_ON_CARRIER)) {
        result |= IDX(MOVE_RESULT_ON_CARRIER);
    }
    if (target->ScreenPos() == previousPosition) {
        result |= IDX(MOVE_RESULT_NO_POSITION_CHANGE);
    }
    return result;
}

RVA(0x001671c0, 0x97)
i32 CGameLevel::MoveAxisAligned(CGameObject* t, i32 x, i32 y, i32 flags) {
    i32 result = 0;
    Coord currentPosition = t->ScreenPos();
    if (x > currentPosition.m_x) {
        result = MoveStepXHi(t, x, y, &x, flags);
    } else if (x < currentPosition.m_x) {
        result = MoveStepXLo(t, x, y, &x, flags);
    }
    if (y > currentPosition.m_y) {
        result |= MoveStepYHi(t, x, y, &y, flags);
    } else if (y < currentPosition.m_y) {
        result |= MoveStepYLo(t, x, y, &y, flags);
    }
    t->SetScreenPos(x, y);
    return result;
}

// @early-stop
RVA(0x00167260, 0x1ef)
i32 CGameLevel::MoveStepXHi(CGameObject* t, i32 x, i32 y, i32* px, i32 flags) {
    i32 state = 0;
    i32 xEnd = x + t->m_extent.right;
    i32 yHi = t->m_extent.bottom + y;
    i32 yLo = t->m_extent.top + y;
    while (yLo <= yHi) {
        TileCollisionKind result;
        PROBE_TILE(this, xEnd, yLo, result);
        if (result == TILEKIND_GROUND
            && (t->m_flags & IDX(WWD_GAME_OBJECT_FLAG_GROUND_IS_PASSABLE))) {
            result = TILEKIND_PASSABLE;
        }
        if (result == TILEKIND_SOLID || result == TILEKIND_GROUND) {
            i32 lo = t->m_screenPosition.m_x + t->m_extent.right;
            x = xEnd - 1;
            state |= IDX(MOVE_RESULT_AXIS_BLOCKED | MOVE_RESULT_TILE_RIGHT);
            for (; x > lo; x--) {
                if (AxisProbe(x, yLo) == TILEKIND_PASSABLE) {
                    x -= t->m_extent.right;
                    goto have_x;
                }
            }
            x = t->m_screenPosition.m_x;
        have_x:
            if (x == t->m_screenPosition.m_x) {
                *px = t->m_screenPosition.m_x;
                return state;
            }
        }
        if (yLo == yHi) {
            yLo++;
        } else {
            yLo += t->m_stride.m_y;
            if (yLo > yHi) {
                yLo = yHi;
            }
        }
    }
    if (BroadPhase(t, x, y) != 0) {
        *px = t->m_screenPosition.m_x;
        return state | IDX(MOVE_RESULT_OBJECT_COLLISION | MOVE_RESULT_OBJECT_RIGHT);
    }
    *px = x;
    return state;
}

// @early-stop
RVA(0x00167450, 0x1ef)
i32 CGameLevel::MoveStepXLo(CGameObject* t, i32 x, i32 y, i32* px, i32 flags) {
    i32 state = 0;
    i32 xEnd = x + t->m_extent.left;
    i32 yHi = t->m_extent.bottom + y;
    i32 yLo = t->m_extent.top + y;
    while (yLo <= yHi) {
        TileCollisionKind result;
        PROBE_TILE(this, xEnd, yLo, result);
        if (result == TILEKIND_GROUND
            && (t->m_flags & IDX(WWD_GAME_OBJECT_FLAG_GROUND_IS_PASSABLE))) {
            result = TILEKIND_PASSABLE;
        }
        if (result == TILEKIND_SOLID || result == TILEKIND_GROUND) {
            i32 lo = t->m_screenPosition.m_x + t->m_extent.left;
            x = xEnd + 1;
            state |= IDX(MOVE_RESULT_AXIS_BLOCKED | MOVE_RESULT_TILE_LEFT);
            for (; x < lo; x++) {
                if (AxisProbe(x, yLo) == TILEKIND_PASSABLE) {
                    x -= t->m_extent.left;
                    goto have_x;
                }
            }
            x = t->m_screenPosition.m_x;
        have_x:
            if (x == t->m_screenPosition.m_x) {
                *px = t->m_screenPosition.m_x;
                return state;
            }
        }
        if (yLo == yHi) {
            yLo++;
        } else {
            yLo += t->m_stride.m_y;
            if (yLo > yHi) {
                yLo = yHi;
            }
        }
    }
    if (BroadPhase(t, x, y) != 0) {
        *px = t->m_screenPosition.m_x;
        return state | IDX(MOVE_RESULT_OBJECT_COLLISION | MOVE_RESULT_OBJECT_LEFT);
    }
    *px = x;
    return state;
}

// @early-stop
RVA(0x00167640, 0x1eb)
i32 CGameLevel::MoveStepYHi(CGameObject* t, i32 x, i32 y, i32* py, i32 flags) {
    i32 colHi = t->m_extent.right + x;
    i32 fixedY = y + t->m_extent.bottom;
    i32 col = t->m_extent.left + x;
    i32 state = 0;
    while (col <= colHi) {
        TileCollisionKind result;
        PROBE_TILE(this, col, fixedY, result);
        if (result == TILEKIND_GROUND
            && (t->m_flags & IDX(WWD_GAME_OBJECT_FLAG_GROUND_IS_PASSABLE))) {
            result = TILEKIND_PASSABLE;
        }
        if (result == TILEKIND_SOLID || result == TILEKIND_GROUND) {
            i32 lo = t->m_screenPosition.m_y + t->m_extent.bottom;
            y = fixedY - 1;
            state |= IDX(MOVE_RESULT_AXIS_BLOCKED | MOVE_RESULT_TILE_BOTTOM);
            for (; y > lo; y--) {
                if (AxisProbe(col, y) == TILEKIND_PASSABLE) {
                    y -= t->m_extent.bottom;
                    goto have_y;
                }
            }
            y = t->m_screenPosition.m_y;
        have_y:
            if (y == t->m_screenPosition.m_y) {
                *py = t->m_screenPosition.m_y;
                return state;
            }
        }
        if (col == colHi) {
            col++;
        } else {
            col += t->m_stride.m_x;
            if (col > colHi) {
                col = colHi;
            }
        }
    }
    if (BroadPhase(t, x, y) != 0) {
        *py = t->m_screenPosition.m_y;
        return state | IDX(MOVE_RESULT_OBJECT_COLLISION | MOVE_RESULT_OBJECT_BOTTOM);
    }
    *py = y;
    return state;
}

// @early-stop
RVA(0x00167830, 0x1eb)
i32 CGameLevel::MoveStepYLo(CGameObject* t, i32 x, i32 y, i32* py, i32 flags) {
    i32 colHi = t->m_extent.right + x;
    i32 fixedY = y + t->m_extent.top;
    i32 col = t->m_extent.left + x;
    i32 state = 0;
    while (col <= colHi) {
        TileCollisionKind result;
        PROBE_TILE(this, col, fixedY, result);
        if (result == TILEKIND_GROUND
            && (t->m_flags & IDX(WWD_GAME_OBJECT_FLAG_GROUND_IS_PASSABLE))) {
            result = TILEKIND_PASSABLE;
        }
        if (result == TILEKIND_SOLID || result == TILEKIND_GROUND) {
            i32 lo = t->m_screenPosition.m_y + t->m_extent.top;
            y = fixedY + 1;
            state |= IDX(MOVE_RESULT_AXIS_BLOCKED | MOVE_RESULT_TILE_TOP);
            for (; y < lo; y++) {
                if (AxisProbe(col, y) == TILEKIND_PASSABLE) {
                    y -= t->m_extent.top;
                    goto have_y;
                }
            }
            y = t->m_screenPosition.m_y;
        have_y:
            if (y == t->m_screenPosition.m_y) {
                *py = t->m_screenPosition.m_y;
                return state;
            }
        }
        if (col == colHi) {
            col++;
        } else {
            col += t->m_stride.m_x;
            if (col > colHi) {
                col = colHi;
            }
        }
    }
    if (BroadPhase(t, x, y) != 0) {
        *py = t->m_screenPosition.m_y;
        return state | IDX(MOVE_RESULT_OBJECT_COLLISION | MOVE_RESULT_OBJECT_TOP);
    }
    *py = y;
    return state;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00167a20, 0x11b)
i32 CGameLevel::ResolveRightX(CGameObject* t, i32 x, i32 y) {
    i32 sx = t->m_screenPosition.m_x;
    i32 limit = sx + t->m_extent.right;
    for (x--; x > limit; x--) {
        TileCollisionKind result;
        PROBE_TILE(this, x, y, result);
        if (result == TILEKIND_PASSABLE) {
            return x - t->m_extent.right;
        }
    }
    return t->m_screenPosition.m_x;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00167b40, 0x11b)
i32 CGameLevel::ResolveLeftX(CGameObject* t, i32 x, i32 y) {
    i32 limit = t->m_screenPosition.m_x;
    limit += t->m_extent.left;
    for (x++; x < limit; x++) {
        TileCollisionKind result;
        PROBE_TILE(this, x, y, result);
        if (result == TILEKIND_PASSABLE) {
            return x - t->m_extent.left;
        }
    }
    return t->m_screenPosition.m_x;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00167c60, 0x11b)
i32 CGameLevel::ResolveBottomY(CGameObject* t, i32 x, i32 y) {
    i32 sy = t->m_screenPosition.m_y;
    i32 limit = sy + t->m_extent.bottom;
    for (y--; y > limit; y--) {
        TileCollisionKind result;
        PROBE_TILE(this, x, y, result);
        if (result == TILEKIND_PASSABLE) {
            return y - t->m_extent.bottom;
        }
    }
    return t->m_screenPosition.m_y;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00167d80, 0x11b)
i32 CGameLevel::ResolveTopY(CGameObject* t, i32 x, i32 y) {
    i32 sy = t->m_screenPosition.m_y;
    i32 e = t->m_extent.top;
    i32 limit = sy + e;
    for (y++; y < limit; y++) {
        TileCollisionKind result;
        PROBE_TILE(this, x, y, result);
        if (result == TILEKIND_PASSABLE) {
            return y - t->m_extent.top;
        }
    }
    return t->m_screenPosition.m_y;
}

RVA(0x00167ea0, 0x1b9)
i32 CGameLevel::BroadPhase(CGameObject* t, i32 candX, i32 candY) {
    if (!(t->m_flags & IDX(WWD_GAME_OBJECT_FLAG_COLLIDE_WITH_OBJECTS))) {
        return 0;
    }
    CDDrawChildGroup* children = OwnerMgr()->m_childGroup;
    POSITION pos = children->m_list.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = children->NextChild(pos);
        if (obj != t && (obj->m_flags & IDX(WWD_GAME_OBJECT_FLAG_COLLIDE_WITH_OBJECTS))
            && (t->m_collMask & obj->m_objectType) && t->m_extent.left != COORD_UNSET
            && obj->m_extent.left != COORD_UNSET) {
            CRect currentBounds(
                t->m_extent.left + t->m_screenPosition.m_x,
                t->m_extent.top + t->m_screenPosition.m_y,
                t->m_screenPosition.m_x + t->m_extent.right,
                t->m_extent.bottom + t->m_screenPosition.m_y
            );
            CRect otherBounds(
                obj->m_screenPosition.m_x + obj->m_extent.left,
                obj->m_extent.top + obj->m_screenPosition.m_y,
                obj->m_screenPosition.m_x + obj->m_extent.right,
                obj->m_screenPosition.m_y + obj->m_extent.bottom
            );
            if (currentBounds.left > otherBounds.right || currentBounds.right < otherBounds.left
                || currentBounds.top > otherBounds.bottom
                || currentBounds.bottom < otherBounds.top) {
                CRect candidateBounds(
                    candX + t->m_extent.left,
                    candY + t->m_extent.top,
                    t->m_extent.right + candX,
                    t->m_extent.bottom + candY
                );
                if (candidateBounds.left <= otherBounds.right
                    && candidateBounds.right >= otherBounds.left
                    && candidateBounds.top <= otherBounds.bottom
                    && candidateBounds.bottom >= otherBounds.top) {
                    i32 fire;
                    if (t->m_collisionLogic != NULL) {
                        t->m_hitOther = obj;
                        fire = t->m_collisionLogic->m_dispatch(t);
                    } else {
                        fire = 1;
                    }
                    if (fire != 0) {
                        if (t->m_collMask & obj->m_objectType) {
                            if (obj->m_collisionLogic != NULL) {
                                obj->m_hitOther = t;
                                obj->m_collisionLogic->m_dispatch(obj);
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

RVA(0x00168060, 0x18)
void CWwdGridShell::OnFound(WwdRegion* r) {
    CGameObject* obj = r->m_object;
    obj->OwnerMgr()->m_childGroup->InsertSorted(obj, 1);
}

RVA(0x00168080, 0x1f6)
RVA_COMPGEN(0x00168280, 0x1e, ??_GCWwdGridShell@@UAEPAXI@Z)
RVA_COMPGEN(0x001682a0, 0x46, ??1CWwdGridShell@@UAE@XZ)
RVA_COMPGEN(0x00168bf0, 0x1e, ??_GCWwdGrid@@UAEPAXI@Z)
RVA_COMPGEN(0x00168c10, 0x46, ??1CWwdGrid@@UAE@XZ)

i32 CWwdSpatialMgr::Init(
    CDDrawChildGroup* owner,
    RECT* levelBounds,
    i32* defaultGridCellSize,
    i32* largeGridCellSize,
    i32* smallGridCellSize,
    i32* defaultRegionSize,
    i32* largeRegionSize,
    i32* smallRegionSize
) {
    if (owner) {
        CSize defaultGridSize(defaultGridCellSize[0], defaultGridCellSize[1]);
        CSize largeGridSize(largeGridCellSize[0], largeGridCellSize[1]);
        CSize smallGridSize(smallGridCellSize[0], smallGridCellSize[1]);
        CSize defaultRegion(defaultRegionSize[0], defaultRegionSize[1]);
        CSize largeRegion(largeRegionSize[0], largeRegionSize[1]);
        CSize smallRegion(smallRegionSize[0], smallRegionSize[1]);
        m_defaultRegionGrid = new CWwdGridShell;
        m_largeRegionGrid = new CWwdGridShell;
        m_smallRegionGrid = new CWwdGridShell;
        if (m_defaultRegionGrid && m_largeRegionGrid && m_smallRegionGrid
            && m_defaultRegionGrid->Setup(*levelBounds, defaultGridSize.cx, defaultGridSize.cy)
            && m_largeRegionGrid->Setup(*levelBounds, largeGridSize.cx, largeGridSize.cy)
            && m_smallRegionGrid->Setup(*levelBounds, smallGridSize.cx, smallGridSize.cy)) {
            m_defaultRegionRect = MakeRect(0, 0, defaultRegion.cx - 1, defaultRegion.cy - 1);
            m_defaultRegionHalfSize = CSize(defaultRegion.cx / 2, defaultRegion.cy / 2);
            m_largeRegionRect = MakeRect(0, 0, largeRegion.cx - 1, largeRegion.cy - 1);
            m_largeRegionHalfSize = CSize(largeRegion.cx / 2, largeRegion.cy / 2);
            m_smallRegionRect = MakeRect(0, 0, smallRegion.cx - 1, smallRegion.cy - 1);
            m_smallRegionHalfSize = CSize(smallRegion.cx / 2, smallRegion.cy / 2);
            m_activeGroup = owner;
            m_levelBounds = *levelBounds;
            m_activeCenter.Set(static_cast<i32>(0xffffa932), static_cast<i32>(0xffffa932));
            return 1;
        }
    }
    return 0;
}

#undef PROBE_TILE
