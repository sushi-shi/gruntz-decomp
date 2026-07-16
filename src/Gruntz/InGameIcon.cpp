// InGameIcon.cpp - the in-game HUD/cursor icon (a CUserLogic-derived game
// object). Five __thiscall methods reconstructed in ascending-RVA order:
//   0x011d00  ~CInGameIcon   (the bare CUserLogic teardown, /GX frame)
//   0x097680  HandleInput    (the cursor/key input handler -> game-state fields)
//   0x0986b0  PlaceAt        (place/click the icon into a tile cell)
//   0x098c90  Serialize      (CArchive round-trip of the icon state)
//   0x099b10  SetField54     (CMap-lookup the +0x54 id)
//
// CUserLogic / CUserBase / EngStr / CGameObject come from <Gruntz/UserLogic.h>;
// the game-manager singleton (g_gameReg) + the icon factory/records from the
// class header. Engine callees are reloc-masked (no body).
#include <Mfc.h>           // real MFC CMapStringToOb (the icon registry map's Lookup @0x1b8438)
#include <Wap32/zBitVec.h> // GetRetAddr/g_projActCache/g_retAddrBreadcrumb
#include <Io/FileMem.h>    // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/InGameIcon.h>
#include <Gruntz/InGameText.h>     // CInGameText + g_textDispatch (its TU folds in below, wave3-J)
#include <Gruntz/TypeKeyColl.h>    // g_typeCounter (the shared type-id counter)
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable (g_gameReg->m_spriteFactory; GetSel)
#include <Gruntz/SerialArchive.h>  // CSerialArchive (Read +0x2c / Write +0x30) for SerializeMove
#include <Gruntz/SerialObjRef.h>   // the +0x34 serialized-object-reference (Chain @0x8c00)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor::Advance (the +0x1a0 sub-object sync)
#include <Gruntz/Play.h>             // CPlay - g_gameReg->m_curState's concrete play state

// The per-frame draw-delta the anim-cursor sync carries (0x6bf3bc, BSS; the same view
// the indicator sprites use). External/no-body so the load reloc-masks.
extern "C" u32 g_engineFrameDelta;

// The MS-CRT-style LCG rand PeekCycle inlines to roll a random peek sprite (the same
// generator src/Globals.cpp binds + BootyWalkAnim inlines): lazy-seed from timeGetTime,
// advance the 214013/2531011 recurrence, take the top 15 bits.
extern "C" {}

#include <rva.h>

#include <string.h>               // inline strcmp: the ctor's icon-name dispatch chain
#include <Bute/ButeMgr.h>         // CButeTree (the bute store Setup queries)
#include <Wap32/ZVec.h>           // zDArray (the command-dispatch tables)
#include <Gruntz/LogicFnTable.h>  // the shared LogicFnTable dispatch-table shape
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Globals.h>
#include <Wap32/ZDArrayDerived.h>

// The cmd-grid cells are CGrunts; LoadPickupSprites @0x3c6a / LoadGruntTypeTable @0x3bd9
// are CGrunt methods.
#include <Gruntz/Grunt.h> // canonical CGrunt (LoadPickupSprites/LoadGruntTypeTable)

// The global bute store the icon Setup queries (g_buteTree.Find). Owned by
// another TU; declared extern so `ecx=&g_buteTree; call Find` reloc-masks.

// The bute manager singleton the builder queries for the WarpStone target
// (g_buteMgr.GetInt) - declared in <Gruntz/UserLogic.h> (pulled via the header).

// The sprite/animation factory reached as g_gameReg->m_world->m_8 is the canonical
// CSpriteFactory (shared <Gruntz/SpriteFactory.h>); CreateSprite (0x1597b0, __thiscall)
// builds a "SimpleAnimation" glitter sprite (returned as the created CGameObject).

// The current game-state (g_gameReg->m_curState) IS the canonical CPlay during play;
// the icon-setup path reads its level index (CState::m_levelIndex @+0x1c) and stores the
// four WarpStone target pairs into CPlay::m_anchors[4] (@+0x384, stride 8, {m_x, m_y}).
// of the CState* base - single inheritance, address-identical).

// ===========================================================================
// The two file-scope command-dispatch tables (zDArray<member-fn-ptr>) the icon
// registration thunks construct + populate. The ctor (zDArray::Construct, the
// 0x408710 method: stride-4 base init + the 0x5e70fc vptr stamp) reloc-masks.
// Their static-init thunks below build each table over the index band [0x7d0,
// 0x7da].  Shared shape: <Gruntz/LogicFnTable.h>.
// ===========================================================================
DATA(0x002458b0)
extern LogicFnTable g_iconActionTable;
DATA(0x00245928)
extern LogicFnTable g_iconStateTable;

// --- the shared registration infrastructure (mirror of CInGameText's) --------
// The zvec error globals the inlined accessors touch on a bounds miss.

// The shared alloc-scratch cache the zvec IndexToPtr slow path passes to Set: the
// canonical g_projActCache @0x2bf464 (retail's reloc target; the old
// g_zvecErrSentinel @0x1f0464 was a WRONG global - a real but unrelated rva).

// The running game clock (0x245588). The old g_frameTime C++ name lost the
// keep-last dedup at this rva to the canonical extern-C _g_645588; use that so
// the drift/seed loads bind (it IS the clock, not an "icon default").
extern "C" u32 g_frameTime;

// CInGameText's member-fn-ptr dispatch table (folded into this TU, wave3-J). The
// DATA binding lives here in the .cpp (a header DATA() is not scanned by labels.py);
// the sub-field DIR32s (base+0x4.. +0x20) reloc-mask via the base symbol + addend.
DATA(0x00245950)
// @undefined-data: needs storage here, but zDArray has no default ctor (retail's
// static-init thunk calls the 4-arg 0x16de30 ctor on the .bss object). Blocked on
// settling zDArray's default-ctor form - same wall as the LogicFnTable globals.
extern zDArray g_textDispatch;

// g_iconRegCounter was a SECOND NAME for g_typeCounter (0x21aea8 shared type counter) - same address,
// so nothing ever defined it. Unified onto the canonical.

// The scratch name-vec (zDArray<CString> @ 0x6bf650): the registration path
// IndexToPtr's it (growing + CString-constructing fresh slots) to stash the key.

// The two registration key strings (.data constants).
// s_iconKeyA was a SECOND NAME for s_codeA (0x20a454) - same address,
// so nothing ever defined it. Unified onto the canonical.
// s_iconKeyB was a SECOND NAME for s_actKeyB (0x20d1bc) - same address,
// so nothing ever defined it. Unified onto the canonical.

