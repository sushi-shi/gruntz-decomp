#ifndef WAP32_ZBITVEC_H
#define WAP32_ZBITVEC_H

#include <Ints.h>
#include <rva.h>

struct CVariantSlot;
class istream;
class ostream;

extern CVariantSlot g_zBitSetErrorSlot;
extern CVariantSlot g_globalErrorSlot;
extern CVariantSlot g_dynamicArrayErrorSlot;
extern i32 g_defaultProjActSize;
extern void* g_retAddrBreadcrumb;

extern char* g_errDataInvalid;
extern char* g_errOverflow;
extern char* g_errOutOfRange;
extern char* g_errNullArg;
extern char* g_errExists;
extern char* g_errBadArg;
extern char* g_errNoFile;
extern char* g_errOutOfMem;

void* GetRetAddr();
void* GetCallerRetAddr();

class zErrHandling {
public:
    zErrHandling(CVariantSlot* errSink);
    virtual ~zErrHandling();

    void Report(void* sentinel, i32 code);

    CVariantSlot* m_errSink;
};
SIZE(0x8);
SIZE_UNKNOWN();

class zBitVec : public zErrHandling {
public:
    zBitVec();
    zBitVec(i32 idx, i32 sizehint);
    zBitVec(const char* tokens, i32 minSize);
    zBitVec& operator=(const zBitVec& o);
    virtual ~zBitVec() OVERRIDE;
    i32 SetSize(i32 nbits);
    i32 EnsureSize(i32 nbits);
    zBitVec* Or(zBitVec* o);
    zBitVec* SetBit(u32 idx);

    i32 m_capacity;

    union {
        u32* m_words;
        u32 m_inline;
    };

    i32 GetBit(u32 idx) const {
        if (idx >= static_cast<u32>(m_capacity)) {
            return 0;
        }
        const u32* words = static_cast<u32>(m_capacity) > 0x20 ? m_words : &m_inline;
        return (words[idx >> 5] & (1 << (idx & 0x1f))) != 0;
    }

    zBitVec* Set(u32 idx) {
        if (idx >= static_cast<u32>(m_capacity)) {
            return SetBit(idx);
        }
        u32* words = static_cast<u32>(m_capacity) > 0x20 ? m_words : &m_inline;
        words[idx >> 5] |= 1 << (idx & 0x1f);
        return this;
    }
};
SIZE(0x10);

ostream& operator<<(ostream& accum, const zBitVec& bits);
istream& operator>>(istream& accum, zBitVec& bits);

#endif // WAP32_ZBITVEC_H
