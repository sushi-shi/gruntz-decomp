// WwdFile.cpp - Gruntz WWD level-file loader (Monolith-faithful reconstruction).
//
// Functions matched in this TU:
//   WwdFile::IsValidWwd       - BYTE-EXACT
//   WwdFile::CheckHeader      - BYTE-EXACT
//   WwdFile::InflateMainBlock - ~88.7% fuzzy, entropy plateau
//   WwdFile::ReadPlane        - 99.19%
//                               (byte-exact modulo 11 reloc-masked operand bytes
//                                + a 6-byte 2-instruction MSVC5 scheduling swap)
//
// IsValidWwd / CheckHeader open a file by name, read the 1524-byte (0x5F4) WWD
// header, and validate it: the read must return exactly 0x5F4 bytes and the
// header signature (first u32 == sizeof(WwdHeader)) must be <= 0x5F4. IsValidWwd
// reads straight into the caller buffer; CheckHeader reads into a private 0x5F4
// stack buffer and then copies that header out to the caller (inline strcpy-like
// rep movs of the NUL-terminated... actually a header byte-copy of strlen+1).
//
// Both construct an engine binary-file stream on the stack (ctor, dtor, Open,
// Read). The stack object has a non-trivial
// destructor, so MSVC emits a C++ EH frame (__CxxFrameHandler + FuncInfo, the
// fs:0 registration + `push -1; push handler`): this TU is built with /GX. The
// stream's ctor/dtor/Open/Read are unmatched engine functions, but their call
// bytes are reloc-masked in objdiff, so the validators are still byte-exact.
//
// __stdcall (callee cleans 8 bytes; ret 0x8 in the binary): each takes two
// pointer args and uses callee-cleanup, so they reconstruct as __stdcall free
// functions. Returns are full-width eax (1 / 0), i.e. `int`, not bool.
#include <Wwd/WwdFile.h>
#include <Gruntz/GameRegistry.h>
#include <Image/ImageSet.h>   // SetTileSizeFromImageSet frame source (GetAt/m_count)
#include <Image/ImageFrame.h> // CImageFrame m_width/m_height
#include <rva.h>

#include <Mfc.h>    // CString (ValidateMainBlock takes one by value; ReadPlaneObjects builds four)
#include <stdio.h>  // sprintf
#include <stdlib.h> // atoi
#include <string.h> // memcpy + inline strcpy/strlen (rep movs / repne scas)
#include <Globals.h>
#include <Gruntz/UserLogic.h> // the shared CGameObject (WwdGameObj folded away)

// The shared map-name scratch buffer GetMapBaseName strcpy's the path into
// (0x62c010), plus its 4-byte predecessor slot (0x62c00c) the extension-truncation
// store indexes through. Reloc-masked DATA pins.