// The handler member functions loaded into the dispatch slots (FUN_004023d3 /
// 0x403c06 into the action table; 0x40370b into the state table). Referenced by
// address so the stored DIR32 operand reloc-masks.
extern i32 IconAction_4023d3();
extern i32 IconAction_403c06();
extern i32 IconState_40370b();

// The zDArray<CString> accessor inlined WITH the per-slot CString-ctor fixup over
// the freshly-grown region (the zDArray::IndexToPtr body).
static inline i32 ResolveNameSlot(zDArray* v, i32 idx) {
    i32 r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else if (v->GrowTo(idx, 0)) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else {
        i32 sentinel = (i32)g_projActCache;
        g_retAddrBreadcrumb = GetRetAddr();
        v->m_errSink->Set((void*)v, sentinel, 0xc);
        r = v->m_spare;
    }
    CString* slot = (CString*)v->m_alloc;
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

// The plain _zvec accessor inlined (no fixup) - the dispatch-table slot resolver.
static inline i32 ResolveSlot(_zvec* v, i32 idx) {
    i32 lo = v->m_lo;
    v->m_grown = 0;
    if (idx >= lo && idx <= v->m_hi) {
        return v->m_base + (idx - lo) * v->m_stride;
    }
    if (v->GrowTo(idx, 0)) {
        return v->m_base + (idx - v->m_lo) * v->m_stride;
    }
    i32 sentinel = (i32)g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    v->m_errSink->Set((void*)v, sentinel, 0xc);
    return v->m_spare;
}

// ===========================================================================
// CInGameIcon::~CInGameIcon  (0x011d00)
// ===========================================================================
// The leaf adds no destructible members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. The empty body is enough.
RVA(0x00011d00, 0x44)
CInGameIcon::~CInGameIcon() {}

// ===========================================================================
// CInGameText::~CInGameText  (0x011dc0)
// ===========================================================================
// The leaf adds no destructible members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. The empty body is enough.
RVA(0x00011dc0, 0x44)
CInGameText::~CInGameText() {}

// ===========================================================================
// CInGameIcon::CInGameIcon(CGameObject*)  (0x095b10)  -- the HUD-icon builder
// ===========================================================================
// Folds the shared CUserLogic(CGameObject*) init (link ctor + logic-type register
// + the three built-in handlers + the data seed; see <Gruntz/UserLogic.h>), stamps
// its own vftable (0x5e7d04), then:
//   - snaps the owner's screen pos to the 0x20 tile grid centre,
//   - flags the owner (+0x74 sentinel / +0x8 |= 0x20000),
//   - swaps the aux bute node (old -> m_30) and seeds the cycle geometry,
//   - the big inline-strcmp dispatch off the icon's type name (owner->m_194+0x24):
//     a code id into owner->m_124 and a category-configure call (SetupSprite), with
//     the treasure / powerup(red glitter) / secret(mission gate) / curse(green
//     glitter) groups; the WarpStonez items also stash the waypoint {x,y} into the
//     level record (g_gameReg->m_curState +0x384.. per index) and stamp m_128,
//   - for a WarpStone in test mode, formats the per-level warp target name and
//     re-applies it,
//   - builds the glitter overlay sprite, then a Check() gate either marks the
//     owner's tile cell occupied (owner->m_188 -> cell+8, toggle 0x40000) or hides
//     the icon (owner->m_8 |= 0x10000).
//
// @early-stop
// Complete reconstruction, ~89.8% fuzzy (0%->90% from the bare stub); parked below
// 100% on two intertwined MSVC5 /O2 walls of this 5616-byte /GX megafunction, both
// verified via llvm-objdump -dr base-vs-target:
//   (1) FRAME-SIZE + EH-STATE shift. The whole CUserLogic(obj) base-ctor fold + own
//   zero-init head is BYTE-EXACT (vptr stamps, link ctor, EngStr temp, the three
//   AddLogic* calls, the data seed - all identical). But cl allocates the local
//   frame at sub esp,0x1c vs retail's 0x18: retail keeps `glitter` in edi across the
//   WarpStone-format block so its [esp+0x38] slot is reused for the warpName CString,
//   while cl spills glitter (its lifetime spans that block), forcing warpName to a
//   fresh slot (+4). That shifts every [esp+N] operand and bumps the CString EH
//   trylevel stamp (ebx=4 vs retail 5) in the tail. Not source-steerable.
//   (2) INLINE-STRCMP regalloc pin (docs/patterns/zero-register-pinning.md family).
//   The ~40-block name dispatch is shape-faithful (same sbb/sbb byte compare, same
//   id/category, same tail-merged SetupSprite cross-jump), but cl caches the name
//   pointer in edi from block 1 (`mov eax,edi`) whereas retail reloads [esp+0x10]
//   for the first ~5 blocks then caches in edx - a free-list coin-flip that shifts
//   the block byte stream. Every call, string literal, field offset, immediate and
//   control-flow edge matches retail. Deferred to the final sweep.
RVA(0x00095b10, 0x15f0)
CInGameIcon::CInGameIcon(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    // --- CInGameIcon own-field zero-init (retail store order @0x95c00) ---
    m_driftPos = 0;
    m_driftThresh = 0;
    m_driftPosHi = 0;
    m_driftThreshHi = 0;
    m_68 = 0;
    m_70 = 0;
    m_6c = 0;
    m_74 = 0;

    // snap owner's screen pos to the 0x20 tile grid centre
    obj->m_screenX = (obj->m_screenX & ~0x1f) + 0x10;
    obj->m_screenY = (obj->m_screenY & ~0x1f) + 0x10;

    if (obj->m_latchedAnimId != 0x17318) {
        obj->m_latchedAnimId = 0x17318;
        obj->m_flags |= 0x20000;
    }

    // swap the aux bute node (save old into m_30) + seed the cycle geometry
    AnimWorkerObj* aux = m_objAux;
    m_prevAnimSetNode = aux->m_1c;
    aux->m_1c = g_buteTree.Find(s_codeA);
    m_savedGeoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);

    m_38->m_flags |= 2;
    SetupSprite(0);

    // second zero batch (retail @0x95ca1)
    m_glitterSprite = 0;
    m_68 = 0;
    m_70 = 0;
    m_6c = 0;
    m_74 = 0;

    i32 glitter = 0;
    char* rec = obj->m_194;
    if (rec != 0) {
        CString name;
        name = rec + 0x24;

        if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_BOMBZ") == 0) {
            m_object->m_124 = 1;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_BOOMERANGZ") == 0) {
            m_object->m_124 = 2;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_BRICKZ") == 0) {
            m_object->m_124 = 3;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_CLUBZ") == 0) {
            m_object->m_124 = 4;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ") == 0) {
            m_object->m_124 = 5;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GLOVEZ") == 0) {
            m_object->m_124 = 6;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GOOBERZ") == 0) {
            m_object->m_124 = 7;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GRAVITYBOOTZ") == 0) {
            m_object->m_124 = 8;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GUNHATZ") == 0) {
            m_object->m_124 = 9;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_NERFGUNZ") == 0) {
            m_object->m_124 = 0xa;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_ROCKZ") == 0) {
            m_object->m_124 = 0xb;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SHIELDZ") == 0) {
            m_object->m_124 = 0xc;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SHOVELZ") == 0) {
            m_object->m_124 = 0xd;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SPRINGZ") == 0) {
            m_object->m_124 = 0xe;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SPYZ") == 0) {
            m_object->m_124 = 0xf;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SWORDZ") == 0) {
            m_object->m_124 = 0x10;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_TIMEBOMBZ") == 0) {
            m_object->m_124 = 0x11;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_TOOBZ") == 0) {
            m_object->m_124 = 0x12;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WANDZ") == 0) {
            m_object->m_124 = 0x13;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ1") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 1;
            CPlay* lvl = (CPlay*)g_gameReg->m_curState;
            lvl->m_anchors[0].m_x = m_object->m_screenX;
            lvl->m_anchors[0].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ2") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 2;
            CPlay* lvl = (CPlay*)g_gameReg->m_curState;
            lvl->m_anchors[1].m_x = m_object->m_screenX;
            lvl->m_anchors[1].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ3") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 3;
            CPlay* lvl = (CPlay*)g_gameReg->m_curState;
            lvl->m_anchors[2].m_x = m_object->m_screenX;
            lvl->m_anchors[2].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ4") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 4;
            CPlay* lvl = (CPlay*)g_gameReg->m_curState;
            lvl->m_anchors[3].m_x = m_object->m_screenX;
            lvl->m_anchors[3].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WELDERZ") == 0) {
            m_object->m_124 = 0x15;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WINGZ") == 0) {
            m_object->m_124 = 0x16;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_BABYWALKERZ") == 0) {
            m_object->m_124 = 0x17;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_BEACHBALLZ") == 0) {
            m_object->m_124 = 0x18;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_BIGWHEELZ") == 0) {
            m_object->m_124 = 0x19;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_GOKARTZ") == 0) {
            m_object->m_124 = 0x1a;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_JACKINTHEBOXZ") == 0) {
            m_object->m_124 = 0x1b;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_JUMPROPEZ") == 0) {
            m_object->m_124 = 0x1c;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_POGOSTICKZ") == 0) {
            m_object->m_124 = 0x1d;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_SCROLLZ") == 0) {
            m_object->m_124 = 0x1e;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_SQUEAKTOYZ") == 0) {
            m_object->m_124 = 0x1f;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_YOYOZ") == 0) {
            m_object->m_124 = 0x20;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_MEGAPHONEZ") == 0) {
            m_object->m_124 = 0x32;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_HEALTH1") == 0) {
            m_object->m_124 = 0x33;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_HEALTH2") == 0) {
            m_object->m_124 = 0x34;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_HEALTH3") == 0) {
            m_object->m_124 = 0x35;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_CONVERSION") == 0) {
            m_object->m_124 = 0x39;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_DEATHTOUCH") == 0) {
            m_object->m_124 = 0x3a;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_GHOST") == 0) {
            m_object->m_124 = 0x36;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_INVULNERABILITY") == 0) {
            m_object->m_124 = 0x38;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_REACTIVEARMOR") == 0) {
            m_object->m_124 = 0x3c;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_ROIDZ") == 0) {
            m_object->m_124 = 0x3b;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_SUPERSPEED") == 0) {
            m_object->m_124 = 0x37;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETW") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
                m_38->m_flags |= 0x10000;
                return;
            }
            m_object->m_124 = 0x5a;
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETA") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
                m_38->m_flags |= 0x10000;
                return;
            }
            m_object->m_124 = 0x5b;
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETR") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
                m_38->m_flags |= 0x10000;
                return;
            }
            m_object->m_124 = 0x5c;
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETP") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
                m_38->m_flags |= 0x10000;
                return;
            }
            m_object->m_124 = 0x5d;
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_STOPWATCH") == 0) {
            m_object->m_124 = 0x4b;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_COIN") == 0) {
            m_object->m_124 = 0x50;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_TOYBOX") == 0) {
            m_object->m_124 = 0x55;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_MINICAM") == 0) {
            m_object->m_124 = 0x40;
            glitter = 1;
            SetupSprite("GAME_CURSE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_SCREENSHAKE") == 0) {
            m_object->m_124 = 0x3e;
            glitter = 1;
            SetupSprite("GAME_CURSE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_RANDOMCOLORZ") == 0) {
            m_object->m_124 = 0x3d;
            glitter = 1;
            SetupSprite("GAME_CURSE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_BLACKSCREEN") == 0) {
            m_object->m_124 = 0x3f;
            glitter = 1;
            SetupSprite("GAME_CURSE");
        }
    }

    // WarpStone test-mode: re-apply the per-level warp target sprite name.
    if (m_object->m_124 == 0x14 && g_gameReg->m_134 == 1) {
        CPlay* lvl = (CPlay*)g_gameReg->m_curState;
        CString levelStr;
        levelStr.Format("Level%i", lvl->m_levelIndex);
        CString warpName;
        i32 target = g_buteMgr.GetInt("WarpStone", levelStr);
        warpName.Format("GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i", target);
        m_object->ApplyName(warpName);
        m_object->m_placeMode = target;
    }

    // glitter overlay sprite for the powerup / curse groups
    if (glitter != 0) {
        CGameObject* fx = g_gameReg->m_world->m_8->CreateSprite(
            0,
            m_object->m_screenX,
            m_object->m_screenY,
            0x17319,
            "SimpleAnimation",
            0x40003
        );
        m_glitterSprite = fx;
        if (glitter == 2) {
            fx->ApplyName("GAME_GLITTERRED");
        }
        if (glitter == 1) {
            m_glitterSprite->ApplyName("GAME_GLITTERGREEN");
        }
        m_glitterSprite->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }

    if (Check() == 0) {
        m_38->m_flags |= 0x10000;
        return;
    }

    // mark the owner's tile cell occupied (or clear the occupancy bit)
    i32 mv = m_object->m_188;
    CTileGrid* grid = g_gameReg->m_tileGrid;
    i32 col = m_object->m_screenX >> 5;
    i32 row = m_object->m_screenY >> 5;
    if ((u32)col < (u32)grid->m_c && (u32)row < (u32)grid->m_10) {
        char* cell = (char*)grid->m_8[row] + col * 0x1c;
        *(i32*)(cell + 8) = mv;
        char* cell0 = (char*)grid->m_8[row] + col * 0x1c;
        if (mv != 0) {
            *(i32*)cell0 |= 0x40000;
        } else {
            *(i32*)cell0 &= ~0x40000;
        }
    }
    m_object->m_stateFlags &= ~1;
}

