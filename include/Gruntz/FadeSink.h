#ifndef GRUNTZ_GRUNTZ_FADESINK_H
#define GRUNTZ_GRUNTZ_FADESINK_H
#include <rva.h>

#include <Ints.h>

SIZE_UNKNOWN(IFadeSink);
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
