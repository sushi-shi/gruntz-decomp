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
// CDDrawWorkerHost::ReadPlaneObjects @0x162af0 CALLS the shared CGameObject ctor
// COMDAT (0x15b390) instead of folding it, so this TU takes the declaration-only
// form of it - the per-TU guard described in <Gruntz/WwdGridIter.h>. The body lives
// in src/Wwd/WwdFactoryObject.cpp.
#define CGAMEOBJECT_OOL_CTOR

#include <Gruntz/GruntzMgr.h> // C linkage for the definitions below (inherited, not restated)
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
#include <DDrawMgr/DDrawWorkerMapSmall.h> // m_workerMap->m_cachedWorker (a CAniRecordBase2)
#include <DDrawMgr/DirectDrawMgr.h>       // CDDPalette - the cached worker's m_buf palette
#include <DDrawMgr/DDrawSubMgrPages.h>    // m_drawTarget->m_frontPair
#include <DDrawMgr/DDrawSurfacePair.h>    // ->m_bpp (the ex CPlaneSurfDesc::m_format)
#include <DDrawMgr/DDrawChildGroup.h>     // m_childGroup (the worker source)
#include <Io/FileMem.h> // the REAL serialize-stream base CFileMemBase (Save/Load's Read@+0x2c/Write@+0x30)
#include <Wwd/WwdSpatialMgr.h> // the canonical spatial/scroll worker (m_scroll)
#include <rva.h>

#include <stdio.h>  // sprintf (ValidateTiles diagnostics)
#include <string.h> // strcpy/memcpy/memset (inline rep movs / rep stos)
#include <Gruntz/Loadable.h>

