// LobbyObjB.cpp - the CMulti m_520 lobby object + its 0x64-stride slot element
// (mis-attributed to CMulti by the dynamic-this tracer). See CLobbyObjB.h.
//
// Reconstructed in ascending retail-RVA order. Both destructors are /GX (the
// embedded destructible members force the fs:0 EH frame); the body calls + the
// member teardowns are external no-body fns -> their `call rel32` reloc-mask.
// ---------------------------------------------------------------------------
#include <rva.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LobbyObjB.h>

// ---------------------------------------------------------------------------
// 0x0b6220: ~CLobbyObjB. Runs the lobby drain (body call) then vector-destroys
// the embedded 4x0x64 slot table at +0x20 (per-element dtor = ~CLobbySlot).
// The /GX frame + body call + 'eh vector destructor iterator' fall out of the
// member array (docs/patterns/eh-dtor-model-members-as-destructible.md). 100%.
RVA(0x000b6220, 0x54)
CLobbyObjB::~CLobbyObjB() {
    Body_bf000();
}

// ---------------------------------------------------------------------------
// 0x0b62a0: ~CLobbySlot. Runs a body call then tears down the +0x20 member.
// The /GX frame + body call + member ~CLobbySlotInner fall out of the destructible
// member (docs/patterns/eh-dtor-model-members-as-destructible.md). 100%.
RVA(0x000b62a0, 0x4a)
CLobbySlot::~CLobbySlot() {
    Body_c0bb0();
}

// ---------------------------------------------------------------------------
// 0x0bc3f0: fill `out` with the slot's host name (delegated to the +0xc manager's
// 0x1f450 getter) and return it.
// @early-stop
// by-value-CString-return NRVO-temp wall: retail (and the identical-shape 0x1f450
// getter it forwards to) reserves + zeroes a stack temp (`push ecx; mov [esp+4],0`)
// from the CString-by-value return path; our signature is locked to `CString*` by the
// matched CMulti.cpp call site (`*slot->BuildHostName(&nm)`), so that temp can't be
// reproduced. Delegation + return-the-arg are otherwise exact. ~67%.
RVA(0x000bc3f0, 0x1e)
CString* CLobbySlot::BuildHostName(CString* out) {
    *out = ((GruntzPlayer*)m_mgr)->GetName();
    return out;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CLobbyObjB);
SIZE_UNKNOWN(CLobbySlot);
SIZE_UNKNOWN(CLobbySlotInner);
SIZE_UNKNOWN(CLobbySlotMgr);
