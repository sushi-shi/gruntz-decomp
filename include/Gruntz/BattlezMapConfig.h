// BattlezMapConfig.h - the shared, single definition of CBattlezMapConfig, the
// Battlez (multiplayer) per-team spawn/board manager. This ONE class was formerly
// modeled twice, under two hedged/placeholder names, because two TUs reconstruct
// two lifecycle PHASES of the same 0x1e8-byte object:
//
//   * BattlezMapConfig.cpp  - the LOAD phase: CBattlezMapConfig::LoadConfig reads
//     the [Battlez] bute-config group + the level start markers into the object.
//   * the BattlezMapConfig RUN-phase unit - the RUN phase: the ~40 spawn/board state-machine
//     methods that drive the loaded object each tick.
//
// DISAMBIGUATION EVIDENCE (this/ecx Frida trace, gruntz.analysis.this_cluster):
// the real recovered symbol ?LoadConfig@CBattlezMapConfig@@QAEHPAUCLevelInfo@@HH@Z
// (RVA 0x25020) is called on the SAME objects (ecx 0x1891508/740/978/bb0, spaced
// exactly 0x238 apart - the m_ctx per-band record stride) as every one of the
// ~40 run-phase methods; each object is class-pure. Hence it is ONE class, not
// the "CBattlezSpawnMgr_or_CGruntSpawnMgr" hedge (which was never a real symbol -
// no CBattlezSpawnMgr/CGruntSpawnMgr symbol exists; those substrings only ever
// appeared inside the hedge name itself). The exact size 0x1e8 is pinned by
// GruntzPlayer embedding one at +0x38 (next member at +0x220).
//
// The two phases name the SAME offsets differently (load: bute-item percents +
// level handles; run: spawn timers + band-threshold tables), and interpret the
// three head pointers with phase-appropriate types. Rather than force one naming
// (which would need casts or awkward array-index writes and lose one phase's
// meaning), the class models the two field VIEWS as anonymous unions: both TUs
// keep their own readable names at the same offsets, so this is a pure rename +
// header unification - matching-neutral (identical field offsets, no body change).
//
// The four growable MFC arrays are shared between phases (their types are FIXED by
// the ctor's member-construct vtable stamps):
//   +0xdc / +0xf0   vtable s_CPtrArray  @0x5ec2dc  -> CPtrArray
//   +0x104 / +0x118 vtable s_CDWordArray@0x5ec29c  -> CDWordArray
// Both are the real MFC afxcoll classes (0x14 B: vptr@0, m_pData@+4, m_nSize@+8,
// m_nMaxSize@+0xc, m_nGrowBy@+0x10). LoadConfig's per-marker SetAtGrow appends to
// the +0xdc/+0xf0 CPtrArray inner count word (m_candArray.m_nSize / m_0f0.m_nSize).
//
// Field names are placeholders (m_<hexoffset> where the role is unproven); only
// OFFSETS + emitted code bytes are load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_BATTLEZMAPCONFIG_H
#define SRC_GRUNTZ_BATTLEZMAPCONFIG_H
#include <rva.h>

#include <Mfc.h> // CPtrArray, CDWordArray (real afxcoll, 0x14 layout); DWORD

// ---- Run-phase engine object pointers (spawn view) --------------------------
// The three TU-shared pointer members (m_ctx/m_triggerMgr/m_board) point at real
// engine objects, modeled as the file-local views defined in the BattlezMapConfig RUN-phase unit:
//   m_ctx        = the level/game spawn context (CTriggerMgr@+0x68 + per-band
//                  records, 0x238 stride);
//   m_triggerMgr = the level's CTriggerMgr (the 4x15 cell grid);
//   m_board      = the CBrickz pathfinding-grid container.
class CTriggerMgr;
struct GruntSpawnCtx;
struct Board;
// ---- Load-phase engine object pointers (config view) ------------------------
// The same three slots hold level-info-derived handles during LoadConfig (the
// object is filled from a CLevelInfo before the run phase reinterprets the slots).
struct CLevelInfo;
struct CMapDims;
struct CLevelSpawnInfo;

SIZE(CBattlezMapConfig, 0x1e8);
class CBattlezMapConfig {
public:
    // ---- load phase (BattlezMapConfig.cpp) --------------------------------
    i32 LoadConfig(CLevelInfo* lvl, i32 id, i32 diff); // 0x025020

