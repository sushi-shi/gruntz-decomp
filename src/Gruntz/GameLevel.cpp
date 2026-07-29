#include <Wwd/WwdFile.h> // C linkage for the definitions below (inherited, not restated)
#include <Mfc.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/SerialArchive.h>     // CFileMemBase (== CFileMemBase; the EditDispatch stream)
#include <Io/FileMem.h>               // CFileMemBase complete type (Read/Write dispatch)
#include <Wap32/Object.h>             // CObject grand-base (slots 0-4) for the CImageSetN variants
#include <Gruntz/ParseSource.h>       // canonical CParseSource (BeginParse/EndParse)
#include <Gruntz/UserLogic.h>         // canonical CGameObject (the movement target)
#include <DDrawMgr/DDrawSurfaceMgr.h> // the m_0c world root (the chain owner)
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup (the object chain)
#include <Io/FileStream.h>            // CFile (Open/Read/GetLength/ctor/dtor reloc-masked)
#include <Gruntz/ImageSets.h>         // CImageSet1/2/3 variant records + RezAlloc/RezFree
#include <DDrawMgr/DDrawWorkerHost.h> // the REAL plane class (CDDrawWorkerHost == CDDrawWorkerHost)
#include <rva.h>

#include <stdlib.h> // abs - the /Oi intrinsic (cdq/xor/sub), NOT an `if (x<0) x=-x;` branch
#include <string.h> // strcpy, memset

static const i32 AXIS_UNSET = static_cast<i32>(0x80000000);

static inline void StampParamBlock(CGameLevel* o) {
    o->m_pairA[0] = 500;
    o->m_pairA[1] = 250;
    o->m_pairB[0] = 1000;
    o->m_pairB[1] = 1000;
    o->m_pairC[0] = 250;
    o->m_pairC[1] = 125;
    o->m_rectA.w = 1600;
    o->m_rectA.h = 1200;
    o->m_rectB.w = 2560;
    o->m_rectB.h = 1920;
    o->m_rectC.w = 768;
    o->m_rectC.h = 576;
}

// ===========================================================================
// CGameLevel ("CDDrawResolveSubMgrLayout") constructor. Three args (ret 0xc): they land at
// +0x4, +0x8, +0xc. Inlined base ctor (CLoadable, in the header) stamps the
// CLoadable base vftable @0x5efc30 and the args, then the three MFC arrays at
// +0x20/+0x34/+0x48 are constructed, then the derived CGameLevel vftable @0x5f0150
// is stamped and the +0x10 sentinel, the +0x5c/+0x60 main-plane fields, the
// +0x64/+0x68 pair (0x40), and the shared +0xb0..+0xdc default-parameter block are
// written. Carries the /GX EH frame because the three array members are
// destructible.
//
// @early-stop
// reloc-name mask + store-scheduling entropy plateau (~94%). Re-pinning the arrays to
// their genuine shape (CByteArray + two CDWordArrays, all out-of-line ctors) fixed the
// whole array-construction prologue: the three `leal +0x20/+0x34/+0x48; movb EH-state
// 0/1/2; call ??0..Array` sequence and the two cl-emitted vptr stores (base ??_7CLoadable
// orphan + derived ??_7CGameLevel @0x5f0150) now match retail exactly (48.8%->94.4%).
// The three header words are stamped in the BASE-ctor phase - retail writes m_id /
// m_flags / m_ownerCtx between the CLoadable vptr stamp and the m_planes/m_imageSets
// member ctors, which a leaf BODY can never reach - so the ctor DELEGATES to
// CLoadable(id, flags, owner) instead of respelling the three stores (61.8% -> 75%).
// Two residuals remain, neither source-steerable: (1) reloc-name masks - retail ICF-
// folded the identical CByteArray/CDWordArray default ctors to ONE `CByteArray` symbol,
// so our two `??0CDWordArray@@QAE@XZ` calls + the `push $handler` funcinfo mask against
// retail's folded names; (2) the tail store scheduling - cl parks the 0xfa immediate in
// eax and stamps the ??_7CGameLevel vptr before the m_b4/m_c0 stores, while retail keeps
// 0xfa in ecx and floats the vptr stamp later (matching-patterns.md entropy: an
// independent immediate-to-memory store has no dep to pin its slot). Logic + offsets +
// CFG + EH frame exact.
RVA(0x0015ccd0, 0x118)
CGameLevel::CGameLevel(CDDrawSurfaceMgr* owner, i32 a2, i32 a3) : CLoadable(a2, a3, owner) {
    // the three header words are stamped in the BASE-ctor phase (retail writes them
    // between the CLoadable vptr stamp and the m_planes/m_imageSets member ctors,
    // which the leaf body cannot reach) - delegate, do not respell them here.
    m_maxStepX = 0x40;
    m_maxStepY = 0x40;
    m_pairA[1] = 250;
    m_pairC[0] = 250;
    m_pairB[0] = 1000;
    m_pairB[1] = 1000;

    // cl auto-stamps &??_7CGameLevel here (the derived phase of the two-phase store).
    m_planeCtx.left = LEVEL_COORD_UNSET;
    m_mainPlane = 0;
    m_mainIndex = -1;
    m_checksum = 0;
    m_pairA[0] = 500;
    m_pairC[1] = 125;
    m_rectA.w = 1600;
    m_rectA.h = 1200;
    m_rectB.w = 2560;
    m_rectB.h = 1920;
    m_rectC.w = 768;
    m_rectC.h = 576;
}

RVA(0x0015d280, 0x279)
i32 CGameLevel::LoadWwd(WwdHeader* hdr) {
    ReleaseChildren(); // vtable +0x44 (slot 17), the pre-load reset

    if (hdr->wwdSignature > 0x5f4) { // signature must be <= 1524
        return 0;
    }

    // Copy the 1524-byte header into the level object (rep movs 0x17d dwords).
    m_header = *hdr;

    // block starts as the header itself (uncompressed planes follow in place); the
    // COMPRESS branch overwrites it with the inflated main block. Initializing block
    // to hdr at the top makes its live range begin at the hdr load, which is why the
    // retail compiler pins `block` in the callee-saved register and reloads `hdr`'s
    // own fields through a spilled pointer for the rest of the function.
    // the header IS the head of the file image and every table below is located by a
    // runtime offset from it - the byte cursor is byte-forced by the on-disk format
    char* block = reinterpret_cast<char*>(hdr);
    Bytef* ehAlloc = 0; // inflate buffer freed on every exit path

    // The flags field is read twice (the COMPRESS test and the m_flags store); the
    // retail compiler materializes &hdr->flags once and dereferences it both times,
    // so model it as a cached pointer.
    u32* pflags = &hdr->flags;

    if (*pflags & 0x2) // COMPRESS: inflate the main block
    {
        u32 allocSize = hdr->mainBlockLength + hdr->wwdSignature + 0x40;
        Bytef* buf = static_cast<Bytef*>(operator new(allocSize));
        if (buf == 0) {
            return 0;
        }

        // same cursor, now over the inflated image - byte-forced by the format
        block = reinterpret_cast<char*>(WwdFile_InflateMainBlock(hdr, buf, allocSize - 0x20));
        if (block == 0) {
            operator delete(buf);
            return 0;
        }
        ehAlloc = buf;
    }

    strcpy(m_levelName, hdr->levelName); // inline strlen + rep movs
    m_flags = *pflags;
    m_checksum = hdr->checksum;

    i32 result = 0; // image-set result (the >=0 success / -1 failure sentinel)

    // --- plane loop ---------------------------------------------------------
    char* cursor = block + hdr->planesOffset;
    u32 i = 0;
    // `> 0` on the UNSIGNED count, not `!= 0`: retail's guard is `test eax,eax / jbe`,
    // which is the negation of an unsigned `> 0` (CF is always clear after `test`, so the
    // encoding is the only difference and only the source operator picks it).
    if (hdr->numPlanes > 0) {
        do {
            // byte-forced by the on-disk format: the plane records are packed 0xa0
            // apart inside the mapped main block, located by a RUNTIME dword offset
            if (ReadPlane(reinterpret_cast<const WwdPlaneHeader*>(cursor), block, &m_planeCtx)
                == 0) {
                goto fail;
            }
            ++i;
            cursor += 0xa0; // WwdPlaneHeader stride
        } while (i < hdr->numPlanes);
    }

    // --- image-set descriptors ---------------------------------------------
    // The descriptor read is its own int-returning routine (inlined here): it
    // validates the record pointer, walks `count` descriptors appending each
    // CTileImageSet, and returns the number read (or -1 on a bad pointer / failed
    // read). count is re-read from the record header each iteration (rec is spilled).
    if (hdr->tileDescriptionsOffset != 0) {
        // same byte cursor as the plane loop - byte-forced by the on-disk format: the
        // table is located by a RUNTIME dword offset from the mapped block, so no
        // declared member can name it (retail 0x15d3a4 `mov eax,[edx+0x2e4]`)
        WwdTileDescTable* rec = // byte-forced by the on-disk format (see above)
            reinterpret_cast<WwdTileDescTable*>(block + hdr->tileDescriptionsOffset);
        char* elem = rec->m_descriptors;
        if (elem == 0) {
            result = -1;
        } else if (rec == 0) {
            result = -1;
        } else {
            i32 n = 0;
            i32 j = 0;
            while (static_cast<u32>(j) < rec->m_count) {
                CTileImageSet* set = ReadImageSet(elem);
                if (set == 0) {
                    result = -1;
                    goto check_result;
                }
                ++n;
                elem += set->GetStride(); // vtable +0x24 stride advance
                m_imageSets.SetAtGrow(j, static_cast<CObject*>(set));
                ++j;
            }
            result = n;
        }
    check_result:
        if (result < 0) {
            goto fail;
        }
    }

    // --- scaled start coords on the main plane + every plane ---------------
    // For each plane the WWD start coords are placed (and, unless the plane has
    // the origin-fixed flag bit0, multiplied by the plane's parallax factors)
    // into m_scaledX/m_scaledY; then the per-plane coord recompute runs.
    {
        i32 startX = hdr->startX;
        i32 startY = hdr->startY;
        CDDrawWorkerHost* mp = m_mainPlane;
        if (mp->m_flags & 1) {
            mp->m_scaledX = static_cast<float>(startX);
            mp->m_scaledY = static_cast<float>(startY);
        } else {
            mp->m_scaledX = static_cast<float>(startX) * mp->m_scaleX;
            mp->m_scaledY = static_cast<float>(startY) * mp->m_scaleY;
        }
        mp->RecomputePlaneCoords();

        // Re-derive the start coords from the main plane's origin for the rest.
        i32 ox = m_mainPlane->m_snappedX;
        i32 oy = m_mainPlane->m_snappedY;
        i32 i2 = 0;
        while (i2 < m_planes.GetSize()) // GetSize() == the plane count
        {
            if (i2 != m_mainIndex) {
                CDDrawWorkerHost* p = static_cast<CDDrawWorkerHost*>(m_planes[i2]);
                if (p->m_flags & 1) {
                    p->m_scaledX = static_cast<float>(ox);
                    p->m_scaledY = static_cast<float>(oy);
                } else {
                    p->m_scaledX = static_cast<float>(ox) * p->m_scaleX;
                    p->m_scaledY = static_cast<float>(oy) * p->m_scaleY;
                }
                p->RecomputePlaneCoords();
            }
            ++i2;
        }
    }

    if (ehAlloc != 0) {
        operator delete(ehAlloc);
    }
    return 1;

fail:
    if (ehAlloc != 0) {
        operator delete(ehAlloc);
    }
    return 0;
}

