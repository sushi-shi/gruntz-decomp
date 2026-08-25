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
