#ifndef GRUNTZ_GRUNTZ_TARGETSELECTIONKIND_H
#define GRUNTZ_GRUNTZ_TARGETSELECTIONKIND_H

#include <Enums.h>

// What CTriggerMgr::HandleTargetSelection targets. The three explicit arms either record
// a map point, record a Grunt, or enqueue a toy command against the point/Grunt.
GZ_ENUM_BEGIN(TargetSelectionKind)
    TARGET_SELECTION_AUTO = 0,
    TARGET_SELECTION_POINT = 1,
    TARGET_SELECTION_GRUNT = 2,
    TARGET_SELECTION_TOY = 3
GZ_ENUM_END(TargetSelectionKind)

#endif // GRUNTZ_GRUNTZ_TARGETSELECTIONKIND_H