RVA(0x00161190, 0x1f)
i32 CGameLevel::IsLoaded() {
    if (m_planeCtx.left == LEVEL_COORD_UNSET) {
        goto fail;
    }
    if (m_ownerCtx == 0) {
        goto fail;
    }
    if (m_id != -1) {
        return 1;
    }

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// SetCoordExtents: zeroes the min corner of the +0x10 record, stores (w-1, h-1)
// as the max corner, stamps the default-extents block, returns 1.
//
// RESIDUE (~84%, NOT a logic/offset/type/CFG error): byte-for-byte identical to
// the target EXCEPT the position of one instruction - the immediate store
// `mov dword ptr [ecx+0xb0], 0x1f4`. The retail compiler schedules it mid-block
// (after +0xbc, before +0xb4); here MSVC hoists the same store to the earliest
// free slot (right after `mov eax,[esp+4]`, before `dec eax`). Same bytes, same
// register allocation everywhere else. This is the documented store-scheduling
// entropy (matching-patterns.md "optimizer reorders field stores"): an
// independent immediate-to-memory store has no register dependency to pin it, so
// MSVC floats it freely. Every source ordering tried either kept this single
// slip or regressed the eax(0x3e8)/edx(0xfa) allocation (b8,bc,b0,b4 order ->
// ~75%); calling the param block before the +0x10 writes moves the whole block
// ahead (wrong). Logic + offsets + CFG are exact, so this is left as the plateau.
// @early-stop
// store-scheduling entropy (~84%): the body is byte-exact EXCEPT the independent
// m_pairA[0]=500 direct-immediate store, which cl hoists into the w-read/dec window while
// retail emits it after the m_b8/m_pairB[1]=1000 stores. Inlining the block in retail's
// store order regressed it further (74.8%); not source-steerable. Deferred.
RVA(0x0015d030, 0x92)
i32 CGameLevel::SetCoordExtents(i32 w, i32 h) {
    m_planeCtx.left = 0;
    m_planeCtx.top = 0;
    m_planeCtx.right = w - 1;
    m_planeCtx.bottom = h - 1;
    StampParamBlock(this);
    return 1;
}

RVA(0x0015d500, 0x127)
i32 CGameLevel::LoadFromFile(const char* path) {
    CFile file;

    if (!file.Open(path, 0, 0)) {
        return 0;
    }

    void* buf = operator new(file.GetLength());
    if (!buf) {
        return 0;
    }

    file.Read(buf, file.GetLength());
    if (LoadWwd(static_cast<WwdHeader*>(buf)) == 0) { // vtable +0x38 (slot 14) load virtual
        operator delete(buf);
        return 0;
    }
    operator delete(buf);
    return 1;
}

RVA(0x0015d630, 0x41)
i32 CGameLevel::LoadFromSource(CParseSource* arg) {
    char* handle = arg->BeginParse();
    if (handle == 0) {
        return 0;
    }
    // BeginParse is a PROVEN-heterogeneous slot: the same virtual yields a WWD block
    // here, a RIFF blob in RezSync and a raw size in DDrawSubMgrLeafScan, so its return
    // stays the generic handle the shared ParseSource.h declares and each consumer names
    // the concrete record at its own seam. This is that one seam for the WWD source.
    if (LoadWwd(reinterpret_cast<WwdHeader*>(handle)) == 0) { // vtable +0x38 (slot 14) load virtual
        arg->EndParse();
        return 0;
    }
    arg->EndParse();
    return 1;
}

// ---------------------------------------------------------------------------
// Scalar-deleting destructor (vtable slot 1): run the destructor, then free the
// object when bit0 of the flag is set; returns `this`. The compiler-standard thunk.
// The scalar-deleting dtor is the compiler-generated ??_G (folded from ~CGameLevel);
// pin it by mangled name since it has no source body.
RVA_COMPGEN(0x001611c0, 0x1e, ??_GCGameLevel@@UAEPAXI@Z)

RVA(0x001611e0, 0x82)
CGameLevel::~CGameLevel() {
    Unload(); // level cleanup (releases children, clears the header)
    // m_imageSets / m_planes / m_array20 auto-destruct here; ~CLoadable follows.
}

RVA(0x0015d1f0, 0x87)
void CGameLevel::Unload() {
    i32 i;
    for (i = 0; i < m_planes.GetSize(); i++) {
        CDDrawWorkerHost* child = static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i]);
        if (child) {
            delete child; // the inherited virtual scalar-deleting dtor (+0x04, flag 1)
        }
    }
    m_planes.SetSize(0, -1);
    for (i = 0; i < m_imageSets.GetSize(); i++) {
        CTileImageSet* child = static_cast<CTileImageSet*>(m_imageSets.GetData()[i]);
        if (child) {
            delete child; // the inherited virtual scalar-deleting dtor (+0x04, flag 1)
        }
    }
    m_imageSets.SetSize(0, -1);
    m_planeCtx.left = LEVEL_COORD_UNSET;
    m_mainPlane = 0;
    m_mainIndex = -1;
    memset(&m_header, 0, 1524);
}

RVA(0x0015d680, 0x71)
void CGameLevel::ReleaseChildren() {
    i32 i;
    for (i = 0; i < m_planes.GetSize(); i++) {
        CDDrawWorkerHost* child = static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i]);
        if (child) {
            delete child; // the inherited virtual scalar-deleting dtor (+0x04, flag 1)
        }
    }
    m_planes.SetSize(0, -1);
    for (i = 0; i < m_imageSets.GetSize(); i++) {
        CTileImageSet* child = static_cast<CTileImageSet*>(m_imageSets.GetData()[i]);
        if (child) {
            delete child; // the inherited virtual scalar-deleting dtor (+0x04, flag 1)
        }
    }
    m_imageSets.SetSize(0, -1);
    m_mainPlane = 0;
    m_mainIndex = -1;
}

RVA(0x0015cdf0, 0xb8)
i32 CGameLevel::SetCoordsAndLoad40(const char* path, LevelCoordRect* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    if (LoadFromFile(path) == 0) { // vtable +0x40 (slot 16)
        Unload();                  // vtable +0x1c (slot 7), fail/reset hook
        return 0;
    }
    return 1;
}

RVA(0x0015ceb0, 0xb8)
i32 CGameLevel::SetCoordsAndLoad3C(CParseSource* src, LevelCoordRect* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    if (LoadFromSource(src) == 0) { // vtable +0x3c (slot 15)
        Unload();                   // vtable +0x1c (slot 7), fail/reset hook
        return 0;
    }
    return 1;
}

RVA(0x0015cf70, 0xb8)
i32 CGameLevel::SetCoordsAndLoad38(WwdHeader* hdr, LevelCoordRect* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    if (LoadWwd(hdr) == 0) { // vtable +0x38 (slot 14)
        Unload();            // vtable +0x1c (slot 7), fail/reset hook
        return 0;
    }
    return 1;
}

RVA(0x0015d0d0, 0x99)
i32 CGameLevel::SetCoords(LevelCoordRect* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    return 1;
}

RVA(0x0015d820, 0xa3)
CTileImageSet* CGameLevel::ReadImageSet(void* record) {
    if (record == 0) {
        return 0;
    }
    CTileImageSet* set;
    switch (*static_cast<i32*>(record)) {
        case 1:
            set = new CImageSet1;
            break;
        case 2:
            set = new CImageSet2;
            break;
        case 3:
            set = new CImageSet3;
            break;
        default:
            return 0;
    }

    if (set->Parse(record) == 0) {
        if (set != 0) {
            delete set; // the inherited virtual scalar-deleting dtor (+0x04, flag 1)
        }
        return 0;
    }
    return set;
}

RVA(0x0015d8d0, 0xc3)
CDDrawWorkerHost*
CGameLevel::ReadPlane(const WwdPlaneHeader* planeData, const char* blockBase, void* /*unused*/) {
    CDDrawWorkerHost* plane = new CDDrawWorkerHost(OwnerMgr(), m_planes.GetSize(), 0);

    if (plane->Read(planeData, blockBase, &m_planeCtx) == 0) {
        if (plane) {
            delete plane; // the virtual scalar-deleting dtor (vtable +0x4, flag 1)
        }
        return 0;
    }

    m_planes.SetAtGrow(m_planes.GetSize(), static_cast<CObject*>(plane));

    if (plane->m_flags & 1) // MAIN plane
    {
        m_mainPlane = plane;
        m_mainIndex = m_planes.GetSize() - 1;
    }

    return plane;
}

RVA(0x0015d9a0, 0xdc)
CDDrawWorkerHost*
CGameLevel::ReadObjectPlane(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, const char* name) {
    CDDrawWorkerHost* plane = new CDDrawWorkerHost(OwnerMgr(), m_planes.GetSize(), 0);

    if (plane->InitGeometry(a1, a2, a3, a4, a5, a6, &m_planeCtx, const_cast<char*>(name)) == 0) {
        if (plane) {
            delete plane; // the virtual scalar-deleting dtor (vtable +0x4, flag 1)
        }
        return 0;
    }

    m_planes.SetAtGrow(m_planes.GetSize(), static_cast<CObject*>(plane));

    if (plane->m_flags & 1) // MAIN plane
    {
        m_mainPlane = plane;
        m_mainIndex = m_planes.GetSize() - 1;
    }

    return plane;
}

RVA(0x00161270, 0xb2)
i32 CGameLevel::AxisProbe(i32 coord, i32 limit) {
    // Same shape as PROBE_TILE, but the standalone reads the second coord only AFTER
    // clamping the first (retail defers the edi load past the X-clamp block).
    i32 px = coord;
    if (px < 0) {
        px = 0;
    } else {
        if (px >= m_mainPlane->m_wrapW) {
            px = m_mainPlane->m_wrapW - 1;
        }
    }
    i32 py = limit;
    if (py < 0) {
        py = 0;
    } else {
        if (py >= m_mainPlane->m_wrapH) {
            py = m_mainPlane->m_wrapH - 1;
        }
    }
    CDDrawWorkerHost* pl = m_mainPlane;
    i32 qx = px >> pl->m_shiftX;
    i32 qy = py >> pl->m_shiftY;
    i32 col = qx;
    i32 subX = px - (qx << pl->m_shiftX);
    i32 idx = pl->m_colOffsets[qy] + col;
    i32 subY = py - (qy << pl->m_shiftY);
    i32 tile = pl->m_tileGrid[idx];
    if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
        return 0;
    }
    CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
    return set->GetCollisionAt(subX, subY);
}

typedef CFileMemBase EditSink;

RVA(0x0006b330, 0x2a)
i32 CGameLevel::PointInBounds(const LevelCoordRect* r, i32 x, i32 y) {
    if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) {
        return 1;
    }
    return 0;
}

RVA(0x00082600, 0x73)
i32 CGameLevel::LookupTile(i32 x, i32 y) {
    CDDrawWorkerHost* mp;
    if (x < 0) {
        x = 0;
    } else {
        mp = m_mainPlane;
        if (x >= mp->m_gridW) {
            x = mp->m_gridW - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        mp = m_mainPlane;
        if (y >= mp->m_gridH) {
            y = mp->m_gridH - 1;
        }
    }
    mp = m_mainPlane;
    i32 tile = mp->m_tileGrid[mp->m_colOffsets[y] + x];
    if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
        return 0;
    }
    CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
    return set->GetCollisionAt(0, 0); // slot +0x20, called with (0, 0)
}

RVA(0x00160ee0, 0xd)
void CGameLevel::MainPlaneNotify() {
    if (m_mainPlane != 0) {
        m_mainPlane->InitScrollRects(); // 0x163420
    }
}

