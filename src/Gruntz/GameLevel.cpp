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
#include <Io/FileStream.h>   // CFileIO (Open/Read/GetLength/ctor/dtor reloc-masked)
#include <rva.h>

#include <string.h>  // strcpy, memset

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

// The 4-int coordinate/extent record at CGameLevel+0x10.
struct RemusCoords {
    int m_0;
    int m_4;
    int m_8;
    int m_c;
};

// The parse-source object VirtualMethodUnknown3C drives: BeginParse (FUN_00539960
// @0x139960) opens/primes it and returns a handle; EndParse (FUN_005399d0
// @0x1399d0) tears it down. Both are unmatched engine leaves taking the source as
// `this`; declared with no body so the thiscall sites reloc-mask in objdiff.
struct RemusParseSource {
    int  BeginParse();
    void EndParse();
};

// UnknownChild - placeholder for whatever class lives in the pointer arrays
// at +0x38 and +0x4c. Only vtable slot 4 (+0x04, virtual Release(1)) is used.
class UnknownChild {
public:
    virtual void Dummy();
    virtual void Release(int arg);
};

// The two-phase vftables. The inlined RemusBase ctor (in GameLevel.h) stamps the
// base vftable; the derived CGameLevel ctor below stamps the derived one after the
// three array members are constructed. Both stores are reloc-masked DIR32.
DATA(0x1efc30)
extern void *g_severusWorkerBaseVtbl;   // base (SeverusWorker) vftable
DATA(0x1f0150)
extern void *g_gameLevelVtbl;           // derived CGameLevel vftable

static inline void StampLevelVtbl(CGameLevel *o) { *(void **)o = &g_gameLevelVtbl; }

