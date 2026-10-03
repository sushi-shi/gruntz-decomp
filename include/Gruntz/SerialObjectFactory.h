#ifndef GRUNTZ_SERIALOBJECTFACTORY_H
#define GRUNTZ_SERIALOBJECTFACTORY_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

class CDDrawSurfaceMgr;
class CFileMemBase;

i32 __cdecl GameSerializationCallback(
    CDDrawSurfaceMgr* ctx,
    CFileMemBase* archive,
    SerialMode mode,
    LogicTypeId typeId,
    void* payload
);

#endif
