// LevelPlane.cpp - CLevelPlane, CGameLevel's typed per-plane object (the real
// engine plane, CPlane in WwdFile.h). Split out of the GameLevel god-TU: the
// class definition lives in <Gruntz/GameLevel.h>; this TU carries the three
// matched non-virtual bodies in retail-RVA order:
//   InitGeometry_1619f0 (0x1619f0) - geometry seed + tile-grid alloc
//   RecomputePlaneCoords (0x161c90) - wrap/clamp the scaled scroll coords
//   Build               (0x161e80) - re-place one plane from a coord rect
//
// CLevelPlane emits no vtable here (its virtuals are declared-only, reloc-masked
// through the array-release slot); CGameLevel constructs its planes via the engine,
// so there is no ctor instantiation to emit ??_7CLevelPlane in this TU.
#include <Gruntz/GameLevel.h>
#include <rva.h>

#include <string.h> // strcpy

// CLevelPlane::InitGeometry_1619f0 (0x1619f0, CDDrawWorkerHost vtable slot +0x24):
// seed tile/wrap/origin/shift fields from the 8 args, log2 the tile shifts, strcpy
// the name, alloc the tile grid + column-offset table, tail-call RecomputePlaneCoords.
// __thiscall, 8 args (ret 0x20), returns 1.
// @early-stop
// 78.3% codegen wall (twin of CLevelPlane::Build): logic/fields/offsets/CFG/args
// byte-faithful (the two log2 shift loops, the strcpy inline rep-movs, the
// CopyRect IAT call + m_bounds50 re-derive, the fild/fmul/__ftol float scale, the
// two operator-new allocations and the column-offset fill all match).  Residual is
// a zero-register-pinning wall: retail assigns the 8 args to edx/eax/ecx/esi/ebp in
// a different rotation than our cl and schedules the ~20 field seeds interleaved
// with the m_30/m_34 products differently.  Same values, same stores; no source
// lever picks the arg->register map (docs/patterns/zero-register-pinning.md).
RVA(0x001619f0, 0x1f7)
i32 CLevelPlane::InitGeometry_1619f0(i32 w, i32 h, i32 tileW, i32 tileH, i32 depthX,
                                     i32 depthY, LevelCoordRect* bounds, char* name) {
    m_width = w;
    m_height = h;
    m_tilePixW = tileW;
    m_tilePixH = tileH;
    m_bounds50.minX = bounds->minX;
    m_bounds50.minY = bounds->minY;
    m_bounds50.maxX = bounds->maxX;
    m_bounds50.maxY = bounds->maxY;
    m_94 = depthX;
    m_98 = depthY;
    m_60 = 0;
    m_64 = 0;
    m_6c = tileH;
    m_wrapW = tileW * w;
    m_wrapH = tileH * h;
    m_68 = tileW;
    i32 pw = m_bounds50.maxX - m_bounds50.minX + 1;
    i32 ph = m_bounds50.maxY - m_bounds50.minY + 1;
    m_viewW = pw;
    m_viewH = ph;
    m_anchorX = pw / 2;
    m_anchorY = ph / 2;
    m_shiftX = 0;
    if (tileW > 1) {
        i32 v = tileW;
        do {
            v >>= 1;
            m_shiftX = m_shiftX + 1;
        } while (v > 1);
    }
    m_shiftY = 0;
    if (tileW > 1) {
        i32 v = tileW;
        do {
            v >>= 1;
            m_shiftY = m_shiftY + 1;
        } while (v > 1);
    }
    if (name != 0) {
        strcpy(m_name, name);
    }
    if (bounds->minX != (i32)0x80000000) {
        LevelCoordRect local;
        CopyRect((RECT*)&local, (RECT*)bounds);
        m_bounds50 = local;
        i32 pw2 = m_bounds50.maxX - m_bounds50.minX + 1;
        i32 ph2 = m_bounds50.maxY - m_bounds50.minY + 1;
        m_viewW = pw2;
        m_viewH = ph2;
        m_anchorX = pw2 / 2;
        m_anchorY = ph2 / 2;
        RecomputePlaneCoords();
    }
    m_scaleX = (float)m_94 * 0.01f;
    m_scaleY = (float)m_98 * 0.01f;
    m_tileGrid = (i32*)operator new(m_width * m_height * 4);
    m_colOffsets = (i32*)operator new(m_height * 4);
    for (i32 i = 0; i < m_height; i++) {
        m_colOffsets[i] = i * m_width;
    }
    m_scaledX = 0;
    m_scaledY = 0;
    RecomputePlaneCoords();
    return 1;
}

