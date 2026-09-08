#ifndef GRUNTZ_INPUTDEVICEGROUP_H
#define GRUNTZ_INPUTDEVICEGROUP_H

#include <Utils/FixedPtrArray.h>

class CInputDevBase;
typedef CFixedPtrArray<CInputDevBase, 32> CInputDeviceGroup;
extern CInputDeviceGroup* g_actorList;

#endif // GRUNTZ_INPUTDEVICEGROUP_H
