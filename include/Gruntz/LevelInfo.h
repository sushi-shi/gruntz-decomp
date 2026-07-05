// LevelInfo.h - the shared, single definition of CLevelInfo, the per-level
// descriptor. This is a REAL retail class name (recovered from two symbols):
//   ?BuildLevelTitleString@@YAXPAUHWND__@@HPAUCLevelInfo@@@Z   (0x0e44e0)
//   ?LoadConfig@CBattlezMapConfig@@QAEHPAUCLevelInfo@@HH@Z     (0x025020)
// formerly modeled as two divergent .cpp-local views (wave 3 fold):
//   * LevelInfoDlg.cpp  - the level-SELECT dialog reads m_levelNum/m_path/m_name/
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

// Config-phase engine handles (full defs live in BattlezMapConfig.cpp; only these
// slots of CLevelInfo point at them):
struct CLevelSpawnInfo; // +0x2c  (its +0x2e4 word seeds CBattlezMapConfig::m_14)
struct CLevelList;      // +0x30  the level object-list root (m_8 = the walked list)
struct CMapDims;        // +0x70  the map dimensions (its +0xc drives the /3, >>2 fields)

SIZE_UNKNOWN(CLevelInfo); // extends past +0xfc; total retail size not pinned
struct CLevelInfo {
    char m_pad00[0x4];
    i32 m_levelNum;               // +0x04  level number (1..)
    char m_pad08[0x2c - 0x8];     // +0x08
    CLevelSpawnInfo* m_spawnInfo; // +0x2c  spawn-info handle (LoadConfig)
    CLevelList* m_objList;        // +0x30  object-list root (LoadConfig marker walk)
    char m_pad34[0x35 - 0x34];    // +0x34
    char m_path[0x68 - 0x35];     // +0x35  level file path (Open); buffer ends at m_68
    void* m_68;                   // +0x68  (LoadConfig -> CBattlezMapConfig::m_8)
    char m_pad6c[0x70 - 0x6c];    // +0x6c
    CMapDims* m_dims;             // +0x70  map dims (LoadConfig -> m_dims)
    char m_pad74[0x75 - 0x74];    // +0x74
    char m_name[0xf8 - 0x75];     // +0x75  display name / custom level path
    i32 m_isCustom;               // +0xf8  flag: custom level
    i32 m_isBattlez;              // +0xfc  flag: battlez (vs questz)
};

#endif // SRC_GRUNTZ_LEVELINFO_H