RVA(0x00160ef0, 0x42)
i32 CGameLevel::ValidateAllPlanes(char* errOut) {
    i32 ok = 1;
    if (errOut != 0) {
        *errOut = 0;
    }
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        if ((static_cast<CDDrawWorkerHost*>(m_planes[i]))->ValidateTiles(errOut) == 0) { // 0x163510
            ok = 0;
        }
    }
    return ok;
}

RVA(0x0015da80, 0x47)
void CGameLevel::BuildAllPlanes(LevelCoordRect* coords) {
    m_planeCtx = *coords;
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        (static_cast<CDDrawWorkerHost*>(m_planes[i]))->Build(coords);
    }
}

// ---------------------------------------------------------------------------
// SetExtentsAndBuildAll: when both w and h are positive, build the half-open box
// {0, 0, w-1, h-1} into a local LevelCoordRect, PUBLISH it into m_planeCtx with a
// whole-struct assignment, then drive Build(&local) on every plane. Returns 1
// (0 if either extent is non-positive).
//
// The struct assignment is what the old "zero-pin regalloc wall" note was really
// looking at: `m_planeCtx = rect` is why retail dedicates a 4th callee-saved reg
// to `lea ebx,[esi+0x10]` (the copy destination) and why the zero appears TWICE
// (`xor eax,eax` for the local's stores, `xor ecx,ecx` for the copy) instead of
// being pinned once. Spelling the eight fields as interleaved member+local stores
// could not produce either. Now EXACT.
RVA(0x0015d700, 0x81)
i32 CGameLevel::SetExtentsAndBuildAll(i32 w, i32 h) {
    if (w <= 0) {
        return 0;
    }
    if (h <= 0) {
        return 0;
    }
    i32 maxX = w - 1;
    i32 maxY = h - 1;
    LevelCoordRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = maxX;
    rect.bottom = maxY;
    m_planeCtx = rect;
    i32 i = 0;
    if (m_planes.GetSize() > 0) {
        do {
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i]))->Build(&rect);
            ++i;
        } while (i < m_planes.GetSize());
    }
    return 1;
}

RVA(0x0015dad0, 0x2c)
void CGameLevel::SyncToMainIndex(void* visitor) {
    i32 i = 0;
    if (m_mainIndex >= 0) {
        do {
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i]))
                ->Draw(static_cast<CPlaneDrawCtx*>(visitor)); // 0x162010
            ++i;
        } while (i <= m_mainIndex);
    }
}

RVA(0x0015db00, 0x2e)
void CGameLevel::SyncAfterMainIndex(void* visitor) {
    i32 i = m_mainIndex + 1;
    if (i < m_planes.GetSize()) {
        do {
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i]))
                ->Draw(static_cast<CPlaneDrawCtx*>(visitor)); // 0x162010
            ++i;
        } while (i < m_planes.GetSize());
    }
}

// ---------------------------------------------------------------------------
// FindPlaneByName: case-insensitive search for the plane named `name`; null if none.
// ---------------------------------------------------------------------------
// MoveToward: drive DispatchMove toward (arg1, arg2) on `target`, never moving more
// than this level's per-axis step limits (m_maxStepX/m_maxStepY) at once. A direct call when
// the move is within limits or forced by the target flags / kind 7; otherwise an
// incremental stepping loop that re-runs DispatchMove until it reaches the goal or is
// reported blocked.
//
// @early-stop
// 76.0% (was 70.8) and the basic-block SKELETON is now IDENTICAL to retail - 32 blocks,
// every edge matching, three of them instruction-for-instruction. Two real bugs were
// fixed: the distance tests are the /Oi `abs()` intrinsic (`cdq/xor/sub`), not
// `if (d<0) d=-d;` (docs/patterns/abs-intrinsic-cdq-xor-sub-vs-hand-rolled-negate.md,
// which alone cost 5 extra blocks), and the four loop-exit conditions were an inverted
// nested test that CONTINUED when the move mode changed and when DispatchMove reported
// "the scroll did not move" (0x400000) - i.e. it could spin. Residue is the callee-saved
// ASSIGNMENT: retail spills `this` to a stack home and keeps sx/arg2/arg1 in ebx/edi/ecx
// (2 local dwords + three parameter homes reused for ok/stepX/goalX), cl keeps `this` in
// ebx and spills less. Deferred to the final sweep.
RVA(0x0015de40, 0x164)
i32 CGameLevel::MoveToward(CGameObject* target, i32 arg1, i32 arg2, i32 arg3) {
    CGameObject* t = target;
    i32 limX = m_maxStepX;
    // t->m_screenX is read ONCE and kept (retail holds it in ebx from the prologue right
    // through the `sx > arg1` step-sign test); m_maxStepY is read at the stepY point, not
    // hoisted next to limX (retail's `mov ebp,[this+0x68]` sits inside the loop preamble).
    i32 sx = t->m_screenX;

    // abs(), not `if (d < 0) d = -d;`: retail's `cdq / xor eax,edx / sub eax,edx` IS the
    // /Oi abs intrinsic; the hand-rolled negate emits a `jns`/`neg` branch pair and splits
    // the block skeleton.
    i32 dx = abs(sx - arg1);
    if (dx <= limX) {
        i32 dy = abs(t->m_screenY - arg2);
        if (dy <= m_maxStepY) {
            return DispatchMove(target, arg1, arg2, arg3);
        }
    }

    if (t->m_flags & 0x10) {
        return DispatchMove(target, arg1, arg2, arg3);
    }

    i32 kind = t->m_moveMode;
    if (kind == 7) {
        return DispatchMove(target, arg1, arg2, arg3);
    }

    // --- incremental stepping toward (arg1, arg2) ---------------------------
    i32 stepX = limX;
    i32 goalX = arg1;
    if (sx > arg1) {
        stepX = -stepX;
    }
    i32 stepY = m_maxStepY;
    if (t->m_screenY > arg2) {
        stepY = -stepY;
    }

    i32 ok = 1;
    do {
        i32 nx = stepX + t->m_screenX;
        if (stepX > 0) {
            if (nx > goalX) {
                nx = goalX;
            }
        } else {
            if (nx < goalX) {
                nx = goalX;
            }
        }
        i32 ny = stepY + t->m_screenY;
        if (stepY > 0) {
            if (ny > arg2) {
                ny = arg2;
            }
        } else {
            if (ny < arg2) {
                ny = arg2;
            }
        }

        i32 flags = DispatchMove(target, nx, ny, arg3);

        // Four INDEPENDENT stop conditions, each `ok = 0` (retail: `jne <ok=0>` twice, then
        // the goal pair, then `test eax,0x400000 / je <skip the store>`). `ok` is seeded to
        // 1 once BEFORE the loop and is never re-raised inside it. The old nested form had
        // the first and last arms inverted - it CONTINUED when the move mode changed and
        // when DispatchMove reported "the scroll did not move" (0x400000), i.e. it spun.
        if (t->m_moveMode != kind) {
            ok = 0;
        } else if ((flags & 0x10000) != 0) {
            ok = 0;
        } else if (t->m_screenX == goalX && t->m_screenY == arg2) {
            ok = 0;
        } else if ((flags & 0x400000) != 0) {
            ok = 0;
        }
    } while (ok != 0);
    return ok;
}

RVA(0x0015dde0, 0x5c)
CDDrawWorkerHost* CGameLevel::FindPlaneByName(const char* name) {
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        CDDrawWorkerHost* p =
            (i >= 0 && i < m_planes.GetSize()) ? static_cast<CDDrawWorkerHost*>(m_planes[i]) : 0;
        if (_strcmpi(name, p->m_name) == 0) {
            return static_cast<CDDrawWorkerHost*>(p);
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// VisitVisible: z-ordered object render walk. When the level is origin-fixed
// (m_flags & 1) walk ctx's object chain, Draw each object whose z-key is below the
// running plane's z bound, Sync the planes around it; otherwise Sync every plane
// (around the main index) and dispatch ctx's Hook. `visitor` (1st param) is the
// render visitor every Sync/Draw/Hook receives; `ctx` (2nd param) is the chain.
//
RVA(0x0015dc90, 0x141)
void CGameLevel::VisitVisible(void* visitor, CDDrawChildGroup* ctx) {
    // The engine lea's the +0x10 list record's ADDRESS and null-checks it (always
    // live) before loading the head - the CObList member keeps that byte shape.
    CObList* chain = &ctx->m_list;

    if ((m_flags & 1) && chain != 0 && (m_planes.GetSize() > 0 ? m_planes.GetData()[0] : 0) != 0) {
        (static_cast<CDDrawWorkerHost*>((m_planes.GetSize() > 0 ? m_planes.GetData()[0] : 0)))
            ->Draw(static_cast<CPlaneDrawCtx*>(visitor));
        POSITION pos = chain->GetHeadPosition();

        i32 i = 1;
        if (m_planes.GetSize() > i) {
            do {
                CDDrawWorkerHost* p = (i >= 0 && i < m_planes.GetSize())
                                          ? static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i])
                                          : 0;
                i32 zBound = p->m_zBound;
                i32 blocked = 0;
                while (pos != 0 && blocked == 0) {
                    POSITION cur = pos;
                    CGameObject* pl = static_cast<CGameObject*>(chain->GetNext(pos));
                    if (pl->m_sortKey < zBound) { // z-key vs the plane's z bound
                        pl->Render(static_cast<CDDrawSurfacePair*>(visitor));
                    } else {
                        pos = cur;
                        blocked = 1;
                    }
                }
                // retail range-checks this second GetData too (`test ebp,ebp / jl`
                // + `cmp ebp,[edi+0x3c] / jge`) - the same GetAt bound as above
                (i >= 0 && i < m_planes.GetSize()
                     ? static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i])
                     : 0)
                    ->Draw(static_cast<CPlaneDrawCtx*>(visitor));
                ++i;
            } while (i < m_planes.GetSize());
        }

        while (pos != 0) {
            static_cast<CGameObject*>(chain->GetNext(pos))
                ->Render(static_cast<CDDrawSurfacePair*>(visitor));
        }
        return;
    }

    // --- not origin-fixed: Sync planes around the main index + the ctx hook ---
    i32 idx = 0;
    if (m_mainIndex >= 0) {
        do {
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[idx]))
                ->Draw(static_cast<CPlaneDrawCtx*>(visitor));
            ++idx;
        } while (idx <= m_mainIndex);
    }
    ctx->WalkDispatch2C(static_cast<CDDrawSurfacePair*>(visitor));
    i32 j = m_mainIndex + 1;
    if (j < m_planes.GetSize()) {
        do {
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[j]))
                ->Draw(static_cast<CPlaneDrawCtx*>(visitor));
            ++j;
        } while (j < m_planes.GetSize());
    }
}

RVA(0x00160f40, 0x23)
void CGameLevel::NotifyAllPlanes() {
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        (static_cast<CDDrawWorkerHost*>(m_planes[i]))->ResolveColorKey(); // 0x163670
    }
}

