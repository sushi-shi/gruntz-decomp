// LevelPlane.cpp - the plane/render module of the level subsystem.
// original TU: plane/render module (@identity-TODO - filename unknown; the
// CDDrawWorkerHost + CDDrawWorkerHost + WwdFile-plane-method + CImageSet3-helper +
// CDDrawWorkerHost bodies at [0x1615a0 .. 0x1638c0])
//
// Consolidated by retail .text birth position (interval dossier 0x15ccd0, wave1-C):
// the plane/render TU is [0x161350 .. 0x163a00] - heavily WOVEN, one obj - holding
//   CDDrawWorkerHost ctor/ReadPlaneBlock-gap/RegisterNamed   (0x1615a0/640/c50)
//   CDDrawWorkerHost InitGeometry/RecomputePlaneCoords/Build      (0x1619f0/c90/e80)
//   CImageSet3 grid-owner leaves Unload/Prune/GetSize        (0x161bf0/0x1628d0/0x1633e0)
//   CDDrawWorkerHost SetTileSize(FromImageSet)/Draw/CenterScrollA+B/InitScrollRects/
//     ValidateTiles/ResolveColorKey/Save/Load (+ the serialize dispatcher)
//   WwdFile RebuildPlanes/ReadPlaneObjects                   (0x1628f0/0x162af0)
// (The 0x161350-0x161558 CImageSet1/2/3 scalar-dtor pocket before the ctor is
// COMDAT-at-usage emission - those classes home elsewhere; ~CDDrawWorkerHost
// @0x163af0 is past the TU end and stays in DDrawWorkerHost.cpp.)
//
// Class definitions stay canonical: <Gruntz/GameLevel.h> (CDDrawWorkerHost + the
// CDDrawWorker view + <Wwd/WwdFile.h> CDDrawWorkerHost/WwdFile; CDDrawWorkerHost == the canonical
// CDDrawWorkerHost, the stream is the real CFileMemBase),
// <DDrawMgr/DDrawWorkerHost.h> (CDDrawWorkerHost), <Gruntz/UserLogic.h>
// (CGameObject). Bodies are strictly RVA-ascending; only offsets + emitted
// bytes are load-bearing (campaign doctrine).
#include <Mfc.h>
#include <Gruntz/WwdGameObject.h> // complete CWwdGameObject: the CGameObject downcast is static
#include <DDrawMgr/PixelShift.h>  // g_rUp/g_gUp/g_bUp/g_rDown/g_gDown/g_bDown
#include <Gruntz/GameLevel.h> // CDDrawWorkerHost + LevelCoordRect + CDDrawWorker view (+ WwdFile.h)
#include <Gruntz/UserLogic.h> // the shared CGameObject (ReadPlaneObjects' 0x1dc object)
#include <Image/CImage.h>     // CImage m_gridW/m_gridH (SetTileSizeFromImageSet)
#include <Image/ImageSet.h> // the REAL CDDrawWorker (0x6c frame collection): SetTileSizeFromImageSet's
#include <DDrawMgr/DDSurface.h>       // CDDSurface::BltEx/BltFast (the Draw blit callees)
#include <DDrawMgr/DDrawWorkerHost.h> // canonical CDDrawWorkerHost (ctor + RegisterNamed here)
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry->m_10map
#include <DDrawMgr/DDrawWorkerCache.h>    // m_workerCache->m_10
#include <DDrawMgr/DDrawWorkerMapSmall.h> // m_workerMap->m_palOwner
#include <DDrawMgr/DDrawSubMgrPages.h>    // m_drawTarget->m_frontPair
#include <DDrawMgr/DDrawSurfacePair.h>    // ->m_bpp (the ex CPlaneSurfDesc::m_format)
#include <DDrawMgr/DDrawChildGroup.h>     // m_childGroup (the worker source)
#include <Io/FileMem.h> // the REAL serialize-stream base CFileMemBase (Save/Load's Read@+0x2c/Write@+0x30)
#include <Wwd/WwdSpatialMgr.h> // the canonical spatial/scroll worker (m_scroll)
#include <rva.h>

#include <stdio.h>  // sprintf (ValidateTiles diagnostics)
#include <string.h> // strcpy/memcpy/memset (inline rep movs / rep stos)

// ---------------------------------------------------------------------------
// The WWD "imageSet3" grid-owner pocket. src/Image/ImageSet3.cpp hosts the same
// object's /GX out-of-line dtor (0x161500, COMDAT-at-usage) + its 0x166e00 pixel
// scan; the three non-EH leaf methods below are birth-positioned INSIDE this
// plane TU. Local view duplicated from that TU (@identity-TODO: the grid-owner's
// name-conflation with the Gruntz CImageSet3 variant record is unresolved).
// The +0xb0 spatial grid is a CWwdSpatialMgr (canonical, <DDrawMgr/DDrawWorkerHost.h>).
// CDDrawWorkerHost::Unload prunes it (PruneCount 0x1688b0), runs its OUT-OF-LINE /GX
// complete dtor (~CWwdSpatialMgr @0x163a40, body in WwdSpatialMgr.cpp; the ex-C163a40
// GetSize (0x168430) is the serialized-size accessor (WwdSpatialMgr.cpp defines it).
// All reloc-masked __thiscall callees (no body).

// the identity was CDDrawWorkerHost all along - the three bodies read +0x20/+0x24/
// +0xb0 = m_buffer0/m_buffer1/m_spatialWorker, and 0x161bf0 IS CDDrawWorkerHost's
// vtable slot 7 per the retail slot map @0x1f0270. The "@identity-TODO name-
// conflation with the Gruntz CImageSet3 variant record" is resolved: the 0x18-byte
// record class (<Gruntz/ImageSets.h>) cannot even hold a +0xb0 member.]

RVA(0x001615a0, 0x9a)
CDDrawWorkerHost::CDDrawWorkerHost(CDDrawSurfaceMgr* mapData, i32 field04, i32 flags) {
    m_id = field04;
    m_flags = flags;
    m_ownerCtx =
        reinterpret_cast<i32>(mapData); // (fused CLoadable ctor stores - the CResolveNode shape)
    // m_frameSets (::CObArray) default-constructed here (0x1b55e9).
    m_tileGrid = 0;
    m_colOffsets = 0;
    m_scroll = 0;
    m_scaleX = 1.0f;
    m_scaleY = 1.0f;
    m_bounds50.left = -1; // pre-Build sentinel (the ex-m_bounds50.left reading, reconciled)
    memset(&m_bltFx, 0, sizeof(m_bltFx));
    m_bltFx.dwSize = sizeof(DDBLTFX); // 100
}