    // ---- run phase (the BattlezMapConfig RUN-phase unit) -------------------------------
    CBattlezMapConfig();
    ~CBattlezMapConfig();
    void FreeArrays();
    i32 Method_025c20();
    void Clear_02ade0();
    i32 Method_02c0a0(i32, i32);
    i32 Method_030530(i32);
    i32 Method_0305b0(i32, i32, i32);
    i32 Method_02bfc0(i32, void*, i32, i32);
    i32 Method_02ed90(i32);
    i32 Serialize_02b420(void*);
    i32 Deserialize_02b950(void*);
    i32 Method_030730(i32, i32, i32, i32);
    i32 Method_030990(i32, i32);
    i32 Method_0350d0(i32);
    void* Method_030f20(void*, i32, i32);
    i32 Method_0300c0(i32, i32, i32, i32, i32, i32);
    i32 Method_0302c0(i32, i32, i32, i32, i32);
    i32 Method_026470(i32);
    i32 Method_034460(i32);
    i32 Method_025d90();
    i32 Method_02f620(i32);
    i32 Method_029b40(i32);
    i32 Method_02d800(i32, i32, i32, i32);
    i32 Method_02edb0(i32, i32, i32, i32);
    i32 Method_030b20(i32, i32, i32);
    void* Method_02ad40(i32); // 0x02ad40  pick a random idle (m_busy==0) unit from a band row
    i32 Method_02c080(i32);   // 0x02c080  trivial: return 1
    i32 Method_034c70(i32);   // 0x034c70  board-tile spawn check for a queued unit
    i32 Method_0358a0(i32);   // 0x0358a0  idle-unit retarget / despawn / near-band keep
    i32 winapi_0267c0_IntersectRect_PtInRect();
    i32 winapi_02a570_IntersectRect(i32);
    i32 winapi_02ab80_PtInRect(i32, i32, i32, i32);
    i32 winapi_02ae00_IntersectRect(i32, i32);
    i32 winapi_02c140_IntersectRect_PtInRect(i32);
    i32 winapi_02dfa0_IntersectRect(i32, i32, i32, i32);
    i32 winapi_02e3a0_PtInRect(i32);
    i32 winapi_031ca0_IntersectRect(i32);
    i32 winapi_032060_IntersectRect(i32);

