// BzState.h - the deferred sub-object VIEWS of the booty ("WARP" spell) game-state's
// g_gameReg (0x64556c) object web + HUD sink. Co-included with <Gruntz/GameMode.h>
// by BootyMessages.cpp (the HUD message / secret-bonus / idle-grunt overlays) and
// BootyWalkAnim.cpp (the per-frame walking-grunt tick), whose bodies are real
// CBootyState:: methods (the former `class BzState` view is dissolved onto the
// canonical CBootyState - see the note near the end of this header). The two files
// previously carried divergent partial VIEWS of these types (e.g. GetRecordValue was
// declared 0-arg in one and (i32) in the other for the SAME retail fn 0xfced0 -
// reconciled here to its true 1-arg shape, MEASURED from the `ret 0x4` + `[esp+8]`
// idx read). Only offsets / code bytes are load-bearing (campaign doctrine); every
// callee is unmatched engine code reached by reloc-masked external thunk (no body).
#ifndef GRUNTZ_GRUNTZ_BZSTATE_H
#define GRUNTZ_GRUNTZ_BZSTATE_H

#include <Ints.h>
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // CGameObject (the created idle-grunt sprites)

#include <rva.h>

class CString; // <Mfc.h>; used by reference in BzState::FormatHudText's declaration

// The per-level record (g_gameReg->m_levelRecord). m_recordBase points at the
// group-records table GetRecordValue indexes (0x40-byte stride, +0x28 payload).
struct BzLevelRecord {
    void* m_recordBase; // +0x00  group-records table base
    i32 m_levelIndex;   // +0x04  area/level index
    i32 m_suppressGate; // +0x08  suppress gate
    i32 m_worldFlag;    // +0x0c  world/training flag
    char m_pad10[0x44 - 0x10];
    i32 m_progressFlag; // +0x44  progress flag
};
SIZE_UNKNOWN(BzLevelRecord);

// The top-level window holder reached via g_gameReg->m_wnd; m_hwnd is its HWND.
struct BzWndHolder {
    char m_pad00[0x4];
    i32 m_hwnd; // +0x04  HWND
};
SIZE_UNKNOWN(BzWndHolder);

// The idle/walking grunt sprite (m_trailSprites[]/m_visSprites[]/m_animSprites[]
// elements) is the shared CGameObject (<Gruntz/UserLogic.h>): ApplyName (0x150540)
// caches the named first frame; ApplyLookupGeometry (0x1505b0) resolves its cycle
// geometry; the booty walk drives m_stateFlags (bit0 visible), the draw-fill block
// and the screen position (the former BzSprite view mis-read m_screenX/m_screenY
// as "sprite id"/"timer" - the stored values are onscreen coordinates; its +0x1a0
// completion sub is the canonical m_1c0/m_1c8 anim-sink pair).

// The playback sub-object of a sound entry (BzSoundEntry::m_player).
struct BzSoundPlayer {};
SIZE_UNKNOWN(BzSoundPlayer);

// A named ambient sound entry. m_player is the playback sub-object; m_lastPlayed /
// m_interval rate-limit the cue.
struct BzSoundEntry {
    char m_pad00[0x10];
    BzSoundPlayer* m_player; // +0x10  player sub-object
    u32 m_lastPlayed;        // +0x14  last-played stamp
    u32 m_interval;          // +0x18  interval
};
SIZE_UNKNOWN(BzSoundEntry);

struct BzSoundSet {
    char m_pad00[0x10];
    i32 m_findTable; // +0x10 (CMapStringToPtr body starts here; cast at Find)
    char m_pad14[0x30 - 0x14];
    i32 m_playing; // +0x30  is-playing gate
};
SIZE_UNKNOWN(BzSoundSet);

// g_gameReg->m_soundHolder->m_spriteFactory - the canonical CSpriteFactory
// (<Gruntz/SpriteFactory.h>) the booty setup builds its per-player idle grunts
// through (CreateSprite @0x1597b0).
struct BzSoundHolder {
    char m_pad00[0x8];
    CSpriteFactory* m_spriteFactory; // +0x08  sprite/animation factory
    char m_pad0c[0x28 - 0xc];
    BzSoundSet* m_soundSet; // +0x28
};
SIZE_UNKNOWN(BzSoundHolder);

// g_gameReg->m_cuePlayer - fires a positional sound cue.
struct BzCuePlayer {};
SIZE_UNKNOWN(BzCuePlayer);

// g_gameReg->m_selSource - resolves the active selection handle.
struct BzSelSource {};
SIZE_UNKNOWN(BzSelSource);

