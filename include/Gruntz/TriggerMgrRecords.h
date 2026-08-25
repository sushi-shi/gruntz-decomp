#ifndef GRUNTZ_TRIGGERMGR_RECORDS_H
#define GRUNTZ_TRIGGERMGR_RECORDS_H

#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/TraitorMode.h>
#include <Gruntz/TriggerMgr.h>
#include <Ints.h>

enum {
    kPendingFxIdBase = 0xc8
};

class CGruntPuddle;
class CGrunt;
struct Coord;

RVA(0x00075a90, 0x27)
inline i32 TmFlagsAllow(i32 a, i32 b, i32 c) {
    i32 m = b & a;
    if (m & BRICKZ_CELL_OCCUPIED) {
        return 0;
    }
    if (m && !(c & a)) {
        return 0;
    }
    return 1;
}

GruntDirectionCell __stdcall TmDeflectStep(
    CGrunt* g,
    i32 goalX,
    i32 goalY,
    i32 unusedX,
    i32 unusedY,
    GruntDirection dir,
    Coord* pCell,
    i32* pFlags
);

#endif // GRUNTZ_TRIGGERMGR_RECORDS_H
