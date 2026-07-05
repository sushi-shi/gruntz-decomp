// FadeSink.h - IFadeSink, the COM-style fade-notify sink CFader::RunFade pokes once
// per newly-reached frame (through the CFader's set-via-Set2c sink pointer). *sink is
// an interface whose first field is a C-vtable; slot 0x58 (== slot 22) is invoked
// __stdcall(this, 1, 0).
//
// P2 IDENTITY-RECOVERY TODO (flagged, not a wall): this is NOT a vendored DirectX
// interface - slot 22 of IDirectDrawSurface is GetSurfaceDesc (1 arg, mismatch) and
// IDirectSoundBuffer has only ~21 slots, so no ddraw/dsound method has a 22nd slot
// taking (this, i32, i32) (batch-2 interface-recovery task; the fade-vtbl worker found
// NO DX match). It is a genuine ENGINE-INTERNAL fade-notify sink whose concrete class
// lives in the fade-owner TU (unrecovered). Modeled as a named 1-slot interface here
// (real header, NOT a .cpp-local view); external/reloc-masked. When the owner TU is
// recovered, this dissolves onto the real class.
#ifndef GRUNTZ_GRUNTZ_FADESINK_H
#define GRUNTZ_GRUNTZ_FADESINK_H

#include <Ints.h>

struct IFadeSink;
SIZE_UNKNOWN(IFadeSinkVtbl);
struct IFadeSinkVtbl {
    char _00[0x58];
    void(__stdcall* m_58)(IFadeSink*, i32, i32); // +0x58 (slot 22) fade-notify
};
SIZE_UNKNOWN(IFadeSink);
struct IFadeSink {
    IFadeSinkVtbl* vtbl; // +0x00  C-vtable
};

#endif // GRUNTZ_GRUNTZ_FADESINK_H
