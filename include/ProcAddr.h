#ifndef GRUNTZ_PROCADDR_H
#define GRUNTZ_PROCADDR_H

#include <Win32.h>

// ONE seam: GetProcAddress's FARPROC to the typed function pointer it really
// is. C++ has no conversion between two unrelated function-pointer types, so
// the era source had to write a cast here; this spells the same reinterpret at
// the ten TOOLHELP32 / SFManager sites without one. Do not add data-pointer
// members - a T*/FARPROC union is a different (and unproven) claim.
template<class F> union ProcAddr {
    FARPROC m_raw;
    F m_fn;
};

#endif // GRUNTZ_PROCADDR_H
