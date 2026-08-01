#ifndef GRUNTZ_PROCADDR_H
#define GRUNTZ_PROCADDR_H

#include <Win32.h>

template<class F> union ProcAddr {
    FARPROC m_raw;
    F m_fn;
};

#endif // GRUNTZ_PROCADDR_H