// 0x161640 (930 B) = the plane-block reader, CDDrawWorkerHost vtable slot 10
// (??_7CDDrawWorkerHost @0x1f0270+0x28) - the slot CGameLevel::ReadPlane
// dispatches. __thiscall(planeData record, blockBase, LevelCoordRect* bounds), ret 0xc.
//
// DECODED STRUCTURE (retail-verified 2026-07-13; every callee named):
//   1. Guard: `if (planeData[0] != 0xa0) return 0;` - the WwdPlaneHeader stride.
//   2. Tokenizer over the record's trailing imageset-NAME LIST: base = blockBase +
//      planeData[+0x88], length = planeData[+0x7c]. It walks bytes, treating any char
//      OUTSIDE [0x30, 0x80) (and NUL) as a separator, and copies each token into a
//      0x80-byte stack buffer at [esp+0x28].
//   3. Per token: `m_mapData->...->Lookup(token, out)` (0x1b8008 == CMapStringToOb::
//      Lookup - the SAME name->object registry RegisterNamed resolves through), then
//      `m_frameSets.SetAtGrow(i, out)` (0x1b5822 == CObArray::SetAtGrow) - i.e. it
//      populates the +0x9c frame-set array the Draw loop indexes by handle>>16.
//   4. Copies the record's geometry into `this`, calls SetTileSize (0x161f00), the
//      g_pCopyRect fn-ptr (0x6c44bc) for the bounds rect, and the DAT_005f02a0 float
//      scale - then steps 6-9 are BYTE-IDENTICAL to InitGeometry (two
//      operator_new allocations: the tile grid + the column-offset table, then the
//      tail-call to RecomputePlaneCoords 0x161c90).
//   5. Finally drives RebuildPlanes (0x1628f0) for the object block.
//
// WHAT BLOCKS IT (not a model wall - a cost/plateau judgment): its steps 6-9 ARE
// InitGeometry, and that sibling is itself parked at 78.33% on the documented
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md - retail's
// arg->register rotation over the ~20 field seeds is not source-steerable). A faithful
// 930-B reconstruction here would inherit that same plateau over its largest span, and
// a PARTIAL body is worse than a stub (it under-counts AND diverges the TU's regalloc).
// The honest sequencing is leaf-first: crack InitGeometry's register pinning,
// then this body follows mechanically from the structure above.
// @confidence: high
// @source: vtable_hierarchy-slot-map+ReadPlane-dispatch+full-disasm-decode
// @stub
RVA(0x00161640, 0x3a2)
i32 CDDrawWorkerHost::Read(void* planeData, void* blockBase, void* bounds) {
    return 0;
}

// CDDrawWorkerHost::InitGeometry (0x1619f0, CDDrawWorkerHost vtable slot +0x24):
// seed tile/wrap/origin/shift fields from the 8 args, log2 the tile shifts, strcpy
// the name, alloc the tile grid + column-offset table, tail-call RecomputePlaneCoords.
// __thiscall, 8 args (ret 0x20), returns 1.
// @early-stop
// 78.3% codegen wall (twin of CDDrawWorkerHost::Build): logic/fields/offsets/CFG/args
// byte-faithful (the two log2 shift loops, the strcpy inline rep-movs, the
// CopyRect IAT call + m_bounds50 re-derive, the fild/fmul/__ftol float scale, the
// two operator-new allocations and the column-offset fill all match).  Residual is
// a zero-register-pinning wall: retail assigns the 8 args to edx/eax/ecx/esi/ebp in
// a different rotation than our cl and schedules the ~20 field seeds interleaved
// with the m_30/m_34 products differently.  Same values, same stores; no source
// lever picks the arg->register map (docs/patterns/zero-register-pinning.md).
RVA(0x001619f0, 0x1f7)
i32 CDDrawWorkerHost::InitGeometry(
    i32 w,
    i32 h,
    i32 tileW,
    i32 tileH,
    i32 depthX,
    i32 depthY,
    LevelCoordRect* bounds,
    char* name
) {
    m_gridW = w;
    m_gridH = h;
    m_tilePxW = tileW;
    m_tilePxH = tileH;
    m_bounds50.left = bounds->left;
    m_bounds50.top = bounds->top;
    m_bounds50.right = bounds->right;
    m_bounds50.bottom = bounds->bottom;
    m_94 = depthX;
    m_98 = depthY;
    m_fillRect.left = 0;
    m_fillRect.top = 0;
    m_fillRect.bottom = tileH;
    m_wrapW = tileW * w;
    m_wrapH = tileH * h;
    m_fillRect.right = tileW;
    i32 pw = m_bounds50.right - m_bounds50.left + 1;
    i32 ph = m_bounds50.bottom - m_bounds50.top + 1;
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
    if (bounds->left != static_cast<i32>(0x80000000)) {
        LevelCoordRect local;
        CopyRect((&local), (bounds));
        m_bounds50 = local;
        i32 pw2 = m_bounds50.right - m_bounds50.left + 1;
        i32 ph2 = m_bounds50.bottom - m_bounds50.top + 1;
        m_viewW = pw2;
        m_viewH = ph2;
        m_anchorX = pw2 / 2;
        m_anchorY = ph2 / 2;
        RecomputePlaneCoords();
    }
    m_scaleX = static_cast<float>(m_94) * 0.01f;
    m_scaleY = static_cast<float>(m_98) * 0.01f;
    m_tileGrid = static_cast<i32*>(operator new(m_gridW * m_gridH * 4));
    m_colOffsets = static_cast<i32*>(operator new(m_gridH * 4));
    for (i32 i = 0; i < m_gridH; i++) {
        m_colOffsets[i] = i * m_gridW;
    }
    m_scaledX = 0;
    m_scaledY = 0;
    RecomputePlaneCoords();
    return 1;
}

RVA(0x00161bf0, 0x5e)
void CDDrawWorkerHost::Unload() {
    if (m_scroll != 0) {
        m_scroll->PruneCount();
    }
    CWwdSpatialMgr* g = m_scroll;
    delete g; // ~CWwdSpatialMgr non-virtual, out-of-line (0x163a40) + ??3 (no null-out)
    if (m_tileGrid != 0) {
        ::operator delete(m_tileGrid);
        m_tileGrid = 0;
    }
    if (m_colOffsets != 0) {
        ::operator delete(m_colOffsets);
        m_colOffsets = 0;
    }
}

// ===========================================================================
// 0x161c50 - RegisterNamed(index, key): resolve `key` to a named object through the
// owner context's map (m_mapData -> sub-manager -> +0x10 CMapStringToOb) and cache the
// result (or null on a miss) at m_frameSets[index] (SetAtGrow). __thiscall, ret 8.
// Same lookup chain as CDDrawWorkerB::Helper. m_mapData is the CLoadable base's
// +0x0c owner context (declared i32; the reinterpret is the CLoadable ctx handle).
// ===========================================================================
// @early-stop
// 90.48%: identical Lookup out-param zero-init reorder wall as CDDrawWorkerB::
// Helper - retail emits the `mov [esp+N],0` (val=0) AFTER both Lookup arg
// pushes (push &val / push key), cl emits it BETWEEN them. Verified byte-exact
// elsewhere (llvm-objdump -dr): the only differing bytes are that 1-instruction
// slot. Logic/offsets/both call sites/movsbl-narrowed index all match. Not
// source-steerable (same as Helper's documented note).
RVA(0x00161c50, 0x3f)
void CDDrawWorkerHost::RegisterNamed(char index, const char* key) {
    CObject* val = 0;
    OwnerMgr()->m_imageRegistry->m_10map.Lookup(key, val);
    m_frameSets.SetAtGrow(index, val);
}

