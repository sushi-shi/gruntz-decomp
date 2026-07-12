// Fwd114ec0.cpp - the world-present toolbar-builder forwarder pair (its own dev obj:
// the contiguous 0x114ec0-0x114f3e .text block). Carved out of GruntzMgrCmd.cpp in
// REHOME D9 (that file's real body is CGruntzMgr::HandleCommand @0x862f0, whose
// WM_COMMAND 0x8070 path calls Fwd114ec0 via the 0x277a thunk).
//
// Fwd114ec0 @0x114ec0 - straight 6-arg forwarder to the guarded forwarder Fwd114f00.
// Fwd114f00 @0x114f00 - its a2 IS `this` (a CGruntzMgr - HandleCommand's case-0x8070
// caller passes `(i32)this`), so the chain a2->m_30->m_4->m_10->m_2c dissolves onto the
// real m_world (CWorldZ) -> m_4 (CWorldSub4) -> m_10 (the command-context object) ->
// +0x2c, which it forwards plus the six args to 0x267b. Only the final +0x2c object's
// class is unrecovered (a small placeholder). Both __cdecl.
#include <Ints.h>

#include <Gruntz/GruntzMgr.h> // the real CGruntzMgr (a2's true class) + m_world chain

#include <rva.h>

void Fwd114f00(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6); // 0x114f00 (0x21c1 thunk target)

// 0x114ec0 - straight 6-arg forwarder to the guarded forwarder Fwd114f00 (via the
// 0x21c1 thunk). The HandleCommand toolbar-builder path (thunk 0x277a) calls this.
RVA(0x00114ec0, 0x27)
void Fwd114ec0(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    Fwd114f00(a1, a2, a3, a4, a5, a6);
}

struct CObj114f { // the CWorldSub4::m_10 command-context object (identity-TODO)
    char pad0[0x2c];
    void* m_2c; // +0x2c
};
SIZE_UNKNOWN(CObj114f);
extern "C" void Func267b(void* v, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6); // 0x267b
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns): cl shares one pop;ret
// tail across the two null guards; retail emits the inline ret at each site. Deref
// chain + 6-arg re-push forward are byte-faithful.
RVA(0x00114f00, 0x3e)
void Fwd114f00(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    CObj114f* obj = (CObj114f*)((CGruntzMgr*)a2)->m_world->m_4->m_10;
    if (obj == 0) {
        return;
    }
    if (obj->m_2c == 0) {
        return;
    }
    Func267b(obj->m_2c, a1, a2, a3, a4, a5, a6);
}
