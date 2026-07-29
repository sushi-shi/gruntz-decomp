// PlayPlaneScan.h - the PlayPlaneScan TU's external declarations.
#ifndef GRUNTZ_PLAYPLANESCAN_H
#define GRUNTZ_PLAYPLANESCAN_H

#include <Mfc.h> // afx.h FIRST (umbrella for any Win32 types below)
#include <Ints.h>
#include <rva.h>

// The plane-type markers this TU compares m_notify against are the REAL
// GameObjNotifyFn registrants (<Gruntz/GameObjectFactory.h>); the old
// PlaneType_*/PlaneQuad* `void()` aliases were views of their ILT thunks:
//   0x40137a CreateGiantRock   0x403d0f CreateCoveredPowerup
//   0x4017e4 CreateGruntCreationPoint  0x40192e CreateExitTrigger
//   0x403148 CreateFortressFlag        0x401087 CreateWayPoint
//   0x40164f CreateGuardPoint          0x4019bf CreateBrickz

#endif // GRUNTZ_PLAYPLANESCAN_H