RVA(0x00161c90, 0x1e4)
void CDDrawWorkerHost::RecomputePlaneCoords() {
    CDDrawWorkerHost* p = this;
    u32 flags = p->m_flags;
    i32 wrapX = flags & 4;

    // --- X axis: wrap/clamp scaledX into the tile grid -----------------------
    if (wrapX) {
        if (p->m_scaledX < 0.0f) {
            do {
                p->m_scaledX += static_cast<float>(p->m_wrapW);
            } while (p->m_scaledX < 0.0f);
        }
        if (p->m_scaledX >= static_cast<float>(p->m_wrapW)) {
            float t = p->m_scaledX;
            do {
                t -= static_cast<float>(p->m_wrapW);
            } while (t >= static_cast<float>(p->m_wrapW));
            p->m_scaledX = t;
        }
    } else {
        if (p->m_scaledX < 0.0f) {
            p->m_scaledX = 0;
        } else if (static_cast<float>(p->m_wrapW) <= p->m_scaledX) {
            p->m_scaledX = static_cast<float>((p->m_wrapW - 1));
        }
    }

    // --- Y axis: identical wrap/clamp on scaledY/tilesHigh -------------------
    i32 wrapY = flags & 8;
    if (wrapY) {
        if (p->m_scaledY < 0.0f) {
            do {
                p->m_scaledY += static_cast<float>(p->m_wrapH);
            } while (p->m_scaledY < 0.0f);
        }
        if (p->m_scaledY >= static_cast<float>(p->m_wrapH)) {
            float t = p->m_scaledY;
            do {
                t -= static_cast<float>(p->m_wrapH);
            } while (t >= static_cast<float>(p->m_wrapH));
            p->m_scaledY = t;
        }
    } else {
        if (p->m_scaledY < 0.0f) {
            p->m_scaledY = 0;
        } else if (static_cast<float>(p->m_wrapH) <= p->m_scaledY) {
            p->m_scaledY = static_cast<float>((p->m_wrapH - 1));
        }
    }

    // --- snap to integer + derive the tile origin ----------------------------
    i32 ix = static_cast<i32>(p->m_scaledX);
    p->m_snappedX = ix;
    i32 iy = static_cast<i32>(p->m_scaledY);
    p->m_snappedY = iy;

    i32 ox = ix - p->m_anchorX;
    p->m_originX = ox;
    if (ox < 0) {
        if (wrapX) {
            p->m_originX = p->m_wrapW + ox;
        } else {
            p->m_originX = 0;
        }
    }

    i32 oy = iy - p->m_anchorY;
    p->m_originY = oy;
    if (oy < 0) {
        if (wrapY) {
            p->m_originY = p->m_wrapH + oy;
        } else {
            p->m_originY = 0;
        }
    }

    // --- derive the far tile extents (clamped, unless wrapping) ---------------
    i32 ex = p->m_viewW + p->m_originX - 1;
    i32 ey = p->m_viewH + p->m_originY - 1;
    p->m_extentX = ex;
    p->m_extentY = ey;
    if (ex >= p->m_wrapW && wrapX == 0) {
        i32 over = ex - p->m_wrapW + 1;
        p->m_extentX = ex - over;
        p->m_originX = p->m_originX - over;
    }
    if (ey >= p->m_wrapH && wrapY == 0) {
        i32 over = ey - p->m_wrapH + 1;
        p->m_extentY = ey - over;
        p->m_originY = p->m_originY - over;
    }
}

// ===========================================================================
// CDDrawWorkerHost::Build (0x161e80) - re-place one plane from the level coord rect.
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
void CDDrawWorkerHost::Build(LevelCoordRect* coords) {
    if (coords->left != static_cast<i32>(0x80000000)) {
        LevelCoordRect local;
        CopyRect((&local), (coords));
        m_bounds50 = local;
        i32 width = m_bounds50.right - m_bounds50.left + 1;
        i32 height = m_bounds50.bottom - m_bounds50.top + 1;
        m_viewW = width;
        m_viewH = height;
        m_anchorX = width / 2;
        m_anchorY = height / 2;
        RecomputePlaneCoords();
    }
}

// ===========================================================================
// CDDrawWorkerHost::SetTileSize (__thiscall, ret 8) - given the tile pixel
// size (tileW, tileH), derive the plane's pixel-wrap dims (grid count * tile px),
// the tile px size, the (0,0,tileW,tileH) default fill rect, and the two log2
// shift amounts. The retail code derives BOTH shifts from tileW (the shiftY loop
// reuses the width, not the height - reproduced verbatim).
//
// @early-stop
// scheduling/regalloc wall (~88%): body byte-exact, but retail loads arg1 before
// the callee-save pushes (product in edi) and parks the shiftY accumulator in esi;
// cl loads m_gridW before the pushes (product in edx) and reuses edi for shiftY.
// Operand-order swaps don't move it; not source-steerable.
// ===========================================================================
RVA(0x00161f00, 0x75)
void CDDrawWorkerHost::SetTileSize(i32 tileW, i32 tileH) {
    m_wrapW = m_gridW * tileW;
    m_tilePxH = tileH;
    m_fillRect.bottom = tileH;
    m_tilePxW = tileW;
    m_fillRect.left = 0;
    m_fillRect.top = 0;
    m_fillRect.right = tileW;
    m_wrapH = m_gridH * tileH;
    m_shiftX = 0;
    for (i32 t = tileW; t > 1; t >>= 1) {
        m_shiftX++;
    }
    m_shiftY = 0;
    for (i32 u = tileW; u > 1; u >>= 1) {
        m_shiftY++;
    }
}

RVA(0x00161fa0, 0x6c)
void CDDrawWorkerHost::SetTileSizeFromImageSet(CDDrawWorker* set) {
    for (i32 i = 0; i < set->m_items.GetSize(); i++) {
        if (set->GetAt(i) != 0) {
            CImage* f = set->GetAt(i);
            SetTileSize(f->m_width, f->m_height);
            break;
        }
    }
}

