#ifndef GRUNTZ_USERBASELINK_H
#define GRUNTZ_USERBASELINK_H

#include <Wap32/zBitVec.h>
#include <rva.h>

#include <EmptyString.h>

struct CUserBaseLink {
    CUserBaseLink();
    ~CUserBaseLink() {}
    zBitVec m_str;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_USERBASELINK_H
