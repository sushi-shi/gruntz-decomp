#include <rva.h>

#include <Mfc.h>

#include <Gruntz/Brickz.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TriggerMgr.h>

// @early-stop
RVA(0x00077790, 0x630)
void CMapMgr::ComputeCellFlags(i32 x, i32 y, i32 id3) {

    BrickzCell* cell = &m_rows[y][x];
    CGameLevel* level = m_attrMgr->m_level;

    i32 cx = x;
    if (x < 0) {
        cx = 0;
    } else if (x >= level->m_mainPlane->m_gridW) {
        cx = level->m_mainPlane->m_gridW - 1;
    }
    i32 cy = y;
    if (y < 0) {
        cy = 0;
    } else if (y >= level->m_mainPlane->m_gridH) {
        cy = level->m_mainPlane->m_gridH - 1;
    }
    i32 id = level->m_mainPlane->m_tileGrid[level->m_mainPlane->m_colOffsets[cy] + cx];
    i32 typeCode;
    if (id == static_cast<i32>(0xeeeeeeee) || id == -1) {
        typeCode = 0;
    } else {
        typeCode = (static_cast<CTileImageSet*>(level->m_imageSets.GetAt(id & 0xffff)))
                       ->GetCollisionAt(0, 0);
    }
    i32 oldFlags = cell->m_flags;
    i32 keep = oldFlags & 0x1bf40000;
    i32 edgeBit = oldFlags & 0x20000000;

    switch (typeCode - 1) {
        case 0:
            cell->m_flags = 0x1;
            break;
        case 3:
            cell->m_flags = 0x2;
            break;
        case 9:
            cell->m_flags = 0x100;
            break;
        case 10:
            cell->m_flags = 0x80;
            break;
        case 11:
            cell->m_flags = 0x80;
            break;
        case 12:
            cell->m_flags = 0x80;
            break;
        case 13:
            cell->m_flags = 0x80;
            break;
        case 14:
            cell->m_flags = 0x80;
            break;
        case 15:
            cell->m_flags = 0x80;
            break;
        case 16:
            cell->m_flags = 0x80;
            break;
        case 17:
            cell->m_flags = 0x80;
            break;
        case 18:
            cell->m_flags = 0x80;
            break;
        case 29:
            cell->m_flags = 0x2021;
            break;
        case 30:
            cell->m_flags = 0x2021;
            break;
        case 31:
            cell->m_flags = 0x400;
            break;
        case 32:
            cell->m_flags = 0x2021;
            break;
        case 33:
            cell->m_flags = 0x10000;
            break;
        case 34:
            cell->m_flags = 0x42;
            break;
        case 35:
            cell->m_flags = 0x800;
            break;
        case 50:
            cell->m_flags = 0x4;
            break;
        case 51:
            cell->m_flags = 0x4;
            break;
        case 52:
            cell->m_flags = 0x4;
            break;
        case 53:
            cell->m_flags = 0x4;
            break;
        case 54:
            cell->m_flags = 0x4;
            break;
        case 55:
            cell->m_flags = 0x4;
            break;
        case 56:
            cell->m_flags = 0x4;
            break;
        case 57:
            cell->m_flags = 0x4;
            break;
        case 58:
            cell->m_flags = 0x4;
            break;
        case 59:
            cell->m_flags = 0x4;
            break;
        case 60:
            cell->m_flags = 0x4;
            break;
        case 61:
            cell->m_flags = 0x4;
            break;
        case 62:
            cell->m_flags = 0x4;
            break;
        case 63:
            cell->m_flags = 0x4;
            break;
        case 64:
            cell->m_flags = 0x4;
            break;
        case 65:
            cell->m_flags = 0x4;
            break;
        case 92:
            cell->m_flags = 0x4002008;
            break;
        case 94:
            cell->m_flags = 0x4002008;
            break;
        case 96:
            cell->m_flags = 0x4002008;
            break;
        case 98:
            cell->m_flags = 0x4002008;
            break;
        case 100:
            cell->m_flags = 0x4002008;
            break;
        case 102:
            cell->m_flags = 0x4002008;
            break;
        case 104:
            cell->m_flags = 0x4002008;
            break;
        case 107:
            cell->m_flags = 0x108;
            break;
        case 109:
            cell->m_flags = 0xa;
            break;
        case 113:
            cell->m_flags = 0x300;
            break;
        case 115:
            cell->m_flags = 0x202;
            break;
        case 149:
            cell->m_flags = 0x8000;
            break;
        case 150:
            cell->m_flags = 0x6021;
            break;
        case 151:
            cell->m_flags = 0x6021;
            break;
        case 152:
            cell->m_flags = 0x6021;
            break;
        case 153:
            cell->m_flags = 0x2001;
            break;
        default:
            cell->m_flags = (id3 == -1) ? 2 : 0;
            break;
    }
    if (edgeBit != 0) {
        cell->m_flags |= 0x20000000;
    }
    cell->m_flags |= keep;
    cell->m_tileId = id3;
    cell->m_typeCode = typeCode;

    i32 colCount = static_cast<i32>(m_width);
    for (i32 r = y - 1; r <= y + 1; r++) {
        if (r < 0 || static_cast<u32>(r) >= static_cast<u32>(m_height)) {
            continue;
        }
        for (i32 c = x - 1; c <= x + 1; c++) {
            if (c < 0 || static_cast<u32>(c) >= static_cast<u32>(m_width)) {
                continue;
            }
            BrickzCell* nc = &m_rows[r][c];
            i32 nf = nc->m_flags;
            if ((nf & 0x100) == 0) {
                continue;
            }
            BrickzCell* up = (r != 0) ? nc - colCount : 0;
            BrickzCell* down = (r < static_cast<i32>(m_height) - 1) ? nc + colCount : 0;
            BrickzCell* right = (c < colCount - 1) ? nc + 1 : 0;
            BrickzCell* left = (c != 0) ? nc - 1 : 0;
            BrickzCell* ur = (up && right) ? up + 1 : 0;
            BrickzCell* dl = (down && left) ? down - 1 : 0;
            BrickzCell* ul = (up && left) ? up - 1 : 0;
            BrickzCell* dr = (down && right) ? down + 1 : 0;
            nf &= ~0x1000;
            nc->m_flags = nf;
            if (up && down && !(up->m_flags & 0x939) && !(down->m_flags & 0x939)) {
                goto setbit;
            }
            if (right && left && !(right->m_flags & 0x939) && !(left->m_flags & 0x939)) {
                goto setbit;
            }
            if (ur && dl && !(ur->m_flags & 0x939) && !(dl->m_flags & 0x939)) {
                goto setbit;
            }
            if (ul && dr && !(ul->m_flags & 0x939) && !(dr->m_flags & 0x939)) {
            setbit:
                nc->m_flags = nf | 0x1000;
            }
        }
    }
}

