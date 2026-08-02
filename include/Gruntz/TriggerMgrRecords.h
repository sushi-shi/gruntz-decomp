#ifndef GRUNTZ_TRIGGERMGR_RECORDS_H
#define GRUNTZ_TRIGGERMGR_RECORDS_H

#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/TraitorMode.h>
#include <Gruntz/TriggerMgr.h>
#include <Ints.h>

enum {
    kPendingFxIdBase = 0xc8
};

class CGruntPuddle;

extern CButeMgr g_buteMgr;
extern "C" u32 g_frameTime;

void Str_Free(void* node);

struct CGridCell {
    i32 m_flags;
    char _pad[0x1c - 4];
};
SIZE_UNKNOWN();
struct CGridLookup {
    char _00[8];
    CGridCell** m_rows;
    i32 m_width;
    i32 m_height;
    i32 Lookup(i32 x, i32 y);
};
SIZE_UNKNOWN();

#endif // GRUNTZ_TRIGGERMGR_RECORDS_H