// ===========================================================================
// CInGameIcon::HandleInput  (0x097680)
// ===========================================================================
// Reads the owning CGameObject's input/command context (m_10): its command id
// (+0x124), a (key,sub) pair (+0x114/+0x118) and a sub-command (+0x130). For the
// 0x55 ("cursor-place") command in the 0x17..0x20 key band, it resolves a icon
// id from the per-player icon table (g_gameReg+0x158, 71*8 stride) and fires the
// factory probe; otherwise, for the 0x1e/0x13 commands it maps the sub-command
// through a small jump table to a fixed icon id and fires. On a hit it stamps the
// command id back into m_10 (+0x58/+0x50/+0x4c) and returns 1; the no-match
// paths return 0.
//
// @early-stop
// CODE BYTE-EXACT - residual is the jumptable-data-overlap scoring artifact
// (docs/patterns/jumptable-data-overlap.md): the dense 0x1e/0x13 switch lowers to
// a .rdata jump table that cl emits as local $L labels while the delinked target
// carries self-relocs, so objdiff under-counts the table region (~9%). The
// dispatch, the index table and every case body are byte-identical to retail
// (verified by raw byte-compare). Effectively matched; deferred only for the
// jump-table reloc-typing fix.
RVA(0x00097680, 0xf5)
i32 CInGameIcon::HandleInput() {
    CGameObject* obj = m_object;
    i32 cmd = obj->m_124;
    i32 rec;
    if (cmd == 0x55) {
        i32 key = obj->m_114;
        i32 sub = obj->m_118;
        if (sub < 0x17 || sub > 0x20) {
            return 0;
        }
        i32 slot = key * 71;
        i32 icon = ((i32*)((char*)g_gameReg + 0x158))[slot * 2];
        if (icon < 0 || icon >= 0x11) {
            icon = 0;
        }
        rec = g_gameReg->m_spriteFactory->GetSel(icon, 0);
        if (rec == 0) {
            rec = g_gameReg->m_spriteFactory->GetSel(1, 0);
        }
    } else if (cmd == 0x1e || cmd == 0x13) {
        i32 icon;
        switch (obj->m_130) {
            case 1:
                icon = 0x10;
                break;
            case 2:
                icon = 1;
                break;
            case 3:
                icon = 0;
                break;
            case 4:
                icon = 0xc;
                break;
            case 5:
                icon = 2;
                break;
            case 6:
                icon = 3;
                break;
            default:
                icon = 7;
                break;
        }
        rec = g_gameReg->m_spriteFactory->GetSel(icon, 0);
        if (rec == 0) {
            rec = g_gameReg->m_spriteFactory->GetSel(1, 0);
        }
    } else {
        return 1;
    }
    CGameObject* o = m_object;
    o->m_drawActive = 1;
    o->m_drawFillCmd = 0xa;
    o->m_drawFillArg = rec;
    return 1;
}