#define DRAW_CELL(handle, xp, yp, srcp)                                                            \
    do {                                                                                           \
        u32 h_ = static_cast<u32>(handle);                                                         \
        if (h_ == 0xeeeeeeee) {                                                                    \
            dr.left = (xp);                                                                        \
            dr.top = (yp);                                                                         \
            dr.right = (xp) + ((srcp)->right - (srcp)->left);                                      \
            dr.bottom = (yp) + ((srcp)->bottom - (srcp)->top);                                     \
            surf->BltEx(&dr, 0, 0, 0x1000400, &m_bltFx);                                           \
        } else if (h_ != 0xffffffff) {                                                             \
            CPlaneFrame* fr_ = (reinterpret_cast<CPlaneFrame**>(m_frameSets.GetData()))[h_ >> 16]; \
            i32 idx_ = static_cast<i32>(h_ & 0xffff);                                              \
            CPlaneTile* e_;                                                                        \
            if (idx_ >= fr_->m_lo && idx_ <= fr_->m_hi) {                                          \
                e_ = fr_->m_frames[idx_];                                                          \
            } else {                                                                               \
                e_ = 0;                                                                            \
            }                                                                                      \
            surf->BltFast((xp), (yp), e_->m_src, (srcp), e_->m_trans);                             \
        }                                                                                          \
    } while (0)

// @early-stop
// Complete reconstruction of the 2237-byte toroidally-wrapped tile-grid renderer
// (~80.6%, up from a 0.1% bare stub). The five-band walk (top row: TL corner /
// top strip / TR corner; per interior row: left col / interior cols / right col;
// bottom row: BL / bottom strip / BR), the per-region clip math, the column/row
// wrap (mod m_gridW / m_gridH), the handle resolution and the BltEx/BltFast
// callees are all reproduced; the frame (0x94) now matches retail exactly. Parked
// on a whole-function regalloc/scheduling wall (permuter-confirmed: an operand-
// order search moved it only 80.613 -> 80.615): retail pins viewX->ebx /
// viewY->edi where cl swaps them, and reuses the shiftX register (ebp) both to
// zero-init the src-rect left/top and to hold the deferred ctx->m_surface load
// where cl keeps the surface live in its own register from the top; the per-site
// dest-rect operand order (add-then-sub vs sub-then-add) and per-loop counter
// slot numbering also diverge. Logic + offsets + CFG byte-faithful; a leaf-first
// regalloc grind is deferred to the final sweep.
RVA(0x00162010, 0x8bd)
void CDDrawWorkerHost::Draw(CPlaneDrawCtx* ctx) {
    if ((m_flags & 2) != 0) {
        return;
    }
    CDDSurface* surf = ctx->m_surface;

    i32 colL = m_originX >> m_shiftX;
    i32 leftW = ((colL + 1) << m_shiftX) - m_originX;
    i32 rowT = m_originY >> m_shiftY;
    i32 topH = ((rowT + 1) << m_shiftY) - m_originY;
    i32 colR = m_extentX >> m_shiftX;
    i32 rightW = m_extentX - (colR << m_shiftX) + 1;
    i32 rowB = m_extentY >> m_shiftY;
    i32 botH = m_extentY - (rowB << m_shiftY) + 1;
    i32 nCols = colR - colL - 1;
    i32 nRows = rowB - rowT - 1;

    RECT topSrc = {0, m_tilePxH - topH, m_tilePxW, m_tilePxH};   // top strip: clip top
    RECT leftSrc = {m_tilePxW - leftW, 0, m_tilePxW, m_tilePxH}; // left col: clip left
    RECT rightSrc = {0, 0, rightW, m_tilePxH};                   // right col: clip right
    RECT corner;                                                 // reused, four corners
    RECT dr;                                                     // shared BltEx dest rect

    i32 x, y, col, row, i;
    i32 rowBase;

    // ---- top row: TL corner, top strip, TR corner ----
    y = m_bounds50.top;
    x = m_bounds50.left;
    rowBase = m_colOffsets[rowT];
    corner.left = m_tilePxW - leftW;
    corner.top = m_tilePxH - topH;
    corner.right = m_tilePxW;
    corner.bottom = m_tilePxH;
    DRAW_CELL(m_tileGrid[rowBase + colL], x, y, &corner);
    x += leftW;
    col = colL + 1;
    if (col >= m_gridW) {
        col = 0;
    }
    for (i = nCols; i > 0; i--) {
        DRAW_CELL(m_tileGrid[rowBase + col], x, y, &topSrc);
        x += m_tilePxW;
        if (++col >= m_gridW) {
            col = 0;
        }
    }
    corner.left = 0;
    corner.top = m_tilePxH - topH;
    corner.right = rightW;
    corner.bottom = m_tilePxH;
    DRAW_CELL(m_tileGrid[rowBase + col], x, y, &corner);

    // ---- interior rows: left col, interior cols, right col ----
    y += topH;
    row = rowT + 1;
    if (row >= m_gridH) {
        row = 0;
    }
    for (i32 r = nRows; r > 0; r--) {
        rowBase = m_colOffsets[row];
        x = m_bounds50.left;
        DRAW_CELL(m_tileGrid[rowBase + colL], x, y, &leftSrc);
        x += leftW;
        col = colL + 1;
        if (col >= m_gridW) {
            col = 0;
        }
        for (i = nCols; i > 0; i--) {
            DRAW_CELL(m_tileGrid[rowBase + col], x, y, &m_fillRect);
            x += m_tilePxW;
            if (++col >= m_gridW) {
                col = 0;
            }
        }
        DRAW_CELL(m_tileGrid[rowBase + col], x, y, &rightSrc);
        y += m_tilePxH;
        if (++row >= m_gridH) {
            row = 0;
        }
    }

    // ---- bottom row: BL corner, bottom strip, BR corner ----
    RECT botSrc = {0, 0, m_tilePxW, botH}; // bottom strip: clip bottom
    x = m_bounds50.left;
    rowBase = m_colOffsets[row];
    corner.left = m_tilePxW - leftW;
    corner.top = 0;
    corner.right = m_tilePxW;
    corner.bottom = botH;
    DRAW_CELL(m_tileGrid[rowBase + colL], x, y, &corner);
    x += leftW;
    col = colL + 1;
    if (col >= m_gridW) {
        col = 0;
    }
    for (i = nCols; i > 0; i--) {
        DRAW_CELL(m_tileGrid[rowBase + col], x, y, &botSrc);
        x += m_tilePxW;
        if (++col >= m_gridW) {
            col = 0;
        }
    }
    corner.left = 0;
    corner.top = 0;
    corner.right = rightW;
    corner.bottom = botH;
    DRAW_CELL(m_tileGrid[rowBase + col], x, y, &corner);
}
#undef DRAW_CELL

#include <Gruntz/Loadable.h>
inline void* operator new(u32, void* p) {
    return p;
} // placement (embedded sub-object ctor)

RVA(0x001628d0, 0x12)
i32 CDDrawWorkerHost::Prune() {
    if (m_scroll == 0) {
        return 0;
    }
    return m_scroll->PruneCount();
}

