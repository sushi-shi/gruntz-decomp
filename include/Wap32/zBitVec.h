#ifndef WAP32_ZBITVEC_H
#define WAP32_ZBITVEC_H

#include <Ints.h>
#include <rva.h>

extern char g_containerName[];    // 0x6bf408 (base ctor const char* arg)
extern i32 g_defaultProjActSize;  // 0x61ad28 (fallback capacity)
extern void* g_projActCache;      // 0x6bf464 (?g_projActCache@@3PAXA)
extern void* g_retAddrBreadcrumb; // 0x6bf428 (?g_projActAllocResult; OOM record cell)
extern void* g_projActName;       // 0x6bf454 (bad-arg diagnostic record cell)

void* GetRetAddr();       // 0x16d990
void* GetCallerRetAddr(); // 0x16e0f0

struct CVariantSlot;

class zErrHandling {
public:
    zErrHandling(void* errSink); // 0x16d9c0 (defined in src/Gruntz/GameText.cpp)
    virtual ~zErrHandling();     // [0] the ONLY slot (??_G 0x16da40; real ~ at 0x16da60)

    // 0x034960: the OUTLINED overflow/OOM report path (defined in BattlezMapConfig.cpp,
    // where it sits in retail RVA order). Body = `g_retAddrBreadcrumb = GetRetAddr();
    // m_errSink->Set(this, sentinel, code);` - the exact tail of the inlined grow-on-miss
    // fast path (CActReg::ResolveEntry spells the same two statements). It was modeled as a
    // .cpp-local `ZErrTarget` view {vptr@0, m_err@+0x04}: that IS this class's layout
    // (vptr@0, m_errSink@+0x04), so the view is dissolved onto zErrHandling.
    void Report(i32 sentinel, i32 code); // 0x034960

    CVariantSlot* m_errSink; // +0x04  the error sink this object registers with
};
SIZE(0x8); // { vptr @0, m_errSink @4 }
SIZE_UNKNOWN(); // error-reporter subobject view

class zBitVec : public zErrHandling {
public:
    zBitVec();                                // default (link-embedded; see CUserBaseLink)
    zBitVec(i32 idx, i32 sizehint);           // 0x16d790 (??0zBitVec@@QAE@HH@Z)
    zBitVec(const char* tokens, i32 minSize); // 0x16d3a0 (whitespace/'-' number-list parser)
    zBitVec& operator=(const zBitVec& o);     // 0x16d2f0 (deep copy of the word band)
    virtual ~zBitVec() OVERRIDE;              // [0] 0x16d2a0 (real ~; ??_G thunk @0x16d2d0)
    i32 SetSize(i32 nbits);                   // 0x16e100 (?SetSize@zBitVec@@QAEHH@Z)
    i32 EnsureSize(i32 nbits);                // 0x1936e0 (?EnsureSize@zBitVec@@QAEHH@Z)
    zBitVec* Or(zBitVec* o);                  // 0x193680 (word-wise union; grows to o)
    zBitVec* SetBit(u32 idx);                 // 0x193640 (src/Utils/BitArray.cpp)

    i32 m_capacity; // +0x08  capacity in bits (signed: SetSize's round-up matches as
                    // int; the ctor/EnsureSize cast (u32) for the unsigned `>0x20` jbe)
    u32* m_words;   // +0x0c  SBO word band (inline u32 when m_capacity <= 0x20)
};
SIZE(0x10);


#endif // WAP32_ZBITVEC_H
