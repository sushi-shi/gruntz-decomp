// MoviePlayer.h - the DDrawMgr movie/FMV player (formerly the "severus"
// codename placeholder; Ghidra FUN_0017c6f0 cluster). A large (>0x8694 B) movie/stream
// decode object: its main cluster (0x17b500..0x17c790) wraps DirectDrawCreate,
// ShowCursor and the Smacker decoder around an embedded decode store at +0x540
// (the canonical CFecFile: CFile @ +0x124, CDWordArray @ +0x138) and a Rez-owned
// scratch embed at +0x868c.
//
// VPTR AUDIT (all-vtables-die batch 3): the former "manual-vtable-stamp device"
// claim was disproven at the byte level. CMoviePlayer+0x00 and the decode store's
// +0x00 are plain DATA flags (Abort `cmp [this],0 / mov [this],0` - compared and
// zeroed, never dispatched; the /GX dtor 0x38fc0 stamps NO whole-object vptr).
// Only the +0x868c playlist embed is genuinely polymorphic (real dtor stamps
// 0x5e971c/0x5e8cb4) - realized below as the REAL MFC
// CArray<PLAYLISTINFOSTRUCT*, PLAYLISTINFOSTRUCT*> (RTTI-proven).
// Offsets + emitted bytes are the load-bearing fact; field names are placeholders.
//
// Reconstructed across two units (separate retail TUs):
//   src/Io/MoviePlayer.cpp        - Open (0x17c6f0, frameless) + ~CMoviePlayer (0x038fc0, /GX Eh)
//   src/Io/SmackerVideoWindow.cpp - the 0x17c040..0x17d6b0 playback cluster (Init/
//       CreateVideoWindow/Teardown/Open*/Pump/Advance/CloseSmacker/Frame/...)
// (The latter's former TU-local "CSmackWin" view - same +0x04 active flag, +0x540
// decode store, and the 0x17c510/0x17c630 RVAs this header already claimed - is
// folded here; its SmackSub +0x540 view == CMovieDecodeStore, Shutdown == Abort
// @0x17b570. boundaryupper2's CPageStore17b510 is a third view of the decode store.)
#ifndef GRUNTZ_CMOVIEPLAYER_H
#define GRUNTZ_CMOVIEPLAYER_H

#include <Mfc.h>      // MFC CFile/CDWordArray (the movie file + decode-buffer dtors)
#include <afxtempl.h> // MFC CArray - the +0x868c playlist embed's REAL template class
#include <Ints.h>
#include <rva.h>             // OVERRIDE / VTBL / SIZE macros
#include <Wap32/Object.h>    // CObject - the scratch embed's polymorphic grand-base
#include <Crypto/FecCrypt.h> // CFecFile - the +0x540 embedded decode store

struct SmackTag;           // the RAD Smacker stream handle (<smack.h>'s `Smack` typedef tag)
struct IDirectDrawSurface; // <ddraw.h> in the dispatching TUs; pointer-only here
class CWnd;                // real MFC CWnd (<afxwin.h> in the dispatching TU)
struct HWND__;             // strong HWND tag (windows.h STRICT)

// The Rez heap free (reloc-masked rel32 callee, __cdecl 1 arg). 0x1b9b82.
#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

// (The CMovieFile + CMovieDecodeStore VIEWS are DISSOLVED: the +0x540 decode store
// is the ONE canonical CFecFile (<Crypto/FecCrypt.h>) - same +0x00 open gate,
// +0x124 embedded MFC CFile (dtor 0x1bf121), +0x138 ::CDWordArray index (ctor
// 0x1b4b43 / dtor 0x1b4b76; the four MFC array classes are byte-identical, so the
// FID rows are AMBIG - `python -m gruntz.analysis.mfc_class 0x1b4b76` asked the
// binary). sizeof(CFecFile)==0x814c puts the playlist embed at
// 0x540+0x814c == +0x868c EXACTLY - the old 0x200-byte store view + pad were a
// mis-split of the same span. ~CFecFile (0x0390a0, defined in MoviePlayer.cpp)
// is the store teardown; ~CMoviePlayer inlines it (same retail TU).)