// @early-stop
// throwing-new EH-frame + embedded-vtable-stamp wall: the worker rebuild + the
// 6-pair init + the ReadPlaneObjects loop are faithful, but (a) the
// partial-construct exception cleanup frame's trylevel/handler bytes are not
// source-steerable, and (b) the worker's EMBEDDED sub-object at +0x70 has its
// vtable stamped manually (g_planeRenderVtbl @0x5f02a8 = ??_7CWwdGridIter, realized
// in WwdSpatialMgr.cpp; and g_wapObjectDtorVtbl @0x5e8cb4 = the CObject base-dtor
// table on the fail path). The worker's own +0x00 vptr is ZEROED here (not a
// polymorphic outer object), so a plain `new CWwdSpatialMgr` cannot express this; the
// embedded-object-at-offset re-stamp is the only expressible form (wall).
RVA(0x001628f0, 0x1fc)
i32 CDDrawWorkerHost::RebuildPlanes(i32 base, i32 count) {
    if (base == 0) {
        return 0;
    }

    CWwdSpatialMgr*& worker = m_scroll;
    if (worker) {
        worker->FreeGrids();
        worker->m_iter.~CWwdGridIter();
        ::operator delete(worker);
        worker = 0;
    }

    // The shared grid rect Init hands each grid ctor: (0, 0, m_wrapW-1, m_wrapH-1)
    // from this worker host's own plane extents.
    RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = m_wrapW - 1;
    rc.bottom = m_wrapH - 1;

    CDDrawSurfaceMgr* reg = OwnerMgr();
    CDDrawChildGroup* src = reg->m_childGroup;
    if (src == 0) {
        return 0;
    }
    CGameLevel* hdr = reg->m_level;
    if (hdr == 0) {
        return 0;
    }

    // The six geometry pairs Init reads: the three Setup cell-size pairs (m_pairA/B/C
    // @0xb0/0xb8/0xc0) then the three grid rect/origin dim pairs (m_rectA/B/C @0xc8/0xd0/0xd8).
    i32 p0[2] = {hdr->m_pairA[0], hdr->m_pairA[1]};
    i32 p1[2] = {hdr->m_pairB[0], hdr->m_pairB[1]};
    i32 p2[2] = {hdr->m_pairC[0], hdr->m_pairC[1]};
    i32 p3[2] = {hdr->m_rectAWidth, hdr->m_rectAHeight};
    i32 p4[2] = {hdr->m_rectBWidth, hdr->m_rectBHeight};
    i32 p5[2] = {hdr->m_rectCWidth, hdr->m_rectCHeight};

    CWwdSpatialMgr* nw = static_cast<CWwdSpatialMgr*>(::operator new(0xb8));
    if (nw) {
        // the factory's raw seed (retail skips the iter vptr install): zero the
        // iter cursor pair, the mgr/grid slots and the cursor-grid latch - typed.
        nw->m_iter.m_grid = 0;
        nw->m_iter.m_cur = 0;
        nw->m_mgr = 0;
        nw->m_grid0 = 0;
        nw->m_grid1 = 0;
        nw->m_grid2 = 0;
        nw->m_curGrid = 0;
    }
    worker = nw;
    if (nw->Init(src, &rc, p0, p1, p2, p3, p4, p5) == 0) {
        CWwdSpatialMgr* w = m_scroll;
        if (w) {
            w->FreeGrids();
            // base-subobject vptr restore is compiler-managed via the CObject base; manual g_wapObjectDtorVtbl stamp dropped (% ok)
            ::operator delete(w);
        }
        worker = 0;
        return 0;
    }

    for (i32 i = 0; i < count; i++) {
        i32 r = ReadPlaneObjects(reinterpret_cast<const i32*>(base));
        if (r == 0) {
            return 0;
        }
        base += r;
    }
    return 1;
}

