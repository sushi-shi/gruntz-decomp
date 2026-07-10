// AnimWorkerFull.h - the full anim-worker object (foreign retail vtable 0x5efb80),
// the class whose reduced message-pump facet is `Worker` in <Gruntz/AnimWorker.h>.
// Real-polymorphic: cl auto-stamps the vptr (??_7WorkerFull@@6B@; 10 declared-only
// slots reloc-mask). The 3-arg ctor lives in AnimWorkerCtor.cpp.
//
// Unifying WorkerFull with the pump-view `Worker` (same object, same +0x18/+0x1c
// offsets) is deferred - the ctor is @early-stop on a vptr-last wall and the two
// facets live in different TUs.
#ifndef GRUNTZ_ANIMWORKERFULL_H
#define GRUNTZ_ANIMWORKERFULL_H

#include <Ints.h>

struct WorkerFull {
    virtual void Slot00(); // +0x00  vptr
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    WorkerFull(i32 a, i32 b, i32 c);
    i32 m_04; // +0x04  <- b
    i32 m_08; // +0x08  <- c
    i32 m_0c; // +0x0c  <- a
    i32 m_10;
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    char m_pad20[0x170 - 0x20];
    i32 m_170;
    i32 m_174;
    i32 m_178;
};

#endif // GRUNTZ_ANIMWORKERFULL_H
