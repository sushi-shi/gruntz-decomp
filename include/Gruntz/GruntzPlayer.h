// GruntzPlayer.h - the per-player options/state record embedded in CGruntzMgr at
// +0x150 (a 4-element array, one slot per player; each 0x238 bytes). The slot
// carries a name CString (default "Player"), a block of config scalars, an
// embedded CBattlezMapConfig spawn/board bundle at +0x38 (0x1e8 B; see
// <Gruntz/BattlezMapConfig.h>), and a trailing scalar block (+0x220..). No vtable
// (the class is non-polymorphic).
//
// THIS IS THE ONE CLASS behind a 6-way name conflation (all now folded onto it):
//   GruntzPlayer | CGruntSpawnLevel (src/Gruntz/GruntSpawnLevel.cpp) |
//   CGruntzMgrOptions (GruntzMgr.h) | OptionsSlot (GruntzMgr.cpp) |
//   CMultiMgrOptions + CSlotConfig (Multi.h) | CFocusSlot (GameRegistry.h, still open)
// PROOF, from the ILT thunks the CGruntzMgr ctor/dtor hand to the __ehvec iterators for
// the m_options[4] array:  ctor thunk 0x2a7c -> 0x0da790,  dtor thunk 0x1465 -> 0x083260.
// 0x0da790 constructs a CString at +0x04 AND the +0x38 sub-object whose ctor/dtor are
// 0x024dc0 / 0x024f80 == ??0CBattlezMapConfig / ??1CBattlezMapConfig. So the array
// element IS this class, its default ctor IS 0x0da790, its dtor IS 0x083260, and the
// +0x38 member IS a real CBattlezMapConfig (not raw storage).
//
// COROLLARY (this is what un-sticks two old @early-stops): 0x0da960 is NOT a constructor.
// It is the frameless field-seed helper Clear() - a 0x5b twin of Reset (0x0da9e0, 0x60):
// same 14 scalar stores, no member construction, no /GX frame. It was mis-bound as
// ??0GruntzPlayer@@QAE@XZ, which is exactly why that "ctor" could never lose its EH frame
// (docs said "retail's is frameless and ours is not") and why ~CGruntSpawnLevel could not
// reference its own first teardown call.
//
// Two constructors (a default and an int-seeded one), the Clear/Reset field-seed pair, a
// Serialize that streams every field through a CArchive-like order object (kind 4 = save /
// kind 7 = load) and forwards the 4-arg command to the +0x38 bundle, plus a static helper
// returning the default player name "Player".
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_GRUNTZPLAYER_H
#define SRC_GRUNTZ_GRUNTZPLAYER_H
#include <rva.h>

#include <Gruntz/BattlezMapConfig.h> // the REAL +0x38 sub-object (0x1e8 B; ctor 0x24dc0)
#include <Mfc.h>                     // CString (real MFC, 4-byte m_pchData)

SIZE_UNKNOWN(GruntzPlayer);
class GruntzPlayer {
public:
    GruntzPlayer();  // 0x0da790 (default; constructs m_name + m_038, then Clear)
    ~GruntzPlayer(); // 0x083260 (Clear, then ~CBattlezMapConfig, then ~CString)
    // 0x0da870 (/GX, seeds the name with "Player"). NOT a constructor: the retail
    // tail is `mov eax,1; ret 4` (a ctor returns `this`), and both callers TEST the
    // return (CGruntzMgr::Run's 4-slot seed loop `je error`; the CPlay team reset).
    // The old ??0GruntzPlayer@@QAE@H@Z binding was a misidentification.
    i32 SeedForSlot(i32 index);
    void Clear(); // 0x0da960 (frameless field seed; ctor + dtor call it)
    i32 Reset();  // 0x0da9e0 (frameless re-init; empties name, returns 1)
    // 0x0db200 (ex "Cdb200::Swap", a 7th name for this class - see Play.cpp): move the
    // player onto sound/voice channel `channel` if it is free, releasing the old one.
    i32 SwapChannel(i32 channel);
    i32 ClearRoundState(); // 0x0daa60 (marks active, clears per-round scalars)
    RVA(0x0001f450, 0x20)
    CString GetName() {
        return m_name;
    }
    i32 Serialize(void* ar, i32 kind, i32 a3, i32 a4); // 0x0dace0
    i32 Deactivate(); // 0x0db2f0 (ex "Cdb2f0::Finalize"; clears the board bundle + m_020)
    static CString GetDefaultName(); // 0x0dafb0 (/GX, returns "Player")

    i32 m_playerIndex;  // +0x000  = -1 (default) / index (seeded)
    CString m_name;     // +0x004  name ("Player")
    i32 m_008;          // +0x008  per-player selected sprite descriptor/index (CPlay's
                        //         grid walk feeds m_options[g_curPlayer].m_008 to the
                        //         sprite table's GetSel/LoadSprite; Multi uses it as the
                        //         player/slot id for chat AddItem + the net-slot table)
    i32 m_00c;          // +0x00c  (serialized) per-mode id / sound id / key word
    i32 m_configId;     // +0x010  = 0; per-slot config id (LoadConfig arg; roster combo base)
    i32 m_014;          // +0x014  = 1; armed / arrival gate (roster: human-vs-computer)
    i32 m_slotKey;      // +0x018  = -2; slot key (CGruntzMgr::FindOptionsSlot match)
    i32 m_readyFlag;    // +0x01c  (serialized) roster: ready flag
    i32 m_liveGate;     // +0x020  = 0; loaded / live gate
    i32 m_clearedRound; // +0x024  (serialized) "already cleared this round"
    i32 m_joined;       // +0x028  joined
    i32 m_doneFlag;     // +0x02c  = 0; done
    i32 m_030;          // +0x030  = 0
    char m_pad034[0x38 - 0x34]; // +0x034
    // The REAL embedded spawn/board bundle. Proven by the array element ctor/dtor
    // (0x0da790 / 0x083260), whose +0x38 member calls are ??0/??1CBattlezMapConfig
    // (0x024dc0 / 0x024f80) - and by every consumer, all of which cast this block to
    // CBattlezMapConfig* to call LoadConfig / FreeArrays / Clear.
    CBattlezMapConfig m_038;      // +0x038  (0x1e8 B, ends at 0x220)
    i32 m_focusX;                 // +0x220  = 0 (snapped focus x)
    i32 m_focusY;                 // +0x224  = 0 (snapped focus y)
    i32 m_comboSel;               // +0x228  = 0xf in the ctor/Clear seed; the battlez
                                  //          dialog's per-slot dropdown selection (+1)
    i32 m_latency;                // +0x22c  = 0  net-slot latency (the roster watchdog
                                  //          displays it; ex the CNetPlayerSlot +0x37c view)
    i32 m_230;                    // +0x230  = 0
    char m_pad234[0x238 - 0x234]; // +0x234
}; // 0x238

#endif /* SRC_GRUNTZ_GRUNTZPLAYER_H */
