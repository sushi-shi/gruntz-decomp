#ifndef NET_DPLAYSESSIONFLAGS_H
#define NET_DPLAYSESSIONFLAGS_H

#include <Win32.h>

#include <dplay.h>

// DPSESSION_* flags retail uses that the DPLAY.H shipped with MSVC 5.0 does not
// define. Guarded, so a newer SDK on the include path wins.
//
// CNetMgr::EnumGroupsInto sets its session's dwFlags to 0xa044, which decomposes
// into exactly these four with nothing left over - and each is a decision a game
// session would make: migrate the host if it drops, keep the connection alive,
// use the DirectPlay protocol, optimise for latency.
//
// This is the same gap that left eight DPERR_* names spelled as HRESULTs in
// NetMgrReportError: the game was built against a later DirectPlay SDK than its
// own compiler bundled.
#ifndef DPSESSION_MIGRATEHOST
#define DPSESSION_MIGRATEHOST 0x00000004
#endif
#ifndef DPSESSION_KEEPALIVE
#define DPSESSION_KEEPALIVE 0x00000040
#endif
#ifndef DPSESSION_OPTIMIZELATENCY
#define DPSESSION_OPTIMIZELATENCY 0x00002000
#endif
#ifndef DPSESSION_DIRECTPLAYPROTOCOL
#define DPSESSION_DIRECTPLAYPROTOCOL 0x00008000
#endif

#endif // NET_DPLAYSESSIONFLAGS_H
