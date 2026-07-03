// CInGameIcon.cpp - the in-game HUD/cursor icon (a CUserLogic-derived game
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
#include <Gruntz/CInGameIcon.h>

#include <rva.h>

#include <string.h>       // inline strcmp: the ctor's icon-name dispatch chain
#include <Bute/ButeMgr.h> // CButeTree (the bute store Setup queries)
#include <Wap32/ZVec.h>   // zDArray (the command-dispatch tables)
#include <Globals.h>

// The global bute store the icon Setup queries (g_buteTree.Find). Owned by
// another TU; declared extern so `ecx=&g_buteTree; call Find` reloc-masks.
extern CButeTree g_buteTree;

// The bute manager singleton the builder queries for the WarpStone target
// (g_buteMgr.GetInt) - declared in <Gruntz/UserLogic.h> (pulled via the header).

// EngFmt (0x1b2cf5): __cdecl variadic sprintf-into-CString - the WarpStone path
// formats "Level%i" and "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i".
void EngFmt(CString* out, const char* fmt, ...);

// The sprite/animation factory reached as g_gameReg->m_world->m_8 (its +0x8 field);
// CreateSprite (0x1597b0, __thiscall) builds a "SimpleAnimation" glitter sprite.
struct IconSpriteFactory {
    CGameObject*
    CreateSprite(i32 a0, i32 x, i32 y, i32 id, const char* desc, i32 flags); // 0x1597b0
};

// The current game-state (g_gameReg->m_curState) viewed by the icon-setup path as
// the per-level warp holder: +0x1c the level number, +0x384.. four (x,y) WarpStone
// target pairs the tool-icon dispatch stores. (Per-mode downcast of CState*.)
struct IconLevelState {
    char m_pad00[0x1c];
    i32 m_levelNum; // +0x1c  the current level number
    char m_pad20[0x384 - 0x20];
    i32 m_warpTarget[8]; // +0x384  four (x,y) WarpStone target pairs
};
SIZE_UNKNOWN(IconLevelState);

// ===========================================================================
// The two file-scope command-dispatch tables (zDArray<member-fn-ptr>) the icon
// registration thunks construct + populate. The ctor (zDArray::Construct, the
// 0x408710 method: stride-4 base init + the 0x5e70fc vptr stamp) reloc-masks.
// Their static-init thunks below build each table over the index band [0x7d0,
// 0x7da].
// ===========================================================================
struct LogicFnTable : public zDArray {
    LogicFnTable* Construct(i32 lo, i32 hi); // 0x408710 (zDArray<T> ctor, returns this)
};

DATA(0x002458b0)
extern LogicFnTable g_iconActionTable; // 0x6458b0
DATA(0x00245928)
extern LogicFnTable g_iconStateTable; // 0x645928

// --- the shared registration infrastructure (mirror of CInGameText's) --------
// The zvec error globals the inlined accessors touch on a bounds miss.
extern void* zErr_CaptureRetB(); // 0x16d990

DATA(0x0021aea8)
extern i32 g_iconRegCounter; // 0x61aea8  (running registration index)

// The scratch name-vec (zDArray<CString> @ 0x6bf650): the registration path
// IndexToPtr's it (growing + CString-constructing fresh slots) to stash the key.
struct NameVec : public zDArray {};
DATA(0x002bf650)
extern NameVec g_buteNameVec; // 0x6bf650

// The two registration key strings (.data constants).
DATA(0x0020a454)
extern const char s_iconKeyA[]; // 0x60a454
DATA(0x0020d1bc)
extern const char s_iconKeyB[]; // 0x60d1bc

// The handler member functions loaded into the dispatch slots (FUN_004023d3 /
// 0x403c06 into the action table; 0x40370b into the state table). Referenced by
// address so the stored DIR32 operand reloc-masks.
extern i32 IconAction_4023d3();
extern i32 IconAction_403c06();
extern i32 IconState_40370b();

