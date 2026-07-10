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
#include <Wap32/Object.h>       // CObject grand-base (slots 0-4) for the CImageSetN variants
#include <Gruntz/ParseSource.h> // canonical CParseSource (BeginParse/EndParse)
#include <Gruntz/UserLogic.h>   // canonical CGameObject (the movement target) + world chain types
#include <Io/FileStream.h>      // CFileIO (Open/Read/GetLength/ctor/dtor reloc-masked)
#include <rva.h>

#include <string.h> // strcpy, memset

// The collision-relevant tile codes among the broad tile-type space CImageSet::
// GetCollisionAt (and the tile probes that dispatch it) returns. NOT a closed enum:
// CGameLevel::LookupTile hands the same result to BrickzLoad as a full tile-type code
// switched over cases up to 113, so only these low codes carry a movement-collision
// meaning - named from how the movement/scroll steppers below branch on them; every
// other code is treated as passable. File-local (only the steppers here consume them),
// so GetCollisionAt keeps its i32 return. Matching-neutral: each folds to the same int
// immediate the retail steppers compare. #define, not an enum: introducing ANY enum
// DECLARATION into this @early-stop-heavy TU perturbs MSVC5's whole-TU scheduling enough
// to shift the unrelated BroadPhase regalloc wall by -0.13% fuzzy; the preprocessor form
// has no AST footprint and is byte-identical to baseline (verified: named/anon enum both
// regress, #define is exactly neutral). #undef'd at end of file.
#define kTilePassable 0 // empty tile / any non-colliding code
#define kTileSoft 1     // soft-blocking (triggers the inward axis re-scan)
#define kTileSoft2 2    // soft-blocking; 0x400-flag downgradeable, and blocks a fall
#define kTileHard 3     // hard-blocking (the axis gates' `== kTileHard` stop code)
#define kTileSpecial 4  // special (folds the target's 0x400000 flag)

// ===========================================================================
// The CDDrawLevelData methods, merged in here as CGameLevel.
//
// CGameLevel is the same class the engine handles via the obfuscated name
// CDDrawLevelData (its vtable slot 0x38 is CGameLevel::LoadWwd). The methods
// below were reconstructed as CDDrawLevelData::* and are moved here onto
// CGameLevel. Each touches the level's own members through their named fields
// (m_planeCtx@+0x10, m_planes/m_imageSets, m_owner@+0x0c, m_04@+0x04, the
// m_b0..m_dc default-extents block, m_header@+0xe0). The objects they dispatch
// into are the real CPlane (WwdFile.h) for the per-plane objects and the
// canonical CGameObject (<Gruntz/UserLogic.h>) for the movement/collision
// target (formerly per-family window structs; see the IDENTITY note below).
//
// The shared "default extents" block at +0xb0..+0xdc is stamped with the same
// constants by the ctor and several edit methods (StampParamBlock):
//     +0xb0 = 500  +0xb4 = 250  +0xb8 = 1000 +0xbc = 1000
//     +0xc0 = 250  +0xc4 = 125  +0xc8 = 1600 +0xcc = 1200
//     +0xd0 = 2560 +0xd4 = 1920 +0xd8 = 768  +0xdc = 576
// ===========================================================================

// (LevelCoordRect - the 4-int coord record at +0x10 - is defined in GameLevel.h.)

// The parse-source object LoadFromSource drives is the canonical CParseSource
// (included above): BeginParse (0x139960) opens/primes it and returns a handle;
// EndParse (0x1399d0) tears it down. Both are unmatched engine leaves taking the
// source as `this`; declared with no body so the thiscall sites reloc-mask.

// The pointer arrays hold real objects: m_planes -> CLevelPlane* (the level.s typed
// view of the engine plane, GameLevel.h), m_imageSets -> CImageSet*. Both carry the
// +0x04 release slot (CLevelPlane::dtor / CImageSet::Release). The per-plane object.s
// tile grid, extents, coord-recompute outputs, tile->pixel shifts and name are all
// named CLevelPlane members reached without a per-site cast off m_mainPlane.

// The two-phase vptr stores are now cl-emitted: the inlined CLoadable ctor (in
// GameLevel.h) auto-stamps the base vptr (&??_7CLoadable, orphan reloc-masked
// against retail 0x5efc30) and the derived CGameLevel ctor auto-stamps the derived
// vptr (&??_7CGameLevel, bound @0x5f0150 via VTBL below) after the three array
// members are constructed. The only remaining manual vtable store is the grand-base
// teardown vftable ~CLoadable restamps after the member dtors run (@0x5e8cb4).
DATA(0x001e8cb4)

// The "unset" sentinel the ctor writes into the coord record's min corner; the
// readiness predicate (IsLoaded) tests for it and Unload restores it.
static const i32 LEVEL_COORD_UNSET = (i32)0x80000000;

// LookupTile's empty-cell sentinels: 0xeeeeeeee is the uninitialized-heap fill
// (no tile placed); -1 is the explicit "clear" marker.
static const i32 TILE_UNINIT = (i32)0xeeeeeeee;
static const i32 TILE_CLEAR = -1;