// One movie-clip descriptor the playlist array holds by pointer. PLAYLISTINFOSTRUCT
// is the RETAIL struct name - the RTTI COL at the playlist vtable 0x1e971c reads
// .?AV?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@. PlayList walks the array opening +
// pumping each clip. m_src==0 aborts the run; m_openArg==0 selects the OpenLo path,
// else the full Open path.
struct PLAYLISTINFOSTRUCT {
    i32 m_src;     // +0x00  source handle (0 => stop the run / invalid entry)
    i32 m_openArg; // +0x04  0 => OpenLo, else Open's second arg
    i32 m_08;      // +0x08
    i32 m_useDS;   // +0x0c
    i32 m_10;      // +0x10
    i32 m_14;      // +0x14
    i32 m_flags;   // +0x18  Pump flags
    i32 m_count;   // +0x1c  Pump loop count
};
SIZE(PLAYLISTINFOSTRUCT, 0x20);

// The Rez-owned playlist embed at worker+0x868c: the REAL MFC template
// CArray<PLAYLISTINFOSTRUCT*, PLAYLISTINFOSTRUCT*> (afxtempl.h) - RTTI-proven
// (COL at 0x1e971c). Its 5-slot vtable is the CObject shape with two own slots:
// slot 1 ??_G (thunk 0x4040f7 -> sdd 0x3a1a0 -> ??1 0x39f20) + slot 2 Serialize
// (thunk 0x401e56 -> 0x39fa0); slots 0/3/4 inherited from CObject. The retail-kept
// COMDAT copies are pinned in src/Gruntz/ArraySerialize.cpp (the explicit
// instantiation host). Layout: vptr +0x00 / m_pData +0x04 (worker+0x8690, Rez-owned
// clip-pointer array) / m_nSize +0x08 / m_nMaxSize +0x0c / m_nGrowBy +0x10.
// (The former hand-rolled `CMovieScratch : CObject` twin of this layout is
// dissolved onto the real template.)
typedef CArray<PLAYLISTINFOSTRUCT*, PLAYLISTINFOSTRUCT*> CMoviePlaylist;

