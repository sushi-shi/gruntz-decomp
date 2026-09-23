#ifndef GRUNTZ_ZTOOLS_ZVEC_H
#define GRUNTZ_ZTOOLS_ZVEC_H

#include <rva.h>

#include <AddrWord.h>
#include <Enums.h>
#include <Ints.h>
#include <ZTools/Error.h>

GZ_ENUM_CONST_BEGIN(ZVecSentinel)
    ZVEC_NO_SCRATCH_ADDRESS = 1
GZ_ENUM_CONST_END(ZVecSentinel)

inline char* ZVecNoScratch() {

    AddrWord<char> sentinel;
    sentinel.m_word = ZVEC_NO_SCRATCH_ADDRESS;
    return sentinel.m_addr;
}

class _zvec : public zErrHandling {
public:
    _zvec(i32 stride, i32 lo, i32 hi, void* scratch);

    virtual ~_zvec() OVERRIDE;

    i32 m_lo;
    i32 m_hi;
    char* m_base;
    void* m_spare;
    i32 m_stride;
};

class _zdvec : public _zvec {
public:
    _zdvec(i32 stride, i32 lo, i32 hi, void* scratch);
    void* GrowTo(i32 idx, i32 at);
    void* IndexToPtr(i32 i) {
        m_grown = 0;
        return (i < m_lo || i > m_hi) ? (GrowTo(i, 0) ? m_base + m_stride * (i - m_lo)
                                                      : (Report(g_errOutOfMem, 0xc), m_spare))
                                      : m_base + m_stride * (i - m_lo);
    }

    void* m_alloc;
    i32 m_grown;
};

#endif // GRUNTZ_ZTOOLS_ZVEC_H
