#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGrid.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Object.h>
#include <Wwd/MoveFlags.h>
#include <Wwd/MoveMode.h>
#include <Wwd/WwdGridShell.h>
#include <Wwd/WwdSpatialMgr.h>

RVA(0x00167410, 0x83)
i32 CGameLevel::ApplyMove(CGameObject* target, i32 destX, i32 destY, i32 moveFlags) {
    i32 result = 0;
    i32 prevX = target->m_screenX;
    i32 prevY = target->m_screenY;
    MoveMode moveMode = target->m_moveMode;

    if (moveMode > MOVE_NONE) {
        if (moveMode > MOVE_GROUNDED_LAST) {
            if (moveMode == MOVE_DIRECT) {
                target->m_screenX = destX;
                target->m_screenY = destY;
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
    if (objectFlags & 0x10) {
        result |= IDX(MOVE_RESULT_ON_CARRIER);
    }
    if (target->m_screenX == prevX && target->m_screenY == prevY) {
        result |= IDX(MOVE_RESULT_NO_POSITION_CHANGE);
    }
    return result;
}

RVA(0x001674a0, 0x97)
i32 CGameLevel::MoveAxisAligned(CGameObject* t, i32 x, i32 y, i32 flags) {
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

// @early-stop
RVA(0x00167540, 0x1ef)
i32 CGameLevel::MoveStepXHi(CGameObject* t, i32 x, i32 y, i32* px, i32 flags) {
    i32 xEnd = x + t->m_extent.right;
    i32 yHi = t->m_extent.bottom + y;
    i32 yLo = t->m_extent.top + y;
    i32 state = 0;
    while (yLo <= yHi) {
        TileCollisionKind result;
        {
            i32 cx = xEnd;
            if (cx < 0) {
                cx = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cx >= pc->m_planePixelWidth) {
                    cx = pc->m_planePixelWidth - 1;
                }
            }
            i32 cy = yLo;
            if (cy < 0) {
                cy = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cy >= pc->m_planePixelHeight) {
                    cy = pc->m_planePixelHeight - 1;
                }
            }
            CDDrawWorkerHost* pl = m_mainPlane;
            i32 qx = cx >> pl->m_shiftX;
            i32 qy = cy >> pl->m_shiftY;
            i32 col = qx;
            i32 subX = cx - (qx << pl->m_shiftX);
            i32 idx = pl->m_tileRowOffsets[qy] + col;
            i32 subY = cy - (qy << pl->m_shiftY);
            i32 tile = pl->m_tileHandles[idx];
            if (tile == UNINIT_FILL || tile == TILE_CLEAR) {
                result = TILEKIND_PASSABLE;
            } else {
                CTileImageSet* set =
                    static_cast<CTileImageSet*>(m_imageSets[tile & WWD_TILE_IMAGE_SET_INDEX_MASK]);
                result = set->GetCollisionAt(subX, subY);
            }
        }
        if (result == TILEKIND_GROUND
            && (t->m_flags & IDX(WWD_GAME_OBJECT_FLAG_GROUND_IS_PASSABLE))) {
            result = TILEKIND_PASSABLE;
        }
        if (result == TILEKIND_SOLID || result == TILEKIND_GROUND) {
            i32 lo = t->m_screenX + t->m_extent.right;
            i32 j = xEnd - 1;
            state |= IDX(MOVE_RESULT_AXIS_BLOCKED | MOVE_RESULT_TILE_RIGHT);
            for (; j > lo; j--) {
                if (AxisProbe(j, yLo) == TILEKIND_PASSABLE) {
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
        return state | IDX(MOVE_RESULT_OBJECT_COLLISION | MOVE_RESULT_OBJECT_RIGHT);
    }
    *px = x;
    return state;
}

// @early-stop
RVA(0x00167730, 0x1ef)
i32 CGameLevel::MoveStepXLo(CGameObject* t, i32 x, i32 y, i32* px, i32 flags) {
    i32 xEnd = x + t->m_extent.left;
    i32 yHi = t->m_extent.bottom + y;
    i32 yLo = t->m_extent.top + y;
    i32 state = 0;
    while (yLo <= yHi) {
        TileCollisionKind result;
        {
            i32 cx = xEnd;
            if (cx < 0) {
                cx = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cx >= pc->m_planePixelWidth) {
                    cx = pc->m_planePixelWidth - 1;
                }
            }
            i32 cy = yLo;
            if (cy < 0) {
                cy = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cy >= pc->m_planePixelHeight) {
                    cy = pc->m_planePixelHeight - 1;
                }
            }
            CDDrawWorkerHost* pl = m_mainPlane;
            i32 qx = cx >> pl->m_shiftX;
            i32 qy = cy >> pl->m_shiftY;
            i32 col = qx;
            i32 subX = cx - (qx << pl->m_shiftX);
            i32 idx = pl->m_tileRowOffsets[qy] + col;
            i32 subY = cy - (qy << pl->m_shiftY);
            i32 tile = pl->m_tileHandles[idx];
            if (tile == UNINIT_FILL || tile == TILE_CLEAR) {
                result = TILEKIND_PASSABLE;
            } else {
                CTileImageSet* set =
                    static_cast<CTileImageSet*>(m_imageSets[tile & WWD_TILE_IMAGE_SET_INDEX_MASK]);
                result = set->GetCollisionAt(subX, subY);
            }
        }
        if (result == TILEKIND_GROUND
            && (t->m_flags & IDX(WWD_GAME_OBJECT_FLAG_GROUND_IS_PASSABLE))) {
            result = TILEKIND_PASSABLE;
        }
        if (result == TILEKIND_SOLID || result == TILEKIND_GROUND) {
            i32 lo = t->m_screenX + t->m_extent.left;
            i32 j = xEnd + 1;
            state |= IDX(MOVE_RESULT_AXIS_BLOCKED | MOVE_RESULT_TILE_LEFT);
            for (; j < lo; j++) {
                if (AxisProbe(j, yLo) == TILEKIND_PASSABLE) {
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
        return state | IDX(MOVE_RESULT_OBJECT_COLLISION | MOVE_RESULT_OBJECT_LEFT);
    }
    *px = x;
    return state;
}

// @early-stop
RVA(0x00167920, 0x1eb)
i32 CGameLevel::MoveStepYHi(CGameObject* t, i32 x, i32 y, i32* py, i32 flags) {
    i32 colHi = t->m_extent.right + x;
    i32 fixedY = y + t->m_extent.bottom;
    i32 col = t->m_extent.left + x;
    i32 state = 0;
    while (col <= colHi) {
        TileCollisionKind result;
        {
            i32 cx = col;
            if (cx < 0) {
                cx = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cx >= pc->m_planePixelWidth) {
                    cx = pc->m_planePixelWidth - 1;
                }
            }
            i32 cy = fixedY;
            if (cy < 0) {
                cy = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cy >= pc->m_planePixelHeight) {
                    cy = pc->m_planePixelHeight - 1;
                }
            }
            CDDrawWorkerHost* pl = m_mainPlane;
            i32 qx = cx >> pl->m_shiftX;
            i32 qy = cy >> pl->m_shiftY;
            i32 c = qx;
            i32 subX = cx - (qx << pl->m_shiftX);
            i32 idx = pl->m_tileRowOffsets[qy] + c;
            i32 subY = cy - (qy << pl->m_shiftY);
            i32 tile = pl->m_tileHandles[idx];
            if (tile == UNINIT_FILL || tile == TILE_CLEAR) {
                result = TILEKIND_PASSABLE;
            } else {
                CTileImageSet* set =
                    static_cast<CTileImageSet*>(m_imageSets[tile & WWD_TILE_IMAGE_SET_INDEX_MASK]);
                result = set->GetCollisionAt(subX, subY);
            }
        }
        if (result == TILEKIND_GROUND
            && (t->m_flags & IDX(WWD_GAME_OBJECT_FLAG_GROUND_IS_PASSABLE))) {
            result = TILEKIND_PASSABLE;
        }
        if (result == TILEKIND_SOLID || result == TILEKIND_GROUND) {
            i32 lo = t->m_screenY + t->m_extent.bottom;
            i32 j = fixedY - 1;
            state |= IDX(MOVE_RESULT_AXIS_BLOCKED | MOVE_RESULT_TILE_BOTTOM);
            for (; j > lo; j--) {
                if (AxisProbe(col, j) == TILEKIND_PASSABLE) {
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
        return state | IDX(MOVE_RESULT_OBJECT_COLLISION | MOVE_RESULT_OBJECT_BOTTOM);
    }
    *py = y;
    return state;
}

// @early-stop
RVA(0x00167b10, 0x1eb)
i32 CGameLevel::MoveStepYLo(CGameObject* t, i32 x, i32 y, i32* py, i32 flags) {
    i32 colHi = t->m_extent.right + x;
    i32 fixedY = y + t->m_extent.top;
    i32 col = t->m_extent.left + x;
    i32 state = 0;
    while (col <= colHi) {
        TileCollisionKind result;
        {
            i32 cx = col;
            if (cx < 0) {
                cx = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cx >= pc->m_planePixelWidth) {
                    cx = pc->m_planePixelWidth - 1;
                }
            }
            i32 cy = fixedY;
            if (cy < 0) {
                cy = 0;
            } else {
                CDDrawWorkerHost* pc = m_mainPlane;
                if (cy >= pc->m_planePixelHeight) {
                    cy = pc->m_planePixelHeight - 1;
                }
            }
            CDDrawWorkerHost* pl = m_mainPlane;
            i32 qx = cx >> pl->m_shiftX;
            i32 qy = cy >> pl->m_shiftY;
            i32 c = qx;
            i32 subX = cx - (qx << pl->m_shiftX);
            i32 idx = pl->m_tileRowOffsets[qy] + c;
            i32 subY = cy - (qy << pl->m_shiftY);
            i32 tile = pl->m_tileHandles[idx];
            if (tile == UNINIT_FILL || tile == TILE_CLEAR) {
                result = TILEKIND_PASSABLE;
            } else {
                CTileImageSet* set =
                    static_cast<CTileImageSet*>(m_imageSets[tile & WWD_TILE_IMAGE_SET_INDEX_MASK]);
                result = set->GetCollisionAt(subX, subY);
            }
        }
        if (result == TILEKIND_GROUND
            && (t->m_flags & IDX(WWD_GAME_OBJECT_FLAG_GROUND_IS_PASSABLE))) {
            result = TILEKIND_PASSABLE;
        }
        if (result == TILEKIND_SOLID || result == TILEKIND_GROUND) {
            i32 lo = t->m_screenY + t->m_extent.top;
            i32 j = fixedY + 1;
            state |= IDX(MOVE_RESULT_AXIS_BLOCKED | MOVE_RESULT_TILE_TOP);
            for (; j < lo; j++) {
                if (AxisProbe(col, j) == TILEKIND_PASSABLE) {
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
        return state | IDX(MOVE_RESULT_OBJECT_COLLISION | MOVE_RESULT_OBJECT_TOP);
    }
    *py = y;
    return state;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00167d00, 0x11b)
i32 CGameLevel::ResolveRightX(CGameObject* t, i32 x, i32 y) {
    i32 sx = t->m_screenX;
    i32 limit = sx + t->m_extent.right;
    for (x--; x > limit; x--) {
        TileCollisionKind result;
        PROBE_TILE(this, x, y, result);
        if (result == TILEKIND_PASSABLE) {
            return x - t->m_extent.right;
        }
    }
    return t->m_screenX;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00167e20, 0x11b)
i32 CGameLevel::ResolveLeftX(CGameObject* t, i32 x, i32 y) {
    i32 limit = t->m_screenX;
    limit += t->m_extent.left;
    for (x++; x < limit; x++) {
        TileCollisionKind result;
        PROBE_TILE(this, x, y, result);
        if (result == TILEKIND_PASSABLE) {
            return x - t->m_extent.left;
        }
    }
    return t->m_screenX;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00167f40, 0x11b)
i32 CGameLevel::ResolveBottomY(CGameObject* t, i32 x, i32 y) {
    i32 sy = t->m_screenY;
    i32 limit = sy + t->m_extent.bottom;
    for (y--; y > limit; y--) {
        TileCollisionKind result;
        PROBE_TILE(this, x, y, result);
        if (result == TILEKIND_PASSABLE) {
            return y - t->m_extent.bottom;
        }
    }
    return t->m_screenY;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00168060, 0x11b)
i32 CGameLevel::ResolveTopY(CGameObject* t, i32 x, i32 y) {
    i32 sy = t->m_screenY;
    i32 e = t->m_extent.top;
    i32 limit = sy + e;
    for (y++; y < limit; y++) {
        TileCollisionKind result;
        PROBE_TILE(this, x, y, result);
        if (result == TILEKIND_PASSABLE) {
            return y - t->m_extent.top;
        }
    }
    return t->m_screenY;
}

RVA(0x00168180, 0x1b9)
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

RVA(0x00168340, 0x18)
void CWwdGridShell::OnFound(WwdRegion* r) {
    CGameObject* obj = r->m_object;
    obj->OwnerMgr()->m_childGroup->InsertSorted(obj, 1);
}

RVA(0x00168360, 0x1f6)
RVA_COMPGEN(0x00168560, 0x1e, ??_GCWwdGridShell@@UAEPAXI@Z)
RVA_COMPGEN(0x00168580, 0x46, ??1CWwdGridShell@@UAE@XZ)
RVA_COMPGEN(0x00168ed0, 0x1e, ??_GCWwdGrid@@UAEPAXI@Z)
RVA_COMPGEN(0x00168ef0, 0x46, ??1CWwdGrid@@UAE@XZ)

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
        m_defaultRegionGrid = new CWwdGridShell;
        m_largeRegionGrid = new CWwdGridShell;
        m_smallRegionGrid = new CWwdGridShell;
        if (m_defaultRegionGrid && m_largeRegionGrid && m_smallRegionGrid
            && m_defaultRegionGrid
                   ->Setup(*levelBounds, defaultGridCellSize[0], defaultGridCellSize[1])
            && m_largeRegionGrid->Setup(*levelBounds, largeGridCellSize[0], largeGridCellSize[1])
            && m_smallRegionGrid->Setup(*levelBounds, smallGridCellSize[0], smallGridCellSize[1])) {
            m_defaultRegionRect.left = 0;
            m_defaultRegionRect.top = 0;
            m_defaultRegionRect.right = defaultRegionSize[0] - 1;
            m_defaultRegionRect.bottom = defaultRegionSize[1] - 1;
            m_defaultRegionHalfWidth = defaultRegionSize[0] / 2;
            m_defaultRegionHalfHeight = defaultRegionSize[1] / 2;
            m_largeRegionRect.left = 0;
            m_largeRegionRect.top = 0;
            m_largeRegionRect.right = largeRegionSize[0] - 1;
            m_largeRegionRect.bottom = largeRegionSize[1] - 1;
            m_largeRegionHalfWidth = largeRegionSize[0] / 2;
            m_largeRegionHalfHeight = largeRegionSize[1] / 2;
            m_smallRegionRect.left = 0;
            m_smallRegionRect.top = 0;
            m_smallRegionRect.right = smallRegionSize[0] - 1;
            m_smallRegionRect.bottom = smallRegionSize[1] - 1;
            m_smallRegionHalfWidth = smallRegionSize[0] / 2;
            m_smallRegionHalfHeight = smallRegionSize[1] / 2;
            m_activeGroup = owner;
            SetRect(
                &m_levelBounds,
                levelBounds->left,
                levelBounds->top,
                levelBounds->right,
                levelBounds->bottom
            );
            m_activeCenterX = static_cast<i32>(0xffffa932);
            m_activeCenterY = static_cast<i32>(0xffffa932);
            return 1;
        }
    }
    return 0;
}

#undef PROBE_TILE