// Stamps the shared +0xb0..+0xdc "default parameters" block. Defined inline so it
// folds into each method exactly as the retail compiler emitted the block inline.
static inline void StampParamBlock(CGameLevel *o)
{
    *(int*)((char*)o + 0xb0) = 500;
    *(int*)((char*)o + 0xb4) = 250;
    *(int*)((char*)o + 0xb8) = 1000;
    *(int*)((char*)o + 0xbc) = 1000;
    *(int*)((char*)o + 0xc0) = 250;
    *(int*)((char*)o + 0xc4) = 125;
    *(int*)((char*)o + 0xc8) = 1600;
    *(int*)((char*)o + 0xcc) = 1200;
    *(int*)((char*)o + 0xd0) = 2560;
    *(int*)((char*)o + 0xd4) = 1920;
    *(int*)((char*)o + 0xd8) = 768;
    *(int*)((char*)o + 0xdc) = 576;
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
RVA(0x15ccd0, 0x118)
CGameLevel::CGameLevel(int a1, int a2, int a3)
    : RemusBase(a1, a2, a3)
{
    *(int*)((char*)this + 0x64) = 0x40;
    *(int*)((char*)this + 0x68) = 0x40;
    *(int*)((char*)this + 0xb4) = 250;
    *(int*)((char*)this + 0xc0) = 250;
    *(int*)((char*)this + 0xb8) = 1000;
    *(int*)((char*)this + 0xbc) = 1000;

    StampLevelVtbl(this);
    *(int*)((char*)this + 0x10) = (int)0x80000000;
    *(int*)((char*)this + 0x5c) = 0;
    *(int*)((char*)this + 0x60) = -1;
    *(int*)((char*)this + 0xac) = 0;
    *(int*)((char*)this + 0xb0) = 500;
    *(int*)((char*)this + 0xc4) = 125;
    *(int*)((char*)this + 0xc8) = 1600;
    *(int*)((char*)this + 0xcc) = 1200;
    *(int*)((char*)this + 0xd0) = 2560;
    *(int*)((char*)this + 0xd4) = 1920;
    *(int*)((char*)this + 0xd8) = 768;
    *(int*)((char*)this + 0xdc) = 576;
}

RVA(0x15d280, 0x279)
int CGameLevel::LoadWwd(WwdHeader* hdr)
{
    Reset();                                  // vtable +0x44

    if (hdr->wwdSignature > 0x5f4)            // signature must be <= 1524
        return 0;

    // Copy the 1524-byte header into the level object (rep movs 0x17d dwords).
    m_header = *hdr;

    char* block;
    char* ehAlloc = 0;                        // inflate buffer tracked by the EH state

    if (hdr->flags & 0x2)                     // COMPRESS: inflate the main block
    {
        unsigned int allocSize =
            hdr->mainBlockLength + hdr->wwdSignature + 0x40;
        char* buf = (char*)operator new(allocSize);
        if (buf == 0)
            return 0;

        block = (char*)WwdFile_InflateMainBlock((WwdHeader*)hdr, (Bytef*)buf,
                                                allocSize - 0x20);
        if (block == 0)
        {
            operator delete(buf);
            return 0;
        }
        ehAlloc = buf;
    }
    else
    {
        block = (char*)hdr;                   // uncompressed: planes follow in place
    }

    strcpy(m_levelName, hdr->levelName);      // inline strlen + rep movs
    m_flags = hdr->flags;
    m_checksum = hdr->checksum;

    // --- plane loop ---------------------------------------------------------
    char* cursor = block + hdr->planesOffset;
    unsigned int i = 0;
    if (hdr->numPlanes != 0)
    {
        do
        {
            if (ReadPlane(cursor, block, &m_planeCtx) == 0)
                goto fail;
            ++i;
            cursor += 0xa0;                   // WwdPlaneHeader stride
        } while (i < hdr->numPlanes);
    }

    // --- image-set descriptors ---------------------------------------------
    if (hdr->tileDescriptionsOffset != 0)
    {
        char* rec = block + hdr->tileDescriptionsOffset;
        // (the target re-tests the cursor against the record header before the
        // loop; the descriptor count lives in the descriptor block header.)
        unsigned int count = *(unsigned int*)(rec + 0x8);
        unsigned int j = 0;
        while (j < count)
        {
            CImageSet* set = ReadImageSet(rec + 0x20);
            if (set == 0)
                goto fail;
            ++j;
            rec += set->GetStride();          // vtable +0x24 stride advance
            m_imageSets.SetAtGrow(j - 1, (DWORD)set);
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
        if (mp->m_flags & 1)
        {
            mp->m_scaledX = (float)startX;
            mp->m_scaledY = (float)startY;
        }
        else
        {
            mp->m_scaledX = (float)startX * mp->m_scaleX;
            mp->m_scaledY = (float)startY * mp->m_scaleY;
        }
        RecomputePlaneCoords(mp);

        // Re-derive the start coords from the main plane's origin for the rest.
        int ox = m_mainPlane->m_originX;
        int oy = m_mainPlane->m_originY;
        int i2 = 0;
        while (i2 < m_planes.GetSize())       // GetSize() == the plane count
        {
            if (i2 != m_mainIndex)
            {
                CPlane* p = (CPlane*)m_planes[i2];
                if (p->m_flags & 1)
                {
                    p->m_scaledX = (float)ox;
                    p->m_scaledY = (float)oy;
                }
                else
                {
                    p->m_scaledX = (float)ox * p->m_scaleX;
                    p->m_scaledY = (float)oy * p->m_scaleY;
                }
                RecomputePlaneCoords(p);
            }
            ++i2;
        }
    }

    if (ehAlloc != 0)
        operator delete(ehAlloc);
    return 1;

fail:
    if (ehAlloc != 0)
        operator delete(ehAlloc);
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
RVA(0x161190, 0x1f)
int CGameLevel::VirtualMethodUnknown14()
{
    if (*(int*)((char*)this + 0x10) == (int)0x80000000)
        goto fail;
    if (*(int*)((char*)this + 0x0c) == 0)
        goto fail;
    if (*(int*)((char*)this + 0x04) != -1)
        return 1;

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
RVA(0x15d030, 0x8f)
int CGameLevel::VirtualMethodUnknown34(int arg0, int arg1)
{
    *(int*)((char*)this + 0x10) = 0;
    *(int*)((char*)this + 0x14) = 0;
    *(int*)((char*)this + 0x18) = arg0 - 1;
    *(int*)((char*)this + 0x1c) = arg1 - 1;
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
RVA(0x15d500, 0x127)
int CGameLevel::VirtualMethodUnknown40(const char *path)
{
    CFileIO file;

    if (!file.Open(path, 0, 0))
        return 0;

    void *buf = operator new(file.GetLength());
    if (!buf)
        return 0;

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
RVA(0x15d630, 0x41)
int CGameLevel::VirtualMethodUnknown3C(RemusParseSource *arg)
{
    int handle = arg->BeginParse();
    if (handle == 0)
        return 0;
    if (Vfunc38(handle) == 0) {
        arg->EndParse();
        return 0;
    }
    arg->EndParse();
    return 1;
}



// @confidence: high
// @source: tomalla
// @stub
RVA(0x1611c0, 0x1e)
void CGameLevel::Stub_1611c0() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x1611e0, 0x82)
void CGameLevel::Stub_1611e0() {}






// ---------------------------------------------------------------------------
// Like Unknown44 plus resets the sentinel and zeroes the WwdHeader buffer.
// ---------------------------------------------------------------------------
RVA(0x15d1f0, 0x87)
int CGameLevel::VirtualMethodUnknown1C()
{
    int i;
    for (i = 0; i < *(int*)((char*)this + 0x3c); i++) {
        UnknownChild *child = (UnknownChild *)(*(void***)((char*)this + 0x38))[i];
        if (child)
            child->Release(1);
    }
    ((CDWordArray *)((char *)this + 0x34))->SetSize(0, -1);
    for (i = 0; i < *(int*)((char*)this + 0x50); i++) {
        UnknownChild *child = (UnknownChild *)(*(void***)((char*)this + 0x4c))[i];
        if (child)
            child->Release(1);
    }
    ((CDWordArray *)((char *)this + 0x48))->SetSize(0, -1);
    *(int*)((char*)this + 0x10) = (int)0x80000000;
    *(int*)((char*)this + 0x5c) = 0;
    *(int*)((char*)this + 0x60) = -1;
    memset((char*)this + 0xe0, 0, 1524);
    return 0;
}

// ---------------------------------------------------------------------------
// Releases all child pointers, resets both CDWordArrays, clears members.
// ---------------------------------------------------------------------------
RVA(0x15d680, 0x71)
void CGameLevel::VirtualMethodUnknown44()
{
    int i;
    for (i = 0; i < *(int*)((char*)this + 0x3c); i++) {
        UnknownChild *child = (UnknownChild *)(*(void***)((char*)this + 0x38))[i];
        if (child)
            child->Release(1);
    }
    ((CDWordArray *)((char *)this + 0x34))->SetSize(0, -1);
    for (i = 0; i < *(int*)((char*)this + 0x50); i++) {
        UnknownChild *child = (UnknownChild *)(*(void***)((char*)this + 0x4c))[i];
        if (child)
            child->Release(1);
    }
    ((CDWordArray *)((char *)this + 0x48))->SetSize(0, -1);
    *(int*)((char*)this + 0x5c) = 0;
    *(int*)((char*)this + 0x60) = -1;
}

// ---------------------------------------------------------------------------
// Returns constant 0x19 (25) — a type-tag or enum identifier.
// ---------------------------------------------------------------------------
RVA(0x1611b0, 0x6)
int CGameLevel::VirtualMethodUnknown20()
{
    return 0x19;
}

// --- restored: matching's RemusCoords sibling definitions (do not drop) ---
// ---------------------------------------------------------------------------
// As Unknown24 but dispatches the +0x40 sibling virtual.
RVA(0x15cdf0, 0xb8)
int CGameLevel::VirtualMethodUnknown2C(int arg1, RemusCoords *coords)
{
    *(RemusCoords*)((char*)this + 0x10) = *coords;
    StampParamBlock(this);
    if (Vfunc40(arg1) == 0) {
        Vfunc1C();
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// As Unknown24 but dispatches the +0x3c sibling virtual.
RVA(0x15ceb0, 0xb8)
int CGameLevel::VirtualMethodUnknown28(int arg1, RemusCoords *coords)
{
    *(RemusCoords*)((char*)this + 0x10) = *coords;
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
RVA(0x15cf70, 0xb8)
int CGameLevel::VirtualMethodUnknown24(int arg1, RemusCoords *coords)
{
    *(RemusCoords*)((char*)this + 0x10) = *coords;
    StampParamBlock(this);
    if (Vfunc38(arg1) == 0) {
        Vfunc1C();
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Loads the +0x10 record from *coords, stamps the param block, returns 1.
RVA(0x15d0d0, 0x99)
int CGameLevel::VirtualMethodUnknown30(RemusCoords *coords)
{
    *(RemusCoords*)((char*)this + 0x10) = *coords;
    StampParamBlock(this);
    return 1;
}