// The tile grid is a flat dword run of m_gridW*m_gridH cells.  The byte size is
// written out at each of the three sites that need it rather than through a shared
// helper, because the multiply's operand ORDER is byte-visible (cl leaves the first
// operand in the register for the `imul`) and retail's three sites do not agree on
// it: Read/Load use `m_gridH * m_gridW`, Save uses `m_gridW * m_gridH`.

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
CDDrawWorkerHost::CDDrawWorkerHost(CDDrawSurfaceMgr* mapData, i32 field04, i32 flags)
    : CLoadable(field04, flags, mapData) {
    // The base trio is CLoadable's ctor, NOT three body assignments: retail stores
    // m_id/m_flags/m_ownerCtx BEFORE the m_frameSets CObArray member ctor, and only a
    // base/mem-init store can land there (a body store is emitted after every member
    // ctor). The param loads sit at the very top of the prologue for the same reason.
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
// Retail INLINES two of this class's own leaf methods here (both also exist
// out-of-line, at 0x161c50 and 0x161fa0): the per-token registry lookup +
// SetAtGrow IS RegisterNamed, and the `flags & 0x10` arm IS
// SetTileSizeFromImageSet. They are transcribed inline below, which is what the
// bytes say. (Modelling them as real in-class inlines was TRIED and does not work:
// cl then emits NO out-of-line copy at all, so the 0x161c50/0x161fa0 labels vanish -
// retail's copies exist because other TUs referenced them.)
// @confidence: high
// @source: vtable_hierarchy-slot-map+ReadPlane-dispatch+full-disasm-decode
// @early-stop
// 96.7%: complete + byte-faithful reconstruction; three small scheduling islands
// remain. (1) the Lookup out-param zero-init - retail emits `mov [esp+N],0` AFTER
// both arg pushes AND the this-setup, cl emits it between the pushes (the identical
// residue RegisterNamed itself is parked on). (2) the `shl eax,2 / push eax`
// operator-new size: retail interleaves it after the FIRST fmul, cl sinks it past the
// second. (3) the scroll-origin fild pair: retail loads both ints to registers and
// spills them to fresh stack temps ([esp+0x14]/[esp+0x10]) before each fild, cl filds
// straight out of [esi+0x6c]/[esi+0x68] - int locals do not stop cl forward-
// substituting the load into the fild operand. Everything else (the tokenizer's two
// char loops incl. the SIB base/index order, the inlined RegisterNamed, the inlined
// SetTileSizeFromImageSet with its doubled GetAt bounds check, the whole geometry
// seed block, the CopyRect + view re-derive, both operator-new allocations, the tile
// copy loop, the column-offset fill and the RebuildPlanes tail) is byte-exact.
RVA(0x00161640, 0x3a2)
i32 CDDrawWorkerHost::Read(
    const WwdPlaneHeader* pd,
    const char* blockBase,
    LevelCoordRect* bounds
) {
    if (pd->headerSize != 0xa0) {
        return 0;
    }

    // --- 1. the image-set NAME LIST: `imageSetsCount` tokens, separated by any byte
    // outside ['0', 0x80). Each token names an entry of the owner's image-set registry;
    // the resolved worker (or NULL on a miss) is cached at m_frameSets[n], the slot the
    // draw loop indexes by handle>>16.
    char nameBuf[0x80];
    i32 pos = 0;
    const char* names = blockBase + pd->imageSetsOffset;
    for (u32 n = 0; n < pd->imageSetsCount; n++) {
        i32 len = 0;
        while ((names[pos] < '0' || names[pos] > 0x80) && names[pos] != 0) {
            pos++;
        }
        while (names[pos] >= '0' && names[pos] < 0x80 && names[pos] != 0) {
            nameBuf[len] = names[pos];
            len++;
            pos++;
        }
        nameBuf[len] = 0;
        if (len > 0) {
            // RegisterNamed (0x161c50), inlined by retail
            CObject* val;
            val = 0;
            OwnerMgr()->m_imageRegistry->m_10map.Lookup(nameBuf, val);
            m_frameSets.SetAtGrow(static_cast<char>(n), val);
        }
    }

    // --- 2. the record geometry ------------------------------------------------
    m_flags = pd->flags;
    m_94 = pd->movementXPercent;
    m_98 = pd->movementYPercent;
    m_scaledX = 0;
    m_scaledY = 0;
    m_zBound = -999999;
    m_gridW = pd->tilesWide;
    m_gridH = pd->tilesHigh;
    m_tilePxW = pd->tilePixelWidth;
    m_tilePxH = pd->tilePixelHeight;
    m_zBound = pd->zCoord;
    m_bounds50.left = bounds->left;
    m_bounds50.top = bounds->top;
    m_bounds50.right = bounds->right;
    m_bounds50.bottom = bounds->bottom;
    m_fillRect.left = 0;
    m_fillRect.top = 0;
    m_fillRect.right = m_tilePxW;
    m_fillRect.bottom = m_tilePxH;
    m_wrapW = m_tilePxW * m_gridW;
    m_wrapH = m_tilePxH * m_gridH;

    if (m_flags & 0x10) {
        // SetTileSizeFromImageSet (0x161fa0) on the FIRST image set, inlined by retail
        CDDrawWorker* set = (m_frameSets.GetSize() > 0) ? FrameSetAt(0) : 0;
        for (i32 f = 0; f < set->m_items.GetSize(); f++) {
            if (set->GetAt(f) != 0) {
                CImage* first = set->GetAt(f);
                SetTileSize(first->m_width, first->m_height);
                break;
            }
        }
    } else {
        SetTileSize(pd->tilePixelWidth, pd->tilePixelHeight);
    }

    strcpy(m_name, pd->name);
    m_bltFx.dwFillColor = pd->fillColor;
    m_flags = pd->flags;

    if (bounds->left != LEVEL_COORD_UNSET) {
        LevelCoordRect local;
        CopyRect((&local), (bounds));
        m_bounds50 = local;
        m_viewW = m_bounds50.right - m_bounds50.left + 1;
        m_viewH = m_bounds50.bottom - m_bounds50.top + 1;
        m_anchorX = m_viewW / 2;
        m_anchorY = m_viewH / 2;
        RecomputePlaneCoords();
    }

    m_scaleX = static_cast<float>(m_94) * 0.01f;
    m_scaleY = static_cast<float>(m_98) * 0.01f;

    // --- 3. the tile-handle grid: a flat dword run at tilesOffset ---------------
    m_tileGrid = static_cast<i32*>(operator new(m_gridH * m_gridW * 4));
    // byte-forced by the on-disk format: the grid is a raw dword run located by a
    // RUNTIME offset into the mapped main block, so no declared member can name it
    const i32* cell = reinterpret_cast<const i32*>(blockBase + pd->tilesOffset);
    for (u32 t = 0; t < static_cast<u32>(m_gridH * m_gridW); t++) {
        m_tileGrid[t] = *cell;
        cell++;
    }

    m_colOffsets = static_cast<i32*>(operator new(m_gridH * 4));
    for (i32 c = 0; c < m_gridH; c++) {
        m_colOffsets[c] = c * m_gridW;
    }

    // --- 4. the scroll origin: parallax-scaled unless the plane is origin-fixed --
    i32 originY = pd->scrollY;
    i32 originX = pd->scrollX;
    float sy = static_cast<float>(originY);
    float sx = static_cast<float>(originX);
    if ((m_flags & 1) == 0) {
        sx *= m_scaleX;
        sy *= m_scaleY;
    }
    m_scaledX = sx;
    m_scaledY = sy;
    RecomputePlaneCoords();

    // --- 5. the serialized object block ----------------------------------------
    if (pd->objectsOffset != 0) {
        if (RebuildPlanes(blockBase + pd->objectsOffset, pd->objectsCount) == 0) {
            return 0;
        }
    }
    return 1;
}

// CDDrawWorkerHost::InitGeometry (0x1619f0, CDDrawWorkerHost vtable slot +0x24):
// seed tile/wrap/origin/shift fields from the 8 args, log2 the tile shifts, strcpy
// the name, alloc the tile grid + column-offset table, tail-call RecomputePlaneCoords.
// __thiscall, 8 args (ret 0x20), returns 1.
// @early-stop
// 94.0% (was 78.3%: the recorded "zero-register-pinning wall" was largely a
// mislabeled source bug - the `i32 pw = ...; m_viewW = pw;` temporaries in BOTH
// view-derive blocks. Assigning m_viewW/m_viewH directly and reading them back for
// the halves took it 78.3 -> 94.0 and made the twin Build EXACT.) Logic/fields/
// offsets/CFG/args byte-faithful; the residue is the remaining arg->register
// rotation over the ~20 field seeds (docs/patterns/zero-register-pinning.md).
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
    m_viewW = m_bounds50.right - m_bounds50.left + 1;
    m_viewH = m_bounds50.bottom - m_bounds50.top + 1;
    m_anchorX = m_viewW / 2;
    m_anchorY = m_viewH / 2;
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
        m_viewW = m_bounds50.right - m_bounds50.left + 1;
        m_viewH = m_bounds50.bottom - m_bounds50.top + 1;
        m_anchorX = m_viewW / 2;
        m_anchorY = m_viewH / 2;
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
    CObject* val;
    val = 0;
    OwnerMgr()->m_imageRegistry->m_10map.Lookup(key, val);
    m_frameSets.SetAtGrow(index, val);
}

