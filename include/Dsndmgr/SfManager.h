#ifndef GRUNTZ_DSNDMGR_SFMANAGER_H
#define GRUNTZ_DSNDMGR_SFMANAGER_H

#include <SFMAN.H>

typedef LRESULT (*SfGetLoadedBankPathname2)(SFDEVINDEX, PSFMIDILOCATION);

typedef int(__cdecl* SfManagerFactory)(int flags, SFMANL101API** out);

#endif // GRUNTZ_DSNDMGR_SFMANAGER_H