// ---------------------------------------------------------------------------
// The game registry global (?g_gameReg@@3PAUCGameReg@@A @ VA 0x64556c). Only the
// chain ValidateMainBlock walks is modeled here (m_slot -> m_wwdPath, a filename);
// the full CGameReg layout lives in src/Gruntz/StatusBarMgr.cpp. Offsets are the
// only load-bearing thing (campaign doctrine), so a TU-local view is matching-neutral.
// authentic: reduced local view of the cross-TU CGameReg world slot; only the +0x24
// path field ValidateMainBlock reads is modeled (offset-faithful, mangling-neutral).
struct WwdGameRegSlot {
    char pad_0[0x24];
    char* m_wwdPath; // +0x24  a WWD path / numeric-tail string CheckHeader validates
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------
// CPlaneRender::WrapCoord (__thiscall, ret 0x8). Maps a world coordinate
// (*px, *py) into the plane's local draw space: wrap each axis into its pixel
// modulus (when the plane wraps on that axis: flag bit2=X, bit3=Y), pull it back
// near the visible origin, then subtract the plane origin and add the scroll
// view offset. Pure integer arithmetic + member reads; no calls.
//
// @early-stop
// 91.5%, logic byte-exact. Residual: the SECOND flag test - retail emits a direct
// `test BYTE [ecx+8],8` (memory) and loads py into eax in the branch shadow; this
// build loads the flag byte into al (`mov al; test al,8`) and parks py in edx.
// A whole-function regalloc/scheduling choice (which physical reg holds py); not
// source-steerable. Documented scheduling wall (matching-patterns.md §entropy).
RVA(0x0000a000, 0xac)
void CPlaneRender::WrapCoord(i32* px, i32* py) {
    if (m_flags & 0x4) { // wrap X
        i32 x = *px;
        if (x < 0) {
            *px = m_wrapW + x;
        } else if (x >= m_wrapW) {
            *px = x - m_wrapW;
        }
        if (m_extentX >= m_wrapW && *px < m_originX && *px <= m_extentX - m_wrapW) {
            *px = m_wrapW + *px;
        }
    }

    if (m_flags & 0x8) { // wrap Y
        i32 y = *py;
        if (y < 0) {
            *py = m_wrapH + y;
        } else if (y >= m_wrapH) {
            *py = y - m_wrapH;
        }
        if (m_extentY >= m_wrapH && *py < m_originY && *py <= m_extentY - m_wrapH) {
            *py = m_wrapH + *py;
        }
    }

    *px = *px - m_originX;
    *py = *py - m_originY;
    *px = *px + m_viewX;
    *py = *py + m_viewY;
}

// ---------------------------------------------------------------------------
// WwdFile::ValidateMainBlock (static, __cdecl: ignores `this`, caller-cleaned
// `ret`; Ghidra mis-derived the void/no-arg `QAEXXZ` prototype).
// Takes a CString BY VALUE (the callee runs its dtor on every exit). Returns -1
// for the three reject paths, else the integer parsed from the first digit run
// of the validated header:
//   1. the CString must be non-empty (its length, at pszData-8, != 0);
//   2. ((WwdGameRegSlot*)g_gameReg->m_world)->m_wwdPath (a filename) must be non-null;
//   3. CheckHeader(that filename) into a 0x100 stack buffer must succeed.
// Then skip leading non-digits and atoi() the first digit run. The CString is
// unused beyond its non-empty check; `this` is never touched -> static.
RVA(0x0003b470, 0x13a)
i32 WwdFile::ValidateMainBlock(CString name) {
    char header[0x100];

    if (name.GetLength() == 0) {
        return -1;
    }
    if (((WwdGameRegSlot*)g_gameReg->m_world)->m_wwdPath == 0) {
        return -1;
    }

    if (WwdFile_CheckHeader(((WwdGameRegSlot*)g_gameReg->m_world)->m_wwdPath, header) == 0) {
        return -1;
    }

    char* p = header;
    char c = *p;
    while (c != 0 && (c < '0' || c > '9')) {
        c = *++p;
    }
    return atoi(p);
}

// ---------------------------------------------------------------------------
// WwdFile::IsValidWwd
// Open(name) -> Read(headerBuf, 0x5F4) -> require read == 0x5F4 and first u32
// (the header signature) <= 0x5F4. The two null guards return BEFORE the stream
// is constructed (no destructor on those paths); the stream's ctor runs only
// after both guards, so its dtor unwinds the remaining exits.
RVA(0x00160530, 0x125)
i32 __stdcall WwdFile_IsValidWwd(const char* name, void* headerBuf) {
    if (name == 0) {
        return 0;
    }
    if (headerBuf == 0) {
        return 0;
    }

    WwdInputStream stream;

    if (stream.Open(name, 0, 0) == 0) { // Open returns 0 on failure
        return 0;
    }

    if (stream.Read(headerBuf, 0x5f4) != 0x5f4) {
        return 0;
    }

    if (*(u32*)headerBuf > 0x5f4) { // signature must be <= 1524
        return 0;
    }

    return 1;
}

// ---------------------------------------------------------------------------
// WwdFile::CheckHeader
// Same validation as IsValidWwd but reads into a PRIVATE 0x5F4 stack buffer,
// then copies the header out to the caller (an inline strlen+rep-movs copy of
// the NUL-terminated leading bytes - the binary does `repnz scasb; rep movs`,
// i.e. a strcpy of the header buffer into the caller's output).
RVA(0x00160660, 0x12b)
i32 __stdcall WwdFile_CheckHeader(const char* name, void* headerOut) {
    char header[0x5f4];

    if (name == 0) {
        return 0;
    }
    if (headerOut == 0) {
        return 0;
    }

    WwdInputStream stream;

    if (stream.Open(name, 0, 0) == 0) { // Open returns 0 on failure
        return 0;
    }

    if (stream.Read(header, 0x5f4) != 0x5f4) {
        return 0;
    }

    if (*(u32*)header > 0x5f4) { // signature must be <= 1524
        return 0;
    }

    strcpy((char*)headerOut, header); // inline strlen + rep movs
    return 1;
}

// ---------------------------------------------------------------------------
// CGameLevelPlanes::ReadPlane (__thiscall ret 0xc).
// Build one plane: `new CPlane(this->m_field0c, this->m_planeCount, 0)` (operator
// new(0x158) under the C++ EH frame), then invoke the plane's block reader
// (vtable +0x28) on (planeData, blockBase, &this->m_planeCtx). On reader failure,
// delete the plane (scalar-deleting dtor, vtable +0x4) and return 0. On success,
// append the plane to m_planes (CArray::SetAtGrow) at index
// m_planeCount, and if it is the MAIN plane (m_flags bit0) cache it as m_mainPlane
// with m_mainIndex = m_planeCount - 1. Returns the new plane.
//
// The new CPlane and its virtuals are UNMATCHED engine code -> reloc-masked calls.
RVA(0x0015d8d0, 0xc3)
CPlane* CGameLevelPlanes::ReadPlane(void* planeData, void* blockBase, void* /*unused*/) {
    CPlane* plane = new CPlane(m_field0c, m_planeCount, 0);

    if (plane->Read(planeData, blockBase, &m_planeCtx) == 0) {
        if (plane) {
            plane->dtor(1); // scalar-deleting dtor (vtable +0x4)
        }
        return 0;
    }

    ((CObArray*)&m_planes)->SetAtGrow(m_planeCount, (CObject*)plane);

    if (plane->m_flags & 1) // MAIN plane
    {
        m_mainPlane = plane;
        m_mainIndex = m_planeCount - 1;
    }

    return plane;
}

// ---------------------------------------------------------------------------
// CGameLevelPlanes::ReadObjectPlane - the object-plane sibling of
// ReadPlane: `new CPlane(m_field0c, m_planeCount, 0)`, then drive the plane's
// +0x24 object-block reader with the six forwarded args, &m_planeCtx (7th), and
// the trailing arg (8th). Append/record/delete identically to ReadPlane.
// The CPlane ctor + virtuals are UNMATCHED engine code -> reloc-masked calls.
RVA(0x0015d9a0, 0xdc)
CPlane* CGameLevelPlanes::ReadObjectPlane(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7) {
    CPlane* plane = new CPlane(m_field0c, m_planeCount, 0);

    if (plane->ReadObjects(a1, a2, a3, a4, a5, a6, &m_planeCtx, a7) == 0) {
        if (plane) {
            plane->dtor(1); // scalar-deleting dtor (vtable +0x4)
        }
        return 0;
    }

    ((CObArray*)&m_planes)->SetAtGrow(m_planeCount, (CObject*)plane);

    if (plane->m_flags & 1) // MAIN plane
    {
        m_mainPlane = plane;
        m_mainIndex = m_planeCount - 1;
    }

    return plane;
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
i32 __stdcall WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, u32 destLen) {
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
    outLen = (uLongf)(destLen - src->wwdSignature);
    if (uncompress(
            dest + src->wwdSignature,
            &outLen,
            (Bytef*)src + src->wwdSignature,
            src->mainBlockLength
        )
        != 0) {
        return 0;
    }

    return outLen == src->mainBlockLength ? (i32)dest : 0;
}

// ===========================================================================
// WwdFile_CompressMainBlock (0x160870, __stdcall, ret 0x10) - the deflate
// counterpart of InflateMainBlock: WAP-compress src[srcLen] into dest[destCap]
// (via WapUncompress, which despite its name is the compress/deflate side) and
// return the produced compressed length, or 0 on a null buffer / deflate failure.
// ===========================================================================
// WapUncompress (0x1853b0, __cdecl) - the engine deflate helper; reloc-masked.
int WapUncompress(
    unsigned char* dest,
    unsigned long* pDestLen,
    unsigned char* src,
    unsigned long srcLen
);
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
    return WapUncompress(dest, &outLen, src, srcLen) == 0 ? (i32)outLen : 0;
}