// The zDArray<CString> accessor inlined WITH the per-slot CString-ctor fixup over
// the freshly-grown region (the zDArray::IndexToPtr body).
static inline i32 ResolveNameSlot(NameVec* v, i32 idx) {
    i32 r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else if (v->GrowTo(idx, 0)) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else {
        i32 sentinel = g_zvecErrSentinel;
        g_zvecErrToken = zErr_CaptureRetB();
        v->m_err->Error(v, sentinel, 0xc);
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
    i32 sentinel = g_zvecErrSentinel;
    g_zvecErrToken = zErr_CaptureRetB();
    v->m_err->Error(v, sentinel, 0xc);
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
    CGameObjAux* aux = m_objAux;
    m_prevAnimSetNode = aux->m_1c;
    aux->m_1c = g_buteTree.Find(s_iconKeyA);
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
    void* rec = *(void**)((char*)obj + 0x194);
    if (rec != 0) {
        CString name;
        name = (const char*)rec + 0x24;

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
            IconLevelState* lvl = (IconLevelState*)g_gameReg->m_curState;
            lvl->m_warpTarget[0] = m_object->m_screenX;
            lvl->m_warpTarget[1] = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ2") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 2;
            IconLevelState* lvl = (IconLevelState*)g_gameReg->m_curState;
            lvl->m_warpTarget[2] = m_object->m_screenX;
            lvl->m_warpTarget[3] = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ3") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 3;
            IconLevelState* lvl = (IconLevelState*)g_gameReg->m_curState;
            lvl->m_warpTarget[4] = m_object->m_screenX;
            lvl->m_warpTarget[5] = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ4") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 4;
            IconLevelState* lvl = (IconLevelState*)g_gameReg->m_curState;
            lvl->m_warpTarget[6] = m_object->m_screenX;
            lvl->m_warpTarget[7] = m_object->m_screenY;
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
        IconLevelState* lvl = (IconLevelState*)g_gameReg->m_curState;
        CString levelStr;
        EngFmt(&levelStr, "Level%i", lvl->m_levelNum);
        CString warpName;
        i32 target = g_buteMgr.GetInt("WarpStone", levelStr);
        EngFmt(&warpName, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i", target);
        m_object->ApplyName(warpName);
        m_object->m_placeMode = target;
    }

    // glitter overlay sprite for the powerup / curse groups
    if (glitter != 0) {
        IconSpriteFactory* fac = (IconSpriteFactory*)g_gameReg->m_world->m_8;
        CGameObject* fx = fac->CreateSprite(
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
        rec = ((CIconFactory*)g_gameReg->m_74)->GetByIndex(icon, 0);
        if (rec == 0) {
            rec = ((CIconFactory*)g_gameReg->m_74)->GetByIndex(1, 0);
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
        rec = ((CIconFactory*)g_gameReg->m_74)->GetByIndex(icon, 0);
        if (rec == 0) {
            rec = ((CIconFactory*)g_gameReg->m_74)->GetByIndex(1, 0);
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
// InitIconActionTable  (0x097800)
// ===========================================================================
// File-scope static-init thunk: construct the action dispatch table over the
// index band [0x7d0, 0x7da].
RVA(0x00097800, 0x15)
void InitIconActionTable() {
    g_iconActionTable.Construct(0x7d0, 0x7da);
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
    i32 idxA = (i32)g_buteTree.Find(s_iconKeyA);
    if (idxA == 0) {
        g_buteTree.Insert(s_iconKeyA, (void*)g_iconRegCounter);
        i32 slot = ResolveNameSlot(&g_buteNameVec, g_iconRegCounter);
        *(CString*)slot = s_iconKeyA;
        g_iconRegCounter++;
    }
    i32 dslotA = ResolveSlot(&g_iconActionTable, idxA);
    *(void**)dslotA = (void*)&IconAction_4023d3;

    i32 idxB = (i32)g_buteTree.Find(s_iconKeyB);
    if (idxB == 0) {
        g_buteTree.Insert(s_iconKeyB, (void*)g_iconRegCounter);
        i32 slot = ResolveNameSlot(&g_buteNameVec, g_iconRegCounter);
        *(CString*)slot = s_iconKeyB;
        g_iconRegCounter++;
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
    g_iconStateTable.Construct(0x7d0, 0x7da);
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
    i32 idx = (i32)g_buteTree.Find(s_iconKeyA);
    if (idx == 0) {
        g_buteTree.Insert(s_iconKeyA, (void*)g_iconRegCounter);
        i32 slot = ResolveNameSlot(&g_buteNameVec, g_iconRegCounter);
        *(CString*)slot = s_iconKeyA;
        g_iconRegCounter++;
    }
    i32 dslot = ResolveSlot(&g_iconStateTable, idx);
    *(void**)dslot = (void*)&IconState_40370b;
}

// ===========================================================================
// CInGameIcon::RefreshCell  (0x098340)
// ===========================================================================
// If the icon's tracked position has drifted at least g_iconDefault past the
// owning object's stored position (a 64-bit signed compare of {m_driftPosHi:m_driftPos}
// against {m_driftThreshHi:m_driftThresh}), OR the owning object's tile cell is empty/off-grid,
// flag the +0x38 render object dirty (|= 0x10000). Returns 0.
RVA(0x00098340, 0x71)
i32 CInGameIcon::RefreshCell() {
    CGameObject* obj = m_object;
    i32 tileY = obj->m_screenX >> 5;
    i32 tileX = (obj->m_screenY + 0x18) >> 5;
    i64 delta = (i64)(u32)g_iconDefault - *(i64*)&m_driftPos;
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
        CIconRecord* cell = ((CIconRecord**)((char*)reg->m_68 + 0x1c))[idx];
        i32 ok;
        if (cell == 0 || cell->m_1fc == 0) {
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
        if (m_cmapId != 0) {
            CGameObject* o = m_object;
            if (o->m_screenX < reg->m_viewOriginR && o->m_screenX >= reg->m_viewOriginL
                && o->m_screenY < reg->m_viewOriginB && o->m_screenY >= reg->m_viewOriginT) {
                Eng_PostCmd(g_inputCtx, 0, 0, 0);
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
    CIconRecord* cell = ((CIconRecord**)((char*)reg->m_68 + 0x1c))[idx];
    i32 ok;
    if (cell == 0 || cell->m_1fc == 0) {
        ok = 0;
    } else {
        ok = cell->LoadPickupSprites(cmd, 0, 0, sub, 1);
    }
    reg = g_gameReg;
    if (ok == 0) {
        return 0;
    }
    if (cmd == 0x14) {
        CIconRecord* placed = ((CIconRecord**)((char*)reg->m_68 + 0x1c))[idx];
        if (placed != 0) {
            placed->m_38c = m_object->m_placeMode;
            reg = g_gameReg;
        }
    }
    if (m_cmapId != 0) {
        CGameObject* o = m_object;
        if (o->m_screenX < reg->m_viewOriginR && o->m_screenX >= reg->m_viewOriginL
            && o->m_screenY < reg->m_viewOriginB && o->m_screenY >= reg->m_viewOriginT) {
            Eng_PostCmd(g_inputCtx, 0, 0, 0);
            reg = g_gameReg;
        }
    }
    ClearTileBit(reg, m_object);
    CGameObject* owner = m_38;
    if (owner->m_120 > 0) {
        owner->m_stateFlags |= 1;
        CGameObjAux* aux = m_objAux;
        m_prevAnimSetNode = aux->m_1c;
        aux->m_1c = g_buteTree.Find(g_iconBute);
        owner = m_38;
        m_driftPos = owner->m_120;
        m_driftPosHi = 0;
        m_driftThresh = g_iconDefault;
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
// CInGameIcon::SetField54  (0x099b10)
// ===========================================================================
// When v != 0, look it up in the registry's CMap (g_gameReg->m_world->m_28, Lookup
// at +0x10) into a local, then store the located value (or 0) into +0x54.
RVA(0x00099b10, 0x36)
void CInGameIcon::SetField54(i32 v) {
    void* found = 0;
    if (v != 0) {
        found = 0;
        ((CGameRegMapHolder*)g_gameReg->m_world)->m_28->m_10map.Lookup((void*)v, &found);
    }
    m_cmapId = (i32)found;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CGameRegMapHolder);
SIZE_UNKNOWN(CIconFactory);
SIZE_UNKNOWN(CIconMap);
SIZE_UNKNOWN(CIconMapHolder);
SIZE_UNKNOWN(CIconRecord);
SIZE_UNKNOWN(CInGameIcon);
SIZE_UNKNOWN(IconSpriteFactory);
SIZE_UNKNOWN(LogicFnTable);
SIZE_UNKNOWN(NameVec);
