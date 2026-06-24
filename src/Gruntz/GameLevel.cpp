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
// The CDDrawLevelData/Remus methods, merged in here as CGameLevel.
//
// CGameLevel is the same class the engine handles via the obfuscated name
// CDDrawLevelData (its vtable slot 0x38 is CGameLevel::LoadWwd). The methods
// below were reconstructed as CDDrawLevelData::* and are moved here VERBATIM
// onto CGameLevel. Where a method touches a member that GameLevel.h's CGameLevel
// does NOT already expose at the exact offset (the +0x10 coordinate record, the
// +0x38/+0x4c child-pointer arrays and their +0x3c/+0x50 counts, +0x04, +0x0c,
// the +0xb0..+0xdc default-parameter block, the +0xe0 WwdHeader buffer), the
// access is written as a raw offset cast on `this` so codegen is byte-identical
// regardless of GameLevel.h member naming.
//
// The class carries a 4-int coordinate/extent record at +0x10 and a shared
// "default parameters" block at +0xb0..+0xdc that several methods stamp with the
// same constants (also written by the ctor):
//     +0xb0 = 500  +0xb4 = 250  +0xb8 = 1000 +0xbc = 1000
//     +0xc0 = 250  +0xc4 = 125  +0xc8 = 1600 +0xcc = 1200
//     +0xd0 = 2560 +0xd4 = 1920 +0xd8 = 768  +0xdc = 576
// ===========================================================================

// (RemusCoords - the 4-int coord record at +0x10 - is defined in GameLevel.h.)

// The parse-source object VirtualMethodUnknown3C drives: BeginParse (FUN_00539960
// @0x139960) opens/primes it and returns a handle; EndParse (FUN_005399d0
// @0x1399d0) tears it down. Both are unmatched engine leaves taking the source as
// `this`; declared with no body so the thiscall sites reloc-mask in objdiff.
struct RemusParseSource {
    int BeginParse();
    void EndParse();
};

// UnknownChild - placeholder for whatever class lives in the pointer arrays
// at +0x38 and +0x4c. Only vtable slot 4 (+0x04, virtual Release(1)) is used.
class UnknownChild {
public:
    virtual void Dummy();
    virtual void Release(int arg);
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
    unsigned int flags; // +0x08
    char pad_c[0x10 - 0x0c];
    float scaledX; // +0x10
    float scaledY; // +0x14
    char pad_18[0x30 - 0x18];
    int tilesWide; // +0x30
    int tilesHigh; // +0x34
    char pad_38[0x40 - 0x38];
    int originX; // +0x40
    int originY; // +0x44
    int extentX; // +0x48
    int extentY; // +0x4c
    char pad_50[0x70 - 0x50];
    int viewW;   // +0x70
    int viewH;   // +0x74
    int anchorX; // +0x78
    int anchorY; // +0x7c
    char pad_80[0x84 - 0x80];
    int intX; // +0x84
    int intY; // +0x88

    void RecomputePlaneCoords();
};

// The two-phase vftables. The inlined RemusBase ctor (in GameLevel.h) stamps the
// base vftable; the derived CGameLevel ctor below stamps the derived one after the
// three array members are constructed. Both stores are reloc-masked DIR32.
DATA(0x001efc30)
extern void* g_severusWorkerBaseVtbl; // base (SeverusWorker) vftable
DATA(0x001f0150)
extern void* g_gameLevelVtbl; // derived CGameLevel vftable
// The base-subobject vftable the destructor restores after the member dtors run
// (RemusBase::~RemusBase's vptr store - a different table from the base CTOR's).
DATA(0x001e8cb4)
extern void* g_remusBaseDtorVtbl;