// @early-stop
// RE-TESTED 2026-07-13: the recorded "non-ctor factory-stamp wall" is STALE. It
// described a manual `*(void**)obj = &g_wwdObjVtbl` re-stamp pair - and there is no
// such stamp in this body any more (they were dropped in the all-vtables-real batch;
// only the three "vptr install dropped" comments remain). Nothing here is parked on a
// vtable model at all. What IS left is ordinary residue of a 2054-byte factory whose
// two object ctors (0x15b390 / 0x156cb0) are unmatched engine code: the /GX frame over
// four destructible CString temps + the reloc-masked ctor/Load/Apply* call chain.
// Logic byte-faithful; deferred to the final sweep on size, not on a model wall.
RVA(0x00162af0, 0x806)
i32 CDDrawWorkerHost::ReadPlaneObjects(const i32* src) {
    if (src == 0) {
        return 0;
    }

    i32 id = src[0];
    u32 nameLen = static_cast<u32>(src[1]);
    u32 logicLen = static_cast<u32>(src[2]);
    u32 imageSetLen = static_cast<u32>(src[3]);
    u32 soundLen = static_cast<u32>(src[4]);
    i32 x = src[5];
    i32 y = src[6];
    i32 z = src[7];
    i32 gridIndex = src[8];

    CWwdGameObjectA* obj = static_cast<CWwdGameObjectA*>(operator new(0x1dc));
    if (obj == 0) {
        return 0;
    }

    obj->Construct(OwnerMgr(), id, 0);

    // Construct the embedded sub-object at +0x1A0, then re-stamp both vtables (the
    // base ctors leave a base vtable; ReadPlaneObjects promotes both to their
    // derived types) and zero the trailing fields the derived layout adds.
    new (static_cast<void*>(&obj->m_1a0))
        CLoadable(m_ownerCtx, id, 0); // the embedded loadable (ctor 0x156cb0)
    // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
    obj->m_1a0.m_10 = 0;
    obj->m_1a0.m_14 = 0;
    obj->m_1a0.m_element = 0;

    // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
    obj->m_18c = -1;
    obj->m_190 = -1;
    obj->m_layer = 0;
    obj->m_194 = 0;
    obj->m_19c = 0;

    // Copy the four trailing length-prefixed strings into stack CStrings. They
    // begin right after the fixed 0x11C record.
    const char* strCursor = reinterpret_cast<const char*>(src) + 0x11c;
    char buf[0x400];

    i32 n;
    n = static_cast<i32>(nameLen);
    if (n > 0) {
        memcpy(buf, strCursor, n);
        strCursor += n;
    }
    buf[n] = 0;
    CString name(buf);

    n = static_cast<i32>(logicLen);
    if (n > 0) {
        memcpy(buf, strCursor, n);
        strCursor += n;
    }
    buf[n] = 0;
    CString logic(buf);

    n = static_cast<i32>(imageSetLen);
    if (n > 0) {
        memcpy(buf, strCursor, n);
        strCursor += n;
    }
    buf[n] = 0;
    CString imageSet(buf);

    n = static_cast<i32>(soundLen);
    if (n > 0) {
        memcpy(buf, strCursor, n);
        strCursor += n;
    }
    buf[n] = 0;
    CString sound(buf);

    // Grid bounds check on x/y; failure deletes the object and returns the bytes
    // consumed so far (so the caller still advances over the bad record).
    if (x < 0 || x >= m_wrapW || y < 0 || y >= m_wrapH) {
        reinterpret_cast<WwdRetailSlot16Facet*>(obj)->Delete(
            1
        ); // retail BARE scalar-delete call (no null guard - plain `delete` adds one)
        return static_cast<i32>((strCursor - reinterpret_cast<const char*>(src)));
    }

    // If an image set is named, require it to be present in the level map.
    i32 loaded = 1;
    if (imageSet.GetLength() != 0) {
        void* found = 0;
        CObject* foundOb = 0;
        loaded =
            OwnerMgr()->m_workerCache->m_10.Lookup(static_cast<const char*>(imageSet), foundOb);
        found = foundOb;
    }

    if (!loaded) {
        reinterpret_cast<WwdRetailSlot16Facet*>(obj)->Delete(
            1
        ); // retail BARE scalar-delete call (no null guard - plain `delete` adds one)
        return static_cast<i32>((strCursor - reinterpret_cast<const char*>(src)));
    }

    // Run the object's load virtual (reads the fixed record into the object).
    if (obj->Setup(static_cast<i32>(logicLen), id, reinterpret_cast<i32>(strCursor), id) == 0) {
        reinterpret_cast<WwdRetailSlot16Facet*>(obj)->Delete(
            1
        ); // retail BARE scalar-delete call (no null guard - plain `delete` adds one)
        return 0;
    }

    obj->m_flags |= 0x40000;

    AnimWorkerObj* anim = obj->m_7c;
    if (anim == 0) {
        reinterpret_cast<WwdRetailSlot16Facet*>(obj)->Delete(
            1
        ); // retail BARE scalar-delete call (no null guard - plain `delete` adds one)
        return 0;
    }

    i32* sub = reinterpret_cast<i32*>(anim);

    // Apply name -> sprite first-frame cache (indexed when src[?] != -1).
    if (logic.GetLength() != 0) {
        if (z != -1) {
            obj->ApplyLookupSprite(static_cast<const char*>(logic), z);
        } else {
            obj->ApplyName(static_cast<const char*>(logic));
        }
    }

    // Apply sound -> anim geometry + logic.
    if (sound.GetLength() != 0) {
        obj->ApplyLookupGeometry(static_cast<const char*>(sound), 0);
        obj->LookupAnimSprite(static_cast<const char*>(sound));
    }

    // Apply imageSet -> the object's +0xdc name CString.
    if (imageSet.GetLength() != 0) {
        obj->m_dc = static_cast<const char*>(imageSet);
    }

    // Scatter the trailing record fields. `p` advances through the record from
    // its dynamic-flags field onward.
    const i32* p = &src[10]; // record +0x28 (skip addFlags @+0x24)

    obj->m_flags |= static_cast<u32>(*p++); // dynamicFlags       (+0x08)
    obj->m_stateFlags = *p++;               // drawFlags          (+0x40)
    sub[0x28 / 4] = *p++;                   // userFlags
    // The six-int "user-value" union (+0x114..+0x128). These are the WWD object
    // record's canonical Score/Points/Powerup/Damage/Smarts/Health fields (the
    // names the Gruntz Level Editor's Edit-Objects "Attributes" dialog uses), each
    // REINTERPRETED per CUserLogic leaf - e.g. for a GruntStartingPoint enemy Grunt
    // Points=AI type (1-16), Smarts=team (0-3), Powerup=carried Tool/Toy id; for a
    // CoveredPowerup Powerup=covered object id (0-99), Smarts=revealed tile, Score=
    // megaphone order. Same physical fields, different views (this is why UserLogic.h
    // labels them by their spotlight/teleporter meaning). Authoritative field
    // semantics + the id spaces: docs/domain/README.md.
    obj->m_114 = *p++;               // score              (+0x114)
    obj->m_118 = *p++;               // points  (enemy AI type / megaphone tool id)   (+0x118)
    obj->m_11c = *p++;               // powerup (CoveredPowerup id 0-99 / carried tool) (+0x11c)
    obj->m_120 = *p++;               // damage             (+0x120)
    obj->m_124 = *p++;               // smarts  (enemy team 0-3 / revealed tile)       (+0x124)
    obj->m_placeMode = *p++;         // health             (+0x128)
    obj->m_extent.left = *p++;       // moveRect.l         (+0x134)
    obj->m_extent.top = *p++;        // moveRect.t         (+0x138)
    obj->m_extent.right = *p++;      // moveRect.r         (+0x13c)
    obj->m_extent.bottom = *p++;     // moveRect.b         (+0x140)
    obj->m_area.left = *p++;         // hitRect.l          (+0x144)
    obj->m_area.top = *p++;          // hitRect.t          (+0x148)
    obj->m_area.right = *p++;        // hitRect.r          (+0x14c)
    obj->m_area.bottom = *p++;       // hitRect.b          (+0x150)
    obj->m_switchRect.left = *p++;   // attackRect.l       (+0x154)
    obj->m_switchRect.top = *p++;    // attackRect.t       (+0x158)
    obj->m_switchRect.right = *p++;  // attackRect.r       (+0x15c)
    obj->m_switchRect.bottom = *p++; // attackRect.b       (+0x160)
    obj->m_clip.left = *p++;         // clipRect.l         (+0x64)
    obj->m_clip.top = *p++;          // clipRect.t         (+0x68)
    obj->m_clip.right = *p++;        // clipRect.r         (+0x6c)
    obj->m_clip.bottom = *p++;       // clipRect.b         (+0x70)

    if (obj->m_area.left == 0 && obj->m_area.right == 0) {
        obj->m_area.left = static_cast<i32>(0x80000000);
    }
    if (obj->m_extent.left == 0 && obj->m_extent.right == 0) {
        obj->m_extent.left = static_cast<i32>(0x80000000);
    }
    if (obj->m_clip.left == 0 && obj->m_clip.right == 0) {
        obj->m_clip.left = static_cast<i32>(0x80000000);
    }
    if (obj->m_switchRect.left == 0 && obj->m_switchRect.right == 0) {
        obj->m_switchRect.left = static_cast<i32>(0x80000000);
    }

    sub[0xf0 / 4] = *p++;
    sub[0xf4 / 4] = *p++;
    sub[0xf8 / 4] = *p++;
    sub[0xfc / 4] = *p++;
    sub[0x100 / 4] = *p++;
    sub[0x104 / 4] = *p++;
    sub[0x108 / 4] = *p++;
    sub[0x10c / 4] = *p++;
    sub[0x64 / 4] = *p++;
    sub[0x68 / 4] = *p++;
    sub[0x6c / 4] = *p++;
    sub[0x70 / 4] = *p++;
    sub[0x74 / 4] = *p++;
    sub[0x78 / 4] = *p++;
    sub[0x7c / 4] = *p++;
    sub[0x80 / 4] = *p++;
    sub[0x2c / 4] = *p++;
    sub[0x34 / 4] = *p++;
    sub[0x30 / 4] = *p++;
    sub[0x38 / 4] = *p++;
    obj->m_164 = *p++;
    obj->m_168 = *p++;
    sub[0x44 / 4] = *p++;
    sub[0x48 / 4] = *p++;
    sub[0xb8 / 4] = *p++;
    sub[0xbc / 4] = *p++;
    sub[0xc8 / 4] = *p++;
    sub[0xcc / 4] = *p++;
    obj->m_12c = *p++;
    obj->m_130 = *p++;
    sub[0x20 / 4] = *p++;
    sub[0x24 / 4] = *p++;
    obj->m_collCategory = *p++; // +0xe8
    obj->m_ec = *p++;           // +0xec

    u32 w = static_cast<u32>(*p++);
    if (w > 0) {
        obj->m_strideX = static_cast<i32>(w); // +0xf8
    }
    u32 h = static_cast<u32>(*p++);
    if (h > 0) {
        obj->m_strideY = static_cast<i32>(h); // +0xfc
    }

    // Retail: `mov ecx,[this+0xb0]; call 0x1688f0` - it LOADS the spatial worker and
    // registers the object with it. (The old view took the ADDRESS of +0xb0 and called
    // CObList::AddTail on it - a `lea` where retail emits a `mov`, and a mis-bound
    // NAFXCW symbol; +0xb0 holds a POINTER, as RebuildPlanes' `new(0xb8)` store proves.)
    m_scroll->RemoveObject(static_cast<CWwdGameObject*>(obj));

    return static_cast<i32>((strCursor - reinterpret_cast<const char*>(src)));
}