RVA(0x00161c90, 0x1e4)
void CDDrawWorkerHost::RecomputePlaneCoords() {
    CDDrawWorkerHost* p = this;
    u32 flags = p->m_flags;
    i32 wrapX, wrapY;
    wrapX = flags & 4;

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
    wrapY = flags & 8;
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
    p->m_viewRect.left = ox;
    if (ox < 0) {
        if (wrapX) {
            p->m_viewRect.left = p->m_wrapW + ox;
        } else {
            p->m_viewRect.left = 0;
        }
    }

    i32 oy = iy - p->m_anchorY;
    p->m_viewRect.top = oy;
    if (oy < 0) {
        if (wrapY) {
            p->m_viewRect.top = p->m_wrapH + oy;
        } else {
            p->m_viewRect.top = 0;
        }
    }

    // --- derive the far tile extents (clamped, unless wrapping) ---------------
    i32 ex = p->m_viewW + p->m_viewRect.left - 1;
    i32 ey = p->m_viewH + p->m_viewRect.top - 1;
    p->m_viewRect.right = ex;
    p->m_viewRect.bottom = ey;
    if (ex >= p->m_wrapW && wrapX == 0) {
        i32 over = ex - p->m_wrapW + 1;
        p->m_viewRect.right = ex - over;
        p->m_viewRect.left = p->m_viewRect.left - over;
    }
    if (ey >= p->m_wrapH && wrapY == 0) {
        i32 over = ey - p->m_wrapH + 1;
        p->m_viewRect.bottom = ey - over;
        p->m_viewRect.top = p->m_viewRect.top - over;
    }
}