// The three CImageSet variant vftables stamped by ReadImageSet (kind 1/2/3). Their
// contents are UNMATCHED engine code, so the factory stamps the RETAIL tables by
// address (reloc-masked DIR32) rather than letting the compiler emit a divergent
// vtable. (Transitional manual-stamp workaround per matcher doctrine.)
DATA(0x001f0198)
extern void* g_imageSet1Vtbl; // kind 1 (0x10-byte variant)
DATA(0x001f01e0)
extern void* g_imageSet2Vtbl; // kind 2 (0x24-byte variant)
DATA(0x001f0228)
extern void* g_imageSet3Vtbl; // kind 3 (0x18-byte variant)

static inline void StampLevelVtbl(CGameLevel* o) {
    *(void**)o = &g_gameLevelVtbl;
}
static inline void StampRemusBaseDtorVtbl(CGameLevel* o) {
    *(void**)o = &g_remusBaseDtorVtbl;
}

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
// CGameLevel ("UnknownRemus") constructor. Three args (ret 0xc): they land at
// +0x4, +0x8, +0xc. Inlined base ctor (RemusBase, in the header) stamps the
// SeverusWorker base vftable @0x5efc30 and the args, then the three MFC arrays at
// +0x20/+0x34/+0x48 are constructed, then the derived CGameLevel vftable @0x5f0150
// is stamped and the +0x10 sentinel, the +0x5c/+0x60 main-plane fields, the
// +0x64/+0x68 pair (0x40), and the shared +0xb0..+0xdc default-parameter block are
// written. Carries the /GX EH frame because the three array members are
// destructible.
//
// RESIDUE (~89%, NOT a logic/offset/type/CFG error): the body is byte-for-byte
// identical to retail (both two-phase vptr stores, both 0x40/0xfa/0x3e8/param-block
// constants, the EH frame, the ret 0xc, every member offset) EXCEPT two pure
// compiler-internal scheduling choices: (1) MSVC's funcinfo EH-state numbering base
// is shifted by one (retail tags the three array ctors 0/1/2; this build uses the
// -1 entry state for the first, then 0/1) and (2) one immediate (0xfa) lands in a
// different register (eax vs ecx) because `this` is reloaded for the fs:0 restore
// one slot earlier here. Logic + all offsets + the two-phase construction + CFG +
// the EH frame are exact; this is the documented store-scheduling / EH-state-base
// entropy plateau (matching-patterns.md §entropy, .claude/agents/orchestrator.md §2a/§8).
RVA(0x0015ccd0, 0x118)
CGameLevel::CGameLevel(int a1, int a2, int a3) : RemusBase(a1, a2, a3) {
    m_64 = 0x40;
    m_68 = 0x40;
    m_b4 = 250;
    m_c0 = 250;
    m_b8 = 1000;
    m_bc = 1000;

    StampLevelVtbl(this);
    m_planeCtx.m_0 = (int)0x80000000;
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
int CGameLevel::LoadWwd(WwdHeader* hdr) {
    Reset(); // vtable +0x44

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
    unsigned int* pflags = &hdr->flags;

    if (*pflags & 0x2) // COMPRESS: inflate the main block
    {
        unsigned int allocSize = hdr->mainBlockLength + hdr->wwdSignature + 0x40;
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

    int result = 0; // image-set result (the >=0 success / -1 failure sentinel)

    // --- plane loop ---------------------------------------------------------
    char* cursor = block + hdr->planesOffset;
    unsigned int i = 0;
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
            int n = 0;
            int j = 0;
            while ((unsigned int)j < *(unsigned int*)(rec + 0x8)) {
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
        int startX = hdr->startX;
        int startY = hdr->startY;
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
        int ox = m_mainPlane->m_originX;
        int oy = m_mainPlane->m_originY;
        int i2 = 0;
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
// Remus adds a +0x10 sentinel check before the common parent/status predicate.
RVA(0x00161190, 0x1f)
int CGameLevel::VirtualMethodUnknown14() {
    if (m_planeCtx.m_0 == (int)0x80000000) {
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
// Zeroes the first two ints of the +0x10 record, stores (arg0-1)/(arg1-1) into
// the last two, stamps the param block, returns 1.
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
RVA(0x0015d030, 0x8f)
int CGameLevel::VirtualMethodUnknown34(int arg0, int arg1) {
    m_planeCtx.m_0 = 0;
    m_planeCtx.m_4 = 0;
    m_planeCtx.m_8 = arg0 - 1;
    m_planeCtx.m_c = arg1 - 1;
    StampParamBlock(this);
    return 1;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs (merged from UnknownRemus).
// -------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// VirtualMethodUnknown40 (vtable +0x40): open the named file, slurp it whole into
// a heap buffer, and hand the buffer to the +0x38 load virtual (the same slot
// LoadWwd dispatches). Returns 1 on success, 0 on any failure. The local CFileIO
// + the heap buffer are both freed on every exit, which is why the TU carries the
// /GX EH frame. NOTE: the file Read's byte count is DISCARDED (no compare to the
// length) - only the +0x38 virtual's result decides success.
RVA(0x0015d500, 0x127)
int CGameLevel::VirtualMethodUnknown40(const char* path) {
    CFileIO file;

    if (!file.Open(path, 0, 0)) {
        return 0;
    }

    void* buf = operator new(file.GetLength());
    if (!buf) {
        return 0;
    }

    file.Read(buf, file.GetLength());
    if (Vfunc38((int)buf) == 0) {
        operator delete(buf);
        return 0;
    }
    operator delete(buf);
    return 1;
}

// ---------------------------------------------------------------------------
// VirtualMethodUnknown3C (vtable +0x3c): drive a parse/load through `arg`. Begin
// it (BeginParse on arg); if that fails, return 0. Else feed its handle to the
// +0x38 load virtual and finish the parse (EndParse on arg) regardless, returning
// the +0x38 result (1/0). BeginParse/EndParse are unmatched engine leaves on the
// arg object (reloc-masked thiscall).
RVA(0x0015d630, 0x41)
int CGameLevel::VirtualMethodUnknown3C(RemusParseSource* arg) {
    int handle = arg->BeginParse();
    if (handle == 0) {
        return 0;
    }
    if (Vfunc38(handle) == 0) {
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
void* CGameLevel::ScalarDtor(unsigned int flags) {
    this->~CGameLevel(); // call ??1CGameLevel
    if (flags & 1) {
        operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// Destructor: stamp the derived vftable, run the level cleanup, let the three
// array members destruct (reverse construction order), then ~RemusBase restores
// the base subobject (resets m_04/m_flags/m_0c + the base dtor vftable). The
// destructible array members give the /GX EH frame.
RVA(0x001611e0, 0x82)
CGameLevel::~CGameLevel() {
    StampLevelVtbl(this);     // derived vftable @0x5f0150 (dtor entry)
    VirtualMethodUnknown1C(); // level cleanup (releases children, clears the header)
    // m_imageSets / m_planes / m_array20 auto-destruct here; ~RemusBase follows.
}

// ---------------------------------------------------------------------------
// Like Unknown44 plus resets the sentinel and zeroes the WwdHeader buffer.
// ---------------------------------------------------------------------------
RVA(0x0015d1f0, 0x87)
int CGameLevel::VirtualMethodUnknown1C() {
    int i;
    for (i = 0; i < m_planes.GetSize(); i++) {
        UnknownChild* child = (UnknownChild*)m_planes.GetData()[i];
        if (child) {
            child->Release(1);
        }
    }
    m_planes.SetSize(0, -1);
    for (i = 0; i < m_imageSets.GetSize(); i++) {
        UnknownChild* child = (UnknownChild*)m_imageSets.GetData()[i];
        if (child) {
            child->Release(1);
        }
    }
    m_imageSets.SetSize(0, -1);
    m_planeCtx.m_0 = (int)0x80000000;
    m_mainPlane = 0;
    m_mainIndex = -1;
    memset(&m_header, 0, 1524);
    return 0;
}

// ---------------------------------------------------------------------------
// Releases all child pointers, resets both CDWordArrays, clears members.
// ---------------------------------------------------------------------------
RVA(0x0015d680, 0x71)
void CGameLevel::VirtualMethodUnknown44() {
    int i;
    for (i = 0; i < m_planes.GetSize(); i++) {
        UnknownChild* child = (UnknownChild*)m_planes.GetData()[i];
        if (child) {
            child->Release(1);
        }
    }
    m_planes.SetSize(0, -1);
    for (i = 0; i < m_imageSets.GetSize(); i++) {
        UnknownChild* child = (UnknownChild*)m_imageSets.GetData()[i];
        if (child) {
            child->Release(1);
        }
    }
    m_imageSets.SetSize(0, -1);
    m_mainPlane = 0;
    m_mainIndex = -1;
}

// ---------------------------------------------------------------------------
// Returns constant 0x19 (25) — a type-tag or enum identifier.
// ---------------------------------------------------------------------------
RVA(0x001611b0, 0x6)
int CGameLevel::VirtualMethodUnknown20() {
    return 0x19;
}

// --- restored: matching's RemusCoords sibling definitions (do not drop) ---
// ---------------------------------------------------------------------------
// As Unknown24 but dispatches the +0x40 sibling virtual.
RVA(0x0015cdf0, 0xb8)
int CGameLevel::VirtualMethodUnknown2C(int arg1, RemusCoords* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    if (Vfunc40(arg1) == 0) {
        Vfunc1C();
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// As Unknown24 but dispatches the +0x3c sibling virtual.
RVA(0x0015ceb0, 0xb8)
int CGameLevel::VirtualMethodUnknown28(int arg1, RemusCoords* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    if (Vfunc3C(arg1) == 0) {
        Vfunc1C();
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Loads the +0x10 record from *coords, stamps the param block, then dispatches
// the +0x38 sibling virtual with arg1. On a 0 result it runs the +0x1c hook and
// returns 0; otherwise returns 1.
RVA(0x0015cf70, 0xb8)
int CGameLevel::VirtualMethodUnknown24(int arg1, RemusCoords* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    if (Vfunc38(arg1) == 0) {
        Vfunc1C();
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Loads the +0x10 record from *coords, stamps the param block, returns 1.
RVA(0x0015d0d0, 0x99)
int CGameLevel::VirtualMethodUnknown30(RemusCoords* coords) {
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
// The three CImageSet variants the factory allocates. Each is a non-polymorphic
// shell whose INLINE ctor manually stamps the matching external vftable (and
// zeroes its count/cursor fields), so `new CImageSetN` lowers to exactly the
// retail `operator new(size); if (p) { stamp }` shape - the allocation result
// stays in eax across the field stores, then folds into the shared merge. The
// padding pins each size: kind 1 = 0x10, kind 2 = 0x24, kind 3 = 0x18.
struct CImageSet1 {
    CImageSet1() {
        *(void**)this = &g_imageSet1Vtbl;
        m_04 = 0;
    }
    void* m_vtbl; // +0x00
    int m_04;     // +0x04
    char pad_8[0x10 - 0x08];
};
struct CImageSet2 {
    CImageSet2() {
        *(void**)this = &g_imageSet2Vtbl;
        m_04 = 0;
    }
    void* m_vtbl; // +0x00
    int m_04;     // +0x04
    char pad_8[0x24 - 0x08];
};
struct CImageSet3 {
    CImageSet3() {
        *(void**)this = &g_imageSet3Vtbl;
        m_04 = 0;
        m_14 = 0;
    }
    void* m_vtbl; // +0x00
    int m_04;     // +0x04
    char pad_8[0x14 - 0x08];
    int m_14; // +0x14
};

RVA(0x0015d820, 0xa3)
CImageSet* CGameLevel::ReadImageSet(void* record) {
    if (record == 0) {
        return 0;
    }
    CImageSet* set;
    switch (*(int*)record) {
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

    if (set->Parse(record) != 0) {
        return set;
    }
    if (set != 0) {
        set->Release(1);
    }
    return 0;
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
    unsigned int flags = p->flags;
    int wrapX = flags & 4;

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
    int wrapY = flags & 8;
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
    int ix = (int)p->scaledX;
    p->intX = ix;
    int iy = (int)p->scaledY;
    p->intY = iy;

    int ox = ix - p->anchorX;
    p->originX = ox;
    if (ox < 0) {
        if (wrapX) {
            p->originX = p->tilesWide + ox;
        } else {
            p->originX = 0;
        }
    }

    int oy = iy - p->anchorY;
    p->originY = oy;
    if (oy < 0) {
        if (wrapY) {
            p->originY = p->tilesHigh + oy;
        } else {
            p->originY = 0;
        }
    }

    // --- derive the far tile extents (clamped, unless wrapping) ---------------
    int ex = p->viewW + p->originX - 1;
    int ey = p->viewH + p->originY - 1;
    p->extentX = ex;
    p->extentY = ey;
    if (ex >= p->tilesWide && wrapX == 0) {
        int over = ex - p->tilesWide + 1;
        p->extentX = ex - over;
        p->originX = p->originX - over;
    }
    if (ey >= p->tilesHigh && wrapY == 0) {
        int over = ey - p->tilesHigh + 1;
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
    unsigned int flags; // +0x08
    char pad_c[0x20 - 0x0c];
    int* tileBase; // +0x20
    int* rowOfs;   // +0x24
    int width;     // +0x28
    int height;    // +0x2c
    char pad_30[0x74 - 0x30];
    int limit; // +0x74
    char pad_78[0x80 - 0x78];
    int cap; // +0x80
    char pad_84[0xb4 - 0x84];
    char name[4]; // +0xb4

    void Build(RemusCoords* coords); // @0x161e80 (ret 4)
    void Sync(int arg);              // @0x162010 (ret 4)
    void Refresh();                  // @0x163670 (ret)
};

// Three zero-arg __thiscall methods on the main plane the forwarders tail into.
struct MainPlane {
    int QueryA();  // @0x163300
    int QueryB();  // @0x163370
    void Notify(); // @0x163420
};

// __strcmpi (CRT) - reloc-masked. Declared with no header to keep the cdecl shape.
extern "C" int __cdecl _strcmpi(const char*, const char*);

// LevelScroll - the scroll-state record the edit-state methods manipulate: the
// +0x5c/+0x60 scroll x/y, +0x64/+0x68 limits, +0x08 flags and +0xe4 edit-state
// brush-kind all live on the same object that elsewhere is the level container.
// Accessed via raw offsets so the codegen is naming-independent (the same offsets
// are typed differently by the level-load methods).
struct LevelScroll {
    char pad_0[0x08];
    unsigned int flags; // +0x08
    char pad_c[0x5c - 0x0c];
    int scrollX; // +0x5c
    int scrollY; // +0x60
    int limitX;  // +0x64
    int limitY;  // +0x68
    char pad_6c[0xe4 - 0x6c];
    int editKind; // +0xe4
};

// The edit-state sub-dispatch leaves, all __stdcall(target, a1, a2, a3) returning a
// state-flag word (kinds 1..2 -> ScrollKindDispatch12). Reloc-masked engine leaves.
extern "C" int __stdcall ScrollKindDispatch12(LevelScroll* lvl, int a, int b, int c); // @0x1671c0
extern int __stdcall EditHandlerA(LevelScroll*, int, int, int);                       // @0x15e130
extern int __stdcall EditHandlerB(LevelScroll*, int, int, int);                       // @0x15e4b0
extern int __stdcall EditHandlerC(LevelScroll*, int, int, int);                       // @0x15e2f0
extern int __stdcall EditHandlerD(LevelScroll*, int, int, int);                       // @0x15e5b0

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
    virtual void GetName(char* buf, int cap); // +0x2c
    virtual void SetName(char* buf, int cap); // +0x30
};

// Resolve a level/name to a tile id (returns nonzero on success). __stdcall. @0x163710
extern int __stdcall ResolveLevelName(void* sink, int a, int b, int c);

// ---------------------------------------------------------------------------
// PointInBounds (free cdecl): tile (x, y) inside the [minX,maxX) x [minY,maxY)
// half-open box (RemusCoords minX/minY/maxX/maxY at +0/+4/+8/+0xc). ret.
RVA(0x0006b330, 0x2a)
int CGameLevel::PointInBounds(const RemusCoords* r, int x, int y) {
    if (x < r->m_8 && x >= r->m_0 && y < r->m_c && y >= r->m_4) {
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// LookupTile: clamp (x, y) into the main plane's tile grid, fetch the tile id from
// its row-indexed tile map, and (when not the empty/clear sentinel) dispatch the
// referenced image set's slot +0x20 with (0, 0). ret 8.
RVA(0x00082600, 0x73)
int CGameLevel::LookupTile(int x, int y) {
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
    int tile = mp->tileBase[mp->rowOfs[y] + x];
    if (tile == (int)0xeeeeeeee || tile == -1) {
        return 0;
    }
    CImageSet* set = (CImageSet*)m_imageSets[tile & 0xffff];
    return set->dummy8(0, 0); // slot +0x20, called with (0, 0)
}

// ---------------------------------------------------------------------------
// Three forwarders to a method on the main plane; no-op / 0 when none.
RVA(0x000cedf0, 0xf)
int CGameLevel::MainPlaneQueryA() {
    if (m_mainPlane != 0) {
        return ((MainPlane*)m_mainPlane)->QueryA();
    }
    return 0;
}

RVA(0x000cee10, 0xf)
int CGameLevel::MainPlaneQueryB() {
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
void CGameLevel::BuildAllPlanes(RemusCoords* coords) {
    m_planeCtx = *coords;
    for (int i = 0; i < m_planes.GetSize(); i++) {
        ((LevelPlane*)m_planes[i])->Build(coords);
    }
}

// ---------------------------------------------------------------------------
// FindPlaneByName: case-insensitive search for the plane named `name`; null if none.
// ---------------------------------------------------------------------------
// ClampScroll: drive EditSwitch toward (arg1, arg2) on `target`, never moving more
// than this level's per-axis step limits (m_64/m_68) at once. A direct call when
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
int CGameLevel::ClampScroll(void* target, int arg1, int arg2, int arg3) {
    LevelScroll* t = (LevelScroll*)target;
    int limX = m_64;
    int limY = m_68;

    int dx = t->scrollX - arg1;
    if (dx < 0) {
        dx = -dx;
    }
    if (dx <= limX) {
        int dy = t->scrollY - arg2;
        if (dy < 0) {
            dy = -dy;
        }
        if (dy <= m_68) {
            return EditSwitch(target, arg1, arg2, arg3);
        }
    }

    if (t->flags & 0x10) {
        return EditSwitch(target, arg1, arg2, arg3);
    }

    int kind = t->editKind;
    if (kind == 7) {
        return EditSwitch(target, arg1, arg2, arg3);
    }

    // --- incremental stepping toward (arg1, arg2) ---------------------------
    int stepX = limX;
    int goalX = arg1;
    if (t->scrollX > arg1) {
        stepX = -stepX;
    }
    int stepY = limY;
    if (t->scrollY > arg2) {
        stepY = -stepY;
    }

    int ok = 1;
    do {
        int nx = stepX + t->scrollX;
        if (stepX > 0) {
            if (nx > goalX) {
                nx = goalX;
            }
        } else {
            if (nx < goalX) {
                nx = goalX;
            }
        }
        int ny = stepY + t->scrollY;
        if (stepY > 0) {
            if (ny > arg2) {
                ny = arg2;
            }
        } else {
            if (ny < arg2) {
                ny = arg2;
            }
        }

        int flags = EditSwitch(target, nx, ny, arg3);

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
    for (int i = 0; i < m_planes.GetSize(); i++) {
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
    virtual void v28(int arg);  // +0x28
    virtual void Draw(int arg); // +0x2c
    char pad_4[0x74 - 0x04];
    int depth; // +0x74
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
    virtual void Hook(int arg); // +0x28
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
// register-scheduling wall (~86%): the interleaved object-chain walk + per-plane Sync
// loop pins ebp/esi/ebx across calls in an order MSVC reproduces only for one spilling
// of the chain cursor; logic + offsets + CFG are exact. Deferred to the final sweep.
RVA(0x0015dc90, 0x141)
void CGameLevel::VisitVisible(void* visitor, int ctx) {
    VisitCtx* c = (VisitCtx*)ctx;
    VisitChain* chain = &c->m_chain;

    if ((m_flags & 1) && chain != 0 && (m_planes.GetSize() > 0 ? m_planes.GetData()[0] : 0) != 0) {
        ((LevelPlane*)(m_planes.GetSize() > 0 ? m_planes.GetData()[0] : 0))->Sync((int)visitor);
        ObjNode* node = chain->head;

        int i = 1;
        if (m_planes.GetSize() > i) {
            do {
                LevelPlane* p =
                    (i >= 0 && i < m_planes.GetSize()) ? (LevelPlane*)m_planes.GetData()[i] : 0;
                int cap = p->cap;
                int blocked = 0;
                while (node != 0 && blocked == 0) {
                    ObjNode* cur = node;
                    node = node->next;
                    ObjPayload* pl = cur->obj;
                    if (pl->depth >= cap) {
                        node = cur;
                        blocked = 1;
                    } else {
                        pl->Draw((int)visitor);
                    }
                }
                ((LevelPlane*)m_planes.GetData()[i])->Sync((int)visitor);
                ++i;
            } while (i < m_planes.GetSize());
        }

        while (node != 0) {
            ObjNode* cur = node;
            node = node->next;
            cur->obj->Draw((int)visitor);
        }
        return;
    }

    // --- not origin-fixed: Sync planes around the main index + the ctx hook ---
    int idx = 0;
    if (m_mainIndex >= 0) {
        do {
            ((LevelPlane*)m_planes.GetData()[idx])->Sync((int)visitor);
            ++idx;
        } while (idx <= m_mainIndex);
    }
    c->Hook((int)visitor);
    int j = m_mainIndex + 1;
    if (j < m_planes.GetSize()) {
        do {
            ((LevelPlane*)m_planes.GetData()[j])->Sync((int)visitor);
            ++j;
        } while (j < m_planes.GetSize());
    }
}

// ---------------------------------------------------------------------------
// NotifyAllPlanes: Refresh() across every plane.
RVA(0x00160f40, 0x23)
void CGameLevel::NotifyAllPlanes() {
    for (int i = 0; i < m_planes.GetSize(); i++) {
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
int __stdcall CGameLevel::ApplyScroll(CGameLevel* lvl, int a, int b, int c) {
    LevelScroll* s = (LevelScroll*)lvl;
    int eax = 0;
    int prevX = s->scrollX;
    int prevY = s->scrollY;
    int kind = s->editKind;

    if (kind > 0) {
        if (kind > 2) {
            if (kind == 7) {
                s->scrollX = a;
                s->scrollY = b;
            }
        } else {
            eax = ScrollKindDispatch12(s, a, b, c);
        }
    }

    if (eax & 0x20000) {
        eax |= 0x10000;
    }
    unsigned int f = s->flags;
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
int CGameLevel::EditDispatch(void* sink, int arg1, int arg2, int arg3) {
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
int CGameLevel::EditSwitch(void* target, int a1, int a2, int a3) {
    if (m_flags & 4) {
        return ApplyScroll((CGameLevel*)target, a1, a2, a3);
    }

    LevelScroll* s = (LevelScroll*)target;
    int eax = 0;
    int kind = s->editKind;
    int prevX = s->scrollX;
    int prevY = s->scrollY;

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
    unsigned int f = s->flags;
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
