// CDDrawWorkerMgr.h - the DDraw "worker manager" held at the surface manager's
// +0x0c region and reached (as PreviewMgr+0x04 / status-bar +0x04) by several
// game-side TUs. It is the __thiscall receiver for the 0x158xxx surface ops
// (Method_158b40 install-image .. Method_159ef0). Polymorphic: its own vtable
// carries a slot at +0x3c that Method_159ef0 tail-calls; the class is never
// constructed in a reconstruction TU, so no ??_7 is emitted (the slots are
// declared-only for the through-vptr dispatch).
//
// Single owner of the class shape. The method bodies live in
// src/Gruntz/CDDrawSubMgr.cpp (owner unit); the other consumers (StateImages,
// CAttract, CSoundFxEmitter, LevelPreview) previously re-declared their own
// per-TU views of this class and now share this def. The two owned-pointer member
// types (CDDrawWorkerNode @+0x0c, CDDrawSurfacePair @+0x10..+0x18) are
// forward-declared (pointer-only here) so this header does not drag in - or clash
// with - a consumer's local surface/worker views.
//
// Field names are placeholders (m_<hexoffset>); only offsets + emitted code bytes
// are load-bearing (campaign doctrine).
#ifndef GRUNTZ_GRUNTZ_CDDRAWWORKERMGR_H
#define GRUNTZ_GRUNTZ_CDDRAWWORKERMGR_H

#include <Ints.h>
#include <rva.h>

// +0x0c worker (a geometry source @+0x04, a dispatcher @+0x08, flag word @+0x34).
class CDDrawWorkerNode;
// +0x10/+0x14/+0x18 surface elements (the front/back/overlay pairs).
class CDDrawSurfacePair;

// The worker manager (this for the 0x158xxx methods).  Polymorphic: its own
// vtable holds a slot at +0x3c that Method_159ef0 tail-calls.
SIZE_UNKNOWN(CDDrawWorkerMgr);
class CDDrawWorkerMgr {
public:
    virtual void Vfunc00();
    virtual void Vfunc04();
    virtual void Vfunc08();
    virtual void Vfunc0c();
    virtual void Vfunc10();
    virtual void Vfunc14();
    virtual void Vfunc18();
    virtual void Vfunc1c();
    virtual void Vfunc20();
    virtual void Vfunc24();
    virtual void Vfunc28();
    virtual void Vfunc2c();
    virtual void Vfunc30();
    virtual void Vfunc34();
    virtual void Vfunc38();
    virtual void Vfunc3c(); // slot 15 (@0x3c): Method_159ef0 forwards here

    // Surface ops.
    i32 Method_158b40(i32 arg1, i32 arg2);
    void Method_158b90();
    i32 Method_158bc0();
    i32 Method_158bf0(i32 a1, i32 a2, i32 a3);
    i32 Method_158cb0(i32 a1, i32 a2);
    void Method_158d50(i32 a1);
    i32 Method_158c70(CDDrawSurfacePair* dst);
    i32 Method_158d20();
    i32 Method_158dc0();
    i32 Method_158e40();
    i32 Method_158e90();
    i32 Method_158ee0();
    void Method_159ef0();

    char m_pad04[0x0c - 0x04];        // +0x04 .. +0x0b
    CDDrawWorkerNode* m_worker;       // +0x0c worker (flag at [+0x34])
    CDDrawSurfacePair* m_frontPair;   // +0x10
    CDDrawSurfacePair* m_backPair;    // +0x14
    CDDrawSurfacePair* m_overlayPair; // +0x18
};

#endif // GRUNTZ_GRUNTZ_CDDRAWWORKERMGR_H