// ===========================================================================
// CInGameIcon::RunAction  (0x097880)
// ===========================================================================
// Resolve g_iconActionTable's slot for `id` (the inline zvec ResolveSlot fast
// [lo,hi] range + slow GrowTo/GetRetAddr/Set rebuild) and, if it holds a registered
// handler PMF, re-resolve the slot and dispatch the PMF on `this`. ResolveSlot has
// side effects (m_grown=0, may grow) so cl re-evaluates it for the guarded call
// rather than CSE-ing - hence the two inline expansions.
RVA(0x00097880, 0x102)
void CInGameIcon::RunAction(i32 id) {
    if (*(IconActHandler*)ResolveSlot(&g_iconActionTable, id) != 0) {
        (this->*(*(IconActHandler*)ResolveSlot(&g_iconActionTable, id)))();
    }
}

// ===========================================================================
// InitIconActionTable  (0x097800)
// ===========================================================================
// File-scope static-init thunk: construct the action dispatch table over the
// index band [0x7d0, 0x7da].
RVA(0x00097800, 0x15)
void InitIconActionTable() {
    ((CZDArrayDerived*)&g_iconActionTable)->Construct(0x7d0, 0x7da);
}

// ===========================================================================
// RegisterIconActions  (0x0979e0)
// ===========================================================================
// Register two icon-action handlers into g_iconActionTable: key A -> 0x4023d3,
// key B -> 0x403c06. Each: bute-tree Find (Insert + cache the name into the
// scratch zDArray<CString> when absent, bump the counter), resolve the table
// slot for the key index, load the handler member-fn-ptr.
// ---------------------------------------------------------------------------
// @early-stop
// inlined zDArray/zvec IndexToPtr regalloc wall (the documented ZVec family, see
// ZVec.cpp + RegisterTextLogic): both register blocks + the CString-ctor fixup
// loops are reconstructed faithfully, but cl pins the index/this/base across the
// grow branches differently than retail. Logic + find/insert + the fn-ptr stores
// correct; the register assignment is not source-steerable.
RVA(0x000979e0, 0x2ac)
void RegisterIconActions() {
    i32 idxA = (i32)g_buteTree.Find(s_codeA);
    if (idxA == 0) {
        g_buteTree.Insert(s_codeA, (void*)g_typeCounter);
        i32 slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *(CString*)slot = s_codeA;
        g_typeCounter++;
    }
    i32 dslotA = ResolveSlot(&g_iconActionTable, idxA);
    *(void**)dslotA = (void*)&IconAction_4023d3;

    i32 idxB = (i32)g_buteTree.Find(s_actKeyB);
    if (idxB == 0) {
        g_buteTree.Insert(s_actKeyB, (void*)g_typeCounter);
        i32 slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *(CString*)slot = s_actKeyB;
        g_typeCounter++;
    }
    i32 dslotB = ResolveSlot(&g_iconActionTable, idxB);
    *(void**)dslotB = (void*)&IconAction_403c06;
}

// ===========================================================================
// InitIconStateTable  (0x097d60)
// ===========================================================================
// File-scope static-init thunk: construct the state dispatch table over the
// index band [0x7d0, 0x7da].
RVA(0x00097d60, 0x15)
void InitIconStateTable() {
    ((CZDArrayDerived*)&g_iconStateTable)->Construct(0x7d0, 0x7da);
}