// ---------------------------------------------------------------------------
// CDDrawWorkerHost::CenterScrollA / CenterScrollB (__thiscall, returns int). Compute
// a scroll target for the plane's camera sub-object (+0xB0) and hand it to the
// camera's SetTarget (returning its result). When the plane wraps an axis (flag
// bit2=X, bit3=Y) the target is the (int) scroll origin (m_scaledX/Y); otherwise
// it is the rect mid-point ((origin+extent)/2 + 1). A and B differ only in the
// camera method called (0x168340 vs 0x168500) and the symmetric mid-point pairing.
//
// @early-stop
// 87.9%, logic byte-exact (the int return + `return 0` guard restored retail's
// inline epilogues, 83.5%->87.9%). Two residuals, both uncontrollable MSVC5
// scheduling/regalloc: (1) retail SHRINK-WRAPS the callee-save pushes - only
// ebp/esi before the null guard, edi/ebx after it passes - while this build pushes
// all four upfront; (2) the mid-point `add` loads m_40-first (A) / m_48-first (B)
// in retail but this build loads the higher-offset field first regardless of
// source operand order. Documented prologue/member-load scheduling wall. See
// docs/patterns/shrink-wrapped-callee-save-push.md.
RVA(0x00163300, 0x70)
i32 CDDrawWorkerHost::CenterScrollA() {
    CWwdSpatialMgr* scroll = m_scroll;
    if (scroll == 0) {
        return 0;
    }

    u32 flags = m_flags;

    i32 x;
    if (flags & 0x4) {
        x = static_cast<i32>(m_scaledX);
    } else {
        x = (m_originX + m_extentX) / 2 + 1;
    }

    i32 y;
    if (flags & 0x8) {
        y = static_cast<i32>(m_scaledY);
        return scroll->ScrollTo(x, y);
    }
    y = (m_originY + m_extentY) / 2 + 1;
    return scroll->ScrollTo(x, y);
}

// @early-stop
// 87.9%, same shrink-wrapped-push / member-load scheduling wall as CenterScrollA.
RVA(0x00163370, 0x70)
i32 CDDrawWorkerHost::CenterScrollB() {
    CWwdSpatialMgr* scroll = m_scroll;
    if (scroll == 0) {
        return 0;
    }

    u32 flags = m_flags;

    i32 x;
    if (flags & 0x4) {
        x = static_cast<i32>(m_scaledX);
    } else {
        x = (m_extentX + m_originX) / 2 + 1;
    }

    i32 y;
    if (flags & 0x8) {
        y = static_cast<i32>(m_scaledY);
        return scroll->Relocate(x, y);
    }
    y = (m_extentY + m_originY) / 2 + 1;
    return scroll->Relocate(x, y);
}

RVA(0x001633e0, 0x12)
i32 CDDrawWorkerHost::GetSize() {
    if (m_scroll == 0) {
        return 0;
    }
    return m_scroll->GetSize();
}

RVA(0x00163420, 0xf0)
void CDDrawWorkerHost::InitScrollRects() {
    if (m_scroll == 0) {
        return;
    }
    CGameLevel* g = OwnerMgr()->m_level;
    if (g == 0) {
        return;
    }

    i32 c8 = g->m_rectAWidth;
    i32 cc = g->m_rectAHeight;
    i32 d0 = g->m_rectBWidth;
    i32 d4 = g->m_rectBHeight;
    i32 d8 = g->m_rectCWidth;
    i32 dc = g->m_rectCHeight;

    CWwdSpatialMgr* s = m_scroll;
    s->m_rect0Left = 0;
    s->m_rect0Top = 0;
    s->m_rect0Right = c8 - 1;
    s->m_rect0Bottom = cc - 1;
    s->m_org0x = c8 / 2;
    s->m_org0y = cc / 2;

    s = m_scroll;
    s->m_rect1Left = 0;
    s->m_rect1Top = 0;
    s->m_rect1Right = d0 - 1;
    s->m_rect1Bottom = d4 - 1;
    s->m_org1x = d0 / 2;
    s->m_org1y = d4 / 2;

    s = m_scroll;
    s->m_rect2Left = 0;
    s->m_rect2Top = 0;
    s->m_rect2Right = d8 - 1;
    s->m_rect2Bottom = dc - 1;
    s->m_org2x = d8 / 2;
    s->m_org2y = dc / 2;

    s = m_scroll;
    s->m_scrollX = -22222;
    s->m_scrollY = -22222;
}

