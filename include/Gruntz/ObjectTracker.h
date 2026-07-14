// ObjectTracker.h - CObjectTracker, a per-tick game-object logic leaf that TRACKS a peer
// game-object: it mirrors its own screen position, relays the tracked peer's (area,sub)
// trigger-grid id + screen position to the registry's command grid, and (throttled)
// re-places at the peer's tile and fires an on-screen cue.
//
// IDENTITY: the retail RTTI class name is not recovered (Ghidra placeholder was
// "Obj0f7d90"); "CObjectTracker" and the semantic member names below are a BEHAVIORAL
// reconstruction from the single matched method (Update @0x0f7d90). Offsets (in the //
// comments) remain the load-bearing fact.
#ifndef GRUNTZ_OBJECTTRACKER_H
#define GRUNTZ_OBJECTTRACKER_H
#include <rva.h>

struct CGameObject; // <Gruntz/UserLogic.h> - the bound screen object (m_screenX/m_screenY)
class CObjectTracker;

// The peer getter reached through m_peerSource: the thunk 0x253b resolves to 0x477df0
// == ?FindNearestEnemy@CTriggerMgr@@ (the ex-CGruntTileMgr view is dissolved, 2026-07-14:
// CGrunt::m_tileMgr +0x260 IS the CTriggerMgr). So m_peerSource IS a CTriggerMgr* and
// GetPeer IS FindNearestEnemy(CGrunt*)->CGrunt*.
//
// @identity-TODO (stronger now): this tracker view is almost certainly ::CGrunt itself -
// SEVEN of its fields land on CGrunt at the identical offset (m_display==m_10 CGruntHud,
// m_posX/m_posY==m_lastTilePxX/Y @0x17c/0x180, m_areaId/m_subId==m_tileOwnerHi/Lo
// @0x1ec/0x1f0, m_active==m_entranceCommitted @0x1fc, m_peerSource==m_tileMgr @0x260),
// which also explains the CGrunt-typed FindNearestEnemy binding cast-free. Blocked only
// on attributing Update @0x0f7d90 (whose vtable slot / owner is unverified); once that is
// chased, fold the whole view onto CGrunt instead of introducing casts here.
struct CPeerSource {
    CObjectTracker* GetPeer(CObjectTracker* self); // 0x253b thunk == CGruntTileMgr::GetOccupant
};
SIZE_UNKNOWN(CPeerSource);

SIZE_UNKNOWN(CObjectTracker);
class CObjectTracker {
public:
    char pad00[0x10];
    CGameObject* m_display; // +0x10  this tracker's own screen-position holder (bound object)
    char pad14[0x17c - 0x14];
    int m_posX; // +0x17c  tracker screen x (published to m_lastX)
    int m_posY; // +0x180  tracker screen y (published to m_lastY)
    char pad184[0x198 - 0x184];
    void* m_target; // +0x198  tracked-target gate (null -> idle)
    char pad19c[0x1ec - 0x19c];
    int m_areaId; // +0x1ec  trigger-grid area id (ReportObjectAt arg 0)
    int m_subId;  // +0x1f0  trigger-grid sub id  (ReportObjectAt arg 1)
    char pad1f4[0x1fc - 0x1f4];
    int m_active; // +0x1fc  active gate (peer must have this set)
    char pad200[0x248 - 0x200];
    int m_placeKind; // +0x248  kind arg handed to PlaceAtTile
    char pad24c[0x260 - 0x24c];
    CPeerSource* m_peerSource; // +0x260  peer-object getter holder (a CGruntTileMgr*)
    char pad264[0x2d0 - 0x264];
    int m_idleState; // +0x2d0  idle-reset state (= 5 when target null)
    int m_idleFlag;  // +0x2d4  idle-reset flag  (= 0 when target null)
    char pad2d8[0x2ec - 0x2d8];
    unsigned m_reportTimer; // +0x2ec  re-place/report throttle (> 1000 to fire)
    char pad2f0[0x300 - 0x2f0];
    int m_lastX; // +0x300  published copy of m_posX
    int m_lastY; // +0x304  published copy of m_posY
    char pad308[0x390 - 0x308];
    int m_cueArmed; // +0x390  one-shot: fire the on-screen cue this tick

    int CheckScreenPos(int x, int y);      // 0x1e97 thunk (peer screen-pos probe)
    int CheckOwnerCell(int area, int sub); // 0x1014 thunk (peer owner-cell probe)
    void PlaceAtTile(int tileX, int tileY, int a, int kind, int b, int c); // 0x1640 thunk
    int Update();                          // 0x0f7d90  the per-tick step
};

#endif // GRUNTZ_OBJECTTRACKER_H
