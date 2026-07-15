// TraitorMode.h - the global traitor/betrayal-mode flag.
//
// g_traitorMode (?g_traitorMode@@3HA @0x6455b0) gates the traitor-mode combat rules;
// DEFINED in src/Gruntz/Grunt.cpp. C++-linkage int, DATA-reloc-masked. Declared once here
// (a minimal owner header) so the GruntzMgr/combat consumers stop re-`extern`-ing it and
// need not pull the heavier trigger-views header (whose CTmNode collides with the local
// CTmNode in GruntzMgrCmd.cpp).
#ifndef INCLUDE_GRUNTZ_TRAITORMODE_H
#define INCLUDE_GRUNTZ_TRAITORMODE_H
#include <Ints.h>

extern i32 g_traitorMode; // 0x6455b0 (defined in Grunt.cpp)

#endif // INCLUDE_GRUNTZ_TRAITORMODE_H