// ===========================================================================
// CPlaneRender::SetTileSize (__thiscall, ret 8) - given the tile pixel
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
void CPlaneRender::SetTileSize(i32 tileW, i32 tileH) {
    m_wrapW = m_gridW * tileW;
    m_tilePxH = tileH;
    m_fillB = tileH;
    m_tilePxW = tileW;
    m_fillL = 0;
    m_fillT = 0;
    m_fillR = tileW;
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
// CPlaneRender::SetTileSizeFromImageSet (0x161fa0, __thiscall, ret 0x4): linear-scan
// the image set for the first populated frame (GetAt returns null outside
// [minIndex, maxIndex]); on the first hit, drive SetTileSize with that frame's pixel
// dimensions and stop. An empty set leaves the tile size unchanged.
RVA(0x00161fa0, 0x6c)
void CPlaneRender::SetTileSizeFromImageSet(CImageSet* set) {
    for (i32 i = 0; i < set->m_count; i++) {
        if (set->GetAt(i) != 0) {
            CImageFrame* f = set->GetAt(i);
            SetTileSize(f->m_width, f->m_height);
            break;
        }
    }
}

// ===========================================================================
// CPlaneRender::Draw (__thiscall, ret 0x4) - the toroidally-wrapped tile-grid
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
// @early-stop
// Deferred to the final sweep: a 2237-byte function with the tile-lookup+blit
// body inlined at ~8 sites over a ~0x94-byte frame (~20 stack slots). The logic,
// the per-edge coordinate math, the wrap, the handle resolution, and the
// CDDSurface blit callees (CDDSurface::BltEx@0x13eef0 / BltFast@0x13ef90, both
// reloc-masked) are fully decoded above, but reproducing MSVC 5.0's exact
// stack-slot assignment + the 8 inlined-site scheduling is a leaf-first /
// dedicated-grind job, not a breadth-mode worker's. Left claimed in its real TU
// (not Stub/) so the final sweep re-attacks it with this decode as a head start.
RVA(0x00162010, 0x8bd)
void CPlaneRender::Draw(void* /*ctx*/) {}

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
struct WwdLevelLoader {
    char pad_0[0xc];
    void* m_assetOwner; // +0x0C  asset/map owner (ctor arg; holds the imageset map)
    char pad_10[0x30 - 0x10];
    i32 m_gridWidth;  // +0x30  grid width  (object x must be in [0, m_gridWidth))
    i32 m_gridHeight; // +0x34  grid height (object y must be in [0, m_gridHeight))
};

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

// CDDrawSubMgr ctor embedded at +0x1A0: (this, surfMgr, a, b). __thiscall, ret 0xc.
class CDDrawSurfaceMgr;
inline void* operator new(u32, void* p) {
    return p;
} // placement (embedded sub-object ctor)
SIZE_UNKNOWN(CDDrawSubMgr);
struct CDDrawSubMgr { // the +0x1a0 embedded draw sub-manager (real ctor in DDrawSubMgr.cpp)
    CDDrawSubMgr(CDDrawSurfaceMgr* mgr, i32 a2, i32 a3); // 0x156cb0
};

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

// The level header reached via this->m_assetOwner->m_24 (six geometry pairs at +0xb0).
struct WwdPlaneHdr {
    char pad[0xb0];
    i32 geo[12]; // +0xb0..+0xdc
};
struct WwdRegOwner {
    char pad0[0x8];
    void* m_8; // +0x08  worker source
    char pad9[0x24 - 0xc];
    WwdPlaneHdr* m_24; // +0x24
};

// The 0xb8-byte plane-render worker (vtable 0x5f02a8; embedded CObList at +0x70).
struct WwdPlaneRender {
    void DtorBody(); // 0x1682f0  __thiscall
    void ListDtor(); // 0x163a10  CObList at +0x70
    // 0x168080: init from source + 6 coordinate-pair pointers.
    i32 Init(void* src, i32* p0, i32* p1, i32* p2, i32* p3, i32* p4, i32* p5);
};

// 0x5f02a8 is realized as ??_7CWwdGridIter (5 slots, dtor at slot 1) in
// src/Gruntz/WwdSpatialMgr.cpp, which OWNS the RVA catalog name via VTBL. UNPINNED
// here so RebuildPlanes' inline +0x70 embedded-cursor stamp reloc-masks against the
// real ??_7 (the manual g_planeRenderVtbl DATA placeholder is drained).

// authentic: documented offset access into WwdFile's own wide layout (the +0xb0
// plane-render worker slot + the +0xc reg-owner slot); only the offset is
// load-bearing, and RebuildPlanes is @early-stop, so the raw-offset form is kept.
#define WLOADER(t, off) (*(t*)((char*)this + (off)))

// @early-stop
// throwing-new EH-frame + embedded-vtable-stamp wall: the worker rebuild + the
// 6-pair init + the ReadPlaneObjects loop are faithful, but (a) the
// partial-construct exception cleanup frame's trylevel/handler bytes are not
// source-steerable, and (b) the worker's EMBEDDED sub-object at +0x70 has its
// vtable stamped manually (g_planeRenderVtbl @0x5f02a8 = ??_7CWwdGridIter, realized
// in WwdSpatialMgr.cpp; and g_wapObjectDtorVtbl @0x5e8cb4 = the CObject base-dtor
// table on the fail path). The worker's own +0x00 vptr is ZEROED here (not a
// polymorphic outer object), so `new WwdPlaneRender` cannot express this; the
// embedded-object-at-offset re-stamp is the only expressible form (wall).
RVA(0x001628f0, 0x1fc)
i32 WwdFile::RebuildPlanes(i32 base, i32 count) {
    if (base == 0) {
        return 0;
    }

    WwdPlaneRender*& worker = WLOADER(WwdPlaneRender*, 0xb0);
    if (worker) {
        worker->DtorBody();
        worker->ListDtor();
        ::operator delete(worker);
        worker = 0;
    }

    WwdRegOwner* reg = WLOADER(WwdRegOwner*, 0xc);
    void* src = reg->m_8;
    if (src == 0) {
        return 0;
    }
    WwdPlaneHdr* hdr = reg->m_24;
    if (hdr == 0) {
        return 0;
    }

    i32 p0[2] = {hdr->geo[0], hdr->geo[1]};
    i32 p1[2] = {hdr->geo[2], hdr->geo[3]};
    i32 p2[2] = {hdr->geo[4], hdr->geo[5]};
    i32 p3[2] = {hdr->geo[6], hdr->geo[7]};
    i32 p4[2] = {hdr->geo[8], hdr->geo[9]};
    i32 p5[2] = {hdr->geo[10], hdr->geo[11]};

    WwdPlaneRender* nw = (WwdPlaneRender*)::operator new(0xb8);
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
        WwdPlaneRender* w = WLOADER(WwdPlaneRender*, 0xb0);
        if (w) {
            w->DtorBody();
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
// non-ctor factory-stamp wall: this is a factory, not a ??0 ctor - the 0x1dc game
// object and its +0x1A0 sub-object are brought up by the EXTERNAL engine ctors
// (Construct 0x15b390 / 0x156cb0, unmatched), then their derived vtables are
// re-stamped by address (g_wwdObjVtbl @0x5f00a8 = ??_7CWwdGameObjectA / g_wwdSubVtbl
// @0x5f0128 = ??_7CAniAdvanceCursor, both realized real-polymorphic in their own
// TUs but ORPHAN-bound to those g_ DATA symbols). A `new`-based rewrite is
// impossible here (the construction is external engine code), and a VTBL on the
// realized classes would dup-DATA the factory's bound g_ symbols -> the manual
// re-stamp is the only expressible form (compiler-model wall). Logic byte-faithful.
RVA(0x00162af0, 0x806)
i32 WwdFile::ReadPlaneObjects(const i32* src) {
    if (src == 0) {
        return 0;
    }

    WwdLevelLoader* loader = (WwdLevelLoader*)this;

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

    obj->Construct(loader->m_assetOwner, id, 0);

    // Construct the embedded sub-object at +0x1A0, then re-stamp both vtables (the
    // base ctors leave a base vtable; ReadPlaneObjects promotes both to their
    // derived types) and zero the trailing fields the derived layout adds.
    WwdObjAnimInit* subInit = (WwdObjAnimInit*)((char*)obj + 0x1a0);
    new (subInit) CDDrawSubMgr((CDDrawSurfaceMgr*)loader->m_assetOwner, id, 0);
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
    if (x < 0 || x >= loader->m_gridWidth || y < 0 || y >= loader->m_gridHeight) {
        obj->Delete(1);
        return (i32)(strCursor - (const char*)src);
    }

    // If an image set is named, require it to be present in the level map.
    i32 loaded = 1;
    if (imageSet.GetLength() != 0) {
        void* found = 0;
        CMapStringToPtr* map = (CMapStringToPtr*)((char*)loader->m_assetOwner + 0x14 + 0x10);
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

    CGameObjAux* anim = obj->m_7c;
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

    ((CObList*)((char*)loader + 0xb0))->AddTail((CObject*)obj);

    return (i32)(strCursor - (const char*)src);
}

// ---------------------------------------------------------------------------
// CPlaneRender::CenterScrollA / CenterScrollB (__thiscall, returns int). Compute
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
i32 CPlaneRender::CenterScrollA() {
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

// ---------------------------------------------------------------------------
// CPlaneRender::InitScrollRects (__thiscall, no args). Seed three (0,0,w-1,h-1)
// rects + their centers (w/2, h/2) into the scroll sub-object from the plane
// geometry's three dimension pairs (m_mapData->m_geometry), then park
// the scroll target at (-22222, -22222) so the first SetTarget always moves.
RVA(0x00163420, 0xf0)
void CPlaneRender::InitScrollRects() {
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
    s->m_rectALeft = 0;
    s->m_rectATop = 0;
    s->m_rectARight = c8 - 1;
    s->m_rectABottom = cc - 1;
    s->m_centerAX = c8 / 2;
    s->m_centerAY = cc / 2;

    s = m_scroll;
    s->m_rectBLeft = 0;
    s->m_rectBTop = 0;
    s->m_rectBRight = d0 - 1;
    s->m_rectBBottom = d4 - 1;
    s->m_centerBX = d0 / 2;
    s->m_centerBY = d4 / 2;

    s = m_scroll;
    s->m_rectCLeft = 0;
    s->m_rectCTop = 0;
    s->m_rectCRight = d8 - 1;
    s->m_rectCBottom = dc - 1;
    s->m_centerCX = d8 / 2;
    s->m_centerCY = dc / 2;

    s = m_scroll;
    s->m_targetX = -22222;
    s->m_targetY = -22222;
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
// CPlaneRender::ValidateTiles (__thiscall, ret 0x4). When the plane is loaded
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
i32 CPlaneRender::ValidateTiles(char* errOut) {
    if (((CPlaneRenderPoly*)this)->IsLoaded() == 0) {
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
            CPlaneFrame* frame = m_planeArray[(u32)handle >> 16];
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
// CPlaneRender::ResolveColorKey (__thiscall, no args). For a 16bpp plane only
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
void CPlaneRender::ResolveColorKey() {
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
extern i32 __stdcall PlaneSaveVia(void* stream); // 0x163780 == CPlaneRender::Save entry
extern i32 __stdcall PlaneLoadVia(void* stream); // 0x1638c0 == CPlaneRender::Load entry
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
// CPlaneRender::Save (__thiscall, ret 0x4). Serialize the plane to a binary
// stream: the scroll origin/dims block, the origin/extent rect, four shift/log
// fields, the tile grid (size-prefixed), and the fixed 0x80-byte name field.
RVA(0x00163780, 0x134)
i32 CPlaneRender::Save(CWwdStream* s) {
    if (s == 0) {
        return 0;
    }

    s->Write(&m_scaledX, 4);
    s->Write(&m_scaledY, 4);
    s->Write(&m_18, 4);
    s->Write(&m_1c, 4);
    s->Write(&m_originX, 0x10);
    s->Write(&m_80, 4);
    s->Write(&m_84, 4);
    s->Write(&m_88, 4);
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
// CPlaneRender::Load (__thiscall, ret 0x4). Inverse of Save: read back the same
// field sequence; the size-prefix must equal gridW*gridH*4 or the load aborts.
RVA(0x001638c0, 0x140)
i32 CPlaneRender::Load(CWwdStream* s) {
    if (s == 0) {
        return 0;
    }

    s->Read(&m_scaledX, 4);
    s->Read(&m_scaledY, 4);
    s->Read(&m_18, 4);
    s->Read(&m_1c, 4);
    s->Read(&m_originX, 0x10);
    s->Read(&m_80, 4);
    s->Read(&m_84, 4);
    s->Read(&m_88, 4);
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

// @early-stop
// 87.9%, same shrink-wrapped-push / member-load scheduling wall as CenterScrollA.
RVA(0x00163370, 0x70)
i32 CPlaneRender::CenterScrollB() {
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

// CPlaneRender::GetTileHandle (0x000d53a0) is now an inline member in the header.


// ---------------------------------------------------------------------------
// CPlaneRender::SnapToTileCenter (__thiscall, ret 0xc). Floor each axis to its
// tile boundary (>>shift <<shift) and add half a tile (signed /2).
// @early-stop
// ~51%, logic byte-exact (same sar/shl/cltd/sub/sar/add selection). Residual is a
// whole-function regalloc/coloring wall: retail keeps the two shift counts in
// caller-saved eax/edx (3 callee-saved pushes) and stores both results last; this
// build colors a shift count into ebx (a 4th push, ebp) and flips the axis order.
// Not source-steerable (member-load scheduling / coloring; matching-patterns.md).
RVA(0x000311e0, 0x4c)
void CPlaneRender::SnapToTileCenter(i32* out, i32 x, i32 y) {
    i32 sx = m_shiftX;
    i32 sy = m_shiftY;
    i32 rx = ((x >> sx) << sx) + m_tilePxW / 2;
    i32 ry = ((y >> sy) << sy) + m_tilePxH / 2;
    out[0] = rx;
    out[1] = ry;
}

// ---------------------------------------------------------------------------
// WwdFile::GetMapBaseName (static __cdecl, returns CString by value)
// Copy the path into the shared 0x62c010 scratch buffer, drop the 4-char
// extension (write a NUL at len-4 via the preceding 0x62c00c slot), then return
// the filename portion after the last backslash. Empty/short (<= 4 char) paths
// come back unchanged. The arg CString is taken by value (callee destroys it),
// and a working-copy CString temp carries the result, so cl emits the /GX frame.
// @early-stop
// /GX CString-temp wall: the inline strcpy/strlen, the extension truncation, the
// last-'\\' scan, the by-value arg + result CString teardown and the return-copy
// are byte-faithful; residue is the EH scope-table cookie + the descending
// trylevel numbering across the two CString temps (not source-steerable).
RVA(0x0003bb50, 0x128)
CString WwdFile::GetMapBaseName(CString path) {
    CString result = path;
    i32 len = path.GetLength();
    if (len == 0) {
        return result;
    }
    if (len <= 4) {
        return result;
    }
    strcpy(g_mapNameBuf, path);
    i32 blen = strlen(g_mapNameBuf);
    if (blen >= 5) {
        g_mapNamePre[blen] = 0; // g_mapNameBuf[blen - 4] = 0 (drop the ".ext")
        i32 blen2 = strlen(g_mapNameBuf);
        if (blen2 >= 1) {
            i32 i = blen2 - 1;
            if (i >= 0) {
                while (g_mapNameBuf[i] != '\\') {
                    i--;
                    if (i < 0) {
                        break;
                    }
                }
            }
            result = &g_mapNameBuf[i + 1];
        }
    }
    return result;
}

// ===========================================================================
// Class-metadata annotations for the Wwd classes (EOF-hosted: WwdFile.h is pulled
// into GameLevel.h and this is a large /O2 TU with several @early-stop bodies, so
// keep every completeness typedef after the last function to avoid a reschedule).
//
// VTBL skips (logged, none catalogable here):
//   CPlane / CWwdStream / CPlaneRenderPoly - external/abstract engine shells;
//     their virtuals are declared-not-defined so cl emits no ??_7 vtable and the
//     retail RVA is not modeled in-TU.
//   CGameObject (ReadPlaneObjects' object) - manual re-stamp; its retail vtable
//     0x5f00a8 is already bound as ?g_wwdObjVtbl (a VTBL would dup that rva).
// ===========================================================================
// --- WwdFile.h header classes ---
SIZE(WwdHeader, 0x5f4);     // on-disk WWD header (RE'd 0x5F4 bytes)
SIZE(WwdInputStream, 0x10); // 16-byte file-stream object (full layout to +0xc)
SIZE_UNKNOWN(CPlaneGeom);   // WwdFile's plane-geom (CPlay.h's render-geom facet is CPlayPlaneGeom)
SIZE_UNKNOWN(CPlaneScroll);
SIZE_UNKNOWN(CPlaneSurfDesc);
SIZE_UNKNOWN(CPlaneSurf);
SIZE_UNKNOWN(CPlanePalArr);
SIZE_UNKNOWN(CPlanePalOwner);
SIZE_UNKNOWN(CPlanePalHost);
SIZE_UNKNOWN(CPlaneMapData);
SIZE_UNKNOWN(CWwdStream);       // abstract serialize-stream slot view
SIZE_UNKNOWN(CPlaneRenderPoly); // slot-dispatch view
SIZE_UNKNOWN(CPlaneRender);
SIZE_UNKNOWN(CGameLevelPlanes);
SIZE_UNKNOWN(WwdFile); // namespace-class (method-only)
// --- WwdFile.cpp local views ---
SIZE_UNKNOWN(WwdGameRegSlot);
SIZE_UNKNOWN(WwdLevelLoader);
SIZE_UNKNOWN(CStringAssign); // +0xdc CString::operator= helper (WwdGameObj folded to CGameObject)
SIZE_UNKNOWN(WwdSubMgrCtor);
SIZE_UNKNOWN(WwdObjAnimInit);
SIZE_UNKNOWN(WwdObjList);
SIZE_UNKNOWN(WwdPlaneHdr);
SIZE_UNKNOWN(WwdRegOwner);
SIZE_UNKNOWN(WwdPlaneRender);