// The +0x134 axis-low bracket's "unset" sentinel (INT_MIN): WalkColumnDown and
// BroadPhase test it before treating an object's AABB as a live box.
static const i32 AXIS_UNSET = (i32)0x80000000;

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
// reloc-name mask + store-scheduling entropy plateau (~94%). Re-pinning the arrays to
// their genuine shape (CByteArray + two CDWordArrays, all out-of-line ctors) fixed the
// whole array-construction prologue: the three `leal +0x20/+0x34/+0x48; movb EH-state
// 0/1/2; call ??0..Array` sequence and the two cl-emitted vptr stores (base ??_7CLoadable
// orphan + derived ??_7CGameLevel @0x5f0150) now match retail exactly (48.8%->94.4%).
// Two residuals remain, neither source-steerable: (1) reloc-name masks - retail ICF-
// folded the identical CByteArray/CDWordArray default ctors to ONE `CByteArray` symbol,
// so our two `??0CDWordArray@@QAE@XZ` calls + the `push $handler` funcinfo mask against
// retail's folded names; (2) the tail store scheduling - cl parks the 0xfa immediate in
// eax and stamps the ??_7CGameLevel vptr before the m_b4/m_c0 stores, while retail keeps
// 0xfa in ecx and floats the vptr stamp later (matching-patterns.md §entropy: an
// independent immediate-to-memory store has no dep to pin its slot). Logic + offsets +
// CFG + EH frame exact.
RVA(0x0015ccd0, 0x118)
CGameLevel::CGameLevel(i32 a1, i32 a2, i32 a3) {
    m_04 = a2;
    m_08 = a3;
    m_0c = a1; // (merged CLoadable ctor)
    m_maxStepX = 0x40;
    m_maxStepY = 0x40;
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
    Bytef* ehAlloc = 0; // inflate buffer freed on every exit path

    // The flags field is read twice (the COMPRESS test and the m_flags store); the
    // retail compiler materializes &hdr->flags once and dereferences it both times,
    // so model it as a cached pointer.
    u32* pflags = &hdr->flags;

    if (*pflags & 0x2) // COMPRESS: inflate the main block
    {
        u32 allocSize = hdr->mainBlockLength + hdr->wwdSignature + 0x40;
        Bytef* buf = (Bytef*)operator new(allocSize);
        if (buf == 0) {
            return 0;
        }

        block = (char*)WwdFile_InflateMainBlock(hdr, buf, allocSize - 0x20);
        if (block == 0) {
            operator delete(buf);
            return 0;
        }
        ehAlloc = buf;
    }

    strcpy(m_levelName, hdr->levelName); // inline strlen + rep movs
    m_08 = *pflags;
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
        CLevelPlane* mp = m_mainPlane;
        if (mp->m_flags & 1) {
            mp->m_scaledX = (float)startX;
            mp->m_scaledY = (float)startY;
        } else {
            mp->m_scaledX = (float)startX * mp->m_scaleX;
            mp->m_scaledY = (float)startY * mp->m_scaleY;
        }
        mp->RecomputePlaneCoords();

        // Re-derive the start coords from the main plane's origin for the rest.
        i32 ox = m_mainPlane->m_originX;
        i32 oy = m_mainPlane->m_originY;
        i32 i2 = 0;
        while (i2 < m_planes.GetSize()) // GetSize() == the plane count
        {
            if (i2 != m_mainIndex) {
                CLevelPlane* p = (CLevelPlane*)m_planes[i2];
                if (p->m_flags & 1) {
                    p->m_scaledX = (float)ox;
                    p->m_scaledY = (float)oy;
                } else {
                    p->m_scaledX = (float)ox * p->m_scaleX;
                    p->m_scaledY = (float)oy * p->m_scaleY;
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

// ===========================================================================
// Merged CDDrawLevelData leaves (now CGameLevel). All are plain /O2 /MT leaves:
// NO SEH frame, NO string/global relocations (dumps report "Relocations: none") -
// they only touch member offsets (written as raw casts on `this`), an argument
// struct, and sibling virtuals via the object's own vtable.
//
// The three 184-byte siblings (variant24/28/2C) are identical except for which
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
    if (m_0c == 0) {
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
    // handle = the in-memory WWD block here; BeginParse's return stays a generic
    // i32 handle in the shared ParseSource.h (other sources yield RIFF blobs /
    // sizes - see RezSync / DDrawSubMgrLeafScan), so the cast is the honest bridge.
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
// The scalar-deleting dtor is the compiler-generated ??_G (folded from ~CGameLevel);
// pin it by mangled name since it has no source body.
// @rva-symbol: ??_GCGameLevel@@UAEPAXI@Z 0x001611c0 0x1e

// ---------------------------------------------------------------------------
// Destructor: cl auto-stamps the derived vftable @0x5f0150 at dtor entry
// (polymorphic), then runs the level cleanup, lets the three array members destruct
// (reverse construction order), then ~CLoadable restores the base subobject
// (resets m_04/m_08/m_0c + the grand-base dtor vftable @0x5e8cb4). The
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
        CLevelPlane* child = (CLevelPlane*)m_planes.GetData()[i];
        if (child) {
            child->dtor(1); // scalar-deleting dtor (+0x04)
        }
    }
    m_planes.SetSize(0, -1);
    for (i = 0; i < m_imageSets.GetSize(); i++) {
        CImageSet* child = (CImageSet*)m_imageSets.GetData()[i];
        if (child) {
            child->Release(1); // release/free hook (+0x04)
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
        CLevelPlane* child = (CLevelPlane*)m_planes.GetData()[i];
        if (child) {
            child->dtor(1); // scalar-deleting dtor (+0x04)
        }
    }
    m_planes.SetSize(0, -1);
    for (i = 0; i < m_imageSets.GetSize(); i++) {
        CImageSet* child = (CImageSet*)m_imageSets.GetData()[i];
        if (child) {
            child->Release(1); // release/free hook (+0x04)
        }
    }
    m_imageSets.SetSize(0, -1);
    m_mainPlane = 0;
    m_mainIndex = -1;
}

// CGameLevel::GetClassId (0x001611b0) is now an inline member in the header.

// --- the SetCoordsAndLoadNN sibling family (do not drop) -------------------
// ---------------------------------------------------------------------------
// As SetCoordsAndLoad38 but dispatches the +0x40 load virtual.
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

// ---------------------------------------------------------------------------
// As SetCoordsAndLoad38 but dispatches the +0x3c load virtual.
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

// ---------------------------------------------------------------------------
// Loads the +0x10 record from *coords, stamps the param block, then dispatches
// the +0x38 load virtual with arg1. On a 0 result it runs the +0x1c hook and
// returns 0; otherwise returns 1.
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

// The three CImageSet variants the factory allocates. REAL-POLYMORPHIC: each is an
// 18-slot class deriving the CObject grand-base (slots 0-4 inherited, slot 1 =
// its virtual dtor), so cl emits its ??_7CImageSetN@@6B@ (bound below via VTBL to the
// retail vtable) and AUTO-stamps the vptr in the INLINE ctor - the base-subobject
// stamp dead-store-elides, lowering `new CImageSetN` to exactly the retail
// `RezAlloc(size); if (p) { stamp vptr; zero fields }` shape. Only slot 5 (+0x14
// Parse) is a matched body; the inherited base thunks + engine slots are declared-
// only (their vtable entries reloc-mask). The vptr sits at +0x00 (implicit); the
// padding pins each size: kind 1 = 0x10, kind 2 = 0x24, kind 3 = 0x18. Slot RVAs
// (from retail 0x5f0198/01e0/0228) noted per class.
struct CImageSet1 : CObject {
    virtual ~CImageSet1() OVERRIDE; // slot 1 (CObject dtor)
    // slots 0-4 inherited from CObject (slot 1 = its virtual dtor; cl auto-
    // stamps this vptr in the inline ctor, the base stamp dead-store-elides).
    virtual i32 Parse(void* record); // [5]  +0x14  0x166d40
    virtual void s18();              // [6]  0x161330
    virtual void s1c();              // [7]  0x161340
    virtual i32
    GetCollisionAt(i32 x, i32 y); // [8]  +0x20  0x161380  per-pixel collision-kind query
    virtual i32 GetStride();      // [9]  +0x24  0x161410  record byte length (cursor advance)
    virtual void s28();           // [10] 0x161390
    virtual void s2c();           // [11] 0x1613a0
    virtual void s30();           // [12] 0x1613b0
    virtual void s34();           // [13] 0x1613c0
    virtual void s38();           // [14] 0x1613d0
    virtual void s3c();           // [15] 0x1613e0
    virtual void s40();           // [16] 0x1613f0
    virtual void s44();           // [17] 0x161400
    CImageSet1() {
        m_04 = 0; // cl auto-stamps &??_7CImageSet1 first
    }
    void* operator new(size_t n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }
    RVA(0x00161370, 0x7)
    void DtorBase() {
        // base-subobject vptr restore is compiler-managed via the CObject base; manual g_wapObjectDtorVtbl stamp dropped (% ok)
    }
    i32 m_04; // +0x04
    i32 m_08; // +0x08
    i32 m_0c; // +0x0c
};
struct CImageSet2 : CObject {
    virtual ~CImageSet2() OVERRIDE; // slot 1 (CObject dtor)
    // slots 0-4 inherited from CObject (slot 1 = its virtual dtor).
    virtual i32 Parse(void* record); // [5]  +0x14  0x166990
    virtual void s18();              // [6]  0x161420
    virtual void s1c();              // [7]  0x161430
    virtual i32
    GetCollisionAt(i32 x, i32 y); // [8]  +0x20  0x161470  per-pixel collision-kind query
    virtual i32 GetStride();      // [9]  +0x24  0x1614a0  record byte length (cursor advance)
    // [10] 0x1669e0: bounds-clamp query - if (a,b) is inside the {m_14..m_1c}x{m_18..m_20}
    // box, report the near x-edge coord + its paired value in *outA/*outB (1), else 0.
    virtual i32 Query_1669e0(i32 a, i32 b, i32* outA, i32* outB);
    // slots 11-17: directional edge-finder variants (near/far x/y edge; the value-check
    // forms take an i32 to match against the paired cell before reporting).
    virtual i32 Query_166a40(i32 a, i32 b, i32 val, i32* out);    // [11] 0x166a40
    virtual i32 Query_166b90(i32 a, i32 b, i32* outA, i32* outB); // [12] 0x166b90
    virtual i32 Query_166bf0(i32 a, i32 b, i32 val, i32* out);    // [13] 0x166bf0
    virtual i32 Query_166ab0(i32 a, i32 b, i32* outA, i32* outB); // [14] 0x166ab0
    virtual i32 Query_166b20(i32 a, i32 b, i32 val, i32* out);    // [15] 0x166b20
    virtual i32 Query_166c60(i32 a, i32 b, i32* outA, i32* outB); // [16] 0x166c60
    virtual i32 Query_166cd0(i32 a, i32 b, i32 val, i32* out);    // [17] 0x166cd0
    CImageSet2() {
        m_04 = 0; // cl auto-stamps &??_7CImageSet2 first
    }
    void* operator new(size_t n) {
        return RezAlloc(n);
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
struct CImageSet3 : CObject {
    virtual ~CImageSet3() OVERRIDE; // slot 1 (CObject dtor)
    // slots 0-4 inherited from CObject (slot 1 = its virtual dtor).
    virtual i32 Parse(void* record); // [5]  +0x14  0x166d70
    virtual void s18();              // [6]  0x1614b0
    virtual void s1c();              // [7]  0x1614d0
    virtual i32
    GetCollisionAt(i32 x, i32 y); // [8]  +0x20  0x161570  per-pixel collision-kind query
    virtual i32 GetStride();      // [9]  +0x24  0x161590  record byte length (cursor advance)
    virtual void s28();           // [10] 0x166e00 (ScanRunLeft, in ImageSet3.cpp's model)
    virtual void s2c();           // [11] 0x166e60 (recovery gap, not a stub)
    // [12] 0x166eb0: vertical run-scan UP from (x,y) - walk to the first row whose pixel
    // at column x differs from (x,y)'s; report that row + its value.
    virtual i32 ScanUp_166eb0(i32 x, i32 y, i32* outY, i32* outVal);
    // [13-17]: the directional scan family - UP/RIGHT/DOWN run scans (the *Gate forms
    // walk to the first pixel that EQUALS `val`, reporting only the coord).
    virtual i32 ScanUpGate_166f20(i32 x, i32 y, i32 val, i32* outY);    // [13] 0x166f20
    virtual i32 ScanRight_166f80(i32 x, i32 y, i32* outX, i32* outVal); // [14] 0x166f80
    virtual i32 ScanRightGate_166ff0(i32 x, i32 y, i32 val, i32* outX); // [15] 0x166ff0
    virtual i32 ScanDown_167050(i32 x, i32 y, i32* outY, i32* outVal);  // [16] 0x167050
    virtual i32 ScanDownGate_1670d0(i32 x, i32 y, i32 val, i32* outY);  // [17] 0x1670d0
    CImageSet3() {
        m_width = 0; // cl auto-stamps &??_7CImageSet3 first
        m_pixels = 0;
    }
    void* operator new(size_t n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }
    i32 m_width;      // +0x04  tile width
    i32 m_height;     // +0x08  tile height
    i32 m_heightLog2; // +0x0c  log2(height)
    i32 m_byteSize;   // +0x10  width*height (byte size)
    void* m_pixels;   // +0x14  owned pixel buffer
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

// CImageSet1::DtorBase (0x00161370) is now an inline member in the header.

// CImageSet2::GetCollisionAt (0x161470, slot 8): return the interior collision
// kind m_10 when (x,y) is inside the {m_14..m_1c} x {m_18..m_20} box, else the
// exterior kind m_0c. __thiscall, 2 args (ret 0x8).
RVA(0x00161470, 0x2c)
i32 CImageSet2::GetCollisionAt(i32 x, i32 y) {
    if (x < m_14 || x > m_1c || y < m_18 || y > m_20) {
        return m_0c;
    }
    return m_10;
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

// CImageSet2::Query_1669e0 (0x1669e0, slot 10): reject when (a,b) is outside the
// {m_14..m_1c} x {m_18..m_20} box; otherwise, past the far x-edge (a>m_1c) report the
// x-edge m_1c + its value m_10, else the near edge (m_14-1, needing m_14>0) + m_0c.
// __thiscall, 4 args (ret 0x10).
RVA(0x001669e0, 0x5e)
i32 CImageSet2::Query_1669e0(i32 a, i32 b, i32* outA, i32* outB) {
    if (b < m_18 || b > m_20 || a < m_14) {
        return 0;
    }
    if (a > m_1c) {
        *outA = m_1c;
        *outB = m_10;
        return 1;
    }
    if (m_14 <= 0) {
        return 0;
    }
    *outA = m_14 - 1;
    *outB = m_0c;
    return 1;
}

// 0x166a40 (slot 11): x-edge query with a value gate - like Query_1669e0 but the paired
// cell (m_10 far / m_0c near) must equal `val`, and it reports only the near x coord.
RVA(0x00166a40, 0x62)
i32 CImageSet2::Query_166a40(i32 a, i32 b, i32 val, i32* out) {
    if (b < m_18 || b > m_20 || a < m_14) {
        return 0;
    }
    if (a > m_1c) {
        if (m_10 != val) {
            return 0;
        }
        *out = m_1c;
        return 1;
    }
    if (m_14 <= 0) {
        return 0;
    }
    if (m_0c != val) {
        return 0;
    }
    *out = m_14 - 1;
    return 1;
}

// 0x166b90 (slot 12): the y-axis twin of Query_1669e0 - clamp on the {m_18..m_20} row.
RVA(0x00166b90, 0x5e)
i32 CImageSet2::Query_166b90(i32 a, i32 b, i32* outA, i32* outB) {
    if (a < m_14 || a > m_1c || b < m_18) {
        return 0;
    }
    if (b > m_20) {
        *outA = m_20;
        *outB = m_10;
        return 1;
    }
    if (m_18 <= 0) {
        return 0;
    }
    *outA = m_18 - 1;
    *outB = m_0c;
    return 1;
}

// 0x166bf0 (slot 13): y-edge query with a value gate (twin of Query_166a40).
RVA(0x00166bf0, 0x62)
i32 CImageSet2::Query_166bf0(i32 a, i32 b, i32 val, i32* out) {
    if (a < m_14 || a > m_1c || b < m_18) {
        return 0;
    }
    if (b > m_20) {
        if (m_10 != val) {
            return 0;
        }
        *out = m_20;
        return 1;
    }
    if (m_18 <= 0) {
        return 0;
    }
    if (m_0c != val) {
        return 0;
    }
    *out = m_18 - 1;
    return 1;
}

// 0x166ab0 (slot 14): the far-x-edge query - reject past m_1c, clamp below m_14 (report
// m_14/m_10) else report the next cell (m_1c+1/m_0c) unless it hits the m_04-1 wall.
RVA(0x00166ab0, 0x62)
i32 CImageSet2::Query_166ab0(i32 a, i32 b, i32* outA, i32* outB) {
    if (b < m_18 || b > m_20 || a > m_1c) {
        return 0;
    }
    if (a < m_14) {
        *outA = m_14;
        *outB = m_10;
        return 1;
    }
    if (m_1c >= m_04 - 1) {
        return 0;
    }
    *outA = m_1c + 1;
    *outB = m_0c;
    return 1;
}

// 0x166b20 (slot 15): far-x-edge query with a value gate (twin of Query_166ab0).
RVA(0x00166b20, 0x66)
i32 CImageSet2::Query_166b20(i32 a, i32 b, i32 val, i32* out) {
    if (b < m_18 || b > m_20 || a > m_1c) {
        return 0;
    }
    if (a < m_14) {
        if (m_10 != val) {
            return 0;
        }
        *out = m_14;
        return 1;
    }
    if (m_1c >= m_04 - 1) {
        return 0;
    }
    if (m_0c != val) {
        return 0;
    }
    *out = m_1c + 1;
    return 1;
}

// 0x166c60 (slot 16): the far-y-edge query (twin of Query_166ab0 on the m_18/m_20/m_08 row).
RVA(0x00166c60, 0x62)
i32 CImageSet2::Query_166c60(i32 a, i32 b, i32* outA, i32* outB) {
    if (a < m_14 || a > m_1c || b > m_20) {
        return 0;
    }
    if (b < m_18) {
        *outA = m_18;
        *outB = m_10;
        return 1;
    }
    if (m_20 >= m_08 - 1) {
        return 0;
    }
    *outA = m_20 + 1;
    *outB = m_0c;
    return 1;
}

// 0x166cd0 (slot 17): far-y-edge query with a value gate (twin of Query_166c60).
RVA(0x00166cd0, 0x66)
i32 CImageSet2::Query_166cd0(i32 a, i32 b, i32 val, i32* out) {
    if (a < m_14 || a > m_1c || b > m_20) {
        return 0;
    }
    if (b < m_18) {
        if (m_10 != val) {
            return 0;
        }
        *out = m_18;
        return 1;
    }
    if (m_20 >= m_08 - 1) {
        return 0;
    }
    if (m_0c != val) {
        return 0;
    }
    *out = m_20 + 1;
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
    m_width = w;
    i32 h = *p++;
    m_height = h;
    m_heightLog2 = 0;
    m_byteSize = w * h;
    for (; h > 1; h >>= 1) {
        m_heightLog2++;
    }
    if ((1 << m_heightLog2) != w) {
        return 0;
    }
    void* dst = operator new(m_byteSize);
    m_pixels = dst;
    if (dst == 0) {
        return 0;
    }
    memcpy(dst, p, m_byteSize);
    return 1;
}

// 0x166eb0 (slot 12): from the pixel at (x,y), walk UP the column (row -= 1, offset
// -= m_width) while the pixel value stays equal to the start pixel. On the first
// differing row, report it in *outY and its value in *outVal (1); if the top edge
// (y reaches 0) is hit first, return 0. The vertical twin of ScanRunLeft_166e00.
// Accessing m_pixels directly (not via a cached base) makes cl re-read it at the cold
// found block through `this` (kept in ebx), matching retail's register schedule.
RVA(0x00166eb0, 0x6a)
i32 CImageSet3::ScanUp_166eb0(i32 x, i32 y, i32* outY, i32* outVal) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = ((u8*)m_pixels)[off];
    while (y > 0) {
        off -= m_width;
        --y;
        if (((u8*)m_pixels)[off] != target) {
            *outY = y;
            *outVal = ((u8*)m_pixels)[off];
            return 1;
        }
    }
    return 0;
}

// 0x166f20 (slot 13): scan UP for the first row whose column-x pixel EQUALS `val`;
// report the row in *outY. Pointer walk (the value gate uses the free register).
// @early-stop
// ~82% regalloc wall (same-instruction-count; the value gate + coord out compete
// for the callee-saved reg, flipping the this/base spill vs retail). docs/patterns/zero-register-pinning.md.
// @early-stop
// ~82% regalloc wall (same instr count; value gate + coord out compete for the
// callee-saved reg, flipping this/base spill). docs/patterns/zero-register-pinning.md.
RVA(0x00166f20, 0x52)
i32 CImageSet3::ScanUpGate_166f20(i32 x, i32 y, i32 val, i32* outY) {
    u8* p = (u8*)m_pixels + ((y << m_heightLog2) + x);
    while (y > 0) {
        p -= m_width;
        --y;
        if (*p == val) {
            *outY = y;
            return 1;
        }
    }
    return 0;
}

// 0x166f80 (slot 14): scan RIGHT for the first column whose pixel differs from (x,y)'s;
// report it + its value. Stops at the m_width-1 edge.
// @early-stop
// ~78% regalloc wall: the lim (m_width-1) local competes with `this` for a
// callee-saved reg (the lim-free ScanUp is 100%); same instrs, swapped operands.
// @early-stop
// ~78% regalloc wall: the lim (m_width-1) local competes with `this` for a callee-
// saved reg (the lim-free ScanUp is 100%); same instrs, swapped operands.
RVA(0x00166f80, 0x68)
i32 CImageSet3::ScanRight_166f80(i32 x, i32 y, i32* outX, i32* outVal) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = ((u8*)m_pixels)[off];
    i32 lim = m_width - 1;
    while (x < lim) {
        ++x;
        ++off;
        if (((u8*)m_pixels)[off] != target) {
            *outX = x;
            *outVal = ((u8*)m_pixels)[off];
            return 1;
        }
    }
    return 0;
}

// 0x166ff0 (slot 15): scan RIGHT for the first column whose pixel EQUALS `val`; report
// the column in *outX. Pointer walk, stops at the m_width-1 edge.
// @early-stop
// ~72% regalloc wall (lim + pointer-walk + value-gate pressure). Logic byte-faithful.
RVA(0x00166ff0, 0x52)
i32 CImageSet3::ScanRightGate_166ff0(i32 x, i32 y, i32 val, i32* outX) {
    i32 lim = m_width - 1;
    u8* p = (u8*)m_pixels + ((y << m_heightLog2) + x);
    while (x < lim) {
        ++x;
        ++p;
        if (*p == val) {
            *outX = x;
            return 1;
        }
    }
    return 0;
}

// 0x167050 (slot 16): scan DOWN for the first row whose column-x pixel differs from
// (x,y)'s; report it + its value. Stops at the m_height-1 edge.
// @early-stop
// ~70% regalloc wall: retail keeps this in ebp (found-block m_pixels re-read) + spills
// lim; our cl spills this + keeps lim. Same instrs. docs/patterns/zero-register-pinning.md.
RVA(0x00167050, 0x74)
i32 CImageSet3::ScanDown_167050(i32 x, i32 y, i32* outY, i32* outVal) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = ((u8*)m_pixels)[off];
    i32 lim = m_height - 1;
    while (y < lim) {
        off += m_width;
        ++y;
        if (((u8*)m_pixels)[off] != target) {
            *outY = y;
            *outVal = ((u8*)m_pixels)[off];
            return 1;
        }
    }
    return 0;
}

// 0x1670d0 (slot 17): scan DOWN for the first row whose column-x pixel EQUALS `val`;
// report the row in *outY. Offset form, stops at the m_height-1 edge.
// @early-stop
// ~94% regalloc coin-flip (lim vs this callee-saved coloring). Logic byte-faithful.
RVA(0x001670d0, 0x5d)
i32 CImageSet3::ScanDownGate_1670d0(i32 x, i32 y, i32 val, i32* outY) {
    i32 off = (y << m_heightLog2) + x;
    i32 lim = m_height - 1;
    while (y < lim) {
        off += m_width;
        ++y;
        if (((u8*)m_pixels)[off] == val) {
            *outY = y;
            return 1;
        }
    }
    return 0;
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
// drive it per plane (m_planes[i]). Folded from Stub/ApiMiscHelpers.cpp
// (GeoHost_161e80::Build) - the local view IS CLevelPlane.
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

// ===========================================================================
// The trace-discovered CGameLevel cluster (13 leaves). All are plain /O2 /MT
// leaves touching only member offsets, the per-plane objects, and engine sibling
// callees that reloc-mask (no string/global relocations except the jump tables).
// ===========================================================================

// The per-plane object stored in m_planes is CLevelPlane (GameLevel.h). CGameLevel drives
// its Build(coords)/Sync(visitor)/Refresh() slots (unmatched engine __thiscall leaves,
// reloc-masked call sites) and reads its tile grid / extents / name directly.

// ===========================================================================
// IDENTITY (resolved): the movement/collision "target" of every method below is
// the canonical CGameObject (<Gruntz/UserLogic.h>). Proven by the callers:
// CMovingLogic::Update passes its bound object (CUserLogic::m_object, a
// CGameObject*) to MoveToward and reads the same m_moveMode/m_flags/m_carrier
// fields; the trigger ctors initialize the same m_extent/m_area records. The
// former per-family window structs (LevelScroll/ScrollTarget/EditTarget/ProbeObj/
// HoldPayload/ProbeTarget/BPObj + the BP chain shells) are COLLAPSED into it; the
// world chain (CGameObjNode/CGameObjChain/CGameObjWorld) lives beside it in
// UserLogic.h. The level steps each object's m_screenX/m_screenY through the tile
// probes; m_extentL/T/R/B are the object's per-side collision extents,
// m_areaL/T/R its stand/activation box, m_carrier + m_deltaX/Y the platform ride.
// ===========================================================================

// The mode-1..2 sub-dispatch is CGameLevel::MoveKindDispatch12 (@0x1671c0,
// __thiscall), reconstructed further below. ApplyMove's call to it reloc-masks to
// the same address regardless of convention; modeling it as this __stdcall leaf
// gives ApplyMove's surrounding code a closer byte match (94.78%) than the
// literal method-call form (92.61%) - see ApplyMove's @early-stop note.
extern "C" i32 __stdcall MoveSubDispatch12(CGameObject* obj, i32 a, i32 b, i32 c); // @0x1671c0

// ===========================================================================
// The sibling move-leaf bodies (StepAxisLo/Hi, FreeMove, Advance{A,B},
// StepAxisAlt, SpanCheck, + the 15fe40 stand-fit validator). All are plain
// /O2 /MT __thiscall leaves (this=this level), NO relocations: they touch only
// the moving object's m_extentL..B, m_screenX/Y, m_moveMode, m_strideX/Y, the
// level's main plane (+0x5c) tile grid, and the image-set array (+0x4c) -
// dispatching the image set's GetCollisionAt (+0x20) to probe a tile, exactly
// like AxisProbe (@0x161270) inlined.
// ===========================================================================

// The tile probe reads the main plane (CLevelPlane) at its probe offsets: the wrap moduli
// m_wrapW/m_wrapH (+0x30/+0x34, clamp bounds), the log2-tile shifts m_shiftX/m_shiftY
// (+0x8c/+0x90), the column-offset table m_colOffsets (+0x24) and the tile grid
// m_tileGrid (+0x20).

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
            CLevelPlane* pc_ = (LVL)->m_mainPlane;                                                 \
            if (px_ >= pc_->m_wrapW) {                                                             \
                px_ = pc_->m_wrapW - 1;                                                            \
            }                                                                                      \
        }                                                                                          \
        if (py_ < 0) {                                                                             \
            py_ = 0;                                                                               \
        } else {                                                                                   \
            CLevelPlane* pc_ = (LVL)->m_mainPlane;                                                 \
            if (py_ >= pc_->m_wrapH) {                                                             \
                py_ = pc_->m_wrapH - 1;                                                            \
            }                                                                                      \
        }                                                                                          \
        CLevelPlane* pl_ = (LVL)->m_mainPlane;                                                     \
        i32 qx_ = px_ >> pl_->m_shiftX;                                                            \
        i32 qy_ = py_ >> pl_->m_shiftY;                                                            \
        i32 col_ = qx_;                                                                            \
        i32 subX_ = px_ - (qx_ << pl_->m_shiftX);                                                  \
        i32 idx_ = pl_->m_colOffsets[qy_] + col_;                                                  \
        i32 subY_ = py_ - (qy_ << pl_->m_shiftY);                                                  \
        i32 tile_ = pl_->m_tileGrid[idx_];                                                         \
        if (tile_ == TILE_UNINIT || tile_ == TILE_CLEAR) {                                         \
            (RESULT) = kTilePassable;                                                              \
        } else {                                                                                   \
            CImageSet* set_ = (CImageSet*)m_imageSets[tile_ & 0xffff];                             \
            (RESULT) = set_->GetCollisionAt(subX_, subY_);                                         \
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
        CLevelPlane* pc = m_mainPlane;
        if (px >= pc->m_wrapW) {
            px = pc->m_wrapW - 1;
        }
    }
    i32 py = limit;
    if (py < 0) {
        py = 0;
    } else {
        CLevelPlane* pc = m_mainPlane;
        if (py >= pc->m_wrapH) {
            py = pc->m_wrapH - 1;
        }
    }
    CLevelPlane* pl = m_mainPlane;
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
    CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
    return set->GetCollisionAt(subX, subY);
}

// EditSink - the serializer `arg0` of EditDispatch: a polymorphic object whose slots
// +0x2c (read a name into buf) and +0x30 (write buf as a name) are used.
SIZE_UNKNOWN(EditSink);
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
extern i32 __stdcall ResolveLevelName(EditSink* sink, i32 a, i32 b, i32 c);

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
    CLevelPlane* mp;
    if (x < 0) {
        x = 0;
    } else {
        mp = m_mainPlane;
        if (x >= mp->m_width) {
            x = mp->m_width - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        mp = m_mainPlane;
        if (y >= mp->m_height) {
            y = mp->m_height - 1;
        }
    }
    mp = m_mainPlane;
    i32 tile = mp->m_tileGrid[mp->m_colOffsets[y] + x];
    if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
        return 0;
    }
    CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
    return set->GetCollisionAt(0, 0); // slot +0x20, called with (0, 0)
}

// ---------------------------------------------------------------------------
// Three forwarders to a method on the main plane; no-op / 0 when none.
RVA(0x000cedf0, 0xf)
i32 CGameLevel::MainPlaneQueryA() {
    if (m_mainPlane != 0) {
        return (m_mainPlane)->QueryA();
    }
    return 0;
}

RVA(0x000cee10, 0xf)
i32 CGameLevel::MainPlaneQueryB() {
    if (m_mainPlane != 0) {
        return (m_mainPlane)->QueryB();
    }
    return 0;
}

RVA(0x00160ee0, 0xd)
void CGameLevel::MainPlaneNotify() {
    if (m_mainPlane != 0) {
        (m_mainPlane)->Notify();
    }
}

// ---------------------------------------------------------------------------
// ValidateAllPlanes (0x160ef0, ret 4): clear *errOut (when non-null) then run
// ValidateTiles(errOut) on every plane; returns 1 only if all planes validated.
RVA(0x00160ef0, 0x42)
i32 CGameLevel::ValidateAllPlanes(char* errOut) {
    i32 ok = 1;
    if (errOut != 0) {
        *errOut = 0;
    }
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        if (((CLevelPlane*)m_planes[i])->ValidateTiles(errOut) == 0) {
            ok = 0;
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// BuildAllPlanes: copy *coords into m_planeCtx, then Build(coords) on every plane.
RVA(0x0015da80, 0x47)
void CGameLevel::BuildAllPlanes(LevelCoordRect* coords) {
    m_planeCtx = *coords;
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        ((CLevelPlane*)m_planes[i])->Build(coords);
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
            ((CLevelPlane*)m_planes.GetData()[i])->Build(&rect);
            ++i;
        } while (i < m_planes.GetSize());
    }
    return 1;
}

// ---------------------------------------------------------------------------
// SyncToMainIndex: Sync(visitor) on every plane from index 0 through m_mainIndex
// inclusive (nothing when there is no main plane). The first half of the
// non-origin-fixed VisitVisible path, lifted as its own helper.
RVA(0x0015dad0, 0x2c)
void CGameLevel::SyncToMainIndex(void* visitor) {
    i32 i = 0;
    if (m_mainIndex >= 0) {
        do {
            ((CLevelPlane*)m_planes.GetData()[i])->Sync(visitor);
            ++i;
        } while (i <= m_mainIndex);
    }
}

// ---------------------------------------------------------------------------
// SyncAfterMainIndex: Sync(visitor) on every plane after the main index up to the
// plane count. The second half of the non-origin-fixed VisitVisible path.
RVA(0x0015db00, 0x2e)
void CGameLevel::SyncAfterMainIndex(void* visitor) {
    i32 i = m_mainIndex + 1;
    if (i < m_planes.GetSize()) {
        do {
            ((CLevelPlane*)m_planes.GetData()[i])->Sync(visitor);
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
// scheduling/spill wall: the stepping loop's six spilled locals (the per-axis step,
// the running goal, the ok flag) and the abs() lowering schedule into a register
// assignment MSVC reproduces only for one spill order; logic + offsets + CFG are
// exact. Deferred to the final sweep.
RVA(0x0015de40, 0x164)
i32 CGameLevel::MoveToward(CGameObject* target, i32 arg1, i32 arg2, i32 arg3) {
    CGameObject* t = target;
    i32 limX = m_maxStepX;
    i32 limY = m_maxStepY;

    i32 dx = t->m_screenX - arg1;
    if (dx < 0) {
        dx = -dx;
    }
    if (dx <= limX) {
        i32 dy = t->m_screenY - arg2;
        if (dy < 0) {
            dy = -dy;
        }
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
    if (t->m_screenX > arg1) {
        stepX = -stepX;
    }
    i32 stepY = limY;
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

        ok = 1;
        if (t->m_moveMode == kind && (flags & 0x10000) == 0) {
            if (t->m_screenX == goalX && t->m_screenY == arg2) {
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
        CLevelPlane* p = (i >= 0 && i < m_planes.GetSize()) ? (CLevelPlane*)m_planes[i] : 0;
        if (_strcmpi(name, p->m_name) == 0) {
            return (CPlane*)p;
        }
    }
    return 0;
}

// (The world object chain - CGameObjNode/CGameObjChain - is the canonical model
// in <Gruntz/UserLogic.h>; the chain payloads are CGameObjects. The objects'
// +0x74 z-key is read here for the draw ordering - the canonical field is named
// m_latchedAnimId (historical); CheckpointTrigger derives it as
// layer-base + screenY + bias, i.e. a cached z-order key. Rename deferred until
// the UserLogic TU is free.)

// ---------------------------------------------------------------------------
// VisitVisible: z-ordered object render walk. When the level is origin-fixed
// (m_08 & 1) walk ctx's object chain, Draw each object whose z-key is below the
// running plane's z bound, Sync the planes around it; otherwise Sync every plane
// (around the main index) and dispatch ctx's Hook. `visitor` (1st param) is the
// render visitor every Sync/Draw/Hook receives; `ctx` (2nd param) is the chain.
//
// @early-stop
// register-scheduling wall (~92%): the inner draw-gate branch polarity now matches
// retail (`if (depth < cap) Draw; else block` -> jge block, Draw fall-through). Residue
// is the chain cursor's saved-reg shuffle - retail keeps a 2nd copy of `cur` in edx and
// reloads `cap` from spill each iteration; cl keeps cap live in edx and restores via the
// surviving eax. Logic + offsets + CFG exact; allocator coin-flip. Deferred to the final sweep.
RVA(0x0015dc90, 0x141)
void CGameLevel::VisitVisible(void* visitor, CGameObjChain* ctx) {
    // The engine lea's the +0x10 list record's ADDRESS and null-checks it (always
    // live) before loading the head - the sub-struct keeps that byte shape.
    CGameObjChain::List* chain = &ctx->m_list;

    if ((m_08 & 1) && chain != 0 && (m_planes.GetSize() > 0 ? m_planes.GetData()[0] : 0) != 0) {
        ((CLevelPlane*)(m_planes.GetSize() > 0 ? m_planes.GetData()[0] : 0))->Sync(visitor);
        CGameObjNode* node = chain->head;

        i32 i = 1;
        if (m_planes.GetSize() > i) {
            do {
                CLevelPlane* p =
                    (i >= 0 && i < m_planes.GetSize()) ? (CLevelPlane*)m_planes.GetData()[i] : 0;
                i32 zBound = p->m_zBound;
                i32 blocked = 0;
                while (node != 0 && blocked == 0) {
                    CGameObjNode* cur = node;
                    node = node->next;
                    CGameObject* pl = cur->obj;
                    if (pl->m_latchedAnimId < zBound) { // z-key vs the plane's z bound
                        pl->Draw(visitor);
                    } else {
                        node = cur;
                        blocked = 1;
                    }
                }
                ((CLevelPlane*)m_planes.GetData()[i])->Sync(visitor);
                ++i;
            } while (i < m_planes.GetSize());
        }

        while (node != 0) {
            CGameObjNode* cur = node;
            node = node->next;
            cur->obj->Draw(visitor);
        }
        return;
    }

    // --- not origin-fixed: Sync planes around the main index + the ctx hook ---
    i32 idx = 0;
    if (m_mainIndex >= 0) {
        do {
            ((CLevelPlane*)m_planes.GetData()[idx])->Sync(visitor);
            ++idx;
        } while (idx <= m_mainIndex);
    }
    ctx->Hook(visitor);
    i32 j = m_mainIndex + 1;
    if (j < m_planes.GetSize()) {
        do {
            ((CLevelPlane*)m_planes.GetData()[j])->Sync(visitor);
            ++j;
        } while (j < m_planes.GetSize());
    }
}

// ---------------------------------------------------------------------------
// NotifyAllPlanes: Refresh() across every plane.
RVA(0x00160f40, 0x23)
void CGameLevel::NotifyAllPlanes() {
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        ((CLevelPlane*)m_planes[i])->Refresh();
    }
}

// ---------------------------------------------------------------------------
// ApplyMove: drive the +0xe4 edit-state machine. editKind <= 0: nothing; kind 7
// commits the new scroll x/y directly; kinds 1..2 fan to MoveKindDispatch12. Then
// fold flag bits into the result and tag 0x400000 when the scroll did not move.
//
// @early-stop
// call-arg-materialization entropy (~95%): logic/offsets/CFG/__stdcall conv are exact;
// the only residue is the MoveKindDispatch12 call setup - retail interleaves a reload
// between the arg pushes (2 regs), MSVC pre-loads all three (eax/ecx/edx). See
// docs/patterns/pin-local-for-callee-saved-reg.md. Entropy tail; deferred.
RVA(0x00167130, 0x83)
i32 __stdcall ApplyMove(CGameObject* obj, i32 a, i32 b, i32 c) {
    CGameObject* s = obj;
    i32 eax = 0;
    i32 prevX = s->m_screenX;
    i32 prevY = s->m_screenY;
    i32 kind = s->m_moveMode;

    if (kind > 0) {
        if (kind > 2) {
            if (kind == 7) {
                s->m_screenX = a;
                s->m_screenY = b;
            }
        } else {
            eax = MoveSubDispatch12(s, a, b, c);
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

// ---------------------------------------------------------------------------
// MoveKindDispatch12 (@0x1671c0): drive both axes toward (x,y). For the X axis,
// if x is above/below the target's current scrollX call the matching hi/lo stepper
// (which clamps x in place through &x); same for Y; OR the two results. Finally
// commit the (possibly stepped) screen x/y (+0x5c/+0x60) back into the target and
// return the accumulated flag word. this=CGameLevel; `t` is the moved CGameObject
// (verified: CMovingLogic::Update loads m_object->owner->m_level as this and passes
// m_object as the target - see the IDENTITY note above).
RVA(0x001671c0, 0x97)
i32 CGameLevel::MoveKindDispatch12(CGameObject* t, i32 x, i32 y, i32 flags) {
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

// ===========================================================================
// The four axis steppers (MoveStepXHi/XLo/YHi/YLo @0x167260/450/640/830).
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

// MoveStepXHi - 0x167260. Fixed X = high edge (x + axisMid); sweep Y, scan X down.
// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167260, 0x1ef)
i32 CGameLevel::MoveStepXHi(CGameObject* t, i32 x, i32 y, i32* px, i32 flags) {
    i32 xEnd = x + t->m_extentR;
    i32 yHi = t->m_extentB + y;
    i32 yLo = t->m_extentT + y;
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
            CLevelPlane* pc = m_mainPlane;
            if (cx >= pc->m_wrapW) {
                cx = pc->m_wrapW - 1;
            }
        }
        i32 cy = yLo;
        if (cy < 0) {
            cy = 0;
        } else {
            CLevelPlane* pc = m_mainPlane;
            if (cy >= pc->m_wrapH) {
                cy = pc->m_wrapH - 1;
            }
        }
        CLevelPlane* pl = m_mainPlane;
        i32 qx = cx >> pl->m_shiftX;
        i32 qy = cy >> pl->m_shiftY;
        i32 col = qx;
        i32 subX = cx - (qx << pl->m_shiftX);
        i32 idx = pl->m_colOffsets[qy] + col;
        i32 subY = cy - (qy << pl->m_shiftY);
        i32 tile = pl->m_tileGrid[idx];
        if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
            result = kTilePassable;
        } else {
            CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
            result = set->GetCollisionAt(subX, subY);
        }
    }
    if (result == kTileSoft2 && (t->m_flags & 0x400)) {
        result = kTilePassable;
    }
    if (result == kTileSoft || result == kTileSoft2) {
        i32 lo = t->m_screenX + t->m_extentR;
        i32 j = xEnd - 1;
        state |= 0x60000;
        for (; j > lo; j--) {
            if (AxisProbe(j, yLo) == kTilePassable) {
                j -= t->m_extentR;
                goto have_x;
            }
        }
        j = t->m_screenX;
    have_x:
        x = j;
        if (j == t->m_screenX) {
            goto done_eq;
        }
    }
    if (yLo == yHi) {
        yLo++;
    } else {
        yLo += t->m_strideY;
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
    if (BroadPhase(t, x, y) != 0) {
        *px = t->m_screenX;
        return state | 0x22000000;
    }
    *px = x;
    return state;
done_eq:
    *px = t->m_screenX;
    return state;
}

// MoveStepXLo - 0x167450. Fixed X = low edge (x + axisLoA); sweep Y, scan X up.
// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167450, 0x1ef)
i32 CGameLevel::MoveStepXLo(CGameObject* t, i32 x, i32 y, i32* px, i32 flags) {
    i32 xEnd = x + t->m_extentL;
    i32 yHi = t->m_extentB + y;
    i32 yLo = t->m_extentT + y;
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
            CLevelPlane* pc = m_mainPlane;
            if (cx >= pc->m_wrapW) {
                cx = pc->m_wrapW - 1;
            }
        }
        i32 cy = yLo;
        if (cy < 0) {
            cy = 0;
        } else {
            CLevelPlane* pc = m_mainPlane;
            if (cy >= pc->m_wrapH) {
                cy = pc->m_wrapH - 1;
            }
        }
        CLevelPlane* pl = m_mainPlane;
        i32 qx = cx >> pl->m_shiftX;
        i32 qy = cy >> pl->m_shiftY;
        i32 col = qx;
        i32 subX = cx - (qx << pl->m_shiftX);
        i32 idx = pl->m_colOffsets[qy] + col;
        i32 subY = cy - (qy << pl->m_shiftY);
        i32 tile = pl->m_tileGrid[idx];
        if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
            result = kTilePassable;
        } else {
            CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
            result = set->GetCollisionAt(subX, subY);
        }
    }
    if (result == kTileSoft2 && (t->m_flags & 0x400)) {
        result = kTilePassable;
    }
    if (result == kTileSoft || result == kTileSoft2) {
        i32 lo = t->m_screenX + t->m_extentL;
        i32 j = xEnd + 1;
        state |= 0xa0000;
        for (; j < lo; j++) {
            if (AxisProbe(j, yLo) == kTilePassable) {
                j -= t->m_extentL;
                goto have_x;
            }
        }
        j = t->m_screenX;
    have_x:
        x = j;
        if (j == t->m_screenX) {
            goto done_eq;
        }
    }
    if (yLo == yHi) {
        yLo++;
    } else {
        yLo += t->m_strideY;
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
    if (BroadPhase(t, x, y) != 0) {
        *px = t->m_screenX;
        return state | 0x82000000;
    }
    *px = x;
    return state;
done_eq:
    *px = t->m_screenX;
    return state;
}

// MoveStepYHi - 0x167640. Fixed Y = high edge (y + axisHi); sweep X, scan Y down.
// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167640, 0x1eb)
i32 CGameLevel::MoveStepYHi(CGameObject* t, i32 x, i32 y, i32* py, i32 flags) {
    i32 colHi = t->m_extentR + x;
    i32 fixedY = y + t->m_extentB;
    i32 col = t->m_extentL + x;
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
            CLevelPlane* pc = m_mainPlane;
            if (cx >= pc->m_wrapW) {
                cx = pc->m_wrapW - 1;
            }
        }
        i32 cy = fixedY;
        if (cy < 0) {
            cy = 0;
        } else {
            CLevelPlane* pc = m_mainPlane;
            if (cy >= pc->m_wrapH) {
                cy = pc->m_wrapH - 1;
            }
        }
        CLevelPlane* pl = m_mainPlane;
        i32 qx = cx >> pl->m_shiftX;
        i32 qy = cy >> pl->m_shiftY;
        i32 c = qx;
        i32 subX = cx - (qx << pl->m_shiftX);
        i32 idx = pl->m_colOffsets[qy] + c;
        i32 subY = cy - (qy << pl->m_shiftY);
        i32 tile = pl->m_tileGrid[idx];
        if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
            result = kTilePassable;
        } else {
            CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
            result = set->GetCollisionAt(subX, subY);
        }
    }
    if (result == kTileSoft2 && (t->m_flags & 0x400)) {
        result = kTilePassable;
    }
    if (result == kTileSoft || result == kTileSoft2) {
        i32 lo = t->m_screenY + t->m_extentB;
        i32 j = fixedY - 1;
        state |= 0x1020000;
        for (; j > lo; j--) {
            if (AxisProbe(col, j) == kTilePassable) {
                j -= t->m_extentB;
                goto have_y;
            }
        }
        j = t->m_screenY;
    have_y:
        y = j;
        if (j == t->m_screenY) {
            goto done_eq;
        }
    }
    if (col == colHi) {
        col++;
    } else {
        col += t->m_strideX;
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
    if (BroadPhase(t, x, y) != 0) {
        *py = t->m_screenY;
        return state | 0x42000000;
    }
    *py = y;
    return state;
done_eq:
    *py = t->m_screenY;
    return state;
}

// MoveStepYLo - 0x167830. Fixed Y = low edge (y + axisLoB); sweep X, scan Y up.
// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167830, 0x1eb)
i32 CGameLevel::MoveStepYLo(CGameObject* t, i32 x, i32 y, i32* py, i32 flags) {
    i32 colHi = t->m_extentR + x;
    i32 fixedY = y + t->m_extentT;
    i32 col = t->m_extentL + x;
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
            CLevelPlane* pc = m_mainPlane;
            if (cx >= pc->m_wrapW) {
                cx = pc->m_wrapW - 1;
            }
        }
        i32 cy = fixedY;
        if (cy < 0) {
            cy = 0;
        } else {
            CLevelPlane* pc = m_mainPlane;
            if (cy >= pc->m_wrapH) {
                cy = pc->m_wrapH - 1;
            }
        }
        CLevelPlane* pl = m_mainPlane;
        i32 qx = cx >> pl->m_shiftX;
        i32 qy = cy >> pl->m_shiftY;
        i32 c = qx;
        i32 subX = cx - (qx << pl->m_shiftX);
        i32 idx = pl->m_colOffsets[qy] + c;
        i32 subY = cy - (qy << pl->m_shiftY);
        i32 tile = pl->m_tileGrid[idx];
        if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
            result = kTilePassable;
        } else {
            CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
            result = set->GetCollisionAt(subX, subY);
        }
    }
    if (result == kTileSoft2 && (t->m_flags & 0x400)) {
        result = kTilePassable;
    }
    if (result == kTileSoft || result == kTileSoft2) {
        i32 lo = t->m_screenY + t->m_extentT;
        i32 j = fixedY + 1;
        state |= 0x820000;
        for (; j < lo; j++) {
            if (AxisProbe(col, j) == kTilePassable) {
                j -= t->m_extentT;
                goto have_y;
            }
        }
        j = t->m_screenY;
    have_y:
        y = j;
        if (j == t->m_screenY) {
            goto done_eq;
        }
    }
    if (col == colHi) {
        col++;
    } else {
        col += t->m_strideX;
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
    if (BroadPhase(t, x, y) != 0) {
        *py = t->m_screenY;
        return state | 0x12000000;
    }
    *py = y;
    return state;
done_eq:
    *py = t->m_screenY;
    return state;
}

// ===========================================================================
// BroadPhase - 0x167ea0 (__thiscall, ret 0xc). The AABB broad-phase the steppers
// tail into. `t` is the moving CGameObject; it walks the world's object chain
// (this level's m_0c owner -> m_objChain) and, for every other collision-active
// object whose category matches t's mask and whose extents are set (m_extentL !=
// sentinel), tests whether t currently overlaps it. If NOT (a separation on any
// axis) but t's CANDIDATE box (at candX, candY) WOULD overlap, it stores the
// other party in t->m_hitOther and fires t's worker m_collideNotify; on a
// nonzero reply it fires the object's own notify (masks permitting), returns 1.
// (The `(CGameObjWorld*)m_0c` cast is language-forced: the CLoadable base stores
// the owning context as a generic i32 across the whole family.)
// ===========================================================================

// @early-stop
// regalloc coin-flip: retail pins resolvedX in esi from prologue; MSVC5 reuses esi for the scan index (no source lever forces it)
RVA(0x00167ea0, 0x1b9)
i32 CGameLevel::BroadPhase(CGameObject* t, i32 candX, i32 candY) {
    if (!(t->m_flags & 0x100)) {
        return 0;
    }
    CGameObjNode* node = ((CGameObjWorld*)m_0c)->m_objChain->m_list.head;
    if (node == 0) {
        return 0;
    }
    do {
        CGameObjNode* nx = node->next;
        CGameObject* obj = node->obj;
        if (obj != t && (obj->m_flags & 0x100) && (t->m_collMask & obj->m_collCategory)
            && t->m_extentL != AXIS_UNSET && obj->m_extentL != AXIS_UNSET) {
            i32 tLeft = t->m_extentL + t->m_screenX;
            i32 tBot = t->m_extentT + t->m_screenY;
            i32 tRight = t->m_screenX + t->m_extentR;
            i32 tTop = t->m_extentB + t->m_screenY;
            i32 oLeft = obj->m_screenX + obj->m_extentL;
            i32 oBot = obj->m_extentT + obj->m_screenY;
            i32 oTop = obj->m_screenY + obj->m_extentB;
            i32 oRight = obj->m_screenX + obj->m_extentR;
            if (tLeft > oRight || tRight < oLeft || tBot > oTop || tTop < oBot) {
                i32 cLeft = candX + t->m_extentL;
                i32 cRight = t->m_extentR + candX;
                i32 cBot = t->m_extentT + candY;
                i32 cTop = t->m_extentB + candY;
                if (cLeft <= oRight && cRight >= oLeft && cBot <= oTop && cTop >= oBot) {
                    i32 fire;
                    if (t->m_collideWorker != 0) {
                        t->m_hitOther = obj;
                        fire = t->m_collideWorker->m_collideNotify(t);
                    } else {
                        fire = 1;
                    }
                    if (fire != 0) {
                        if (t->m_collMask & obj->m_collCategory) {
                            if (obj->m_collideWorker != 0) {
                                obj->m_hitOther = t;
                                obj->m_collideWorker->m_collideNotify(obj);
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
    return ResolveLevelName(s, arg2, arg2, arg3) != 0 ? 1 : 0;
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
    if (m_08 & 4) {
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

// ===========================================================================
// The four per-brush-kind edit handlers (MoveHandlerA..D). All are __thiscall
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
// MoveHandlerA (kinds 1/2/5): axis-1 step toward a1, axis-2 advance (AdvanceA),
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
i32 CGameLevel::MoveHandlerA(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 result = 0;
    i32 coord = a1;

    if (a1 > t->m_screenX) {
        result = StepAxisLo(t, a1, a2, &coord, a3);
    } else if (a1 < t->m_screenX) {
        result = StepAxisHi(t, a1, a2, &coord, a3);
    }

    if (a2 < t->m_screenY) {
        a2 = AdvanceA(t, coord, a2, a3);
    }

    if (a3 & 1) {
        i32 limit = t->m_extentT + a2 - 1;
        if (AxisProbe(coord, limit) == kTileHard) {
            if (a3 & 0x10) {
                i32 lo = coord;
                i32 hi = coord;
                if (ClampSpan(coord, limit, &lo, &hi) != 0) {
                    coord = (hi + lo) / 2;
                }
            }
            t->m_moveMode = 6;
        }
    } else if (a3 & 2) {
        i32 limit = t->m_extentB + a2 + 2;
        if (AxisProbe(coord, limit) == kTileHard) {
            if (a3 & 0x10) {
                i32 lo = coord;
                i32 hi = coord;
                if (ClampSpan(coord, limit, &lo, &hi) != 0) {
                    coord = (hi + lo) / 2;
                }
            }
            t->m_moveMode = 6;
        }
    } else {
        if (t->m_flags & 0x10) {
            if (HoldMove(t, t->m_carrier, coord, a2, a3) == 0) {
                t->m_moveMode = 4;
            }
        } else {
            a2 = FreeMove(t, coord, a2, a3);
        }
    }

    t->m_screenX = coord;
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
    i32 coord = a1;

    if (a1 > t->m_screenX) {
        result = StepAxisLo(t, a1, a2, &coord, a3);
    } else if (a1 < t->m_screenX) {
        result = StepAxisHi(t, a1, a2, &coord, a3);
    }

    if (a3 & 8) {
        i32 outY = a2;
        if (StepAxisAlt(t, coord, a2, &outY, a3) != 0) {
            a2 = outY;
        }
    }

    if (t->m_moveMode != 1) {
        a2 = AdvanceB(t, coord, a2, a3);
    }

    if (a3 & 1) {
        i32 limit = t->m_extentT + a2 - 1;
        i32 saved = coord;
        if (AxisProbe(coord, limit) == kTileHard) {
            if (a3 & 0x10) {
                i32 lo = saved;
                i32 hi = saved;
                if (ClampSpan(saved, limit, &lo, &hi) != 0) {
                    coord = (hi + lo) / 2;
                }
                t->m_moveMode = 6;
            } else {
                t->m_moveMode = 6;
            }
        }
    }

    if (t->m_moveMode == 1 && coord != savedA1) {
        if (result & 0x20000) {
            result &= 0xfff1ffff;
            if (coord > t->m_screenX) {
                result |= StepAxisLo(t, coord, a2, &coord, a3);
            } else if (coord < t->m_screenX) {
                result |= StepAxisHi(t, coord, a2, &coord, a3);
            }
        }
    }

    t->m_screenX = coord;
    t->m_screenY = a2;
    return result;
}

// ---------------------------------------------------------------------------
// MoveHandlerB (kind 3, also kind 8 down-moves): axis-1 step, axis-2 advance
// (AdvanceA, unconditional), the low AxisProbe + ClampSpan re-bracket, then commit.
//
// @early-stop
// register-scheduling wall: 4 saved regs across StepAxis/AdvanceA/AxisProbe/ClampSpan
// + the spilled bracket slots; logic + offsets + CFG + conventions exact. Deferred.
RVA(0x0015e4b0, 0xf7)
i32 CGameLevel::MoveHandlerB(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 result = 0;
    i32 coord = a1;

    if (a1 > t->m_screenX) {
        result = StepAxisLo(t, a1, a2, &coord, a3);
    } else if (a1 < t->m_screenX) {
        result = StepAxisHi(t, a1, a2, &coord, a3);
    }

    i32 cursor = AdvanceA(t, coord, a2, a3);

    if (a3 & 1) {
        i32 limit = t->m_extentT + cursor - 1;
        if (AxisProbe(coord, limit) == kTileHard) {
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
            t->m_moveMode = 6;
        }
    }

    t->m_screenX = coord;
    t->m_screenY = cursor;
    return result;
}

// ---------------------------------------------------------------------------
// MoveHandlerD (kind 6): drives the axis-2 advance first (AdvanceB on a down-move,
// else AdvanceA), runs the +0x138/+0x140 two-probe (low then high limit, blocking on
// ==3), and on the up-move path a SpanCheck validate that may clamp the cursor below
// the +0x140 high limit; then a final axis-1 step and commit.
//
// @early-stop
// register-scheduling wall: the two-probe + SpanCheck + step tail across 4 saved regs
// + spilled cursor/limit slots; logic + offsets + CFG + sibling conventions exact.
// Deferred to the final sweep.
RVA(0x0015e5b0, 0x162)
i32 CGameLevel::MoveHandlerD(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 result = 0;
    i32 cursor;

    if (t->m_screenY >= a1) {
        cursor = AdvanceB(t, a2, a1, a3);
        if (t->m_moveMode != 1) {
            i32 hi = t->m_extentB + cursor + 1;
            i32 lo = t->m_extentT + cursor - 1;
            if (AxisProbe(a2, lo) != kTileHard && AxisProbe(a2, hi) != kTileHard) {
                t->m_moveMode = 4;
            }
        }
    } else {
        cursor = AdvanceA(t, a1, a2, a3);
        i32 hi = t->m_extentB + cursor + 1;
        i32 lo = t->m_extentT + cursor - 1;
        if (AxisProbe(a2, lo) != kTileHard && AxisProbe(a2, hi) != kTileHard) {
            i32 probe = a2;
            i32 want = (t->m_extentB + cursor + 1) - cursor + t->m_screenY;
            if (SpanCheck(want, t->m_extentB + cursor + 1, probe, &probe) != 0 && probe > cursor) {
                t->m_moveMode = 1;
                cursor = probe - t->m_extentB - 1;
            }
        }
    }

    i32 coord = a2;
    if (a2 > t->m_screenX) {
        result = StepAxisLo(t, a2, cursor, &coord, a3);
    } else if (a2 < t->m_screenX) {
        result = StepAxisHi(t, a2, cursor, &coord, a3);
    }

    t->m_screenX = coord;
    t->m_screenY = cursor;
    return result;
}

// ===========================================================================
// The sibling move leaves the MoveHandlers dispatch into (ascending RVA).
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
i32 CGameLevel::StepAxisLo(CGameObject* t, i32 a1, i32 a2, i32* outX, i32 a3) {
    i32 mid = t->m_extentR + a1;
    i32 lo = t->m_extentT + a2;
    i32 hi = t->m_extentB + a2;
    i32 cur = lo;

    if (lo <= hi) {
        do {
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
i32 CGameLevel::StepAxisHi(CGameObject* t, i32 a1, i32 a2, i32* outX, i32 a3) {
    i32 mid = t->m_extentL + a1;
    i32 lo = t->m_extentT + a2;
    i32 hi = t->m_extentB + a2;
    i32 cur = lo;

    if (lo <= hi) {
        do {
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
i32 CGameLevel::FreeMove(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 mid = t->m_extentR + a1;
    i32 cur = t->m_extentL + a1;
    i32 hiY = t->m_extentB + a2 + 1;

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
// @early-stop
// register-scheduling wall (large /O2 body): two inlined PROBE_TILE copies + two
// AxisProbe gate loops + the strided walk pin the saved regs in an order MSVC
// reproduces only for one spill order; logic + offsets + CFG exact. Deferred.
RVA(0x0015ede0, 0x2a7)
i32 CGameLevel::AdvanceB(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 lo = t->m_extentL + a1;
    i32 mid = t->m_extentR + a1;
    i32 hiY = a2 + t->m_extentB + 1;

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
                i32 floor = t->m_screenY + t->m_extentB;
                if (hiY >= floor) {
                    i32 y = hiY;
                    do {
                        i32 g = AxisProbe(cur, y);
                        if (g != kTileSoft && g != kTileSoft2) {
                            t->m_moveMode = 1;
                            return y - t->m_extentB;
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
                                return (y + 1) - t->m_extentB - 1;
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
i32 CGameLevel::AdvanceA(CGameObject* t, i32 a1, i32 a2, i32 a3) {
    i32 cur = t->m_extentL + a1;
    i32 mid = t->m_extentR + a1;
    i32 ceil = a2 + t->m_extentT - 1;

    if (cur <= mid) {
        do {
            i32 result;
            PROBE_TILE(this, cur, ceil, result);
            if (result == kTileSoft) {
                i32 floor = t->m_screenY + t->m_extentT - 1;
                if (ceil <= floor) {
                    i32 y = ceil;
                    do {
                        if (AxisProbe(cur, y) != kTileSoft) {
                            t->m_moveMode = 4;
                            return y - t->m_extentT;
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
        if (result != kTileHard) {
            *out = cur + 1;
            return 1;
        }
        --cur;
    } while (cur >= c);

    return 0;
}

// ---------------------------------------------------------------------------
// StepAxisAlt (@0x15fdb0): the carrier-latch step. Only runs when a3 bit3 is set;
// walks the world's object chain and, for each candidate of collision category
// 0x80 (carrier/platform), runs the AltStepValidate stand-fit. The first carrier
// that fits is latched (t->m_carrier, mode 1, flags bit4 = riding) and returns 1;
// an exhausted chain returns 0. (CMovingLogic::Update then advances the rider by
// the carrier's m_deltaX/Y each frame.)
RVA(0x0015fdb0, 0x8a)
i32 CGameLevel::StepAxisAlt(CGameObject* t, i32 a1, i32 a2, i32* outY, i32 a3) {
    if ((a3 & 8) == 0) {
        return 0;
    }

    CGameObjNode* node = ((CGameObjWorld*)m_0c)->m_objChain->m_list.head;
    while (node != 0) {
        CGameObjNode* cur = node;
        node = node->next;
        CGameObject* pl = cur->obj;
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
// @early-stop
// register-scheduling wall (~72%): logic + offsets + CFG + the merged epilogue are
// exact; residue is the +0x60(scrollY) spill materialization and the cmpHi==tHi
// branch polarity (retail makes the ok-target fall-through). Deferred to the final sweep.
RVA(0x0015fe40, 0xd4)
i32 CGameLevel::AltStepValidate(CGameObject* t, CGameObject* p, i32 a1, i32 a2, i32* outY, i32 a3) {

    if (p->m_areaL == -1) {
        goto fail;
    }
    if (t->m_extentL == -1) {
        goto fail;
    }
    {
        i32 sy = t->m_screenY;
        if (sy > a2) {
            goto fail;
        }

        i32 boxL = p->m_areaL + p->m_screenX;
        i32 boxR = p->m_areaR + p->m_screenX;
        i32 boxT = p->m_screenY + p->m_areaT;
        i32 tLoA = t->m_extentL + a1;
        i32 tMid = t->m_extentR + a1;
        i32 tHi = t->m_extentB + a2;
        i32 cmpHi = t->m_extentB + sy;

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
        if (cmpHi == tHi) {
            if (tHi != boxT - 1) {
                goto fail;
            }
        } else {
            if (tHi < boxT - 1) {
                goto fail;
            }
        }

        *outY = boxT - t->m_extentB - 1;
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
// the rider's feet (m_extentB + a2) still sit exactly on the stand surface
// (m_areaT-derived row - 1). All field reads; no calls. ret 0x14.
//
// @early-stop
// regalloc/spill wall (~90%): logic + offsets + CFG + the prologue gates + the
// dec/sete-cl result are byte-faithful; residue is one spill choice in the box-overlap
// add chain - retail keeps tMid/tLoA both live (boxL via a combined lea) while our cl
// spills tLoA to [esp+0x24]. Reordering the local computes regresses it (84%); not
// source-steerable. Deferred to the final sweep.
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
    if (p->m_areaL == -1) {
        return 0;
    }
    if (et->m_extentL == -1) {
        return 0;
    }

    i32 ox = p->m_screenX;
    i32 boxL = ox + p->m_areaL;
    i32 boxR = ox + p->m_areaR;
    i32 boxT = p->m_screenY + p->m_areaT;
    i32 hi = et->m_extentB + a2;
    i32 tMid = et->m_extentR + a1;
    i32 tLoA = et->m_extentL + a1;
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
RVA(0x0015ffe0, 0x99)
i32 CGameLevel::ClampSpan(i32 x, i32 y, i32* outLo, i32* outHi) {
    if (x < 0) {
        x = 0;
    } else {
        CLevelPlane* pc = m_mainPlane;
        if (x >= pc->m_wrapW) {
            x = pc->m_wrapW - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        CLevelPlane* pc = m_mainPlane;
        if (y >= pc->m_wrapH) {
            y = pc->m_wrapH - 1;
        }
    }
    CLevelPlane* pl = m_mainPlane;
    i32 qx = x >> pl->m_shiftX;
    i32 alignedX = qx << pl->m_shiftX;
    i32 qy = y >> pl->m_shiftY;
    i32 idx = pl->m_colOffsets[qy] + qx;
    i32 tile = pl->m_tileGrid[idx];
    if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
        return 0;
    }
    CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
    *outLo = alignedX;
    *outHi = alignedX + set->m_width - 1;
    return 1;
}

// ---------------------------------------------------------------------------
// ProbeHeadSoft (@0x160450): probe the tile straight above the object at
// (m_screenX, m_screenY + m_extentT + dy) and return whether it is soft-blocking
// (== kTileSoft). The inlined PROBE_TILE shape; the result==1 test is shared by
// both the tile-hit and empty-tile paths (retail merges the sete). ret 8.
//
// @early-stop
// ~94.9%: logic + offsets + CFG + the inlined PROBE_TILE clamp/shift/dispatch are
// byte-faithful; residue is the clamp branch's free-register assignment - the same
// PROBE_TILE-shape regalloc entropy documented on ProbeColumn/AxisProbe. Not
// source-steerable; deferred to the final sweep.
RVA(0x00160450, 0xd6)
i32 CGameLevel::ProbeHeadSoft(CGameObject* t, i32 dy) {
    i32 px = t->m_screenX;
    i32 py = t->m_screenY + t->m_extentT + dy;
    i32 result;
    PROBE_TILE(this, px, py, result);
    return result == kTileSoft;
}

// ---------------------------------------------------------------------------
// ProbeFeetKind (@0x1608c0): the feet-edge twin of ProbeColumn - probe the tile at
// (m_screenX + dx, m_extentB + m_screenY) and return the image set's GetCollisionAt
// kind raw (0 for an empty/clear tile). The inlined PROBE_TILE shape. ret 8.
//
// @early-stop
// 92.56% - identical shape+% to its twin ProbeColumn (0x160980, extentT vs extentB):
// logic/offsets/CFG/PROBE_TILE clamp-shift-dispatch byte-faithful, residue is the
// clamp branch's free-register coloring (this/o kept in esi vs retail reusing the
// arg1 scratch edx). Same wall as ProbeColumn; deferred to the final sweep.
RVA(0x001608c0, 0xc0)
i32 CGameLevel::ProbeFeetKind(CGameObject* t, i32 dx) {
    i32 px = t->m_screenX + dx;
    i32 py = t->m_extentB + t->m_screenY;
    i32 result;
    PROBE_TILE(this, px, py, result);
    return result;
}

// ---------------------------------------------------------------------------
// ProbeColumn (@0x160980): probe the single tile at the object's top edge
// (m_screenX + dx, m_extentT + m_screenY), clamped into the main plane grid,
// returning the image set's GetCollisionAt (+0x20) dispatch (0 for an
// empty/clear tile). The inlined AxisProbe shape (PROBE_TILE). ret 8.
//
// @early-stop
// regalloc wall (~93%): logic + offsets + CFG + the inlined PROBE_TILE clamp/shift/
// dispatch are byte-faithful; residue is the clamp branch's spill/register assignment
// (the same PROBE_TILE-shape entropy as SpanCheck/AxisProbe). Deferred to the final sweep.
RVA(0x00160980, 0xc0)
i32 CGameLevel::ProbeColumn(CGameObject* t, i32 dx) {
    i32 px = t->m_screenX + dx;
    i32 py = t->m_extentT + t->m_screenY;
    i32 result;
    PROBE_TILE(this, px, py, result);
    return result;
}

// ---------------------------------------------------------------------------
// WalkColumnDown (@0x160a40): ground snap. From the object's feet row (m_extentB +
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
    if (t->m_extentL == AXIS_UNSET) {
        return 0;
    }
    if (m_mainPlane == 0) {
        return 0;
    }

    i32 px = t->m_screenX;
    i32 row = t->m_extentB + t->m_screenY;
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

// ===========================================================================
// Level-management + tile-scan cluster (reconstructed from the CGameLevel .text
// block; RVA-adjacent to the LoadWwd/plane methods above). All are __thiscall
// (this=level), indirect-dispatch helpers (no rel32 caller). The tile-scan
// members drive the same inlined PROBE_TILE tile probe as the move steppers.
// ===========================================================================

// ---------------------------------------------------------------------------
// ReadImageSets (@0x15d790): the image-set descriptor loop (LoadWwd step 7). For
// each of the dir[2] records, ReadImageSet(cursor) builds a CImageSet variant,
// SetAtGrow it into m_imageSets, and advance the cursor by the record's GetStride.
// Returns the count read; -1 on a null cursor/dir or a failed record.
RVA(0x0015d790, 0x8b)
i32 CGameLevel::ReadImageSets(const u32* dir, char* cursor) {
    if (cursor == 0) {
        return -1;
    }
    if (dir == 0) {
        return -1;
    }
    i32 n = 0;
    for (i32 i = 0; (u32)i < dir[2]; i++) {
        CImageSet* set = ReadImageSet(cursor);
        if (set == 0) {
            return -1;
        }
        n++;
        cursor += set->GetStride();
        m_imageSets.SetAtGrow(i, (DWORD)set);
    }
    return n;
}

// ---------------------------------------------------------------------------
// RemovePlane (@0x15db30): delete the plane at `index` (scalar-deleting dtor +
// CDWordArray::RemoveAt). If it was the MAIN plane, promote the last remaining
// plane: clear every plane's MAIN bit, then set m_mainIndex/m_mainPlane to the
// last plane and stamp its MAIN bit. 0 on an invalid index/null plane, else 1.
RVA(0x0015db30, 0xae)
i32 CGameLevel::RemovePlane(i32 index) {
    CLevelPlane* p = (index >= 0 && index < m_planes.GetSize()) ? (CLevelPlane*)m_planes[index] : 0;
    if (p == 0) {
        return 0;
    }
    i32 wasMain = p->m_flags & 1;
    p->dtor(1);
    m_planes.RemoveAt(index, 1);
    if (wasMain) {
        i32 last = m_planes.GetSize() - 1;
        CLevelPlane* lp =
            (last >= 0 && last < m_planes.GetSize()) ? (CLevelPlane*)m_planes[last] : 0;
        if (lp != 0) {
            m_mainIndex = -1;
            m_mainPlane = 0;
            for (i32 i = 0; i < m_planes.GetSize(); i++) {
                ((CLevelPlane*)m_planes[i])->m_flags &= ~1;
            }
            m_mainIndex = last;
            m_mainPlane = lp;
            lp->m_flags |= 1;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15dbe0: MovePlane - move the plane at index `from` to index `to` in the plane
// array (RemoveAt then InsertAt). Rejects a negative `from` or an out-of-range `to`;
// a no-op move (from==to) or missing element returns without touching the array. When
// the moved plane is the MAIN plane, retarget m_mainIndex to `to`. 2 args (ret 8).
RVA(0x0015dbe0, 0xa3)
i32 CGameLevel::MovePlane(i32 from, i32 to) {
    if (from >= 0 && to < m_planes.GetSize()) {
        if (from == to) {
            return 1;
        }
        CLevelPlane* el = (from < m_planes.GetSize()) ? (CLevelPlane*)m_planes[from] : 0;
        if (el != 0) {
            m_planes.RemoveAt(from, 1);
            m_planes.InsertAt(to, (DWORD)el, 1);
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
// @early-stop
// ~95.6%: the top-tested `while (col <= hiX)` form (not `if(col>hiX)return;do{}while`)
// reproduced retail's loop rotation - the `jg exit; jmp into-body; back-edge reloads
// fixedY` layout (was 85.5% with the do-while, which reloaded fixedY every iteration).
// Residual is the prologue arg-load order (a3 vs a2 into ebx/edx) + the shared
// PROBE_TILE Y-clamp mainPlane-temp register (eax vs ecx). Not source-steerable.
RVA(0x0015e9c0, 0x139)
i32 CGameLevel::ScanSpanTop(CGameObject* t, i32 x, i32 y, i32 unused) {
    i32 fixedY = t->m_extentT + y;
    i32 hiX = t->m_extentR + x;
    i32 col = t->m_extentL + x;
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
// (m_screenY + m_extentB) while the tiles stay soft (1/2); the first non-soft
// tile commits *out = row - m_extentB and returns 1. An exhausted scan returns 0.
//
// @early-stop
// register-scheduling wall: PROBE_TILE-shape spill/register entropy (this/limit/row
// slots); logic + offsets + CFG exact. Deferred to the final sweep.
RVA(0x0015f090, 0x127)
i32 CGameLevel::SnapFloorDown(CGameObject* t, i32 x, i32 y, i32* out) {
    i32 limit = t->m_screenY + t->m_extentB;
    for (i32 row = y; row >= limit; row--) {
        i32 result;
        PROBE_TILE(this, x, row, result);
        if (result != kTileSoft && result != kTileSoft2) {
            *out = row - t->m_extentB;
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// SnapCeilUp (@0x15f340): the mirror of SnapFloorDown scanning upward from y to
// (m_screenY + m_extentT - 1) while tiles stay soft (1); the first non-soft tile
// commits *out = row - m_extentT and returns 1. An exhausted scan returns 0.
//
// @early-stop
// register-scheduling wall: PROBE_TILE-shape spill/register entropy; logic +
// offsets + CFG exact. Deferred to the final sweep.
RVA(0x0015f340, 0x124)
i32 CGameLevel::SnapCeilUp(CGameObject* t, i32 x, i32 y, i32* out) {
    i32 limit = t->m_screenY + t->m_extentT - 1;
    for (i32 row = y; row <= limit; row++) {
        i32 result;
        PROBE_TILE(this, x, row, result);
        if (result != kTileSoft) {
            *out = row - t->m_extentT;
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// ProbeSpanHard (@0x15f470): hard-block test across the object's vertical span at
// column x. Probe the tile at the top edge (m_extentT + off - 1); if it is hard-
// blocking (== kTileHard) succeed. Otherwise probe the bottom edge (m_extentB +
// off + 1) and return whether IT is hard-blocking. Two inlined PROBE_TILE copies
// (the top row succeeds early); x is re-clamped for each. ret 0xc.
RVA(0x0015f470, 0x193)
i32 CGameLevel::ProbeSpanHard(CGameObject* t, i32 x, i32 off) {
    i32 py2 = t->m_extentB + off + 1;
    i32 py1 = t->m_extentT + off - 1;
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
// AxisProbe, then run a downward SpanCheck-style scan (from m_screenY+m_extentB+1
// down to headRow) for the first non-hard tile; on a hit past the cursor it turns
// the object mode 1 and re-bases the cursor. Returns the resolved cursor.
//
// @early-stop
// ~83.6%: the `goto done` shared-exit merged retail's four `return cursor` paths onto
// the one 0x15f795 block (cursor in ebp). Residual is regalloc/frame: cursor spills to
// [esp+0x14] (ours) vs [esp+0x28] (retail), and retail recomputes b as
// `screenY - cursor + headRow` reusing headRow while our cl computes it directly (the
// literal `screenY - cursor + headRow` spelling regresses to 82.3% - not steerable).
// AdvanceA + two AxisProbe calls + inlined span scan otherwise byte-exact. Deferred.
RVA(0x0015f610, 0x191)
i32 CGameLevel::ResolveMoveDown(CGameObject* t, i32 x, i32 y, i32 flags) {
    i32 cursor = AdvanceA(t, x, y, flags);
    i32 headRow = t->m_extentB + cursor + 1;
    i32 footRow = t->m_extentT + cursor - 1;
    if (AxisProbe(x, footRow) == kTileHard) {
        goto done;
    }
    if (AxisProbe(x, headRow) == kTileHard) {
        goto done;
    }
    {
        i32 b = t->m_screenY + t->m_extentB + 1;
        if (b > headRow) {
            i32 cur = b - 1;
            if (cur >= headRow) {
                do {
                    i32 result;
                    PROBE_TILE(this, x, cur, result);
                    if (result != kTileHard) {
                        if (cur + 1 > cursor) {
                            t->m_moveMode = 1;
                            cursor = cur + 1 - t->m_extentB - 1;
                        }
                        goto done;
                    }
                    --cur;
                } while (cur >= headRow);
            }
        }
    }
done:
    return cursor;
}

// ---------------------------------------------------------------------------
// ResolveMoveUp (@0x15f7b0): AdvanceB the cursor; unless the object already turned
// mode 1, if neither the foot row (inlined probe) nor the head row (AxisProbe) is
// hard, turn the object mode 4. Returns the cursor.
//
// @early-stop
// ~91.3%: logic byte-faithful (AdvanceB + moveMode gate + footRow inline-probe +
// headRow AxisProbe, verified). Residual is regalloc/frame, NOT reloc: the this/t
// pair lands this->esi,t->edi (ours) vs this->edi,t->esi (retail) - a free-list swap -
// and the mode-1 fast path's `push ebp` is shrink-wrapped past the moveMode test in
// retail but hoisted upfront by our cl (docs/patterns/shrink-wrapped-callee-save-push).
// Neither is source-steerable. Deferred to the final sweep.
RVA(0x0015f7b0, 0x11f)
i32 CGameLevel::ResolveMoveUp(CGameObject* t, i32 x, i32 y, i32 flags) {
    i32 cursor = AdvanceB(t, x, y, flags);
    if (t->m_moveMode != 1) {
        i32 headRow = t->m_extentB + cursor + 1;
        i32 footRow = t->m_extentT + cursor - 1;
        i32 result;
        PROBE_TILE(this, x, footRow, result);
        if (result != kTileHard) {
            if (AxisProbe(x, headRow) != kTileHard) {
                t->m_moveMode = 4;
            }
        }
    }
    return cursor;
}

// ---------------------------------------------------------------------------
// StepGroundDown (@0x15f9f0): probe the foot row (m_extentB+y+2) at x; a hard tile
// returns 1 (with, when arg flags bit4 set, a ClampSpan re-bracket writing the span
// midpoint into *out). A non-hard tile returns 0.
//
// @early-stop
// register-scheduling wall: the inlined PROBE_TILE + ClampSpan bracket + signed-halve
// pin the spill slots; logic + offsets + CFG + the ClampSpan convention exact. Deferred.
RVA(0x0015f9f0, 0x11a)
i32 CGameLevel::StepGroundDown(CGameObject* t, i32 x, i32 y, i32* out, i32 flags) {
    i32 probeY = t->m_extentB + y + 2;
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
// (m_extentT+y-1) at x. Same hard-tile / ClampSpan-midpoint behaviour.
//
// @early-stop
// register-scheduling wall: same PROBE_TILE + ClampSpan shape as StepGroundDown;
// logic + offsets + CFG exact. Deferred to the final sweep.
RVA(0x0015fb10, 0x119)
i32 CGameLevel::StepGroundUp(CGameObject* t, i32 x, i32 y, i32* out, i32 flags) {
    i32 probeY = t->m_extentT + y - 1;
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
//
// @early-stop
// register-scheduling wall: two inlined PROBE_TILE copies pin the spilled this/x/y
// slots; logic + offsets + CFG exact. Deferred to the final sweep.
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
// m_screenY+m_extentB+1); returns 1 if it is soft (1 or 2), else 0. (The retail
// re-probes the same tile per compare - two inlined copies.)
//
// @early-stop
// ~98.6%: the `goto yes` shared-exit reproduced retail's single `return 1` block
// reached by `je` from each probe (was 90.9% with per-probe inline `return 1`).
// Residual is the PROBE_TILE Y-clamp mainPlane-temp register (eax vs ecx) entropy
// tail. Not source-steerable. Deferred to the final sweep.
RVA(0x00160080, 0x187)
i32 CGameLevel::ProbeFootSoft(CGameObject* t, i32 dx) {
    i32 row = t->m_screenY + t->m_extentB + 1;
    i32 r1;
    PROBE_TILE(this, dx + t->m_screenX, row, r1);
    if (r1 == kTileSoft) {
        goto yes;
    }
    i32 r2;
    PROBE_TILE(this, dx + t->m_screenX, row, r2);
    if (r2 == kTileSoft2) {
        goto yes;
    }
    return 0;
yes:
    return 1;
}

// ---------------------------------------------------------------------------
// ProbeFootBlocked (@0x160210): like ProbeFootSoft but returns 1 if the foot tile
// is any blocking kind (soft 1, soft2 2, or hard 3), else 0. Three inlined probes.
//
// @early-stop
// ~82.6% (was 73.9%): the `goto yes` shared-exit fixed the block order (retail's
// single `return 1` reached by `je` from all three probes; the per-probe inline
// `return 1` inverted every branch). Residual is a genuine regalloc THRESHOLD: with
// three probes MSVC pins `this` in the callee-saved ebx (mov ebx,ecx) where retail
// keeps `row` in ebx and spills `this` to [esp+0x10] (reloaded per probe) - the same
// source at two probes (ProbeFootSoft) matches at 98.6%, so it is the 3-probe
// register pressure, not the shape. Not source-steerable. Deferred to the final sweep.
RVA(0x00160210, 0x234)
i32 CGameLevel::ProbeFootBlocked(CGameObject* t, i32 dx) {
    i32 row = t->m_screenY + t->m_extentB + 1;
    i32 r1;
    PROBE_TILE(this, dx + t->m_screenX, row, r1);
    if (r1 == kTileSoft) {
        goto yes;
    }
    i32 r2;
    PROBE_TILE(this, dx + t->m_screenX, row, r2);
    if (r2 == kTileSoft2) {
        goto yes;
    }
    i32 r3;
    PROBE_TILE(this, dx + t->m_screenX, row, r3);
    if (r3 == kTileHard) {
        goto yes;
    }
    return 0;
yes:
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

// ===========================================================================
// The four wall-slide coordinate resolvers (@0x167a20..0x167d80). Each scans one
// axis in one direction from a desired coord toward the object's per-side extent
// limit for the nearest passable (0) tile, returning that tile's coord converted
// back to the object's screen space (coord - extent) - or the object's current
// screen coord when no passable tile is found before the limit. A tight single
// inlined-PROBE_TILE loop.
// ===========================================================================

// @early-stop
// ~93%: the `for` form fixed the exit-block order (jg loop + two distinct screenX
// exits). Residual is two byte-level codegen picks, NOT reloc: (1) the scheduler
// hoists the `x-1` init above the limit computation and materializes col via
// `lea ebx,[edx-1]` (x kept live) instead of retail's `mov ebx,[esp+N]; dec ebx`;
// (2) the PROBE_TILE Y-clamp reads m_mainPlane into eax (our cl) vs ecx (retail) -
// a free-register pick for a dead temp (the X-clamp matches at eax in both). Neither
// is source-steerable. Deferred to the final sweep.
RVA(0x00167a20, 0x11b)
i32 CGameLevel::ResolveRightX(CGameObject* t, i32 x, i32 y) {
    i32 limit = t->m_screenX + t->m_extentR;
    for (i32 col = x - 1; col > limit; col--) {
        i32 result;
        PROBE_TILE(this, col, y, result);
        if (result == kTilePassable) {
            return col - t->m_extentR;
        }
    }
    return t->m_screenX;
}

// @early-stop
// ~93%: `for`-form fixed exit-block order; residual = the x+1 init hoisted above the
// limit compute + the Y-clamp mainPlane-temp register (eax vs ecx). See ResolveRightX.
RVA(0x00167b40, 0x11b)
i32 CGameLevel::ResolveLeftX(CGameObject* t, i32 x, i32 y) {
    i32 limit = t->m_screenX + t->m_extentL;
    for (i32 col = x + 1; col < limit; col++) {
        i32 result;
        PROBE_TILE(this, col, y, result);
        if (result == kTilePassable) {
            return col - t->m_extentL;
        }
    }
    return t->m_screenX;
}

// @early-stop
// ~96.8%: `for`-form fixed exit-block order; residual = the y-1 init hoist + the
// Y-clamp mainPlane-temp register (eax vs ecx). See ResolveRightX.
RVA(0x00167c60, 0x11b)
i32 CGameLevel::ResolveBottomY(CGameObject* t, i32 x, i32 y) {
    i32 limit = t->m_screenY + t->m_extentB;
    for (i32 row = y - 1; row > limit; row--) {
        i32 result;
        PROBE_TILE(this, x, row, result);
        if (result == kTilePassable) {
            return row - t->m_extentB;
        }
    }
    return t->m_screenY;
}

// @early-stop
// ~96.8%: `for`-form fixed exit-block order; residual = the y+1 init hoist + the
// Y-clamp mainPlane-temp register (eax vs ecx). See ResolveRightX.
RVA(0x00167d80, 0x11b)
i32 CGameLevel::ResolveTopY(CGameObject* t, i32 x, i32 y) {
    i32 limit = t->m_screenY + t->m_extentT;
    for (i32 row = y + 1; row < limit; row++) {
        i32 result;
        PROBE_TILE(this, x, row, result);
        if (result == kTilePassable) {
            return row - t->m_extentT;
        }
    }
    return t->m_screenY;
}

// --- class-metadata: the FORCE-REALIZED vtables (were the g_gameLevelVtbl /
// g_imageSet1/2/3Vtbl manual stamps + vtbl-placeholders placeholders). cl now emits
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
RELOC_VTBL(CImageSet, 0x001eaa2c); // vtable reloc-masks a bound datum (dtor-stamp verified)

// tile-collision code names (file-local; see the block near the top of this TU).
#undef kTilePassable
#undef kTileSoft
#undef kTileSoft2
#undef kTileHard
#undef kTileSpecial

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
