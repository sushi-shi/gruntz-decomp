// QueueDrainHost.h - the level's GAME-OBJECT COLLECTION and its list cell.
//
// ONE class, formerly FIVE .cpp-local views: `CQueueDrainHost`+`CQueueProbeNode`
// (QueueDrainHost.cpp), `SceneColl`+`SceneNode` and `CLevelList`(inner)+`CLevelNode`
// (BattlezMapConfig.cpp). They are the same object reached by the same expression:
//
//   CLevelInfo::m_objList (+0x30) -> m_coll (+0x08)   [the walked collection]
//
// - the config-phase marker loops walk it through the GetFirst/GetNext cursor (+0x64):
//   `n = coll->m_head; coll->m_cursor = n->m_next; return n->m_data;`
// - the run-phase spawn scan drains it through the SECOND cursor (+0x68), which is
//   what Drain_031250 (RVA 0x31250) pops, re-seeding m_scan = m_head to restart.
// Both cursors walk the SAME node list (head @+0x14) of the SAME payload type - a
// CGameObject (its slot-8 GetTypeId is the "+0x20 probe" the drain filters on).
//
// @identity-TODO: the RTTI name is not recovered (no vtable, no ctor site found for
// it); `CQueueDrainHost` is retained because 0x31250's symbol is already bound to it
// tree-wide. Everything ELSE about it is binary-evidenced (offsets above).
#ifndef GRUNTZ_GRUNTZ_QUEUEDRAINHOST_H
#define GRUNTZ_GRUNTZ_QUEUEDRAINHOST_H

#include <Ints.h>
#include <rva.h>

struct CGameObject; // <Gruntz/UserLogic.h> - the collection's payload

// One list cell (the MFC CObList node shape: next@+0x00, prev@+0x04 unused by these
// walkers, data@+0x08).
SIZE_UNKNOWN(CQueueProbeNode);
struct CQueueProbeNode {
    CQueueProbeNode* m_next; // +0x00
    char m_pad04[0x04];      // +0x04  (prev; unused here)
    CGameObject* m_data;     // +0x08  payload
};

SIZE_UNKNOWN(CQueueDrainHost);
class CQueueDrainHost {
public:
    // 0x31250: pop nodes off m_scan until one whose payload's GetTypeId() (vtable slot
    // 8, +0x20) is 5; 0 when the scan is exhausted.
    CGameObject* Drain_031250();

    char m_pad00[0x14];
    CQueueProbeNode* m_head; // +0x14  list head (first cell)
    char m_pad18[0x64 - 0x18];
    CQueueProbeNode* m_cursor; // +0x64  GetFirst/GetNext walk cursor
    CQueueProbeNode* m_scan;   // +0x68  Drain_031250 pop cursor (re-seeded from m_head)
};

#endif // GRUNTZ_GRUNTZ_QUEUEDRAINHOST_H
