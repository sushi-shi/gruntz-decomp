// GruntDirStatics.h - the per-TU static direction-cell family. Retail emits
// these nine cells + their $E initializers into every TU that saw this header.
// The .CRT$XC census counts exactly 106 runs of exactly 9, and exactly 106 .cpp
// files include this header, so the two sides are closed against each other
// (docs/patterns/crt-xc-table-is-the-static-initializer-census.md). Two of the
// 106 are modelled as named globals rather than manifest rows: g_gruntMoveDir*
// in GruntSteps.cpp and g_gruntDir* in GruntCombat.cpp.
// Copies are dead data - only their dynamic initializers ever touch them.
// Pins: config/static_data_copies.tsv (DATA() in a header is ignored).
#ifndef SRC_GRUNTZ_GRUNTDIRSTATICS_H
#define SRC_GRUNTZ_GRUNTDIRSTATICS_H

#include <Gruntz/GruntDirection.h>

static GruntDirectionCell s_gruntDirEast(1, 2, DIR_EAST);
static GruntDirectionCell s_gruntDirNorth(0, 1, DIR_NORTH);
static GruntDirectionCell s_gruntDirSouth(2, 1, DIR_SOUTH);
static GruntDirectionCell s_gruntDirWest(1, 0, DIR_WEST);
static GruntDirectionCell s_gruntDirNorthEast(0, 2, DIR_NORTHEAST);
static GruntDirectionCell s_gruntDirNorthWest(0, 0, DIR_NORTHWEST);
static GruntDirectionCell s_gruntDirSouthEast(2, 2, DIR_SOUTHEAST);
static GruntDirectionCell s_gruntDirCenter(1, 1, DIR_CENTER);
static GruntDirectionCell s_gruntDirSouthWest(2, 0, DIR_SOUTHWEST);

#endif // SRC_GRUNTZ_GRUNTDIRSTATICS_H
