// GameLevel.cpp - CGameLevel::LoadWwd, the WWD level-load orchestrator.
//
// Functions in this TU:
//   CGameLevel::LoadWwd  (vtable slot 0x38)
//
// LoadWwd is the level-load driver. Faithful reconstruction (carcass driven toward
// byte-exact; see header for the member layout). Flow, straight off the bytes:
//
//   1. Reset()                              (this->vtable[+0x44])
//   2. if (hdr->wwdSignature > 0x5F4) return 0;
//   3. m_header = *hdr;                      (rep movs 0x17d dwords = 1524 B)
//   4. if (hdr->flags & COMPRESS) {          (test [hdr+8],0x2)
//        char* block = (char*)operator new(hdr->mainBlockLength
//                                           + hdr->wwdSignature + 0x40);
//        if (!block) return 0;
//        block = WwdFile_InflateMainBlock(hdr, block, size);
//        if (!block) { operator delete(...); return 0; }   // ehAlloc tracked
//      } else block = (char*)hdr;            (uncompressed: read in place)
//   5. strcpy(m_levelName, hdr->levelName);  (inline strlen + rep movs)
//      m_flags = hdr->flags; m_checksum = hdr->checksum;
//   6. for (i = 0; i < hdr->numPlanes; ++i)  (plane cursor stride 0xA0)
//        if (!ReadPlane(cursor, block, &m_planeCtx)) goto fail;
//   7. if (hdr->tileDescriptionsOffset) {    (image-set descriptors)
//        for (j = 0; j < count; ++j) {
//          CImageSet* s = ReadImageSet(cursor); if (!s) goto fail;
//          m_imageSets.SetAtGrow(j, s); cursor += s->GetStride();
//        }
//      }
//   8. recompute the scaled start coords on the main plane + every plane;
//      free the tracked inflate buffer; return 1.
//
// CPlane / CImageSet / the per-plane reader / ReadImageSet / RecomputePlaneCoords /
// InflateMainBlock / operator new/delete / SetAtGrow are reloc-masked calls.
// <Mfc.h> brings real MFC afxcoll: CDWordArray (the engine stores the pointer arrays as DWORDs).
#include <Mfc.h>
#include <Gruntz/GameLevel.h>
#include <Io/FileStream.h> // CFileIO (Open/Read/GetLength/ctor/dtor reloc-masked)
#include <rva.h>

#include <string.h> // strcpy, memset

// ===========================================================================
// The CDDrawLevelData methods, merged in here as CGameLevel.
//
// CGameLevel is the same class the engine handles via the obfuscated name
// CDDrawLevelData (its vtable slot 0x38 is CGameLevel::LoadWwd). The methods
// below were reconstructed as CDDrawLevelData::* and are moved here onto
// CGameLevel. Each touches the level's own members through their named fields
// (m_planeCtx@+0x10, m_planes/m_imageSets, m_owner@+0x0c, m_04@+0x04, the
// m_b0..m_dc default-extents block, m_header@+0xe0). The per-plane / edit-state /
// visit-context objects they dispatch into are UNMATCHED engine classes, modeled
// as typed window structs (PlaneGeom/LevelPlane/MainPlane/LevelScroll/VisitCtx/
// EditSink) that view the same object at the offsets each method touches.
//
// The shared "default extents" block at +0xb0..+0xdc is stamped with the same
// constants by the ctor and several edit methods (StampParamBlock):
//     +0xb0 = 500  +0xb4 = 250  +0xb8 = 1000 +0xbc = 1000
//     +0xc0 = 250  +0xc4 = 125  +0xc8 = 1600 +0xcc = 1200
//     +0xd0 = 2560 +0xd4 = 1920 +0xd8 = 768  +0xdc = 576
// ===========================================================================

// (LevelCoordRect - the 4-int coord record at +0x10 - is defined in GameLevel.h.)

// The parse-source object LoadFromSource drives: BeginParse (FUN_00539960
// @0x139960) opens/primes it and returns a handle; EndParse (FUN_005399d0
// @0x1399d0) tears it down. Both are unmatched engine leaves taking the source as
// `this`; declared with no body so the thiscall sites reloc-mask in objdiff.
struct CParseSource {
    i32 BeginParse();
    void EndParse();
};

// CGameLevelChild - placeholder for whatever class lives in the pointer arrays
// at +0x38 and +0x4c. Only vtable slot 4 (+0x04, virtual Release(1)) is used.
class CGameLevelChild {
public:
    virtual void Dummy();
    virtual void Release(i32 arg);
};

// PlaneGeom - the in-memory plane the level recomputes coords on. This is the
// same object as CPlane (WwdFile.h), viewed at the offsets RecomputePlaneCoords
// touches; RecomputePlaneCoords is a __thiscall taking the plane as `this` (the
// retail call site is a bare `call` with the plane already in ecx - NO pushed
// argument), so it is modeled as a method here per the matcher __thiscall idiom.
// Layout (a window onto CPlane):
//   +0x08 flags  (bit2 = wrap X, bit3 = wrap Y)
//   +0x10/+0x14  scaledX / scaledY (float scroll origin, pre-stored by LoadWwd)
//   +0x30/+0x34  tilesWide / tilesHigh (int wrap modulus)
//   +0x40/+0x44  out: tile-origin X / Y     +0x48/+0x4c out: tile-extent X / Y
//   +0x70/+0x74  viewport tiles across/down +0x78/+0x7c view-anchor X / Y
//   +0x84/+0x88  out: integer scaledX / scaledY
struct PlaneGeom {
    char pad_0[0x08];
    u32 flags; // +0x08
    char pad_c[0x10 - 0x0c];
    float scaledX; // +0x10
    float scaledY; // +0x14
    char pad_18[0x30 - 0x18];
    i32 tilesWide; // +0x30
    i32 tilesHigh; // +0x34
    char pad_38[0x40 - 0x38];
    i32 originX; // +0x40
    i32 originY; // +0x44
    i32 extentX; // +0x48
    i32 extentY; // +0x4c
    char pad_50[0x70 - 0x50];
    i32 viewW;   // +0x70
    i32 viewH;   // +0x74
    i32 anchorX; // +0x78
    i32 anchorY; // +0x7c
    char pad_80[0x84 - 0x80];
    i32 intX; // +0x84
    i32 intY; // +0x88

    void RecomputePlaneCoords();
};

// The two-phase vptr stores are now cl-emitted: the inlined CLoadable ctor (in
// GameLevel.h) auto-stamps the base vptr (&??_7CLoadable, orphan reloc-masked
// against retail 0x5efc30) and the derived CGameLevel ctor auto-stamps the derived
// vptr (&??_7CGameLevel, bound @0x5f0150 via VTBL below) after the three array
// members are constructed. The only remaining manual vtable store is the grand-base
// teardown vftable ~CLoadable restamps after the member dtors run (@0x5e8cb4).
DATA(0x001e8cb4)
extern void* g_wapObjectDtorVtbl;

// The "unset" sentinel the ctor writes into the coord record's min corner; the
// readiness predicate (IsLoaded) tests for it and Unload restores it.
static const i32 LEVEL_COORD_UNSET = (i32)0x80000000;

// LookupTile's empty-cell sentinels: 0xeeeeeeee is the uninitialized-heap fill
// (no tile placed); -1 is the explicit "clear" marker.
static const i32 TILE_UNINIT = (i32)0xeeeeeeee;
static const i32 TILE_CLEAR = -1;