// ---------------------------------------------------------------------------
// CLevelPlane::RecomputePlaneCoords - recompute one plane's scaled scroll origin
// and visible-tile extents from its (already-scaled) float coords. __thiscall
// with `this` = the plane (ecx); reloc-masks only the float 0.0 constant and the
// CRT __ftol helper (the (int)float casts). X and Y are computed identically:
// wrap (flags bit set) folds the coord modulo the tile count into [0, count);
// else it clamps to [0, count-1].
RVA(0x00161c90, 0x1e4)
void CLevelPlane::RecomputePlaneCoords() {
    CLevelPlane* p = this;
    u32 flags = p->m_flags;
    i32 wrapX = flags & 4;

    // --- X axis: wrap/clamp scaledX into the tile grid -----------------------
    if (wrapX) {
        if (p->m_scaledX < 0.0f) {
            do {
                p->m_scaledX += (float)p->m_wrapW;
            } while (p->m_scaledX < 0.0f);
        }
        if (p->m_scaledX >= (float)p->m_wrapW) {
            float t = p->m_scaledX;
            do {
                t -= (float)p->m_wrapW;
            } while (t >= (float)p->m_wrapW);
            p->m_scaledX = t;
        }
    } else {
        if (p->m_scaledX < 0.0f) {
            p->m_scaledX = 0;
        } else if ((float)p->m_wrapW <= p->m_scaledX) {
            p->m_scaledX = (float)(p->m_wrapW - 1);
        }
    }

    // --- Y axis: identical wrap/clamp on scaledY/tilesHigh -------------------
    i32 wrapY = flags & 8;
    if (wrapY) {
        if (p->m_scaledY < 0.0f) {
            do {
                p->m_scaledY += (float)p->m_wrapH;
            } while (p->m_scaledY < 0.0f);
        }
        if (p->m_scaledY >= (float)p->m_wrapH) {
            float t = p->m_scaledY;
            do {
                t -= (float)p->m_wrapH;
            } while (t >= (float)p->m_wrapH);
            p->m_scaledY = t;
        }
    } else {
        if (p->m_scaledY < 0.0f) {
            p->m_scaledY = 0;
        } else if ((float)p->m_wrapH <= p->m_scaledY) {
            p->m_scaledY = (float)(p->m_wrapH - 1);
        }
    }

    // --- snap to integer + derive the tile origin ----------------------------
    i32 ix = (i32)p->m_scaledX;
    p->m_originX = ix;
    i32 iy = (i32)p->m_scaledY;
    p->m_originY = iy;

    i32 ox = ix - p->m_anchorX;
    p->m_tileOriginX = ox;
    if (ox < 0) {
        if (wrapX) {
            p->m_tileOriginX = p->m_wrapW + ox;
        } else {
            p->m_tileOriginX = 0;
        }
    }

    i32 oy = iy - p->m_anchorY;
    p->m_tileOriginY = oy;
    if (oy < 0) {
        if (wrapY) {
            p->m_tileOriginY = p->m_wrapH + oy;
        } else {
            p->m_tileOriginY = 0;
        }
    }

    // --- derive the far tile extents (clamped, unless wrapping) ---------------
    i32 ex = p->m_viewW + p->m_tileOriginX - 1;
    i32 ey = p->m_viewH + p->m_tileOriginY - 1;
    p->m_tileExtentX = ex;
    p->m_tileExtentY = ey;
    if (ex >= p->m_wrapW && wrapX == 0) {
        i32 over = ex - p->m_wrapW + 1;
        p->m_tileExtentX = ex - over;
        p->m_tileOriginX = p->m_tileOriginX - over;
    }
    if (ey >= p->m_wrapH && wrapY == 0) {
        i32 over = ey - p->m_wrapH + 1;
        p->m_tileExtentY = ey - over;
        p->m_tileOriginY = p->m_tileOriginY - over;
    }
}

// ===========================================================================
// CLevelPlane::Build (0x161e80) - re-place one plane from the level coord rect.
// Unless the rect is unset (minX == INT_MIN sentinel), copy it into the plane's
// +0x50 bounds, derive the view size (w/h = max-min+1) and the half-size anchor,
// then RecomputePlaneCoords. CGameLevel::SetExtentsAndBuildAll / BuildAllPlanes
// drive it per plane (m_planes[i]).
// @early-stop
// codegen wall (~90.4%): the sentinel gate, the CopyRect(&local, coords) IAT call,
// the 4-dword copy to m_bounds50, the max-min+1 size + cdq/sar half-size and the
// RecomputePlaneCoords tail are all byte-faithful; the residual is a local /O2
// scheduling of the width/height derivation. Complete + correct logic.
// ===========================================================================
RVA(0x00161e80, 0x79)
void CLevelPlane::Build(LevelCoordRect* coords) {
    if (coords->minX != (i32)0x80000000) {
        LevelCoordRect local;
        CopyRect((RECT*)&local, (RECT*)coords);
        m_bounds50 = local;
        i32 width = m_bounds50.maxX - m_bounds50.minX + 1;
        i32 height = m_bounds50.maxY - m_bounds50.minY + 1;
        m_viewW = width;
        m_viewH = height;
        m_anchorX = width / 2;
        m_anchorY = height / 2;
        RecomputePlaneCoords();
    }
}
