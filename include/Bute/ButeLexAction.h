#ifndef GRUNTZ_BUTE_BUTELEXACTION_H
#define GRUNTZ_BUTE_BUTELEXACTION_H

#include <Enums.h>

// What CButeMgr::Parse does with the current character, read out of the lexer's
// transition table. Each value is named by exactly the three things its own arm
// decides: whether the character joins the token, whether it is consumed, and
// whether the token ends here.
//
//                 char kept?   consumed?   token ends?
//   ERROR             -            -            -        reports and gives up
//   TAKE             yes          yes           no
//   SKIP              no          yes           no
//   ACCEPT_TAKE      yes          yes          yes
//   ACCEPT_SKIP       no          yes          yes
//   ACCEPT_PUSHBACK   no           NO          yes       left for the next token
//
// ACCEPT_PUSHBACK is the one worth spelling out: its arm is the only one that
// never calls NextChar, so the character that ended this token is still current
// when the next one starts scanning.
GZ_ENUM_BEGIN(ButeLexAction)
    LEXACT_ERROR = 0,
    LEXACT_TAKE = 1,
    LEXACT_SKIP = 2,
    LEXACT_ACCEPT_TAKE = 3,
    LEXACT_ACCEPT_SKIP = 4,
    LEXACT_ACCEPT_PUSHBACK = 5
GZ_ENUM_END(ButeLexAction)

// The third axis of g_transTable[state][charclass][3]. Not a value domain - it
// is which of the three parallel tables a lookup reads, so it is a constant bag
// indexing a real array.
GZ_ENUM_CONST_BEGIN(ButeLexSlot)
    LEXSLOT_ACTION = 0,
    LEXSLOT_TARGET = 1,
    LEXSLOT_STATE = 2
GZ_ENUM_CONST_END(ButeLexSlot)

#endif // GRUNTZ_BUTE_BUTELEXACTION_H
