#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGrid.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Object.h>
#include <Wwd/MoveMode.h>
#include <Wwd/WwdGridShell.h>
#include <Wwd/WwdSpatialMgr.h>

// @early-stop
RVA(0x00167130, 0x83)
i32 CGameLevel::ApplyMove(CGameObject* obj, i32 a, i32 b, i32 c) {
    CGameObject* s = obj;
    i32 eax = 0;
    i32 prevX = s->m_screenX;
    i32 prevY = s->m_screenY;
    MoveMode kind = s->m_moveMode;

    if (kind > MOVE_NONE) {
        if (kind > MOVE_GROUNDED_LAST) {
            if (kind == MOVE_DIRECT) {
                s->m_screenX = a;
                s->m_screenY = b;
            }
        } else {
            eax = MoveAxisAligned(s, a, b, c);
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
RVA(0x00167260, 0x1ef)
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
            if (tile == UNINIT_FILL || tile == TILE_CLEAR) {
                result = TILEKIND_PASSABLE;
            } else {
                CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
                // Ingest: the raw WWD attribute byte for this cell.
                result = set->GetCollisionAt(subX, subY);
            }
        }
        if (result == TILEKIND_SOFT2 && (t->m_flags & 0x400)) {
            result = TILEKIND_PASSABLE;
        }
        if (result == TILEKIND_SOFT || result == TILEKIND_SOFT2) {
            i32 lo = t->m_screenX + t->m_extent.right;
            i32 j = xEnd - 1;
            state |= 0x60000;
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
        return state | 0x22000000;
    }
    *px = x;
    return state;
}

// @early-stop
RVA(0x00167450, 0x1ef)
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
            if (tile == UNINIT_FILL || tile == TILE_CLEAR) {
                result = TILEKIND_PASSABLE;
            } else {
                CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
                // Ingest: the raw WWD attribute byte for this cell.
                result = set->GetCollisionAt(subX, subY);
            }
        }
        if (result == TILEKIND_SOFT2 && (t->m_flags & 0x400)) {
            result = TILEKIND_PASSABLE;
        }
        if (result == TILEKIND_SOFT || result == TILEKIND_SOFT2) {
            i32 lo = t->m_screenX + t->m_extent.left;
            i32 j = xEnd + 1;
            state |= 0xa0000;
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
        return state | 0x82000000;
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
            if (tile == UNINIT_FILL || tile == TILE_CLEAR) {
                result = TILEKIND_PASSABLE;
            } else {
                CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
                // Ingest: the raw WWD attribute byte for this cell.
                result = set->GetCollisionAt(subX, subY);
            }
        }
        if (result == TILEKIND_SOFT2 && (t->m_flags & 0x400)) {
            result = TILEKIND_PASSABLE;
        }
        if (result == TILEKIND_SOFT || result == TILEKIND_SOFT2) {
            i32 lo = t->m_screenY + t->m_extent.bottom;
            i32 j = fixedY - 1;
            state |= 0x1020000;
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
        return state | 0x42000000;
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
            if (tile == UNINIT_FILL || tile == TILE_CLEAR) {
                result = TILEKIND_PASSABLE;
            } else {
                CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
                // Ingest: the raw WWD attribute byte for this cell.
                result = set->GetCollisionAt(subX, subY);
            }
        }
        if (result == TILEKIND_SOFT2 && (t->m_flags & 0x400)) {
            result = TILEKIND_PASSABLE;
        }
        if (result == TILEKIND_SOFT || result == TILEKIND_SOFT2) {
            i32 lo = t->m_screenY + t->m_extent.top;
            i32 j = fixedY + 1;
            state |= 0x820000;
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
        return state | 0x12000000;
    }
    *py = y;
    return state;
}

RVA(0x00167a20, 0x11b)
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

RVA(0x00167b40, 0x11b)
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

RVA(0x00167c60, 0x11b)
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

RVA(0x00167d80, 0x11b)
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

// @early-stop
RVA(0x00167ea0, 0x1b9)
i32 CGameLevel::BroadPhase(CGameObject* t, i32 candX, i32 candY) {
    if (!(t->m_flags & 0x100)) {
        return 0;
    }
    CObList& chain = OwnerMgr()->m_childGroup->m_list;
    POSITION pos = chain.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = static_cast<CGameObject*>(chain.GetNext(pos));
        if (obj != t && (obj->m_flags & 0x100) && (t->m_collMask & obj->m_objectType)
            && t->m_extent.left != COORD_UNSET && obj->m_extent.left != COORD_UNSET) {
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
                    if (t->m_collideWorker != NULL) {
                        t->m_hitOther = obj;
                        fire = t->m_collideWorker->m_notify(t);
                    } else {
                        fire = 1;
                    }
                    if (fire != 0) {
                        if (t->m_collMask & obj->m_objectType) {
                            if (obj->m_collideWorker != NULL) {
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

RVA(0x00168060, 0x18)
void CWwdGridShell::OnFound(WwdRegion* r) {
    CGameObject* obj = r->m_object;
    obj->OwnerMgr()->m_childGroup->InsertSorted(obj, 1);
}

RVA(0x00168080, 0x1f6)
RVA_COMPGEN(0x00168280, 0x1e, ??_GCWwdGridShell@@UAEPAXI@Z)

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

#undef TILEKIND_PASSABLE
#undef TILEKIND_SOFT
#undef TILEKIND_SOFT2
#undef TILEKIND_HARD
#undef TILEKIND_SPECIAL
#undef PROBE_TILE
