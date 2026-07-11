// zBitVec.h - CContainerErr (the WAP32 container-error base, vtable
// ??_7CContainerErr@@6B@ @0x5f04cc) + zBitVec (its derived small-buffer bit-vector,
// vtable ??_7zBitVec@@6B@ @0x5f04c8). These are the devs' REAL class names: zBitVec
// (from the delinked ??0zBitVec@@QAE@HH@Z / ?SetSize@zBitVec / ?EnsureSize@zBitVec
// symbols) derived from CContainerErr (its ??_7CContainerErr@@6B@ RTTI-less vtable).
//
// CANONICAL home for the container family that was modeled 3x: <Gruntz/UserBaseLink.h>
// (as the Ghidra label "EngStr"), <Wap32/EngStr.h> (as unrelated CContainerErr +
// zBitVec), and <Gruntz/ProjActCache.h> (as zBitVec : CContainerErr). The "EngStr"
// class IS zBitVec: its ~ (0x16d2a0), operator= (0x16d2f0) and the 836B ctor
// (0x16d3a0) all stamp the ??_7zBitVec vptr @0x5f04c8 and derive from CContainerErr.
//
// LAYOUT (proven): CButeStore (a CContainerErr, CObj50 multiple-inheritance dtor,
// DiscoveredEh.cpp) places CObj50 at +0x08 -> CContainerErr is exactly 8 bytes
// {vptr@0, m_errSink@4}. The base ctor (0x16d9c0) writes ONLY +0x04 and the vptr;
// the +0x08 capacity and +0x0c word band are written only by zBitVec's own methods,
// so they are zBitVec fields, not CContainerErr fields. GameText.h's non-virtual
// 8-byte CContainerErr view corroborates the 8-byte base.
//
// NOTE - the CContainerErr CTOR (0x16d9c0) is DEFINED (byte-exact) in gametext.cpp
// under a deliberately NON-virtual dual-view (its ctor must store the vptr LAST, so a
// real `virtual` would regress it - vtable-conversion-log.md). That is a required
// ODR split (like CGameRegistry/CGruntzMgr); GameText.h and this header never coexist
// in one TU. Here the ctor is declared-only, so derived base-ctor calls reloc-mask.
#ifndef WAP32_ZBITVEC_H
#define WAP32_ZBITVEC_H

#include <Ints.h>
#include <rva.h>

// Container-error diagnostic globals (.data): the const char* name the base ctor
// records, the fallback bit-vector size, and the shared error-record cells the
// container OOM paths report through. Shared by every container ctor (zBitVec's,
// CUserBaseLink's) - reloc-masked. (The "projAct" names predate the recovery that
// this is the generic container error infrastructure.)
DATA(0x002bf408)
extern char g_containerName[]; // 0x6bf408 (base ctor const char* arg)
DATA(0x0021ad28)
extern i32 g_defaultProjActSize; // 0x61ad28 (fallback capacity)
DATA(0x002bf464)
extern void* g_projActCache;      // 0x6bf464 (?g_projActCache@@3PAXA)
extern void* g_retAddrBreadcrumb; // 0x6bf428 (?g_projActAllocResult; OOM record cell)
extern void* g_projActName;       // 0x6bf454 (bad-arg diagnostic record cell)

// _ReturnAddress()-style helper (0x16e0f0: mov eax,[ebp+4]; ret) - records where the
// failing allocation was requested. Reloc-masked (no body).
void* GetCallerRetAddr(); // 0x16e0f0

// The +0x04 error sink: the container reports sizing/argument failures through it
// (Set 0x16d850) and unregisters at teardown (Remove 0x16e360). Its ONE real
// definition lives in <Bute/ButeTree.h> (the shared trie/registry error sink); the
// prior zErrRegistry (Wap32/EngStr.h) and CVariantSlot (ProjActCache.h) views of the
// SAME +0x04 object are folded into it. Only a forward declaration is needed here
// (m_errSink is a pointer); the TUs that CALL its methods include <Bute/ButeTree.h>.
struct CVariantSlot;

// The container-error reporting base. REAL polymorphic: the virtual dtor (0x16da60)
// lets cl emit ??_7CContainerErr and auto-stamp the implicit vptr; slots 1..6 are
// declared-only engine virtuals living in other TUs (reloc-masked references). The
// const-char* ctor (0x16d9c0) is declared-only here (defined non-virtually in
// gametext). The destructible base is what forces zBitVec's /GX ctor-unwind frame.
SIZE_UNKNOWN(CContainerErr);
class CContainerErr {
public:
    CContainerErr(const char* msg); // 0x16d9c0 (??0CContainerErr@@QAE@PBD@Z)
    virtual ~CContainerErr();       // [0] 0x16da40 (??_G slot); real ~ at 0x16da60

    virtual void Slot04_16dde0(); // [1] 0x16dde0 (declared-only)
    virtual void Slot08_16df20(); // [2] 0x16df20 (declared-only)
    virtual void Slot0C_16dfa0(); // [3] 0x16dfa0 (declared-only)
    virtual void Slot10_16ea80(); // [4] 0x16ea80 (declared-only)
    virtual void Slot14_16e9c0(); // [5] 0x16e9c0 (declared-only)
    virtual void Slot18_16ea20(); // [6] 0x16ea20 (declared-only)

    CVariantSlot* m_errSink; // +0x04  the registered error handler / sink
};

// zBitVec - the derived small-buffer bit-vector: capacity in bits @+0x08, the DWORD
// word band @+0x0c (when capacity <= 0x20 the single word is stored INLINE at +0x0c
// and addressed as &m_words). Its own vtable ??_7zBitVec @0x5f04c8 is stamped by each
// ctor right after the base ctor returns; the virtual dtor override makes cl emit the
// distinct most-derived vtable + the implicit re-stamp.
SIZE(zBitVec, 0x10);
class zBitVec : public CContainerErr {
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

// NOTE: the CUserBaseLink link sub-object (which embeds a zBitVec) lives in
// <Gruntz/UserBaseLink.h>, NOT here - putting it in this header pulls it into
// src/Wap32/EngStr.cpp and header-fattens/reschedules zBitVec::SetSize (measured
// 98.7% -> 85.3%). EngStr.cpp needs only the container classes above.

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
VTBL(zBitVec, 0x001f04c8);

#endif // WAP32_ZBITVEC_H
