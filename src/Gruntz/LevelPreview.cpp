// LevelPreview.cpp - the level-preview screen tick (RVA 0xde420).
//
// Advances the PREVIEW%i counter, resolves the next \SCREENZ\%s namespace, fades
// its title in and (when the live surface is free) plays the teleporter-open cue;
// on a failed fade it cancels the command. Field names are placeholders; only
// offsets + code bytes are load-bearing.
#include <Mfc.h> // real MFC CString (operator=(LPCSTR) 0x1b9e74, reloc-masked)
#include <rva.h>

#include <DDrawMgr/CDDSurface.h> // canonical CDDSurface + IDirectDrawSurfaceZ (IsLost/Flip)
#include <stdio.h>

#include <Bute/SymTab.h>
#include <Gruntz/SoundCueMgr.h> // the ONE CSoundCueMgr shape (ConfigureItem @0x1360d0)
#include <Globals.h>

extern "C" {
    DATA(0x0061ab20)
    extern i32 g_sndEnabled;
    DATA(0x0061ab24)
    extern i32 g_sndCueTag;
    DATA(0x006bf3c0)
    extern u32 g_killCueClock;
}

// CSoundCueMgr::ConfigureItem (@0x1360d0) is modeled in <Gruntz/SoundCueMgr.h>.
struct CueObj {
    char m_pad00[0x10];
    CSoundCueMgr* m_10; // +0x10
    i32 m_14;           // +0x14
    i32 m_18;           // +0x18
};
class CCueHashTable {
public:
    i32 Lookup(const char* szName, CueObj** ppOut); // 0x1b8438
};
// The per-frame error sink: CGruntzMgr::ReportError (@0x8dc60, __thiscall, reloc-
// masked). This is the CGruntzMgr game-manager singleton (the one true shape lives
// in <Gruntz/GruntzMgr.h>); modeled here as a minimal reloc-masked-callee view -
// this TU cannot pull the full canonical header (its transitive CDDSurface clashes
// with this TU's local DirectDraw surface view). Named distinctly so it is not
// mistaken for the WAP32::CGameMgr engine base class.
class CGruntzMgrErrSink {
public:
    void ReportError(i32 code, i32 flags); // 0x8dc60  CGruntzMgr::ReportError
};

// The countdown's audio kill hook (the sound mgr at the status-bar holder's +0x2c;
// __thiscall, arg -1 == "all"). 0x136e20, reloc-masked.
class CSoundMgr {
public:
    i32 KillCue(i32 which); // 0x136e20
};

// The live DirectDraw COM surface is IDirectDrawSurfaceZ (slot 24 IsLost, +0x60);
// the engine wrapper is the canonical CDDSurface (held COM surface @+0x08 == m_8,
// Flip @0x13e850). Both from <DDrawMgr/CDDSurface.h>.

// The render host reached via the worker mgr's +0x10; carries the back surface at
// +0x2c.
struct CRenderHost {
    char m_pad00[0x2c];
    CDDSurface* m_2c; // +0x2c
};

// The DirectDraw worker mgr (PreviewMgr+0x04); Method_158b40 @0x158b40 (__thiscall)
// installs the resolved screen image; +0x10 is the render host.
struct CDDrawWorkerMgr {
    char m_pad00[0x10];
    CRenderHost* m_10;                      // +0x10
    i32 Method_158b40(i32 image, i32 mode); // 0x158b40
};

struct CStatusBarHolder {
    char m_pad00[0x10];
    CCueHashTable m_10map; // +0x10
    char m_pad14[0x2c - 0x14];
    CSoundMgr* m_2c; // +0x2c
    i32 m_30;        // +0x30
};
struct PreviewMgr {
    char m_pad00[4];
    CDDrawWorkerMgr* m_04; // +0x04
    char m_pad08[0x28 - 0x08];
    CStatusBarHolder* m_28; // +0x28
};

// The bute datum the screen namespace is interned under (?g_screenTag@@3HA
// @0x504358); referenced by address so the DIR32 operand reloc-masks.

// The engine's per-frame clock delta (?g_wap32FrameDelta@@3HA @0x653c74). Signed
// in the engine, but the timer compare/subtract below promote against the unsigned
// +0x1b8 timer (the unsigned ja/jb the countdown uses).

class CPreviewState {
public:
    // The screen's vtable (vptr @+0x00); only slot 8 (+0x20) - the per-frame
    // "advance" - is dispatched. The leading slots are placeholders so the index
    // lands at 8; the vtable itself lives in another TU (no ctor here -> none is
    // emitted), so these never need a body.
    virtual i32 Vf0();
    virtual i32 Vf1();
    virtual i32 Vf2();
    virtual i32 Vf3();
    virtual i32 Vf4();
    virtual i32 Vf5();
    virtual i32 Vf6();
    virtual i32 Vf7();
    virtual i32 Advance(); // slot 8 (+0x20)

    i32 Tick();                                                     // 0x0de200
    i32 FadeInTitle(char* name, i32 a, i32 b, i32 c, i32 d, i32 e); // 0x0fa1f0
    void RetireScene(i32 a, i32 b, i32 c, i32 d);                   // 0x0fa8f0
    void Cancel();                                                  // 0x0de590
    void LoadLevelPreviewScreen();
    i32 LoadScreen(char* name, i32 doFlip, i32 a2, i32 a3); // 0x0fab90

