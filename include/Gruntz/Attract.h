#ifndef GRUNTZ_GRUNTZ_CATTRACT_H
#define GRUNTZ_GRUNTZ_CATTRACT_H

#include <Ints.h>
#include <rva.h> // OVERRIDE
#include <Gruntz/State.h>

#include <DDrawMgr/DDSurface.h> // canonical CDDSurface (the flip/render target)

extern "C" i32 g_attractStateCount;

class CAttractSceneSlot {
public:
    // PrimeScene @? IS CGruntzSoundZ::IsPlaying; cast at the call.
    // RestoreScene @? IS CGruntzSoundZ::StopAndFlush; cast at the call.
};

class CAttractVideo {
public:
    char m_pad00[0x48];
    CAttractSceneSlot* m_48; // +0x48  scene/scheduler handle
};

class CSymParser; // <Bute/SymParser.h> (ResolvePath 0x13c030); m_8 re-typed

class DirectSoundMgr; // <Dsndmgr/DirectSoundMgr.h> (full def at the call sites)
struct CAttractHost {
    char m_pad00[0x10];
    DirectSoundMgr* m_10; // +0x10  voice/host object (the real DirectSoundMgr)
};

class CGruntzMgr;

class CAttractScreenObj {
public:
    void* ResolveScreen(char* path, void* tag); // 0x120120
};

class CAttract : public CState {
public:
    // slot 1  0x013fb0 (AttractState.cpp; retail ??_7CAttract slot 1 = ILT
    // 0x211c -> 0x13fb0, ex "EnterAttractMode") - enter (or re-enter) the attract
    // scene. Chains the base default via CState::LoadGameAssetNamespaces().
    virtual i32 LoadGameAssetNamespaces(i32 a, i32 b, i32 mode) OVERRIDE;
    // Own vtable slots (RTTI vtbl@0x5ea194, 26 slots; slot order anchored by
    // CState). Every slot CAttract overrides is declared here in slot order; the
    // two slots whose bodies live elsewhere / are deferred are declared-only (the
    // vtable references them reloc-masked - the vtable itself is not diffed). The
    // EH-framed `??1` (slot-0 deleting dtor `??_G` dispatches here) re-stamps the
    // CAttract vtable, runs the slot-2 release, re-stamps CState, chains base cleanup.
    virtual ~CAttract() OVERRIDE;             // slot 0  0x08cd90 (??1) / 0x08cd60 (??_G)
    virtual void ReleaseResources() OVERRIDE; // slot 2  (+0x08) 0x0140d0
    // Update is declared OUT-OF-LINE (body + RVA in AttractState.cpp) ON PURPOSE. An
    // RVA() sitting on an INLINE body in a header is only safe while exactly ONE TU emits
    // that COMDAT: the moment a second TU includes the header and instantiates the class
    // (GruntzMgr.cpp does, to `new CAttract`), cl emits the inline COMDAT there too, both
    // units claim rva 0x0008cd40, and merge_labels re-attributes the symbol to the last
    // one - the function silently drops out of attractstate's diff. Keep the body in the
    // owning TU for any class a second TU constructs.
    virtual GameStateId Update() OVERRIDE; // slot 4  (+0x10) 0x08cd40
    virtual i32 Render() OVERRIDE;         // slot 5  (+0x14) 0x0143e0  attract per-frame poll/draw
    virtual i32 Vslot06() OVERRIDE; // slot 6  (+0x18) 0x014630  random-title roll (IsActive gate)
    virtual i32 Vslot07() OVERRIDE; // slot 7  (+0x1c) 0x0147b0  host/paint poll
    virtual i32 InputVirtual() OVERRIDE; // slot 8  (+0x20) 0x014520  random-title roll (page gate)
    virtual i32 Vslot09(i32)
        OVERRIDE; // slot 9  (+0x24) 0x014120  title-screen entry (425B /GX; Attract.cpp)
    virtual i32 FrameSlot28(i32 arg) OVERRIDE; // slot 10 (+0x28) 0x014340  per-frame voice poll
    virtual i32 Vslot0c(i32, i32)
        OVERRIDE; // slot 12 (+0x30) 0x014720  (declared-only: ESC/SPACE/ENTER cmd)
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14 (+0x38) 0x014770  post-exit command

    // Non-virtual attract methods (the rest of the title/menu logic). (The ex
    // "EnterAttractMode" decl is GONE - it IS the slot-1 LoadGameAssetNamespaces
    // override above, RTTI+ILT-proven; the old "signature differs" note was stale,
    // both are __thiscall (i32,i32,i32) ret 0xc.)
    i32 RefreshTitle(i32 unused);  // 0x39160
    i32 LoadTitleConfig(i32 mode); // 0xa03f0
    i32 Activate();                // 0xa0a30

    // (FadeInTitle 0xfa1f0 / RunTitle 0xfa300 / RunTitleSeq 0xfa350 / RetireScene 0xfa8f0
    //  are CState-base title-roll/transition methods now - declared in <Gruntz/State.h>,
    //  inherited here. The former fake CAttract::BuildMenuPage @0xfa8f0 view is dissolved.)
    // engine tail helpers (__thiscall, reached via ILT thunks).
    void CommitStage(); // FUN_004a05a0

    // Typed views of the inherited CState slots re-typed to the attract facets that
    // share them (the object at each slot IS that facet in the attract state; the
    // base declares them generically because other states put other types there).
    // Inline -> the same `mov reg,[this+off]` falls out with no extra codegen.
    // (menuRoot()/screenObj() moved to CState with the title-roll cluster.)
    CSymParser* stateMgr() {
        return static_cast<CSymParser*>(m_8);
    }
    CAttractVideo* video() {
        return reinterpret_cast<CAttractVideo*>(m_4);
    }
    CGruntzMgr* owner() {
        return static_cast<CGruntzMgr*>(m_4);
    }
    CSymTab* attractState() {
        return (m_2c);
    }

    // The attract-specific block sits past the CState spine (which ends at +0x1a4).
    char m_pad1a8[0x1b4 - 0x1a8];
    u32 m_idleTimer;      // +0x1b4  attract idle/timeout countdown (unsigned: jb tick)
    CAttractHost* m_host; // +0x1b8  host/sound sub-object
    i32 m_activeFlag;     // +0x1bc  attract-active flag
};
VTBL(CAttract, 0x001ea194);

#endif // GRUNTZ_GRUNTZ_CATTRACT_H
