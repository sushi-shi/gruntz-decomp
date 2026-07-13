// LevelInfo.h - the shared, single definition of CLevelInfo, the per-level
// descriptor. This is a REAL retail class name (recovered from two symbols):
//   ?BuildLevelTitleString@@YAXPAUHWND__@@HPAUCLevelInfo@@@Z   (0x0e44e0)
//   ?LoadConfig@CBattlezMapConfig@@QAEHPAUCLevelInfo@@HH@Z     (0x025020)
// formerly modeled as two divergent .cpp-local views (wave 3 fold):
//   * SaveGame.cpp (level-preview half)  - the level-SELECT dialog reads m_levelNum/m_path/m_name/
//     the m_isCustom/m_isBattlez flags (BuildLevelTitleString formats the title,
//     opens the level file, extracts the preview image).
//   * BattlezMapConfig.cpp - CBattlezMapConfig::LoadConfig reads the three engine
//     handles m_spawnInfo(+0x2c)/m_objList(+0x30)/m_68(+0x68)/m_dims(+0x70).
// The two views read DISJOINT offsets (no naming conflict). The old dialog view
// modeled a single 0x35..0x74 char path buffer that ran OVER the +0x68/+0x70
// pointer members LoadConfig dereferences; here the path buffer ends at the m_68
// pointer (matching-neutral: BuildLevelTitleString only takes &m_path, never its
// length). Field names are placeholders where the role is unproven; only OFFSETS +
// emitted code bytes are load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_LEVELINFO_H
#define SRC_GRUNTZ_LEVELINFO_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/QueueDrainHost.h> // CQueueDrainHost - the walked object collection

class CMapMgr;               // <Gruntz/MapMgr.h>   (== CBrickzGrid) the tile/path grid
class CTriggerMgr;           // <Gruntz/TriggerMgr.h>
struct CGameObject;          // <Gruntz/UserLogic.h>
class CTileTriggerContainer; // <Gruntz/TileTriggerContainer.h>

// The spawn-info handle at +0x2c. Its +0x2e4 is the level's tile-trigger CONTAINER
// (settled 2026-07-13: ModeObjInit/Multi construct it as the 4-CPtrList + m_74
// container; FindChild @0x116ee0 / FindByField0C @0x1171d0 are container methods) -
// LoadConfig copies it into CBattlezMapConfig::m_14, and the run phase fires
// FindChild / FindByField0C on it (`mov ecx,[this+0x14]`).
// Was TWO .cpp-local views of one object (CLevelSpawnInfo + CArriveSub10b).
SIZE_UNKNOWN(CLevelSpawnInfo);
struct CLevelSpawnInfo {
    char m_pad00[0x2e4];
    CTileTriggerContainer* m_2e4; // +0x2e4  the level's tile-trigger container
};

// The level object-list ROOT (CLevelInfo::m_objList, +0x30). m_coll is the collection
// the marker loops / spawn scan walk; m_view holds the world->screen mapper. Was TWO
// .cpp-local views of one object (`CLevelList` outer + `Scene`); the object hung off
// m_view (+0x24) was a third (`SceneView24`).
SIZE_UNKNOWN(CLevelViewHolder);
struct CLevelViewHolder {
    char m_pad00[0x5c];
    struct CPlaneRender* m_5c; // +0x5c  the plane/view mapper (WorldToScreen)
};
SIZE_UNKNOWN(CLevelList);
struct CLevelList {
    char m_pad00[0x8];
    CQueueDrainHost* m_coll; // +0x08  the walked game-object collection
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