// ---------------------------------------------------------------------------
// CDDrawWorkerHost::ValidateTiles (__thiscall, ret 0x4). When the plane is loaded
// (vtable +0x14), walk the row-major tile grid: each handle (skipping the -1 and
// 0xEEEEEEEE sentinels) must resolve to a non-null plane frame (m_planeArray
// [handle>>16]) and an in-range tile value; on a bad ref, if `errOut` is non-null,
// format the diagnostic ("Plane %s: Bad map image set value" / "...tile value")
// into it. Returns 1.
//
// @early-stop
// 92.5%, logic byte-exact (the double loop, both sentinels, the frame/tile range
// checks, both sprintf+strcpy error paths, and the result/dead-flag stack pair all
// match retail). Residual is the MSVC5 inlined-sprintf/strcpy register scheduling
// across the two error sites - a documented entropy/scheduling tail.
RVA(0x00163510, 0x156)
i32 CDDrawWorkerHost::ValidateTiles(char* errOut) {
    if (IsLoaded() == 0) { // the class's own vtable slot 5 (+0x14, 0x163a90)
        return 0;
    }

    char msg[0x80];
    i32 result = 1;
    for (i32 row = 0; row < m_gridH; row++) {
        for (i32 col = 0; col < m_gridW; col++) {
            i32 handle = m_tileGrid[m_colOffsets[row] + col];
            if (handle == -1 || static_cast<u32>(handle) == 0xeeeeeeee) {
                continue;
            }
            CPlaneFrame* frame = (reinterpret_cast<CPlaneFrame**>(
                m_frameSets.GetData()
            ))[static_cast<u32>(handle) >> 16];
            if (frame == 0) {
                result = 0;
                if (errOut != 0) {
                    sprintf(
                        msg,
                        "Plane %s: Bad map image set value (%i) at %i,%i\n",
                        m_name,
                        static_cast<u32>(handle) >> 16,
                        col,
                        row
                    );
                    strcpy(errOut, msg);
                }
                continue;
            }
            i32 tile = handle & 0xffff;
            void* resolved;
            if (tile >= frame->m_lo && tile <= frame->m_hi) {
                resolved = frame->m_frames[tile];
            } else {
                resolved = 0;
            }
            if (resolved == 0) {
                result = 0;
                if (errOut != 0) {
                    sprintf(
                        msg,
                        "Plane %s: Bad map tile value (%i) at %i,%i\n",
                        m_name,
                        tile,
                        col,
                        row
                    );
                    strcpy(errOut, msg);
                }
            }
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// CDDrawWorkerHost::ResolveColorKey (__thiscall, no args). For a 16bpp plane only
// (skip 8bpp), pack the RGB888 palette entry at index m_colorKey (m_mapData's
// palette chain) into a screen-native RGB565 word and store it back in place at
// m_colorKey.
//
// @early-stop
// 66.6%, logic byte-exact (the format gate, the index bounds, the palette chain,
// and the RGB565 pack spelling are the proven-exact SpriteRef idiom). Residual is a
// whole-function regalloc wall: retail pins `this` in ebp (freeing esi/edi for the
// rgb/index pair) and accumulates the pack in eax; our cl pins `this` in edi and
// accumulates in edx. Not source-steerable (the live-range allocation differs once
// rgb/index come from memory rather than register locals). docs/patterns/
// zero-register-pinning.md family.
RVA(0x00163670, 0x95)
void CDDrawWorkerHost::ResolveColorKey() {
    i32 format = OwnerMgr()->m_drawTarget->m_frontPair->m_bpp;
    if (format == 8) {
        return;
    }
    if (format != 0x10) {
        return;
    }

    i32 idx = m_bltFx.dwFillColor;
    if (idx < 0) {
        return;
    }
    if (idx > 0xff) {
        return;
    }

    // The cached worker's palette chain (+0x64 -> +0x10 -> +0x0c RGB888). The cast
    // is the flagged @identity-TODO tail of the cascade: the slot's element type is
    // the map's CObject*, and this worker's concrete palette-bearing class is the one
    // link no caller/new-site names (see <Wwd/WwdFile.h>).
    CPlanePalOwner* owner =
        reinterpret_cast<CPlanePalOwner*>(OwnerMgr()->m_workerMap->m_cachedWorker);
    if (owner == 0) {
        return;
    }
    u8* rgb = owner->m_palette->m_rgb;
    if (rgb == 0) {
        return;
    }

    m_bltFx.dwFillColor = static_cast<u16>(
        ((static_cast<u8>((static_cast<u8>(rgb[idx * 4 + 0]) >> static_cast<u8>(g_rDown))) << g_rUp)
         | (static_cast<u8>((static_cast<u8>(rgb[idx * 4 + 1]) >> static_cast<u8>(g_gDown)))
            << g_gUp)
         | static_cast<u8>((static_cast<u8>(rgb[idx * 4 + 2]) >> static_cast<u8>(g_bDown))))
    );
}

// @early-stop
// jump-table-shape wall (~84%): retail lowers the kind switch (cases 3..8, only 4 and 7
// active) to a dense `jmp [eax*4+table]`; MSVC here folds the 4 default-equal cases and
// emits a compare ladder. Forcing 6 explicit cases still merges them (78%); the 2-case
// ladder is closest. Logic complete.
RVA(0x00163710, 0x42)
i32 CDDrawWorkerHost::SerializeDispatch(CFileMemBase* s, i32 kind, i32, i32) {
    if (!s) {
        return 0;
    }
    switch (kind) {
        case 4:
            if (!Save(s)) {
                return 0;
            }
            break;
        case 7:
            if (!Load(s)) {
                return 0;
            }
            break;
    }
    return 1;
}

RVA(0x00163780, 0x134)
i32 CDDrawWorkerHost::Save(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }

    s->Write(&m_scaledX, 4);
    s->Write(&m_scaledY, 4);
    s->Write(&m_scaleX, 4);
    s->Write(&m_scaleY, 4);
    s->Write(&m_originX, 0x10);
    s->Write(&m_zBound, 4);
    s->Write(&m_snappedX, 4);
    s->Write(&m_snappedY, 4);
    s->Write(&m_94, 4);
    s->Write(&m_98, 4);

    i32 gridSize = m_gridW * m_gridH * 4;
    s->Write(&gridSize, 4);
    s->Write(m_tileGrid, gridSize);

    char buf[0x80];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, m_name);
    s->Write(buf, 0x80);
    return 1;
}

// ---------------------------------------------------------------------------
// CDDrawWorkerHost::Load (__thiscall, ret 0x4). Inverse of Save: read back the same
// field sequence; the size-prefix must equal gridW*gridH*4 or the load aborts.
// @early-stop
// 99.98% fwd-decl-census butterfly (docs/patterns/header-fwd-decl-count-regalloc-
// butterfly.md): the 2026-07-19 CButeSection==CButeMgr fold's ButeMgr.h class-def
// addition flipped ONE load pair here (closure census unchanged - the content/
// position variant, same firing as SBI_MenuItem::DecCounter). Shape byte-correct.
RVA(0x001638c0, 0x140)
i32 CDDrawWorkerHost::Load(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }

    s->Read(&m_scaledX, 4);
    s->Read(&m_scaledY, 4);
    s->Read(&m_scaleX, 4);
    s->Read(&m_scaleY, 4);
    s->Read(&m_originX, 0x10);
    s->Read(&m_zBound, 4);
    s->Read(&m_snappedX, 4);
    s->Read(&m_snappedY, 4);
    s->Read(&m_94, 4);
    s->Read(&m_98, 4);

    i32 gridSize = 0;
    s->Read(&gridSize, 4);
    if (gridSize != m_gridH * m_gridW * 4) {
        return 0;
    }
    s->Read(m_tileGrid, gridSize);

    char buf[0x80];
    s->Read(buf, 0x80);
    strcpy(m_name, buf);
    return 1;
}

// ===========================================================================
// Class-metadata annotations (EOF-hosted: large /O2 TU with several @early-stop
// bodies; keep the completeness typedefs after the last function).
// ===========================================================================
// --- local views moved with their bodies from src/Wwd/WwdFile.cpp ---
