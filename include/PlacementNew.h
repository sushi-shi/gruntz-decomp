#ifndef GRUNTZ_PLACEMENTNEW_H
#define GRUNTZ_PLACEMENTNEW_H

#include <Ints.h>

// MSVC 5.0 ships no <new>, so the placement form has to be declared by hand. It was
// transcribed once per TU (12 copies); this is the one definition they share.
inline void* operator new(u32, void* p) {
    return p;
}

#endif // GRUNTZ_PLACEMENTNEW_H
