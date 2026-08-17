#ifndef GRUNTZ_GRUNTZ_GRUNTCOMBAT_H
#define GRUNTZ_GRUNTZ_GRUNTCOMBAT_H

#include <Gruntz/ActReg.h>
#include <Ints.h>

extern const u8 g_hitTable[23][23];
extern const float g_quarterScale;
extern const float g_slopeTwo;
extern const float g_slopeNegTwo;
extern const double g_combatSlopeHalf;
extern const double g_combatSlopeNegHalf;

static inline void GruntScratchTeardown();

#endif // GRUNTZ_GRUNTZ_GRUNTCOMBAT_H
