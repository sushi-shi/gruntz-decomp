// UnknownClassArrays.h - the (tomalla-named) config/array bundle whose ctor,
// dtor, and FreeArrays method byte-match retail. The class owns four growable MFC
// arrays - two CPtrArray (+0xdc / +0xf0) and two CDWordArray (+0x104 / +0x118) -
// followed by a block of scalar config fields the ctor seeds with magic startup
// values. FreeArrays recycles the two CPtrArrays' element pointers onto a global
// intrusive freelist, then empties all four arrays.
//
// The array types are FIXED by their retail RTTI/vtable records:
//   +0xdc / +0xf0   vtable s_CPtrArray  @0x5ec2dc  -> CPtrArray
//   +0x104 / +0x118 vtable s_CDWordArray@0x5ec29c  -> CDWordArray
// Both are the real MFC afxcoll classes (0x14 B: vptr@0, m_pData@+4, m_nSize@+8,
// m_nMaxSize@+0xc, m_nGrowBy@+0x10).
//
// Fields are named from their observed use across the 41 methods; only OFFSETS +
// the emitted code bytes are load-bearing (campaign doctrine), so the renames are
// matching-neutral. The remaining m_<hexoffset> fields are seeded-then-serialized
// config constants with no read site in this TU (role unprovable from usage). The
// class is referenced from CBattlezMapConfig (same layout, the map-config loader).
#ifndef SRC_GRUNTZ_UNKNOWNCLASSARRAYS_H
#define SRC_GRUNTZ_UNKNOWNCLASSARRAYS_H
#include <rva.h>

#include <Mfc.h> // CPtrArray, CDWordArray (real afxcoll, 0x14 layout)

// The three TU-shared pointer members below (m_ctx/m_triggerMgr/m_board) point at real engine
// objects, modeled as the file-local views defined in UnknownClassArrays.cpp:
//   m_ctx = the level/game spawn context (holds the CTriggerMgr at +0x68 + the
//           per-band records, 0x238 stride);
//   m_triggerMgr = the level's CTriggerMgr (<Gruntz/TriggerMgr.h>, the 4x15 cell grid);
//   m_board = the CBrickz pathfinding-grid container.
// Forward-declared so each member carries its real type (was void*/char* + per-site
// facet casts). All are 4-byte pointers, so the retype is matching-neutral.
class CTriggerMgr;
struct GruntSpawnCtx;
struct Board;

SIZE_UNKNOWN(CBattlezSpawnMgr_or_CGruntSpawnMgr);
class CBattlezSpawnMgr_or_CGruntSpawnMgr {
public:
    CBattlezSpawnMgr_or_CGruntSpawnMgr();
    ~CBattlezSpawnMgr_or_CGruntSpawnMgr();
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
    void* Method_02ad40(i32);     // 0x02ad40  pick a random idle (m_busy==0) unit from a band row
    i32 Method_02c080(i32);       // 0x02c080  trivial: return 1
    i32 Method_034c70(i32);       // 0x034c70  board-tile spawn check for a queued unit
    i32 Method_0358a0(i32);       // 0x0358a0  idle-unit retarget / despawn / near-band keep
    void Method_034960(i32, i32); // 0x034960  zvec error-report wrapper (this = a _zvec)
    i32 winapi_0267c0_IntersectRect_PtInRect();
    i32 winapi_02a570_IntersectRect(i32);
    i32 winapi_02ab80_PtInRect(i32, i32, i32, i32);
    i32 winapi_02ae00_IntersectRect(i32, i32);
    i32 winapi_02c140_IntersectRect_PtInRect(i32);
    i32 winapi_02dfa0_IntersectRect(i32, i32, i32, i32);
    i32 winapi_02e3a0_PtInRect(i32);
    i32 winapi_031ca0_IntersectRect(i32);
    i32 winapi_032060_IntersectRect(i32);

