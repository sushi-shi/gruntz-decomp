#include <Mfc.h>              // real MFC CMapStringToOb (the icon registry map's Lookup @0x1b8438)
#include <Wap32/zBitVec.h>    // GetRetAddr/g_projActCache/g_retAddrBreadcrumb
#include <Io/FileMem.h>       // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Gruntz/GruntzMgr.h> // complete CGruntzMgr (g_gameReg real type)
#include <Gruntz/InGameIcon.h>
#include <Gruntz/LeafCue.h> // LeafCue::PlayIfElapsed (the stale-ecx cue pokes)
#include <Gruntz/ToyPeek.h> // CToyPeek::FireActivation @0x97de0 (its slot 4 lives in this .text run)
#include <Gruntz/ActReg.h>
#include <Gruntz/InGameText.h> // CInGameText + CActRegPool<CInGameText>::s_table (its TU folds in below, wave3-J)
#include <Gruntz/TypeKeyColl.h>    // g_typeCounter (the shared type-id counter)
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable (g_gameReg->m_spriteFactory; GetSel)
#include <Gruntz/SerialArchive.h>  // CFileMemBase (Read +0x2c / Write +0x30) for SerializeMove
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor::Advance (the +0x1a0 sub-object sync)
#include <Gruntz/Play.h>             // CPlay - g_gameReg->m_curState's concrete play state

#include <rva.h>

#include <string.h>                   // inline strcmp: the ctor's icon-name dispatch chain
#include <Bute/ButeMgr.h>             // CButeTree (the bute store Setup queries)
#include <Wap32/ZVec.h>               // _zdvec (the command-dispatch tables)
#include <Gruntz/LogicFnTable.h>      // the shared CActReg dispatch-table shape
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/AniElement.h>        // CAniElement complete (KeyOfValue takes the CObject upcast)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // the anim registry (m_10 map + KeyOfValue)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // world sound registry and its keyed asset map

#include <Gruntz/Grunt.h>      // canonical CGrunt (LoadPickupSprites/LoadGruntTypeTable)
#include <Gruntz/TriggerMgr.h> // CTriggerMgr - m_cmdGrid (its m_grid CGrunt cells; ex CIconRecord)
#include <Gruntz/Brickz.h>     // canonical BrickzCell - the 0x1c-byte tile cell at m_rows[y][x]
#include <Gruntz/SoundState.h> // ex Globals.h transitive
#include <Gruntz/Random.h>     // ex Globals.h transitive
#include <Utils/MapTyped.h>    // MapLookupById - the forced id->void* key pun

VTBL(CInGameText, 0x001e7cac);
VTBL(CInGameIcon, 0x001e7d04);
template<> DATA(0x002458b0)
CActReg CActRegPool<CInGameIcon>::s_table(2000, 2010);
template<> DATA(0x00245928)
CActReg CActRegPool<CToyPeek>::s_table(2000, 2010);
template<> DATA(0x00245950)
CActReg CActRegPool<CInGameText>::s_table(2000, 2010);

static inline CString* ResolveNameSlot(CTypeCollRuntime* v, i32 idx) {
    CString* r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->Elem(idx);
    } else if (v->GrowTo(idx, 0)) {
        r = v->Elem(idx);
    } else {
        void* sentinel = g_projActCache;
        g_retAddrBreadcrumb = GetRetAddr();
        v->m_errSink->Set(static_cast<void*>(v), sentinel, 0xc);
        r = v->Scratch();
    }
    CString* slot = v->Slots();
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

// The same two resolvers one inline level shallower: the grow-fail tail stays as the
// out-of-line zErrHandling::Report call (0x34960) instead of expanding. cl5 spends its
// inline budget from the outside in, so the TWO-key registrar below keeps the tail
// outlined at three of its four lookups while the one-key registrars expand both.
// docs/patterns/act-registrar-report-outline-budget.md
static inline CString* ResolveNameSlotCallReport(CTypeCollRuntime* v, i32 idx) {
    CString* r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->Elem(idx);
    } else if (v->GrowTo(idx, 0)) {
        r = v->Elem(idx);
    } else {
        v->Report(g_projActCache, 0xc);
        r = v->Scratch();
    }
    CString* slot = v->Slots();
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

// (the act-table side of the same lever is zDArray<T>::ResolveEntryCallReport, called
// DIRECTLY on s_table below - routing it through a wrapper here costs enough inline
// budget that cl outlines the whole accessor.)

