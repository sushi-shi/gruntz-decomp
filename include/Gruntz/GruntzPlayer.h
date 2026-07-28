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
#include <Gruntz/SerialArchive.h>    // CFileMemBase - the Serialize stream
#include <Mfc.h>                     // CString (real MFC, 4-byte m_pchData)

// The per-slot round-trip-time accumulator: a cumulative moving average, folded
// by CMulti's 0x420 handler as `avg = (avg * count + sample) / (count + 1)`.
// It is a real SUB-OBJECT, not two loose i32s, and the /GX bookkeeping proves it:
//  - ~GruntzPlayer (0x083260) enters at unwind state 2 with only m_name and m_038
//    to tear down, i.e. the class owns a THIRD destructible subobject declared
//    after m_038 whose destruction emits no code;
//  - the default ctor (0x0da790) zeroes BOTH dwords immediately after
//    ??0CBattlezMapConfig and before the state advance - the member-init slot -
//    and then Clear's inlined seed zeroes them AGAIN at the tail (which is why the
//    old two-loose-fields model had to duplicate the statements by hand).
struct PlayerLatency {
    PlayerLatency() {
        m_avg = 0;
        m_count = 0;
    }
    ~PlayerLatency() {}
    // Zeroing the pair through the sub-object (not two loose `m_latency.m_x = 0`
    // statements) is what keeps the preceding `m_comboSel = 0xf` IMMEDIATE store in
    // source position: cl floats a lone imm store to the end of a same-register store
    // RUN, and the inlined member call ends that run.
    void Clear() {
        m_avg = 0;
        m_count = 0;
    }

    i32 m_avg;   // +0x00 (0x22c) running mean round-trip time (the roster displays it)
    i32 m_count; // +0x04 (0x230) samples already folded into the mean
};
SIZE(0x8);

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
    i32 Serialize(CFileMemBase* ar, i32 kind, i32 a3, i32 a4); // 0x0dace0
    i32 Deactivate(); // 0x0db2f0 (ex "Cdb2f0::Finalize"; clears the board bundle + m_020)
    CString GetDefaultName(i32); // 0x0dafb0 (/GX, returns "Player"; ret 0x8 = thiscall + 1 arg)

    // +0x000  = -1 (default) / index (seeded). The net layer ships its low byte as the
    // per-command player id (CNetCmdSlot::ProcessCmd's `m_desc->m_playerIndex & 0xff`
    // ack-flag index; CMulti's `(char)slot->m_desc->m_playerIndex` wire field).
    i32 m_playerIndex;
    CString m_name; // +0x004  name ("Player")
    i32 m_008;      // +0x008  per-player selected sprite descriptor/index (CPlay's
                    //         grid walk feeds m_options[g_curPlayer].m_008 to the
                    //         sprite table's GetSel/LoadSprite; Multi uses it as the
                    //         player/slot id for chat AddItem + the net-slot table)
    i32 m_00c;      // +0x00c  (serialized) per-mode id / sound id / key word
    i32 m_configId; // +0x010  = 0; per-slot config id (LoadConfig arg; roster combo base)
    i32 m_014;      // +0x014  = 1; armed / arrival gate (roster: human-vs-computer)
    // +0x018  = -2; the DirectPlay player id owning this roster slot (CGruntzMgr::
    // FindOptionsSlot's match key; compared against CMulti::m_hostIndex; the net layer
    // uses it as SetData's `idTo` and as CNetSession::FindCmdSlot's lookup key).
    i32 m_slotKey;
    i32 m_readyFlag;            // +0x01c  (serialized) roster: ready flag
    i32 m_liveGate;             // +0x020  = 0; loaded / live gate
    i32 m_clearedRound;         // +0x024  (serialized) "already cleared this round"
    i32 m_joined;               // +0x028  joined
    i32 m_doneFlag;             // +0x02c  = 0; done (CNetSession::Reconcile + CMulti set it on the
                                //          player whose channel just got reset/dropped)
    i32 m_030;                  // +0x030  = 0
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
    PlayerLatency m_latency;      // +0x22c  the round-trip-time accumulator
    char m_pad234[0x238 - 0x234]; // +0x234
}; // 0x238
SIZE_UNKNOWN();

#endif /* SRC_GRUNTZ_GRUNTZPLAYER_H */
