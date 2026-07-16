// BootyMessages.h - the secret-bonus message records (owner: BootyMessages.cpp,
// the "+0x3d"-encoded banner/message tables decoded in place by the SetAt cipher).
#ifndef GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#define GRUNTZ_GRUNTZ_BOOTYMESSAGES_H

// One per-level secret-message row (0xa0 bytes): two encoded strings, indexed
// (rowBase*3 + row).
struct SecretMsgRow {
    char strA[0x20]; // +0x00  encoded line A
    char strB[0x80]; // +0x20  encoded line B
};

#endif // GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