// ---------------------------------------------------------------------------
// EditDispatch: case 4 pushes this level's name into the sink; case 7 pulls a name
// from the sink into this level; then (if a main plane exists) resolve the name to
// a tile id and return 1/0. ret 0x10.
//
// @early-stop
// 92.55% (was 49.78 - the `default: goto tail;` below forces retail's dense 3..8 jump
// table; the "density wall" was cl folding the empty arms into the default before the
// decision, see docs/patterns/empty-switch-arms-fold-into-default-and-kill-the-jump-
// table.md). Every code byte through the `jmp [eax*4+tbl]` now matches (llvm-objdump -dr,
// base vs target: identical offsets, the `ja` lands on the same +0xb3). The residue is the
// delinker's jump-table SYMBOL naming - base carries the table under its own `$L…`/`$tail$`
// symbols where the delinked target packs it inside the function symbol
// (jumptable-data-overlap). The tail ResolveLevelName arg list was a real bug - it passed
// arg2 twice, dropping arg1; retail pushes arg1/arg2/arg3 (fixed earlier).
RVA(0x00160f70, 0xfa)
i32 CGameLevel::EditDispatch(void* sink, i32 arg1, i32 arg2, i32 arg3) {
    EditSink* s = static_cast<EditSink*>(sink);
    if (s == 0) {
        return 0;
    }

    char buf[0x80];
    // The empty 3/5/6/8 arms only reach the jump-table decision if `default` has a DISTINCT
    // target - otherwise cl folds them into it and drops to a cmp/je chain (that is what
    // made this function read as a "jump-table-density wall"). `default: goto tail;` is
    // exactly distinct enough: cl keeps all six table entries AND still lands them on the
    // single shared tail, giving retail's `add eax,-3 / cmp eax,5 / ja tail /
    // jmp [eax*4+tbl]` with the 3/5/6/8 entries pointing at that tail. Spelling the tail
    // out under `default` instead also builds the table but leaves a 50-byte duplicate.
    switch (arg1) {
        case 3:
            break;
        case 4:
            memset(buf, 0, sizeof(buf));
            strcpy(buf, m_levelName);
            s->Write(buf, 0x80);
            break;
        case 5:
            break;
        case 6:
            break;
        case 7:
            s->Read(buf, 0x80);
            strcpy(m_levelName, buf);
            break;
        case 8:
            break;
        default:
            goto tail;
    }

tail:
    if (m_mainPlane == 0) {
        return 0;
    }
    return m_mainPlane->SerializeDispatch(s, arg1, arg2, arg3) != 0 ? 1 : 0;
}

RVA(0x001610a0, 0x70)
i32 CGameLevel::SaveName(void* sink) {
    EditSink* s = static_cast<EditSink*>(sink);
    if (s == 0) {
        return 0;
    }

    char buf[0x80];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, m_levelName);
    s->Write(buf, 0x80);
    return 1;
}

RVA(0x00161110, 0x64)
i32 CGameLevel::LoadName(void* sink) {
    EditSink* s = static_cast<EditSink*>(sink);
    if (s == 0) {
        return 0;
    }

    char buf[0x80];
    s->Read(buf, 0x80);
    strcpy(m_levelName, buf);
    return 1;
}

