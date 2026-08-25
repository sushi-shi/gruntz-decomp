#ifndef GRUNTZ_MAPLOGIC_H
#define GRUNTZ_MAPLOGIC_H

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

i32 SerializeScrollState(CFileMemBase* ar, SerialMode mode, LogicTypeId, i32);

#endif // GRUNTZ_MAPLOGIC_H