    i32 m_active;              // +0x000  active gate (methods bail when 0; Clear_02ade0 zeroes it)
    GruntSpawnCtx* m_ctx;      // +0x004  the level/game spawn context (records + CTriggerMgr@+0x68)
    CTriggerMgr* m_triggerMgr; // +0x008  the level's CTriggerMgr (the 4x15 cell grid @+0x1c)
    Board* m_board;            // +0x00c  the CBrickz pathfinding-grid / tile-map
    char m_pad010[0x18 - 0x10]; // +0x010  (untouched by ctor)
    i32 m_curCell;              // +0x018  = 0  (current cell index)
    i32 m_01c;                  // +0x01c  = 1
    i32 m_020;                  // +0x020  = 0x40
    i32 m_024;                  // +0x024  = 0x40
    i32 m_028;                  // +0x028  = 0x40
    i32 m_02c;                  // +0x02c  = 0x32
    i32 m_spawnPct;             // +0x030  = 0x32
    i32 m_034;                  // +0x034  (serialized)
    i32 m_038;                  // +0x038  (serialized)
    i32 m_03c;                  // +0x03c  (serialized)
    i32 m_040;                  // +0x040  (serialized)
    i32 m_044;                  // +0x044  (serialized)
    i32 m_spawnInterval;        // +0x048  = 0
    i32 m_spawnTimer;           // +0x04c  = 0
    i32 m_spawnLastFire;        // +0x050  = 0
    i32 m_repickInterval;       // +0x054  = 0
    i32 m_repickLastFire;       // +0x058  = 0
    i32 m_repickTimer;          // +0x05c  = 0
    i32 m_060;                  // +0x060  (serialized)
    i32 m_064;                  // +0x064  (serialized)
    i32 m_068;                  // +0x068  (serialized)
    i32 m_06c;                  // +0x06c  (serialized)
    i32 m_070;                  // +0x070  (serialized)
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
    i32 m_0d0;                  // +0x0d0  (serialized, 8 B with m_0d4)
    i32 m_0d4;                  // +0x0d4
    i32 m_0d8;                  // +0x0d8  (serialized)
    CPtrArray m_candArray;      // +0x0dc  CPtrArray  (m_pData@+0xe0, m_nSize@+0xe4)
    CPtrArray m_0f0;            // +0x0f0  CPtrArray  (m_pData@+0xf4, m_nSize@+0xf8)
    CDWordArray m_104;          // +0x104  CDWordArray
    CDWordArray m_118;          // +0x118  CDWordArray
    i32 m_12c;                  // +0x12c  (serialized, 4-dword inline array)
    i32 m_130;                  // +0x130
    i32 m_134;                  // +0x134
    i32 m_138;                  // +0x138
    i32 m_13c;                  // +0x13c  = 0
    i32 m_140;                  // +0x140  = 0
    i32 m_144;                  // +0x144
    i32 m_claimTimer;           // +0x148  (cleared by 02c0a0)
    i32 m_14c;                  // +0x14c  (serialized)
    // Idle-behaviour chooser config (config-loaded elsewhere, not ctor/serialized;
    // read by Method_02f620 / Method_02edb0). m_bandSplitA/B split the top-level
    // [1..N] roll into three behaviour bands; m_band{,B,C,A}Div are the per-roll
    // divisors; each m_band{C,B,A}Thresh[] is an ascending probability table mapping
    // a rolled value to an anim/state index.
    i32 m_bandSplitA;      // +0x150  band-A/B split bound
    i32 m_bandSplitB;      // +0x154  band-B/C split bound
    i32 m_bandDiv;         // +0x158  top-level band-selector roll divisor
    i32 m_bandCThresh[3];  // +0x15c  band-C thresholds (modes 0x23..0x25)
    i32 m_bandCDiv;        // +0x168  band-C roll divisor
    i32 m_bandBThresh[9];  // +0x16c  band-B thresholds (modes 0x17..0x1f)
    i32 m_bandBDiv;        // +0x190  band-B roll divisor
    i32 m_bandAThresh[20]; // +0x194  band-A thresholds (modes 1..0x16)
    i32 m_bandADiv;        // +0x1e4  band-A roll divisor
};

#endif // SRC_GRUNTZ_UNKNOWNCLASSARRAYS_H
