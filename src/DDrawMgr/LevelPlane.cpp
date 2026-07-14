// LevelPlane.cpp - the plane/render module of the level subsystem.
// original TU: plane/render module (@identity-TODO - filename unknown; the
// CLevelPlane + CPlaneRender + WwdFile-plane-method + CImageSet3-helper +
// CDDrawWorkerHost bodies at [0x1615a0 .. 0x1638c0])
//
// Consolidated by retail .text birth position (interval dossier 0x15ccd0, wave1-C):
// the plane/render TU is [0x161350 .. 0x163a00] - heavily WOVEN, one obj - holding
//   CDDrawWorkerHost ctor/ReadPlaneBlock-gap/RegisterNamed   (0x1615a0/640/c50)
//   CLevelPlane InitGeometry/RecomputePlaneCoords/Build      (0x1619f0/c90/e80)
//   CImageSet3 grid-owner leaves Cleanup/Prune/GetSize       (0x161bf0/0x1628d0/0x1633e0)
//   CPlaneRender SetTileSize(FromImageSet)/Draw/CenterScrollA+B/InitScrollRects/
//     ValidateTiles/ResolveColorKey/Save/Load (+ the serialize dispatcher)
//   WwdFile RebuildPlanes/ReadPlaneObjects                   (0x1628f0/0x162af0)
// (The 0x161350-0x161558 CImageSet1/2/3 scalar-dtor pocket before the ctor is
// COMDAT-at-usage emission - those classes home elsewhere; ~CDDrawWorkerHost
// @0x163af0 is past the TU end and stays in DDrawWorkerHost.cpp.)
//
// Class definitions stay canonical: <Gruntz/GameLevel.h> (CLevelPlane + the
// CImageSet view + <Wwd/WwdFile.h> CPlaneRender/WwdFile; CPlane == the canonical
// CDDrawWorkerHost, the stream is the real CFileMemBase),
// <DDrawMgr/DDrawWorkerHost.h> (CDDrawWorkerHost), <Gruntz/UserLogic.h>
// (CGameObject). Bodies are strictly RVA-ascending; only offsets + emitted
// bytes are load-bearing (campaign doctrine).
#include <Mfc.h>
#include <Gruntz/GameLevel.h> // CLevelPlane + LevelCoordRect + CImageSet view (+ WwdFile.h)
#include <Gruntz/UserLogic.h> // the shared CGameObject (ReadPlaneObjects' 0x1dc object)
#include <Image/CImage.h>     // CImage m_width/m_height (SetTileSizeFromImageSet)
#include <Image/ImageSet.h> // the REAL CImageSet (0x6c frame collection): SetTileSizeFromImageSet's
                            // arg. Was the GameLevel.h tile-descriptor class of the same NAME,
// which had these frame fields grafted on; that class is CTileImageSet now.
#include <DDrawMgr/DDSurface.h>       // CDDSurface::BltEx/BltFast (the Draw blit callees)
#include <DDrawMgr/DDrawWorkerHost.h> // canonical CDDrawWorkerHost (ctor + RegisterNamed here)
#include <Io/FileMem.h> // the REAL serialize-stream base CFileMemBase (Save/Load's Read@+0x2c/Write@+0x30)
#include <Wwd/WwdSpatialMgr.h>       // the canonical spatial/scroll worker (m_scroll)
#include <DDrawMgr/DDrawWorkerCtx.h> // shared CDDrawWorkerCtx (RegisterNamed's map chain)
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
// CDDrawWorkerHost::Cleanup_161bf0 prunes it (PruneCount 0x1688b0), runs its OUT-OF-LINE /GX
// complete dtor (~CWwdSpatialMgr @0x163a40, body in WwdSpatialMgr.cpp; the ex-C163a40
// placeholder identity is dissolved), then ::operator delete (0x1b9b82) frees it.
// GetSize (0x168430) is the serialized-size accessor (WwdSpatialMgr.cpp defines it).
// All reloc-masked __thiscall callees (no body).

// [The local "CImageSet3" grid-owner view that sat here is DISSOLVED 2026-07-13:
// the identity was CDDrawWorkerHost all along - the three bodies read +0x20/+0x24/
// +0xb0 = m_buffer0/m_buffer1/m_spatialWorker, and 0x161bf0 IS CDDrawWorkerHost's
// vtable slot 7 per the retail slot map @0x1f0270. The "@identity-TODO name-
// conflation with the Gruntz CImageSet3 variant record" is resolved: the 0x18-byte
// record class (<Gruntz/ImageSets.h>) cannot even hold a +0xb0 member.]

