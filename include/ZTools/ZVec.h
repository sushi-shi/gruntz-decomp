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
    char* m_spare;
    i32 m_stride;
};

class _zdvec : public _zvec {
public:
    _zdvec(i32 stride, i32 lo, i32 hi, void* scratch);
    void* GrowTo(i32 idx, i32 at);
    char* IndexToPtr(i32 i) {
        char* r;
        m_grown = 0;
        if (i >= m_lo && i <= m_hi) {
            r = m_base + (i - m_lo) * m_stride;
        } else if (GrowTo(i, 0)) {
            r = m_base + (i - m_lo) * m_stride;
        } else {
            char* msg = g_errOutOfMem;
            g_retAddrBreadcrumb = GetRetAddr();
            m_errSink->Set(this, msg, 0xc);
            r = m_spare;
        }
        return r;
    }

    char* m_alloc;
    i32 m_grown;
};

#endif // GRUNTZ_ZTOOLS_ZVEC_H
