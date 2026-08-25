#ifndef GRUNTZ_WWD_WWDOBJECTTYPE_H
#define GRUNTZ_WWD_WWDOBJECTTYPE_H

#include <Enums.h>

// Proven bits in CGameObject::m_objectType. The field is a collision
// category mask loaded from WWD data, so it remains an integer at the ABI
// boundary and only evidence-backed bits are named here.
GZ_ENUM_CONST_BEGIN(WwdObjectType)
    WWD_OBJECT_TYPE_PLATFORM = 0x80,
    // 0x47e8c `mov DWORD PTR [eax+0xe8],0x1000` in ??0CGrunt@@QAE@PAX@Z, the
    // only site that writes the field. It was 0x100000 here - an extra zero.
    WWD_OBJECT_TYPE_GRUNT = 0x1000
GZ_ENUM_CONST_END(WwdObjectType)

#endif // GRUNTZ_WWD_WWDOBJECTTYPE_H