// ===========================================================================
// CDDrawWorkerHost::Build (0x161e80) - re-place one plane from the level coord rect.
// Unless the rect is unset (minX == INT_MIN sentinel), copy it into the plane's
// +0x50 bounds, derive the view size (w/h = max-min+1) and the half-size anchor,
// then RecomputePlaneCoords. CGameLevel::SetExtentsAndBuildAll / BuildAllPlanes
// drive it per plane (m_planes[i]).
// EXACT since 2026-07-28. The recorded "~90.4% codegen wall in the width/height
// derivation" was a mislabeled source bug: the `i32 width = ...; m_viewW = width;`
// temporaries forced cl to compute both differences up front into two extra
// callee-saved registers and finish with `inc`, where retail interleaves the stores
// and uses `lea r,[x+1]`. Assigning the members DIRECTLY (and reading m_viewW/m_viewH
// back for the halves) reproduces retail exactly - the same fix took InitGeometry
// 78.3 -> 94.0 and CDDrawWorkerHost::Read past the same block.
// ===========================================================================
RVA(0x00161e80, 0x79)
void CDDrawWorkerHost::Build(LevelCoordRect* coords) {
    if (coords->left != static_cast<i32>(0x80000000)) {
        LevelCoordRect local;
        CopyRect((&local), (coords));
        m_bounds50 = local;
        m_viewW = m_bounds50.right - m_bounds50.left + 1;
        m_viewH = m_bounds50.bottom - m_bounds50.top + 1;
        m_anchorX = m_viewW / 2;
        m_anchorY = m_viewH / 2;
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
            CDDrawWorker* fr_ = FrameSetAt(h_ >> 16);                                              \
            i32 idx_ = static_cast<i32>(h_ & 0xffff);                                              \
            CImage* e_ = fr_->GetAt(idx_);                                                         \
            surf->BltFast((xp), (yp), e_->m_surface, (srcp), e_->m_loadResult);                    \
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

    i32 colL = m_viewRect.left >> m_shiftX;
    i32 leftW = ((colL + 1) << m_shiftX) - m_viewRect.left;
    i32 rowT = m_viewRect.top >> m_shiftY;
    i32 topH = ((rowT + 1) << m_shiftY) - m_viewRect.top;
    i32 colR = m_viewRect.right >> m_shiftX;
    i32 rightW = m_viewRect.right - (colR << m_shiftX) + 1;
    i32 rowB = m_viewRect.bottom >> m_shiftY;
    i32 botH = m_viewRect.bottom - (rowB << m_shiftY) + 1;
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
i32 CDDrawWorkerHost::RebuildPlanes(const char* base, i32 count) {
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
    i32 p3[2] = {hdr->m_rectA.w, hdr->m_rectA.h};
    i32 p4[2] = {hdr->m_rectB.w, hdr->m_rectB.h};
    i32 p5[2] = {hdr->m_rectC.w, hdr->m_rectC.h};

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
        // byte-forced: the cursor crossing into a record IS the on-disk format's
        // boundary - the block is a byte stream of variable-length records
        i32 r = ReadPlaneObjects(reinterpret_cast<const PlaneObjectRecord*>(base));
        if (r == 0) {
            return 0;
        }
        base += r;
    }
    return 1;
}

// @early-stop
// The `call 0x15b390` half is now reproduced (the CGAMEOBJECT_OOL_CTOR guard at the
// top of this file; 69.9 -> 75.0 %). Residual is this 2054-byte function's own
// regalloc/scheduling, not the ctor shape.
RVA(0x00162af0, 0x806)

i32 CDDrawWorkerHost::ReadPlaneObjects(const PlaneObjectRecord* src) {
    if (src == 0) {
        return 0;
    }

    i32 id = src->m_id;
    u32 nameLen = src->m_nameLen;
    u32 logicLen = src->m_logicLen;
    u32 imageSetLen = src->m_imageSetLen;
    u32 soundLen = src->m_soundLen;
    i32 x = src->m_x;
    i32 y = src->m_y;
    i32 z = src->m_z;
    i32 gridIndex = src->m_gridIndex;

    // `new CWwdGameObjectA(...)`: cl CALLS the shared CGameObject ctor COMDAT
    // (0x15b390) and inlines the A part - the +0x1a0 cursor (whose own CLoadable
    // base ctor it calls out-of-line at 0x156cb0), the 0x5f00a8 stamp and the
    // +0x18c..+0x19c tail.
    CWwdGameObjectA* obj = new CWwdGameObjectA(OwnerMgr(), id, 0);
    if (obj == 0) {
        return 0;
    }

    // Copy the four trailing length-prefixed strings into stack CStrings. They
    // begin right after the fixed 0x11C record.
    const char* strCursor = src->m_strings;
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
        delete obj;
        return static_cast<i32>((strCursor - src->m_strings)) + 0x11c;
    }

    // The named image set must resolve to a registered type template in the owner's
    // worker cache. CORRECTED 2026-07-27: an UNNAMED set bails too - retail 0x162d94
    // reads the CString length out of the CStringData header (`mov edx,[eax-0x8]`) and
    // `je 0x162dc3` jumps STRAIGHT to the same delete-and-bail a failed lookup takes.
    // The old `i32 loaded = 1` default let an empty name fall through into Setup.
    // The out-param is CObject*& (CMapStringToOb's interface); the narrowing is
    // language-forced - CDDrawWorkerCache::CreateWorker @0x1652c0 is that map's only
    // writer and every value it stores is a ??_7AnimWorkerObj@@6B@-stamped record.
    AnimWorkerObj* tmpl = 0;
    if (imageSet.GetLength() != 0) {
        CObject* foundOb = 0;
        OwnerMgr()->m_workerCache->m_10.Lookup(static_cast<const char*>(imageSet), foundOb);
        tmpl = static_cast<AnimWorkerObj*>(foundOb);
    }
    if (tmpl == 0) {
        delete obj;
        return static_cast<i32>((strCursor - src->m_strings)) + 0x11c;
    }

    // Slot-10 build. CORRECTED 2026-07-27 in ALL FOUR arguments (the old
    // `Setup(logicLen, id, strCursor, (CObject*)id)` carried its own @identity-TODO
    // saying one transcription had to be wrong - it was this one). Retail 0x162dd9:
    //     mov ecx,[esp+0x38] / push eax / mov eax,[esp+0x48] / push eax / push edi /
    //     push ecx / call [edx+0x28]
    // -> a1=[esp+0x38], a2=edi=[esp+0x3c], a3=[esp+0x48], a4=the Lookup out-param.
    // Those frame slots are pinned by the prologue's record walk (`lea ebp,[esi+4]` +
    // `add ebp,4` => ebp = &m_logicLen at 0x162b2b; ebp+12/+16/+20 = m_x/m_y/m_z land
    // in exactly those three slots), and the bounds guard above tests the first two
    // against m_wrapW/m_wrapH. CGameObject::Setup's own body (m_screenX=a1,
    // m_screenY=a2, m_sortKey=a3) independently confirms the x/y/z reading.
    if (obj->Setup(x, y, z, tmpl) == 0) {
        delete obj;
        return 0;
    }

    obj->m_flags |= 0x40000;

    AnimWorkerObj* anim = obj->m_7c;
    if (anim == 0) {
        delete obj;
        return 0;
    }

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
    const i32* p = src->m_tail; // record +0x28 (skip m_addFlags @+0x24)

    obj->m_flags |= static_cast<u32>(*p++); // dynamicFlags       (+0x08)
    obj->m_stateFlags = *p++;               // drawFlags          (+0x40)
    anim->m_28 = *p++;                      // userFlags
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

    anim->m_switchRectA.left = *p++;
    anim->m_switchRectA.top = *p++;
    anim->m_switchRectA.right = *p++;
    anim->m_switchRectA.bottom = *p++;
    anim->m_switchRectB.left = *p++;
    anim->m_switchRectB.top = *p++;
    anim->m_switchRectB.right = *p++;
    anim->m_switchRectB.bottom = *p++;
    anim->m_64 = *p++;
    anim->m_68 = *p++;
    anim->m_6c = *p++;
    anim->m_70 = *p++;
    anim->m_74 = *p++;
    anim->m_78 = *p++;
    anim->m_7c = *p++;
    anim->m_80 = *p++;
    anim->m_2c = *p++;
    anim->m_34 = *p++;
    anim->m_30 = *p++;
    anim->m_38 = *p++;
    obj->m_164 = *p++;
    obj->m_168 = *p++;
    anim->m_44 = *p++;
    anim->m_48 = *p++;
    anim->m_b8 = *p++;
    anim->m_bc = *p++;
    anim->m_c8 = *p++;
    anim->m_cc = *p++;
    obj->m_12c = *p++;
    obj->m_130 = *p++;
    anim->m_20 = *p++;
    anim->m_24 = *p++;
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

    return static_cast<i32>((strCursor - src->m_strings)) + 0x11c;
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
// inline epilogues, 83.5%->87.9%). Two residuals: (1) retail SHRINK-WRAPS the
// callee-save pushes - only ebp/esi before the null guard, edi/ebx after it passes -
// while this build pushes all four upfront (the positive-gate lever was measured on
// THESE two and cost 15 points, docs/patterns/positive-gate-enables-shrink-wrap.md);
// (2) the mid-point `add` loads m_40-first (A) / m_48-first (B) in retail. Measured
// 2026-07-28: swapping the two `+` operands is canonicalized away (byte-identical
// both ways, in both functions), and routing the pair through an inlined 2-arg
// helper - whose args ARE materialized right-to-left - gives 0x48/0x4c-first in
// BOTH, i.e. it fixes B and cannot fix A. No spelling reaches A's low-offset-first
// order. See docs/patterns/shrink-wrapped-callee-save-push.md.
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
        x = (m_viewRect.left + m_viewRect.right) / 2 + 1;
    }

    i32 y;
    if (flags & 0x8) {
        y = static_cast<i32>(m_scaledY);
        return scroll->ScrollTo(x, y);
    }
    y = (m_viewRect.top + m_viewRect.bottom) / 2 + 1;
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
        x = (m_viewRect.right + m_viewRect.left) / 2 + 1;
    }

    i32 y;
    if (flags & 0x8) {
        y = static_cast<i32>(m_scaledY);
        return scroll->Relocate(x, y);
    }
    y = (m_viewRect.bottom + m_viewRect.top) / 2 + 1;
    return scroll->Relocate(x, y);
}

