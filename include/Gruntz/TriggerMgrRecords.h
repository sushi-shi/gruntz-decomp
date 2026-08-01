#ifndef GRUNTZ_TRIGGERMGR_RECORDS_H
#define GRUNTZ_TRIGGERMGR_RECORDS_H

#include <Ints.h>
#include <Mfc.h>
#include <rva.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TraitorMode.h>

enum {
    kPendingFxIdBase = 0xc8
};

class CGruntPuddle;

extern CButeMgr g_buteMgr;
extern "C" u32 g_frameTime;

void Str_Free(void* node);

struct CGridCell {
    i32 m_0;
    char _pad[0x1c - 4];
};
SIZE_UNKNOWN();
struct CGridLookup {
    char _00[8];
    CGridCell** m_8;
    i32 m_c;
    i32 m_10;
    i32 Lookup(i32 x, i32 y);
};
SIZE_UNKNOWN();

#endif // GRUNTZ_TRIGGERMGR_RECORDS_H