// ===========================================================================
// CInGameIcon::~CInGameIcon  (0x011d00)
// ===========================================================================
// The leaf adds no destructible members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. The empty body is enough.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CInGameIcon() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00011cd0, 0x1e, ??_GCInGameIcon@@UAEPAXI@Z)
RVA_COMPGEN(0x00011d00, 0x44, ??1CInGameIcon@@UAE@XZ)

// ===========================================================================
// CInGameText::~CInGameText  (0x011dc0)
// ===========================================================================
// The leaf adds no destructible members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. The empty body is enough.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CInGameText() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00011d90, 0x1e, ??_GCInGameText@@UAEPAXI@Z)
RVA_COMPGEN(0x00011dc0, 0x44, ??1CInGameText@@UAE@XZ)

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
//   - builds the glitter overlay sprite, then a HandleInput() gate either marks the
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
CInGameIcon::CInGameIcon(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    // --- CInGameIcon own-field zero-init (retail store order @0x95c00) ---
    m_driftPos.m_lo = 0;
    m_driftThresh.m_lo = 0;
    m_driftPos.m_hi = 0;
    m_driftThresh.m_hi = 0;
    m_peekTimer.m_lo = 0;
    m_peekWindow.m_lo = 0;
    m_peekTimer.m_hi = 0;
    m_peekWindow.m_hi = 0;

    // snap owner's screen pos to the 0x20 tile grid centre
    obj->m_screenX = (obj->m_screenX & ~0x1f) + 0x10;
    obj->m_screenY = (obj->m_screenY & ~0x1f) + 0x10;

    if (obj->m_sortKey != 0x17318) {
        obj->m_sortKey = 0x17318;
        obj->m_flags |= 0x20000;
    }

    // swap the aux bute node (save old into m_30) + seed the cycle geometry
    AnimWorkerObj* aux = m_objAux;
    m_prevAnimSetNode = aux->m_1c;
    aux->m_1c = ActFindId("A");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);

    m_38->m_flags |= 2;
    SetupSprite(0);

    // second zero batch (retail @0x95ca1)
    m_glitterSprite = 0;
    m_peekTimer.m_lo = 0;
    m_peekWindow.m_lo = 0;
    m_peekTimer.m_hi = 0;
    m_peekWindow.m_hi = 0;

    i32 glitter = 0;
    char* rec = static_cast<CWwdGameObjectA*>(obj)->m_194; // the handed obj IS the A-kind sprite
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
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            lvl->m_anchors[0].m_x = m_object->m_screenX;
            lvl->m_anchors[0].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ2") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 2;
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            lvl->m_anchors[1].m_x = m_object->m_screenX;
            lvl->m_anchors[1].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ3") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 3;
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            lvl->m_anchors[2].m_x = m_object->m_screenX;
            lvl->m_anchors[2].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ4") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 4;
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
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
        CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
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
        CWwdGameObjectA* fx = g_gameReg->m_world->m_childGroup->CreateSprite(
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

    if (HandleInput() == 0) {
        m_38->m_flags |= 0x10000;
        return;
    }

    // mark the owner's tile cell occupied (or clear the occupancy bit)
    i32 mv = m_object->m_188;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    i32 col = m_object->m_screenX >> 5;
    i32 row = m_object->m_screenY >> 5;
    if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
        && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
        i32* cell = &grid->m_rowInts[row][col * 7];
        cell[2] = mv;
        i32* cell0 = &grid->m_rowInts[row][col * 7];
        if (mv != 0) {
            cell0[0] |= 0x40000;
        } else {
            cell0[0] &= ~0x40000;
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
    CWwdGameObjectA* obj = m_object;
    i32 cmd = obj->m_124;
    CShadeTable* rec;
    if (cmd == 0x55) {
        i32 key = obj->m_114;
        i32 sub = obj->m_118;
        if (sub < 0x17 || sub > 0x20) {
            return 0;
        }
        i32 icon = g_gameReg->m_options[key].m_008;
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
    CWwdGameObjectA* o = m_object;
    o->m_drawActive = 1;
    o->m_drawFillCmd = 0xa;
    o->m_drawFillArg = rec;
    return 1;
}

RVA(0x00097880, 0x102)
void CInGameIcon::FireActivation(i32 id) {
    if (*CActRegPool<CInGameIcon>::s_table.ResolveEntry(id) != 0) {
        (this->*(*CActRegPool<CInGameIcon>::s_table.ResolveEntry(id)))();
    }
}

// ===========================================================================
// RegisterIconActions  (0x0979e0)
// ===========================================================================
// Register two icon-action handlers into CActRegPool<CInGameIcon>::s_table: key A -> 0x4023d3,
// key B -> 0x403c06. Each: bute-tree Find (Insert + cache the name into the
// scratch zDArray<CString> when absent, bump the counter), resolve the table
// slot for the key index, load the handler member-fn-ptr.
// ---------------------------------------------------------------------------
// Two-key registrar: cl5 spends its inline budget from the outside in, so only the
// SECOND key's name lookup expands the grow-fail report; the other three lookups keep
// it as the out-of-line zErrHandling::Report call.
// docs/patterns/act-registrar-report-outline-budget.md
RVA(0x000979e0, 0x2ac)
void RegisterIconActions() {
    i32 idxA = ActFindId("A");
    if (idxA == 0) {
        ActInsertId("A", g_typeCounter);
        idxA = g_typeCounter;
        CString* slot = ResolveNameSlotCallReport(&g_typeColl, g_typeCounter);
        *slot = "A";
        g_typeCounter++;
    }
    CActHandler* dslotA = CActRegPool<CInGameIcon>::s_table.ResolveEntryCallReport(idxA);
    *dslotA = static_cast<CActHandler>(&CInGameIcon::PeekCycle);

    i32 idxB = ActFindId("B");
    if (idxB == 0) {
        ActInsertId("B", g_typeCounter);
        idxB = g_typeCounter;
        CString* slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *slot = "B";
        g_typeCounter++;
    }
    CActHandler* dslotB = CActRegPool<CInGameIcon>::s_table.ResolveEntryCallReport(idxB);
    *dslotB = static_cast<CActHandler>(&CInGameIcon::Reposition);
}

RVA(0x00097de0, 0x102)
void CToyPeek::FireActivation(i32 id) {
    if (*CActRegPool<CToyPeek>::s_table.ResolveEntry(id) != 0) {
        (this->*(*CActRegPool<CToyPeek>::s_table.ResolveEntry(id)))();
    }
}

// ===========================================================================
// RegisterIconState  (0x097f40)
// ===========================================================================
// Register one icon-state handler into CActRegPool<CToyPeek>::s_table (key A -> 0x40370b):
// bute-tree Find (Insert + cache the name when absent, bump the counter),
// resolve the table slot for the key index, load the handler member-fn-ptr.
// ---------------------------------------------------------------------------
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
RVA(0x00097f40, 0x18d)
void RegisterIconState() {
    i32 idx = ActFindId("A");
    if (idx == 0) {
        ActInsertId("A", g_typeCounter);
        idx = g_typeCounter;
        CString* slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *slot = "A";
        g_typeCounter++;
    }
    CActHandler* dslot = CActRegPool<CToyPeek>::s_table.ResolveEntry(idx);
    *dslot = static_cast<CActHandler>(&CInGameIcon::RefreshCell);
}

RVA(0x00098340, 0x71)
i32 CInGameIcon::RefreshCell() {
    CWwdGameObjectA* obj = m_object;
    i32 tileY = obj->m_screenX >> 5;
    i32 tileX = (obj->m_screenY + 0x18) >> 5;
    i64 delta = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_driftPos.m_v;
    if (delta < m_driftThresh.m_v) {
        CMapMgr* grid = g_gameReg->m_tileGrid;
        i32 cell;
        if (static_cast<u32>(tileY) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tileX) < static_cast<u32>(grid->m_height)) {
            BrickzCell* row = grid->m_rows[tileX];
            cell = row[tileY].m_8;
        } else {
            cell = 0;
        }
        if (cell != 0) {
            return 0;
        }
    }
    CWwdGameObjectA* r = m_38;
    r->m_flags |= 0x10000;
    return 0;
}

// ===========================================================================
// CInGameIcon::PeekCycle  (0x0984b0)
// ===========================================================================
// Per-frame peek update: advance the +0x1a0 anim cursor, then dispatch on the icon's
// command id. For the 0x55 (cursor) command, if the icon's tile cell carries any
// action/occupancy flag (& 0x939 or & 2) it clears that cell's occupancy and flags the
// +0x38 object dirty. For the 0x13/0x1e (peek) commands, once the peek timer
// ({m_peekTimer} vs {m_peekWindow}) elapses it rolls a random pickup sprite (the inline LCG rand()%17
// -> GetSel), publishes it into the bound object's draw fields, and re-arms the timer
// ({m_peekWindow}=0xfa, {m_peekTimer}=g_frameTime). Returns 0.
//
// @early-stop
// 88.2% - block topology is IDENTICAL (22 vs 22, every edge matching). Two of the five
// residual instructions are gone now that the tile cell is spelled as the BrickzCell it
// is (retail scales x*28 into a byte offset ONCE and uses it as [row+off+8] then
// [row+off], i.e. struct indexing - it emits the memory RMW `and [eax],0xfffbffff` by
// itself). The remaining FOUR are one cause: cl CSEs `grid->m_width` out of the two
// bounds checks into a FIFTH callee-saved register, so the prologue gains `push ebp`,
// both epilogues a `pop ebp`, and the extra live value re-colors everything downstream.
// Retail re-reads `[edx+0xc]`/`[edx+0x10]` at BOTH checks. TESTED and not steerable -
// five spellings of the second gate all reproduce the CSE: operand swap (87.2, worse),
// De Morgan, dropping the redundant u32 cast, hoisting the clear into an inline helper,
// and hoisting BOTH gates into inline helpers (all 88.2 exactly). With no store between
// the two loads the CSE is legal and cl always takes it.
RVA(0x000984b0, 0x186)
i32 CInGameIcon::PeekCycle() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CWwdGameObjectA* obj = m_object;
    i32 cmd = obj->m_124;
    if (cmd == 0x55) {
        CGruntzMgr* reg = g_gameReg;
        i32 tileY = obj->m_screenY >> 5;
        CMapMgr* grid = reg->m_tileGrid;
        i32 tileX = obj->m_screenX >> 5;
        i32 cell;
        if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {
            cell = grid->m_rows[tileY][tileX].m_0;
        } else {
            cell = 1;
        }
        if ((cell & 0x939) != 0 || (cell & 2) != 0) {
            if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {
                BrickzCell* row0 = grid->m_rows[tileY];
                row0[tileX].m_8 = 0;
                BrickzCell* row1 = grid->m_rows[tileY];
                row1[tileX].m_0 &= ~0x40000;
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
    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_peekTimer.m_v >= m_peekWindow.m_v) {
        u32 x;
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            x = ::timeGetTime();
        } else {
            x = g_randSeed;
        }
        g_randSeed = x * 214013 + 2531011;
        CShadeTable* rec = g_gameReg->m_spriteFactory->GetSel(
            ((static_cast<i32>(g_randSeed) >> 16) & 0x7fff) % 0x11,
            0
        );
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 0xa;
        o->m_drawFillArg = rec;
        m_peekWindow.m_lo = 0xfa;
        m_peekWindow.m_hi = 0;
        m_peekTimer.m_lo = g_frameTime;
        m_peekTimer.m_hi = 0;
    }
    return 0;
}

static inline void ClearTileBit(CGruntzMgr* reg, CGameObject* owner) {
    CMapMgr* grid = reg->m_tileGrid;
    i32 tileX = owner->m_screenY >> 5;
    i32 tileY = owner->m_screenX >> 5;
    if (static_cast<u32>(tileY) < static_cast<u32>(grid->m_width)
        && static_cast<u32>(tileX) < static_cast<u32>(grid->m_height)) {
        // the walk is transposed here: row = tileX, cell = tileY (7 ints per cell)
        i32 cellInt = tileY * 8 - tileY;
        i32* cell0 = grid->m_rowInts[tileX];
        cell0[cellInt + 2] = 0;
        i32* cell1 = grid->m_rowInts[tileX];
        cell1[cellInt] &= ~0x40000;
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
// 69.1% - re-audited against the disasm. The residual is ONE cause and it is the
// documented ONE/ZERO-register pinning (docs/patterns/zero-register-pinning.md, which
// proves it a coin-flip on a structurally identical twin): retail materialises the
// constant 1 in ebx up front and spends it BOTH on the reject gate (`cmp eax,ebx` for
// `m_134 == 1`) and on `matchActive = 1` (`mov [esp+0x14],ebx`), then `xor ebx,ebx`
// re-uses the same register as the zero. cl emits `mov edx,1` / immediates instead, and
// the freed/occupied register cascades through the whole 780-byte body (it also lets
// retail schedule `push edi` after the first compare and the g_gameReg load before the
// pushes). TESTED: rewriting the reject as a positive gate wrapping the whole body -
// the lever that took CInGameText::Update 79.5 -> 93.8 - is byte-IDENTICAL here (cl
// canonicalises the negation), because this reject is a bare `return 0` with no side
// effects for cl to have to place. Left in the literal `if (A && B && C) return 0;`
// form, which is what retail's branch structure spells.
RVA(0x000986b0, 0x30c)
i32 CInGameIcon::PlaceAt(i32 arg0, i32 arg1) {
    CGruntzMgr* reg = g_gameReg;
    if (reg->m_134 == 1 && arg0 != g_curPlayer && m_object->m_124 != 0x55) {
        return 0;
    }
    CWwdGameObjectA* obj = m_object;
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
        CGrunt* cell = reg->m_cmdGrid->m_grid[idx];
        i32 ok;
        if (cell == 0 || cell->m_entranceCommitted == 0) {
            ok = 0;
        } else if (matchActive) {
            ok = cell->LoadPickupSprites(param, flag, 0, sub, 0);
        } else {
            ok = cell->LoadGruntTypeTable(param, flag, sub, 0);
        }
        reg = g_gameReg;
        if (ok == 0) {
            return 0;
        }
        if (m_cue != 0) {
            CWwdGameObjectA* o = m_object;
            if (o->m_screenX < reg->m_viewBounds.right && o->m_screenX >= reg->m_viewBounds.left
                && o->m_screenY < reg->m_viewBounds.bottom
                && o->m_screenY >= reg->m_viewBounds.top) {
                // The receiver IS m_cue. The ex-"stale ecx / no receiver load" reading
                // was wrong: retail 0x986b0 loads `mov ecx,[esi+0x54]` for the null test
                // above and NOTHING between there and the call touches ecx, so the
                // thiscall simply reuses it. No cast, no tag-global stand-in.
                m_cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                reg = g_gameReg;
            }
        }
        ClearTileBit(reg, m_object);
        CWwdGameObjectA* r = m_38;
        r->m_flags |= 0x10000;
        return 1;
    }

    // ---- full place path (cmd != 0x55) ----
    i32 sub = obj->m_130;
    i32 cmd = obj->m_124;
    i32 idx = arg0 * 15 + arg1;
    CGrunt* cell = reg->m_cmdGrid->m_grid[idx];
    i32 ok;
    if (cell == 0 || cell->m_entranceCommitted == 0) {
        ok = 0;
    } else {
        ok = cell->LoadPickupSprites(cmd, 0, 0, sub, 1);
    }
    reg = g_gameReg;
    if (ok == 0) {
        return 0;
    }
    if (cmd == 0x14) {
        CGrunt* placed = reg->m_cmdGrid->m_grid[idx];
        if (placed != 0) {
            placed->m_38c = m_object->m_placeMode;
            reg = g_gameReg;
        }
    }
    if (m_cue != 0) {
        CWwdGameObjectA* o = m_object;
        if (o->m_screenX < reg->m_viewBounds.right && o->m_screenX >= reg->m_viewBounds.left
            && o->m_screenY < reg->m_viewBounds.bottom && o->m_screenY >= reg->m_viewBounds.top) {
            // Same as the sibling site above: ecx is the m_cue the null test loaded.
            m_cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
            reg = g_gameReg;
        }
    }
    ClearTileBit(reg, m_object);
    CWwdGameObjectA* owner = m_38;
    if (owner->m_120 > 0) {
        owner->m_stateFlags |= 1;
        AnimWorkerObj* aux = m_objAux;
        m_prevAnimSetNode = aux->m_1c;
        aux->m_1c = ActFindId("B");
        owner = m_38;
        m_driftPos.m_lo = owner->m_120;
        m_driftPos.m_hi = 0;
        m_driftThresh.m_lo = g_frameTime;
        m_driftThresh.m_hi = 0;
        return 1;
    }
    CWwdGameObjectA* rend = m_glitterSprite;
    if (rend != 0) {
        rend->m_flags |= 0x10000;
        m_glitterSprite = 0;
    }
    CWwdGameObjectA* r = m_38;
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
// 97.4% - re-audited instruction by instruction: everything except ONE pair of
// instructions is pure register RENAMING (ebx<->esi, ecx<->edx, edi<->ebx), i.e. the
// same value in a different callee-saved home. The single real difference is in the
// occupancy clear: retail materialises the cell address first (`add eax,ecx`) then
// `mov ecx,[eax]`, while cl folds the add into the addressing mode (`mov ecx,[eax+edx]`)
// and adds afterwards. Same operands, same count, different order.
// The typed-BrickzCell spelling that took PeekCycle 83.7 -> 88.2 goes the OTHER way here
// (measured: read block 97.4->96.3 alone, clear block -1.2, the m_188 publish block -7.0,
// all three -8.2), which is exactly why CMapMgr carries the m_rows/m_rowInts union:
// retail walks this band both ways and this function is on the int-walk side.
// Second real difference, found by jcc_sieve TOPOLOGY 2026-07-28 and worth ONE byte: our
// `&&` merges both false arms onto the end block, where retail CHAINS them - its first
// `je` lands on the second `test eax,eax` and lets the already-zero call result serve as
// the null. All three spellings that reproduce that chain cost more than the byte they buy:
// `CGameObject* hit = 0; if (Lookup(..)) hit = ..;` promotes hit to ebp (92.83), and both
// the explicit if/else and the ternary make cl emit a branchless `neg eax`/sbb select
// (93.26 / 93.97). Measured 2026-07-28; the `&&` stays.
RVA(0x00098a90, 0x18d)
i32 CInGameIcon::Reposition() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    i64 delta = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_driftPos.m_v;
    if (delta >= m_driftThresh.m_v) {
        CWwdGameObjectA* r = m_38;
        r->m_stateFlags &= ~1;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("A");

        CGruntzMgr* reg = g_gameReg;
        CWwdGameObjectA* obj = m_object;
        i32 tileX = obj->m_screenX >> 5;
        i32 tileY = obj->m_screenY >> 5;
        CMapMgr* grid = reg->m_tileGrid;
        i32 cellVal;
        if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {
            cellVal = grid->m_rowInts[tileY][tileX * 7 + 2];
        } else {
            cellVal = 0;
        }
        if (cellVal != 0) {
            void* found = 0;
            if (MapLookupById(reg->m_world->m_childGroup->m_map48, cellVal, found) && found != 0) {
                (static_cast<CGameObject*>(found))->m_flags |= 0x10000;
            }
        }
        reg = g_gameReg;
        grid = reg->m_tileGrid;
        if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {
            grid->m_rowInts[tileY][tileX * 7 + 2] = 0;
            grid->m_rowInts[tileY][tileX * 7] &= ~0x40000;
        }
        obj = m_object;
        grid = g_gameReg->m_tileGrid;
        i32 tileX2 = obj->m_screenX >> 5;
        i32 tileY2 = obj->m_screenY >> 5;
        i32 mv = obj->m_188;
        if (static_cast<u32>(tileX2) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tileY2) < static_cast<u32>(grid->m_height)) {
            grid->m_rowInts[tileY2][tileX2 * 7 + 2] = mv;
            if (mv != 0) {
                grid->m_rowInts[tileY2][tileX2 * 7] |= 0x40000;
            } else {
                grid->m_rowInts[tileY2][tileX2 * 7] &= ~0x40000;
            }
        }
    }
    return 0;
}

// ===========================================================================
// CInGameIcon::Serialize  (0x098c90)
// ===========================================================================
// The CArchive load/store of the icon state: guard on the archive, chain the
// shared CUserLogic::SerializeMove, then a `sub 4 / sub 3 / dec` running tag
// switch dispatching per-mode CArchive Read/Write (vtable +0x2c write / +0x30
// read) of the +0x34..+0x78 fields, with inline strlen/strcpy CString round-trips
// (repne scas / rep movs), a g_serialCounter bump, and two registry CMap lookups
// (0x1b8438 / 0x1b8760) re-binding the +0x40/+0x54 ids.
//
// The span is 0x382 (898 B), NOT Ghidra's 0x31f (799): the body's branches reach 0x99000
// and it runs on to a final `ret 0x10` at 0x9900f with nop padding to 0x99020. With the
// short span the delinked target was truncated mid-instruction, which is what the old
// stub called blocker (1).
//
// The opening block is CWapX::Chain (0x8c00) EXPANDED IN PLACE - same key buffer, same
// m_blob, same m_34/m_38/m_3c seeds, same `m_3c->m_ownerCtx->m_animRegistry->m_10`
// lookup - not a call to it. Retail duplicated it here rather than chaining, so it is
// spelled out below; making Chain inline instead would reshape its real callers
// (CAniCycle::SerializeMove @0xf470 and friends), which `call 0x8c00` it.
// @early-stop
// 89.97%. Everything up to and including the two timer pairs is byte-exact. The residue
// is one stack-allocator choice plus its fallout: retail puts the CHAIN-half key buffer
// at the HIGH slot (esp+0x9c) and the tail's at the LOW one (esp+0x1c); cl gives us the
// mirror, so roughly ten `lea`/`push` sites differ only in their displacement
// (disp8 vs disp32) and the CString temp lands 4 bytes off. Not source-steerable:
// measured both declaration orders and a point-of-use declaration for the second buffer
// - all three produce the identical layout, because the two arrays are the SAME SIZE and
// cl orders equal-size arrays by which has its address taken first, which the algorithm
// fixes. (Declaration order DOES control it when the sizes differ - see
// CPlay::DrawDebugStatsFull @0xcf0a0, where buf/0x200 and two 0x40 scratches land in
// declaration order.) The last small item is `mov edi,[eax]` vs `mov edi,[esp+0x10]`
// after each Key/FindKeyOfValue: retail reads the returned CString's m_pchData straight
// off the return-buffer pointer. Consuming the temp in place does emit that, but gives
// the two call sites separate 4-byte slots where retail shares one, costing more frame
// than it saves (measured: 87.47% vs 87.13% at the time).
RVA(0x00098c90, 0x382)
i32 CInGameIcon::SerializeMove(CFileMemBase* ar, i32 mode, i32 a3, CGameObject* obj) {
    // TWO 0x80 key buffers, not one - retail's frame is 0x10c and holds both (the
    // inlined-Chain half formats through the upper one at esp+0x9c, the icon tail
    // through the lower at esp+0x1c). Sharing one costs 0x80 of frame and shifts every
    // local. Every mode test below is a SWITCH, not an if/else-if ladder: retail emits
    // `cmp esi,4; je <store>; cmp esi,7; jne <skip>` - the 4-arm compared first, the
    // 7-arm laid out first - which is what cl gives a switch whose cases read 7 then 4.
    char chainName[0x80];

    if (ar == 0) {
        return 0;
    }
    if (CUserLogic::SerializeMove(ar, mode, a3, obj) == 0) {
        return 0;
    }

    // --- the inlined CWapX::Chain half ---
    switch (mode) {
        case 7: {
            ar->Read(chainName, 0x80);
            ar->Read(m_blob, 0x10);
            m_34 = obj;
            m_38 = static_cast<CWwdGameObjectA*>(obj);
            m_3c = obj->m_7c;
            if (strlen(chainName) == 0) {
                m_value = 0;
            } else {
                void* val = 0;
                m_3c->m_ownerCtx->m_animRegistry->m_10.Lookup(chainName, val);
                m_value = static_cast<CAniElement*>(val);
            }
            break;
        }
        case 4: {
            memset(chainName, 0, sizeof(chainName));
            if (m_value != 0) {
                CString nm = m_3c->m_ownerCtx->m_animRegistry->KeyOfValue(m_value);
                strcpy(chainName, static_cast<const char*>(nm));
            }
            ar->Write(chainName, 0x80);
            ar->Write(m_blob, 0x10);
            break;
        }
    }

    // --- the two 64-bit timer pairs, each walked by ONE advancing cursor ---
    // Retail hoists a single `lea edi,[this+N]` ABOVE the mode compare and steps it with
    // `add edi,8`; two separate `&member` expressions emit two leas instead. The cursor is
    // a Clock64* now that the members are typed: sizeof(Clock64) is 8, so `++` IS retail's
    // `add edi,8` (this was an i32* stepping += 2 while the halves were untyped).
    Clock64* drift = &m_driftPos;
    switch (mode) {
        case 7:
            ar->Read(drift, 8);
            drift++;
            ar->Read(drift, 8);
            break;
        case 4:
            ar->Write(drift, 8);
            drift++;
            ar->Write(drift, 8);
            break;
    }
    Clock64* idle = &m_peekTimer;
    switch (mode) {
        case 7:
            ar->Read(idle, 8);
            idle++;
            ar->Read(idle, 8);
            break;
        case 4:
            ar->Write(idle, 8);
            idle++;
            ar->Write(idle, 8);
            break;
    }

    // --- the icon's own tail: the sound cue by name + the glitter sprite by id ---
    char tailName[0x80];
    switch (mode) {
        case 4: {
            memset(tailName, 0, sizeof(tailName));
            if (m_cue != 0) {
                CString nm = m_3c->m_ownerCtx->m_soundRegistry->FindKeyOfValue(m_cue);
                strcpy(tailName, static_cast<const char*>(nm));
            }
            ar->Write(tailName, 0x80);
            g_serialCounter++;
            i32 id = 0;
            if (m_glitterSprite != 0) {
                id = m_glitterSprite->m_188;
            }
            ar->Write(&id, 4);
            break;
        }
        case 7: {
            ar->Read(tailName, 0x80);
            // `== 0` here, unlike CWarlord::SerializeMove's eleven anim blocks which want
            // `!= 0` - measured both ways on both functions; the arm cl puts on the
            // fallthrough differs per function and only the diff says which.
            if (strlen(tailName) == 0) {
                m_cue = 0;
            } else {
                void* val = 0;
                m_3c->m_ownerCtx->m_soundRegistry->m_10.Lookup(tailName, val);
                m_cue = static_cast<LeafCue*>(val);
            }
            g_serialCounter++;
            i32 id = 0;
            ar->Read(&id, 4);
            void* found = 0;
            CWwdGameObjectA* sprite = 0;
            if (MapLookupById(m_3c->m_ownerCtx->m_childGroup->m_map48, id, found) != 0 && found != 0
                && static_cast<CGameObject*>(found)->GetClassId() == CLASSID_SERIALREF) {
                sprite = static_cast<CWwdGameObjectA*>(found);
            }
            m_glitterSprite = sprite;
            if (sprite != 0) {
                break;
            }
            // A missing sprite is only tolerated when no id was stored at all.
            if (id != 0) {
                return 0;
            }
            break;
        }
        case 8:
            if (HandleInput() == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

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
CInGameText::CInGameText(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    if (g_gameReg->m_134 == 2) {
        m_38->m_flags |= 0x10000;
        return;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_value = m_38->m_1a0.m_14;
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
    if (m_object->m_sortKey != 0x17318) {
        m_object->m_sortKey = 0x17318;
        m_object->m_flags |= 0x20000;
    }
    m_cachedAreaId = -1;
    m_cachedSubId = -1;
}

RVA(0x00099460, 0x102)
void CInGameText::FireActivation(i32 idx) {
    if (*CActRegPool<CInGameText>::s_table.ResolveEntry(idx) != 0) {
        CActHandler fn = *CActRegPool<CInGameText>::s_table.ResolveEntry(idx);
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
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
RVA(0x000995c0, 0x18d)
void RegisterTextLogic() {
    i32 idx = ActFindId("A");
    if (idx == 0) {
        ActInsertId("A", g_typeCounter);
        idx = g_typeCounter;
        CString* slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *slot = "A";
        g_typeCounter++;
    }
    CActHandler* dslot = CActRegPool<CInGameText>::s_table.ResolveEntry(idx);
    *dslot = static_cast<CActHandler>(&CInGameText::Update);
}

RVA(0x00099a30, 0xaa)
i32 CInGameText::SerializeMove(CFileMemBase* ar, i32 tag, i32 a, CGameObject* b) {
    if (ar == 0) {
        return 0;
    }
    if (CUserLogic::SerializeMove(ar, tag, a, b) == 0) {
        return 0;
    }
    if (Chain(ar, tag, a, b) == 0) {
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

// @early-stop
// 64.2% on 54 bytes - logic and structure are identical; the residual is the documented
// zero-register pinning (docs/patterns/zero-register-pinning.md). Retail materialises 0
// in ecx and reuses it three ways (`cmp eax,ecx` for the null test, the out-param slot
// store, and the null-path `m_cue = 0`), where cl emits `test ecx,ecx` + an immediate
// store; and retail reads the parameter from [esp+4] BEFORE `push esi`. Three spellings
// measured: collapsing cue+found into ONE variable to share the zero makes it WORSE
// (56.4, and 55.6 with the g_gameReg hoist), which is the coin-flip the pattern doc
// describes. Hoisting g_gameReg into its own local ahead of the out-param zero is the
// one lever that did move it (63.4 -> 64.2) and is kept.
RVA(0x00099b10, 0x36)
void CInGameIcon::SetupSprite(const char* category) {
    LeafCue* cue = 0;
    if (category != 0) {
        // the singleton is read BEFORE the out-param is zeroed - retail's
        // `mov edx,[g_gameReg]` precedes `mov [esp+8],ecx` - so it needs its own local
        CGruntzMgr* reg = g_gameReg;
        void* found = 0; // CMapStringToPtr's value slot (Lookup 0x1b8438 takes void*&)
        reg->m_world->m_soundRegistry->m_10.Lookup(category, found);
        cue = static_cast<LeafCue*>(found);
    }
    m_cue = cue;
}
