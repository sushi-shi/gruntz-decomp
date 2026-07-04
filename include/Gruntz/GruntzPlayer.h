// GruntzPlayer.h - the per-player options/state record embedded in CGruntzMgr at
// +0x150 (a 4-element array, one slot per player; each 0x238 bytes). The slot
// carries a name CString (default "Player"), a block of config scalars, an
// embedded CBattlezMapConfig spawn/board bundle at +0x38 (0x1e8 B; see
// <Gruntz/BattlezMapConfig.h>), and a trailing scalar block (+0x220..). No vtable
// (the class is non-polymorphic). NOTE: the ctors here do NOT member-construct the
// embedded bundle (the default ctor is frameless - it cannot construct the bundle's
// throwing MFC array members), so the slot is modeled as raw storage; the bundle is
// constructed on the slot elsewhere. Hence the member stays `char m_038[..]`.
//
// Two constructors (a default and an int-seeded one), a frameless re-init that
// clears the scalar block, a Serialize that streams every field through a
// CArchive-like order object (kind 4 = save / kind 7 = load) and forwards the
// 4-arg command to the +0x38 bundle, plus a static helper returning the default
// player name "Player".
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_GRUNTZPLAYER_H
#define SRC_GRUNTZ_GRUNTZPLAYER_H
#include <rva.h>

#include <Mfc.h> // CString (real MFC, 4-byte m_pchData)

SIZE_UNKNOWN(GruntzPlayer);
class GruntzPlayer {
public:
    GruntzPlayer();          // 0x0da960 (default)
    GruntzPlayer(i32 index); // 0x0da870 (/GX, seeds the name with "Player")
    i32 Reset();             // 0x0da9e0 (frameless re-init; empties name, returns 1)
    CString GetName();       // 0x01f450 (return m_name by value)
    i32 Serialize(void* ar, i32 kind, i32 a3, i32 a4); // 0x0dace0
    static CString GetDefaultName();                   // 0x0dafb0 (/GX, returns "Player")

    i32 m_playerIndex;          // +0x000  = -1 (default) / index (seeded)
    CString m_name;             // +0x004  name ("Player")
    i32 m_008;                  // +0x008
    i32 m_00c;                  // +0x00c  (serialized)
    i32 m_010;                  // +0x010  = 0
    i32 m_014;                  // +0x014  = 1
    i32 m_018;                  // +0x018  = -2 (the embedded sub-object EH state)
    i32 m_01c;                  // +0x01c  (serialized)
    i32 m_020;                  // +0x020  = 0
    i32 m_024;                  // +0x024  (serialized)
    i32 m_028;                  // +0x028
    i32 m_02c;                  // +0x02c  = 0
    i32 m_030;                  // +0x030  = 0
    char m_pad034[0x38 - 0x34]; // +0x034
    char m_038[0x220 - 0x38];   // +0x038  embedded CBattlezMapConfig bundle (0x1e8 B, raw storage)
    i32 m_220;                  // +0x220  = 0
    i32 m_224;                  // +0x224  = 0
    i32 m_228;                  // +0x228  = 0xf
    i32 m_22c;                  // +0x22c  = 0
    i32 m_230;                  // +0x230  = 0
    char m_pad234[0x238 - 0x234]; // +0x234
}; // 0x238

#endif /* SRC_GRUNTZ_GRUNTZPLAYER_H */
