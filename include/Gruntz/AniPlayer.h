// AniPlayer.h - CAniPlayer, the timed-play SBI leaf (placeholder name; the exact
// RTTI leaf is unpinned - @identity-TODO below).
//
// IDENTITY (dossier #16, waveM-judgment): the old standalone "CAniPlayer" view was
// a parallel model of the SBI item chain itself - its +0x04/+0x14/+0x24/+0x28/
// +0x2c/+0x30/+0x34/+0x38 fields are CStatusBarItem/CSBI_Image/CSBI_ImageSet's,
// its +0x3c..+0x50 block is CSBI_ImageSetAni's m_3c..m_50, and its Init/Tick/
// SetRange/TickRenderCurrent/TickRenderFrame bodies are those classes' vtable
// slot bodies (proven via vtbl 0x1eac0c/0x1eac4c/0x1ead6c thunks; re-attributed
// to SBI_Image.h / SBI_ImageSet.h / SBI_ImageSetAni.h). What remains HERE is the
// deeper leaf that adds the i64 timed-play window at +0x58/+0x60 and the four
// non-slot methods driving it:
//   0x0e5ad0  Start        (Init + stamp the timed-play window)
//   0x0e5b90  TickToggle   (flip the frame between the range endpoints on expiry)
//   0x0e5c10  RenderCel    (resolve + blit the current cel)
//   0x0e5c90  Serialize    (chain the CSBI_ImageSetAni leg + round-trip the window)
// @identity-TODO: no vtable slot points at these four (non-virtual) and no direct
// caller survives outside their ILT thunks; the class is a CSBI_ImageSetAni-derived
// leaf (Serialize chains 0xe7cd0 as its direct base leg) - likely the statz-tab /
// warlord-head family, unpinned. Its obj is the [0xe5800-frags .. 0xe5d17] block.
#ifndef GRUNTZ_GRUNTZ_CANIPLAYER_H
#define GRUNTZ_GRUNTZ_CANIPLAYER_H

#include <rva.h>

#include <Gruntz/SBI_ImageSetAni.h> // the proven base chain (fields m_3c..m_50 + slot bodies)
#include <Gruntz/SerialArchive.h>   // CFileMemBase (Serialize's stream view)

class CAniPlayer : public CSBI_ImageSetAni {
public:
    // Forwards all 14 dwords to the base CSBI_ImageSetAni::Init (slot 13), then stamps the
    // timed-play window. The rect is by VALUE - retail builds it as one 16-byte block in
    // the outgoing frame (see the note on the body).
    i32 Start(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 a2,
        i32 a3,
        SbRect rc,
        const char* key,
        i32 b0,
        i32 b1,
        i32 b2,
        i32 b3,
        i32 b4
    );                                                          // 0xe5ad0
    i32 TickToggle(i32 param);                                  // 0xe5b90
    i32 RenderCel();                                            // 0xe5c10
    i32 Serialize(CFileMemBase* arc, i32 mode, i32 a3, i32 a4); // 0xe5c90

    i32 m_58; // +0x58  timed-play window start (i64 lo)
    i32 m_5c; // +0x5c  (i64 hi)
    i32 m_60; // +0x60  timed-play window duration (i64 lo)
    i32 m_64; // +0x64  (i64 hi)
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_CANIPLAYER_H
