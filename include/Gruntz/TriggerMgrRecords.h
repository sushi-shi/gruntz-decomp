// TriggerMgrRecords.h - CTriggerMgr's OWN record types + TU externs (relocated from
// the *Views.h holding pen 2026-07-20: these are not views of unknown classes - the
// node structs model the real MFC CObList/CPtrList CNode with TYPED payload arms
// (the "generic collections" keep class), CTmGoal is the @identity-TODO +0x23c goal
// object, and CGridCell/CGridLookup are the documented keep-until-megafn grid pair).
#ifndef GRUNTZ_TRIGGERMGR_RECORDS_H
#define GRUNTZ_TRIGGERMGR_RECORDS_H

#include <Ints.h>
#include <Mfc.h>
#include <rva.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/TriggerMgr.h>  // CTriggerMgr (the records' owner class)
#include <Gruntz/TraitorMode.h> // g_traitorMode (DAT_006455b0, the alt-group gate)

// The pending-fx sprite-id base: a cell's logic kind maps to its pending overlay-fx
// sprite id as (kind + kPendingFxIdBase), latched into m_pendingFxKind.
enum {
    kPendingFxIdBase = 0xc8
};

class CGruntPuddle;

// A list node: { CTmNode* m_next; ; (x,y)* m_payload }. The payload is an (x,y)
// pair at +0/+4. Opaque otherwise. These are the record-list / selection-list nodes.
struct CTmNode {
    CTmNode* m_next; // +0x00
    i32 m_4;         // +0x04
    i32* m_payload;  // +0x08  -> { x@+0, y@+4 }
};

// A grid cell's config/type sub-object (cell->m_14): its +0x1c is the config-name id the
// name registry maps to a string. And the goal object (cell->m_154 / the manager's goal),
// whose +0x8 flags word gets the 0x10000 done-bit; full CTmGoal is defined below.
struct CTmGoal;

// The goal object at CTriggerMgr+0x23c; ResetAll ORs 0x10000 into its +0x8 flags.

extern CButeMgr g_buteMgr;
extern "C" u32 g_frameTime; // DAT_00645588 (the level base score / id sentinel)
// (g_644ca4 moved to its only user, TriggerMgr.cpp, where it can carry the DATA()
//  binding a header cannot: a DATA() in a header is ignored by the label pass, so
//  declaring it here bound it to NO retail address at all.)

// (CTmNameReg + its g_nameReg alias-extern are GONE - "the DAT_006bf650 registry"
// IS g_typeColl, the shared CActReg (TypeKeyColl.cpp: "0x6bf650 is a CActReg like
// every other one"); Lookup@0x46e0c0 is a standalone slot-returning ResolveEntry
// copy, now declared on CActReg.)
void Str_Free(void* node);   // CString teardown, 0x1b9b93

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
SIZE_UNKNOWN(CTmGoal);
SIZE_UNKNOWN(CTmRecNode);
SIZE_UNKNOWN(CGridCell);
SIZE_UNKNOWN(CGridLookup);

#endif // GRUNTZ_TRIGGERMGR_RECORDS_H
