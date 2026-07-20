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
#include <Mfc.h> // CMapStringToPtr (the embedded find table)
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>         // CGameObject (the created idle-grunt sprites)

#include <rva.h>

class CString; // <Mfc.h>; used by reference in BzState::FormatHudText's declaration

// (BzLevelRecord is GONE - it was CBattlezData: +0x00 records base / +0x04 count
// (the (count-1)/4 group-of-4 math), +0x44 lands on m_scoreValue exactly.
// BzWndHolder is GONE - it was CGameWnd (m_hwnd @+0x04, <Wap32/Wap32.h>).)
#include <Gruntz/BattlezData.h> // CBattlezData (g_gameReg->m_scoreHud)

// The idle/walking grunt sprite (m_trailSprites[]/m_visSprites[]/m_animSprites[]
// elements) is the shared CGameObject (<Gruntz/UserLogic.h>): ApplyName (0x150540)
// caches the named first frame; ApplyLookupGeometry (0x1505b0) resolves its cycle
// geometry; the booty walk drives m_stateFlags (bit0 visible), the draw-fill block
// and the screen position (the former BzSprite view mis-read m_screenX/m_screenY
// as "sprite id"/"timer" - the stored values are onscreen coordinates; its +0x1a0
// completion sub is the canonical m_1c0/m_1c8 anim-sink pair).

// (BzSoundPlayer/BzSoundEntry/BzSoundSet are GONE - the "sound set" at
// g_gameReg->m_world->+0x28 IS CDDrawSubMgrLeafScan (same CMapStringToPtr @+0x10 -
// the mfc_class-audited Lookup 0x1b8438 band - busy gate m_30, stream m_2c), and its
// cached entries ARE LeafCue (m_10 player / m_14 stamp / m_18 interval, PlayIfElapsed).)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // the real keyed sound-cue cache
#include <Gruntz/LeafCue.h>               // the real cue element

// (BzGameReg is GONE - the eighth and last registry view of the *0x64556c
// CGruntzMgr singleton: m_wnd==the CGameMgr base's m_gameWnd, m_soundHolder==m_world,
// m_cuePlayer==m_cueSink, m_selSource==m_spriteFactory, m_levelRecord==m_scoreHud -
// all same offsets, several same TYPES; the empty BzCuePlayer/BzSelSource placeholder
// views dissolved with it. ChangeState_8fab0 was already on CGruntzMgr.)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)

// DISSOLVED (Fable A2, 2026-07-14): the "BzSink" HUD message-sink view WAS the
// canonical CDDrawSurfaceMgr (CState::m_c == g_gameReg->m_30) - proven by the
// call site's own address chain (BootyMessages 0x1ce60 reads `[0x64556c]+0x30`):
//   BzSink               == CDDrawSurfaceMgr      (<Gruntz/GameRegistry.h>)
//   BzSink::m_loader+04  == m_drawTarget (CDDrawSubMgrPages; Method_158ee0/158e90)
//   BzLoader::m_data+14  == CDDrawSubMgrPages::m_backPair (CDDrawSurfacePair*)
//   BzSink::m_notify+08  == m_8/m_childGroup (CDDrawChildGroup, vtbl 0x1efdc0);
//                           its "OnLoaded" slot 10 (+0x28) IS WalkDispatch2C
//                           @0x159c90 - the per-object render broadcast
//   BzSink::m_dropped+28 == m_28 (CSndHost == CDDrawSubMgrLeafScan); the +0x2c
//                           "dropped sprite" IS its m_2c held SoundStream (Stop()'d)
// The 11-slot BzSink8 placeholder vtable (s00..s24 + OnLoaded) is gone with it;
// BootyMessages.cpp now dispatches through the canonical classes, cast-free.

// NOTE: the former `class BzState` VIEW is dissolved - the booty/secret game-state
// object IS the canonical CBootyState (<Gruntz/GameMode.h>): the booty members
// (m_initGate/m_activation/m_trailSprites/m_visSprites/m_animSprites/m_stepIndex/...)
// now live on CBootyState; the HUD message sink the toasts read at +0xc IS the
// inherited CState::m_c holder (the canonical CDDrawSurfaceMgr - the former
// BzSink view of it is dissolved, see the note above).
// The overlay/tick bodies (BootyMessages.cpp, BootyWalkAnim.cpp) are real
// CBootyState:: methods; RegisterMultiNamespaces folds onto CState::FadeInTitle
// (0xfa1f0), StartTimer onto CBootyState::BuildPage (0xfa8f0), FormatHudText onto
// CState::FormatHudText (0x1af70), PassClickToPlayState onto CGruntzMgr (0x8d780,
// reached via the g_gameReg -> CGruntzMgr singleton cast).

#endif // GRUNTZ_GRUNTZ_BZSTATE_H
