#ifndef GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#define GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#include <rva.h>

struct SecretMsgRow {
    char strA[0x20]; // +0x00  encoded line A
    char strB[0x80]; // +0x20  encoded line B
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
