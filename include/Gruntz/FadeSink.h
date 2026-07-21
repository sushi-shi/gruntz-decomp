#ifndef GRUNTZ_GRUNTZ_FADESINK_H
#define GRUNTZ_GRUNTZ_FADESINK_H
#include <rva.h>

#include <Ints.h>

SIZE_UNKNOWN(IFadeSink);
// VTBL_ABSENT: never-constructed notify facet - RunFade dispatches slot 22 (+0x58,
// __stdcall) on an object reached via the Set2c(**) argument. @identity-TODO: every
// reconstructed SetConfig caller passes 0, so the receiver class is unrecovered
// (the __stdcall slot suggests a COM-shaped interface).
VTBL_ABSENT(IFadeSink);
struct IFadeSink { // real polymorphic; FadeNotify is slot 22 (+0x58), __stdcall
    virtual void S00();
    virtual void S01();
    virtual void S02();
    virtual void S03();
    virtual void S04();
    virtual void S05();
    virtual void S06();
    virtual void S07();
    virtual void S08();
    virtual void S09();
    virtual void S10();
    virtual void S11();
    virtual void S12();
    virtual void S13();
    virtual void S14();
    virtual void S15();
    virtual void S16();
    virtual void S17();
    virtual void S18();
    virtual void S19();
    virtual void S20();
    virtual void S21();
    virtual void __stdcall FadeNotify(i32, i32); // +0x58 (slot 22)
};

#endif // GRUNTZ_GRUNTZ_FADESINK_H