RVA(0x001633e0, 0x12)
i32 CDDrawWorkerHost::GetSize() {
    if (m_scroll == 0) {
        return 0;
    }
    return m_scroll->GetSize();
}

// EXACT. The ex "LOCAL AREA IS TWO DWORDS SHORT" wall was a MIS-MODEL, and the frame
// itself was the evidence: retail's `sub esp,0x10` reserves FOUR dword homes and writes
// only two of them (b.h at [esp+0x14], c.h at [esp+0x1c]), leaving holes exactly where
// b.w and c.w would sit - the contiguous-ascending-frame-slots signature of a local
// AGGREGATE (docs/patterns/local-rect-aggregate-from-contiguous-frame-slots.md). Six
// loose scalar locals give cl a home only for the two dims it spills, so the frame came
// out 8 bytes short. Copying the B and C pairs as LevelDims aggregates reserves all
// four homes and every displacement lands.
RVA(0x00163420, 0xf0)
void CDDrawWorkerHost::InitScrollRects() {
    if (m_scroll == 0) {
        return;
    }
    CGameLevel* g = OwnerMgr()->m_level;
    if (g == 0) {
        return;
    }

    i32 c8 = g->m_rectA.w;
    i32 cc = g->m_rectA.h;
    // The B and C pairs are copied as AGGREGATES: that 4-dword contiguous local block
    // is what reserves retail's `sub esp,0x10` (it writes only the two dims it spills
    // and leaves the b.w/c.w homes as holes). Six loose scalar locals give cl a home
    // for the two spills only, and the frame comes out 8 bytes short.
    LevelDims b;
    b.w = g->m_rectB.w;
    b.h = g->m_rectB.h;
    LevelDims c;
    c.w = g->m_rectC.w;
    c.h = g->m_rectC.h;

    CWwdSpatialMgr* s = m_scroll;
    s->m_rect0.left = 0;
    s->m_rect0.top = 0;
    s->m_rect0.right = c8 - 1;
    s->m_rect0.bottom = cc - 1;
    s->m_org0x = c8 / 2;
    s->m_org0y = cc / 2;

    s = m_scroll;
    s->m_rect1.left = 0;
    s->m_rect1.top = 0;
    s->m_rect1.right = b.w - 1;
    s->m_rect1.bottom = b.h - 1;
    s->m_org1x = b.w / 2;
    s->m_org1y = b.h / 2;

    s = m_scroll;
    s->m_rect2.left = 0;
    s->m_rect2.top = 0;
    s->m_rect2.right = c.w - 1;
    s->m_rect2.bottom = c.h - 1;
    s->m_org2x = c.w / 2;
    s->m_org2y = c.h / 2;

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
// The diagnostics APPEND (strcat), they do not overwrite: retail's inlined copy scans
// the DESTINATION with a second `or ecx,-1 / repnz scasb / dec edi` before the
// `rep movs`, which is cl5's inline strcat end-of-dest search - a plain strcpy emits
// only the source scan. (That 11-byte hole was filed as "inlined-sprintf/strcpy
// register scheduling"; fixing it took 92.5 -> 97.0.)
// @early-stop
// 97.0%: one register-coloring residue left. Retail parks the tile handle in ecx (the
// dead m_colOffsets base) and handle>>16 in eax, paying TWO `mov eax,ecx` copies; cl
// parks the handle in eax (the dead m_tileGrid base, which dies one instruction later)
// and needs only one copy - a strictly cheaper colouring, so no source spelling asks
// for retail's. Hoisting handle>>16 into its own local changes nothing.
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
            u32 setIdx = static_cast<u32>(handle) >> 16;
            CDDrawWorker* frame = FrameSetAt(setIdx);
            if (frame == 0) {
                result = 0;
                if (errOut != 0) {
                    sprintf(
                        msg,
                        "Plane %s: Bad map image set value (%i) at %i,%i\n",
                        m_name,
                        setIdx,
                        col,
                        row
                    );
                    strcat(errOut, msg);
                }
                continue;
            }
            i32 tile = handle & 0xffff;
            CImage* resolved = frame->GetAt(tile);
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
                    strcat(errOut, msg);
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

    // The cached worker's palette chain (+0x64 -> +0x10 -> +0x0c). Identity closed:
    // the cached worker is a CAniRecordBase2 (what every m_map1 value already is) and
    // its m_buf is a CDDPalette, whose m_cacheA at +0x0c is the live 256-entry
    // PALETTEENTRY table - hence the [i*4+0..2] R/G/B reads below.
    CAniRecordBase2* owner = OwnerMgr()->m_workerMap->m_cachedWorker;
    if (owner == 0) {
        return;
    }
    PALETTEENTRY* pal = owner->m_buf->m_cacheA;
    if (pal == 0) {
        return;
    }

    m_bltFx.dwFillColor = static_cast<u16>(
        ((static_cast<u8>((static_cast<u8>(pal[idx].peRed) >> static_cast<u8>(g_rDown))) << g_rUp)
         | (static_cast<u8>((static_cast<u8>(pal[idx].peGreen) >> static_cast<u8>(g_gDown)))
            << g_gUp)
         | static_cast<u8>((static_cast<u8>(pal[idx].peBlue) >> static_cast<u8>(g_bDown))))
    );
}

