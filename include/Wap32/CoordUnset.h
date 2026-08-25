#ifndef WAP32_COORDUNSET_H
#define WAP32_COORDUNSET_H

#include <Enums.h>
#include <Wap32/CoordUnset.h>

GZ_ENUM_CONST_BEGIN(CoordSentinel)
    COORD_UNSET = 0x80000000,
    UNINIT_FILL = 0xeeeeeeee
GZ_ENUM_CONST_END(CoordSentinel)

#endif // WAP32_COORDUNSET_H
