#ifndef GRUNTZ_BUTE_BUTELEXACTION_H
#define GRUNTZ_BUTE_BUTELEXACTION_H

#include <Enums.h>

GZ_ENUM_BEGIN(ButeLexAction)
    LEXACT_ERROR = 0,
    LEXACT_TAKE = 1,
    LEXACT_SKIP = 2,
    LEXACT_ACCEPT_TAKE = 3,
    LEXACT_ACCEPT_SKIP = 4,
    LEXACT_ACCEPT_PUSHBACK = 5
GZ_ENUM_END(ButeLexAction)

struct TranType {
    i16 ActionType;
    i16 A;
    i16 B;
};

#endif // GRUNTZ_BUTE_BUTELEXACTION_H
