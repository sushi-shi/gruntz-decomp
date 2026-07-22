#ifndef GRUNTZ_USERBASELINK_H
#define GRUNTZ_USERBASELINK_H

#include <Wap32/zBitVec.h>
#include <rva.h>

#include <EmptyString.h> // g_emptyString (the shared "" constant)

struct CUserBaseLink {
    CUserBaseLink();    // 0x16d710 (out-of-line; can throw)
    ~CUserBaseLink() {} // inline: folds to the embedded ~zBitVec call in leaf dtors
    zBitVec m_str;      // +0x00  (its name field; the 0x5f04c8 zBitVec vptr)
};
SIZE_UNKNOWN();

#endif // GRUNTZ_USERBASELINK_H