// ===========================================================================
// PROVEN, NOT YET EXECUTED: CMoviePlayer == CDDPageMgr == CDDScreen ARE ONE CLASS
// (VW3 2026-07-17). This block is the EVIDENCE + the executable spec; the union
// itself is a mechanical 3-header/6-TU edit left to the next lane (it was not
// rushed at the tail of a budget - a wrong member union silently corrupts three
// subsystems' offsets, and "honest gap > wrong fold").
//
// WHY THEY ARE ONE CLASS (each item read off retail, none inferred from names):
//
//  1. ONE TU, ONE BAND. All 17 methods of the three names live in src/DDrawMgr/
//     DDPageMgr.cpp and occupy ONE strictly-ascending contiguous RVA band
//     0x17c040..0x17d8a8, INTERLEAVED by name: PageMgr(0x17c040), Movie(0x17c2a0),
//     Screen(0x17c3f0), Movie x6, Screen x3, PageMgr x3, Movie(0x17d720). Per the
//     TU/linker-order invariant one .obj is one TU - three classes cannot interleave
//     like that.
//  2. THE ARRAY HEADER TILES THREE WAYS. Movie's +0x868c playlist is an MFC
//     CArray<PLAYLISTINFOSTRUCT*,...> (RTTI-proven, COL @0x1e971c), whose members
//     land m_pData@+0x8690 / m_nSize@+0x8694 / m_nMaxSize@+0x8698. CDDPageMgr
//     independently models EXACTLY those three slots as m_data / m_count / m_8698,
//     and CDDScreen's m_86a0 ("reset to 0 by Configure") is Movie's m_loopCount
//     @+0x86a0. Three independent readings, one byte layout.
//  3. THE DESC CHAIN TILES. CDDScreen's m_srcDesc (DDSURFACEDESC) sits at +0x9c;
//     DDSURFACEDESC::lPitch is +0x10 and lpSurface +0x24, i.e. +0xac and +0xc0 -
//     which is EXACTLY where CMoviePlayer independently put m_lPitch and
//     m_lpSurface. CDDPageMgr's own desc is the OTHER one at +0x30 (0x6c B, ends
//     +0x9c) - one object, two DDSURFACEDESC scratch buffers (primary + source).
//  4. m_520 IS m_bpp. Movie called +0x520 "palette-mode state (8 => snapshot on new
//     palette)"; Screen/PageMgr both call it m_bpp. The magic 8 is 8bpp.
//  5. CTileInfo IS THE SMACK HANDLE. Screen's +0x10 "CTileInfo*" has m_0/+0x0,
//     m_width/+0x4, m_height/+0x8 and an RGB palette source at +0x6c - that is the
//     RAD Smacker handle's Version/Width/Height/palette, i.e. Movie's +0x10
//     m_smackHandle. Screen's "tile source surface sized to the m_tileInfo dims,
//     OFFSCREENPLAIN|SYSTEMMEMORY" (CheckGrid) is the classic SmackToBuffer sysmem
//     staging surface, and its "tiled blit" is the movie frame blitter.
//
// EXECUTABLE SPEC (offset -> unified member; anonymous unions carry BOTH readings
// where they genuinely differ - byte-neutral, the CNetSession precedent):
//   +0x00 HWND m_window            [Movie's "m_0, NOT a vptr" agrees]
//   +0x04 i32 m_initialized        [== Movie m_active]
//   +0x08 i32 m_streamOpen         [Screen m_8 "cleared by InitMode"]
//   +0x0c i32 m_0c                 [== PageMgr m_c]
//   +0x10 SmackTag* m_smackHandle  [== Screen m_tileInfo; see (5)]
//   +0x14 IDirectDraw2* m_dd2      +0x18 IDirectDraw* m_dd  [Screen+PageMgr agree]
//   +0x1c union { IDirectDrawSurface* m_primary; i32 m_command; }   <- OPEN CONFLICT:
//         Screen+PageMgr say primary surface, Movie::Advance says a pending command
//         it saves/restores (mov ebp,[esi+0x1c] / mov [esi+0x1c],ecx / restore).
//         ARBITRATE by disassembling whether InitMode's [esi+0x1c] is COM-dispatched.
//   +0x20 IDirectDrawSurface* m_primaryRaw   [Screen m_20 "only Release'd"]
//   +0x24 IDirectDrawSurface* m_srcSurf      +0x28 IDirectDrawSurface* m_28
//   +0x2c IDirectDrawPalette* m_palette
//   +0x30 the PageMgr desc union (0x6c: m_descSize/+0x30, m_descFlags/+0x34,
//         m_descCaps/+0x98) - ends exactly at +0x9c
//   +0x9c DDSURFACEDESC m_srcDesc (0x6c) - Movie's m_lPitch/m_lpSurface become
//         m_srcDesc.lPitch / m_srcDesc.lpSurface; ends at +0x108
//   +0x108 union { u8 m_colorSlots[0x400]; PALETTEENTRY m_palEntries[0x100]; }
//         (Screen's precise 0x400 wins over PageMgr's coarse 0x408 blob)
//   +0x508 union { i32 m_508; void* m_directSound; }  [Screen "a31 pass-through"]
//   +0x50c i32 m_50c   +0x510 union { i32 m_510; i32 m_modeTag; }   +0x514 i32 m_514
//   +0x518 u32 m_screenWidth  +0x51c u32 m_screenHeight  +0x520 i32 m_bpp
//   +0x524 m_tilesAcross  +0x528 m_tilesDown  +0x52c m_originX  +0x530 m_originY
//   +0x534 union { RECT* m_destRect; void* m_rezBuffer; }
//   +0x538 union { i32 m_forceSingleRow; i32 m_useDS; }
//   +0x53c CWnd* m_videoWnd
//   +0x540 CFecFile m_540 (0x814c, ends +0x868c)
//   +0x868c CMoviePlaylist m_868c (CArray, 0x14, ends +0x86a0)
//   +0x86a0 i32 m_loopCount        [== Screen m_86a0]
//   => SIZE 0x86a4, which both surviving models already imply.
//
// EXECUTION NOTES (the two traps that made this look harder than it is):
//  * NO PROTECTED-MEMBER PROBLEM. CDDPageMgr::RemoveAt @0x17d600 reads
//    [ebp+0x8694] / [ebp+0x8690] directly, which is simply what MFC's INLINE
//    CArray::GetSize() / operator[] compile to at /O2 - so RemoveAt/FreeAll are
//    written against the array's PUBLIC api (m_868c[i], GetSize(), RemoveAt()) and
//    the raw slots never need touching. PageMgr's "CPageRec with three owned
//    buffers at +0/+0x10/+0x14" IS PLAYLISTINFOSTRUCT (m_src/+0, m_10, m_14) - the
//    three ::operator delete calls at 0x17d63e/50/63 free exactly those.
//  * NO INCLUDE EXPLOSION. Keep CDDScreen / CDDPageMgr as `typedef CMoviePlayer`
//    aliases (the CBrickzGrid==CMapMgr precedent: MSVC5 mangles a member defined
//    through the typedef as the REAL class, so the 17 definitions and every
//    consumer keep compiling and all mangle to ?X@CMoviePlayer@@). DirectDrawMgr.h
//    (18 consumers!) and DDScreen.h then need only `class CMoviePlayer;` + the
//    typedef - NOT this header - so MFC/afxtempl does not leak into 18 DirectDraw
//    TUs. Only the 4 member-using TUs (PaletteCopy/PaletteReset/ImageProbe/
//    PaletteSnapshot) must swap their umbrella to <Mfc.h> first (proven pattern).
//  * COST TO EXPECT: the per-fn MAX history is keyed by mangled name, so the ~17
//    re-keyed rows restart their best-ever. That is bookkeeping, not a regression.
// ===========================================================================
class CMoviePlayer {
public:
    // ----- src/Io/MoviePlayer.cpp -----
    i32 Open(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6); // 0x17c6f0
    ~CMoviePlayer();                                          // 0x038fc0
    // ----- src/Io/SmackerVideoWindow.cpp (the playback cluster) -----
    // (The bring-up 0x17c040 is CDDPageMgr::Init(void*,DDModeInfo*,u32) - the old
    //  `int Init(HWND,i32,i32)` decl here was a fake-view alias of it;
    //  CreateVideoWindow reaches it through the same-object bridge cast.)
    int CreateVideoWindow(i32 a0, i32 a1); // 0x17c2a0
    void Teardown();                                        // 0x17c510
    i32 OpenLo(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5); // 0x17c570
    i32 OpenHi(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5); // 0x17c630
    i32 Pump(i32 flags, i32 count);                         // 0x17c790
    i32 Advance(i32 cmd, i32 loops);                        // 0x17c8e0
    i32 CloseSmacker();                                     // 0x17c9b0
    i32 PlayList(i32 loops);                                // 0x17d720
    i32 Begin(i32 a2, i32 useDS, i32 a4, i32 a5);           // 0x17cfc0 (external)
    i32 Frame();                                            // 0x17caa0
    // Frame's new-palette snapshot (0x17ca10) + dirty-rect blit (0x17cdf0) and
    // Teardown's teardowns (FreeAll 0x17d6b0 / HandleError 0x17cc80) are CDDScreen/
    // CDDPageMgr methods this same object owns - called via the real classes in
    // DDPageMgr.cpp (this cluster is one retail class split into 4 reconstruction views).