// Stamps the shared +0xb0..+0xdc "default parameters" block. Defined inline so it
// folds into each method exactly as the retail compiler emitted the block inline.
static inline void StampParamBlock(CGameLevel* o) {
    o->m_b0 = 500;
    o->m_b4 = 250;
    o->m_b8 = 1000;
    o->m_bc = 1000;
    o->m_c0 = 250;
    o->m_c4 = 125;
    o->m_c8 = 1600;
    o->m_cc = 1200;
    o->m_d0 = 2560;
    o->m_d4 = 1920;
    o->m_d8 = 768;
    o->m_dc = 576;
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
// store-scheduling / EH-state-base entropy plateau + cl-emitted two-phase vptr.
// The two vptr stores are now cl-generated (base ??_7CLoadable orphan + derived
// ??_7CGameLevel @0x5f0150), the derived store now matching retail's &0x5f0150 exactly.
// Residue is the same funcinfo EH-state numbering base shift (retail tags the three
// array ctors 0/1/2; cl uses the -1 entry state then 0/1) plus one immediate (0xfa)
// landing in a different register from the this-reload for the fs:0 restore. Logic +
// all offsets + the two-phase construction + CFG + the EH frame are exact; not
// source-steerable (matching-patterns.md §entropy).
RVA(0x0015ccd0, 0x118)
CGameLevel::CGameLevel(i32 a1, i32 a2, i32 a3) : CLoadable(a1, a2, a3) {
    m_scrollStepX = 0x40;
    m_scrollStepY = 0x40;
    m_b4 = 250;
    m_c0 = 250;
    m_b8 = 1000;
    m_bc = 1000;

    // cl auto-stamps &??_7CGameLevel here (the derived phase of the two-phase store).
    m_planeCtx.minX = LEVEL_COORD_UNSET;
    m_mainPlane = 0;
    m_mainIndex = -1;
    m_checksum = 0;
    m_b0 = 500;
    m_c4 = 125;
    m_c8 = 1600;
    m_cc = 1200;
    m_d0 = 2560;
    m_d4 = 1920;
    m_d8 = 768;
    m_dc = 576;
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
    char* block = (char*)hdr;
    char* ehAlloc = 0; // inflate buffer freed on every exit path

    // The flags field is read twice (the COMPRESS test and the m_flags store); the
    // retail compiler materializes &hdr->flags once and dereferences it both times,
    // so model it as a cached pointer.
    u32* pflags = &hdr->flags;

    if (*pflags & 0x2) // COMPRESS: inflate the main block
    {
        u32 allocSize = hdr->mainBlockLength + hdr->wwdSignature + 0x40;
        char* buf = (char*)operator new(allocSize);
        if (buf == 0) {
            return 0;
        }

        block = (char*)WwdFile_InflateMainBlock((WwdHeader*)hdr, (Bytef*)buf, allocSize - 0x20);
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
    if (hdr->numPlanes != 0) {
        do {
            if (ReadPlane(cursor, block, &m_planeCtx) == 0) {
                goto fail;
            }
            ++i;
            cursor += 0xa0; // WwdPlaneHeader stride
        } while (i < hdr->numPlanes);
    }

    // --- image-set descriptors ---------------------------------------------
    // The descriptor read is its own int-returning routine (inlined here): it
    // validates the record pointer, walks `count` descriptors appending each
    // CImageSet, and returns the number read (or -1 on a bad pointer / failed
    // read). count is re-read from the record header each iteration (rec is spilled).
    if (hdr->tileDescriptionsOffset != 0) {
        char* rec = block + hdr->tileDescriptionsOffset;
        char* elem = rec + 0x20;
        if (elem == 0) {
            result = -1;
        } else if (rec == 0) {
            result = -1;
        } else {
            i32 n = 0;
            i32 j = 0;
            while ((u32)j < *(u32*)(rec + 0x8)) {
                CImageSet* set = ReadImageSet(elem);
                if (set == 0) {
                    result = -1;
                    goto check_result;
                }
                ++n;
                elem += set->GetStride(); // vtable +0x24 stride advance
                m_imageSets.SetAtGrow(j, (DWORD)set);
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
        CPlane* mp = m_mainPlane;
        if (mp->m_flags & 1) {
            mp->m_scaledX = (float)startX;
            mp->m_scaledY = (float)startY;
        } else {
            mp->m_scaledX = (float)startX * mp->m_scaleX;
            mp->m_scaledY = (float)startY * mp->m_scaleY;
        }
        ((PlaneGeom*)mp)->RecomputePlaneCoords();

        // Re-derive the start coords from the main plane's origin for the rest.
        i32 ox = m_mainPlane->m_originX;
        i32 oy = m_mainPlane->m_originY;
        i32 i2 = 0;
        while (i2 < m_planes.GetSize()) // GetSize() == the plane count
        {
            if (i2 != m_mainIndex) {
                CPlane* p = (CPlane*)m_planes[i2];
                if (p->m_flags & 1) {
                    p->m_scaledX = (float)ox;
                    p->m_scaledY = (float)oy;
                } else {
                    p->m_scaledX = (float)ox * p->m_scaleX;
                    p->m_scaledY = (float)oy * p->m_scaleY;
                }
                ((PlaneGeom*)p)->RecomputePlaneCoords();
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

// ===========================================================================
// Merged CDDrawLevelData leaves (now CGameLevel). All are plain /O2 /MT leaves:
// NO SEH frame, NO string/global relocations (dumps report "Relocations: none") -
// they only touch member offsets (written as raw casts on `this`), an argument
// struct, and sibling virtuals via the object's own vtable.
//
// The three 184-byte siblings (Unknown24/28/2C) are identical except for which
// sibling virtual they dispatch to: vtable +0x38 / +0x3c / +0x40 respectively.
// Each loads the +0x10 record from a caller struct, stamps the param block, then
// calls that sibling virtual with arg1; on a 0 result it invokes the +0x1c
// virtual (a "fail/reset" hook) and returns 0, else returns 1.
// ===========================================================================

// ---------------------------------------------------------------------------
// CGameLevel::IsLoaded (0x161190) adds a +0x10 sentinel check before the common parent/status predicate.
RVA(0x00161190, 0x1f)
i32 CGameLevel::IsLoaded() {
    if (m_planeCtx.minX == LEVEL_COORD_UNSET) {
        goto fail;
    }
    if (m_owner == 0) {
        goto fail;
    }
    if (m_04 != -1) {
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
// m_b0=500 direct-immediate store, which cl hoists into the w-read/dec window while
// retail emits it after the m_b8/m_bc=1000 stores. Inlining the block in retail's
// store order regressed it further (74.8%); not source-steerable. Deferred.
RVA(0x0015d030, 0x8f)
i32 CGameLevel::SetCoordExtents(i32 w, i32 h) {
    m_planeCtx.minX = 0;
    m_planeCtx.minY = 0;
    m_planeCtx.maxX = w - 1;
    m_planeCtx.maxY = h - 1;
    StampParamBlock(this);
    return 1;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs (merged from CDDrawResolveSubMgrLayout).
// -------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// LoadFromFile (vtable +0x40): open the named file, slurp it whole into a heap
// buffer, and hand the buffer to the +0x38 load virtual (the same slot LoadWwd
// dispatches). Returns 1 on success, 0 on any failure. The local CFileIO + the
// heap buffer are both freed on every exit, which is why the TU carries the /GX EH
// frame. NOTE: the file Read's byte count is DISCARDED (no compare to the length)
// - only the +0x38 virtual's result decides success.
RVA(0x0015d500, 0x127)
i32 CGameLevel::LoadFromFile(const char* path) {
    CFileIO file;

    if (!file.Open(path, 0, 0)) {
        return 0;
    }

    void* buf = operator new(file.GetLength());
    if (!buf) {
        return 0;
    }

    file.Read(buf, file.GetLength());
    if (LoadWwd((WwdHeader*)buf) == 0) { // vtable +0x38 (slot 14) load virtual
        operator delete(buf);
        return 0;
    }
    operator delete(buf);
    return 1;
}

// ---------------------------------------------------------------------------
// LoadFromSource (vtable +0x3c): drive a parse/load through `arg`. Begin it
// (BeginParse on arg); if that fails, return 0. Else feed its handle to the +0x38
// load virtual and finish the parse (EndParse on arg) regardless, returning the
// +0x38 result (1/0). BeginParse/EndParse are unmatched engine leaves on the arg
// object (reloc-masked thiscall).
RVA(0x0015d630, 0x41)
i32 CGameLevel::LoadFromSource(CParseSource* arg) {
    i32 handle = arg->BeginParse();
    if (handle == 0) {
        return 0;
    }
    if (LoadWwd((WwdHeader*)handle) == 0) { // vtable +0x38 (slot 14) load virtual
        arg->EndParse();
        return 0;
    }
    arg->EndParse();
    return 1;
}

// ---------------------------------------------------------------------------
// Scalar-deleting destructor (vtable slot 1): run the destructor, then free the
// object when bit0 of the flag is set; returns `this`. The compiler-standard thunk.
RVA(0x001611c0, 0x1e)
void* CGameLevel::ScalarDtor(u32 flags) {
    this->~CGameLevel(); // call ??1CGameLevel
    if (flags & 1) {
        operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// Destructor: cl auto-stamps the derived vftable @0x5f0150 at dtor entry
// (polymorphic), then runs the level cleanup, lets the three array members destruct
// (reverse construction order), then ~CLoadable restores the base subobject
// (resets m_04/m_flags/m_owner + the grand-base dtor vftable @0x5e8cb4). The
// destructible array members give the /GX EH frame.
RVA(0x001611e0, 0x82)
CGameLevel::~CGameLevel() {
    Unload(); // level cleanup (releases children, clears the header)
    // m_imageSets / m_planes / m_array20 auto-destruct here; ~CLoadable follows.
}

// ---------------------------------------------------------------------------
// Unload: like ReleaseChildren plus resets the coord sentinel and zeroes the
// WwdHeader buffer.
// ---------------------------------------------------------------------------
RVA(0x0015d1f0, 0x87)
i32 CGameLevel::Unload() {
    i32 i;
    for (i = 0; i < m_planes.GetSize(); i++) {
        CGameLevelChild* child = (CGameLevelChild*)m_planes.GetData()[i];
        if (child) {
            child->Release(1);
        }
    }
    m_planes.SetSize(0, -1);
    for (i = 0; i < m_imageSets.GetSize(); i++) {
        CGameLevelChild* child = (CGameLevelChild*)m_imageSets.GetData()[i];
        if (child) {
            child->Release(1);
        }
    }
    m_imageSets.SetSize(0, -1);
    m_planeCtx.minX = LEVEL_COORD_UNSET;
    m_mainPlane = 0;
    m_mainIndex = -1;
    memset(&m_header, 0, 1524);
    return 0;
}

// ---------------------------------------------------------------------------
// ReleaseChildren: releases all child pointers, resets both CDWordArrays, clears
// the main-plane fields.
// ---------------------------------------------------------------------------
RVA(0x0015d680, 0x71)
void CGameLevel::ReleaseChildren() {
    i32 i;
    for (i = 0; i < m_planes.GetSize(); i++) {
        CGameLevelChild* child = (CGameLevelChild*)m_planes.GetData()[i];
        if (child) {
            child->Release(1);
        }
    }
    m_planes.SetSize(0, -1);
    for (i = 0; i < m_imageSets.GetSize(); i++) {
        CGameLevelChild* child = (CGameLevelChild*)m_imageSets.GetData()[i];
        if (child) {
            child->Release(1);
        }
    }
    m_imageSets.SetSize(0, -1);
    m_mainPlane = 0;
    m_mainIndex = -1;
}

// ---------------------------------------------------------------------------
// GetClassId: returns the class type tag (constant 0x19 / 25).
// ---------------------------------------------------------------------------
RVA(0x001611b0, 0x6)
i32 CGameLevel::GetClassId() {
    return 0x19;
}

// --- the SetCoordsAndLoadNN sibling family (do not drop) -------------------
// ---------------------------------------------------------------------------
// As SetCoordsAndLoad38 but dispatches the +0x40 load virtual.
RVA(0x0015cdf0, 0xb8)
i32 CGameLevel::SetCoordsAndLoad40(i32 arg1, LevelCoordRect* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    if (LoadFromFile((const char*)arg1) == 0) { // vtable +0x40 (slot 16)
        Unload();                               // vtable +0x1c (slot 7), fail/reset hook
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// As SetCoordsAndLoad38 but dispatches the +0x3c load virtual.
RVA(0x0015ceb0, 0xb8)
i32 CGameLevel::SetCoordsAndLoad3C(i32 arg1, LevelCoordRect* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    if (LoadFromSource((CParseSource*)arg1) == 0) { // vtable +0x3c (slot 15)
        Unload();                                   // vtable +0x1c (slot 7), fail/reset hook
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Loads the +0x10 record from *coords, stamps the param block, then dispatches
// the +0x38 load virtual with arg1. On a 0 result it runs the +0x1c hook and
// returns 0; otherwise returns 1.
RVA(0x0015cf70, 0xb8)
i32 CGameLevel::SetCoordsAndLoad38(i32 arg1, LevelCoordRect* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    if (LoadWwd((WwdHeader*)arg1) == 0) { // vtable +0x38 (slot 14)
        Unload();                         // vtable +0x1c (slot 7), fail/reset hook
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// SetCoords: loads the +0x10 record from *coords, stamps the param block, returns 1.
RVA(0x0015d0d0, 0x99)
i32 CGameLevel::SetCoords(LevelCoordRect* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    return 1;
}

// ---------------------------------------------------------------------------
// CGameLevel::ReadImageSet (image-set factory) - reads one image-set descriptor
// from `record`. The first int of the record selects one of three variants;
// `operator new` allocates it (0x10 / 0x24 / 0x18 bytes), the matching engine
// vftable is stamped, and the count/cursor field at +0x04 (plus +0x14 for the
// 0x18-byte kind) is zeroed. The variant's Parse slot (+0x14) then reads the
// record; on a 0 result the object's Release slot (+0x04) frees it and 0 is
// returned. The three vftables are UNMATCHED engine tables, so the stamp
// references the retail tables by address (reloc-masked DIR32). NOTE: retail
// invokes Parse unconditionally - even when the allocation failed and the
// pointer is null - so the deref is written without a guard, matching the bytes.
// The engine routes object allocation through the Rez heap (RezAlloc @0x1b9b46 =
// nothrow operator new / RezFree @0x1b9b82). ReadImageSet `new`s its variants
// through RezAlloc, so each class models it as the class allocator: `new CImageSetN`
// emits a direct `push size; call RezAlloc` instead of the global `??2`.
extern "C" void* RezAlloc(u32 size); // 0x1b9b46
extern "C" void RezFree(void* p);    // 0x1b9b82

// The three CImageSet variants the factory allocates. REAL-POLYMORPHIC now: each is
// a flat 18-slot class so cl emits its ??_7CImageSetN@@6B@ (bound below via VTBL to
// the retail vtable) and AUTO-stamps the vptr in the INLINE ctor - lowering
// `new CImageSetN` to exactly the retail `RezAlloc(size); if (p) { stamp vptr; zero
// fields }` shape. Only slot 5 (+0x14 Parse) is a matched body; the base-thunk +
// engine slots are declared-only (their vtable entries reloc-mask). The vptr sits
// at +0x00 (implicit); the padding pins each size: kind 1 = 0x10, kind 2 = 0x24,
// kind 3 = 0x18. Slot RVAs (from retail 0x5f0198/01e0/0228) noted per class.
struct CImageSet1 {
    virtual void s00();              // [0]  0x1bef01
    virtual void* Release(u32 flag); // [1]  +0x04  0x161350 scalar-deleting dtor
    virtual void s08();              // [2]  0x0028ec
    virtual void s0c();              // [3]  0x00106e
    virtual void s10();              // [4]  0x004034
    virtual i32 Parse(void* record); // [5]  +0x14  0x166d40
    virtual void s18();              // [6]  0x161330
    virtual void s1c();              // [7]  0x161340
    virtual void s20();              // [8]  0x161380
    virtual void s24();              // [9]  +0x24  0x161410 (GetStride slot)
    virtual void s28();              // [10] 0x161390
    virtual void s2c();              // [11] 0x1613a0
    virtual void s30();              // [12] 0x1613b0
    virtual void s34();              // [13] 0x1613c0
    virtual void s38();              // [14] 0x1613d0
    virtual void s3c();              // [15] 0x1613e0
    virtual void s40();              // [16] 0x1613f0
    virtual void s44();              // [17] 0x161400
    CImageSet1() {
        m_04 = 0; // cl auto-stamps &??_7CImageSet1 first
    }
    void* operator new(size_t n) {
        return RezAlloc((u32)n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }
    void DtorBase(); // 0x161370  base-subobject dtor (vtable restamp)
    i32 m_04;        // +0x04
    i32 m_08;        // +0x08
    i32 m_0c;        // +0x0c
};
struct CImageSet2 {
    virtual void s00();              // [0]  0x1bef01
    virtual void* Release(u32 flag); // [1]  +0x04  0x161440 scalar-deleting dtor
    virtual void s08();              // [2]  0x0028ec
    virtual void s0c();              // [3]  0x00106e
    virtual void s10();              // [4]  0x004034
    virtual i32 Parse(void* record); // [5]  +0x14  0x166990
    virtual void s18();              // [6]  0x161420
    virtual void s1c();              // [7]  0x161430
    virtual void s20();              // [8]  0x161470
    virtual void s24();              // [9]  +0x24  0x1614a0 (GetStride slot)
    virtual void s28();              // [10] 0x1669e0
    virtual void s2c();              // [11] 0x166a40
    virtual void s30();              // [12] 0x166b90
    virtual void s34();              // [13] 0x166bf0
    virtual void s38();              // [14] 0x166ab0
    virtual void s3c();              // [15] 0x166b20
    virtual void s40();              // [16] 0x166c60
    virtual void s44();              // [17] 0x166cd0
    CImageSet2() {
        m_04 = 0; // cl auto-stamps &??_7CImageSet2 first
    }
    void* operator new(size_t n) {
        return RezAlloc((u32)n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }
    i32 m_04; // +0x04
    i32 m_08; // +0x08
    i32 m_0c; // +0x0c
    i32 m_10; // +0x10
    i32 m_14; // +0x14
    i32 m_18; // +0x18
    i32 m_1c; // +0x1c
    i32 m_20; // +0x20
};
struct CImageSet3 {
    virtual void s00();              // [0]  0x1bef01
    virtual void* Release(u32 flag); // [1]  +0x04  0x1614e0 scalar-deleting dtor
    virtual void s08();              // [2]  0x0028ec
    virtual void s0c();              // [3]  0x00106e
    virtual void s10();              // [4]  0x004034
    virtual i32 Parse(void* record); // [5]  +0x14  0x166d70
    virtual void s18();              // [6]  0x1614b0
    virtual void s1c();              // [7]  0x1614d0
    virtual void s20();              // [8]  0x161570
    virtual void s24();              // [9]  +0x24  0x161590 (GetStride slot)
    virtual void s28();              // [10] 0x166e00
    virtual void s2c();              // [11] 0x166e60
    virtual void s30();              // [12] 0x166eb0
    virtual void s34();              // [13] 0x166f20
    virtual void s38();              // [14] 0x166f80
    virtual void s3c();              // [15] 0x166ff0
    virtual void s40();              // [16] 0x167050
    virtual void s44();              // [17] 0x1670d0
    CImageSet3() {
        m_04 = 0; // cl auto-stamps &??_7CImageSet3 first
        m_14 = 0;
    }
    void* operator new(size_t n) {
        return RezAlloc((u32)n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }
    i32 m_04;   // +0x04  tile width
    i32 m_08;   // +0x08  tile height
    i32 m_0c;   // +0x0c  log2(height)
    i32 m_10;   // +0x10  width*height (byte size)
    void* m_14; // +0x14  owned pixel buffer
};

RVA(0x0015d820, 0xa3)
CImageSet* CGameLevel::ReadImageSet(void* record) {
    if (record == 0) {
        return 0;
    }
    CImageSet* set;
    switch (*(i32*)record) {
        case 1:
            set = (CImageSet*)new CImageSet1;
            break;
        case 2:
            set = (CImageSet*)new CImageSet2;
            break;
        case 3:
            set = (CImageSet*)new CImageSet3;
            break;
        default:
            return 0;
    }

    if (set->Parse(record) == 0) {
        if (set != 0) {
            set->Release(1);
        }
        return 0;
    }
    return set;
}

// CImageSet1::DtorBase (0x161370) - the base-subobject destructor invoked by the
// scalar-deleting-destructor (g_imageSet1Vtbl slot +0x04, unmatched). The base
// has no members, so it only restamps the base-subobject (CObject-like) dtor
// vftable @0x5e8cb4 - the same table the level family's ~CLoadable restores.
RVA(0x00161370, 0x7)
void CImageSet1::DtorBase() {
    *(void**)this = &g_wapObjectDtorVtbl;
}

// CImageSet1::Parse (0x166d40, g_imageSet1Vtbl slot +0x14). Copies three dwords
// from the WWD record at +0x08.. into m_04/m_08/m_0c via an advancing source
// pointer (retail's `add eax,8; mov (eax); add eax,4` cursor walk) and returns TRUE.
// @early-stop
// tail-peephole wall (same as CImageSet2/3): retail keeps the 2nd store's `add eax,4`
// then reads the 3rd via [eax]; cl folds that advance into the 3rd's +4 displacement.
// The advancing-cursor prologue + first read are byte-exact; the final fold is the
// documented entropy-tail wall (docs/patterns/header-fields-through-cursor-not-index.md).
RVA(0x00166d40, 0x24)
i32 CImageSet1::Parse(void* record) {
    i32* p = (i32*)((char*)record + 8);
    m_04 = *p++;
    m_08 = *p++;
    m_0c = *p++;
    return 1;
}

// CImageSet2::Parse (0x166990, g_imageSet2Vtbl slot +0x14). Copies eight dwords
// from the WWD record at +0x08.. into m_04..m_20 via an advancing source pointer
// and returns TRUE.
// @early-stop
// tail-peephole wall (~94%): retail advances the source pointer on the 7th store
// (add eax,4) then reads the 8th via [eax]; cl folds the 7th advance into the
// 8th's +4 displacement. Body otherwise byte-exact; not source-steerable.
RVA(0x00166990, 0x4c)
i32 CImageSet2::Parse(void* record) {
    i32* p = (i32*)((char*)record + 8);
    m_04 = *p++;
    m_08 = *p++;
    m_0c = *p++;
    m_10 = *p++;
    m_14 = *p++;
    m_18 = *p++;
    m_1c = *p++;
    m_20 = *p++;
    return 1;
}

// CImageSet3::Parse (0x166d70, g_imageSet3Vtbl slot +0x14). Reads tile width/height
// from the record at +0x08/+0x0c, derives the height log2 shift and the byte size,
// and - only when the width is the matching power of two - allocates and copies the
// tile pixels from the record at +0x10 (inline memcpy). TRUE on a successful copy.
// @early-stop
// regalloc wall (~88%): retail parks the width in callee-saved edi (push edi, mov
// edi,ecx) and multiplies via edx; cl keeps the width in edx and multiplies into
// ecx (one fewer move). Logic + memcpy byte-exact; not source-steerable.
RVA(0x00166d70, 0x8d)
i32 CImageSet3::Parse(void* record) {
    i32* p = (i32*)((char*)record + 8);
    i32 w = *p++;
    m_04 = w;
    i32 h = *p++;
    m_08 = h;
    m_0c = 0;
    m_10 = w * h;
    for (; h > 1; h >>= 1) {
        m_0c++;
    }
    if ((1 << m_0c) != w) {
        return 0;
    }
    void* dst = operator new(m_10);
    m_14 = dst;
    if (dst == 0) {
        return 0;
    }
    memcpy(dst, p, m_10);
    return 1;
}

// ---------------------------------------------------------------------------
// PlaneGeom::RecomputePlaneCoords - recompute one plane's scaled scroll origin
// and visible-tile extents from its (already-scaled) float coords. __thiscall
// with `this` = the plane (ecx); reloc-masks only the float 0.0 constant and the
// CRT __ftol helper (the (int)float casts). X and Y are computed identically:
// wrap (flags bit set) folds the coord modulo the tile count into [0, count);
// else it clamps to [0, count-1].
RVA(0x00161c90, 0x1e4)
void PlaneGeom::RecomputePlaneCoords() {
    PlaneGeom* p = this;
    u32 flags = p->flags;
    i32 wrapX = flags & 4;

    // --- X axis: wrap/clamp scaledX into the tile grid -----------------------
    if (wrapX) {
        if (p->scaledX < 0.0f) {
            do {
                p->scaledX += (float)p->tilesWide;
            } while (p->scaledX < 0.0f);
        }
        if (p->scaledX >= (float)p->tilesWide) {
            float t = p->scaledX;
            do {
                t -= (float)p->tilesWide;
            } while (t >= (float)p->tilesWide);
            p->scaledX = t;
        }
    } else {
        if (p->scaledX < 0.0f) {
            p->scaledX = 0;
        } else if ((float)p->tilesWide <= p->scaledX) {
            p->scaledX = (float)(p->tilesWide - 1);
        }
    }

    // --- Y axis: identical wrap/clamp on scaledY/tilesHigh -------------------
    i32 wrapY = flags & 8;
    if (wrapY) {
        if (p->scaledY < 0.0f) {
            do {
                p->scaledY += (float)p->tilesHigh;
            } while (p->scaledY < 0.0f);
        }
        if (p->scaledY >= (float)p->tilesHigh) {
            float t = p->scaledY;
            do {
                t -= (float)p->tilesHigh;
            } while (t >= (float)p->tilesHigh);
            p->scaledY = t;
        }
    } else {
        if (p->scaledY < 0.0f) {
            p->scaledY = 0;
        } else if ((float)p->tilesHigh <= p->scaledY) {
            p->scaledY = (float)(p->tilesHigh - 1);
        }
    }

    // --- snap to integer + derive the tile origin ----------------------------
    i32 ix = (i32)p->scaledX;
    p->intX = ix;
    i32 iy = (i32)p->scaledY;
    p->intY = iy;

    i32 ox = ix - p->anchorX;
    p->originX = ox;
    if (ox < 0) {
        if (wrapX) {
            p->originX = p->tilesWide + ox;
        } else {
            p->originX = 0;
        }
    }

    i32 oy = iy - p->anchorY;
    p->originY = oy;
    if (oy < 0) {
        if (wrapY) {
            p->originY = p->tilesHigh + oy;
        } else {
            p->originY = 0;
        }
    }

    // --- derive the far tile extents (clamped, unless wrapping) ---------------
    i32 ex = p->viewW + p->originX - 1;
    i32 ey = p->viewH + p->originY - 1;
    p->extentX = ex;
    p->extentY = ey;
    if (ex >= p->tilesWide && wrapX == 0) {
        i32 over = ex - p->tilesWide + 1;
        p->extentX = ex - over;
        p->originX = p->originX - over;
    }
    if (ey >= p->tilesHigh && wrapY == 0) {
        i32 over = ey - p->tilesHigh + 1;
        p->extentY = ey - over;
        p->originY = p->originY - over;
    }
}

// ===========================================================================
// The trace-discovered CGameLevel cluster (13 leaves). All are plain /O2 /MT
// leaves touching only member offsets, the per-plane objects, and engine sibling
// callees that reloc-mask (no string/global relocations except the jump tables).
// ===========================================================================

// LevelPlane - a window onto the per-plane object stored in m_planes (CPlane*),
// viewed at the offsets this cluster touches. The named slot methods are UNMATCHED
// engine __thiscall leaves modeled with no body (their call sites reloc-mask):
//   Build(coords)  @0x161e80  - re-place + recompute one plane's coords
//   Sync(arg)      @0x162010  - per-plane visit helper
//   Refresh()      @0x163670  - per-plane refresh hook
// Fields: +0x08 flags, +0x20 tileBase, +0x24 rowOfs, +0x28 width, +0x2c height,
//   +0x74 limit, +0x80 cap, +0xb4 name[].
struct LevelPlane {
    char pad_0[0x08];
    u32 flags; // +0x08
    char pad_c[0x20 - 0x0c];
    i32* tileBase; // +0x20
    i32* rowOfs;   // +0x24
    i32 width;     // +0x28
    i32 height;    // +0x2c
    char pad_30[0x74 - 0x30];
    i32 limit; // +0x74
    char pad_78[0x80 - 0x78];
    i32 cap; // +0x80
    char pad_84[0xb4 - 0x84];
    char name[4]; // +0xb4

    void Build(LevelCoordRect* coords); // @0x161e80 (ret 4)
    void Sync(i32 arg);                 // @0x162010 (ret 4)
    void Refresh();                     // @0x163670 (ret)
};

// Three zero-arg __thiscall methods on the main plane the forwarders tail into.
struct MainPlane {
    i32 QueryA();  // @0x163300
    i32 QueryB();  // @0x163370
    void Notify(); // @0x163420
};

// __strcmpi (CRT) - reloc-masked. Declared with no header to keep the cdecl shape.
extern "C" i32 __cdecl _strcmpi(const char*, const char*);

// LevelScroll - the scroll-state record the edit-state methods manipulate: the
// +0x5c/+0x60 scroll x/y, +0x64/+0x68 limits, +0x08 flags and +0xe4 edit-state
// brush-kind all live on the same object that elsewhere is the level container.
// Accessed via raw offsets so the codegen is naming-independent (the same offsets
// are typed differently by the level-load methods).
struct LevelScroll {
    char pad_0[0x08];
    u32 flags; // +0x08
    char pad_c[0x5c - 0x0c];
    i32 scrollX; // +0x5c
    i32 scrollY; // +0x60
    i32 limitX;  // +0x64
    i32 limitY;  // +0x68
    char pad_6c[0xe4 - 0x6c];
    i32 editKind; // +0xe4
};

// The edit-state sub-dispatch for brush-kinds 1..2 is CGameLevel::ScrollKindDispatch12
// (@0x1671c0, __thiscall), reconstructed further below. ApplyScroll's call to it
// reloc-masks to the same address regardless of convention; modeling it as this
// __stdcall leaf gives ApplyScroll's surrounding code a closer byte match (94.78%)
// than the literal method-call form (92.61%) - see ApplyScroll's @early-stop note.
extern "C" i32 __stdcall EditSubDispatch12(LevelScroll* lvl, i32 a, i32 b, i32 c); // @0x1671c0

// ScrollTarget - the per-axis edit target the four brush handlers (EditHandlerA..D)
// drive. It is the same object EditSwitch hands them; the handlers read its scroll
// x/y at +0x5c/+0x60, the +0x08 flags, the +0x98 hold-anchor, the +0xe4 brush kind
// and the +0x138/+0x140 axis limits. The level itself is the handler's `this`; the
// target is the explicit first argument. Accessed via named fields here (a window
// onto the same offsets LevelScroll types differently for the simple path).
struct ScrollTarget {
    char pad_0[0x08];
    u32 flags; // +0x08  (bit4 = held)
    char pad_c[0x5c - 0x0c];
    i32 scrollX; // +0x5c
    i32 scrollY; // +0x60  (also the running axis-2 index)
    char pad_64[0x98 - 0x64];
    i32 holdAnchor; // +0x98
    char pad_9c[0xe4 - 0x9c];
    i32 editKind; // +0xe4
    char pad_e8[0x138 - 0xe8];
    i32 limitLo; // +0x138
    char pad_13c[0x140 - 0x13c];
    i32 limitHi; // +0x140
};

// The brush-handler sibling leaves (all CGameLevel __thiscall methods - this=level,
// the target/scroll passed explicitly - modeled with no body so the call sites
// reloc-mask). The names track the trace-discovered CGameLevel stub RVAs.
//   StepAxisLo  @0x15e720  step the target toward a lower coord (out via *outX)
//   StepAxisHi  @0x15e870  step the target toward a higher coord
//   AdvanceA    @0x15f1c0  advance the axis-2 cursor (4-arg)
//   ClampSpan   @0x15ffe0  clamp a [lo,hi] span (out via two pointers)
//   HoldMove    @0x15ff20  drive a held move (5-arg)
//   FreeMove    @0x15eb00  drive a free move (4-arg)
//   StepAxisAlt @0x15fdb0  alternate axis-2 stepper (5-arg, out via *outY)
//   AdvanceB    @0x15ede0  advance the axis-2 cursor variant (4-arg)
//   SpanCheck   @0x15f8d0  validate a span fits (4-arg, out via *out)
//   AxisProbe   @0x00161270  per-axis tile probe (returns a kind code; ==3 blocks)

// ===========================================================================
// The brush-handler sibling bodies (StepAxisLo/Hi, FreeMove, Advance{A,B},
// StepAxisAlt, SpanCheck, + the 15fe40 two-object span validator). All are plain
// /O2 /MT __thiscall leaves (this=this level), NO relocations: they touch only the
// edit target's +0x134..+0x140 axis brackets, its +0x5c/+0x60 scroll, +0xe4 brush
// kind, +0xf8/+0xfc per-axis steps, the level's main plane (+0x5c) tile grid, and
// the image-set array (+0x4c) - dispatching the image set's slot +0x20 to probe a
// tile, exactly like AxisProbe (@0x161270) inlined.
// ===========================================================================

// EditTarget - the explicit target arg the brush handlers drive. A window onto the
// edit-state object (the same offsets ScrollTarget names for the simple path, plus
// the +0x134/+0x13c axis-low brackets and the +0xf8/+0xfc per-axis step strides).
struct EditTarget {
    char pad_0[0x08];
    u32 flags; // +0x08
    char pad_c[0x5c - 0x0c];
    i32 scrollX; // +0x5c
    i32 scrollY; // +0x60
    char pad_64[0x98 - 0x64];
    i32 holdAnchor; // +0x98
    char pad_9c[0xe4 - 0x9c];
    i32 editKind; // +0xe4
    char pad_e8[0xf8 - 0xe8];
    i32 stepX; // +0xf8
    i32 stepY; // +0xfc
    char pad_100[0x134 - 0x100];
    i32 axisLoA; // +0x134
    i32 axisLoB; // +0x138
    i32 axisMid; // +0x13c
    i32 axisHi;  // +0x140
};

// ProbeObj - the candidate object AltStepValidate fits against (a game object hanging
// off the owner's chain). Its world-space bounding box is the +0x144/+0x14c tile span
// and +0x148 row, offset by the +0x5c/+0x60 origin; +0x178 is a clamp adjustment.
struct ProbeObj {
    char pad_0[0x5c];
    i32 m_5c; // +0x5c  origin X
    i32 m_60; // +0x60  origin Y
    char pad_64[0x144 - 0x64];
    i32 m_144; // +0x144  box left
    i32 m_148; // +0x148  box row
    i32 m_14c; // +0x14c  box right
    char pad_150[0x178 - 0x150];
    i32 m_178; // +0x178
};

// ProbePlane - the main plane the tile probe reads. Same object as CPlane/CPlaneRender,
// viewed at the probe offsets: the wrap moduli at +0x30/+0x34 (clamp bounds), the
// log2-tile shift amounts at +0x8c/+0x90, the column-offset table at +0x24 and the
// tile grid at +0x20.
struct ProbePlane {
    char pad_0[0x20];
    i32* tileGrid;   // +0x20
    i32* colOffsets; // +0x24
    char pad_28[0x30 - 0x28];
    i32 wrapW; // +0x30
    i32 wrapH; // +0x34
    char pad_38[0x8c - 0x38];
    i32 shiftX; // +0x8c
    i32 shiftY; // +0x90
};

// PROBE_TILE - the inlined per-coord tile probe (== AxisProbe @0x161270). Written as
// a do/while macro so each of the (up to four) copies in a single function schedules
// locally, exactly like the retail copy-paste (docs/patterns/x87-copypaste-vs-inline-fp-block.md).
// Clamps (X,Y) into the plane's tile grid, splits each into a tile index + sub-offset
// via the +0x8c/+0x90 shift, fetches the tile id, and (unless the empty/clear
// sentinel) dispatches the image set's slot +0x20 with the sub-offsets.
#define PROBE_TILE(LVL, X, Y, RESULT)                                                              \
    do {                                                                                           \
        i32 px_ = (X);                                                                             \
        i32 py_ = (Y);                                                                             \
        if (px_ < 0) {                                                                             \
            px_ = 0;                                                                               \
        } else {                                                                                   \
            ProbePlane* pc_ = (ProbePlane*)(LVL)->m_mainPlane;                                     \
            if (px_ >= pc_->wrapW) {                                                               \
                px_ = pc_->wrapW - 1;                                                              \
            }                                                                                      \
        }                                                                                          \
        if (py_ < 0) {                                                                             \
            py_ = 0;                                                                               \
        } else {                                                                                   \
            ProbePlane* pc_ = (ProbePlane*)(LVL)->m_mainPlane;                                     \
            if (py_ >= pc_->wrapH) {                                                               \
                py_ = pc_->wrapH - 1;                                                              \
            }                                                                                      \
        }                                                                                          \
        ProbePlane* pl_ = (ProbePlane*)(LVL)->m_mainPlane;                                         \
        i32 qx_ = px_ >> pl_->shiftX;                                                              \
        i32 qy_ = py_ >> pl_->shiftY;                                                              \
        i32 col_ = qx_;                                                                            \
        i32 subX_ = px_ - (qx_ << pl_->shiftX);                                                    \
        i32 idx_ = pl_->colOffsets[qy_] + col_;                                                    \
        i32 subY_ = py_ - (qy_ << pl_->shiftY);                                                    \
        i32 tile_ = pl_->tileGrid[idx_];                                                           \
        if (tile_ == TILE_UNINIT || tile_ == TILE_CLEAR) {                                         \
            (RESULT) = 0;                                                                          \
        } else {                                                                                   \
            CImageSet* set_ = (CImageSet*)m_imageSets[tile_ & 0xffff];                             \
            (RESULT) = set_->dummy8(subX_, subY_);                                                 \
        }                                                                                          \
    } while (0)

// AxisProbe - 0x161270 (__thiscall, ret 8). The standalone tile probe the editor
// loops call as a reloc-masked leaf: clamp (coord, limit) into the main plane's tile
// grid, split each into tile index + sub-offset, fetch the tile id, and (unless the
// empty/clear sentinel) dispatch the image set's slot +0x20 with the sub-offsets;
// returns the tile kind (callers gate on == 3). This IS the body the PROBE_TILE
// macro inlines elsewhere.
RVA(0x00161270, 0xb2)
i32 CGameLevel::AxisProbe(i32 coord, i32 limit) {
    // Same shape as PROBE_TILE, but the standalone reads the second coord only AFTER
    // clamping the first (retail defers the edi load past the X-clamp block).
    i32 px = coord;
    if (px < 0) {
        px = 0;
    } else {
        ProbePlane* pc = (ProbePlane*)m_mainPlane;
        if (px >= pc->wrapW) {
            px = pc->wrapW - 1;
        }
    }
    i32 py = limit;
    if (py < 0) {
        py = 0;
    } else {
        ProbePlane* pc = (ProbePlane*)m_mainPlane;
        if (py >= pc->wrapH) {
            py = pc->wrapH - 1;
        }
    }
    ProbePlane* pl = (ProbePlane*)m_mainPlane;
    i32 qx = px >> pl->shiftX;
    i32 qy = py >> pl->shiftY;
    i32 col = qx;
    i32 subX = px - (qx << pl->shiftX);
    i32 idx = pl->colOffsets[qy] + col;
    i32 subY = py - (qy << pl->shiftY);
    i32 tile = pl->tileGrid[idx];
    if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
        return 0;
    }
    CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
    return set->dummy8(subX, subY);
}

// EditSink - the serializer `arg0` of EditDispatch: a polymorphic object whose slots
// +0x2c (read a name into buf) and +0x30 (write buf as a name) are used.
struct EditSink {
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void GetName(char* buf, i32 cap); // +0x2c
    virtual void SetName(char* buf, i32 cap); // +0x30
};

// Resolve a level/name to a tile id (returns nonzero on success). __stdcall. @0x163710
extern i32 __stdcall ResolveLevelName(void* sink, i32 a, i32 b, i32 c);

// ---------------------------------------------------------------------------
// PointInBounds (free cdecl): tile (x, y) inside the [minX,maxX) x [minY,maxY)
// half-open box (LevelCoordRect minX/minY/maxX/maxY at +0/+4/+8/+0xc). ret.
RVA(0x0006b330, 0x2a)
i32 CGameLevel::PointInBounds(const LevelCoordRect* r, i32 x, i32 y) {
    if (x < r->maxX && x >= r->minX && y < r->maxY && y >= r->minY) {
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// LookupTile: clamp (x, y) into the main plane's tile grid, fetch the tile id from
// its row-indexed tile map, and (when not the empty/clear sentinel) dispatch the
// referenced image set's slot +0x20 with (0, 0). ret 8.
RVA(0x00082600, 0x73)
i32 CGameLevel::LookupTile(i32 x, i32 y) {
    LevelPlane* mp;
    if (x < 0) {
        x = 0;
    } else {
        mp = (LevelPlane*)m_mainPlane;
        if (x >= mp->width) {
            x = mp->width - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        mp = (LevelPlane*)m_mainPlane;
        if (y >= mp->height) {
            y = mp->height - 1;
        }
    }
    mp = (LevelPlane*)m_mainPlane;
    i32 tile = mp->tileBase[mp->rowOfs[y] + x];
    if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
        return 0;
    }
    CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
    return set->dummy8(0, 0); // slot +0x20, called with (0, 0)
}

// ---------------------------------------------------------------------------
// Three forwarders to a method on the main plane; no-op / 0 when none.
RVA(0x000cedf0, 0xf)
i32 CGameLevel::MainPlaneQueryA() {
    if (m_mainPlane != 0) {
        return ((MainPlane*)m_mainPlane)->QueryA();
    }
    return 0;
}

RVA(0x000cee10, 0xf)
i32 CGameLevel::MainPlaneQueryB() {
    if (m_mainPlane != 0) {
        return ((MainPlane*)m_mainPlane)->QueryB();
    }
    return 0;
}

RVA(0x00160ee0, 0xd)
void CGameLevel::MainPlaneNotify() {
    if (m_mainPlane != 0) {
        ((MainPlane*)m_mainPlane)->Notify();
    }
}

// ---------------------------------------------------------------------------
// BuildAllPlanes: copy *coords into m_planeCtx, then Build(coords) on every plane.
RVA(0x0015da80, 0x47)
void CGameLevel::BuildAllPlanes(LevelCoordRect* coords) {
    m_planeCtx = *coords;
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        ((LevelPlane*)m_planes[i])->Build(coords);
    }
}

// ---------------------------------------------------------------------------
// SetExtentsAndBuildAll: when both w and h are positive, build the half-open box
// {0, 0, w-1, h-1} into BOTH m_planeCtx and a local LevelCoordRect, then drive
// Build(&local) on every plane. Returns 1 (0 if either extent is non-positive).
//
// @early-stop
// regalloc/zero-pin wall (~70%): logic + offsets + CFG + the Build dispatch are
// exact. Residue is the prologue register allocation - retail loads w(arg0) into
// edx as the very first insn and `test edx,edx`/`test edi,edi` the guards, pins
// &m_planeCtx in a 4th saved reg (ebx) and keeps w-1/h-1 live in the guard regs
// for the interleaved member+local stores; our cl uses 3 saved regs, derives the
// guard zero via `xor;cmp` (zero-register-pinning) and writes m_planeCtx directly
// through [esi+0x10+N]. docs/patterns/zero-register-pinning.md +
// pin-local-for-callee-saved-reg.md. Not steerable on a function this small;
// deferred to the final sweep.
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
    m_planeCtx.minX = 0;
    rect.minX = 0;
    rect.maxY = maxY;
    m_planeCtx.minY = 0;
    rect.minY = 0;
    rect.maxX = maxX;
    m_planeCtx.maxX = maxX;
    m_planeCtx.maxY = maxY;
    i32 i = 0;
    if (m_planes.GetSize() > 0) {
        do {
            ((LevelPlane*)m_planes.GetData()[i])->Build(&rect);
            ++i;
        } while (i < m_planes.GetSize());
    }
    return 1;
}

// ---------------------------------------------------------------------------
// SyncToMainIndex: Sync(arg) on every plane from index 0 through m_mainIndex
// inclusive (nothing when there is no main plane). The first half of the
// non-origin-fixed VisitVisible path, lifted as its own helper.
RVA(0x0015dad0, 0x2c)
void CGameLevel::SyncToMainIndex(i32 arg) {
    i32 i = 0;
    if (m_mainIndex >= 0) {
        do {
            ((LevelPlane*)m_planes.GetData()[i])->Sync(arg);
            ++i;
        } while (i <= m_mainIndex);
    }
}

// ---------------------------------------------------------------------------
// SyncAfterMainIndex: Sync(arg) on every plane after the main index up to the
// plane count. The second half of the non-origin-fixed VisitVisible path.
RVA(0x0015db00, 0x2e)
void CGameLevel::SyncAfterMainIndex(i32 arg) {
    i32 i = m_mainIndex + 1;
    if (i < m_planes.GetSize()) {
        do {
            ((LevelPlane*)m_planes.GetData()[i])->Sync(arg);
            ++i;
        } while (i < m_planes.GetSize());
    }
}

// ---------------------------------------------------------------------------
// FindPlaneByName: case-insensitive search for the plane named `name`; null if none.
// ---------------------------------------------------------------------------
// ClampScroll: drive EditSwitch toward (arg1, arg2) on `target`, never moving more
// than this level's per-axis step limits (m_scrollStepX/m_scrollStepY) at once. A direct call when
// the move is within limits or forced by the target flags / kind 7; otherwise an
// incremental stepping loop that re-runs EditSwitch until it reaches the goal or is
// reported blocked.
//
// @early-stop
// scheduling/spill wall: the stepping loop's six spilled locals (the per-axis step,
// the running goal, the ok flag) and the abs() lowering schedule into a register
// assignment MSVC reproduces only for one spill order; logic + offsets + CFG are
// exact. Deferred to the final sweep.
RVA(0x0015de40, 0x164)
i32 CGameLevel::ClampScroll(void* target, i32 arg1, i32 arg2, i32 arg3) {
    LevelScroll* t = (LevelScroll*)target;
    i32 limX = m_scrollStepX;
    i32 limY = m_scrollStepY;

    i32 dx = t->scrollX - arg1;
    if (dx < 0) {
        dx = -dx;
    }
    if (dx <= limX) {
        i32 dy = t->scrollY - arg2;
        if (dy < 0) {
            dy = -dy;
        }
        if (dy <= m_scrollStepY) {
            return EditSwitch(target, arg1, arg2, arg3);
        }
    }

    if (t->flags & 0x10) {
        return EditSwitch(target, arg1, arg2, arg3);
    }

    i32 kind = t->editKind;
    if (kind == 7) {
        return EditSwitch(target, arg1, arg2, arg3);
    }

    // --- incremental stepping toward (arg1, arg2) ---------------------------
    i32 stepX = limX;
    i32 goalX = arg1;
    if (t->scrollX > arg1) {
        stepX = -stepX;
    }
    i32 stepY = limY;
    if (t->scrollY > arg2) {
        stepY = -stepY;
    }

    i32 ok = 1;
    do {
        i32 nx = stepX + t->scrollX;
        if (stepX > 0) {
            if (nx > goalX) {
                nx = goalX;
            }
        } else {
            if (nx < goalX) {
                nx = goalX;
            }
        }
        i32 ny = stepY + t->scrollY;
        if (stepY > 0) {
            if (ny > arg2) {
                ny = arg2;
            }
        } else {
            if (ny < arg2) {
                ny = arg2;
            }
        }

        i32 flags = EditSwitch(target, nx, ny, arg3);

        ok = 1;
        if (t->editKind == kind && (flags & 0x10000) == 0) {
            if (t->scrollX == goalX && t->scrollY == arg2) {
                ok = 0;
            } else if ((flags & 0x400000) == 0) {
                ok = 0;
            }
        }
    } while (ok != 0);
    return ok;
}

RVA(0x0015dde0, 0x5c)
CPlane* CGameLevel::FindPlaneByName(const char* name) {
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        LevelPlane* p = (i >= 0 && i < m_planes.GetSize()) ? (LevelPlane*)m_planes[i] : 0;
        if (_strcmpi(name, p->name) == 0) {
            return (CPlane*)p;
        }
    }
    return 0;
}

// VisitCtx - the `arg1` of VisitVisible: a polymorphic context whose object chain
// hangs off +0x14 (the node list) and whose +0x28 hook is dispatched in the
// non-origin-fixed path. ObjNode - one node in that chain (next@+0, payload@+8);
// the payload object carries a depth key at +0x74 and a +0x2c draw hook.
struct ObjPayload {
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28(i32 arg);  // +0x28
    virtual void Draw(i32 arg); // +0x2c
    char pad_4[0x74 - 0x04];
    i32 depth; // +0x74
};
struct ObjNode {
    ObjNode* next;    // +0x00
    char pad_4[0x04]; // +0x04
    ObjPayload* obj;  // +0x08
};
// VisitCtx - the `ctx` (2nd param) of VisitVisible. Its object chain hangs off a
// sub-record at +0x10 whose +0x04 field is the head node; the engine takes the
// ADDRESS of the +0x10 record (lea), null-checks it (always live), then loads the
// head. The +0x28 hook is dispatched in the non-origin-fixed path.
struct VisitChain {
    char pad_0[0x04];
    ObjNode* head; // +0x04 (i.e. VisitCtx+0x14)
};
struct VisitCtx {
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void Hook(i32 arg); // +0x28
    char pad_4[0x10 - 0x04];
    VisitChain m_chain; // +0x10  (head at +0x14)
};

// ---------------------------------------------------------------------------
// VisitVisible: depth-ordered object visit. When the level is origin-fixed
// (m_flags & 1) walk ctx's object chain, draw each object whose depth is below the
// running plane's cap, Sync the planes around it; otherwise Sync every plane (from
// the main index) and dispatch ctx's +0x28 hook. `visitor` (1st param) is the arg
// every Sync/Draw/Hook receives; `ctx` (2nd param) owns the chain.
//
// @early-stop
// register-scheduling wall (~92%): the inner draw-gate branch polarity now matches
// retail (`if (depth < cap) Draw; else block` -> jge block, Draw fall-through). Residue
// is the chain cursor's saved-reg shuffle - retail keeps a 2nd copy of `cur` in edx and
// reloads `cap` from spill each iteration; cl keeps cap live in edx and restores via the
// surviving eax. Logic + offsets + CFG exact; allocator coin-flip. Deferred to the final sweep.
RVA(0x0015dc90, 0x141)
void CGameLevel::VisitVisible(void* visitor, i32 ctx) {
    VisitCtx* c = (VisitCtx*)ctx;
    VisitChain* chain = &c->m_chain;

    if ((m_flags & 1) && chain != 0 && (m_planes.GetSize() > 0 ? m_planes.GetData()[0] : 0) != 0) {
        ((LevelPlane*)(m_planes.GetSize() > 0 ? m_planes.GetData()[0] : 0))->Sync((i32)visitor);
        ObjNode* node = chain->head;

        i32 i = 1;
        if (m_planes.GetSize() > i) {
            do {
                LevelPlane* p =
                    (i >= 0 && i < m_planes.GetSize()) ? (LevelPlane*)m_planes.GetData()[i] : 0;
                i32 cap = p->cap;
                i32 blocked = 0;
                while (node != 0 && blocked == 0) {
                    ObjNode* cur = node;
                    node = node->next;
                    ObjPayload* pl = cur->obj;
                    if (pl->depth < cap) {
                        pl->Draw((i32)visitor);
                    } else {
                        node = cur;
                        blocked = 1;
                    }
                }
                ((LevelPlane*)m_planes.GetData()[i])->Sync((i32)visitor);
                ++i;
            } while (i < m_planes.GetSize());
        }

        while (node != 0) {
            ObjNode* cur = node;
            node = node->next;
            cur->obj->Draw((i32)visitor);
        }
        return;
    }

    // --- not origin-fixed: Sync planes around the main index + the ctx hook ---
    i32 idx = 0;
    if (m_mainIndex >= 0) {
        do {
            ((LevelPlane*)m_planes.GetData()[idx])->Sync((i32)visitor);
            ++idx;
        } while (idx <= m_mainIndex);
    }
    c->Hook((i32)visitor);
    i32 j = m_mainIndex + 1;
    if (j < m_planes.GetSize()) {
        do {
            ((LevelPlane*)m_planes.GetData()[j])->Sync((i32)visitor);
            ++j;
        } while (j < m_planes.GetSize());
    }
}

// ---------------------------------------------------------------------------
// NotifyAllPlanes: Refresh() across every plane.
RVA(0x00160f40, 0x23)
void CGameLevel::NotifyAllPlanes() {
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        ((LevelPlane*)m_planes[i])->Refresh();
    }
}

// ---------------------------------------------------------------------------
// ApplyScroll: drive the +0xe4 edit-state machine. editKind <= 0: nothing; kind 7
// commits the new scroll x/y directly; kinds 1..2 fan to ScrollKindDispatch12. Then
// fold flag bits into the result and tag 0x400000 when the scroll did not move.
//
// @early-stop
// call-arg-materialization entropy (~95%): logic/offsets/CFG/__stdcall conv are exact;
// the only residue is the ScrollKindDispatch12 call setup - retail interleaves a reload
// between the arg pushes (2 regs), MSVC pre-loads all three (eax/ecx/edx). See
// docs/patterns/pin-local-for-callee-saved-reg.md. Entropy tail; deferred.
RVA(0x00167130, 0x83)
i32 __stdcall ApplyScroll(CGameLevel* lvl, i32 a, i32 b, i32 c) {
    LevelScroll* s = (LevelScroll*)lvl;
    i32 eax = 0;
    i32 prevX = s->scrollX;
    i32 prevY = s->scrollY;
    i32 kind = s->editKind;

    if (kind > 0) {
        if (kind > 2) {
            if (kind == 7) {
                s->scrollX = a;
                s->scrollY = b;
            }
        } else {
            eax = EditSubDispatch12(s, a, b, c);
        }
    }

    if (eax & 0x20000) {
        eax |= 0x10000;
    }
    u32 f = s->flags;
    if (f & 0x400000) {
        eax |= 0x100000;
    }
    if (f & 0x10) {
        eax |= 0x200000;
    }
    if (s->scrollX == prevX && s->scrollY == prevY) {
        eax |= 0x400000;
    }
    return eax;
}

// ---------------------------------------------------------------------------
// ScrollKindDispatch12 (@0x1671c0): drive both axes toward (x,y). For the X axis,
// if x is above/below the target's current scrollX call the matching hi/lo stepper
// (which clamps x in place through &x); same for Y; OR the two results. Finally
// commit the (possibly stepped) scroll x/y back into the target and return the
// accumulated flag word. this=level, target passed explicitly (it is itself a level).
RVA(0x001671c0, 0x97)
i32 CGameLevel::ScrollKindDispatch12(ScrollTarget* t, i32 x, i32 y, i32 flags) {
    i32 result = 0;
    i32 curX = t->scrollX;
    if (x > curX) {
        result = ScrollStepXHi(t, x, y, &x, flags);
    } else if (x < curX) {
        result = ScrollStepXLo(t, x, y, &x, flags);
    }
    i32 curY = t->scrollY;
    if (y > curY) {
        result |= ScrollStepYHi(t, x, y, &y, flags);
    } else if (y < curY) {
        result |= ScrollStepYLo(t, x, y, &y, flags);
    }
    t->scrollX = x;
    t->scrollY = y;
    return result;
}

// ===========================================================================
// The four axis steppers (ScrollStepXHi/XLo/YHi/YLo @0x167260/450/640/830).
// All __thiscall (this=level), ret 0x14. Each sweeps the OTHER axis across a tile
// region [t->axis brackets], inlining the per-tile probe (== AxisProbe). When a
// probed tile reports a blocking kind (1/2), it scans the resolved axis inward via
// AxisProbe to find the first clear coord, accumulating a state-flag word. Finally
// it tails into BroadPhase to test moving objects, writing the resolved scroll coord
// back through the out-pointer and returning the state word.
//   X-variants resolve X (out = &x, scroll +0x5c, step +0xfc, outer Y over +138..+140);
//   Y-variants resolve Y (out = &y, scroll +0x60, step +0xf8, outer X over +134..+13c).
//   Hi-variants fix the high edge (+0x13c / +0x140) and scan down; Lo-variants fix the
//   low edge (+0x134 / +0x138) and scan up. Field names via EditTarget (file-scope).
// ===========================================================================

// ScrollStepXHi - 0x167260. Fixed X = high edge (x + axisMid); sweep Y, scan X down.
// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167260, 0x1ef)
i32 CGameLevel::ScrollStepXHi(ScrollTarget* tp, i32 x, i32 y, i32* px, i32 flags) {
    EditTarget* t = (EditTarget*)tp;
    i32 xEnd = x + t->axisMid;
    i32 yHi = t->axisHi + y;
    i32 yLo = t->axisLoB + y;
    i32 state = 0;
    if (yLo > yHi) {
        goto helper;
    }
looptop: {
    i32 result;
    {
        i32 cx = xEnd;
        if (cx < 0) {
            cx = 0;
        } else {
            ProbePlane* pc = (ProbePlane*)m_mainPlane;
            if (cx >= pc->wrapW) {
                cx = pc->wrapW - 1;
            }
        }
        i32 cy = yLo;
        if (cy < 0) {
            cy = 0;
        } else {
            ProbePlane* pc = (ProbePlane*)m_mainPlane;
            if (cy >= pc->wrapH) {
                cy = pc->wrapH - 1;
            }
        }
        ProbePlane* pl = (ProbePlane*)m_mainPlane;
        i32 qx = cx >> pl->shiftX;
        i32 qy = cy >> pl->shiftY;
        i32 col = qx;
        i32 subX = cx - (qx << pl->shiftX);
        i32 idx = pl->colOffsets[qy] + col;
        i32 subY = cy - (qy << pl->shiftY);
        i32 tile = pl->tileGrid[idx];
        if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
            result = 0;
        } else {
            CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
            result = set->dummy8(subX, subY);
        }
    }
    if (result == 2 && (t->flags & 0x400)) {
        result = 0;
    }
    if (result == 1 || result == 2) {
        i32 lo = t->scrollX + t->axisMid;
        i32 j = xEnd - 1;
        state |= 0x60000;
        for (; j > lo; j--) {
            if (AxisProbe(j, yLo) == 0) {
                j -= t->axisMid;
                goto have_x;
            }
        }
        j = t->scrollX;
    have_x:
        x = j;
        if (j == t->scrollX) {
            goto done_eq;
        }
    }
    if (yLo == yHi) {
        yLo++;
    } else {
        yLo += t->stepY;
        if (yLo <= yHi) {
            goto looptop;
        }
        yLo = yHi;
    }
    if (yLo <= yHi) {
        goto looptop;
    }
}
helper:
    if (BroadPhase(tp, x, y) != 0) {
        *px = t->scrollX;
        return state | 0x22000000;
    }
    *px = x;
    return state;
done_eq:
    *px = t->scrollX;
    return state;
}

// ScrollStepXLo - 0x167450. Fixed X = low edge (x + axisLoA); sweep Y, scan X up.
// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167450, 0x1ef)
i32 CGameLevel::ScrollStepXLo(ScrollTarget* tp, i32 x, i32 y, i32* px, i32 flags) {
    EditTarget* t = (EditTarget*)tp;
    i32 xEnd = x + t->axisLoA;
    i32 yHi = t->axisHi + y;
    i32 yLo = t->axisLoB + y;
    i32 state = 0;
    if (yLo > yHi) {
        goto helper;
    }
looptop: {
    i32 result;
    {
        i32 cx = xEnd;
        if (cx < 0) {
            cx = 0;
        } else {
            ProbePlane* pc = (ProbePlane*)m_mainPlane;
            if (cx >= pc->wrapW) {
                cx = pc->wrapW - 1;
            }
        }
        i32 cy = yLo;
        if (cy < 0) {
            cy = 0;
        } else {
            ProbePlane* pc = (ProbePlane*)m_mainPlane;
            if (cy >= pc->wrapH) {
                cy = pc->wrapH - 1;
            }
        }
        ProbePlane* pl = (ProbePlane*)m_mainPlane;
        i32 qx = cx >> pl->shiftX;
        i32 qy = cy >> pl->shiftY;
        i32 col = qx;
        i32 subX = cx - (qx << pl->shiftX);
        i32 idx = pl->colOffsets[qy] + col;
        i32 subY = cy - (qy << pl->shiftY);
        i32 tile = pl->tileGrid[idx];
        if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
            result = 0;
        } else {
            CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
            result = set->dummy8(subX, subY);
        }
    }
    if (result == 2 && (t->flags & 0x400)) {
        result = 0;
    }
    if (result == 1 || result == 2) {
        i32 lo = t->scrollX + t->axisLoA;
        i32 j = xEnd + 1;
        state |= 0xa0000;
        for (; j < lo; j++) {
            if (AxisProbe(j, yLo) == 0) {
                j -= t->axisLoA;
                goto have_x;
            }
        }
        j = t->scrollX;
    have_x:
        x = j;
        if (j == t->scrollX) {
            goto done_eq;
        }
    }
    if (yLo == yHi) {
        yLo++;
    } else {
        yLo += t->stepY;
        if (yLo <= yHi) {
            goto looptop;
        }
        yLo = yHi;
    }
    if (yLo <= yHi) {
        goto looptop;
    }
}
helper:
    if (BroadPhase(tp, x, y) != 0) {
        *px = t->scrollX;
        return state | 0x82000000;
    }
    *px = x;
    return state;
done_eq:
    *px = t->scrollX;
    return state;
}

// ScrollStepYHi - 0x167640. Fixed Y = high edge (y + axisHi); sweep X, scan Y down.
// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167640, 0x1eb)
i32 CGameLevel::ScrollStepYHi(ScrollTarget* tp, i32 x, i32 y, i32* py, i32 flags) {
    EditTarget* t = (EditTarget*)tp;
    i32 colHi = t->axisMid + x;
    i32 fixedY = y + t->axisHi;
    i32 col = t->axisLoA + x;
    i32 state = 0;
    if (col > colHi) {
        goto helper;
    }
looptop: {
    i32 result;
    {
        i32 cx = col;
        if (cx < 0) {
            cx = 0;
        } else {
            ProbePlane* pc = (ProbePlane*)m_mainPlane;
            if (cx >= pc->wrapW) {
                cx = pc->wrapW - 1;
            }
        }
        i32 cy = fixedY;
        if (cy < 0) {
            cy = 0;
        } else {
            ProbePlane* pc = (ProbePlane*)m_mainPlane;
            if (cy >= pc->wrapH) {
                cy = pc->wrapH - 1;
            }
        }
        ProbePlane* pl = (ProbePlane*)m_mainPlane;
        i32 qx = cx >> pl->shiftX;
        i32 qy = cy >> pl->shiftY;
        i32 c = qx;
        i32 subX = cx - (qx << pl->shiftX);
        i32 idx = pl->colOffsets[qy] + c;
        i32 subY = cy - (qy << pl->shiftY);
        i32 tile = pl->tileGrid[idx];
        if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
            result = 0;
        } else {
            CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
            result = set->dummy8(subX, subY);
        }
    }
    if (result == 2 && (t->flags & 0x400)) {
        result = 0;
    }
    if (result == 1 || result == 2) {
        i32 lo = t->scrollY + t->axisHi;
        i32 j = fixedY - 1;
        state |= 0x1020000;
        for (; j > lo; j--) {
            if (AxisProbe(col, j) == 0) {
                j -= t->axisHi;
                goto have_y;
            }
        }
        j = t->scrollY;
    have_y:
        y = j;
        if (j == t->scrollY) {
            goto done_eq;
        }
    }
    if (col == colHi) {
        col++;
    } else {
        col += t->stepX;
        if (col <= colHi) {
            goto looptop;
        }
        col = colHi;
    }
    if (col <= colHi) {
        goto looptop;
    }
}
helper:
    if (BroadPhase(tp, x, y) != 0) {
        *py = t->scrollY;
        return state | 0x42000000;
    }
    *py = y;
    return state;
done_eq:
    *py = t->scrollY;
    return state;
}

// ScrollStepYLo - 0x167830. Fixed Y = low edge (y + axisLoB); sweep X, scan Y up.
// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167830, 0x1eb)
i32 CGameLevel::ScrollStepYLo(ScrollTarget* tp, i32 x, i32 y, i32* py, i32 flags) {
    EditTarget* t = (EditTarget*)tp;
    i32 colHi = t->axisMid + x;
    i32 fixedY = y + t->axisLoB;
    i32 col = t->axisLoA + x;
    i32 state = 0;
    if (col > colHi) {
        goto helper;
    }
looptop: {
    i32 result;
    {
        i32 cx = col;
        if (cx < 0) {
            cx = 0;
        } else {
            ProbePlane* pc = (ProbePlane*)m_mainPlane;
            if (cx >= pc->wrapW) {
                cx = pc->wrapW - 1;
            }
        }
        i32 cy = fixedY;
        if (cy < 0) {
            cy = 0;
        } else {
            ProbePlane* pc = (ProbePlane*)m_mainPlane;
            if (cy >= pc->wrapH) {
                cy = pc->wrapH - 1;
            }
        }
        ProbePlane* pl = (ProbePlane*)m_mainPlane;
        i32 qx = cx >> pl->shiftX;
        i32 qy = cy >> pl->shiftY;
        i32 c = qx;
        i32 subX = cx - (qx << pl->shiftX);
        i32 idx = pl->colOffsets[qy] + c;
        i32 subY = cy - (qy << pl->shiftY);
        i32 tile = pl->tileGrid[idx];
        if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
            result = 0;
        } else {
            CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
            result = set->dummy8(subX, subY);
        }
    }
    if (result == 2 && (t->flags & 0x400)) {
        result = 0;
    }
    if (result == 1 || result == 2) {
        i32 lo = t->scrollY + t->axisLoB;
        i32 j = fixedY + 1;
        state |= 0x820000;
        for (; j < lo; j++) {
            if (AxisProbe(col, j) == 0) {
                j -= t->axisLoB;
                goto have_y;
            }
        }
        j = t->scrollY;
    have_y:
        y = j;
        if (j == t->scrollY) {
            goto done_eq;
        }
    }
    if (col == colHi) {
        col++;
    } else {
        col += t->stepX;
        if (col <= colHi) {
            goto looptop;
        }
        col = colHi;
    }
    if (col <= colHi) {
        goto looptop;
    }
}
helper:
    if (BroadPhase(tp, x, y) != 0) {
        *py = t->scrollY;
        return state | 0x12000000;
    }
    *py = y;
    return state;
done_eq:
    *py = t->scrollY;
    return state;
}

// ===========================================================================
// BroadPhase - 0x167ea0 (__thiscall, ret 0xc). The AABB broad-phase the steppers
// tail into. `t` is the moving box (an EditTarget viewed as a BPObj); it walks the
// owner's object chain and, for every other active object whose collision masks
// intersect and whose box is set (+0x134 != sentinel), tests whether t currently
// overlaps it. If NOT (a separation on any axis) but t's CANDIDATE box (at candX,
// candY) WOULD overlap, it fires t's +0x90 notifier; on a nonzero reply it fires the
// object's own +0x90 notifier (when the masks still intersect) and returns 1. 0 if
// no object triggers.
// ===========================================================================

// A game object in the broad-phase chain. The brackets at +0x134..+0x140 are the
// object-local AABB (added to the +0x5c/+0x60 world origin); +0x90 is the notifier
// dispatcher (its +0x10 slot is the callback); +0xe8/+0xf4 the collision masks.
struct BPNotifier {
    char pad_0[0x10];
    i32 (*notify)(void* obj); // +0x10
};
struct BPObj {
    char pad_0[0x08];
    u32 flags; // +0x08  (bit 0x100 = active)
    char pad_c[0x5c - 0x0c];
    i32 originX; // +0x5c
    i32 originY; // +0x60
    char pad_64[0x90 - 0x64];
    BPNotifier* notifier; // +0x90
    BPObj* backPtr;       // +0x94
    char pad_98[0xe8 - 0x98];
    u32 maskA; // +0xe8
    char pad_ec[0xf4 - 0xec];
    u32 maskB; // +0xf4
    char pad_f8[0x134 - 0xf8];
    i32 boxL; // +0x134
    i32 boxB; // +0x138
    i32 boxR; // +0x13c
    i32 boxT; // +0x140
};

// The owner's object chain: this->m_owner (+0xc) -> +0x8 (chain mgr) -> +0x14 (head).
// Each node holds the next link at +0x00 and the object pointer at +0x08.
struct BPNode {
    BPNode* next; // +0x00
    char pad_4[0x08 - 0x04];
    BPObj* obj; // +0x08
};
struct BPChainMgr {
    char pad_0[0x14];
    BPNode* head; // +0x14
};
struct BPOwner {
    char pad_0[0x08];
    BPChainMgr* mgr; // +0x08
};

// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167ea0, 0x1b9)
i32 CGameLevel::BroadPhase(ScrollTarget* tp, i32 candX, i32 candY) {
    BPObj* t = (BPObj*)tp;
    if (!(t->flags & 0x100)) {
        return 0;
    }
    BPNode* node = ((BPOwner*)m_owner)->mgr->head;
    if (node == 0) {
        return 0;
    }
    do {
        BPNode* nx = node->next;
        BPObj* obj = node->obj;
        if (obj != t && (obj->flags & 0x100) && (t->maskB & obj->maskA)
            && t->boxL != (i32)0x80000000 && obj->boxL != (i32)0x80000000) {
            i32 tLeft = t->boxL + t->originX;
            i32 tBot = t->boxB + t->originY;
            i32 tRight = t->originX + t->boxR;
            i32 tTop = t->boxT + t->originY;
            i32 oLeft = obj->originX + obj->boxL;
            i32 oBot = obj->boxB + obj->originY;
            i32 oTop = obj->originY + obj->boxT;
            i32 oRight = obj->originX + obj->boxR;
            if (tLeft > oRight || tRight < oLeft || tBot > oTop || tTop < oBot) {
                i32 cLeft = candX + t->boxL;
                i32 cRight = t->boxR + candX;
                i32 cBot = t->boxB + candY;
                i32 cTop = t->boxT + candY;
                if (cLeft <= oRight && cRight >= oLeft && cBot <= oTop && cTop >= oBot) {
                    i32 fire;
                    if (t->notifier != 0) {
                        t->backPtr = obj;
                        fire = t->notifier->notify(t);
                    } else {
                        fire = 1;
                    }
                    if (fire != 0) {
                        if (t->maskB & obj->maskA) {
                            if (obj->notifier != 0) {
                                obj->backPtr = t;
                                obj->notifier->notify(obj);
                            }
                        }
                        return 1;
                    }
                }
            }
        }
        node = nx;
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// EditDispatch: case 4 pushes this level's name into the sink; case 7 pulls a name
// from the sink into this level; then (if a main plane exists) resolve the name to
// a tile id and return 1/0. ret 0x10.
//
// @early-stop
// switch jump-table-density wall (~48%): retail lowers the arg1 switch to a dense
// .rdata jump table over kinds 3..8 (jmp [eax*4+tbl]); MSVC sees only 2 non-empty
// cases (4 & 7) and emits a cmp/subtract chain instead. The strcpy/name-copy case
// bodies are byte-exact. Not steerable from source (case-value density decides the
// lowering) - see docs/patterns/switch-cmpje-tree-vs-jumptable.md. Deferred.
RVA(0x00160f70, 0xfa)
i32 CGameLevel::EditDispatch(void* sink, i32 arg1, i32 arg2, i32 arg3) {
    EditSink* s = (EditSink*)sink;
    if (s == 0) {
        return 0;
    }

    char buf[0x80];
    switch (arg1) {
        case 4:
            memset(buf, 0, sizeof(buf));
            strcpy(buf, m_levelName);
            s->SetName(buf, 0x80);
            break;
        case 7:
            s->GetName(buf, 0x80);
            strcpy(m_levelName, buf);
            break;
    }

    if (m_mainPlane == 0) {
        return 0;
    }
    return ResolveLevelName(sink, arg2, arg2, arg3) != 0 ? 1 : 0;
}

// ---------------------------------------------------------------------------
// EditSwitch: when this->flags & 4, tail into ApplyScroll on `target`; else run
// `target`'s +0xe4 brush-kind switch (kinds 1..8) and fold the flag bits into the
// returned state word, tagging 0x400000 when the scroll did not move.
//
// @early-stop
// call-arg-materialization entropy (~79%): the dense kinds-1..8 jump table, every
// case body, the flag-folding tail and the __stdcall sub-handler convention are
// exact; the residue is the recurring call setup (forward-to-ApplyScroll + 5
// EditHandler sites) where retail interleaves an arg reload between pushes (2 regs)
// and MSVC pre-loads (3 regs). docs/patterns/pin-local-for-callee-saved-reg.md.
// Logic/offsets/CFG exact; deferred to the final sweep.
RVA(0x0015dfb0, 0x15b)
i32 CGameLevel::EditSwitch(void* target, i32 a1, i32 a2, i32 a3) {
    if (m_flags & 4) {
        return ApplyScroll((CGameLevel*)target, a1, a2, a3);
    }

    LevelScroll* s = (LevelScroll*)target;
    i32 eax = 0;
    i32 kind = s->editKind;
    i32 prevX = s->scrollX;
    i32 prevY = s->scrollY;

    switch (kind) {
        case 1:
        case 2:
        case 5:
            eax = EditHandlerA(s, a1, a2, a3);
            break;
        case 3:
            eax = EditHandlerB(s, a1, a2, a3);
            if (s->editKind == 4) {
                eax |= 0x800000;
            }
            break;
        case 4:
            eax = EditHandlerC(s, a1, a2, a3);
            if (s->editKind == 1) {
                eax |= 0x1000000;
            }
            break;
        case 8:
            if (a2 < prevY) {
                eax = EditHandlerB(s, a1, a2, a3);
                if (s->editKind == 4) {
                    eax |= 0x800000;
                    s->editKind = 8;
                }
            } else {
                eax = EditHandlerC(s, a1, a2, a3);
                if (s->editKind == 1) {
                    eax |= 0x1000000;
                }
            }
            break;
        case 6:
            eax = EditHandlerD(s, a1, a2, a3);
            break;
        case 7:
            s->scrollX = a1;
            s->scrollY = a2;
            break;
    }

    if (eax & 0x1820000) {
        eax |= 0x10000;
    }
    u32 f = s->flags;
    if (f & 0x400000) {
        eax |= 0x100000;
    }
    if (f & 0x10) {
        eax |= 0x200000;
    }
    if (s->scrollX == prevX && s->scrollY == prevY) {
        eax |= 0x400000;
    }
    return eax;
}

// ===========================================================================
// The four per-brush-kind edit handlers (EditHandlerA..D). All are __thiscall
// (this=this level, the scroll target passed explicitly), ret 0x10. They step the
// target's +0x5c/+0x60 scroll one axis toward (a1, a2), advance the axis-2 cursor,
// probe the +0x138/+0x140 axis limits via AxisProbe (a ==3 result means blocked),
// and on a block re-clamp the span / average the [lo,hi] bracket. They commit the
// new scroll x/y to +0x5c/+0x60 and return the accumulated state-flag word.
//
// All sibling callees (StepAxisLo/Hi, Advance{A,B}, ClampSpan, Hold/FreeMove,
// StepAxisAlt, SpanCheck, AxisProbe) are UNMATCHED engine CGameLevel leaves modeled
// with no body, so their thiscall sites reloc-mask. `(lo + hi) / 2` lowers to the
// retail `add; cdq; sub; sar 1` signed-halve idiom.
// ===========================================================================

// ---------------------------------------------------------------------------
// EditHandlerA (kinds 1/2/5): axis-1 step toward a1, axis-2 advance (AdvanceA),
// then - per the arg3 low(bit0)/high(bit1) selector - an AxisProbe against the
// matching +0x138/+0x140 limit and, when that blocks (==3), a ClampSpan re-bracket
// whose [lo,hi] midpoint replaces the new coord (gated by the arg3 0x10 bit). The
// no-block tail drives Hold/FreeMove off the target's +0x10 held flag.
//
// @early-stop
// register-scheduling wall: 4 saved regs across 8 distinct sibling dispatches, the
// spilled loc/result/bracket slots and the held-flag tail schedule into a register
// assignment MSVC reproduces only for one spill order; logic + offsets + CFG +
// every sibling convention are exact. Deferred to the final sweep.
RVA(0x0015e130, 0x1bb)
i32 CGameLevel::EditHandlerA(void* target, i32 a1, i32 a2, i32 a3) {
    ScrollTarget* t = (ScrollTarget*)target;
    i32 result = 0;
    i32 coord = a1;

    if (a1 > t->scrollX) {
        result = StepAxisLo(t, a1, a2, &coord, a3);
    } else if (a1 < t->scrollX) {
        result = StepAxisHi(t, a1, a2, &coord, a3);
    }

    if (a2 < t->scrollY) {
        a2 = AdvanceA(t, coord, a2, a3);
    }

    if (a3 & 1) {
        i32 limit = t->limitLo + a2 - 1;
        if (AxisProbe(coord, limit) == 3) {
            if (a3 & 0x10) {
                i32 lo = coord;
                i32 hi = coord;
                if (ClampSpan(coord, limit, &lo, &hi) != 0) {
                    coord = (hi + lo) / 2;
                }
            }
            t->editKind = 6;
        }
    } else if (a3 & 2) {
        i32 limit = t->limitHi + a2 + 2;
        if (AxisProbe(coord, limit) == 3) {
            if (a3 & 0x10) {
                i32 lo = coord;
                i32 hi = coord;
                if (ClampSpan(coord, limit, &lo, &hi) != 0) {
                    coord = (hi + lo) / 2;
                }
            }
            t->editKind = 6;
        }
    } else {
        if (t->flags & 0x10) {
            if (HoldMove(t, t->holdAnchor, coord, a2, a3) == 0) {
                t->editKind = 4;
            }
        } else {
            a2 = FreeMove(t, coord, a2, a3);
        }
    }

    t->scrollX = coord;
    t->scrollY = a2;
    return result;
}

// ---------------------------------------------------------------------------
// EditHandlerC (kind 4): axis-1 step, an alternate axis-2 step (StepAxisAlt gated by
// arg3 bit3), an AdvanceB advance (unless the kind already turned 1), the same low
// AxisProbe + ClampSpan re-bracket as EditHandlerA, then - if the kind ended at 1 and
// the coord moved with the 0x20000 state bit set - one blocked-move retry (clear the
// 0xe0000 bits, re-step the axis).
//
// @early-stop
// register-scheduling wall: same 4-saved-reg / multi-dispatch + spilled-bracket
// scheduling as EditHandlerA, plus the retry tail; logic + offsets + CFG + sibling
// conventions exact. Deferred to the final sweep.
RVA(0x0015e2f0, 0x1b7)
i32 CGameLevel::EditHandlerC(void* target, i32 a1, i32 a2, i32 a3) {
    ScrollTarget* t = (ScrollTarget*)target;
    i32 savedA1 = a1;
    i32 result = 0;
    i32 coord = a1;

    if (a1 > t->scrollX) {
        result = StepAxisLo(t, a1, a2, &coord, a3);
    } else if (a1 < t->scrollX) {
        result = StepAxisHi(t, a1, a2, &coord, a3);
    }

    if (a3 & 8) {
        i32 outY = a2;
        if (StepAxisAlt(t, coord, a2, &outY, a3) != 0) {
            a2 = outY;
        }
    }

    if (t->editKind != 1) {
        a2 = AdvanceB(t, coord, a2, a3);
    }

    if (a3 & 1) {
        i32 limit = t->limitLo + a2 - 1;
        i32 saved = coord;
        if (AxisProbe(coord, limit) == 3) {
            if (a3 & 0x10) {
                i32 lo = saved;
                i32 hi = saved;
                if (ClampSpan(saved, limit, &lo, &hi) != 0) {
                    coord = (hi + lo) / 2;
                }
                t->editKind = 6;
            } else {
                t->editKind = 6;
            }
        }
    }

    if (t->editKind == 1 && coord != savedA1) {
        if (result & 0x20000) {
            result &= 0xfff1ffff;
            if (coord > t->scrollX) {
                result |= StepAxisLo(t, coord, a2, &coord, a3);
            } else if (coord < t->scrollX) {
                result |= StepAxisHi(t, coord, a2, &coord, a3);
            }
        }
    }

    t->scrollX = coord;
    t->scrollY = a2;
    return result;
}

// ---------------------------------------------------------------------------
// EditHandlerB (kind 3, also kind 8 down-moves): axis-1 step, axis-2 advance
// (AdvanceA, unconditional), the low AxisProbe + ClampSpan re-bracket, then commit.
//
// @early-stop
// register-scheduling wall: 4 saved regs across StepAxis/AdvanceA/AxisProbe/ClampSpan
// + the spilled bracket slots; logic + offsets + CFG + conventions exact. Deferred.
RVA(0x0015e4b0, 0xf7)
i32 CGameLevel::EditHandlerB(void* target, i32 a1, i32 a2, i32 a3) {
    ScrollTarget* t = (ScrollTarget*)target;
    i32 result = 0;
    i32 coord = a1;

    if (a1 > t->scrollX) {
        result = StepAxisLo(t, a1, a2, &coord, a3);
    } else if (a1 < t->scrollX) {
        result = StepAxisHi(t, a1, a2, &coord, a3);
    }

    i32 cursor = AdvanceA(t, coord, a2, a3);

    if (a3 & 1) {
        i32 limit = t->limitLo + cursor - 1;
        if (AxisProbe(coord, limit) == 3) {
            i32 mid = coord;
            if (a3 & 0x10) {
                i32 lo = coord;
                i32 hi = coord;
                if (ClampSpan(coord, limit, &lo, &hi) != 0) {
                    mid = (hi + lo) / 2;
                }
            }
            if (a3 & 0x10) {
                coord = mid;
            }
            t->editKind = 6;
        }
    }

    t->scrollX = coord;
    t->scrollY = cursor;
    return result;
}

// ---------------------------------------------------------------------------
// EditHandlerD (kind 6): drives the axis-2 advance first (AdvanceB on a down-move,
// else AdvanceA), runs the +0x138/+0x140 two-probe (low then high limit, blocking on
// ==3), and on the up-move path a SpanCheck validate that may clamp the cursor below
// the +0x140 high limit; then a final axis-1 step and commit.
//
// @early-stop
// register-scheduling wall: the two-probe + SpanCheck + step tail across 4 saved regs
// + spilled cursor/limit slots; logic + offsets + CFG + sibling conventions exact.
// Deferred to the final sweep.
RVA(0x0015e5b0, 0x162)
i32 CGameLevel::EditHandlerD(void* target, i32 a1, i32 a2, i32 a3) {
    ScrollTarget* t = (ScrollTarget*)target;
    i32 result = 0;
    i32 cursor;

    if (t->scrollY >= a1) {
        cursor = AdvanceB(t, a2, a1, a3);
        if (t->editKind != 1) {
            i32 hi = t->limitHi + cursor + 1;
            i32 lo = t->limitLo + cursor - 1;
            if (AxisProbe(a2, lo) != 3 && AxisProbe(a2, hi) != 3) {
                t->editKind = 4;
            }
        }
    } else {
        cursor = AdvanceA(t, a1, a2, a3);
        i32 hi = t->limitHi + cursor + 1;
        i32 lo = t->limitLo + cursor - 1;
        if (AxisProbe(a2, lo) != 3 && AxisProbe(a2, hi) != 3) {
            i32 probe = a2;
            i32 want = (t->limitHi + cursor + 1) - cursor + t->scrollY;
            if (SpanCheck(want, t->limitHi + cursor + 1, probe, &probe) != 0 && probe > cursor) {
                t->editKind = 1;
                cursor = probe - t->limitHi - 1;
            }
        }
    }

    i32 coord = a2;
    if (a2 > t->scrollX) {
        result = StepAxisLo(t, a2, cursor, &coord, a3);
    } else if (a2 < t->scrollX) {
        result = StepAxisHi(t, a2, cursor, &coord, a3);
    }

    t->scrollX = coord;
    t->scrollY = cursor;
    return result;
}

// ===========================================================================
// The sibling brush-handler leaves the EditHandlers dispatch into (ascending RVA).
// ===========================================================================

// ---------------------------------------------------------------------------
// StepAxisLo (@0x15e720): step the target one axis from its low bracket toward the
// requested mid coord, probing the main plane's tiles along the way. The probe walks
// `cur` from (axisLoB + a2) up to (axisHi + a2) in +0xfc strides; the first cur whose
// tile probe returns 1 commits the target's current scroll into *outX and returns
// 0x60000, else *outX = a1 and 0. The tile probe is AxisProbe inlined (PROBE_TILE).
//
// @early-stop
// register-scheduling wall: the inlined PROBE_TILE block + the strided step loop pin
// 4 saved regs (mid/lo/hi/cur) across the per-iteration image-set dispatch in an
// order MSVC reproduces only for one spill assignment; logic + offsets + CFG + the
// probe dispatch are exact. Deferred to the final sweep.
RVA(0x0015e720, 0x14c)
i32 CGameLevel::StepAxisLo(void* target, i32 a1, i32 a2, i32* outX, i32 a3) {
    EditTarget* t = (EditTarget*)target;
    i32 mid = t->axisMid + a1;
    i32 lo = t->axisLoB + a2;
    i32 hi = t->axisHi + a2;
    i32 cur = lo;

    if (lo <= hi) {
        do {
            i32 result;
            PROBE_TILE(this, mid, cur, result);
            if (result == 1) {
                *outX = t->scrollX;
                return 0x60000;
            }
            if (cur == hi) {
                ++cur;
            } else {
                cur += t->stepY;
                if (cur > hi) {
                    cur = hi;
                }
            }
        } while (cur <= hi);
    }

    *outX = a1;
    return 0;
}

// ---------------------------------------------------------------------------
// StepAxisHi (@0x15e870): the mirror of StepAxisLo using the +0x134 high bracket
// (axisLoA) as the loop floor and returning 0xa0000 on a successful step.
//
// @early-stop
// register-scheduling wall: same PROBE_TILE + strided-step shape as StepAxisLo; logic
// + offsets + CFG exact. Deferred to the final sweep.
RVA(0x0015e870, 0x14c)
i32 CGameLevel::StepAxisHi(void* target, i32 a1, i32 a2, i32* outX, i32 a3) {
    EditTarget* t = (EditTarget*)target;
    i32 mid = t->axisLoA + a1;
    i32 lo = t->axisLoB + a2;
    i32 hi = t->axisHi + a2;
    i32 cur = lo;

    if (lo <= hi) {
        do {
            i32 result;
            PROBE_TILE(this, mid, cur, result);
            if (result == 1) {
                *outX = t->scrollX;
                return 0xa0000;
            }
            if (cur == hi) {
                ++cur;
            } else {
                cur += t->stepY;
                if (cur > hi) {
                    cur = hi;
                }
            }
        } while (cur <= hi);
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
i32 CGameLevel::FreeMove(void* target, i32 a1, i32 a2, i32 a3) {
    EditTarget* t = (EditTarget*)target;
    i32 mid = t->axisMid + a1;
    i32 cur = t->axisLoA + a1;
    i32 hiY = t->axisHi + a2 + 1;

    if (cur <= mid) {
        do {
            i32 result;
            PROBE_TILE(this, cur, hiY, result);
            if (result == 1 || result == 2) {
                // kind 1/2: re-probe the one-step-back pair twice to confirm the
                // move still fits (retail outlines the 2nd index lookup to a leaf).
                i32 r2;
                PROBE_TILE(this, cur, hiY - 1, r2);
                if (r2 != 1) {
                    i32 r3;
                    PROBE_TILE(this, cur, hiY - 1, r3);
                    if (r3 != 2) {
                        return a2;
                    }
                }
            } else if (t->editKind != 6 && result == 3) {
                if (AxisProbe(cur, hiY) == 3) {
                    if (AxisProbe(cur, hiY - 1) != 3) {
                        return a2;
                    }
                }
            }
            if (cur == mid) {
                ++cur;
            } else {
                cur += t->stepX;
            }
        } while (cur <= mid);
    }

    t->editKind = 4;
    return a2;
}

// ---------------------------------------------------------------------------
// AdvanceB (@0x15ede0): advance the axis-2 cursor variant. Probes the start coord,
// folds a kind-4 result into the target +0x8 flags (0x400000), then walks the cursor
// down through AxisProbe gates; a clear low/high pair commits brush kind 1 and the
// adjusted cursor. Returns the resolved cursor.
//
// @early-stop
// register-scheduling wall (large /O2 body): two inlined PROBE_TILE copies + two
// AxisProbe gate loops + the strided walk pin the saved regs in an order MSVC
// reproduces only for one spill order; logic + offsets + CFG exact. Deferred.
RVA(0x0015ede0, 0x2a7)
i32 CGameLevel::AdvanceB(void* target, i32 a1, i32 a2, i32 a3) {
    EditTarget* t = (EditTarget*)target;
    i32 lo = t->axisLoA + a1;
    i32 mid = t->axisMid + a1;
    i32 hiY = a2 + t->axisHi + 1;

    i32 first;
    PROBE_TILE(this, a1, hiY, first);
    if (first == 4) {
        t->flags |= 0x400000;
    }
    i32 base = a2 - t->scrollY;

    i32 cur = lo;
    if (cur <= mid) {
        do {
            i32 result;
            PROBE_TILE(this, cur, hiY, result);
            if (result == 1 || result == 2) {
                i32 floor = t->scrollY + t->axisHi;
                if (hiY >= floor) {
                    i32 y = hiY;
                    do {
                        i32 g = AxisProbe(cur, y);
                        if (g != 1 && g != 2) {
                            t->editKind = 1;
                            return y - t->axisHi;
                        }
                        --y;
                    } while (y >= floor);
                }
            } else if (t->editKind != 6 && result == 3) {
                i32 floor = hiY - base;
                if (hiY > floor) {
                    i32 y = hiY - 1;
                    if (y >= floor) {
                        do {
                            if (AxisProbe(cur, y) != 3) {
                                t->editKind = 1;
                                return (y + 1) - t->axisHi - 1;
                            }
                            --y;
                        } while (y >= floor);
                    }
                }
            }
            if (cur == mid) {
                ++cur;
            } else {
                cur += t->stepX;
            }
        } while (cur <= mid);
    }

    return a3;
}

// ---------------------------------------------------------------------------
// AdvanceA (@0x15f1c0): advance the axis-2 cursor. Probes (cur, hi) across the
// strided walk; a hit gates an AxisProbe sweep up the +0x138 column, and on the
// AxisProbe miss commits brush kind 4 and returns the adjusted cursor.
//
// @early-stop
// register-scheduling wall: inlined PROBE_TILE + the AxisProbe gate loop pin the
// saved regs in an order MSVC reproduces only for one spill order; logic + offsets +
// CFG exact. Deferred to the final sweep.
RVA(0x0015f1c0, 0x171)
i32 CGameLevel::AdvanceA(void* target, i32 a1, i32 a2, i32 a3) {
    EditTarget* t = (EditTarget*)target;
    i32 cur = t->axisLoA + a1;
    i32 mid = t->axisMid + a1;
    i32 ceil = a2 + t->axisLoB - 1;

    if (cur <= mid) {
        do {
            i32 result;
            PROBE_TILE(this, cur, ceil, result);
            if (result == 1) {
                i32 floor = t->scrollY + t->axisLoB - 1;
                if (ceil <= floor) {
                    i32 y = ceil;
                    do {
                        if (AxisProbe(cur, y) != 1) {
                            t->editKind = 4;
                            return y - t->axisLoB;
                        }
                        ++y;
                    } while (y <= floor);
                }
            }
            if (cur == mid) {
                ++cur;
            } else {
                cur += t->stepX;
            }
        } while (cur <= mid);
    }

    return a3;
}

// ---------------------------------------------------------------------------
// SpanCheck (@0x15f8d0): validate that the probe column from (b-1) down to c fits -
// the first cur whose tile probe is not "blocked" (!= 3) commits cur+1 into *out and
// returns 1; an empty or all-blocked span returns 0.
//
// @early-stop
// register-scheduling wall: the inlined PROBE_TILE block across the down-counting loop
// pins the saved regs in an order MSVC reproduces only for one spill order; logic +
// offsets + CFG exact. Deferred to the final sweep.
RVA(0x0015f8d0, 0x113)
i32 CGameLevel::SpanCheck(i32 a, i32 b, i32 c, i32* out) {
    if (b <= c) {
        return 0;
    }
    i32 cur = b - 1;
    if (cur < c) {
        return 0;
    }
    do {
        i32 result;
        PROBE_TILE(this, a, cur, result);
        if (result != 3) {
            *out = cur + 1;
            return 1;
        }
        --cur;
    } while (cur >= c);

    return 0;
}

// ---------------------------------------------------------------------------
// StepAxisAlt (@0x15fdb0): alternate axis-2 stepper. Only runs when a3 bit3 is set;
// walks the owner's object chain (m_owner->+0x8->+0x14) and, for each node whose
// payload carries the 0x80 marker at +0xe8, runs AltStepValidate. The first node it
// validates is latched into the target (+0x98 anchor, brush kind 1, +0x8 flag 0x10)
// and returns 1; an exhausted chain returns 0.
RVA(0x0015fdb0, 0x8a)
i32 CGameLevel::StepAxisAlt(void* target, i32 a1, i32 a2, i32* outY, i32 a3) {
    EditTarget* t = (EditTarget*)target;
    if ((a3 & 8) == 0) {
        return 0;
    }

    ObjNode* node = *(ObjNode**)((char*)((char**)m_owner)[2] + 0x14);
    while (node != 0) {
        ObjNode* cur = node;
        node = node->next;
        ObjPayload* pl = cur->obj;
        if (*(i32*)((char*)pl + 0xe8) == 0x80) {
            if (AltStepValidate(t, pl, a1, a2, outY, a3) != 0) {
                t->editKind = 1;
                t->holdAnchor = (i32)pl;
                t->flags |= 0x10;
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
// @early-stop
// register-scheduling wall (~72%): logic + offsets + CFG + the merged epilogue are
// exact; residue is the +0x60(scrollY) spill materialization and the cmpHi==tHi
// branch polarity (retail makes the ok-target fall-through). Deferred to the final sweep.
RVA(0x0015fe40, 0xd4)
i32 CGameLevel::AltStepValidate(void* target, void* payload, i32 a1, i32 a2, i32* outY, i32 a3) {
    EditTarget* t = (EditTarget*)target;
    ProbeObj* p = (ProbeObj*)payload;

    if (p->m_144 == -1) {
        goto fail;
    }
    if (t->axisLoA == -1) {
        goto fail;
    }
    {
        i32 sy = t->scrollY;
        if (sy > a2) {
            goto fail;
        }

        i32 boxL = p->m_144 + p->m_5c;
        i32 boxR = p->m_14c + p->m_5c;
        i32 boxT = p->m_60 + p->m_148;
        i32 tLoA = t->axisLoA + a1;
        i32 tMid = t->axisMid + a1;
        i32 tHi = t->axisHi + a2;
        i32 cmpHi = t->axisHi + sy;

        i32 over = p->m_178;
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
        if (cmpHi == tHi) {
            if (tHi != boxT - 1) {
                goto fail;
            }
        } else {
            if (tHi < boxT - 1) {
                goto fail;
            }
        }

        *outY = boxT - t->axisHi - 1;
        return 1;
    }
fail:
    return 0;
}

// ---------------------------------------------------------------------------
// HoldMove (@0x15ff20): the held-payload geometric fit test EditHandlerA/C run when
// the target's scroll is held against a latched object (anchor = the held payload).
// Gated like AltStepValidate (a3 bit3 set, payload->+0xe8 == 0x80, both low brackets
// valid), it checks the target's bracket box (offset by a1) overlaps the payload's
// world-space box and returns whether the held axis (axisHi + a2) sits exactly on the
// box's bottom edge (boxT - 1). All field reads; no calls. ret 0x14.
//
// @early-stop
// regalloc/spill wall (~90%): logic + offsets + CFG + the prologue gates + the
// dec/sete-cl result are byte-faithful; residue is one spill choice in the box-overlap
// add chain - retail keeps tMid/tLoA both live (boxL via a combined lea) while our cl
// spills tLoA to [esp+0x24]. Reordering the local computes regresses it (84%); not
// source-steerable. Deferred to the final sweep.
struct HoldPayload {
    char pad_0[0x5c];
    i32 m_5c; // +0x5c  origin X
    i32 m_60; // +0x60  origin Y
    char pad_64[0xe8 - 0x64];
    i32 m_e8; // +0xe8  kind marker (held == 0x80)
    char pad_ec[0x144 - 0xec];
    i32 m_144; // +0x144  box left
    i32 m_148; // +0x148  box row
    i32 m_14c; // +0x14c  box right
};
RVA(0x0015ff20, 0xc0)
i32 CGameLevel::HoldMove(void* t, i32 anchor, i32 a1, i32 a2, i32 a3) {
    HoldPayload* p = (HoldPayload*)anchor;
    if (p == 0) {
        return 0;
    }
    if ((a3 & 8) == 0) {
        return 0;
    }
    if (p->m_e8 != 0x80) {
        return 0;
    }
    if (p->m_144 == -1) {
        return 0;
    }
    EditTarget* et = (EditTarget*)t;
    if (et->axisLoA == -1) {
        return 0;
    }

    i32 ox = p->m_5c;
    i32 boxL = ox + p->m_144;
    i32 boxR = ox + p->m_14c;
    i32 boxT = p->m_60 + p->m_148;
    i32 hi = et->axisHi + a2;
    i32 tMid = et->axisMid + a1;
    i32 tLoA = et->axisLoA + a1;
    if (tMid < boxL) {
        return 0;
    }
    if (tLoA > boxR) {
        return 0;
    }
    return hi == boxT - 1;
}

// ---------------------------------------------------------------------------
// ClampSpan (@0x15ffe0): clamp (x, y) into the main plane's tile grid, fetch the
// tile there, and report the tile's column span: *outLo = the tile-aligned x, *outHi
// = that + the image set's width (+0x04) - 1. Returns 1 (0 for an empty/clear tile).
// An inlined tile probe that, unlike AxisProbe, keeps the tile-aligned coord and
// reads the image set's width field instead of dispatching slot +0x20. ret 0x10.
struct SpanImageSet {
    char pad_0[0x04];
    i32 m_04; // +0x04  tile (column) width
};
RVA(0x0015ffe0, 0x99)
i32 CGameLevel::ClampSpan(i32 x, i32 y, i32* outLo, i32* outHi) {
    if (x < 0) {
        x = 0;
    } else {
        ProbePlane* pc = (ProbePlane*)m_mainPlane;
        if (x >= pc->wrapW) {
            x = pc->wrapW - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        ProbePlane* pc = (ProbePlane*)m_mainPlane;
        if (y >= pc->wrapH) {
            y = pc->wrapH - 1;
        }
    }
    ProbePlane* pl = (ProbePlane*)m_mainPlane;
    i32 qx = x >> pl->shiftX;
    i32 alignedX = qx << pl->shiftX;
    i32 qy = y >> pl->shiftY;
    i32 idx = pl->colOffsets[qy] + qx;
    i32 tile = pl->tileGrid[idx];
    if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
        return 0;
    }
    SpanImageSet* set = (SpanImageSet*)m_imageSets[tile & 0xffff];
    *outLo = alignedX;
    *outHi = alignedX + set->m_04 - 1;
    return 1;
}

// ProbeTarget - the edit target ProbeColumn/WalkColumnDown drive: its +0x5c/+0x60
// scroll origin and the +0x134/+0x138/+0x140 axis brackets.
struct ProbeTarget {
    char pad_0[0x5c];
    i32 m_5c; // +0x5c  origin X
    i32 m_60; // +0x60  origin Y
    char pad_64[0x134 - 0x64];
    i32 m_134; // +0x134  axis low bracket
    i32 m_138; // +0x138  axis mid bracket
    char pad_13c[0x140 - 0x13c];
    i32 m_140; // +0x140  axis high bracket
};

// ---------------------------------------------------------------------------
// ProbeColumn (@0x160980): probe the single tile at the target's column origin
// (m_5c + dx, m_138 + m_60), clamped into the main plane grid, returning the image
// set's slot +0x20 dispatch (0 for an empty/clear tile). The inlined AxisProbe shape
// (PROBE_TILE) applied to a target+offset probe point. ret 8.
//
// @early-stop
// regalloc wall (~93%): logic + offsets + CFG + the inlined PROBE_TILE clamp/shift/
// dispatch are byte-faithful; residue is the clamp branch's spill/register assignment
// (the same PROBE_TILE-shape entropy as SpanCheck/AxisProbe). Deferred to the final sweep.
RVA(0x00160980, 0xc0)
i32 CGameLevel::ProbeColumn(void* target, i32 dx) {
    ProbeTarget* t = (ProbeTarget*)target;
    i32 px = t->m_5c + dx;
    i32 py = t->m_138 + t->m_60;
    i32 result;
    PROBE_TILE(this, px, py, result);
    return result;
}

// ---------------------------------------------------------------------------
// WalkColumnDown (@0x160a40): from the start row (target->m_140 + target->m_60), probe
// the tile column at the fixed x (target->m_5c) stepping the row downward until the
// image set's slot +0x20 reports a stop code (1/2/3) or the row runs off the grid
// (>= plane height). On a stop, commit the resolved row back into target->m_60 as
// (m_60 + finalRow - startRow - 1) and return 1; an unset low bracket / missing main
// plane / off-grid walk returns 0.
//
// @early-stop
// register-scheduling wall: the inlined PROBE_TILE + slot-+0x20 dispatch repeated
// across the down-counting walk (start probe + loop probe) pin the 4 saved regs, the
// spilled this/start-row/shiftY/wrapH in a spill order MSVC reproduces only for one
// allocation; logic + offsets + CFG + the commit arithmetic are exact. Deferred to the
// final sweep.
RVA(0x00160a40, 0x201)
i32 CGameLevel::WalkColumnDown(void* target, i32 unused) {
    ProbeTarget* t = (ProbeTarget*)target;
    if (t->m_134 == (i32)0x80000000) {
        return 0;
    }
    if (m_mainPlane == 0) {
        return 0;
    }

    i32 px = t->m_5c;
    i32 row = t->m_140 + t->m_60;
    i32 startRow = row;

    i32 result;
    PROBE_TILE(this, px, row, result);

    while (result != 1) {
        if (result == 2 || result == 3) {
            break;
        }
        ++row;
        if (row >= ((ProbePlane*)m_mainPlane)->wrapH) {
            return 0;
        }
        PROBE_TILE(this, px, row, result);
    }

    i32 final = row - startRow - 1;
    t->m_60 += final;
    return 1;
}

// --- class-metadata: the FORCE-REALIZED vtables (were the g_gameLevelVtbl /
// g_imageSet1/2/3Vtbl manual stamps + UnknownVTables placeholders). cl now emits
// each ??_7 (18 slots), matched slots pointing at the real methods (RVA-bound), the
// base-thunk/engine slots reloc-masked declared-only externs. -------------------
VTBL(CGameLevel, 0x001f0150); // ??_7CGameLevel (was g_gameLevelVtbl)
VTBL(CImageSet1, 0x001f0198); // ??_7CImageSet1 (was g_imageSet1Vtbl)
VTBL(CImageSet2, 0x001f01e0); // ??_7CImageSet2 (was g_imageSet2Vtbl)
VTBL(CImageSet3, 0x001f0228); // ??_7CImageSet3 (was g_imageSet3Vtbl)
SIZE(CImageSet1, 0x10);
SIZE(CImageSet2, 0x24);
SIZE_UNKNOWN(CGameLevel);
SIZE_UNKNOWN(CImageSet);
