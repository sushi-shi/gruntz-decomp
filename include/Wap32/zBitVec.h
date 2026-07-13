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
// (STALE-WALL NOTE, corrected: this used to say "GameText.h and this header never coexist
// in one TU". That was false - GameText.h had exactly ONE includer, so the collision it
// warned about could not happen. Its duplicate CContainerErr view is folded in and gone;
// GameText.h now includes THIS header. Do not resurrect the split.)
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

// The container-error reporting base - the ONE canonical model of the class (the former
// <Gruntz/GameText.h> duplicate view is folded in; nothing else models it).
//
// EXACTLY ONE VIRTUAL. ??_7CContainerErr@@6B@ @0x1f04cc is a ONE-SLOT vtable (its slot 0
// is the ??_G at 0x16da40). The six "Slot04_16dde0 .. Slot18_16ea20" virtuals that used
// to be declared here were FABRICATED: 0x1f04d0 / 0x1f04d4 / 0x1f04d8 / 0x1f04dc /
// 0x1f04e0 / 0x1f04e4 are not this vtable's slots 1..6 - they are the NEIGHBOURING
// classes' own one-slot vtables (CTypeKeyColl, zDArray, CButeNodeEntry, ...), read as if
// they were a 7-slot table. Declaring them inflated every derived class's vtable.
//
// +0x04 IS AN OBJECT POINTER, not a message string: the destructor (0x16da60) does
//     push 0 / mov [ecx],??_7CContainerErr / push ecx / mov ecx,[ecx+4] / call 0x16e360
// i.e. it loads +0x04 into ecx as the `this` of a __thiscall (the error-sink's Remove,
// registering/unregistering this object). So the ctor argument is the SINK to report
// through - the legacy `const char* msg` typing was the mis-model. Typed void* here: the
// call sites pass a sink global by address and no longer need a cast.
SIZE(CContainerErr, 0x8); // { vptr @0, m_errSink @4 }
class CContainerErr {
public:
    CContainerErr(void* errSink); // 0x16d9c0 (defined in src/Gruntz/GameText.cpp)
    virtual ~CContainerErr();     // [0] the ONLY slot (??_G 0x16da40; real ~ at 0x16da60)

    CVariantSlot* m_errSink; // +0x04  the error sink this object registers with
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
VTBL(CContainerErr, 0x001f04cc); // ??_7CContainerErr@@6B@ - ONE slot (the dtor)
VTBL(zBitVec, 0x001f04c8);

#endif // WAP32_ZBITVEC_H