// ===========================================================================
// CInGameIcon::RunState  (0x097de0)
// ===========================================================================
// Same dispatcher archetype as RunAction, over g_iconStateTable: resolve the slot
// for `id` (inline zvec ResolveSlot) and, if it holds a registered handler PMF,
// re-resolve and dispatch the PMF on `this` (two inline ResolveSlot expansions).
RVA(0x00097de0, 0x102)
void CInGameIcon::RunState(i32 id) {
    if (*(IconActHandler*)ResolveSlot(&g_iconStateTable, id) != 0) {
        (this->*(*(IconActHandler*)ResolveSlot(&g_iconStateTable, id)))();
    }
}

// ===========================================================================
// RegisterIconState  (0x097f40)
// ===========================================================================
// Register one icon-state handler into g_iconStateTable (key A -> 0x40370b):
// bute-tree Find (Insert + cache the name when absent, bump the counter),
// resolve the table slot for the key index, load the handler member-fn-ptr.
// ---------------------------------------------------------------------------
// @early-stop
// inlined zDArray/zvec IndexToPtr regalloc wall (the documented ZVec family, see
// ZVec.cpp + RegisterTextLogic ~96%): faithfully reconstructed; cl's index/this
// register assignment across the grow branches differs from retail and is not
// source-steerable. Logic + find/insert + the fn-ptr store correct.
RVA(0x00097f40, 0x18d)
void RegisterIconState() {
    i32 idx = (i32)g_buteTree.Find(s_codeA);
    if (idx == 0) {
        g_buteTree.Insert(s_codeA, (void*)g_typeCounter);
        i32 slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *(CString*)slot = s_codeA;
        g_typeCounter++;
    }
    i32 dslot = ResolveSlot(&g_iconStateTable, idx);
    *(void**)dslot = (void*)&IconState_40370b;
}

// ===========================================================================
// CInGameIcon::RefreshCell  (0x098340)
// ===========================================================================
// If the icon's tracked position has drifted at least g_frameTime past the
// owning object's stored position (a 64-bit signed compare of {m_driftPosHi:m_driftPos}
// against {m_driftThreshHi:m_driftThresh}), OR the owning object's tile cell is empty/off-grid,
// flag the +0x38 render object dirty (|= 0x10000). Returns 0.
RVA(0x00098340, 0x71)
i32 CInGameIcon::RefreshCell() {
    CGameObject* obj = m_object;
    i32 tileY = obj->m_screenX >> 5;
    i32 tileX = (obj->m_screenY + 0x18) >> 5;
    i64 delta = (i64)(u32)g_frameTime - *(i64*)&m_driftPos;
    if (delta < *(i64*)&m_driftThresh) {
        CTileGrid* grid = g_gameReg->m_tileGrid;
        i32 cell;
        if ((u32)tileY < (u32)grid->m_c && (u32)tileX < (u32)grid->m_10) {
            i32* row = grid->m_8[tileX];
            cell = row[tileY * 8 - tileY + 2];
        } else {
            cell = 0;
        }
        if (cell != 0) {
            return 0;
        }
    }
    CGameObject* r = m_38;
    r->m_flags |= 0x10000;
    return 0;
}

// ===========================================================================
// CInGameIcon::SerializeMove  (0x0983e0)  -- vtable slot 1 (the in-level serialize)
// ===========================================================================
// The tile-logic-leaf slot-1 serialize: chain the base CUserLogic::SerializeChain
// (bail 0), the +0x34 serialized-object-reference (CSerialObjRef::Chain, bail 0),
// then round-trip the icon's two i64 drift fields (m_driftPos @+0x58, m_driftThresh
// @+0x60; 8 bytes each) per mode (4 = write @+0x30, 7 = read @+0x2c). Returns 1.
// The archive is the slot-1 CGruntArchive; its Read/Write live at the shared
// CSerialArchive slots (+0x2c/+0x30), reached by that view at the calls.
RVA(0x000983e0, 0x98)
i32 CInGameIcon::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (((CMovingLogicBase*)this)->Serialize((CSerialArchive*)((i32)ar), mode, a3, a4) == 0) {
        return 0;
    }
    if (((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, mode, a3, (CSerialObj*)a4) == 0) {
        return 0;
    }
    // The two i64 drift fields sit contiguous (m_driftPos @+0x58, m_driftThresh @+0x60);
    // retail walks one pointer over the blob (edi += 0x58 hoisted, then edi += 8).
    char* p = (char*)&m_driftPos;
    switch (mode) {
        case 4:
            ((CSerialArchive*)ar)->Write(p, 8);
            p += 8;
            ((CSerialArchive*)ar)->Write(p, 8);
            break;
        case 7:
            ((CSerialArchive*)ar)->Read(p, 8);
            p += 8;
            ((CSerialArchive*)ar)->Read(p, 8);
            break;
    }
    return 1;
}

// ===========================================================================
// CInGameIcon::PeekCycle  (0x0984b0)
// ===========================================================================
// Per-frame peek update: advance the +0x1a0 anim cursor, then dispatch on the icon's
// command id. For the 0x55 (cursor) command, if the icon's tile cell carries any
// action/occupancy flag (& 0x939 or & 2) it clears that cell's occupancy and flags the
// +0x38 object dirty. For the 0x13/0x1e (peek) commands, once the peek timer
// ({m_68} vs {m_70}) elapses it rolls a random pickup sprite (the inline LCG rand()%17
// -> GetSel), publishes it into the bound object's draw fields, and re-arms the timer
// ({m_70}=0xfa, {m_68}=g_frameTime). Returns 0.
//
// @early-stop
// regalloc/scheduling wall (zero-register-pinning class): the command dispatch, the
// tile-cell flag test + occupancy clear, the i64 peek-timer compare, the inline
// timeGetTime-seeded 214013/2531011 rand()%0x11, the GetSel + draw-field publish and the
// timer re-arm are all reconstructed byte-faithfully; cl's exact ebx/edi/esi pin across
// this 390-byte body differs from retail's and is not source-steerable. Deferred.
RVA(0x000984b0, 0x186)
i32 CInGameIcon::PeekCycle() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance(g_engineFrameDelta);
    CGameObject* obj = m_object;
    i32 cmd = obj->m_124;
    if (cmd == 0x55) {
        CGameRegistry* reg = g_gameReg;
        i32 tileY = obj->m_screenY >> 5;
        CTileGrid* grid = reg->m_tileGrid;
        i32 tileX = obj->m_screenX >> 5;
        i32 cell;
        if ((u32)tileX < (u32)grid->m_c && (u32)tileY < (u32)grid->m_10) {
            cell = grid->m_8[tileY][tileX * 7];
        } else {
            cell = 1;
        }
        if ((cell & 0x939) != 0 || (cell & 2) != 0) {
            if ((u32)tileX < (u32)grid->m_c && (u32)tileY < (u32)grid->m_10) {
                grid->m_8[tileY][tileX * 7 + 2] = 0;
                grid->m_8[tileY][tileX * 7] &= ~0x40000;
            }
            m_38->m_flags |= 0x10000;
        }
        return 0;
    }
    if (cmd != 0x13 && cmd != 0x1e) {
        return 0;
    }
    if (obj->m_130 != 0) {
        return 0;
    }
    if ((i64)(u32)g_frameTime - *(i64*)&m_68 >= *(i64*)&m_70) {
        u32 x;
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            x = ::timeGetTime();
        } else {
            x = g_randSeed;
        }
        g_randSeed = x * 214013 + 2531011;
        i32 rec = g_gameReg->m_spriteFactory->GetSel((((i32)g_randSeed >> 16) & 0x7fff) % 0x11, 0);
        CGameObject* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 0xa;
        o->m_drawFillArg = rec;
        m_70 = 0xfa;
        m_74 = 0;
        m_68 = g_frameTime;
        m_6c = 0;
    }
    return 0;
}