    // ----- layout (placeholders; offsets are the load-bearing fact) -----
    // +0x00: NOT a vptr - plain data zeroed by Teardown (no stamp anywhere in the
    // reconstructed cluster; the /GX dtor stamps only the scratch embed's vtables).
    i32 m_0;
    i32 m_active;     // +0x04  active flag (Open bails when 0)
    i32 m_streamOpen; // +0x08  stream-open flag
    char _0c[0x10 - 0xc];
    SmackTag* m_smackHandle; // +0x10  Smacker stream handle (SmackOpen result)
    char _14[0x1c - 0x14];
    i32 m_command; // +0x1c  pending command
    char _20[0x24 - 0x20];
    IDirectDrawSurface* m_24; // +0x24  primary DDraw surface (Lock/Restore/Unlock/Release)
    IDirectDrawSurface* m_28; // +0x28  secondary DDraw surface (Release)
    char _2c[0x9c - 0x2c];
    char m_desc[0xac - 0x9c]; // +0x9c  DDSURFACEDESC head (Lock's out-param)
    i32 m_lPitch;             // +0xac  desc.lPitch (surface stride)
    char _b0[0xc0 - 0xb0];
    LPVOID m_lpSurface; // +0xc0  desc.lpSurface (locked pixel base; the SDK's own LPVOID)
    char _c4[0x508 - 0xc4];
    void* m_directSound; // +0x508  DirectSound
    i32 m_50c;           // +0x50c  frame-locked flag
    i32 m_510;           // +0x510  SmackToBuffer blit flags
    i32 m_514;           // +0x514  full-frame flag
    char _518[0x520 - 0x518];
    i32 m_520; // +0x520  palette-mode state (8 => snapshot on new palette)
    char _524[0x534 - 0x524];
    void* m_rezBuffer;     // +0x534  Rez buffer
    i32 m_useDS;           // +0x538
    CWnd* m_videoWnd;      // +0x53c  the video window (real MFC CWnd)
    CFecFile m_540;        // +0x540  embedded decode store - the canonical CFecFile
                           //         (0x814c B; ends exactly at the +0x868c playlist)
    CMoviePlaylist m_868c; // +0x868c  Rez-owned playlist (CArray<PLAYLISTINFOSTRUCT*>, 0x14 B)
    i32 m_loopCount;       // +0x86a0  loop counter
};

#endif // GRUNTZ_CMOVIEPLAYER_H
