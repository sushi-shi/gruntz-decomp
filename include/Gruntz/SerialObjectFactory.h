#ifndef GRUNTZ_SERIALOBJECTFACTORY_H
#define GRUNTZ_SERIALOBJECTFACTORY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

class CGameWorld;
class CFileMemBase;

i32 __cdecl GameSerializationCallback(
    CGameWorld* ctx,
    CFileMemBase* archive,
    SerialMode mode,
    LogicTypeId typeId,
    void* payload
);

#endif // GRUNTZ_SERIALOBJECTFACTORY_H