// The inactive arms are spelled `return 1`, not `break`: cl5 de-duplicates identical
// EMPTY (`break`) arms BEFORE it decides the switch lowering, so 6 case labels with 4
// empty ones collapse to a 3-target subtract ladder. Spelling every arm with its own
// `return 1` keeps six distinct arms alive through the decision, cl emits retail's
// dense `add eax,-3 / cmp eax,5 / ja / jmp [eax*4+table]` (the table's 6 entries do
// share 3 targets - the merge happens after), and the `s` null test moves off eax so
// the `xor eax,eax` return-0 appears. The two ACTIVE arms must still `break` (an
// explicit `return 1` there makes cl fold the guard into `return Save(s) != 0`, i.e.
// `neg/sbb/neg`, instead of retail's `test eax,eax / jne <shared return 1>`).
// See docs/patterns/switch-empty-arms-dedup-before-jumptable.md.
// @early-stop
// BYTE-EXACT, scored 68% by a tooling artifact - verified with `llvm-objdump -dr` on
// build/objdiff/base/levelplane.obj against retail 0x163710..0x163752: all 66 bytes
// agree (only the jump-table DIR32 and the two rel32 call targets are reloc-masked).
// The score is the delinker/objdiff jump-table symbol split: cl emits the arms under
// their own `$L<n>` label symbols (the jump table needs DIR32 relocs to them), so
// objdiff pairs only the first 0x1e bytes of our symbol against retail's whole 0x42.
// Same artifact as CGameObject::Play @0x151150 (scored 0.00%). MAX 79.27 was the
// WRONG (compare-ladder) shape; do not revert to it.
RVA(0x00163710, 0x42)
i32 CDDrawWorkerHost::SerializeDispatch(CFileMemBase* s, i32 kind, i32, i32) {
    if (!s) {
        return 0;
    }
    switch (kind) {
        case 3:
            return 1;
        case 4:
            if (!Save(s)) {
                return 0;
            }
            break;
        case 5:
            return 1;
        case 6:
            return 1;
        case 7:
            if (!Load(s)) {
                return 0;
            }
            break;
        case 8:
            return 1;
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
    s->Write(&m_viewRect.left, 0x10);
    s->Write(&m_zBound, 4);
    s->Write(&m_snappedX, 4);
    s->Write(&m_snappedY, 4);
    s->Write(&m_94, 4);
    s->Write(&m_98, 4);

    // width FIRST here (Read/Load spell it the other way round): the multiply's operand
    // order is byte-visible - cl leaves the first operand in the register for the `imul`.
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
    s->Read(&m_viewRect.left, 0x10);
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
