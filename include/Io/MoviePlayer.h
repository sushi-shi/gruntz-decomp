// MoviePlayer.h - the DDrawMgr movie/FMV player (formerly the "severus"
// codename placeholder; Ghidra FUN_0017c6f0 cluster). A large (>0x8694 B) movie/stream
// decode object: its main cluster (0x17b500..0x17c790) wraps DirectDrawCreate,
// ShowCursor and the Smacker decoder around an embedded decode store at +0x540
// (CFile @ +0x124, CByteArray @ +0x138) and a Rez-owned scratch embed at +0x868c.
//
// VPTR AUDIT (all-vtables-die batch 3): the former "manual-vtable-stamp device"
// claim was disproven at the byte level. CMoviePlayer+0x00 and the decode store's
// +0x00 are plain DATA flags (Abort `cmp [this],0 / mov [this],0` - compared and
// zeroed, never dispatched; the /GX dtor 0x38fc0 stamps NO whole-object vptr).
// Only the +0x868c scratch embed is genuinely polymorphic (real dtor stamps
// 0x5e971c/0x5e8cb4) - realized below as CMovieScratch : CObject.
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

#include <Mfc.h> // MFC CFile/CByteArray (the movie file + decode-buffer dtors)
#include <Ints.h>
#include <rva.h>          // OVERRIDE / VTBL / SIZE macros
#include <Wap32/Object.h> // CObject - the scratch embed's polymorphic grand-base

struct SmackTag;           // the RAD Smacker stream handle (<smack.h>'s `Smack` typedef tag)
struct IDirectDrawSurface; // <ddraw.h> in the dispatching TUs; pointer-only here
class CWnd;                // real MFC CWnd (<afxwin.h> in the dispatching TU)
struct HWND__;             // strong HWND tag (windows.h STRICT)

// The Rez heap free (reloc-masked rel32 callee, __cdecl 1 arg). 0x1b9b82.
extern "C" void RezFree(void* p);

// An MFC CFile-shaped member: its non-trivial dtor is the reloc-masked engine
// ~CFile (0x1bf121) so the /GX member-teardown trylevel falls out
// (eh-dtor-model-members-as-destructible).
struct CMovieFile {
    // Dtor_1bf121 @0x1bf121 IS CFile::~CFile; cast below.
    ~CMovieFile() {
        ((CFile*)this)->~CFile();
    }
};

// An MFC CByteArray-shaped member: dtor -> the reloc-masked engine ~CByteArray
// (0x1b4b76).
struct CMovieByteArray {
    // Dtor_1b4b76 @0x1b4b76 IS CByteArray::~CByteArray; cast below.
    ~CMovieByteArray() {
        ((CByteArray*)this)->~CByteArray();
    }
};

// The decode store embedded at worker+0x540. Abort() (0x17b570) tears down the
// active decode; the CFile/CByteArray members destruct after it.
struct CMovieDecodeStore {
    // +0x00 active-decode flag: NOT a vptr (Abort disasm: `cmp [this],0` gate +
    // `mov [this],0` clear - compared and stored, never dispatched through).
    i32 m_open;
    char m_pad4[0x124 - 0x04];
    CMovieFile m_124; // +0x124  decode CFile
    char m_pad125[0x138 - 0x125];
    CMovieByteArray m_138; // +0x138  decode CByteArray
    char m_pad139[0x200 - 0x139];

    // Begin @0x17b510 IS CPageStore17b510::Init; cast at the call.
    // OpenA @0x17b5f0 IS CFecFile::ReadArchive; cast at the call.
    // Abort @0x17b570 IS CPageStore17b510::Close; cast at each call.
    // OpenB @0x17b840 IS CPageStore17b510::Lookup; cast at the call.
    ~CMovieDecodeStore();
};

// The Rez-owned scratch embed at worker+0x868c: REAL polymorphic (ALL-VTABLES).
// Retail vtable 0x5e971c is the 5-slot CObject shape with two own slots -
// slot 1 dtor (thunk 0x4040f7) + slot 2 override (thunk 0x401e56); slots 0/3/4
// inherited (0x5bef01/0x40106e/0x404034, == the grand-base's). Deriving
// CObject makes cl emit the dtor's own-vtable stamp at entry and the
// grand-base restamp (masks 0x5e8cb4) at exit - the exact retail stamp pair the
// old manual `m_vptr = &g_*Vtbl` stores hand-rolled. Slot 2 is declared-only
// (unreconstructed body behind the 0x401e56 thunk; reloc-masked slot).
struct CMovieScratch : public CObject {
    virtual ~CMovieScratch() OVERRIDE; // slot 1 override (retail thunk 0x4040f7)
    virtual void Serialize(CArchive& ar) OVERRIDE; // slot 2 (0x001e56, declared-only)

    void* m_4; // +0x04 (worker+0x8690)  Rez-owned buffer
};

class CMoviePlayer {
public:
    // ----- src/Io/MoviePlayer.cpp -----
    i32 Open(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6); // 0x17c6f0
    ~CMoviePlayer();                                          // 0x038fc0
    // ----- src/Io/SmackerVideoWindow.cpp (the playback cluster) -----
    int Init(HWND__* h, i32 a0, i32 a1);                    // 0x17c040
    int CreateVideoWindow(i32 a0, i32 a1);                  // 0x17c2a0
    void Teardown();                                        // 0x17c510
    i32 OpenLo(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5); // 0x17c570
    i32 OpenHi(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5); // 0x17c630
    i32 Pump(i32 flags, i32 count);                         // 0x17c790
    i32 Advance(i32 cmd, i32 loops);                        // 0x17c8e0
    i32 CloseSmacker();                                     // 0x17c9b0
    i32 Begin(i32 a2, i32 useDS, i32 a4, i32 a5);           // 0x17cfc0 (external)
    i32 Frame();                                            // 0x17caa0
    void SnapshotPalette();                     // 0x17ca10 (Frame: new-palette snapshot)
    void BlitDirty(i32 x, i32 y, i32 w, i32 h); // 0x17cdf0 (Frame: dirty-rect blit)
    void Free17d6b0();                          // 0x17d6b0
    void Free17cc80();                          // 0x17cc80

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
    void* m_lpSurface; // +0xc0  desc.lpSurface (locked pixel base)
    char _c4[0x508 - 0xc4];
    void* m_directSound; // +0x508  DirectSound
    i32 m_50c;           // +0x50c  frame-locked flag
    i32 m_510;           // +0x510  SmackToBuffer blit flags
    i32 m_514;           // +0x514  full-frame flag
    char _518[0x520 - 0x518];
    i32 m_520; // +0x520  palette-mode state (8 => snapshot on new palette)
    char _524[0x534 - 0x524];
    void* m_rezBuffer;       // +0x534  Rez buffer
    i32 m_useDS;             // +0x538
    CWnd* m_videoWnd;        // +0x53c  the video window (real MFC CWnd)
    CMovieDecodeStore m_540; // +0x540  embedded decode store
    char m_pad740[0x868c - (0x540 + 0x200)];
    CMovieScratch m_868c; // +0x868c  Rez-owned scratch embed
    char _8694[0x86a0 - 0x8694];
    i32 m_loopCount; // +0x86a0  loop counter
};

#endif // GRUNTZ_CMOVIEPLAYER_H