// ===========================================================================
// 0x1615a0 - CDDrawWorkerHost(a1,a2,a3): the /GX EH ctor. cl inlines the
// CLoadable base ctor (vptr stamp -- reloc-masks the retail intermediate
// g_loadableVtbl 0x5efc30 -- then m_04=a2/m_08=a3/m_0c=a1), constructs the
// +0x9c ::CObArray member (0x1b55e9; its destructible-member trylevel supplies
// the EH frame), stamps the own vftable (0x5f0270), then arms the scalar fields
// (grid/scroll = 0, scaleX/Y = 1.0f, +0x50 = -1) and zero-fills the +0xf4 pool
// (25 dwords) with m_pool[0] = 100. Byte-exact (100%): the retail intermediate base
// stamp 0x5efc30 is reloc-masked, so the compiler-emitted ??_7CLoadable stamp
// matches at the byte level; the CLoadable ctor arg-order (m_04=a2/m_08=a3/
// m_0c=a1) + body store order reproduce the schedule exactly.
// ===========================================================================
RVA(0x001615a0, 0x9a)
CDDrawWorkerHost::CDDrawWorkerHost(CPlaneMapData* mapData, i32 field04, i32 flags) {
    m_04 = field04;
    m_flags = flags;
    m_mapData = mapData; // (merged CLoadable ctor)
    // m_frameSets (::CObArray) default-constructed here (0x1b55e9).
    m_tileGrid = 0;
    m_colOffsets = 0;
    m_scroll = 0;
    m_scaleX = 1.0f;
    m_scaleY = 1.0f;
    m_viewX = -1; // (ex m_50; the two +0x50 readings are unreconciled)
    memset(m_pool, 0, sizeof(m_pool));
    m_pool[0] = 100;
}

// 0x161640 (930 B) = the plane-block reader, CDDrawWorkerHost vtable slot 10
// (??_7CDDrawWorkerHost @0x1f0270+0x28) - the slot CGameLevelPlanes::ReadPlane
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
//      scale - then steps 6-9 are BYTE-IDENTICAL to InitGeometry_1619f0 (two
//      operator_new allocations: the tile grid + the column-offset table, then the
//      tail-call to RecomputePlaneCoords 0x161c90).
//   5. Finally drives RebuildPlanes (0x1628f0) for the object block.
//
// WHAT BLOCKS IT (not a model wall - a cost/plateau judgment): its steps 6-9 ARE
// InitGeometry_1619f0, and that sibling is itself parked at 78.33% on the documented
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md - retail's
// arg->register rotation over the ~20 field seeds is not source-steerable). A faithful
// 930-B reconstruction here would inherit that same plateau over its largest span, and
// a PARTIAL body is worse than a stub (it under-counts AND diverges the TU's regalloc).
// The honest sequencing is leaf-first: crack InitGeometry_1619f0's register pinning,
// then this body follows mechanically from the structure above.
// @confidence: high
// @source: vtable_hierarchy-slot-map+ReadPlane-dispatch+full-disasm-decode
// @stub
RVA(0x00161640, 0x3a2)
i32 CDDrawWorkerHost::Read(void* planeData, void* blockBase, void* bounds) {
    return 0;
}