    // ---- head block 0x000..0xdc: two phase-views of the same bytes ---------
    union {
        // Run-phase (spawn state-machine) view.
        struct {
            i32 m_active;               // +0x000  active gate (methods bail when 0)
            GruntSpawnCtx* m_ctx;       // +0x004  the level/game spawn context
            CTriggerMgr* m_triggerMgr;  // +0x008  the level's CTriggerMgr (4x15 grid)
            Board* m_board;             // +0x00c  the CBrickz pathfinding-grid / tile-map
            char m_pad010[0x18 - 0x10]; // +0x010  (untouched by run ctor)
            i32 m_curCell;              // +0x018  current cell index (=0)
            i32 m_01c;                  // +0x01c  = 1
            i32 m_020;                  // +0x020  = 0x40
            i32 m_024;                  // +0x024  = 0x40
            i32 m_028;                  // +0x028  = 0x40
            i32 m_02c;                  // +0x02c  = 0x32
            i32 m_spawnPct;             // +0x030  = 0x32
            i32 m_034;                  // +0x034
            i32 m_038;                  // +0x038
            i32 m_03c;                  // +0x03c
            i32 m_040;                  // +0x040
            i32 m_044;                  // +0x044
            i32 m_spawnInterval;        // +0x048  = 0
            i32 m_spawnTimer;           // +0x04c  = 0
            i32 m_spawnLastFire;        // +0x050  = 0
            i32 m_repickInterval;       // +0x054  = 0
            i32 m_repickLastFire;       // +0x058  = 0
            i32 m_repickTimer;          // +0x05c  = 0
            i32 m_060;                  // +0x060
            i32 m_064;                  // +0x064
            i32 m_068;                  // +0x068
            i32 m_06c;                  // +0x06c
            i32 m_070;                  // +0x070
            i32 m_budgetMul;            // +0x074  = 0x19
            i32 m_scratch78;            // +0x078  = 0
            i32 m_scratch7c;            // +0x07c  = 0
            i32 m_scratch80;            // +0x080  = 0
            i32 m_scratch84;            // +0x084  = 0
            i32 m_088;                  // +0x088  = 0x32
            i32 m_08c;                  // +0x08c  = 5
            i32 m_090;                  // +0x090  = 5
            i32 m_094;                  // +0x094  = 8
            i32 m_098;                  // +0x098  = 8
            i32 m_09c;                  // +0x09c  = 0x7d0
            i32 m_0a0;                  // +0x0a0  = 0x7d0
            i32 m_0a4;                  // +0x0a4  = 6
            i32 m_0a8;                  // +0x0a8  = 0x32
            i32 m_0ac;                  // +0x0ac  = 8
            i32 m_0b0;                  // +0x0b0  = 8
            i32 m_reserveBudget;        // +0x0b4  = 0x3e8
            i32 m_0b8;                  // +0x0b8  = 0x7d0
            i32 m_moveBudget;           // +0x0bc  = 0x3e8
            i32 m_0c0;                  // +0x0c0  = 0xa
            i32 m_repathBudget;         // +0x0c4  = 0xbb8
            i32 m_0c8;                  // +0x0c8  = 0x7530
            i32 m_0cc;                  // +0x0cc  = 0xbb8
            i32 m_0d0;                  // +0x0d0  (8 B with m_0d4)
            i32 m_0d4;                  // +0x0d4
            i32 m_0d8;                  // +0x0d8
        };
        // Load-phase (LoadConfig) view.
        struct {
            i32 m_0;                 // +0x00  = 1
            CLevelInfo* m_levelInfo; // +0x04  = lvl
            void* m_8;               // +0x08  = lvl->m_68
            CMapDims* m_dims;        // +0x0c  = lvl->m_70
            CLevelSpawnInfo* m_10;   // +0x10  = lvl->m_2c
            i32 m_14;                // +0x14  = m_10->m_2e4
            i32 m_ownerId;           // +0x18  = id (owner/team id)
            char m_pad1c[0x30 - 0x1c];
            DWORD m_defenderChance; // +0x30  DefenderChance
            char m_pad34[0x48 - 0x34];
            DWORD m_gruntCreationTime;    // +0x48  GruntCreationTime (rescaled)
            DWORD m_4c;                   // +0x4c
            i32 m_50;                     // +0x50
            DWORD m_resourceCreationTime; // +0x54  ResourceCreationTime (rescaled)
            i32 m_58;                     // +0x58
            i32 m_5c;                     // +0x5c
            DWORD m_gauntletzChance;      // +0x60  GauntletzChance
            DWORD m_shovelzChance;        // +0x64  ShovelzChance
            DWORD m_spyzChance;           // +0x68  SpyzChance
            DWORD m_brickzChance;         // +0x6c  BrickzChance
            DWORD m_gooberzChance;        // +0x70  GooberzChance
            DWORD m_gruntRatio;           // +0x74  GruntRatio
            i32 m_78;                     // +0x78
            i32 m_7c;                     // +0x7c
            i32 m_80;                     // +0x80
            i32 m_84;                     // +0x84
            char m_pad88[0x8c - 0x88];
            i32 m_8c; // +0x8c  = 6
            i32 m_90; // +0x90  = 6
            i32 m_94; // +0x94  = 6
            i32 m_98; // +0x98  = 6
            char m_pad9c[0xa4 - 0x9c];
            i32 m_a4; // +0xa4  = 8
            char m_pada8[0xac - 0xa8];
            u32 m_ac; // +0xac  = m_dims->m_c / 3
            u32 m_b0; // +0xb0  = m_dims->m_c / 3
            char m_padb4[0xc0 - 0xb4];
            u32 m_c0; // +0xc0  = m_dims->m_c >> 2
            char m_padc4[0xd0 - 0xc4];
            i32 m_markerX; // +0xd0  loop-2 fast-path marker X
            i32 m_markerY; // +0xd4  loop-2 fast-path marker Y
            char m_padd8[0xdc - 0xd8];
        };
    };

    // ---- shared array block 0xdc..0x12c -----------------------------------
    CPtrArray m_candArray; // +0x0dc  CPtrArray  (LoadConfig loop-1 start-coord array)
    CPtrArray m_0f0;       // +0x0f0  CPtrArray  (LoadConfig loop-3 start-coord array)
    CDWordArray m_104;     // +0x104  CDWordArray
    CDWordArray m_118;     // +0x118  CDWordArray

