// CAttract.h - CAttract, the attract/title-screen game-state (RTTI
// .?AVCAttract@@, vtable @0x5ea194). A CState leaf (RTTI base list:
// CAttract -> CState): it overrides the slot-2 resource release, the slot-7
// paint/host poll, the slot-10 per-frame poll, and carries the EH-framed `??1`.
// On top of the CState spine it drives the attract sequence (random TITLE state,
// the title fade, the menu-brightness sink chain, the cursor-visible loop).
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing. The owner/view/menu sub-objects are modeled under TU-local names
// (the offsets are the ctor ground truth).
#ifndef GRUNTZ_GRUNTZ_CATTRACT_H
#define GRUNTZ_GRUNTZ_CATTRACT_H

#include <Ints.h>
#include <rva.h> // OVERRIDE
#include <Gruntz/CState.h>

#include <DDrawMgr/CDDSurface.h> // canonical CDDSurface (the flip/render target)

// ---------------------------------------------------------------------------
// The video-mode sub-object at CAttract+0x4 (== CState::m_4 re-typed). Its first
// method (engine FUN_0048ddd0, __thiscall ret 4 - the RestoreVideoMode shape,
// called with 0) re-asserts the display mode when (re)entering the attract scene.
// The sub-object also carries a scene/scheduler handle at +0x48 on which
// RefreshTitle drives a reset pair.
// ---------------------------------------------------------------------------
class CAttractSceneSlot {
public:
    void PrimeScene();   // FUN_00538920
    void RestoreScene(); // FUN_00538530
};

class CAttractVideo {
public:
    i32 RestoreVideoMode(i32 save);

    char m_pad00[0x48];
    CAttractSceneSlot* m_48; // +0x48  scene/scheduler handle
};

// The attract state machine at CAttract+0x8 (== CState::m_8 re-typed; engine
// FUN_0053c030, __thiscall ret 4): resolves an attract state object by name.
class CAttractState; // resolved state object (LookupState result)

class CAttractStateMgr {
public:
    CAttractState* LookupState(char* name);
};

// The resolved attract state object. EnterAttractMode loads its "SOUNDZ" set
// (engine FUN_0053a230, __thiscall ret 4), yielding an opaque sound handle.
class CAttractState {
public:
    void* LoadSoundz(char* name);
};

// ---------------------------------------------------------------------------
// The menu/brightness sink chain rooted at CAttract+0xc (== CState::m_c). The
// page's +0x14/+0x18 holders carry the brightness targets (their m_2c) that take
// the CButeMgr value; m_04 drives the page enter/exit transitions.
// ---------------------------------------------------------------------------
class CMenuBrightnessTarget {
public:
    void SetBrightness(i32 value, i32 flags); // FUN_0053f460 (ret 8)
};

struct CMenuBrightnessHolder {
    char m_pad00[0x2c];
    CMenuBrightnessTarget* m_2c; // +0x2c  brightness target
};

// The flip/render target is the canonical CDDSurface (m_04->m_10->m_2c->Flip(0);
// ?Flip@CDDSurface@@QAEHPAV1@@Z, FUN_0013e850, __thiscall ret 4).
struct CMenuRenderM10 {
    char m_pad00[0x2c];
    CDDSurface* m_2c; // +0x2c
};

class CMenuPage {
public:
    i32 IsLoaded();    // FUN_00558bc0  (ready-3 predicate; gate for the title roll)
    void TransEnter(); // FUN_00558e40  (mode == 2: enter, runs first)
    void TransTitle(); // FUN_00558e90  (mode != 2: after brightness)
    void TransExit();  // FUN_00558ee0  (mode == 2: after brightness)
    // The host poll tail's blit (engine FUN_00558c70, __thiscall ret 4): pushes
    // m_14, blits onto m_18.
    void BlitFrom(CMenuBrightnessHolder* src); // FUN_00558c70

    char m_pad00[0x10];
    CMenuRenderM10* m_10;        // +0x10  render/flip view (host poll)
    CMenuBrightnessHolder* m_14; // +0x14  title brightness holder / blit src
    CMenuBrightnessHolder* m_18; // +0x18  menu  brightness holder / blit dst
};

// The attract registrar at CMenuRoot+0x28: EnterAttractMode hands it the loaded
// sound handle plus the "ATTRACT"/"_" tags (engine FUN_00557ee0, __thiscall
// ret 0xc) so the attract page is wired into the active menu. The slot-2 release
// (FUN_00557c70, __thiscall ret 8) tears it back down; +0x2c holds a pooled
// resource freed first (FUN_00537a80, __thiscall no-arg).
class CAttractPooledRes {
public:
    void Free();      // FUN_00537a80 (no-arg)
    void Stop(i32 z); // FUN_00536e20 (ret 4)
};

class CAttractRegistrar {
public:
    i32 Register(void* sound, char* type, char* sep); // FUN_00557ee0 (ret 0xc)
    void Release(char* type, char* sep);              // FUN_00557c70 (ret 8)

    char m_pad00[0x2c];
    CAttractPooledRes* m_2c; // +0x2c  pooled resource (Free() if set)
};