// Clear the "occupied" bit (0x40000) in the tile cell the owning object stands
// on. The grid is g_gameReg->m_tileGrid: m_8[tileY] is a row base (each row a flat
// array of 0x1c-byte cells = 7 dwords), the cell for tileX sits at offset
// (tileX*71)*4 ... matching retail's `eax=tileX*8-tileX` then `<<2` and the
// `[m_8[tileY] + eax + 8]=0` / `[m_8[tileY] + eax] &= ~0x40000` pair.
static inline void ClearTileBit(CGameRegistry* reg, CGameObject* owner) {
    CTileGrid* grid = reg->m_tileGrid;
    i32 tileX = owner->m_screenY >> 5;
    i32 tileY = owner->m_screenX >> 5;
    if ((u32)tileY < (u32)grid->m_c && (u32)tileX < (u32)grid->m_10) {
        i32 rowByte = tileX * 4;
        i32 cellOff = (tileY * 8 - tileY) * 4;
        char* cell0 = (char*)*(i32**)((char*)grid->m_8 + rowByte);
        *(i32*)(cell0 + cellOff + 8) = 0;
        char* cell1 = (char*)*(i32**)((char*)grid->m_8 + rowByte);
        *(i32*)(cell1 + cellOff) &= ~0x40000;
    }
}

// ===========================================================================
// CInGameIcon::PlaceAt  (0x0986b0)
// ===========================================================================
// The cursor place/click handler. Resolves the per-player icon record from
// g_gameReg->m_68 [(arg0*15 + arg1) dword index, +0x1c base], binds its sprite
// set (LoadPickupSprites / LoadGruntTypeTable), optionally posts an input flush
// when the icon is on-screen, clears the owner's tile-occupancy bit, and (the
// full non-0x55 path) re-seeds the icon's animation/state fields from the bute
// store. Returns 1 on a successful place, 0 on a reject.
//
// @early-stop
// regalloc/scheduling wall (zero-register-pinning class): 780-byte dense body
// with 5 pinned register vars (ebx=1, ebp, edi, esi=this, edx=g_gameReg). The
// logic, the cell index, both call arg-sets, the on-screen bounds gates, the
// shared tile-clear and the bute re-seed tail are all reconstructed; MSVC's exact
// ebx/ebp/edi allocation across the two halves is not source-steerable. Deferred.
RVA(0x000986b0, 0x30c)
i32 CInGameIcon::PlaceAt(i32 arg0, i32 arg1) {
    CGameRegistry* reg = g_gameReg;
    if (reg->m_134 == 1 && arg0 != g_curPlayer && m_object->m_124 != 0x55) {
        return 0;
    }
    CGameObject* obj = m_object;
    if (obj->m_124 == 0x55) {
        // ---- selection/preview path ----
        i32 param = obj->m_118;
        i32 matchActive = 0;
        i32 flag = 1;
        if (obj->m_114 == arg0) {
            matchActive = 1;
            flag = 0;
        }
        i32 sub = obj->m_130;
        i32 idx = arg0 * 15 + arg1;
        CIconRecord* cell = ((CIconRecord**)((char*)reg->m_cmdGrid + 0x1c))[idx];
        i32 ok;
        if (cell == 0 || cell->m_1fc == 0) {
            ok = 0;
        } else if (matchActive) {
            ok = ((CGrunt*)cell)->LoadPickupSprites(param, flag, 0, sub, 0);
        } else {
            ok = ((CGrunt*)cell)->LoadGruntTypeTable(param, flag, sub, 0);
        }
        reg = g_gameReg;
        if (ok == 0) {
            return 0;
        }
        if (m_cmapId != 0) {
            CGameObject* o = m_object;
            if (o->m_screenX < reg->m_viewOriginR && o->m_screenX >= reg->m_viewOriginL
                && o->m_screenY < reg->m_viewOriginB && o->m_screenY >= reg->m_viewOriginT) {
                Eng_PostCmd(g_sndCueTag, 0, 0, 0);
                reg = g_gameReg;
            }
        }
        ClearTileBit(reg, m_object);
        CGameObject* r = m_38;
        r->m_flags |= 0x10000;
        return 1;
    }

    // ---- full place path (cmd != 0x55) ----
    i32 sub = obj->m_130;
    i32 cmd = obj->m_124;
    i32 idx = arg0 * 15 + arg1;
    CIconRecord* cell = ((CIconRecord**)((char*)reg->m_cmdGrid + 0x1c))[idx];
    i32 ok;
    if (cell == 0 || cell->m_1fc == 0) {
        ok = 0;
    } else {
        ok = ((CGrunt*)cell)->LoadPickupSprites(cmd, 0, 0, sub, 1);
    }
    reg = g_gameReg;
    if (ok == 0) {
        return 0;
    }
    if (cmd == 0x14) {
        CIconRecord* placed = ((CIconRecord**)((char*)reg->m_cmdGrid + 0x1c))[idx];
        if (placed != 0) {
            placed->m_38c = m_object->m_placeMode;
            reg = g_gameReg;
        }
    }
    if (m_cmapId != 0) {
        CGameObject* o = m_object;
        if (o->m_screenX < reg->m_viewOriginR && o->m_screenX >= reg->m_viewOriginL
            && o->m_screenY < reg->m_viewOriginB && o->m_screenY >= reg->m_viewOriginT) {
            Eng_PostCmd(g_sndCueTag, 0, 0, 0);
            reg = g_gameReg;
        }
    }
    ClearTileBit(reg, m_object);
    CGameObject* owner = m_38;
    if (owner->m_120 > 0) {
        owner->m_stateFlags |= 1;
        AnimWorkerObj* aux = m_objAux;
        m_prevAnimSetNode = aux->m_1c;
        aux->m_1c = g_buteTree.Find(s_actKeyB);
        owner = m_38;
        m_driftPos = owner->m_120;
        m_driftPosHi = 0;
        m_driftThresh = g_frameTime;
        m_driftThreshHi = 0;
        return 1;
    }
    CGameObject* rend = m_glitterSprite;
    if (rend != 0) {
        rend->m_flags |= 0x10000;
        m_glitterSprite = 0;
    }
    CGameObject* r = m_38;
    r->m_flags |= 0x10000;
    return 1;
}

