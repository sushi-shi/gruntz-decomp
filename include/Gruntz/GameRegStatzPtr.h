// GameRegStatzPtr.h - the *0x24556c registry singleton under its CStatzGameReg
// facet view. TU-PRIVATE by design (the GameRegMfcPtr.h pattern): extern "C"
// keeps one symbol while each TU picks ONE typed view; never include this and
// GameRegMfcPtr.h in the same TU.
#ifndef GRUNTZ_GAMEREGSTATZPTR_H
#define GRUNTZ_GAMEREGSTATZPTR_H

struct CStatzGameReg;
extern "C" CStatzGameReg* g_gameReg; // *0x24556c singleton (Statz facet view)

#endif // GRUNTZ_GAMEREGSTATZPTR_H
