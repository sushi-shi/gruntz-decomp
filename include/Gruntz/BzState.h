// BzState.h - the shared object graph of the booty ("WARP" spell) game-state and
// its g_mgrSettings (0x64556c) sub-objects. ONE canonical definition of every Bz*
// type, included by BootyMessages.cpp (the HUD message / secret-bonus / idle-grunt
// overlays) and BootyWalkAnim.cpp (the per-frame walking-grunt tick). Both TUs are
// __thiscall methods on BzState reaching the same retail sub-objects; the two files
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

// The per-level record (g_mgrSettings->m_levelRecord). m_recordBase points at the
// group-records table GetRecordValue indexes (0x40-byte stride, +0x28 payload).
struct BzLevelRecord {
    void* m_recordBase; // +0x00  group-records table base
    i32 m_levelIndex;   // +0x04  area/level index
    i32 m_suppressGate; // +0x08  suppress gate
    i32 m_worldFlag;    // +0x0c  world/training flag
    char m_pad10[0x44 - 0x10];
    i32 m_progressFlag; // +0x44  progress flag

    i32 GroupAllScored();        // 0xfce80 (thunk 0x2bda)
    i32 AllRecordsInBounds();    // 0xfccf0 (thunk 0x2bc6)
    float GroupRatio();          // 0xfce00 (thunk 0x30d0)
    i32 GetRecordValue(i32 idx); // 0xfced0 (thunk 0x2bf3), __thiscall(int)
};
SIZE_UNKNOWN(BzLevelRecord);

// The top-level window holder reached via g_mgrSettings->m_wnd; m_hwnd is its HWND.
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
struct BzSoundPlayer {
    i32 IsPlaying();                            // 0x1353f0
    void ConfigurePlay(i32 tag, i32, i32, i32); // 0x1360d0
};
SIZE_UNKNOWN(BzSoundPlayer);

// A named ambient sound entry. m_player is the playback sub-object; m_lastPlayed /
// m_interval rate-limit the cue.
struct BzSoundEntry {
    i32 Play(i32 tag, i32, i32, i32); // 0x1f940 (thunk 0x25fe)
    char m_pad00[0x10];
    BzSoundPlayer* m_player; // +0x10  player sub-object
    u32 m_lastPlayed;        // +0x14  last-played stamp
    u32 m_interval;          // +0x18  interval
};
SIZE_UNKNOWN(BzSoundEntry);

struct BzFindTable {
    void Find(const char* name, BzSoundEntry** out); // 0x1b8438 (Lookup)
};
SIZE_UNKNOWN(BzFindTable);

struct BzSoundSet {
    char m_pad00[0x10];
    BzFindTable m_findTable; // +0x10
    char m_pad14[0x30 - 0x14];
    i32 m_playing; // +0x30  is-playing gate
};
SIZE_UNKNOWN(BzSoundSet);

// g_mgrSettings->m_soundHolder->m_spriteFactory - the canonical CSpriteFactory
// (<Gruntz/SpriteFactory.h>) the booty setup builds its per-player idle grunts
// through (CreateSprite @0x1597b0).
struct BzSoundHolder {
    char m_pad00[0x8];
    CSpriteFactory* m_spriteFactory; // +0x08  sprite/animation factory
    char m_pad0c[0x28 - 0xc];
    BzSoundSet* m_soundSet; // +0x28
};
SIZE_UNKNOWN(BzSoundHolder);

// g_mgrSettings->m_cuePlayer - fires a positional sound cue.
struct BzCuePlayer {
    void Play(i32, i32, i32, i32, i32, i32); // thunk 0x39f4, __thiscall 6-arg
};
SIZE_UNKNOWN(BzCuePlayer);

// g_mgrSettings->m_selSource - resolves the active selection handle.
struct BzSelSource {
    i32 GetSel(i32, i32); // thunk 0x4165
};
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

    void ChangeState(i32); // 0x8fab0 (thunk 0x201d)
};
SIZE_UNKNOWN(BzGameReg);
extern "C" BzGameReg* g_mgrSettings; // *0x64556c

// The grunt-data loader (BzSink::m_loader): Load/Finish; m_data feeds the notify.
struct BzLoader {
    void Load();   // 0x158ee0
    void Finish(); // 0x158e90
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

struct BzSinkSub {
    void Free(); // 0x137a80
};
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

// The booty/secret game-state object hosting the HUD overlays + the per-frame
// walking-grunt tick. m_sink is the HUD message sink; m_trailSprites the trailing
// idle sprites; m_visSprites / m_animSprites the per-player idle sprites (visibility
// / animation); m_stepIndex the active-player step index.
class BzState {
public:
    // Overlays (BootyMessages.cpp).
    void ShowLevelCompleteMessage();            // 0x1c9d0
    i32 ShowSecretBonusMessage();               // 0x18f00
    i32 BuildBootyGruntIdleAnimation();         // 0x1ce60
    void FormatHudText(CString& out, i32 slot); // 0x1af70 (thunk 0x238d)
    i32
    RegisterMultiNamespaces(const char* mode, i32, i32, i32, i32, i32); // 0xfa1f0 (thunk 0x1e60)
    void StartTimer(i32, i32, i32, i32);                                // 0xfa8f0 (thunk 0x1843)
    void PassClickToPlayState(i32, i32, i32);                           // 0x8d780 (thunk 0x17c1)
    // One-time setup of the per-player idle/walking grunt sprite pairs + the per-frame
    // walking-grunt tick (BootyWalkAnim.cpp).
    i32 BuildBootyWalkingGruntz();  // 0x1b450
    i32 UpdateBootyWalkingGruntz(); // 0x1b690

    char m_pad00[0xc];
    BzSink* m_sink; // +0x0c  HUD message sink
    char m_pad10[0x1b4 - 0x10];
    i32 m_initGate; // +0x1b4  init/step gate (armed flag)
    char m_pad1b8[0x1bc - 0x1b8];
    i32 m_stateId; // +0x1bc  overlay/animation state id (0xc7/0xc8/-2)
    char m_pad1c0[0x1d0 - 0x1c0];
    i32 m_initOnce;         // +0x1d0  init-once gate
    i32 m_secretBannerOnce; // +0x1d4  secret-banner once gate
    char m_pad1d8[0x1ec - 0x1d8];
    CGameObject* m_trailSprites[4]; // +0x1ec  trailing idle sprites
    char m_pad1fc[0x200 - 0x1fc];
    i32 m_levelCompleteGate; // +0x200  level-complete gate
    char m_pad204[0x284 - 0x204];
    i32 m_readyFlags[8];    // +0x284  per-slot "ready text" flags
    i32 m_templateFlags[8]; // +0x2a4  per-slot "template" flags
    char m_pad2c4[0x2c8 - 0x2c4];
    CGameObject* m_visSprites[4];  // +0x2c8  per-player idle sprites (visibility)
    CGameObject* m_animSprites[4]; // +0x2d8  per-player idle sprites (animation)
    i32 m_stepIndex;            // +0x2e8  active-player step index
    i32 m_walkStarted;          // +0x2ec  walk-animation-started gate
    i32 m_soundStarted;         // +0x2f0  sound-started gate
    i32 m_secretGate;           // +0x2f4  secret-message gate
};
SIZE_UNKNOWN(BzState);

#endif // GRUNTZ_GRUNTZ_BZSTATE_H