struct CMenuRoot {
    char m_pad00[0x4];
    CMenuPage* m_04; // +0x4  active menu page
    char m_pad08[0x28 - 0x8];
    CAttractRegistrar* m_28; // +0x28  attract page registrar
};

// The title-brightness target's preset/reset method (engine FUN_0053e760,
// __thiscall ret 4) called once before the fade with arg 0.
class CMenuBrightnessReset {
public:
    void Reset(i32 value);
};

// The m_1b8 sound/host sub-object the per-frame poll drives: its +0x10 voice is
// queried (IsPlaying), (re)started, and the registrar's pooled resource is
// stopped on the way out.
class CAttractVoice {
public:
    i32 IsPlaying();                   // FUN_005353f0 (no-arg, ret eax)
    void Restart(i32 a, i32 b, i32 c); // FUN_00535660 (ret 0xc)
};
struct CAttractHost {
    char m_pad00[0x10];
    CAttractVoice* m_10; // +0x10  voice/host object
};

// ---------------------------------------------------------------------------
// CAttract - the attract/title state. Derives from CState (RTTI ground truth);
// the few slots it implements are overridden, the rest inherited. Member offsets
// are reached by re-typing CState's m_4/m_8/m_c plus the attract-specific block.
// ---------------------------------------------------------------------------
class CAttract : public CState {
public:
    // Own vtable slots (RTTI vtbl@0x5ea194, 26 slots; slot order anchored by
    // CState). Every slot CAttract overrides is declared here in slot order; the
    // two slots whose bodies live elsewhere / are deferred are declared-only (the
    // vtable references them reloc-masked - the vtable itself is not diffed). The
    // EH-framed `??1` (slot-0 deleting dtor `??_G` dispatches here) re-stamps the
    // CAttract vtable, runs the slot-2 release, re-stamps CState, chains base cleanup.
    virtual ~CAttract() OVERRIDE;             // slot 0  0x08cd90 (??1) / 0x08cd60 (??_G)
    virtual void ReleaseResources() OVERRIDE; // slot 2  (+0x08) 0x0140d0
    virtual GameStateId Update() OVERRIDE;    // slot 4 (+0x10) 0x08cd40 GAMESTATE_ATTRACT (2)
    virtual i32 Render() OVERRIDE;  // slot 5  (+0x14) 0x0143e0  attract per-frame poll/draw
    virtual i32 Vslot06() OVERRIDE; // slot 6  (+0x18) 0x014630  random-title roll (Vfunc3 gate)
    virtual i32 Vslot07() OVERRIDE; // slot 7  (+0x1c) 0x0147b0  host/paint poll
    virtual i32 InputVirtual() OVERRIDE; // slot 8  (+0x20) 0x014520  random-title roll (page gate)
    virtual i32 Vslot09(i32)
        OVERRIDE; // slot 9  (+0x24) 0x014120  (declared-only: 426B EH, deferred)
    virtual i32 FrameSlot28(i32 arg) OVERRIDE; // slot 10 (+0x28) 0x014340  per-frame voice poll
    virtual i32 Vslot0c(i32, i32)
        OVERRIDE; // slot 12 (+0x30) 0x014720  (declared-only: ESC/SPACE/ENTER cmd)
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14 (+0x38) 0x014770  post-exit command

    // Non-virtual attract methods (the rest of the title/menu logic). EnterAttractMode
    // is the slot-1 body (0x13fb0) but is reached non-virtually; its (int,int,int)
    // signature differs from CState's slot-1 placeholder, so it stays non-virtual.
    i32 EnterAttractMode(i32 a, i32 b, i32 mode);    // 0x13fb0 (slot 1, called non-virtually)
    i32 RefreshTitle(i32 unused);                    // 0x39160
    i32 LoadTitleConfig(i32 mode);                   // 0xa03f0
    i32 Activate();                                  // 0xa0a30
    i32 RunTitle(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x0fa300 (5 args, ret 0x14)

    // The pre-flight gate for EnterAttractMode (engine FUN_004f9ea0, non-virtual
    // __thiscall ret 0xc, reached via ILT thunk): a zero result aborts the entry.
    i32 LoadAttractScene(i32 a, i32 b, i32 mode); // FUN_004f9ea0

    // engine tail helpers (__thiscall, reached via ILT thunks).
    i32 FadeInTitle(char* name, i32 a, i32 b, i32 c, i32 d, i32 e); // FUN_004fa1f0
    i32 RunTitleSeq(char* name, i32 a, i32 b, i32 c, i32 d);        // FUN_004fa350
    i32 BuildMenuPage(i32 x, i32 w, i32 h, i32 flag);               // FUN_004fa8f0
    void CommitStage();                                             // FUN_004a05a0

    // The inherited CState sub-object pointers are reached by re-typing m_4/m_8/
    // m_c/m_2c at the call site (cast, codegen-neutral). The attract-specific
    // block sits past the CState spine (which ends at +0x1a4).
    char m_pad1a8[0x1b4 - 0x1a8];
    u32 m_idleTimer;      // +0x1b4  attract idle/timeout countdown (unsigned: jb tick)
    CAttractHost* m_host; // +0x1b8  host/sound sub-object
    i32 m_activeFlag;     // +0x1bc  attract-active flag
};

#endif // GRUNTZ_GRUNTZ_CATTRACT_H
