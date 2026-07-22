#ifndef GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#define GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#include <rva.h>

struct SecretMsgRow {
    char strA[0x20]; // +0x00  encoded line A
    char strB[0x80]; // +0x20  encoded line B
};
SIZE_UNKNOWN();

extern RECT g_levelMsgRectsA[8]; // 0x60b838  (shared with BootyMessages - stays extern)

#endif // GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
