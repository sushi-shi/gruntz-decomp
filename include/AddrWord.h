#ifndef GRUNTZ_ADDRWORD_H
#define GRUNTZ_ADDRWORD_H

#include <Ints.h>

// ONE seam: a 32-bit integer and a pointer in the same ABI slot. It is DEBT,
// not a keep - a use is a reinterpret_cast that the cast ledger cannot see, so
// justify it the way a cast would be. Forced: an integer used as a
// CMapPtrToPtr / zPTree key (MFC declares the key `void*`), and a pointer's
// numeric value at a formatting or discriminated-slot seam retail really
// passes as an int. NOT forced: an `i32` local, member or parameter that only
// ever holds an address - that is a wrong declaration, and fixing it deletes
// the pun. Members exist only where a site proves one.
template<class T> union AddrWord {
    T* m_addr;
    i32 m_word;
    u32 m_uword;
};

#endif // GRUNTZ_ADDRWORD_H