    CGruntzMgrErrSink* m_04; // +0x04
    i32 m_08;                // +0x08
    PreviewMgr* m_0c;        // +0x0c
    char m_pad10[0x2c - 0x10];
    CSymTab* m_2c; // +0x2c
    char m_pad30[0x1b8 - 0x30];
    u32 m_1b8;     // +0x1b8 countdown timer
    CString m_1bc; // +0x1bc
    i32 m_1c0;     // +0x1c0 preview counter
};

// CPreviewState::Tick (0x0de200) - the per-frame advance: if the live back surface
// is present and NOT lost, or the screen's Advance virtual fails, report the error
// and bail (returns 0); otherwise tick the audio cue and count the +0x1b8 timer
// down by the frame delta (clamped at 0), returning 1 to keep the screen alive.
//
// @early-stop
// 99.78% - pointer-chain scratch-register wall (docs/patterns/reread-member-view-
// pointer.md): the m_0c->m_28->m_2c audio-cue deref - retail reuses eax for the
// m_28 intermediate (`mov eax,[eax+0x28]`), cl picks ecx; 2 register-field bytes.
// Logic + control flow + all externs byte-exact. Final sweep.
RVA(0x000de200, 0x85)
i32 CPreviewState::Tick() {
    IDirectDrawSurfaceZ* surf = m_0c->m_04->m_10->m_2c->m_8;
    if (surf == 0 || surf->vtbl->IsLost(surf) != 0) {
        if (Advance() == 0) {
            m_04->ReportError(0x8006, 0xfa0);
            return 0;
        }
    }
    CSoundMgr* snd = m_0c->m_28->m_2c;
    if (snd != 0) {
        snd->KillCue(-1);
    }
    if ((u32)g_wap32FrameDelta >= m_1b8) {
        m_1b8 = 0;
    } else {
        m_1b8 = m_1b8 - g_wap32FrameDelta;
    }
    return 1;
}

// @early-stop
// 94.7% - entropy tail: the only residual is the cue object `p` getting spilled to
// the stack and reloaded for the final ConfigureItem(this) where retail keeps it in
// eax (the cue temp uses ecx/edi/ebp), plus function-tail nop padding. Logic +
// externs match retail. Final sweep.
RVA(0x000de420, 0x115)
void CPreviewState::LoadLevelPreviewScreen() {
    char buf[64];
    i32 idx = m_1c0;
    m_1c0 = idx + 1;
    sprintf(buf, "PREVIEW%i", idx);
    m_1bc = buf;
    sprintf(buf, "\\SCREENZ\\%s", (const char*)m_1bc);
    m_2c->ResolveQualified(buf, &g_screenTag);
    i32 failed = 0;
    if (FadeInTitle((char*)(const char*)m_1bc, 0, 0, 0, 0, 1) == 0) {
        failed = 1;
    } else {
        CStatusBarHolder* h = m_0c->m_28;
        if (h->m_30 == 0) {
            CueObj* p = 0;
            h->m_10map.Lookup("GAME_TELEPORTEROPEN", &p);
            if (p != 0) {
                i32 tag = g_sndCueTag;
                if (g_sndEnabled != 0 && (u32)(g_killCueClock - p->m_14) >= (u32)p->m_18) {
                    p->m_14 = g_killCueClock;
                    p->m_10->ConfigureItem(tag, 0, 0, 0);
                }
            }
        }
        RetireScene(0x50, 0x3e8, 0, 1);
    }
    m_1b8 = 60000;
    if (failed) {
        Cancel();
    }
}

// CPreviewState::LoadScreen (0x0fab90) - resolve the named "\SCREENZ\<name>"
// namespace through the bute symbol table (tagged with g_screenTag), install the
// resolved image into the DirectDraw worker, and - when asked - flip it to the
// front. Returns 0 if any prerequisite (the preview mgr, the +0x08 gate, or the
// symbol table) is missing or any step fails, else 1.
//
// @early-stop
// 96.39% - pointer-chain scratch-register + arg-eval-order wall (docs/patterns/
// pin-local-for-callee-saved-reg.md: "inner-split hurts 2-arg"): the m_0c->m_04
// object base for the 2-arg Method_158b40 call is hoisted before the arg pushes by
// retail (cl emits it after), and the m_0c->m_04->m_10->m_2c Flip chain picks edx/
// eax where cl picks ecx/edx - ~6 register-field bytes. Logic + control flow + all
// externs (sprintf, g_screenTag, ResolveQualified, Method_158b40, Flip) byte-exact.
// Final sweep.
RVA(0x000fab90, 0xaa)
i32 CPreviewState::LoadScreen(char* name, i32 doFlip, i32 a2, i32 a3) {
    if (m_0c == 0) {
        return 0;
    }
    if (m_08 == 0) {
        return 0;
    }
    if (m_2c == 0) {
        return 0;
    }
    char buf[64];
    sprintf(buf, "\\SCREENZ\\%s", name);
    i32 sym = m_2c->ResolveQualified(buf, &g_screenTag);
    if (sym == 0) {
        return 0;
    }
    if (m_0c->m_04->Method_158b40(sym, 1) == 0) {
        return 0;
    }
    if (doFlip != 0) {
        m_0c->m_04->m_10->m_2c->Flip(0);
    }
    return 1;
}
