#ifndef GRUNTZ_WAP32_ZVEC_H
#define GRUNTZ_WAP32_ZVEC_H

#include <Ints.h>
#include <Wap32/zBitVec.h>
#include <rva.h>
#include <AddrWord.h>

struct CVariantSlot;

inline void* ZVecNoScratch() {

    AddrWord<char> sentinel;
    sentinel.m_word = 1;
    return sentinel.m_addr;
}

class _zvec : public zErrHandling {
public:
    _zvec(i32 stride, i32 lo, i32 hi, void* scratch);

    void* GrowTo(i32 idx, i32 at);
    char* IndexToPtr(i32 idx);
    virtual ~_zvec() OVERRIDE;

    i32 m_lo;
    i32 m_hi;
    char* m_base;
    char* m_spare;
    i32 m_stride;
    char* m_alloc;
    i32 m_grown;
};
SIZE(0x24);

class _zdvec : public _zvec {
public:
    _zdvec(i32 stride, i32 lo, i32 hi, void* scratch);
    char* IndexToPtr(i32 i);
};
SIZE(0x24);

template<class T> class zDArray : public _zdvec {
public:
    zDArray(i32 lo, i32 hi);
    virtual ~zDArray() OVERRIDE;

    T* Resolve(i32 id);
    T* ResolveEntry(i32 id);

    T* ResolveEntryCallReport(i32 id);

    static T* AsElem(char* p) {
        union {
            char* m_bytes;
            T* m_elem;
        } band;
        band.m_bytes = p;
        return band.m_elem;
    }
};
SIZE_UNKNOWN();

#endif // GRUNTZ_WAP32_ZVEC_H
