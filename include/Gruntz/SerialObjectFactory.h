#ifndef GRUNTZ_SERIALOBJECTFACTORY_H
#define GRUNTZ_SERIALOBJECTFACTORY_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

i32 __cdecl
SerialObjectFactory(void* ctx, void* ar, SerialMode mode, LogicTypeId typeId, void* payload);

#endif // GRUNTZ_SERIALOBJECTFACTORY_H
