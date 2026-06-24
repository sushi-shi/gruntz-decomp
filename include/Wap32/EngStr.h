// EngStr.h - the WAP32 engine string/bit-vector helpers around the Ghidra
// `EngStr` text routine (0x16d3a0, C:\Proj\incs). Recovered from the four
// trace-discovered methods of the cluster; the class identities differ:
//
//   * CContainerErr - the container-error reporting BASE (vtable @0x5f04cc, an
//     error-handler subobject at +0x04). It seeds an error-message table
//     ("Out of memory", "Overflow", ...) in its ctor (0x16d9c0) and unregisters
//     the handler in its dtor (0x16da60). The _zvec/zDArray dynamic-vector
//     family and the Bute containers all derive from it (that is the base dtor
//     the derived ~zDArray etc. chain into - the prior "~_zvec" guess for
//     0x16da60 was wrong: it is ~CContainerErr).
//
//   * zBitVec - a dynamic bit array: +0x08 capacity (in bits), +0x0c word band
//     (DWORD[]). SetSize(0x16e100) rounds the requested bit count up to whole
//     32-bit words, mallocs + zero-fills the band, and reports a bit capacity of
//     nwords*32 (or 32 with no band for a <=32-bit request).
#ifndef GRUNTZ_WAP32_ENGSTR_H
#define GRUNTZ_WAP32_ENGSTR_H

#include <Ints.h>
#include <rva.h>

// The handle-table registry the container-error handler unregisters from
// (a global slot pool; 0x16e360 is its remove-entry method). Modeled as an
// opaque __thiscall callee so its rel32 call is reloc-masked.
class zErrRegistry {
public:
    i32 Remove(void* obj, i32 flag); // 0x16e360
};

// The container-error reporting base: vptr + the error-handler subobject.
class CContainerErr {
public:
    ~CContainerErr(); // 0x16da60

    void* m_vptr;        // +0x00  vtable (0x5f04cc)
    zErrRegistry* m_err; // +0x04  the registered error handler
};

// A dynamic bit array (capacity in bits at +0x08, the DWORD word band at +0x0c).
class zBitVec {
public:
    i32 SetSize(i32 nbits); // 0x16e100

    void* m_vptr;   // +0x00
    void* m_pad04;  // +0x04
    i32 m_capacity; // +0x08  capacity in bits
    u32* m_words;   // +0x0c  the DWORD word band
};

#endif // GRUNTZ_WAP32_ENGSTR_H