// The game registry singleton (*0x64556c). m_levelRecord is the per-level record;
// m_soundHolder the ambient sound holder; m_cuePlayer the sound-cue player;
// m_selSource the selection source; m_wnd the top-level window holder.
struct BzGameReg {
    char m_pad00[0x4];
    BzWndHolder* m_wnd; // +0x04
    char m_pad08[0x30 - 0x8];
    BzSoundHolder* m_soundHolder; // +0x30
    char m_pad34[0x60 - 0x34];
    BzCuePlayer* m_cuePlayer; // +0x60
    char m_pad64[0x74 - 0x64];
    BzSelSource* m_selSource; // +0x74
    char m_pad78[0x7c - 0x78];
    BzLevelRecord* m_levelRecord; // +0x7c
    // *g_gameReg's own game-mgr method (== CGruntzMgr::ChangeState_8fab0, reloc-masked)
    // so g_gameReg->ChangeState_8fab0() calls direct - no cross-cast to CGruntzMgr*.
    // (The booty idle tick's per-area click relay reaches the singleton's real
    // CGruntzMgr::PassClickToPlayState @0x8d780 via the g_gameReg -> CGruntzMgr cast -
    // DISASM-PROVEN receiver: ecx = *0x24556c, not `this`.)
    i32 ChangeState_8fab0(i32 arg); // 0x08fab0
};

extern "C" BzGameReg* g_gameReg; // *0x24556c (Booty view of the singleton)
SIZE_UNKNOWN(BzGameReg);

// The grunt-data loader (BzSink::m_loader): Load/Finish; m_data feeds the notify.
struct BzLoader {
    char m_pad00[0x14];
    void* m_data; // +0x14
};
SIZE_UNKNOWN(BzLoader);

// The vtable-bearing notify object (BzSink::m_notify): a real polymorphic object
// whose notify slot lives at vtable +0x28 (index 10). Modeled with declared-only
// virtuals (the slot methods live in another TU) so cl emits NO ??_7 here, yet
// `notify->OnLoaded(arg)` lowers to the same __thiscall dispatch
// `mov ecx,notify; push arg; mov eax,[notify]; call [eax+0x28]` the old PMF table
// produced. This is the real devs' shape (a CObject-like sink), not a hand-roll.
struct BzSink8 {
    virtual void s00();               // +0x00
    virtual void s04();               // +0x04
    virtual void s08();               // +0x08
    virtual void s0c();               // +0x0c
    virtual void s10();               // +0x10
    virtual void s14();               // +0x14
    virtual void s18();               // +0x18
    virtual void s1c();               // +0x1c
    virtual void s20();               // +0x20
    virtual void s24();               // +0x24
    virtual void OnLoaded(void* arg); // +0x28  the notify slot
};

struct BzSinkSub {};
SIZE_UNKNOWN(BzSinkSub);
struct BzSinkSub28 {
    char m_pad00[0x2c];
    BzSinkSub* m_sprite; // +0x2c  dropped sprite (freed on exit)
};
SIZE_UNKNOWN(BzSinkSub28);

// The HUD message sink (BzState::m_sink). m_loader is the grunt-data loader;
// m_notify the vtable-bearing notify object; m_dropped a dropped-sprite holder.
struct BzSink {
    char m_pad00[0x4];
    BzLoader* m_loader; // +0x04
    BzSink8* m_notify;  // +0x08
    char m_pad0c[0x28 - 0xc];
    BzSinkSub28* m_dropped; // +0x28
};
SIZE_UNKNOWN(BzSink);
SIZE_UNKNOWN(BzSink8);

// NOTE: the former `class BzState` VIEW is dissolved - the booty/secret game-state
// object IS the canonical CBootyState (<Gruntz/GameMode.h>): the booty members
// (m_initGate/m_activation/m_trailSprites/m_visSprites/m_animSprites/m_stepIndex/...)
// now live on CBootyState; the HUD message sink the toasts read at +0xc IS the
// inherited CState::m_c holder (this header's BzSink is the still-deferred sub-object
// VIEW of that holder - the g_gameReg / m_c web reconciliation is a separate task).
// The overlay/tick bodies (BootyMessages.cpp, BootyWalkAnim.cpp) are real
// CBootyState:: methods; RegisterMultiNamespaces folds onto CState::FadeInTitle
// (0xfa1f0), StartTimer onto CBootyState::BuildPage (0xfa8f0), FormatHudText onto
// CState::FormatHudText (0x1af70), PassClickToPlayState onto CGruntzMgr (0x8d780,
// reached via the g_gameReg -> CGruntzMgr singleton cast).

#endif // GRUNTZ_GRUNTZ_BZSTATE_H