// ===========================================================================
// CInGameIcon::Reposition  (0x098a90)
// ===========================================================================
// Per-frame drift re-place: advance the +0x1a0 anim cursor, and once the drift
// timer ({m_driftPos} vs {m_driftThresh}) has elapsed, re-seat the icon:
//   - clear the +0x38 object's active bit (m_stateFlags &= ~1),
//   - swap the aux bute node to "A" (saving the old into m_prevAnimSetNode),
//   - resolve the tile the icon currently occupies; if that cell carries a bound
//     object id, look it up in the world sprite factory's +0x48 map and flag it,
//   - clear that cell's occupancy, then re-mark the owner's tile cell with its
//     object id (m_188) - occupied (|=0x40000) when non-zero, cleared otherwise.
// Returns 0.
//
// @early-stop
// regalloc/scheduling wall (zero-register-pinning class): the logic, the i64 drift
// compare, the bute-node swap, both tile-cell index computations, the +0x48 map
// Lookup + flag, and the occupancy set/clear are all reconstructed byte-faithfully;
// cl's exact reload/pin of g_gameReg + the grid across the three tile-cell blocks of
// this 397-byte body differs from retail's and is not source-steerable. Deferred.
RVA(0x00098a90, 0x18d)
i32 CInGameIcon::Reposition() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance(g_engineFrameDelta);
    i64 delta = (i64)(u32)g_frameTime - *(i64*)&m_driftPos;
    if (delta >= *(i64*)&m_driftThresh) {
        CGameObject* r = m_38;
        r->m_stateFlags &= ~1;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find(s_codeA);

        CGameRegistry* reg = g_gameReg;
        CGameObject* obj = m_object;
        i32 tileX = obj->m_screenX >> 5;
        i32 tileY = obj->m_screenY >> 5;
        CTileGrid* grid = reg->m_tileGrid;
        i32 cellVal;
        if ((u32)tileX < (u32)grid->m_c && (u32)tileY < (u32)grid->m_10) {
            cellVal = grid->m_8[tileY][tileX * 7 + 2];
        } else {
            cellVal = 0;
        }
        if (cellVal != 0) {
            void* found = 0;
            if (((CMapPtrToPtr*)((char*)reg->m_world->m_8 + 0x48))->Lookup((void*)cellVal, found)
                && found != 0) {
                ((CGameObject*)found)->m_flags |= 0x10000;
            }
        }
        reg = g_gameReg;
        grid = reg->m_tileGrid;
        if ((u32)tileX < (u32)grid->m_c && (u32)tileY < (u32)grid->m_10) {
            grid->m_8[tileY][tileX * 7 + 2] = 0;
            grid->m_8[tileY][tileX * 7] &= ~0x40000;
        }
        obj = m_object;
        grid = g_gameReg->m_tileGrid;
        i32 tileX2 = obj->m_screenX >> 5;
        i32 tileY2 = obj->m_screenY >> 5;
        i32 mv = obj->m_188;
        if ((u32)tileX2 < (u32)grid->m_c && (u32)tileY2 < (u32)grid->m_10) {
            grid->m_8[tileY2][tileX2 * 7 + 2] = mv;
            if (mv != 0) {
                grid->m_8[tileY2][tileX2 * 7] |= 0x40000;
            } else {
                grid->m_8[tileY2][tileX2 * 7] &= ~0x40000;
            }
        }
    }
    return 0;
}

// ===========================================================================
// CInGameIcon::Serialize  (0x098c90)
// ===========================================================================
// The CArchive load/store of the icon state: guard on the archive, chain the
// shared CUserLogic::SerializeChain, then a `sub 4 / sub 3 / dec` running tag
// switch dispatching per-mode CArchive Read/Write (vtable +0x2c write / +0x30
// read) of the +0x34..+0x78 fields, with inline strlen/strcpy CString round-trips
// (repne scas / rep movs), a g_serialCounter bump, and two registry CMap lookups
// (0x1b8438 / 0x1b8760) re-binding the +0x40/+0x54 ids.
//
// @early-stop
// DEFERRED to the final sweep (NOT reconstructed). Two blockers: (1) the recorded
// boundary is wrong - dump_target/Ghidra report 0x31f (799 B) but the body's own
// branches target 0x99000, ~0x370 B past the entry, so the delinked target object
// is truncated and cannot match until the function length is re-derived; (2) the
// body is a ~880 B dense CArchive/CString/CMap marshaler whose stack-CString temps
// force a /GX EH-state schedule (the eh-state-numbering + outparam-zeroinit walls).
// A faithful reconstruction needs the corrected boundary first; pinned no-body so
// its RVA registers and the unit builds. See report.
RVA(0x00098c90, 0x31f)
i32 CInGameIcon::Serialize(CArchive*, i32, i32, i32) {
    return 0;
}

// ===========================================================================
// CInGameText (ex InGameText.cpp, merged wave3-J): the 0x095b10-0x099b46
// interval is ONE original TU - the text is an I-T-I sandwich (icon x14 | text
// 0x99110..0x99a30 | icon SetField54 @0x99b10) and the private initialized-
// .data extents are contiguous (icon 0x2111b8..0x21136c, text 0x21137c). The
// shared registration identifiers unify onto this TU's names (the ex
// g_textRegCounter/s_textLogicKey duplicates == g_typeCounter/s_codeA).
// ===========================================================================

// The member function the text dispatch slot is loaded with (FUN_00402013, a
// thunk to a CInGameText handler). Referenced by address so its DIR32 reloc-masks.
extern i32 TextLogic_402013();

// The member-function-pointer the text dispatch table resolves and invokes on `this`.
typedef i32 (CUserLogic::*LogicFn)();

