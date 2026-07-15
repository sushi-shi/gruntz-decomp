// GruntSpawnLevel.cpp - the out-of-line default constructor + destructor of
// GruntzPlayer, the per-player options/state record CGruntzMgr embeds four of at
// +0x150 (<Gruntz/GruntzPlayer.h>).
//
// The two local views that used to live here - `CGruntSpawnLevel` (Ghidra's guess at the
// 0xda790 ctor's class) and `Mgr38` (its opaque +0x38 sub-object) - were fake names for
// classes the tree already had. PROOF:
//   * CGruntzMgr's ctor/dtor hand the __ehvec iterators the m_options[4] element ctor and
//     dtor through ILT thunks 0x2a7c / 0x1465, which chase to 0x0da790 / 0x083260 - the
//     two bodies below. So this class IS the m_options element, i.e. GruntzPlayer (which
//     already owns 0x0da870 / 0x0da960 / 0x0da9e0 / 0x0dace0 and the identical layout).
//   * `Mgr38`'s ctor/dtor (0x024dc0 / 0x024f80) ARE ??0CBattlezMapConfig@@QAE@XZ /
//     ??1CBattlezMapConfig@@QAE@XZ - the real 0x1e8-byte bundle, exactly the 0x38..0x220
//     span. It is a typed member of GruntzPlayer now.
// Both are folded; this TU declares no types at all.
#include <Mfc.h>         // CString (MFC TU - precedes <windows.h>)
#include <EmptyString.h> // g_emptyString

#include <Gruntz/GruntzPlayer.h>
#include <rva.h>

// The global empty C string the CString member is seeded from (0x6293f4).

// ===========================================================================
// GruntzPlayer::GruntzPlayer()  (0x0da790) - THE default constructor
// ===========================================================================
// Construct the CString m_name + the CBattlezMapConfig m_038, then run the shared
// frameless field seed (the Clear body at 0x0da960, /O2-inlined here exactly as retail
// inlines it: the -1 / -2 / 1 / 0xf sentinels + the empty-string assign into m_name).
// The two destructible members drive the /GX frame.
// @early-stop
// /GX EH-state wall (docs/seh-eh.md): the member ctor calls (CString, CBattlezMapConfig),
// the empty-string assign and the field seeds are all recovered; the residual is a
// 2-destructible-member ctor's EH state numbering + cl's interleave of the field stores
// (same family as the CWarlord dtor wall).
RVA(0x000da790, 0xb0)
GruntzPlayer::GruntzPlayer() {
    m_22c = 0;
    m_230 = 0;
    m_playerIndex = -1;
    m_018 = -2;
    m_020 = 0;
    m_028 = 0;
    m_014 = 1;
    m_name = g_emptyString;
    m_008 = 0;
    m_010 = 0;
    m_220 = 0;
    m_224 = 0;
    m_comboSel = 0xf;
    m_02c = 0;
    m_030 = 0;
    m_22c = 0;
    m_230 = 0;
}

// ===========================================================================
// GruntzPlayer::~GruntzPlayer  (0x083260)
// ===========================================================================
// Reverse the ctor: run the shared field seed (Clear, 0x0da960 - retail's FIRST call in
// this body; it is referenceable now that 0x0da960 is bound as a METHOD instead of a
// phantom second constructor, which is what previously blocked this dtor), then cl
// destructs the +0x38 CBattlezMapConfig and the CString m_name. The /GX frame numbers
// the teardowns 2 / 0 / -1.
// @early-stop
// /GX teardown-state wall: the Clear() call + both member destructions are present and
// bind to their real bodies; the residual is cl's EH state numbering for the 2-member
// teardown (same family as the ctor above).
RVA(0x00083260, 0x57)
GruntzPlayer::~GruntzPlayer() {
    Clear();
}
