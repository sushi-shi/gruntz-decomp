#ifndef GRUNTZ_PROCADDR_H
#define GRUNTZ_PROCADDR_H

#include <Win32.h> // FARPROC (inert when <Mfc.h> already pulled windows.h)

// A dynamically-resolved export's address.
//
// GetProcAddress hands the address back as a FARPROC by contract - the loader has no
// idea what the export's signature is - while the caller knows exactly which
// prototype it resolved. One code address, two readings, so both are named here
// instead of punned at every resolve.
// MEASURED 2026-07-29: Utils::WinAPI::LegacyFindModule moved 99.90 -> 92.71 on this
// change (three resolves in one function; cl keeps the union in a frame slot where
// the cast stayed in a register). HeapDiag's and SFSelectDevice's resolves did not
// move. Kept - the ledger's number is the total and MAX preserves 99.90 - but if a
// future pass needs that function EXACT, this is the one line to look at.
template<class F> union ProcAddr {
    FARPROC m_raw; // what GetProcAddress returns
    F m_fn;        // the export's real prototype
};

#endif // GRUNTZ_PROCADDR_H