// ---------------------------------------------------------------------------
// DispatchMove: when this->flags & 4, tail into ApplyMove on `target`; else run
// `target`'s +0xe4 brush-kind switch (kinds 1..8) and fold the flag bits into the
// returned state word, tagging 0x400000 when the scroll did not move.
//
// @early-stop
// call-arg-materialization entropy (~79%): the dense kinds-1..8 jump table, every
// case body, the flag-folding tail and the __stdcall sub-handler convention are
// exact; the residue is the recurring call setup (forward-to-ApplyMove + 5
// MoveHandler sites) where retail interleaves an arg reload between pushes (2 regs)
// and MSVC pre-loads (3 regs). docs/patterns/pin-local-for-callee-saved-reg.md.
// Logic/offsets/CFG exact; deferred to the final sweep.
RVA(0x0015dfb0, 0x15b)
i32 CGameLevel::DispatchMove(CGameObject* target, i32 a1, i32 a2, i32 a3) {
    if (m_flags & 4) {
        return ApplyMove(target, a1, a2, a3);
    }

    CGameObject* s = target;
    i32 eax = 0;
    i32 kind = s->m_moveMode;
    i32 prevX = s->m_screenX;
    i32 prevY = s->m_screenY;

    switch (kind) {
        case 1:
        case 2:
        case 5:
            eax = MoveHandlerA(s, a1, a2, a3);
            break;
        case 3:
            eax = MoveHandlerB(s, a1, a2, a3);
            if (s->m_moveMode == 4) {
                eax |= 0x800000;
            }
            break;
        case 4:
            eax = MoveHandlerC(s, a1, a2, a3);
            if (s->m_moveMode == 1) {
                eax |= 0x1000000;
            }
            break;
        case 8:
            if (a2 < prevY) {
                eax = MoveHandlerB(s, a1, a2, a3);
                if (s->m_moveMode == 4) {
                    eax |= 0x800000;
                    s->m_moveMode = 8;
                }
            } else {
                eax = MoveHandlerC(s, a1, a2, a3);
                if (s->m_moveMode == 1) {
                    eax |= 0x1000000;
                }
            }
            break;
        case 6:
            eax = MoveHandlerD(s, a1, a2, a3);
            break;
        case 7:
            s->m_screenX = a1;
            s->m_screenY = a2;
            break;
    }

    if (eax & 0x1820000) {
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
// MoveHandlerA (kinds 1/2/5): axis-1 step toward a1, axis-2 advance (AdvanceA),
// then - per the arg3 low(bit0)/high(bit1) selector - an AxisProbe against the
// matching +0x138/+0x140 limit and, when that blocks (==3), a ClampSpan re-bracket
// whose [lo,hi] midpoint replaces the new coord (gated by the arg3 0x10 bit). The
// no-block tail drives Hold/FreeMove off the target's +0x10 held flag.
//
// @early-stop
// 64.96 -> 70.04 (measured 2026-07-27). The old note called this a pure
// "register-scheduling wall"; it was a CONTROL-FLOW bug the exit-count screen exposed
// (base 4 rets / retail 1). Two real corrections, both byte-evidenced:
// The held-flag tail is NOT an `else` arm: retail 0x15e207/0x15e276 jump the
// probe-MISS of both bit0 and bit1 arms straight into it (0x15e5a7), so it runs
// whenever no hard tile blocked - only a hard tile (m_moveMode = 6) skips it.
// Both arms then converge on ONE bracket-commit block (0x15e58f) that re-tests
// the cached arg3 0x10 bit, which is why `mid` is a separate local (the sibling
// MoveHandlerB already spelled it that way).
// 71.47 -> 76.85: the two arms now `goto` ONE shared bracket block (retail 0x15e283),
// carrying `mid` and the CACHED arg3 0x10 bit in function-scope locals - retail spills
// that bit to [esp+0x1c] and re-tests it at the shared block, which is only expressible
// with one `bracket` local, not two per-arm `if (a3 & 0x10)` copies.
// @early-stop
// residual: base 2 rets vs retail 1 (cl still inlines the commit into the bracket path)
// and a whole-function esi<->edi role swap for this-vs-t. Block-order variants
// (rebracket before the held tail, an explicit `goto commit`) are byte-identical.
RVA(0x0015e130, 0x1bb)
i32 CGameLevel::MoveHandlerA(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 result = 0;

    if (a1 > t->m_screenX) {
        result = StepAxisLo(t, a1, a2, &a1, a3);
    } else if (a1 < t->m_screenX) {
        result = StepAxisHi(t, a1, a2, &a1, a3);
    }

    if (a2 < t->m_screenY) {
        a2 = AdvanceA(t, a1, a2, a3);
    }

    i32 mid;
    i32 bracket;
    if (a3 & 1) {
        i32 limit = t->m_extent.top + a2 - 1;
        if (AxisProbe(a1, limit) == kTileHard) {
            mid = a1;
            bracket = a3 & 0x10;
            if (bracket != 0) {
                i32 lo = a1;
                i32 hi = a1;
                if (ClampSpan(a1, limit, &lo, &hi) != 0) {
                    mid = (hi + lo) / 2;
                }
            }
            goto rebracket;
        }
    } else if (a3 & 2) {
        i32 limit = t->m_extent.bottom + a2 + 2;
        if (AxisProbe(a1, limit) == kTileHard) {
            mid = a1;
            bracket = a3 & 0x10;
            if (bracket != 0) {
                i32 lo = a1;
                i32 hi = a1;
                if (ClampSpan(a1, limit, &lo, &hi) != 0) {
                    mid = (hi + lo) / 2;
                }
            }
            goto rebracket;
        }
    }

    if (t->m_flags & 0x10) {
        if (HoldMove(t, t->m_carrier, a1, a2, a3) == 0) {
            t->m_moveMode = 4;
        }
    } else {
        a2 = FreeMove(t, a1, a2, a3);
    }
    goto commit;

rebracket:
    if (bracket != 0) {
        a1 = mid;
    }
    t->m_moveMode = 6;

commit:
    t->m_screenX = a1;
    t->m_screenY = a2;
    return result;
}

// ---------------------------------------------------------------------------
// MoveHandlerC (kind 4): axis-1 step, an alternate axis-2 step (StepAxisAlt gated by
// arg3 bit3), an AdvanceB advance (unless the kind already turned 1), the same low
// AxisProbe + ClampSpan re-bracket as MoveHandlerA, then - if the kind ended at 1 and
// the coord moved with the 0x20000 state bit set - one blocked-move retry (clear the
// 0xe0000 bits, re-step the axis).
//
// @early-stop
// register-scheduling wall: same 4-saved-reg / multi-dispatch + spilled-bracket
// scheduling as MoveHandlerA, plus the retry tail; logic + offsets + CFG + sibling
// conventions exact. Deferred to the final sweep.
RVA(0x0015e2f0, 0x1b7)
i32 CGameLevel::MoveHandlerC(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 savedA1 = a1;
    i32 result = 0;

    if (a1 > t->m_screenX) {
        result = StepAxisLo(t, a1, a2, &a1, a3);
    } else if (a1 < t->m_screenX) {
        result = StepAxisHi(t, a1, a2, &a1, a3);
    }

    if (a3 & 8) {
        i32 outY = a2;
        if (StepAxisAlt(t, a1, a2, &outY, a3) != 0) {
            a2 = outY;
        }
    }

    if (t->m_moveMode != 1) {
        a2 = AdvanceB(t, a1, a2, a3);
    }

    // same shape as the MoveHandlerA sibling: ONE `mid` local + the cached arg3 0x10
    // bit, so the bracket commit is a single block instead of two arms each with their
    // own `m_moveMode = 6` (80.25 -> 81.78).
    if (a3 & 1) {
        i32 limit = t->m_extent.top + a2 - 1;
        if (AxisProbe(a1, limit) == kTileHard) {
            i32 mid = a1;
            i32 bracket = a3 & 0x10;
            if (bracket != 0) {
                i32 lo = a1;
                i32 hi = a1;
                if (ClampSpan(a1, limit, &lo, &hi) != 0) {
                    mid = (hi + lo) / 2;
                }
            }
            if (bracket != 0) {
                a1 = mid;
            }
            t->m_moveMode = 6;
        }
    }

    if (t->m_moveMode == 1 && a1 != savedA1) {
        if (result & 0x20000) {
            result &= 0xfff1ffff;
            if (a1 > t->m_screenX) {
                result |= StepAxisLo(t, a1, a2, &a1, a3);
            } else if (a1 < t->m_screenX) {
                result |= StepAxisHi(t, a1, a2, &a1, a3);
            }
        }
    }

    t->m_screenX = a1;
    t->m_screenY = a2;
    return result;
}

// ---------------------------------------------------------------------------
// MoveHandlerB (kind 3, also kind 8 down-moves): axis-1 step, axis-2 advance
// (AdvanceA, unconditional), the low AxisProbe + ClampSpan re-bracket, then commit.
//
// @early-stop
// The stepped coord and the advanced cursor are written back through the a1/a2
// PARAMETER slots (retail's `lea ecx,[esp+0x20]` IS &a1, with no init copy - the
// local-variable spelling emitted a redundant `mov [a1-home],reg`). That fixed the
// whole prologue + the StepAxis dispatch. Residue is the tail: cl duplicates the
// commit epilogue for the `a3 & 0x10 == 0` arm where retail shares one, and keeps a2
// in ebp across instead of reloading it. Deferred to the final sweep.
RVA(0x0015e4b0, 0xf7)
i32 CGameLevel::MoveHandlerB(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 result = 0;

    // the stepped coord and the advanced cursor are written back through the a1/a2
    // parameter slots (retail's `lea ecx,[esp+0x20]` IS &a1; there is no copy).
    if (a1 > t->m_screenX) {
        result = StepAxisLo(t, a1, a2, &a1, a3);
    } else if (a1 < t->m_screenX) {
        result = StepAxisHi(t, a1, a2, &a1, a3);
    }

    a2 = AdvanceA(t, a1, a2, a3);

    if (a3 & 1) {
        i32 limit = t->m_extent.top + a2 - 1;
        if (AxisProbe(a1, limit) == kTileHard) {
            i32 mid = a1;
            if (a3 & 0x10) {
                i32 lo = a1;
                i32 hi = a1;
                if (ClampSpan(a1, limit, &lo, &hi) != 0) {
                    mid = (hi + lo) / 2;
                }
            }
            if (a3 & 0x10) {
                a1 = mid;
            }
            t->m_moveMode = 6;
        }
    }

    t->m_screenX = a1;
    t->m_screenY = a2;
    return result;
}

// ---------------------------------------------------------------------------
// MoveHandlerD (kind 6): drives the axis-2 advance first (AdvanceB on a down-move,
// else AdvanceA), runs the +0x138/+0x140 two-probe (low then high limit, blocking on
// ==3), and on the up-move path a SpanCheck validate that may clamp the cursor below
// the +0x140 high limit; then a final axis-1 step and commit.
//
// The argument list was ROTATED (83.0% -> 96.3%): a1 is the X target and a2 the Y
// target, exactly like the MoveHandlerA/B/C siblings - the branch tests screenY against
// a2, both advances take the handler's own (t, a1, a2, a3), AxisProbe/SpanCheck/StepAxis
// all take a1, and SpanCheck's arg list is (col, want, top, &out) not (want, top, col,
// &out). The span floor is also DERIVED from the recomputed head row.
//
// @early-stop
// residue is one redundant `coord = a1` copy (cl re-emits it where retail coalesces the
// local onto a1's dead home) and the ebx/edi colour of this-vs-cursor that follows it.
// Writing through &a1 kills the copy but splits retail's shared argument push block
// (107 differing rows vs 47). Deferred to the final sweep.
RVA(0x0015e5b0, 0x162)
i32 CGameLevel::MoveHandlerD(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 result = 0;
    i32 cursor;
    i32 coord = a1;

    // a1 is the X (column) target and a2 the Y (row) target - the same convention as the
    // MoveHandlerA/B/C siblings; both advances take the handler's own (t, a1, a2, a3).
    if (t->m_screenY < a2) {
        cursor = AdvanceB(t, a1, a2, a3);
        if (t->m_moveMode != 1) {
            i32 hi = t->m_extent.bottom + cursor + 1;
            i32 lo = t->m_extent.top + cursor - 1;
            if (AxisProbe(a1, lo) != kTileHard && AxisProbe(a1, hi) != kTileHard) {
                t->m_moveMode = 4;
            }
        }
    } else {
        cursor = AdvanceA(t, a1, a2, a3);
        i32 hi = t->m_extent.bottom + cursor + 1;
        i32 lo = t->m_extent.top + cursor - 1;
        if (AxisProbe(a1, lo) != kTileHard && AxisProbe(a1, hi) != kTileHard) {
            // the AxisProbe calls invalidate the cached extent, so the span top is
            // recomputed - and the span BOTTOM is derived from it (retail's
            // `lea top; sub cursor; add screenY`), not folded to bottom+1+screenY.
            i32 probe;
            i32 top = t->m_extent.bottom + cursor + 1;
            if (SpanCheck(a1, top - cursor + t->m_screenY, top, &probe) != 0 && probe > cursor) {
                t->m_moveMode = 1;
                cursor = probe - t->m_extent.bottom - 1;
            }
        }
    }

    // the axis-1 step writes back through the a1 parameter slot itself (retail never
    // copies it into a second local - the compare and the commit both read the slot).
    if (coord > t->m_screenX) {
        result = StepAxisLo(t, coord, cursor, &coord, a3);
    } else if (coord < t->m_screenX) {
        result = StepAxisHi(t, coord, cursor, &coord, a3);
    }

    t->m_screenX = coord;
    t->m_screenY = cursor;
    return result;
}

// ---------------------------------------------------------------------------
// StepAxisLo (@0x15e720): step the target one axis from its low bracket toward the
// requested mid coord, probing the main plane's tiles along the way. The probe walks
// `cur` from (axisLoB + a2) up to (axisHi + a2) in +0xfc strides; the first cur whose
// tile probe returns 1 commits the target's current scroll into *outX and returns
// 0x60000, else *outX = a1 and 0. The tile probe is AxisProbe inlined (PROBE_TILE).
//
RVA(0x0015e720, 0x14c)
i32 CGameLevel::StepAxisLo(CGameObject* t, i32 a1, i32 a2, i32* outX, i32 a3) {
    i32 mid = t->m_extent.right + a1;
    i32 lo = t->m_extent.top + a2;
    i32 hi = t->m_extent.bottom + a2;
    i32 cur = lo;

    while (cur <= hi) {
        i32 result;
        PROBE_TILE(this, mid, cur, result);
        if (result == kTileSoft) {
            *outX = t->m_screenX;
            return 0x60000;
        }
        if (cur == hi) {
            ++cur;
        } else {
            cur += t->m_strideY;
            if (cur > hi) {
                cur = hi;
            }
        }
    }

    *outX = a1;
    return 0;
}

// ---------------------------------------------------------------------------
// StepAxisHi (@0x15e870): the mirror of StepAxisLo using the +0x134 high bracket
// (axisLoA) as the loop floor and returning 0xa0000 on a successful step.
//
RVA(0x0015e870, 0x14c)
i32 CGameLevel::StepAxisHi(CGameObject* t, i32 a1, i32 a2, i32* outX, i32 a3) {
    i32 mid = t->m_extent.left + a1;
    i32 lo = t->m_extent.top + a2;
    i32 hi = t->m_extent.bottom + a2;
    i32 cur = lo;

    while (cur <= hi) {
        i32 result;
        PROBE_TILE(this, mid, cur, result);
        if (result == kTileSoft) {
            *outX = t->m_screenX;
            return 0xa0000;
        }
        if (cur == hi) {
            ++cur;
        } else {
            cur += t->m_strideY;
            if (cur > hi) {
                cur = hi;
            }
        }
    }

    *outX = a1;
    return 0;
}

// ---------------------------------------------------------------------------
// FreeMove (@0x15eb00): drive a free (unheld) axis-2 advance from (axisLoB + a1) up to
// (axisHi + a2 + 1) in +0xf8 strides. Per cursor it probes the main plane (PROBE_TILE);
// a kind 1/2 tile re-probes a one-step-back pair, a kind-3 tile re-probes its low pair
// (AxisProbe), and anything that stays blocked tags the target brush kind 4. Returns
// the last accepted cursor.
//
// @early-stop
// register-scheduling wall (large /O2 body): four inlined PROBE_TILE copies + two
// AxisProbe re-probe sites + the strided loop pin 5 saved regs in an order MSVC
// reproduces only for one spill order; logic + offsets + CFG + the probe/dispatch
// conventions are exact. Deferred to the final sweep.
RVA(0x0015eb00, 0x2d2)
i32 CGameLevel::FreeMove(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 mid = t->m_extent.right + a1;
    i32 cur = t->m_extent.left + a1;
    i32 hiY = t->m_extent.bottom + a2 + 1;

    if (cur <= mid) {
        do {
            i32 result;
            PROBE_TILE(this, cur, hiY, result);
            if (result == kTileSoft || result == kTileSoft2) {
                // kind 1/2: re-probe the one-step-back pair twice to confirm the
                // move still fits (retail outlines the 2nd index lookup to a leaf).
                i32 r2;
                PROBE_TILE(this, cur, hiY - 1, r2);
                if (r2 != kTileSoft) {
                    i32 r3;
                    PROBE_TILE(this, cur, hiY - 1, r3);
                    if (r3 != kTileSoft2) {
                        return a2;
                    }
                }
            } else if (t->m_moveMode != 6 && result == kTileHard) {
                if (AxisProbe(cur, hiY) == kTileHard) {
                    if (AxisProbe(cur, hiY - 1) != kTileHard) {
                        return a2;
                    }
                }
            }
            if (cur == mid) {
                ++cur;
            } else {
                cur += t->m_strideX;
            }
        } while (cur <= mid);
    }

    t->m_moveMode = 4;
    return a2;
}

// ---------------------------------------------------------------------------
// AdvanceB (@0x15ede0): advance the axis-2 cursor variant. Probes the start coord,
// folds a kind-4 result into the target +0x8 flags (0x400000), then walks the cursor
// down through AxisProbe gates; a clear low/high pair commits brush kind 1 and the
// adjusted cursor. Returns the resolved cursor.
//
// The walk-off path returns a2 (retail `mov eax,[esp+0x30]`), not a3 - same correction
// as the AdvanceA sibling.
// @early-stop
// tail-merge wall: retail keeps THREE exits, cl folds two into one because both inner
// loops' returns are the same value - `(y+1) - bottom - 1` and `y - bottom` - and cl
// reassociates the first into the second. Four spellings of the `(y+1) - bottom - 1`
// expression (a `row` local, explicit parens, `bottom + 1`, a split statement) all fold.
RVA(0x0015ede0, 0x2a7)
i32 CGameLevel::AdvanceB(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 lo = t->m_extent.left + a1;
    i32 mid = t->m_extent.right + a1;
    i32 hiY = a2 + t->m_extent.bottom + 1;

    i32 first;
    PROBE_TILE(this, a1, hiY, first);
    if (first == kTileSpecial) {
        t->m_flags |= 0x400000;
    }
    i32 base = a2 - t->m_screenY;

    i32 cur = lo;
    if (cur <= mid) {
        do {
            i32 result;
            PROBE_TILE(this, cur, hiY, result);
            if (result == kTileSoft || result == kTileSoft2) {
                i32 floor = t->m_screenY + t->m_extent.bottom;
                if (hiY >= floor) {
                    i32 y = hiY;
                    do {
                        i32 g = AxisProbe(cur, y);
                        if (g != kTileSoft && g != kTileSoft2) {
                            t->m_moveMode = 1;
                            return y - t->m_extent.bottom;
                        }
                        --y;
                    } while (y >= floor);
                }
            } else if (t->m_moveMode != 6 && result == kTileHard) {
                i32 floor = hiY - base;
                if (hiY > floor) {
                    i32 y = hiY - 1;
                    if (y >= floor) {
                        do {
                            if (AxisProbe(cur, y) != kTileHard) {
                                t->m_moveMode = 1;
                                return (y + 1) - t->m_extent.bottom - 1;
                            }
                            --y;
                        } while (y >= floor);
                    }
                }
            }
            if (cur == mid) {
                ++cur;
            } else {
                cur += t->m_strideX;
            }
        } while (cur <= mid);
    }

    // the walk-off path returns the INCOMING axis-2 cursor (a2), not a3: retail's tail
    // is `mov eax,[esp+0x30]` = a2's home. Same correction as the AdvanceA sibling.
    return a2;
}

// ---------------------------------------------------------------------------
// AdvanceA (@0x15f1c0): advance the axis-2 cursor. Probes (cur, hi) across the
// strided walk; a hit gates an AxisProbe sweep up the +0x138 column, and on the
// AxisProbe miss commits brush kind 4 and returns the adjusted cursor.
//
// 97.25 -> 97.40: the miss path returns a2, not a3 (see the tail). That was the
// missing 5th local - retail's `sub esp,0x14`; with a3 returned, a2 was dead after
// `ceil` and cl reused a2's parameter home for it, coming out 4 bytes short.
// @early-stop
// residual: 4 rows in the hit-path epilogue - retail loads `t` into eax and subtracts
// in place (`sub esi,ecx / mov eax,esi`), cl loads it into ecx and moves the cursor to
// eax first. Four spellings of `y - t->m_extent.top` are byte-identical.
RVA(0x0015f1c0, 0x171)
i32 CGameLevel::AdvanceA(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 startCol = t->m_extent.left + a1;
    i32 mid = t->m_extent.right + a1;
    i32 ceil = a2 + t->m_extent.top - 1;
    i32 cur = startCol;

    if (cur <= mid) {
        do {
            i32 result;
            PROBE_TILE(this, cur, ceil, result);
            if (result == kTileSoft) {
                i32 floor = t->m_screenY + t->m_extent.top - 1;
                if (ceil <= floor) {
                    i32 y = ceil;
                    do {
                        // retail probes the SPAN'S START column here, not the walking
                        // cursor: the pushed value comes from the never-updated frame
                        // slot the prologue seeds (the loop cursor lives in a1's home).
                        if (AxisProbe(startCol, y) != kTileSoft) {
                            t->m_moveMode = 4;
                            return y - t->m_extent.top;
                        }
                        ++y;
                    } while (y <= floor);
                }
            }
            if (cur == mid) {
                ++cur;
            } else {
                cur += t->m_strideX;
            }
        } while (cur <= mid);
    }

    // the miss path returns the INCOMING axis-2 cursor (a2), not a3: retail's tail is
    // `mov eax,[esp+0x30]` = a2's home, and a3 is never read at all. Returning a3 also
    // cost the 5th local - with a2 dead after `ceil`, cl reused a2's parameter home for
    // `ceil` and the frame came out 4 bytes short of retail's `sub esp,0x14`.
    return a2;
}

// ---------------------------------------------------------------------------
// SpanCheck (@0x15f8d0): validate that the probe column from (b-1) down to c fits -
// the first cur whose tile probe is not "blocked" (!= 3) commits cur+1 into *out and
// returns 1; an empty or all-blocked span returns 0.
//
RVA(0x0015f8d0, 0x113)
i32 CGameLevel::SpanCheck(i32 a, i32 b, i32 c, i32* out) {
    if (b <= c) {
        return 0;
    }
    i32 cur = b - 1;
    while (cur >= c) {
        i32 result;
        PROBE_TILE(this, a, cur, result);
        if (result != kTileHard) {
            *out = cur + 1;
            return 1;
        }
        --cur;
    }

    return 0;
}

RVA(0x0015fdb0, 0x8a)
i32 CGameLevel::StepAxisAlt(CGameObject* t, i32 a1, i32 a2, i32* outY, i32 a3) {
    if ((a3 & 8) == 0) {
        return 0;
    }

    CObList& chain = OwnerMgr()->m_childGroup->m_list;
    POSITION pos = chain.GetHeadPosition();
    while (pos != 0) {
        CGameObject* pl = static_cast<CGameObject*>(chain.GetNext(pos));
        if (pl->m_collCategory == 0x80) {
            if (AltStepValidate(t, pl, a1, a2, outY, a3) != 0) {
                t->m_moveMode = 1;
                t->m_carrier = pl;
                t->m_flags |= 0x10;
                return 1;
            }
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// AltStepValidate (@0x15fe40): the geometric fit test StepAxisAlt runs per candidate
// object. Both the candidate payload (+0x144/+0x148/+0x14c/+0x178 extents, +0x5c/+0x60
// origin) and the target (+0x134/+0x13c/+0x140 brackets, +0x5c/+0x60 scroll) must have
// valid (!= -1) low brackets, the target's scroll must sit below a2, and the candidate's
// world-space box (offset by a1/a2) must fall inside the target's bracket box; on a fit
// it writes the resolved cursor (payload->+0x60 + payload->+0x148 - target->+0x140 - 1)
// into *outY and returns 1, else 0.
//
// The fail exits are written as one `goto fail` epilogue: MSVC tail-merges every
// `return 0` into the single retail epilogue (the per-`return 0` inline form scored
// 0% on a fully-divergent layout; the merged form is 71.9%).
//
// cmpHi is DERIVED from tHi (`tHi - a2 + sy`, retail's `mov ecx,ebx; sub ecx,ebp; add
// ecx,ebp`), not recomputed as `bottom + sy` - that one line took the whole body to
// instruction-for-instruction identity.
//
// @early-stop
// 89.5%: residue is which dead PARAMETER HOME each spilled local lands in - retail packs
// sy/bottom/tLoA/boxL onto t/p/a1/a2's homes, ours rotates them by one - plus the
// else-arm block PLACEMENT (retail keeps it inline with a `jmp` over the commit block;
// cl sinks it past the commit and inverts its branch). Every declaration permutation
// tried is worse. Deferred to the final sweep.
RVA(0x0015fe40, 0xd4)
i32 CGameLevel::AltStepValidate(CGameObject* t, CGameObject* p, i32 a1, i32 a2, i32* outY, i32 a3) {

    if (p->m_area.left == -1) {
        goto fail;
    }
    if (t->m_extent.left == -1) {
        goto fail;
    }
    {
        i32 sy = t->m_screenY;
        if (sy > a2) {
            goto fail;
        }

        i32 boxL = p->m_area.left + p->m_screenX;
        i32 boxR = p->m_area.right + p->m_screenX;
        i32 boxT = p->m_screenY + p->m_area.top;
        i32 tLoA = t->m_extent.left + a1;
        i32 tMid = t->m_extent.right + a1;
        i32 bottom = t->m_extent.bottom;
        i32 tHi = bottom + a2;
        i32 cmpHi = tHi - a2 + sy;

        i32 over = p->m_deltaY;
        if (over > 0) {
            over = 0;
        }
        i32 ceil = boxT - over;
        if (cmpHi > ceil) {
            goto fail;
        }
        if (tMid < boxL) {
            goto fail;
        }
        if (tLoA > boxR) {
            goto fail;
        }
        // `boxT - 1` is ONE local shared by both arms: retail hoists `lea eax,[edx-1]`
        // above the cmpHi/tHi branch and both arms then `cmp tHi,stand` in that operand
        // order (jne on the equal arm, jl on the other). 85.36 -> 89.49.
        i32 stand = boxT - 1;
        if (cmpHi == tHi) {
            if (tHi != stand) {
                goto fail;
            }
        } else {
            if (tHi < stand) {
                goto fail;
            }
        }

        *outY = boxT - bottom - 1;
        return 1;
    }
fail:
    return 0;
}

// ---------------------------------------------------------------------------
// HoldMove (@0x15ff20): the ride-check MoveHandlerA/C run while the object is
// latched to a carrier (p = et->m_carrier). Gated like AltStepValidate (a3 bit3
// set, carrier category 0x80, both extents valid), it checks the rider's extent
// box (offset by a1) still overlaps the carrier's stand area and returns whether
// the rider's feet (m_extent.bottom + a2) still sit exactly on the stand surface
// (m_area.top-derived row - 1). All field reads; no calls. ret 0x14.
// @early-stop
// 99.98%: ONE pair - retail loads p->m_area.top (+0x148) before p->m_screenY (+0x60), we
// do the reverse; the `add edx,ebx` that consumes them is byte-identical either way.
// docs/patterns/two-member-add-load-order-is-canonicalized.md.
RVA(0x0015ff20, 0xc0)
i32 CGameLevel::HoldMove(CGameObject* et, CGameObject* p, i32 a1, i32 a2, i32 a3) {
    if (p == 0) {
        return 0;
    }
    if ((a3 & 8) == 0) {
        return 0;
    }
    if (p->m_collCategory != 0x80) {
        return 0;
    }
    if (p->m_area.left == -1) {
        return 0;
    }
    if (et->m_extent.left == -1) {
        return 0;
    }

    i32 ox = p->m_screenX;
    i32 boxL = ox + p->m_area.left;
    i32 boxR = ox + p->m_area.right;
    i32 boxT = p->m_screenY + p->m_area.top;
    i32 tMid = et->m_extent.right + a1;
    i32 tLoA = et->m_extent.left + a1;
    // the foot row is computed LAST, right before its compare: that is what keeps the
    // carrier-area temp (m_area.top) in ebx and lets a2 stay a memory operand.
    i32 hi = et->m_extent.bottom + a2;
    if (tMid < boxL) {
        return 0;
    }
    if (tLoA > boxR) {
        return 0;
    }
    return hi == boxT - 1;
}

RVA(0x0015ffe0, 0x99)
i32 CGameLevel::ClampSpan(i32 x, i32 y, i32* outLo, i32* outHi) {
    if (x < 0) {
        x = 0;
    } else {
        if (x >= m_mainPlane->m_wrapW) {
            x = m_mainPlane->m_wrapW - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        if (y >= m_mainPlane->m_wrapH) {
            y = m_mainPlane->m_wrapH - 1;
        }
    }
    CDDrawWorkerHost* pl = m_mainPlane;
    i32 qx = x >> pl->m_shiftX;
    i32 alignedX = qx << pl->m_shiftX;
    i32 qy = y >> pl->m_shiftY;
    i32 idx = pl->m_colOffsets[qy] + qx;
    i32 tile = pl->m_tileGrid[idx];
    if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
        return 0;
    }
    CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
    *outLo = alignedX;
    *outHi = alignedX + set->m_width - 1;
    return 1;
}

// ---------------------------------------------------------------------------
// ProbeHeadSoft (@0x160450): probe the tile straight above the object at
// (m_screenX, m_screenY + m_extent.top + dy) and return whether it is soft-blocking
// (== kTileSoft). The inlined PROBE_TILE shape; the result==1 test is shared by
// both the tile-hit and empty-tile paths (retail merges the sete). ret 8.
//
// @early-stop
// 99.98%: ONE pair - retail loads m_screenY (+0x60) before m_extent.top (+0x138), we do
// the reverse; the `add eax,ebx` that consumes them is byte-identical either way. Same
// canonicalization as ProbeFootSoft/ProbeFootBlocked/HoldMove;
// docs/patterns/two-member-add-load-order-is-canonicalized.md.
RVA(0x00160450, 0xd6)
i32 CGameLevel::ProbeHeadSoft(CGameObject* t, i32 dy) {
    i32 px = t->m_screenX;
    i32 py = t->m_screenY + t->m_extent.top + dy;
    i32 result;
    PROBE_TILE(this, px, py, result);
    return result == kTileSoft;
}

RVA(0x00160530, 0x125)
i32 CGameLevel::IsValidWwd(const char* name, void* headerBuf) {
    if (name == 0) {
        return 0;
    }
    if (headerBuf == 0) {
        return 0;
    }

    CFile stream; // the WWD file stream IS a CFile (ctor/Open/Read/dtor @0x1befd7.. NAFXCW)

    if (stream.Open(name, 0, 0) == 0) { // Open returns 0 on failure
        return 0;
    }

    if (stream.Read(headerBuf, 0x5f4) != 0x5f4) {
        return 0;
    }

    if (*static_cast<u32*>(headerBuf) > 0x5f4) { // signature must be <= 1524
        return 0;
    }

    return 1;
}

RVA(0x00160660, 0x12b)
i32 CGameLevel::ReadWwdHeaderName(const char* name, void* nameOut) {
    WwdHeader header;

    if (name == 0) {
        return 0;
    }
    if (nameOut == 0) {
        return 0;
    }

    CFile stream; // the WWD file stream IS a CFile (ctor/Open/Read/dtor @0x1befd7.. NAFXCW)

    if (stream.Open(name, 0, 0) == 0) { // Open returns 0 on failure
        return 0;
    }

    if (stream.Read(&header, 0x5f4) != 0x5f4) {
        return 0;
    }

    if (header.wwdSignature > 0x5f4) { // signature must be <= 1524
        return 0;
    }

    strcpy(static_cast<char*>(nameOut), header.levelName); // inline strlen + rep movs
    return 1;
}

// ---------------------------------------------------------------------------
// WwdFile::InflateMainBlock
// Validates the header, copies the 0x5F4-byte header prefix into dest, then
// zlib-uncompresses the COMPRESS main block into the remainder. Returns dest on
// success, 0 on any validation/inflate failure.
// @early-stop
// callee-saved regalloc-coloring wall (~88.7%): body byte-identical, but MSVC5 and
// retail break the dest/destLen coloring tie oppositely - both cross the inline memcpy,
// retail pins destLen in ebp and spills dest to [esp+0x18], recompile pins dest in ebp
// and spills destLen. The register swap propagates through the whole body. Not steerable
// from C (same # uses either way; declaration/order-neutral).
RVA(0x00160790, 0xd2)
Bytef* __stdcall WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, u32 destLen) {
    uLongf outLen;

    if (src == 0) {
        return 0;
    }
    if (dest == 0) {
        return 0;
    }

    if (src->wwdSignature > 0x5f4) { // header size (== 1524)
        return 0;
    }
    if ((src->flags & 0x2) == 0) { // require COMPRESS (WwdFlags bit1)
        return 0;
    }
    if (src->mainBlockLength == 0) {
        return 0;
    }
    if (src->mainBlockLength > destLen + src->wwdSignature) {
        return 0;
    }

    memcpy(dest, src, src->wwdSignature); // copy the 1524-byte header prefix
    outLen = static_cast<uLongf>((destLen - src->wwdSignature));
    if (uncompress(
            dest + src->wwdSignature,
            &outLen,
            // the deflate stream starts at the header's OWN length field, a runtime
            // byte offset - byte-forced by the WWD format, no member can name it
            reinterpret_cast<Bytef*>(src) + src->wwdSignature,
            src->mainBlockLength
        )
        != 0) {
        return 0;
    }

    return outLen == src->mainBlockLength ? dest : 0;
}

RVA(0x00160870, 0x43)
i32 __stdcall WwdFile_CompressMainBlock(
    unsigned char* src,
    unsigned long srcLen,
    unsigned char* dest,
    unsigned long destCap
) {
    if (src == 0) {
        return 0;
    }
    if (dest == 0) {
        return 0;
    }
    unsigned long outLen = destCap;
    return WapUncompress(dest, &outLen, src, srcLen) == 0 ? static_cast<i32>(outLen) : 0;
}
// ---------------------------------------------------------------------------
// ProbeFeetKind (@0x1608c0): the feet-edge twin of ProbeColumn - probe the tile at
// (m_screenX + dx, m_extent.bottom + m_screenY) and return the image set's GetCollisionAt
// kind raw (0 for an empty/clear tile). The inlined PROBE_TILE shape. ret 8.
//
RVA(0x001608c0, 0xc0)
i32 CGameLevel::ProbeFeetKind(CGameObject* t, i32 dx) {
    i32 px = t->m_screenX + dx;
    i32 py = t->m_extent.bottom + t->m_screenY;
    i32 result;
    PROBE_TILE(this, px, py, result);
    return result;
}

// ---------------------------------------------------------------------------
// ProbeColumn (@0x160980): probe the single tile at the object's top edge
// (m_screenX + dx, m_extent.top + m_screenY), clamped into the main plane grid,
// returning the image set's GetCollisionAt (+0x20) dispatch (0 for an
// empty/clear tile). The inlined AxisProbe shape (PROBE_TILE). ret 8.
//
RVA(0x00160980, 0xc0)
i32 CGameLevel::ProbeColumn(CGameObject* t, i32 dx) {
    i32 px = t->m_screenX + dx;
    i32 py = t->m_extent.top + t->m_screenY;
    i32 result;
    PROBE_TILE(this, px, py, result);
    return result;
}

// ---------------------------------------------------------------------------
// WalkColumnDown (@0x160a40): ground snap. From the object's feet row (m_extent.bottom +
// m_screenY), probe the tile column at the fixed x (m_screenX) stepping the row
// downward until GetCollisionAt reports a stop code (1/2/3) or the row runs off
// the grid (>= plane height). On a stop, drop the object onto the ground
// (m_screenY += finalRow - startRow - 1) and return 1; unset extents / missing
// main plane / off-grid walk returns 0.
//
// @early-stop
// register-scheduling wall: the inlined PROBE_TILE + slot-+0x20 dispatch repeated
// across the down-counting walk (start probe + loop probe) pin the 4 saved regs, the
// spilled this/start-row/shiftY/wrapH in a spill order MSVC reproduces only for one
// allocation; logic + offsets + CFG + the commit arithmetic are exact. Deferred to the
// final sweep.
RVA(0x00160a40, 0x201)
i32 CGameLevel::WalkColumnDown(CGameObject* t, i32 unused) {
    if (t->m_extent.left == AXIS_UNSET) {
        return 0;
    }
    if (m_mainPlane == 0) {
        return 0;
    }

    i32 px = t->m_screenX;
    i32 row = t->m_extent.bottom + t->m_screenY;
    i32 startRow = row;

    i32 result;
    PROBE_TILE(this, px, row, result);

    while (result != kTileSoft) {
        if (result == kTileSoft2 || result == kTileHard) {
            break;
        }
        ++row;
        if (row >= (m_mainPlane)->m_wrapH) {
            return 0;
        }
        PROBE_TILE(this, px, row, result);
    }

    i32 final = row - startRow - 1;
    t->m_screenY += final;
    return 1;
}

RVA(0x0015d790, 0x8b)
i32 CGameLevel::ReadImageSets(const u32* dir, char* cursor) {
    if (cursor == 0) {
        return -1;
    }
    if (dir == 0) {
        return -1;
    }
    i32 n = 0;
    for (i32 i = 0; static_cast<u32>(i) < dir[2]; i++) {
        CTileImageSet* set = ReadImageSet(cursor);
        if (set == 0) {
            return -1;
        }
        n++;
        cursor += set->GetStride();
        m_imageSets.SetAtGrow(i, static_cast<CObject*>(set));
    }
    return n;
}

RVA(0x0015db30, 0xae)
i32 CGameLevel::RemovePlane(i32 index) {
    CDDrawWorkerHost* p = (index >= 0 && index < m_planes.GetSize())
                              ? static_cast<CDDrawWorkerHost*>(m_planes[index])
                              : 0;
    if (p == 0) {
        return 0;
    }
    i32 wasMain = p->m_flags & 1;
    delete p; // the inherited virtual scalar-deleting dtor (+0x04, flag 1)
    m_planes.RemoveAt(index, 1);
    if (wasMain) {
        i32 last = m_planes.GetSize() - 1;
        CDDrawWorkerHost* lp = (last >= 0 && last < m_planes.GetSize())
                                   ? static_cast<CDDrawWorkerHost*>(m_planes[last])
                                   : 0;
        if (lp != 0) {
            m_mainIndex = -1;
            m_mainPlane = 0;
            for (i32 i = 0; i < m_planes.GetSize(); i++) {
                (static_cast<CDDrawWorkerHost*>(m_planes[i]))->m_flags &= ~1;
            }
            m_mainIndex = last;
            m_mainPlane = lp;
            lp->m_flags |= 1;
        }
    }
    return 1;
}

RVA(0x0015dbe0, 0x70)
i32 CGameLevel::MovePlane(i32 from, i32 to) {
    if (from >= 0 && to < m_planes.GetSize()) {
        if (from == to) {
            return 1;
        }
        CDDrawWorkerHost* el =
            (from < m_planes.GetSize()) ? static_cast<CDDrawWorkerHost*>(m_planes[from]) : 0;
        if (el != 0) {
            m_planes.RemoveAt(from, 1);
            m_planes.InsertAt(to, static_cast<CObject*>(el), 1);
            if (el == m_mainPlane) {
                m_mainIndex = to;
            }
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// ScanSpanTop (@0x15e9c0): scan the object's top-edge span [extentL+x, extentR+x]
// at row (extentT+y), stepping the column by m_strideX (always hitting the far
// column). A soft (1) tile means blocked -> return the object's current m_screenY;
// a clear span returns the proposed row y. (4th arg unused.)
//
// The top-tested `while (col <= hiX)` form (not `if(col>hiX)return;do{}while`) is what
// reproduces retail's loop rotation - `jg exit; jmp into-body; back-edge reloads fixedY`.
// The DECLARATION ORDER hiX/fixedY/col picks the three spill slots and the prologue
// argument-load order; fixedY-first was the 95.7% plateau.
RVA(0x0015e9c0, 0x139)
i32 CGameLevel::ScanSpanTop(CGameObject* t, i32 x, i32 y, i32 unused) {
    i32 hiX = t->m_extent.right + x;
    i32 fixedY = t->m_extent.top + y;
    i32 col = t->m_extent.left + x;
    while (col <= hiX) {
        i32 result;
        PROBE_TILE(this, col, fixedY, result);
        if (result == kTileSoft) {
            return t->m_screenY;
        }
        if (col == hiX) {
            col++;
        } else {
            col += t->m_strideX;
        }
    }
    return y;
}

// ---------------------------------------------------------------------------
// SnapFloorDown (@0x15f090): scan the tile column at x downward from y to
// (m_screenY + m_extent.bottom) while the tiles stay soft (1/2); the first non-soft
// tile commits *out = row - m_extent.bottom and returns 1. An exhausted scan returns 0.
//
RVA(0x0015f090, 0x127)
i32 CGameLevel::SnapFloorDown(CGameObject* t, i32 x, i32 y, i32* out) {
    i32 limit = t->m_screenY + t->m_extent.bottom;
    for (i32 row = y; row >= limit; row--) {
        i32 result;
        PROBE_TILE(this, x, row, result);
        if (result != kTileSoft && result != kTileSoft2) {
            *out = row - t->m_extent.bottom;
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// SnapCeilUp (@0x15f340): the mirror of SnapFloorDown scanning upward from y to
// (m_screenY + m_extent.top - 1) while tiles stay soft (1); the first non-soft tile
// commits *out = row - m_extent.top and returns 1. An exhausted scan returns 0.
//
RVA(0x0015f340, 0x124)
i32 CGameLevel::SnapCeilUp(CGameObject* t, i32 x, i32 y, i32* out) {
    i32 limit = t->m_screenY + t->m_extent.top - 1;
    for (i32 row = y; row <= limit; row++) {
        i32 result;
        PROBE_TILE(this, x, row, result);
        if (result != kTileSoft) {
            *out = row - t->m_extent.top;
            return 1;
        }
    }
    return 0;
}

RVA(0x0015f470, 0x193)
i32 CGameLevel::ProbeSpanHard(CGameObject* t, i32 x, i32 off) {
    i32 py2 = t->m_extent.bottom + off + 1;
    i32 py1 = t->m_extent.top + off - 1;
    i32 r1;
    PROBE_TILE(this, x, py1, r1);
    if (r1 == kTileHard) {
        return 1;
    }
    i32 r2;
    PROBE_TILE(this, x, py2, r2);
    return r2 == kTileHard;
}

// ---------------------------------------------------------------------------
// ResolveMoveDown (@0x15f610): AdvanceA the cursor, gate the head/foot rows via
// AxisProbe, then run a downward SpanCheck-style scan (from m_screenY+m_extent.bottom+1
// down to headRow) for the first non-hard tile; on a hit past the cursor it turns
// the object mode 1 and re-bases the cursor. Returns the resolved cursor.
//
// Three shapes carry this one: the cursor is written back through the `y` PARAMETER
// (retail's cursor slot is y's incoming home), the scan floor is DERIVED from the
// recomputed head row (`head2 - y + screenY`, which is why the folded
// `screenY + bottom + 1` spelling never matched), and the hit path bumps `cur` in place
// (`++cur`) so the -1 survives instead of folding into `cur - bottom`.
RVA(0x0015f610, 0x191)
i32 CGameLevel::ResolveMoveDown(CGameObject* t, i32 x, i32 y, i32 flags) {
    y = AdvanceA(t, x, y, flags);
    i32 headRow = t->m_extent.bottom + y + 1;
    i32 footRow = t->m_extent.top + y - 1;
    if (AxisProbe(x, footRow) == kTileHard) {
        goto done;
    }
    if (AxisProbe(x, headRow) == kTileHard) {
        goto done;
    }
    {
        // the two AxisProbe calls invalidate the cached extent, so the head row is
        // recomputed - and the scan floor is derived FROM it, not folded.
        i32 head2 = t->m_extent.bottom + y + 1;
        i32 b = head2 - y + t->m_screenY;
        if (b > head2) {
            i32 cur = b - 1;
            while (cur >= head2) {
                i32 result;
                PROBE_TILE(this, x, cur, result);
                if (result != kTileHard) {
                    // the row BELOW the first clear one; retail bumps `cur` itself
                    // (`inc`), which is what keeps the -1 out of the fold.
                    ++cur;
                    if (cur > y) {
                        y = cur - t->m_extent.bottom - 1;
                        t->m_moveMode = 1;
                    }
                    goto done;
                }
                --cur;
            }
        }
    }
done:
    return y;
}

// ---------------------------------------------------------------------------
// ResolveMoveUp (@0x15f7b0): AdvanceB the cursor; unless the object already turned
// mode 1, if neither the foot row (inlined probe) nor the head row (AxisProbe) is
// hard, turn the object mode 4. Returns the cursor.
//
// The mode-1 gate is an EARLY RETURN, not an `if (mode != 1) { body }` wrapper: that is
// what shrink-wraps retail's `push ebp` past the test. And the advanced cursor is written
// back through the `y` PARAMETER - retail's cursor slot IS y's incoming home, with no
// copy - which is what the extra `mov [esp+..],reg` in the local-variable spelling was.
RVA(0x0015f7b0, 0x11f)
i32 CGameLevel::ResolveMoveUp(CGameObject* t, i32 x, i32 y, i32 flags) {
    y = AdvanceB(t, x, y, flags);
    if (t->m_moveMode == 1) {
        return y;
    }
    i32 headRow = t->m_extent.bottom + y + 1;
    i32 footRow = t->m_extent.top + y - 1;
    i32 result;
    PROBE_TILE(this, x, footRow, result);
    if (result != kTileHard) {
        if (AxisProbe(x, headRow) != kTileHard) {
            t->m_moveMode = 4;
        }
    }
    return y;
}

// ---------------------------------------------------------------------------
// StepGroundDown (@0x15f9f0): probe the foot row (m_extent.bottom+y+2) at x; a hard tile
// returns 1 (with, when arg flags bit4 set, a ClampSpan re-bracket writing the span
// midpoint into *out). A non-hard tile returns 0.
//
// @early-stop
// register-scheduling wall: the inlined PROBE_TILE + ClampSpan bracket + signed-halve
// pin the spill slots; logic + offsets + CFG + the ClampSpan convention exact. Deferred.
RVA(0x0015f9f0, 0x11a)
i32 CGameLevel::StepGroundDown(CGameObject* t, i32 x, i32 y, i32* out, i32 flags) {
    i32 probeY = t->m_extent.bottom + y + 2;
    i32 result;
    PROBE_TILE(this, x, probeY, result);
    if (result != kTileHard) {
        return 0;
    }
    if (flags & 0x10) {
        i32 lo = x, hi = x;
        *out = x;
        if (ClampSpan(x, probeY, &lo, &hi) != 0) {
            *out = (lo + hi) / 2;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// StepGroundUp (@0x15fb10): the mirror of StepGroundDown, probing the head row
// (m_extent.top+y-1) at x. Same hard-tile / ClampSpan-midpoint behaviour.
//
RVA(0x0015fb10, 0x119)
i32 CGameLevel::StepGroundUp(CGameObject* t, i32 x, i32 y, i32* out, i32 flags) {
    i32 probeY = t->m_extent.top + y - 1;
    i32 result;
    PROBE_TILE(this, x, probeY, result);
    if (result != kTileHard) {
        return 0;
    }
    if (flags & 0x10) {
        i32 lo = x, hi = x;
        *out = x;
        if (ClampSpan(x, probeY, &lo, &hi) != 0) {
            *out = (lo + hi) / 2;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ProbeStepEdge (@0x15fc30): returns 1 iff the tile at (x, y) is hard AND the tile
// one row above (x, y-1) is NOT hard - a wall with clear space above it (a step edge).
// (EXACT: the former "spilled x/y slot" wall was PROBE_TILE declaring px_ before py_ -
// the second inlined copy then reused x's dead param home instead of y's.)
RVA(0x0015fc30, 0x17f)
i32 CGameLevel::ProbeStepEdge(i32 x, i32 y) {
    i32 r1;
    PROBE_TILE(this, x, y, r1);
    if (r1 != kTileHard) {
        return 0;
    }
    i32 r2;
    PROBE_TILE(this, x, y - 1, r2);
    return r2 != kTileHard;
}

// ---------------------------------------------------------------------------
// ProbeFootSoft (@0x160080): probe the tile at the object's foot (m_screenX+dx,
// m_screenY+m_extent.bottom+1); returns 1 if it is soft (1 or 2), else 0. (The retail
// re-probes the same tile per compare - two inlined copies.)
//
// @early-stop
// 99.99% - ONE instruction pair: retail loads m_screenY (+0x60) into eax and
// m_extent.bottom (+0x140) into esi, we do the reverse. The pick is NOT in the source -
// ~35 expression and statement forms were tried (reversed operands, parenthesised,
// third-term-first, either member pre-read into a local, compound `+=`, `++row` split
// out) and all emit the same order; a standalone `struct AB : CGameLevel` replica of the
// IDENTICAL source picks RETAIL's order here, so it is TU-cumulative back-end state.
// docs/patterns/two-member-add-load-order-is-canonicalized.md. Everything else is
// byte-exact (the nested `!=` guards fixed the block order: 98.72 -> 99.99).
RVA(0x00160080, 0x187)
i32 CGameLevel::ProbeFootSoft(CGameObject* t, i32 dx) {
    i32 row = t->m_screenY + t->m_extent.bottom + 1;
    // NESTED not-equal guards, not `if (==) goto yes`: cl gives the last conditional's
    // FALL-THROUGH to whichever arm is the if-BODY. Retail's every probe guard is
    // `je <the single return-1 block, last>` with the next probe (and finally `return 0`)
    // as the fall-through - which is exactly `if (probe != want) { ...next... }`.
    i32 r1;
    PROBE_TILE(this, dx + t->m_screenX, row, r1);
    if (r1 != kTileSoft) {
        i32 r2;
        PROBE_TILE(this, dx + t->m_screenX, row, r2);
        if (r2 != kTileSoft2) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ProbeFootBlocked (@0x160210): like ProbeFootSoft but returns 1 if the foot tile
// is any blocking kind (soft 1, soft2 2, or hard 3), else 0. Three inlined probes.
//
// @early-stop
// 99.99% (was 99.07): the nested `!=` guards fixed the last probe's branch polarity
// and both exit blocks' order. Residual is the same single commutative-add LOAD-ORDER
// pair as ProbeFootSoft - see its note and
// docs/patterns/two-member-add-load-order-is-canonicalized.md.
RVA(0x00160210, 0x234)
i32 CGameLevel::ProbeFootBlocked(CGameObject* t, i32 dx) {
    i32 row = t->m_screenY + t->m_extent.bottom + 1;
    // Nested not-equal guards - see ProbeFootSoft: the if-BODY owns the fall-through, so
    // each probe guard lowers to `je <the single return-1 block>` as retail has it.
    i32 r1;
    PROBE_TILE(this, dx + t->m_screenX, row, r1);
    if (r1 != kTileSoft) {
        i32 r2;
        PROBE_TILE(this, dx + t->m_screenX, row, r2);
        if (r2 != kTileSoft2) {
            i32 r3;
            PROBE_TILE(this, dx + t->m_screenX, row, r3);
            if (r3 != kTileHard) {
                return 0;
            }
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ScanRowSpan (@0x160c50): scan the tile row y from column x0 toward x1 (stepping
// +/- step by direction) - any soft (1) tile returns 0; then probe the end column
// x1 and return whether it is non-soft. A pure tile-line clearance test.
//
// @early-stop
// ~91.7%: the two directional for-loops + final probe are byte-faithful in shape.
// Residual is a free-register swap in the PROBE_TILE clamp: the col (X-loop var) and
// y (fixed row) land in ebx/edi (ours) vs edi/ebx (retail), cascading the clamp temp
// choices; a shared `goto`-return for the two `return 0` paths was matching-neutral
// (retail already merges them). Not source-steerable. Deferred to the final sweep.
RVA(0x00160c50, 0x289)
RVA_COMPGEN(0x00161350, 0x1e, ??_GCImageSet1@@UAEPAXI@Z)
RVA_COMPGEN(0x00161440, 0x1e, ??_GCImageSet2@@UAEPAXI@Z)
RVA_COMPGEN(0x001614e0, 0x1e, ??_GCImageSet3@@UAEPAXI@Z)
i32 CGameLevel::ScanRowSpan(i32 x0, i32 y, i32 x1, i32 step) {
    if (x1 > x0) {
        for (i32 col = x0; col <= x1; col += step) {
            i32 r;
            PROBE_TILE(this, col, y, r);
            if (r == kTileSoft) {
                return 0;
            }
        }
    } else {
        for (i32 col = x0; col >= x1; col -= step) {
            i32 r;
            PROBE_TILE(this, col, y, r);
            if (r == kTileSoft) {
                return 0;
            }
        }
    }
    i32 rf;
    PROBE_TILE(this, x1, y, rf);
    return rf != kTileSoft;
}

VTBL(CGameLevel, 0x001f0150); // ??_7CGameLevel (was g_gameLevelVtbl)
VTBL(CImageSet1, 0x001f0198); // ??_7CImageSet1 (was g_imageSet1Vtbl)
VTBL(CImageSet2, 0x001f01e0); // ??_7CImageSet2 (was g_imageSet2Vtbl)
VTBL(CImageSet3, 0x001f0228); // ??_7CImageSet3 (was g_imageSet3Vtbl)
