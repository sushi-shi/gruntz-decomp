// GruntDirStatics.h - the per-TU static direction-cell family. Retail emits
// these nine cells + their $E initializers into every TU that saw this header
// (66 copies; the extern g_gruntDir* set in GruntCombat.cpp is separate).
// Copies are dead data - only their dynamic initializers ever touch them.
// Pins: config/static_data_copies.tsv (DATA() in a header is ignored).
#ifndef SRC_GRUNTZ_GRUNTDIRSTATICS_H
#define SRC_GRUNTZ_GRUNTDIRSTATICS_H

#include <Gruntz/Grunt.h>

static GruntDirectionCell s_gruntDirEast(1, 2, 3);
static GruntDirectionCell s_gruntDirNorth(0, 1, 1);
static GruntDirectionCell s_gruntDirSouth(2, 1, 5);
static GruntDirectionCell s_gruntDirWest(1, 0, 7);
static GruntDirectionCell s_gruntDirNorthEast(0, 2, 2);
static GruntDirectionCell s_gruntDirNorthWest(0, 0, 8);
static GruntDirectionCell s_gruntDirSouthEast(2, 2, 4);
static GruntDirectionCell s_gruntDirCenter(1, 1, 0);
static GruntDirectionCell s_gruntDirSouthWest(2, 0, 6);

#endif // SRC_GRUNTZ_GRUNTDIRSTATICS_H