// CDDrawWorkerHost::InitGeometry_1619f0 (0x1619f0, CDDrawWorkerHost vtable slot +0x24):
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
i32 CDDrawWorkerHost::InitGeometry_1619f0(
    i32 w,
    i32 h,
    i32 tileW,
    i32 tileH,
    i32 depthX,
    i32 depthY,
    LevelCoordRect* bounds,
    char* name
) {
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
    m_fillRect.left = 0;
    m_fillRect.top = 0;
    m_fillRect.bottom = tileH;
    m_wrapW = tileW * w;
    m_wrapH = tileH * h;
    m_fillRect.right = tileW;
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
// 0x161bf0: tear down the owned resources.  Prune the grid, then destroy + free
// it (no null-out), then free the two RezAlloc'd buffers at +0x20/+0x24 (nulled).
RVA(0x00161bf0, 0x5e)
void CDDrawWorkerHost::Cleanup_161bf0() {
    if (m_scroll != 0) {
        m_scroll->PruneCount();
    }
    CWwdSpatialMgr* g = m_scroll;
    if (g != 0) {
        g->~CWwdSpatialMgr(); // the out-of-line complete dtor (0x163a40)
        ::operator delete(g);
    }
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
// Same lookup chain as CDDrawWorkerB::Helper_166040. m_mapData is the CLoadable base's
// +0x0c owner context (declared i32; the reinterpret is the CLoadable ctx handle).
// ===========================================================================
// @early-stop
// 90.48%: identical Lookup out-param zero-init reorder wall as CDDrawWorkerB::
// Helper_166040 - retail emits the `mov [esp+N],0` (val=0) AFTER both Lookup arg
// pushes (push &val / push key), cl emits it BETWEEN them. Verified byte-exact
// elsewhere (llvm-objdump -dr): the only differing bytes are that 1-instruction
// slot. Logic/offsets/both call sites/movsbl-narrowed index all match. Not
// source-steerable (same as Helper_166040's documented note).
RVA(0x00161c50, 0x3f)
void CDDrawWorkerHost::RegisterNamed(char index, const char* key) {
    CObject* val = 0;
    ((CDDrawWorkerCtx*)m_mapData)->m_10->m_10.Lookup(key, val);
    m_frameSets.SetAtGrow(index, val);
}

// ---------------------------------------------------------------------------
// CDDrawWorkerHost::RecomputePlaneCoords - recompute one plane's scaled scroll origin
// and visible-tile extents from its (already-scaled) float coords. __thiscall
// with `this` = the plane (ecx); reloc-masks only the float 0.0 constant and the
// CRT __ftol helper (the (int)float casts). X and Y are computed identically:
// wrap (flags bit set) folds the coord modulo the tile count into [0, count);
// else it clamps to [0, count-1].
RVA(0x00161c90, 0x1e4)
void CDDrawWorkerHost::RecomputePlaneCoords() {
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
    p->m_snappedX = ix;
    i32 iy = (i32)p->m_scaledY;
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

// ---------------------------------------------------------------------------
// CDDrawWorkerHost::SetTileSizeFromImageSet (0x161fa0, __thiscall, ret 0x4): linear-scan
// the image set for the first populated frame (GetAt returns null outside
// [minIndex, maxIndex]); on the first hit, drive SetTileSize with that frame's pixel
// dimensions and stop. An empty set leaves the tile size unchanged.
RVA(0x00161fa0, 0x6c)
void CDDrawWorkerHost::SetTileSizeFromImageSet(CImageSet* set) {
    for (i32 i = 0; i < set->m_count; i++) {
        if (set->GetAt(i) != 0) {
            CImage* f = set->GetAt(i);
            SetTileSize(f->m_width, f->m_height);
            break;
        }
    }
}

// ===========================================================================
// CDDrawWorkerHost::Draw (__thiscall, ret 0x4) - the toroidally-wrapped tile-grid
// renderer. Takes one context arg (the blit destination owner) at +0xA8; ebp =
// ctx->m_2c is the target CDDSurface (the BltEx/BltFast `this`). If the plane is
// hidden (flag bit1) it returns immediately.
//
// It converts the plane's pixel view-rect [m_40..m_4c] into tile indices via the
// log2 shifts (m_8c=shiftX, m_90=shiftY), computing the partial-tile pad at each
// edge (leftPad/topPad/rightPad/botPad) and the interior tile counts
// (interiorCols = colR-colL-1, interiorRows = rowB-rowT-1). It then walks the
// visible grid in five phases - top-left corner, top strip, top-right corner;
// then per interior row: left column, interior columns, right column; then the
// bottom edge - blitting each cell. For each cell it reads the tile handle from
// the row-major grid m_20[m_24[row] + col]: 0xEEEEEEEE (uninitialised) => a
// clipped fill via CDDSurface::BltEx(&m_f4 blitparam); -1 => skip; else resolve
// the frame (m_a0[handle>>16], bounds-check (handle&0xffff) against the frame's
// [+0x64,+0x68], index its +0x14 frame table) and CDDSurface::BltFast it.
//
// Per-cell blit (the retail inlined this at all 9 region sites). handle==
// 0xEEEEEEEE -> a clipped fill via BltEx(&destRect,...,&m_surface); handle==-1 ->
// skip; else resolve the frame (m_planeArray[handle>>16], bounds-check the low
// 16 bits against [m_lo,m_hi], index m_frames) and BltFast it. The blit's dest
// size equals the src rect size (right-left / bottom-top), so every region only
// differs in the src rect it passes + its screen (x,y). `dr`/`surf` are Draw's
// locals; expanded textually to reproduce the 9 separate inlined sites.
#define DRAW_CELL(handle, xp, yp, srcp)                                                            \
    do {                                                                                           \
        u32 h_ = (u32)(handle);                                                                    \
        if (h_ == 0xeeeeeeee) {                                                                    \
            dr.left = (xp);                                                                        \
            dr.top = (yp);                                                                         \
            dr.right = (xp) + ((srcp)->right - (srcp)->left);                                      \
            dr.bottom = (yp) + ((srcp)->bottom - (srcp)->top);                                     \
            surf->BltEx(&dr, 0, 0, 0x1000400, &m_surface);                                         \
        } else if (h_ != 0xffffffff) {                                                             \
            CPlaneFrame* fr_ = ((CPlaneFrame**)m_frameSets.GetData())[h_ >> 16];                   \
            i32 idx_ = (i32)(h_ & 0xffff);                                                         \
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
    y = m_viewY;
    x = m_viewX;
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
        x = m_viewX;
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
    x = m_viewX;
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

// ===========================================================================
// WwdFile::ReadPlaneObjects (__thiscall, ret 0x4; Ghidra mis-derived the
// `QAEXXZ` void/no-arg prototype). Reads ONE object record at `src` (a pointer
// into the inflated plane-object block) into a freshly allocated game object,
// registers it with the level, and returns the number of source bytes consumed
// (the caller does `src += result` to advance to the next record).
//
// Source record (WwdObjectRecord, wwd_object.h): a fixed 0x11C block of i32
// fields followed by FOUR length-prefixed strings (name, logic, imageSet,
// sound) whose lengths sit at record +0x04/+0x08/+0x0C/+0x10. ReadPlaneObjects:
//   1. captures id (+0) and the four string lengths up front;
//   2. `new` a 0x1DC-byte CObject-derived game object, two-phase constructs it
//      (engine ctor + an embedded CDDrawSubMgr sub-object at +0x1A0) and stamps
//      the two retail vtables (transitional manual stamp - those classes' vtable
//      contents are not modeled here, so the addresses are reloc-masked DATA
//      externs);
//   3. copies the four trailing strings out into stack CStrings (inline
//      rep-movs into a scratch buffer + CString(char*) ctor);
//   4. bounds-checks the object's grid x/y against the level dims, looks the
//      image-set name up in the level CMapStringToOb, then runs the object's
//      vtable +0x28 "load" virtual;
//   5. on success, applies the name/logic/imageSet strings (sprite frame cache /
//      anim geometry / m_imageSetName assign) and scatters ~60 trailing record fields into
//      the object and its +0x7C sub-object via an advancing cursor;
//   6. registers the object with the level and returns bytes-consumed.
// Every failure path destroys the object (vtable +0x04 scalar-deleting dtor) and
// the four CStrings under the /GX unwind frame.
//
// All callees (engine ctor/dtor/load virtual, CDDrawSubMgr ctor, CMapStringToOb
// Lookup, the sprite/anim helpers, the level register) are unmatched engine code
// modeled with no body -> reloc-masked calls.
// ===========================================================================

// The level/plane loader `this`. Only the members ReadPlaneObjects touches are
// pinned (offsets are the load-bearing thing): m_assetOwner (the map/asset owner), and
// m_gridWidth/m_gridHeight (the grid extents the object x/y are range-checked against). m_assetOwner is
// read both as the ctor's owner arg and for the image-set CMapStringToOb lookup.
// (The WwdLevelLoader `this`-view is GONE: `this` IS the plane. Its m_assetOwner
// @+0x0c is the plane's m_mapData (the same owner/context object RegisterNamed
// resolves names through), and its "grid extents" @+0x30/+0x34 are m_wrapW/m_wrapH -
// the TILE counts the record's tile-space x/y are range-checked against.)

// WwdFile::ReadPlaneObjects deserializes each WWD plane record into a freshly
// `operator new(0x1dc)`d shared CGameObject (<Gruntz/UserLogic.h>) - the SAME
// 0x1dc-byte instance CSpriteFactory::CreateSprite builds, brought up by the same
// engine base ctor (0x15b390). The former local `WwdGameObj` view is FOLDED AWAY:
// CGameObject's usage-proven field names win (m_stateFlags/m_extentL../m_strideX..,
// slot [1] Delete / [10] Load reconciled from the ReadPlaneObjects call bytes); the
// WWD-format-invented names (m_score/m_clipLeft/m_width..) do not survive. The +0xdc
// CString slot CGameObject deliberately pads (a real member would inject a ctor into
// every derived TU), so the imageSet assignment goes through the raw-offset
// CStringAssign helper below (CString::operator=(LPCSTR), 0x1b9e74, reloc-masked).

// CString::operator=(LPCSTR) on the +0xdc name slot (NAFXCW, reloc-masked). The
// engine's bare CString handle is one char*; its operator= is out-of-line, modeled
// as a method the &slot handle is reinterpreted through (no member to fold into).
struct CStringAssign {
    // Assign @0x1b9e74 IS CString::operator=; cast at the call.
};

// The +0x1a0 embedded sub-object is a CLoadable (base ctor 0x156cb0; the former
// ctor-only `CDDrawSubMgr` view is DISSOLVED - that name was CLoadable's second
// identity, see <Gruntz/Loadable.h>).
#include <Gruntz/Loadable.h>
inline void* operator new(u32, void* p) {
    return p;
} // placement (embedded sub-object ctor)

// The embedded sub-object's stampable view: vptr@0, then the three DWORDs
// (+0x10/+0x14/+0x18) ReadPlaneObjects zeroes right after re-stamping its vtable.
struct WwdObjAnimInit {
    void* vptr; // +0x00
    char pad_4[0x10 - 0x4];
    i32 z10, z14, z18; // +0x10, +0x14, +0x18
};

// MFC CMapStringToOb::Lookup(key, &valueOut) const. __thiscall, ret 0x8.
// authentic: reached at a COMPUTED address (m_assetOwner+0x14+0x10), not a typed
// member, so the reloc-masked Lookup extern is modeled as a method a raw pointer
// is cast through - there is no member to fold it into.
// WwdStringToObMap is an MFC CMapStringToPtr (Lookup @0x1b8008); the map var is retyped directly.

// Level register: append the finished object to the level (loader+0xb0 is the
// level CObList; AddTail returns POSITION). __thiscall, ret 0x4.

// The object's own vtable (transitional manual stamp; reloc-masked DATA extern).
// The sub-object vtable is realized as ??_7CAniAdvanceCursor@@6B@ (0x5f0128) in
// CAniAdvanceCursor.cpp; referenced here as an UNPINNED extern (the VTBL there
// owns the 0x1f0128 datum name) so this sub-object stamp reloc-masks against it.

// ---------------------------------------------------------------------------
// RebuildPlanes (0x1628f0): tear down the old +0xb0 plane-render worker, then
// allocate a fresh 0xb8-byte one, init it from the level header's 6 geometry
// pairs (CGameReg->m_24->[+0xb0..+0xdc]), and run ReadPlaneObjects `count` times.
// The throwing operator-new + partial-construct cleanup gives the /GX frame.
// ---------------------------------------------------------------------------

// (The WwdRegOwner/WwdPlaneHdr/WwdPlaneRender views are GONE: the +0xc "reg owner"
// IS CPlaneMapData (m_8 worker source, m_24 geometry), the "plane header" IS its
// CPlaneGeom (the six geometry pairs at +0xb0..+0xdc), and the 0xb8-byte
// "plane-render worker" IS the CWwdSpatialMgr grid/scroll worker - its "DtorBody"
// 0x1682f0 is FreeGrids at the SAME RVA, and the vtable it wears (0x5f02a8,
// realized as ??_7CWwdGridIter in WwdSpatialMgr.cpp) is the +0x70 embedded list's.)

// 0x5f02a8 is realized as ??_7CWwdGridIter (5 slots, dtor at slot 1) in
// src/Gruntz/WwdSpatialMgr.cpp, which OWNS the RVA catalog name via VTBL. UNPINNED
// here so RebuildPlanes' inline +0x70 embedded-cursor stamp reloc-masks against the
// real ??_7 (the manual g_planeRenderVtbl DATA placeholder is drained).

// ---------------------------------------------------------------------------
// 0x1628d0: forward the grid's Prune when present (else 0).  __thiscall tail call.
RVA(0x001628d0, 0x12)
i32 CDDrawWorkerHost::Prune_1628d0() {
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
        worker->ListDtor();
        ::operator delete(worker);
        worker = 0;
    }

    CPlaneMapData* reg = m_mapData;
    void* src = reg->m_8;
    if (src == 0) {
        return 0;
    }
    CPlaneGeom* hdr = reg->m_geometry;
    if (hdr == 0) {
        return 0;
    }

    i32 p0[2] = {hdr->m_pairA[0], hdr->m_pairA[1]};
    i32 p1[2] = {hdr->m_pairB[0], hdr->m_pairB[1]};
    i32 p2[2] = {hdr->m_pairC[0], hdr->m_pairC[1]};
    i32 p3[2] = {hdr->m_rectAWidth, hdr->m_rectAHeight};
    i32 p4[2] = {hdr->m_rectBWidth, hdr->m_rectBHeight};
    i32 p5[2] = {hdr->m_rectCWidth, hdr->m_rectCHeight};

    CWwdSpatialMgr* nw = (CWwdSpatialMgr*)::operator new(0xb8);
    if (nw) {
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *(i32*)((char*)nw + 0x74) = 0;
        *(i32*)((char*)nw + 0x78) = 0;
        *(i32*)((char*)nw + 0x00) = 0;
        *(i32*)((char*)nw + 0x04) = 0;
        *(i32*)((char*)nw + 0x08) = 0;
        *(i32*)((char*)nw + 0x0c) = 0;
        *(i32*)((char*)nw + 0xb4) = 0;
    }
    worker = nw;
    if (nw->Init(src, p0, p1, p2, p3, p4, p5) == 0) {
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
        i32 r = ReadPlaneObjects((const i32*)base);
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
    u32 nameLen = (u32)src[1];
    u32 logicLen = (u32)src[2];
    u32 imageSetLen = (u32)src[3];
    u32 soundLen = (u32)src[4];
    i32 x = src[5];
    i32 y = src[6];
    i32 z = src[7];
    i32 gridIndex = src[8];

    CGameObject* obj = (CGameObject*)operator new(0x1dc);
    if (obj == 0) {
        return 0;
    }

    obj->Construct(m_mapData, id, 0);

    // Construct the embedded sub-object at +0x1A0, then re-stamp both vtables (the
    // base ctors leave a base vtable; ReadPlaneObjects promotes both to their
    // derived types) and zero the trailing fields the derived layout adds.
    WwdObjAnimInit* subInit = (WwdObjAnimInit*)((char*)obj + 0x1a0);
    new (subInit) CLoadable((i32)m_mapData, id, 0); // the embedded loadable (ctor 0x156cb0)
    // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
    subInit->z10 = 0;
    subInit->z14 = 0;
    subInit->z18 = 0;

    // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
    obj->m_18c = -1;
    obj->m_190 = -1;
    obj->m_layer = 0;
    obj->m_194 = 0;
    obj->m_19c = 0;

    // Copy the four trailing length-prefixed strings into stack CStrings. They
    // begin right after the fixed 0x11C record.
    const char* strCursor = (const char*)src + 0x11c;
    char buf[0x400];

    i32 n;
    n = (i32)nameLen;
    if (n > 0) {
        memcpy(buf, strCursor, n);
        strCursor += n;
    }
    buf[n] = 0;
    CString name(buf);

    n = (i32)logicLen;
    if (n > 0) {
        memcpy(buf, strCursor, n);
        strCursor += n;
    }
    buf[n] = 0;
    CString logic(buf);

    n = (i32)imageSetLen;
    if (n > 0) {
        memcpy(buf, strCursor, n);
        strCursor += n;
    }
    buf[n] = 0;
    CString imageSet(buf);

    n = (i32)soundLen;
    if (n > 0) {
        memcpy(buf, strCursor, n);
        strCursor += n;
    }
    buf[n] = 0;
    CString sound(buf);

    // Grid bounds check on x/y; failure deletes the object and returns the bytes
    // consumed so far (so the caller still advances over the bad record).
    if (x < 0 || x >= m_wrapW || y < 0 || y >= m_wrapH) {
        obj->Delete(1);
        return (i32)(strCursor - (const char*)src);
    }

    // If an image set is named, require it to be present in the level map.
    i32 loaded = 1;
    if (imageSet.GetLength() != 0) {
        void* found = 0;
        CMapStringToPtr* map = (CMapStringToPtr*)((char*)m_mapData + 0x14 + 0x10);
        loaded = map->Lookup((const char*)imageSet, found);
    }

    if (!loaded) {
        obj->Delete(1);
        return (i32)(strCursor - (const char*)src);
    }

    // Run the object's load virtual (reads the fixed record into the object).
    if (obj->Load((i32)logicLen, id, (i32)strCursor, id) == 0) {
        obj->Delete(1);
        return 0;
    }

    obj->m_flags |= 0x40000;

    AnimWorkerObj* anim = obj->m_7c;
    if (anim == 0) {
        obj->Delete(1);
        return 0;
    }

    i32* sub = (i32*)anim;

    // Apply name -> sprite first-frame cache (indexed when src[?] != -1).
    if (logic.GetLength() != 0) {
        if (z != -1) {
            obj->ApplyLookupSprite((const char*)logic, z);
        } else {
            obj->ApplyName((const char*)logic);
        }
    }

    // Apply sound -> anim geometry + logic.
    if (sound.GetLength() != 0) {
        obj->ApplyLookupGeometry((const char*)sound, 0);
        obj->LookupAnimSprite((const char*)sound);
    }

    // Apply imageSet -> the +0xdc CString slot (CGameObject pads it; raw-offset assign).
    if (imageSet.GetLength() != 0) {
        ((CString*)((char*)obj + 0xdc))->operator=((const char*)imageSet);
    }

    // Scatter the trailing record fields. `p` advances through the record from
    // its dynamic-flags field onward.
    const i32* p = &src[10]; // record +0x28 (skip addFlags @+0x24)

    obj->m_flags |= (u32)*p++; // dynamicFlags       (+0x08)
    obj->m_stateFlags = *p++;  // drawFlags          (+0x40)
    sub[0x28 / 4] = *p++;      // userFlags
    // The six-int "user-value" union (+0x114..+0x128). These are the WWD object
    // record's canonical Score/Points/Powerup/Damage/Smarts/Health fields (the
    // names the Gruntz Level Editor's Edit-Objects "Attributes" dialog uses), each
    // REINTERPRETED per CUserLogic leaf - e.g. for a GruntStartingPoint enemy Grunt
    // Points=AI type (1-16), Smarts=team (0-3), Powerup=carried Tool/Toy id; for a
    // CoveredPowerup Powerup=covered object id (0-99), Smarts=revealed tile, Score=
    // megaphone order. Same physical fields, different views (this is why UserLogic.h
    // labels them by their spotlight/teleporter meaning). Authoritative field
    // semantics + the id spaces: docs/domain/README.md.
    obj->m_114 = *p++;       // score              (+0x114)
    obj->m_118 = *p++;       // points  (enemy AI type / megaphone tool id)   (+0x118)
    obj->m_11c = *p++;       // powerup (CoveredPowerup id 0-99 / carried tool) (+0x11c)
    obj->m_120 = *p++;       // damage             (+0x120)
    obj->m_124 = *p++;       // smarts  (enemy team 0-3 / revealed tile)       (+0x124)
    obj->m_placeMode = *p++; // health             (+0x128)
    obj->m_extentL = *p++;   // moveRect.l         (+0x134)
    obj->m_extentT = *p++;   // moveRect.t         (+0x138)
    obj->m_extentR = *p++;   // moveRect.r         (+0x13c)
    obj->m_extentB = *p++;   // moveRect.b         (+0x140)
    obj->m_areaL = *p++;     // hitRect.l          (+0x144)
    obj->m_areaT = *p++;     // hitRect.t          (+0x148)
    obj->m_areaR = *p++;     // hitRect.r          (+0x14c)
    obj->m_areaB = *p++;     // hitRect.b          (+0x150)
    obj->m_154 = *p++;       // attackRect.l       (+0x154)
    obj->m_158 = *p++;       // attackRect.t       (+0x158)
    obj->m_15c = *p++;       // attackRect.r       (+0x15c)
    obj->m_160 = *p++;       // attackRect.b       (+0x160)
    obj->m_64 = *p++;        // clipRect.l         (+0x64)
    obj->m_68 = *p++;        // clipRect.t         (+0x68)
    obj->m_6c = *p++;        // clipRect.r         (+0x6c)
    obj->m_70 = *p++;        // clipRect.b         (+0x70)

    if (obj->m_areaL == 0 && obj->m_areaR == 0) {
        obj->m_areaL = (i32)0x80000000;
    }
    if (obj->m_extentL == 0 && obj->m_extentR == 0) {
        obj->m_extentL = (i32)0x80000000;
    }
    if (obj->m_64 == 0 && obj->m_6c == 0) {
        obj->m_64 = (i32)0x80000000;
    }
    if (obj->m_154 == 0 && obj->m_15c == 0) {
        obj->m_154 = (i32)0x80000000;
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

    u32 w = (u32)*p++;
    if (w > 0) {
        obj->m_strideX = (i32)w; // +0xf8
    }
    u32 h = (u32)*p++;
    if (h > 0) {
        obj->m_strideY = (i32)h; // +0xfc
    }

    // Retail: `mov ecx,[this+0xb0]; call 0x1688f0` - it LOADS the spatial worker and
    // registers the object with it. (The old view took the ADDRESS of +0xb0 and called
    // CObList::AddTail on it - a `lea` where retail emits a `mov`, and a mis-bound
    // NAFXCW symbol; +0xb0 holds a POINTER, as RebuildPlanes' `new(0xb8)` store proves.)
    m_scroll->RemoveObject((CWwdGameObject*)obj);

    return (i32)(strCursor - (const char*)src);
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
    CPlaneScroll* scroll = m_scroll;
    if (scroll == 0) {
        return 0;
    }

    u32 flags = m_flags;

    i32 x;
    if (flags & 0x4) {
        x = (i32)m_scaledX;
    } else {
        x = (m_originX + m_extentX) / 2 + 1;
    }

    i32 y;
    if (flags & 0x8) {
        y = (i32)m_scaledY;
        return scroll->SetTargetA(x, y);
    }
    y = (m_originY + m_extentY) / 2 + 1;
    return scroll->SetTargetA(x, y);
}

// @early-stop
// 87.9%, same shrink-wrapped-push / member-load scheduling wall as CenterScrollA.
RVA(0x00163370, 0x70)
i32 CDDrawWorkerHost::CenterScrollB() {
    CPlaneScroll* scroll = m_scroll;
    if (scroll == 0) {
        return 0;
    }

    u32 flags = m_flags;

    i32 x;
    if (flags & 0x4) {
        x = (i32)m_scaledX;
    } else {
        x = (m_extentX + m_originX) / 2 + 1;
    }

    i32 y;
    if (flags & 0x8) {
        y = (i32)m_scaledY;
        return scroll->SetTargetB(x, y);
    }
    y = (m_extentY + m_originY) / 2 + 1;
    return scroll->SetTargetB(x, y);
}

// GetSize_1633e0 (0x1633e0): forward the grid's serialized size when present
// (else 0). Out-of-line (retail emits it standalone, tail-forwarding to
// CWwdSpatialMgr::GetSize 0x168430; an inline member folds away and never emits).
RVA(0x001633e0, 0x12)
i32 CDDrawWorkerHost::GetSize_1633e0() {
    if (m_scroll == 0) {
        return 0;
    }
    return m_scroll->GetSize();
}

// ---------------------------------------------------------------------------
// CDDrawWorkerHost::InitScrollRects (__thiscall, no args). Seed three (0,0,w-1,h-1)
// rects + their centers (w/2, h/2) into the scroll sub-object from the plane
// geometry's three dimension pairs (m_mapData->m_geometry), then park
// the scroll target at (-22222, -22222) so the first SetTarget always moves.
RVA(0x00163420, 0xf0)
void CDDrawWorkerHost::InitScrollRects() {
    if (m_scroll == 0) {
        return;
    }
    CPlaneGeom* g = m_mapData->m_geometry;
    if (g == 0) {
        return;
    }

    i32 c8 = g->m_rectAWidth;
    i32 cc = g->m_rectAHeight;
    i32 d0 = g->m_rectBWidth;
    i32 d4 = g->m_rectBHeight;
    i32 d8 = g->m_rectCWidth;
    i32 dc = g->m_rectCHeight;

    CPlaneScroll* s = m_scroll;
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
// The live screen RGB-format shift table at 0x683ea0.. (already named by
// SpriteRef.cpp / CLightFxRender.cpp). Reloc-masked DIR32 data refs.
DATA(0x00283ea0)
extern i32 g_rUp; // 0x683ea0
DATA(0x00283ea4)
extern i32 g_gUp; // 0x683ea4
DATA(0x00283eac)
extern i32 g_rDown; // 0x683eac
DATA(0x00283eb0)
extern i32 g_gDown; // 0x683eb0
DATA(0x00283eb4)
extern i32 g_bDown; // 0x683eb4

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
            if (handle == -1 || (u32)handle == 0xeeeeeeee) {
                continue;
            }
            CPlaneFrame* frame = ((CPlaneFrame**)m_frameSets.GetData())[(u32)handle >> 16];
            if (frame == 0) {
                result = 0;
                if (errOut != 0) {
                    sprintf(
                        msg,
                        "Plane %s: Bad map image set value (%i) at %i,%i\n",
                        m_name,
                        (u32)handle >> 16,
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
    i32 format = m_mapData->m_surface->m_desc->m_format;
    if (format == 8) {
        return;
    }
    if (format != 0x10) {
        return;
    }

    i32 idx = m_colorKey;
    if (idx < 0) {
        return;
    }
    if (idx > 0xff) {
        return;
    }

    CPlanePalOwner* owner = m_mapData->m_paletteHost->m_owner;
    if (owner == 0) {
        return;
    }
    u8* rgb = owner->m_palette->m_rgb;
    if (rgb == 0) {
        return;
    }

    m_colorKey = (u16)(((u8)((u8)rgb[idx * 4 + 0] >> (u8)g_rDown) << g_rUp)
                       | ((u8)((u8)rgb[idx * 4 + 1] >> (u8)g_gDown) << g_gUp)
                       | (u8)((u8)rgb[idx * 4 + 2] >> (u8)g_bDown));
}

// ---------------------------------------------------------------------------
// 0x163710 - the plane-serialize op dispatcher CGameLevel::EditDispatch (0x160f70)
// tail-calls: on kind 4 run the plane Save (0x163780), on kind 7 the plane Load
// (0x1638c0) - failure returns 0, every other kind (3/5/6/8) returns 1. __stdcall,
// 4 args (only the stream + kind used). The Save/Load entries are reached here via a
// __stdcall re-decl of the same RVAs (retail relies on ecx=plane surviving from
// EditDispatch, passing the stream as the lone stack arg - so a member call would emit
// the wrong ecx setup). (Re-homed from src/Stub/BoundaryUpper2.cpp; physically between
// ResolveColorKey 0x163670 and Save 0x163780 in this CPlaneRender TU.)
extern i32 __stdcall PlaneSaveVia(void* stream); // 0x163780 == CDDrawWorkerHost::Save entry
extern i32 __stdcall PlaneLoadVia(void* stream); // 0x1638c0 == CDDrawWorkerHost::Load entry
// @early-stop
// jump-table-shape wall (~84%): retail lowers the kind switch (cases 3..8, only 4 and 7
// active) to a dense `jmp [eax*4+table]`; MSVC here folds the 4 default-equal cases and
// emits a compare ladder. Forcing 6 explicit cases still merges them (78%); the 2-case
// ladder is closest. Logic complete.
RVA(0x00163710, 0x42)
i32 __stdcall PlaneSerializeDispatch(void* stream, i32 kind, i32, i32) {
    if (!stream) {
        return 0;
    }
    switch (kind) {
        case 4:
            if (!PlaneSaveVia(stream)) {
                return 0;
            }
            break;
        case 7:
            if (!PlaneLoadVia(stream)) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CDDrawWorkerHost::Save (__thiscall, ret 0x4). Serialize the plane to a binary
// stream: the scroll origin/dims block, the origin/extent rect, four shift/log
// fields, the tile grid (size-prefixed), and the fixed 0x80-byte name field.
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
SIZE_UNKNOWN(CStringAssign); // +0xdc CString::operator= helper (WwdGameObj folded to CGameObject)
SIZE_UNKNOWN(WwdSubMgrCtor);
SIZE_UNKNOWN(WwdObjAnimInit);