    // ---- tail block 0x12c..0x1e8: two phase-views of the same bytes --------
    union {
        // Run-phase (spawn) view.
        struct {
            i32 m_12c;        // +0x12c  (4-dword inline array)
            i32 m_130;        // +0x130
            i32 m_134;        // +0x134
            i32 m_138;        // +0x138
            i32 m_13c;        // +0x13c  = 0
            i32 m_140;        // +0x140  = 0
            i32 m_144;        // +0x144
            i32 m_claimTimer; // +0x148  (cleared by 02c0a0)
            i32 m_14c;        // +0x14c
            // Idle-behaviour chooser config: m_bandSplitA/B split the top-level roll
            // into three bands; m_band*Div are per-roll divisors; each m_band*Thresh[]
            // is an ascending probability table mapping a roll to an anim/state index.
            i32 m_bandSplitA;      // +0x150  band-A/B split bound
            i32 m_bandSplitB;      // +0x154  band-B/C split bound
            i32 m_bandDiv;         // +0x158  top-level band-selector roll divisor
            i32 m_bandCThresh[3];  // +0x15c  band-C thresholds
            i32 m_bandCDiv;        // +0x168  band-C roll divisor
            i32 m_bandBThresh[9];  // +0x16c  band-B thresholds
            i32 m_bandBDiv;        // +0x190  band-B roll divisor
            i32 m_bandAThresh[20]; // +0x194  band-A thresholds
            i32 m_bandADiv;        // +0x1e4  band-A roll divisor
        };
        // Load-phase (LoadConfig) view: the same 0x150.. table is the per-item
        // spawn-budget running totals seeded from the [Battlez] bute keys.
        struct {
            char m_padTail12c[0x140 - 0x12c];
            i32 m_140b;             // +0x140 = 0
            i32 m_144b;             // +0x144 = ((rand%4)+5)*125*8
            i32 m_148;              // +0x148 = 0
            i32 m_14cb;             // +0x14c = 0
            i32 m_toolzPct;         // +0x150 ToolzPercent (running total seed)
            i32 m_toyzPct;          // +0x154 ToyzPercent
            i32 m_brickzPct;        // +0x158 BrickzPercent
            i32 m_redBrickPct;      // +0x15c RedBrick
            i32 m_blueBrickPct;     // +0x160 BlueBrick
            i32 m_goldBrickPct;     // +0x164 GoldBrick
            i32 m_blackBrickPct;    // +0x168 BlackBrick
            i32 m_babyWalkerzPct;   // +0x16c BabyWalkerz
            i32 m_beachBallzPct;    // +0x170 BeachBallz
            i32 m_bigWheelzPct;     // +0x174 BigWheelz
            i32 m_goKartzPct;       // +0x178 GoKartz
            i32 m_jackInTheBoxzPct; // +0x17c JackInTheBoxz
            i32 m_jumpRopezPct;     // +0x180 JumpRopez
            i32 m_pogoStickzPct;    // +0x184 PogoStickz
            i32 m_scrollzPct;       // +0x188 Scrollz
            i32 m_squeakToyzPct;    // +0x18c SqueakToyz
            i32 m_yoyozPct;         // +0x190 Yoyoz
            i32 m_bombzPct;         // +0x194 Bombz
            i32 m_boomerangzPct;    // +0x198 Boomerangz (+ Brickz)
            i32 m_clubzPct;         // +0x19c Clubz
            i32 m_gauntletzPct;     // +0x1a0 Gauntletz
            i32 m_glovezPct;        // +0x1a4 Glovez
            i32 m_gooberzPct;       // +0x1a8 Gooberz
            i32 m_gravityBootzPct;  // +0x1ac GravityBootz
            i32 m_gunHatzPct;       // +0x1b0 GunHatz
            i32 m_nerfGunzPct;      // +0x1b4 NerfGunz
            i32 m_rockzPct;         // +0x1b8 Rockz
            i32 m_shieldzPct;       // +0x1bc Shieldz
            i32 m_shovelzPct;       // +0x1c0 Shovelz
            i32 m_springzPct;       // +0x1c4 Springz
            i32 m_spyzPct;          // +0x1c8 Spyz
            i32 m_swordzPct;        // +0x1cc Swordz
            i32 m_timeBombzPct;     // +0x1d0 TimeBombz
            i32 m_toobzPct;         // +0x1d4 Toobz
            i32 m_wandzPct;         // +0x1d8 Wandz
            i32 m_welderzPct;       // +0x1dc Welderz
            i32 m_wingzPct;         // +0x1e0 Wingz
            i32 m_1e4;              // +0x1e4 (final running total)
        };
    };
};

#endif // SRC_GRUNTZ_BATTLEZMAPCONFIG_H