RVA(0x00077dc0, 0x1d)
void CDDrawWorkerHost::SetCell(i32 x, i32 y, i32 id) {
    m_tileGrid[m_colOffsets[y] + x] = id;
}

// @early-stop
RVA(0x00077df0, 0x13d)
CGrunt* CTriggerMgr::FindNearestEnemy(CGrunt* w) {
    CGrunt* best = 0;
    i32 bestDist = 0x7fffffff;
    i32 tileX = w->m_lastTilePx.m_x >> 5;
    i32 tileY = w->m_lastTilePx.m_y >> 5;
    CGrunt** rowPtr = m_grid;
    for (i32 i = 0; i < 4; i++) {
        if (i != w->m_tileOwnerHi) {
            CGrunt** colPtr = rowPtr;
            i32 j = 15;
            do {
                CGrunt* cell = *colPtr;
                if (cell && cell->m_entranceCommitted != 0 && cell->m_gruntKind != 0x36) {
                    i32 dx = (cell->m_object->m_screenX >> 5) - tileX;
                    i32 dy = (cell->m_object->m_screenY >> 5) - tileY;
                    i32 dist = dx * dx + dy * dy;
                    if (dist < bestDist) {
                        best = cell;
                        bestDist = dist;
                    }
                }
                colPtr++;
            } while (--j != 0);
        }
        rowPtr += 15;
    }
    i32 k = w->m_reachRect.right + w->m_defenderRadius + 1;
    i32 px = w->m_object->m_screenX >> 5;
    i32 py = w->m_object->m_screenY >> 5;
    RECT rc;
    rc.left = px - k;
    rc.top = py - k;
    rc.right = px + k + 1;
    rc.bottom = py + k + 1;
    if (best) {
        POINT pt;
        pt.x = best->m_object->m_screenX >> 5;
        pt.y = best->m_object->m_screenY >> 5;
        if (!PtInRect(&rc, pt)) {
            best = 0;
        }
    }
    return best;
}
