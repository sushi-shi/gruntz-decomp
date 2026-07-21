#ifndef SRC_GRUNTZ_LEVELINFO_H
#define SRC_GRUNTZ_LEVELINFO_H

#include <Ints.h>
#include <rva.h>

class CDDrawChildGroup;      // <DDrawMgr/DDrawChildGroup.h> the walked object collection (m_coll)

class CMapMgr;               // <Gruntz/MapMgr.h>   (== CBrickzGrid) the tile/path grid
class CTriggerMgr;           // <Gruntz/TriggerMgr.h>
struct CGameObject;          // <Gruntz/UserLogic.h>
class CTileTriggerContainer; // <Gruntz/TileTriggerContainer.h>

SIZE_UNKNOWN(CLevelSpawnInfo);
struct CLevelSpawnInfo {
    char m_pad00[0x2e4];
    CTileTriggerContainer* m_2e4; // +0x2e4  the level's tile-trigger container
};

class CDDrawWorkerHost;

SIZE_UNKNOWN(CLevelViewHolder);
struct CLevelViewHolder {
    char m_pad00[0x5c];
    CDDrawWorkerHost* m_5c; // +0x5c  the plane/view mapper (WorldToScreen)
};
SIZE_UNKNOWN(CLevelList);
struct CLevelList {
    char m_pad00[0x8];
    CDDrawChildGroup* m_coll; // +0x08  the walked game-object collection (ex CQueueDrainHost view)
    char m_pad0c[0x24 - 0xc];
    CLevelViewHolder* m_view; // +0x24  (->m_5c = the world->screen mapper)
};

SIZE_UNKNOWN(CLevelInfo); // extends past +0xfc; total retail size not pinned
struct CLevelInfo {
    char m_pad00[0x4];
    i32 m_levelNum;           // +0x04  level number (1..)
    char m_pad08[0x10 - 0x8]; // +0x08
    // +0x10: the run-phase spawn machine reads this off the SAME object it stores at
    // CBattlezMapConfig+0x04 (`m_ctx`) - and that slot is exactly the `lvl` LoadConfig
    // was handed. So the old .cpp-local `GruntSpawnCtx` view IS this class; its members
    // fold in here (m_10 / m_objList / m_triggerMgr / m_dims). Retail 0x2d800 reads
    // `mov eax,[ctx+0x10]; mov ecx,[eax+0x5c]; mov eax,[eax+0x60]` - the CGameObject
    // pixel-position pair. (The view's "m_cellResolver" @+0x14 was a PHANTOM: retail
    // fires FindChild/FindByField0C on `mov ecx,[this+0x14]` - CBattlezMapConfig's OWN
    // m_cellQuery - not on ctx+0x14. Not modeled here; see BattlezMapConfig.h.)
    CGameObject* m_10;            // +0x10  the level's active object (pixel pos @+0x5c/+0x60)
    char m_pad14[0x2c - 0x14];    // +0x14
    CLevelSpawnInfo* m_spawnInfo; // +0x2c  spawn-info handle (LoadConfig)
    CLevelList* m_objList;        // +0x30  object-list root (LoadConfig marker walk)
    char m_pad34[0x35 - 0x34];    // +0x34
    char m_path[0x68 - 0x35];     // +0x35  level file path (Open); buffer ends at m_68
    CTriggerMgr* m_triggerMgr;    // +0x68  the level's trigger grid (-> CBattlezMapConfig::m_8)
    char m_pad6c[0x70 - 0x6c];    // +0x6c
    CMapMgr* m_dims;              // +0x70  the tile/path grid (-> CBattlezMapConfig::m_board)
    char m_pad74[0x75 - 0x74];    // +0x74
    char m_name[0xf8 - 0x75];     // +0x75  display name / custom level path
    i32 m_isCustom;               // +0xf8  flag: custom level
    i32 m_isBattlez;              // +0xfc  flag: battlez (vs questz)
};

#endif // SRC_GRUNTZ_LEVELINFO_H
