// TriggerMgrRecords.h - CTriggerMgr's OWN record types + TU externs (relocated from
// the *Views.h holding pen 2026-07-20: these are not views of unknown classes - the
// node structs model the real MFC CObList/CPtrList CNode with TYPED payload arms
// (the "generic collections" keep class), and CGridCell/CGridLookup are the documented
// keep-until-megafn grid pair. The ex-CTmGoal +0x23c goal view was RESOLVED: it is the
// CWwdGameObjectA "DoNothing" camera sprite (LoadCameraSprite creates+stores it).
#ifndef GRUNTZ_TRIGGERMGR_RECORDS_H
#define GRUNTZ_TRIGGERMGR_RECORDS_H

#include <Ints.h>
#include <Mfc.h>
#include <rva.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/TriggerMgr.h>  // CTriggerMgr (the records' owner class)
#include <Gruntz/TraitorMode.h> // g_traitorMode (DAT_006455b0, the alt-group gate)

enum {
    kPendingFxIdBase = 0xc8
};

class CGruntPuddle;

struct CTmNode {
    CTmNode* m_next; // +0x00
    i32 m_4;         // +0x04
    i32* m_payload;  // +0x08  -> { x@+0, y@+4 }
};

extern CButeMgr g_buteMgr;
extern "C" u32 g_frameTime; // DAT_00645588 (the level base score / id sentinel)

void Str_Free(void* node); // CString teardown, 0x1b9b93

struct CTmRecNode {
    CTmRecNode* m_next;  // +0x00
    char p0[0x4];        // +0x04
    CGruntPuddle* m_obj; // +0x08  placed object (the baseList puddle element)
};

struct CGridCell {
    i32 m_0;
    char _pad[0x1c - 4];
};
struct CGridLookup {
    char _00[8];
    CGridCell** m_8;          // +0x08  rows
    i32 m_c;                  // +0x0c  width
    i32 m_10;                 // +0x10  height
    i32 Lookup(i32 x, i32 y); // 0x75a40
};

SIZE_UNKNOWN(CTmNode);
SIZE_UNKNOWN(CTmRecNode);
SIZE_UNKNOWN(CGridCell);
SIZE_UNKNOWN(CGridLookup);

#endif // GRUNTZ_TRIGGERMGR_RECORDS_H