// CInGameText::CInGameText @0x099110 - fold the shared CUserLogic(obj) init, then
// (unless the registry is in the no-place mode m_134==2) bind the "A" bute node,
// the cycle geometry, the "GAME_HELPBOX" sprite name; flag the sub-object; run the
// on-screen visibility gate keyed by the bound object's place mode (m_128); and on
// the visible path snap the screen position to the tile grid + seed the +0x74
// layer key and the +0x54/+0x58 scalars to -1.
//
// @early-stop
// register-pinning/eh-ctor-vptr-restamp wall (docs/patterns/zero-register-pinning.md,
// eh-ctor-vptr-restamp-position.md): body byte-faithful (every op/offset/imm/string
// + the m_128 visibility branch tangle match retail; constant 2 pins in ebx like
// retail). Residual is the /GX leaf-vptr re-stamp position + the visibility-gate
// branch-polarity (retail emits `je visible` where structured C emits `jne hide`).
RVA(0x00099110, 0x215)
CInGameText::CInGameText(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    if (g_gameReg->m_134 == 2) {
        m_38->m_flags |= 0x10000;
        return;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_savedGeoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_38->ApplyName("GAME_HELPBOX");
    m_38->m_flags |= 2;

    i32 vis = m_object->m_placeMode;
    if (vis == 1) {
        if (g_gameReg->m_isEasyMode == 0) {
            m_38->m_flags |= 0x10000;
            return;
        }
        if (g_gameReg->m_134 != 1) {
            m_38->m_flags |= 0x10000;
            return;
        }
    } else if (vis == 2) {
        if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
            m_38->m_flags |= 0x10000;
            return;
        }
    }

    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    if (m_object->m_latchedAnimId != 0x17318) {
        m_object->m_latchedAnimId = 0x17318;
        m_object->m_flags |= 0x20000;
    }
    m_cachedAreaId = -1;
    m_cachedSubId = -1;
}

// The activation-coordinate registry view of the dispatch table (g_textDispatch
// @0x645950): InitActReg builds it over the fixed [2000, 2010] range via the
// shared registry ctor (0x408710, __thiscall ret 8).
struct CTextActReg {}; // Construct = CZDArrayDerived::Construct; cast at the call

// CInGameText::InitActReg @0x0993e0 - construct the class's activation-coordinate
// registry (g_textDispatch @0x645950) over [2000, 2010]. Free init thunk.
RVA(0x000993e0, 0x15)
void CInGameText::InitActReg() {
    ((CZDArrayDerived*)&g_textDispatch)->Construct(2000, 2010);
}

// ===========================================================================
// CInGameText::Dispatch  (0x099460)
// ===========================================================================
// Index the global member-fn-ptr table by `idx`; if the resolved slot holds a
// non-null member function, invoke it on `this`. The bounds-check + grow of the
// table accessor is inlined (the _zvec::IndexToPtr body, no out-of-line call),
// computed once for the null-test and once for the call.
RVA(0x00099460, 0x102)
void CInGameText::Dispatch(i32 idx) {
    if (*(void**)ResolveSlot(&g_textDispatch, idx) != 0) {
        LogicFn fn = *(LogicFn*)ResolveSlot(&g_textDispatch, idx);
        (this->*fn)();
    }
}

// ===========================================================================
// RegisterTextLogic  (0x0995c0)
// ===========================================================================
// The file-scope static registration thunk for the text-logic handler: look the
// key up in the bute tree; if absent, Insert it under the running counter and
// cache the key name into the scratch zDArray<CString> slot (growing it), then
// bump the counter. Either way, resolve the dispatch-table slot for the key index
// and load it with the handler member-fn-ptr (FUN_00402013).
// ---------------------------------------------------------------------------
// @early-stop
// inlined zDArray/zvec IndexToPtr regalloc wall (the documented ZVec family - see
// ZVec.cpp's IndexToPtr/GrowTo @early-stops, ~80%): the two inlined accessors +
// the CString-ctor fixup loop are reconstructed faithfully, but cl pins the
// index/this/base across the grow branches differently than retail and permutes
// the two-block offset tails. Logic + the bute find/insert + the fn-ptr store are
// correct; the register assignment is not source-steerable.
RVA(0x000995c0, 0x18d)
void RegisterTextLogic() {
    i32 idx = (i32)g_buteTree.Find(s_codeA);
    if (idx == 0) {
        g_buteTree.Insert(s_codeA, (void*)g_typeCounter);
        i32 slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *(CString*)slot = s_codeA;
        g_typeCounter++;
    }
    i32 dslot = ResolveSlot(&g_textDispatch, idx);
    *(void**)dslot = (void*)&TextLogic_402013;
}

// ===========================================================================
// CInGameText::Serialize  (0x099a30)
// ===========================================================================
// Guard on the archive, chain the shared CUserLogic::SerializeChain, then the
// +0x34 sub-object's own serializer, then round-trip the two own dwords at
// +0x54/+0x58: tag 4 stores (archive Write), tag 7 loads (archive Read).
RVA(0x00099a30, 0xaa)
i32 CInGameText::Serialize(CSerialArchive* ar, i32 tag, i32 a, i32 b) {
    if (ar == 0) {
        return 0;
    }
    if (((CMovingLogicBase*)this)->Serialize((CSerialArchive*)((i32)ar), tag, a, b) == 0) {
        return 0;
    }
    if (((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, a, (CSerialObj*)b) == 0) {
        return 0;
    }
    switch (tag) {
        case 4:
            ar->Write(&m_cachedAreaId, 4);
            ar->Write(&m_cachedSubId, 4);
            break;
        case 7:
            ar->Read(&m_cachedAreaId, 4);
            ar->Read(&m_cachedSubId, 4);
            break;
    }
    return 1;
}

// ===========================================================================
// CInGameIcon::SetField54  (0x099b10)
// ===========================================================================
// When v != 0, look it up in the registry's CMap (g_gameReg->m_world->m_28, Lookup
// at +0x10) into a local, then store the located value (or 0) into +0x54.
// @interleaver CInGameIcon::SetField54 emitted-in <boundary: InGameTextUpdate.cpp
// Update@CInGameText @0x997c0 (before) + AreaMgr.cpp TokenMgrReset99b80 @0x99b80
// (after)>. A /Gy first-use COMDAT the linker scattered between two OTHER units.
RVA(0x00099b10, 0x36)
void CInGameIcon::SetField54(i32 v) {
    void* found = 0; // CMapStringToPtr's value slot (Lookup 0x1b8438 takes void*&)
    if (v != 0) {
        found = 0;
        ((CGameRegMapHolder*)g_gameReg->m_world)->m_28->m_10map.Lookup((const char*)v, found);
    }
    m_cmapId = (i32)found;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CGameRegMapHolder);
SIZE_UNKNOWN(CIconMapHolder);
SIZE_UNKNOWN(CIconRecord);
SIZE_UNKNOWN(CInGameIcon);
SIZE_UNKNOWN(IconSpriteFactory);
SIZE_UNKNOWN(LogicFnTable);
SIZE_UNKNOWN(CInGameText);
SIZE_UNKNOWN(CTextActReg);
