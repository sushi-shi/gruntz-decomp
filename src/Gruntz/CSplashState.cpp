// CSplashState.cpp - the splash-screen game state (its own TU; LoadSounds is the
// only reconstructed method so far). Re-homed from src/Stub/CSplashState.cpp
// (C:\Proj\Gruntz).
//
// CSplashState : public CState (RTTI .?AVCSplashState@@, vtbl@0x1e9d74) - a real
// CState leaf. LoadSounds reads the shared CState owner/view/bank facets cast-free
// through their real INHERITED types:
//   * m_4  (CGruntzMgr*)  - RestoreVideoMode (re-assert the 640x480 display mode).
//   * m_8  (CBankMgr*)    - Lookup the "STATEZ_SPLASH" bank -> CResSource.
//   * m_2c (CResSource*)  - the cached splash bank; LoadGroup its "SOUNDZ" set.
//   * m_c  (CView*)       - its +0x28 sound registry Install()s the loaded set.
// The 0x48ddd0/0x53c030/0x53a230/0x557ee0 call targets are the VA form (RVA +
// 0x400000 image base) of CGruntzMgr::RestoreVideoMode (0x08ddd0), CBankMgr::Lookup
// (0x13c030), CResSource::LoadGroup (0x13a230) and CView SoundRegistry::Install
// (0x157ee0) - the SAME engine methods the in-game CPlay/CView resource facet
// already models; here they are reloc-masked externals.
//
// The gating global g_assetRoot is the REAL MFC CString: CString::GetLength() inlines
// to GetData()->nDataLength == m_pchData[-2], byte-identical to the load's `mov
// eax,ds:g; mov ecx,[eax-8]; test ecx,ecx` guard.
#include <Mfc.h> // CString + <windows.h> (SetCursor)

#include <Gruntz/CBankMgr.h>  // CBankMgr::Lookup / CResSource::LoadGroup (m_8/m_2c)
#include <Gruntz/CState.h>    // CState base (m_4/m_8/m_c/m_2c owner/view/bank facets)
#include <Gruntz/CView.h>     // CView SoundRegistry Install (m_c->m_28 facet)
#include <Gruntz/GruntzMgr.h> // CGruntzMgr::RestoreVideoMode (m_4 facet)
#include <rva.h>

// The global empty C string the sound loader's prefix is seeded from (0x6293f4).
extern "C" char g_emptyString[]; // 0x6293f4

// The global asset-root CString whose emptiness gates the load (0x64e25c).
DATA(0x0064e25c)
extern CString g_assetRoot;

class CSplashState : public CState {
public:
    // The 11 overridden CState slots (vtbl@0x1e9d74; the other 15 inherited). Their
    // bodies live in the class's other TUs; declared-only here (never instantiated in
    // this loader TU) so cl emits no ??_7CSplashState (CView.h declared-only pattern),
    // leaving LoadSounds' member-offset codegen unchanged.
    virtual ~CSplashState() OVERRIDE;            // slot 0
    virtual void Vfunc1() OVERRIDE;              // slot 1
    virtual void ReleaseResources() OVERRIDE;    // slot 2
    virtual i32 Update() OVERRIDE;               // slot 4
    virtual i32 Render() OVERRIDE;               // slot 5
    virtual i32 Vslot06() OVERRIDE;              // slot 6
    virtual i32 InputVirtual() OVERRIDE;         // slot 8
    virtual i32 Vslot09(i32) OVERRIDE;           // slot 9
    virtual i32 FrameSlot28(i32) OVERRIDE;       // slot 10
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14

    i32 LoadSounds(i32 a, i32 b, i32 c);
    // The base asset-namespace loader chained first (reloc-masked external, called
    // on `this`; the same 0x43a9 ILT thunk the other state loaders enter).
    i32 LoadGameAssetNamespaces(i32 a, i32 b, i32 c);
};

// @confidence: high
// @source: decomp-xref
// @early-stop
// Code bytes 100% byte-identical to retail; the reloc operands can never pair by
// name (g_assetRoot @0x64e25c delinks as ?g_netE25c@@..., the "_" arg as
// ?g_dat60b588@@..., and LoadGameAssetNamespaces is an unnamed placeholder), so the
// function stays at the documented reloc-masked plateau, NOT exact. See
// docs/patterns/external-nobody-callee.md + reloc-typing-vptr-global.md.
RVA(0x000f9780, 0x8c)
i32 CSplashState::LoadSounds(i32 a, i32 b, i32 c) {
    if (g_assetRoot.GetLength() == 0) {
        return 0;
    }
    if (!LoadGameAssetNamespaces(a, b, c)) {
        return 0;
    }
    SetCursor(0);
    m_4->RestoreVideoMode(0);

    m_2c = m_8->Lookup("STATEZ_SPLASH");
    if (!m_2c) {
        return 0;
    }

    void* soundz = m_2c->LoadGroup("SOUNDZ");
    if (soundz) {
        m_c->m_28->Install(soundz, g_emptyString, "_");
    }
    return 1;
}
