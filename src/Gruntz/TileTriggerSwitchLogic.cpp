#include <string.h>            // memcpy -> the /Oi `rep movsd` in BuildSmall
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/SoundState.h> // g_sndEnabled/g_sndCueTag
#include <Io/FileMem.h>        // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Mfc.h>
#include <rva.h>

#include <Gruntz/GruntzMgr.h> // the REAL singleton class
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileTriggerContainer.h> // the owner container (m_owner; TtcNode/TtcHead)
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileGridCommand.h>
#include <Gruntz/TileActionEvent.h>
#include <Wwd/WwdFile.h> // CPlaneRender - the canonical plane
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/Grunt.h>

// TileActionEvent.cpp - the per-tile game-action event record (trace placeholder
// tomalla-108). Methods in ascending retail-RVA order. The record shape comes from
// <Gruntz/TileActionEvent.h>; the serializer is the shared CSerialArchive; the game
// registry singleton (g_gameReg) is modeled here with only the offsets these paths
// touch. All engine callees are reloc-masked (no body).
//
// BANKED (code byte-exact, 100% fuzzy): ResetFlag (0x112d80), SetActionCode
//   (0x112da0), MorphByTool (0x113420), Serialize (0x113f10), SerializeFields
//   (0x113f60). The big Process (0x112ee0) is a complete logical reconstruction
//   parked at the two-jump-table wall (@early-stop) for the final sweep.
// <Mfc.h> (not <Win32.h>): UserLogic.h pulls afx via ButeMgr.h/String.h, so the
// umbrella must be the MFC superset kept first (mfc-wall-is-breakable doctrine).
// ---------------------------------------------------------------------------
// The game registry singleton (?g_gameReg@@3PAUWwdGameRegZ@@A at VA 0x64556c).
// Only the offsets this cluster reaches are modeled; reloc-masked DIR32.
// ---------------------------------------------------------------------------

RVA(0x00110430, 0x1c)
CTileTriggerSwitchLogic::CTileTriggerSwitchLogic() {
    // vptr stamp is now IMPLICIT (real polymorphic class) - cl prepends
    // `mov [this], offset ??_7CTileTriggerSwitchLogic@@6B@`, exactly the retail
    // ctor's first instruction, replacing the manual struct stamp.
    for (i32 i = 0; i < 24; i++) {
        m_block[i] = 0;
    }
    m_initGate = 0;
}

RVA(0x00110460, 0x64)
i32 CTileTriggerSwitchLogic::BuildSmall(
    CTileTriggerContainer* owner,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    const i32* rect,
    i32 a7,
    i32 a8,
    i32 a9
) {
    if (m_initGate != 0) {
        return 0;
    }
    if (a2 == 4 && rect[0] == 0) {
        return 0;
    }
    memcpy(m_block, rect, sizeof(m_block));
    return Setup(owner, a2, a3, a4, a5, a7, a8, a9);
}

RVA(0x001104f0, 0x56)
i32 CTileTriggerSwitchLogic::Setup(
    CTileTriggerContainer* owner,
    i32 a1,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7
) {
    if (m_initGate) {
        return 0;
    }
    m_typeId = a1;
    m_08 = a2;
    m_key0c = a3;
    m_key1 = a4;
    m_owner = owner;
    m_18 = a6;
    m_28 = a7;
    m_1c = 0;
    m_linkGate = a5;
    m_initGate = 1;
    return 1;
}

RVA(0x001107f0, 0x1c)
CTileTriggerLogic::CTileTriggerLogic() {
    // m_block initialised before m_1c so the optimiser emits the rep stosl
    // first and reuses the zero register for the +0x1c store afterwards.
    for (i32 i = 0; i < 24; i++) {
        m_block[i] = 0;
    }
    m_initGate = 0;
}

RVA(0x00110820, 0x23)
i32 CTileTriggerLogic::FindIndexByKey(i32 key) {
    for (i32 i = 0; i < 24; i++) {
        if (m_block[i] == key) {
            return 1;
        }
    }
    return 0;
}

static i32 PbResolveCell(CGameLevel* level, i32 x, i32 y) {
    CLevelPlane* plane = level->m_mainPlane;
    if (x < 0) {
        x = 0;
    } else if (x >= plane->m_gridW) {
        x = plane->m_gridW - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= plane->m_gridH) {
        y = plane->m_gridH - 1;
    }
    plane = level->m_mainPlane;
    i32 cell = plane->m_tileGrid[plane->m_colOffsets[y] + x];
    if (cell == TILE_UNINIT || cell == TILE_CLEAR) {
        return 0;
    }
    // CObArray stores CObject*; the element cast is the devs' own (GameLevel.h).
    CTileImageSet* set = static_cast<CTileImageSet*>(level->m_imageSets[cell & 0xffff]);
    return set->GetCollisionAt(0, 0);
}

enum PyramidSpriteType {
    kSpriteTypeBase = 0xf,   // first tile-action sprite type (jump-table base)
    kOrangePyramidUp = 0x62, // case 0x53
    kBlackPyramidUp = 0x64,  // case 0x55
    kGreenPyramidUp = 0x66,  // case 0x57
    kPurplePyramidUp = 0x6a, // case 0x5b
};

// @early-stop
// /GX nested-jump-table megafunction wall (~7%). The grid-cell resolve, the PtInRect
// transition gate, the two CString sprite-key temps and the pyramid-color jump arms are
// reconstructed and match retail's logic. The full 3647-byte body - the 0x66-case jump
// table (RED/WHITE/CHECKPOINT pyramidz + the LEVEL_*BRIDGE* arms are still to write),
// the per-bridge inner grid-scan loops, and the descending /GX exception thread -
// is the documented wall shared by the sibling /GX megafunctions. Deferred to the final
// sweep (docs/patterns/jumptable-data-overlap.md; big-seh-fuzzy-desync.md).
// @interleaver CTileTriggerLogic::Tick emitted-in <boundary:
// BridgeMoveSprites.cpp LoadBridgeMove @0x110860 (before) + GruntzMgr2.cpp SetCellHeight
// @0x111ec0 (after)>. A /Gy COMDAT the linker scattered between two OTHER units - not
// this TU's body run.
RVA(0x00110c10, 0xe3f)
i32 CTileTriggerLogic::Tick() {
    CDDrawSurfaceMgr* world = g_gameReg->m_world; // ebx (spilled to [esp+0x24])
    CGameLevel* level = world->m_level;           // edx
    i32 transId = 0;                              // [esp+0x1c] transition logic handle

    // ---- resolve the source cell id at this trigger's tile (the switch key) ----
    i32 srcId = PbResolveCell(level, m_tileX, m_tileY); // [esp+0x18]

    // ---- the PtInRect transition gate (rect = the view rect at +0x13c) ----
    {
        i32 sy = (m_tileY << 5) + 0x10;
        i32 sx = (m_tileX << 5) + 0x10;
        POINT pt;
        pt.x = sx;
        pt.y = sy;
        if (!PtInRect(reinterpret_cast<const RECT*>(&g_gameReg->m_viewOriginL), pt) || srcId == 0x68
            || srcId == 0x67) {
            transId = 0;
        } else {
            CGameObject* trig =
                world->m_childGroup->CreateSprite(0, sx, sy, 0, "TileTriggerTransition", 0x40003);
            if (trig == 0) {
                return 0; // the pre-CString early exit (0x111140)
            }
            trig->m_7c->m_notify(trig);
            transId = reinterpret_cast<i32>(trig->m_7c->m_logic);
        }
    }

    // ---- the two CString sprite-key temps (real locals -> the /GX dtor states) ----
    CString upTemp;  // [esp+0x14] GAME_PYRAMIDUP/DOWN
    CString keyTemp; // [esp+0x10] GAME_<COLOR>PYRAMIDZ

    switch (static_cast<u32>((srcId - kSpriteTypeBase))) {
        case kGreenPyramidUp - kSpriteTypeBase: // 0x57  GREENPYRAMIDZ
            keyTemp = "GAME_GREENPYRAMIDZ";
            upTemp = (srcId == kGreenPyramidUp) ? "GAME_PYRAMIDUP" : "GAME_PYRAMIDDOWN";
            break;
        case kPurplePyramidUp - kSpriteTypeBase: // 0x5b  PURPLEPYRAMIDZ
            keyTemp = "GAME_PURPLEPYRAMIDZ";
            upTemp = (srcId == kPurplePyramidUp) ? "GAME_PYRAMIDUP" : "GAME_PYRAMIDDOWN";
            break;
        case kOrangePyramidUp - kSpriteTypeBase: // 0x53  ORANGEPYRAMIDZ
            keyTemp = "GAME_ORANGEPYRAMIDZ";
            upTemp = (srcId == kOrangePyramidUp) ? "GAME_PYRAMIDUP" : "GAME_PYRAMIDDOWN";
            break;
        case kBlackPyramidUp - kSpriteTypeBase: // 0x55  BLACKPYRAMIDZ
            keyTemp = "GAME_BLACKPYRAMIDZ";
            upTemp = (srcId == kBlackPyramidUp) ? "GAME_PYRAMIDUP" : "GAME_PYRAMIDDOWN";
            break;
        default:
            break;
    }
    static_cast<void>(transId); // consumed by the still-unreconstructed bridge arms

    return 0;
}

RVA(0x00111f10, 0x12)
CTileMultiTriggerSwitchLogic::CTileMultiTriggerSwitchLogic() {}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::VerifyBlockLinksB
// The FindChild(key, 3) variant of VerifyBlockLinks (byte-identical structure,
// different diagnostic codes 0x44d/0x44e and the slow-lookup kind arg 3 vs 8):
// gate on m_linkGate, find the owner's child that claims this object (FindIndexByKey),
// ack 0x44d on miss; then validate each nonzero block key resolves (FindChild
// (key, 3)) to a gated child, acking 0x44e on a lookup miss.
// ---------------------------------------------------------------------------
// @early-stop
// this-spill frame wall (~86%, same as VerifyBlockLinks 0x112c70): body
// byte-identical; retail reserves a `push ecx` stack local for `this` + reloads it
// to seed the `child` loop cursor, the recompile seeds it from a register. Dead seed
// value, non-steerable frame choice. See docs/patterns/this-spilled-to-local-for-loop-seed.md
// @interleaver CTileTriggerSwitchLogic::VerifyBlockLinksB emitted-in <boundary:
// GruntzMgr2.cpp SetCellHeight @0x111ec0 (before) + GroupOps.cpp Broadcast @0x112080
// (after)>. A /Gy first-use COMDAT the linker scattered between two OTHER units.
RVA(0x00111f40, 0xc4)
i32 CTileTriggerSwitchLogic::VerifyBlockLinksB() {
    if (m_linkGate == 0) {
        return 0;
    }
    // walk the owner CONTAINER's m_list1 (head @ container+0x20) - the 0x9c
    // CTileTriggerLogic children live there.
    TtcNode* node = TtcHead(m_owner->m_list1);
    i32 found = 0;
    CTileTriggerLogic* child = 0;
    while (node != 0) {
        if (found != 0) {
            break;
        }
        TtcNode* cur = node;
        node = node->m_next;
        child = static_cast<CTileTriggerLogic*>(cur->m_data);
        if (child != 0 && child->FindIndexByKey(m_key1) != 0) {
            found = 1;
        }
    }
    if (found == 0) {
        g_gameReg->ReportError(TRIGERR_LINK_BROKEN, TRIGSITE_LINKSB_NO_OWNER);
        return 0;
    }
    i32* p = &child->m_block[0]; // child+0x3c (the child is a 0x9c CTileTriggerLogic)
    for (i32 i = 0; i < 24; i++) {
        i32 key = *p;
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindChild(key, TRIGID_MULTI_SWITCH_3);
        if (c == 0) {
            g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_LINKSB_KEY_MISS);
            return 0;
        }
        if (c->m_linkGate == 0) {
            return 0;
        }
        p++;
    }
    return 0;
}

RVA(0x00112050, 0x12)
CTileExclusiveTriggerSwitchLogic::CTileExclusiveTriggerSwitchLogic() {}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::Broadcast (0x112080) - walk this switch's m_block key
// array; each key must resolve (owner->FindChild(key, 4), acking 0x44f on a miss)
// to a sibling switch; for a resolved sibling that is not THIS switch and is
// link-gated, run its slot-3 virtual, then Tick every m_list1 logic child that
// claims it (FindIndexByKey), acking 0x450 if none does.
// RE-HOMED from GroupOps.cpp (the whole `CGroupBroadcast`/`CFindNode`/
// classes: same layout field-for-field, and this RVA sits inside THIS TU's
// interval 0x110430..0x1140e2 - first-link contiguity says it was defined here).
// ---------------------------------------------------------------------------
// @early-stop
// 84% - regalloc wall: the 0-terminated key-array walk, per-key map Find, the
// inner match/destroy list loop and both diagnostic exits are byte-faithful; the
// residual is loop-induction / counter register colouring.  No EH frame.
RVA(0x00112080, 0x138)
i32 CTileTriggerSwitchLogic::Broadcast() {
    // retail: a DIRECT `call 0x2e0f` (the slot-2 body's ILT thunk) - a qualified,
    // devirtualized call, so spell it qualified.
    CTileTriggerSwitchLogic::SwitchDown();
    i32 counter = 0;
    i32* p = &m_block[0];
    i32 i = 0;
    i32 done = 0;
    do {
        if (i >= 0x18) {
            return 1;
        }
        CTileTriggerSwitchLogic* node = m_owner->FindChild(*p, TRIGID_EXCLUSIVE_SWITCH_4);
        if (node == 0) {
            g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_BCAST_KEY_MISS);
            return 0;
        }
        if (node->m_key1 != m_key1 && node->m_linkGate != 0) {
            node->SwitchUp(); // virtual slot 3
            i32 any = 0;
            for (TtcNode* it = TtcHead(m_owner->m_list1); it != 0; it = it->m_next) {
                CTileTriggerLogic* o = static_cast<CTileTriggerLogic*>(it->m_data);
                if (o != 0 && o->FindIndexByKey(node->m_key1)) {
                    o->Tick(); // slot 0
                    counter++;
                    any = 1;
                }
            }
            if (any == 0) {
                g_gameReg->ReportError(TRIGERR_LINK_BROKEN, TRIGSITE_BCAST_NO_CLAIM);
                return 0;
            }
        }
        i32 next = p[1];
        p++;
        i++;
        if (next == 0) {
            done = 1;
        }
    } while (!done);
    return 1;
}

RVA(0x00112210, 0x12)
CGiantRockLogic::CGiantRockLogic() {}

RVA(0x00112240, 0x12)
CCoveredPowerupLogic::CCoveredPowerupLogic() {}

RVA(0x00112270, 0x12)
CTileTimeTriggerLogic::CTileTimeTriggerLogic() {}

extern "C" i32 g_killCueClock; // _g_killCueClock @0x6bf3c0

// @early-stop
// loop-body regalloc wall (~69%): complete + correct reconstruction - the frame
// (0x14, 5 locals), the PtInRect gate, and ALL of steps 3-5 (the m_cmdGrid Fire, the
// InGameText sprite + m_124 stamp, the view-rect bounds cascade, the sound-registry
// Lookup + kill-cue-clock cooldown Play) are byte-exact. The residual is confined to
// the 3x3 loop body: retail spills BOTH counters i (edi) and j (ebx) to the frame and
// reloads them, chaining the plane through a 2nd register so g_gameReg stays live in
// edi for the +0x70 tileGrid read; MSVC5 here keeps i in ebx and re-loads g_gameReg,
// which also duplicates the inner-tail (an extra jmp + a 2nd copy). Source nudges
// (read-order swap, an explicit g_gameReg loop local, px/py caching) all leave the
// i->edi / j->ebx spill-and-reload assignment unmoved - a non-steerable regalloc pick
// inside the hottest block. Deferred to the final sweep.
RVA(0x001122a0, 0x241)
void CGiantRockLogic::BuildRockBreakInGameText() {
    // The world holder: the ex-CWorldZ view IS CDDrawSurfaceMgr (one object at +0x30;
    // and the sound host at +0x28 - were declared identically on both.
    CDDrawSurfaceMgr* gameMgr = g_gameReg->m_world; // cached only for the loop sprite

    // (1) in-rect gate: is the tile center inside the view rect (+0x13c)?
    i32 inRect = 0;
    POINT pt;
    pt.y = (m_tileY << 5) + 0x10;
    pt.x = (m_tileX << 5) + 0x10;
    if (PtInRect(reinterpret_cast<const RECT*>(&g_gameReg->m_viewOriginL), pt)) {
        inRect = 1;
    }

    // (2) 3x3 neighborhood: write each saved cell value into the level plane + notify
    // the tile grid; when in-rect, spawn a Particlez/LEVEL_ROCKBREAK sprite per cell.
    i32* cursor = &m_matrix[0];
    for (i32 j = 0; j <= 2; j++) {
        for (i32 i = 0; i <= 2; i++) {
            i32 value = *cursor;
            i32 px = i + m_tileX - 1;
            i32 py = j + m_tileY - 1;
            CPlaneRender* plane = g_gameReg->m_world->m_level->m_mainPlane;
            plane->m_tileGrid[plane->m_colOffsets[py] + px] = value;
            g_gameReg->m_tileGrid->Notify(px, py, value);
            if (inRect) {
                CWwdGameObjectA* spr = gameMgr->m_childGroup->CreateSprite(
                    0,
                    ((i + m_tileX) << 5) - 0x10,
                    ((j + m_tileY) << 5) - 0x10,
                    0xcf84f,
                    "Particlez",
                    0x40003
                );
                if (spr != 0) {
                    spr->ApplyName("LEVEL_ROCKBREAK");
                    spr->ApplyLookupGeometry("LEVEL_ROCKBREAK", 0);
                }
            }
            cursor++;
        }
    }

    // (3) fire the command-grid effect at the tile center (cx/cy reused by step 4).
    i32 cx = (m_tileX << 5) + 0x10;
    i32 cy = (m_tileY << 5) + 0x10;
    g_gameReg->m_cmdGrid->LoadPowerupIconSprites(m_c0, cx, cy, static_cast<i32>(m_dutyOffSpan), 1, 0);

    // (4) when +0xc4 is set, spawn an InGameText sprite carrying it.
    if (m_c4 != 0) {
        CGameObject* txt = g_gameReg->m_world->m_childGroup
                               ->CreateSprite(0, cx, cy, 0x17318, "InGameText", 0x40003);
        if (txt == 0) {
            return;
        }
        txt->m_124 = m_c4;
    }

    // (5) on-screen + no active override -> play the LEVEL_ROCKBREAK cue.
    if ((m_tileX << 5) + 0x10 >= g_gameReg->m_viewOriginR
        || (m_tileX << 5) + 0x10 < g_gameReg->m_viewOriginL
        || (m_tileY << 5) + 0x10 >= g_gameReg->m_viewOriginB
        || (m_tileY << 5) + 0x10 < g_gameReg->m_viewOriginT) {
        return;
    }
    CSndHost* sreg =
        gameMgr->m_soundRegistry; // m_28 typed CSndHost* on the canonical holder (GameRegistry.h)
    if (sreg->m_emitGate != 0) {
        return;
    }
    void* out_ob = 0;
    sreg->m_10.Lookup("LEVEL_ROCKBREAK", out_ob);
    LeafCue* out = static_cast<LeafCue*>(out_ob);
    if (out == 0) {
        return;
    }
    if (g_sndEnabled == 0) {
        return;
    }
    i32 kc = g_killCueClock;
    if (static_cast<u32>((kc - out->m_14)) < static_cast<u32>(out->m_18)) {
        return;
    }
    out->m_14 = kc;
    out->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::ApplyMove
// Edits the (m_08,m_0c) tile cell of the active layer: an explicit override m_34
// when set, else by the verb (0x1e->0x5a, 0x1f->0x5b, 0x21->cell+1), marks the
// cell dirty, flags the surrounding screen rect, and (when m_2c is set) posts an
// in-game-text record stamped with m_2c.  Returns 1.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc/addressing wall (~70%): logic + the subtract-chain switch + the
// shared px/py reuse match; the four duplicated grid-access blocks allocate
// registers differently from retail (same scale-4 vs pre-shift split as BumpCell).
RVA(0x00112590, 0x166)
i32 CTileTriggerLogic::ApplyMove(i32 verb) {
    i32 v;
    if (m_tileToken != 0) {
        CGruntzMgr* reg = g_gameReg;
        CPlaneRender* L = reg->m_world->m_level->m_mainPlane;
        L->m_tileGrid[L->m_colOffsets[m_tileY] + m_tileX] = m_tileToken;
        v = m_tileToken;
        (reg->m_tileGrid)->ComputeCellFlags(m_tileX, m_tileY, v);
    } else {
        switch (verb) {
            case 0x22: {
                CGruntzMgr* reg = g_gameReg;
                CPlaneRender* L = reg->m_world->m_level->m_mainPlane;
                v = L->m_tileGrid[L->m_colOffsets[m_tileY] + m_tileX] + 1;
                CPlaneRender* L2 = reg->m_world->m_level->m_mainPlane;
                L2->m_tileGrid[L2->m_colOffsets[m_tileY] + m_tileX] = v;
                (reg->m_tileGrid)->ComputeCellFlags(m_tileX, m_tileY, v);
                break;
            }
            case 0x1f: {
                CGruntzMgr* reg = g_gameReg;
                CPlaneRender* L = reg->m_world->m_level->m_mainPlane;
                L->m_tileGrid[L->m_colOffsets[m_tileY] + m_tileX] = 0x5b;
                (reg->m_tileGrid)->ComputeCellFlags(m_tileX, m_tileY, 0x5b);
                break;
            }
            case 0x1e: {
                CGruntzMgr* reg = g_gameReg;
                CPlaneRender* L = reg->m_world->m_level->m_mainPlane;
                L->m_tileGrid[L->m_colOffsets[m_tileY] + m_tileX] = 0x5a;
                (reg->m_tileGrid)->ComputeCellFlags(m_tileX, m_tileY, 0x5a);
                break;
            }
            default:
                break;
        }
    }
    CGruntzMgr* reg = g_gameReg;
    i32 py = (m_tileY << 5) + 0x10;
    i32 px = (m_tileX << 5) + 0x10;
    reg->m_cmdGrid->LoadPowerupIconSprites(m_dutyOnSpan, px, py, m_dutyOffSpan, 1, 0);
    if (m_leadInSpan != 0) {
        CGameObject* rec =
            reg->m_world->m_childGroup->CreateSprite(0, px, py, 95000, "InGameText", 0x40003);
        if (rec != 0) {
            rec->m_124 = m_leadInSpan;
        }
    }
    return 1;
}

RVA(0x00112760, 0x12)
CTileSecretTriggerLogic::CTileSecretTriggerLogic() {}

RVA(0x00112790, 0x12)
CTileSecretTriggerSwitchLogic::CTileSecretTriggerSwitchLogic() {}

RVA(0x001127c0, 0x12)
CTileTimeTriggerSwitchLogic::CTileTimeTriggerSwitchLogic() {}

RVA(0x001127f0, 0x12)
CCheckpointTriggerSwitchLogic::CCheckpointTriggerSwitchLogic() {}

RVA(0x00112820, 0xc)
i32 CTileSecretTriggerSwitchLogic::SwitchDown() {
    return CTileTriggerSwitchLogic::SwitchDown() != 0;
}

RVA(0x00112840, 0xc)
i32 CTileTimeTriggerSwitchLogic::SwitchDown() {
    return CTileTriggerSwitchLogic::SwitchDown() != 0;
}

RVA(0x00112860, 0xc)
i32 CTileTimeTriggerSwitchLogic::SwitchUp() {
    return CTileTriggerSwitchLogic::SwitchUp() != 0;
}

RVA(0x00112880, 0x12)
void CTileTriggerLogic::RecordMove() {
    m_startClock = g_frameTime;
    m_owner->MoveList1ToList2(this);
}

// ---------------------------------------------------------------------------
// CTileSecretTriggerLogic::Tick (slot-0 override, 0x1128b0). The vtable slot map
// proves the identity: ??_7CTileSecretTriggerLogic@0x1eaf14 slot 0 holds this body
// via ILT thunk 0x18d4.
// The secret trigger's duty tick: swap this trigger's parked tile token with the
// one in the MAIN plane's tile grid at (m_08, m_0c), recompute the cell flags,
// and adopt the previously-parked token. An empty token reports the 0x8009/0x451
// diagnostic and returns 0.
// ---------------------------------------------------------------------------
// @early-stop
// Register-naming wall (~88%, structure byte-exact). Retail has higher register
// pressure: it keeps mgr(edi)/idx/grp live, spills newTok to a stack local
// ([esp+0x1c]/[esp+0x10]) and RE-WALKS the m_world->m_level->m_mainPlane->cells
// chain for the write instead of CSE-ing the cell address. Two levers reproduced
// that shape (54.9 -> 88): (1) cache g_gameReg in a local `mgr`; (2) idx/grp
// read-once locals shared between the cell index and the ComputeCellFlags args;
// (3) crucially, WRITE the cell through the un-cached global g_gameReg while
// READING via mgr - this defeats MSVC's read/write address-CSE, forcing the
// spill+rewalk (62 -> 88). Residual is pure regalloc naming (retail mgr=edi/
// idx=eax/grp=ecx vs base mgr=ecx/idx=edx/grp=eax, plus the idx leaf-read
// scheduled after W/L vs hoisted before) - an unsteerable allocator coin-flip at
// identical instruction count. See docs/patterns/cse-defeat-uncached-global-rewalk.md.
RVA(0x001128b0, 0x88)
i32 CTileSecretTriggerLogic::Tick() {
    i32 oldTok = m_tileToken;
    if (oldTok == 0) {
        g_gameReg->ReportError(0x8009, 0x451);
        return 0;
    }
    CGruntzMgr* mgr = g_gameReg;
    i32 grp = m_tileX;
    i32 idx = m_tileY;
    i32 newTok = mgr->m_world->m_level->m_mainPlane
                     ->m_tileGrid[mgr->m_world->m_level->m_mainPlane->m_colOffsets[idx] + grp];
    g_gameReg->m_world->m_level->m_mainPlane
        ->m_tileGrid[g_gameReg->m_world->m_level->m_mainPlane->m_colOffsets[idx] + grp] = oldTok;
    mgr->m_tileGrid->ComputeCellFlags(grp, idx, oldTok);
    m_tileToken = newTok;
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::Classify
// Drives the command's on/off duty cycle off the running game clock: while the
// elapsed time is within the lead-in (m_2c) it stays active (+1); past it, the
// remainder modulo the on+off period (m_28+m_30) selects the on or off phase,
// firing the slot-0 tick and latching m_dutyOn on each edge.  Returns +1 (active),
// 0 (just turned on, one-shot of type 0x18) or -1 (just turned off, not 0x17).
// ---------------------------------------------------------------------------
// @early-stop
// entropy-tail (~96%): logic + the single-ret1 convergence match; only the last
// type==0x17 case's ret1 is tail-duplicated instead of merged into the shared tail.
// @interleaver CTileTriggerLogic::Classify emitted-in <boundary:
// CTileSecretTriggerLogic::Tick @0x1128b0 (before, now THIS TU) +
// CheckpointSwitchBuild.cpp BuildSmall @0x112a50 (after)>. A /Gy first-use
// COMDAT the linker scattered inside the band.
RVA(0x00112970, 0xad)
i32 CTileTriggerLogic::Classify(i32 arg) {
    u32 elapsed = g_frameTime - m_startClock;
    if (elapsed <= m_leadInSpan) {
        goto ret1;
    }
    elapsed -= m_leadInSpan;
    {
        u32 period = m_dutyOffSpan + m_dutyOnSpan;
        if (elapsed > period) {
            if (m_typeTag == TRIGID_TILE_TRIGGER_24) {
                Tick();
                return 0;
            }
            if (m_typeTag != TRIGID_TIME_TRIGGER_23) {
                if (m_dutyOn == 1) {
                    Tick();
                }
                return -1;
            }
        }
        u32 rem = elapsed % period;
        if (rem < m_dutyOnSpan) {
            if (m_dutyOn != 0) {
                goto ret1;
            }
            Tick();
            m_dutyOn = 1;
            if (m_typeTag == TRIGID_TILE_TRIGGER_24) {
                return 0;
            }
            goto ret1;
        }
        if (m_dutyOn != 1) {
            goto ret1;
        }
        Tick();
        m_dutyOn = 0;
        if (m_typeTag == TRIGID_TIME_TRIGGER_23) {
            goto ret1;
        }
        return -1;
    }
ret1:
    return 1;
}

// Slot 2: reads the active tile layer's cell at (col,row), stores value+1 back, republishes
// it through the tile grid, and SETS the +0x14 flag.  Returns 1.
// @early-stop
// addressing-mode wall (~73%): logic identical; retail indexes the row table with
// a scale-4 address mode (`[rowtbl+y*4]`), the recompile pre-shifts y (shl 2) and
// uses scale-1, propagating through both cell accesses.
RVA(0x00112b70, 0x5a)
i32 CCheckpointTriggerSwitchLogic::SwitchDown() {
    CGruntzMgr* reg = g_gameReg;
    CPlaneRender* layer = reg->m_world->m_level->m_mainPlane;
    i32 v = layer->m_tileGrid[m_08 + layer->m_colOffsets[m_key0c]] + 1;
    CPlaneRender* layer2 = reg->m_world->m_level->m_mainPlane;
    layer2->m_tileGrid[m_08 + layer2->m_colOffsets[m_key0c]] = v;
    (reg->m_tileGrid)->ComputeCellFlags(m_08, m_key0c, v);
    m_linkGate = 1;
    return 1;
}

// Slot 3: the decrement sibling - same cell read/write path, value-1, and CLEARS the +0x14
// flag.  Returns 1.
// @early-stop
// strength-reduction wall (~73%): cl materializes row<<2 (shl ecx,2) and reuses
// scale-1 addressing where retail keeps the row in a scale-4 address mode in both cell
// stores; the shift vs scaled-index pick is not steerable.
RVA(0x00112bf0, 0x5e)
i32 CCheckpointTriggerSwitchLogic::SwitchUp() {
    CGruntzMgr* reg = g_gameReg;
    CPlaneRender* layer = reg->m_world->m_level->m_mainPlane;
    i32 v = layer->m_tileGrid[m_08 + layer->m_colOffsets[m_key0c]] - 1;
    CPlaneRender* layer2 = reg->m_world->m_level->m_mainPlane;
    layer2->m_tileGrid[m_08 + layer2->m_colOffsets[m_key0c]] = v;
    (reg->m_tileGrid)->ComputeCellFlags(m_08, m_key0c, v);
    m_linkGate = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::VerifyBlockLinks
// Linkage validator: if this->m_linkGate is clear, succeed (return 0 short-circuit on
// the null gate).  Otherwise walk the owner's child list (head @ owner->m_20),
// asking each child's FindIndexByKey(this->m_key1) until one claims this object.
// If none does, ack diagnostic 0x452 and fail.  Then, for the claiming child,
// scan its 24-dword key block (child->m_block[4..27]): an empty slot succeeds;
// each nonzero key must resolve via owner->FindChild(key, 8) to a child whose
// m_linkGate is set (else fail, acking 0x453 when the lookup itself misses).
// Returns 1 on the early empty-slot success, 0 otherwise.
// ---------------------------------------------------------------------------
// @early-stop
// this-spill frame wall (~86%): body byte-identical; retail reserves a `push ecx`
// stack local for `this` + reloads it (`mov edi,[esp+0x10]`) to seed the `child`
// loop cursor, the recompile seeds it from a register (`mov edi,ebp`) with no slot.
// Dead seed value, non-steerable frame choice. See
// docs/patterns/this-spilled-to-local-for-loop-seed.md
RVA(0x00112c70, 0xc4)
i32 CTileTriggerSwitchLogic::VerifyBlockLinks() {
    if (m_linkGate == 0) {
        return 0;
    }
    // walk the owner CONTAINER's m_list1 (head @ container+0x20) - the 0x9c
    // CTileTriggerLogic children live there.
    TtcNode* node = TtcHead(m_owner->m_list1);
    i32 found = 0;
    CTileTriggerLogic* child = 0;
    while (node != 0) {
        if (found != 0) {
            break;
        }
        TtcNode* cur = node;
        node = node->m_next;
        child = static_cast<CTileTriggerLogic*>(cur->m_data);
        if (child != 0 && child->FindIndexByKey(m_key1) != 0) {
            found = 1;
        }
    }
    if (found == 0) {
        g_gameReg->ReportError(TRIGERR_LINK_BROKEN, TRIGSITE_LINKS_NO_OWNER);
        return 0;
    }
    i32* p = &child->m_block[0]; // child+0x3c (the child is a 0x9c CTileTriggerLogic)
    for (i32 i = 0; i < 24; i++) {
        i32 key = *p;
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindChild(key, TRIGID_CHECKPOINT_SWITCH_8);
        if (c == 0) {
            g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_LINKS_KEY_MISS);
            return 0;
        }
        if (c->m_linkGate == 0) {
            return 0;
        }
        p++;
    }
    return 0;
}

RVA(0x00112d80, 0xa)
CTileActionEvent::CTileActionEvent() {
    m_10 = 0;
}

RVA(0x00112da0, 0x9e)
i32 CTileActionEvent::SetActionCode(i32 code) {
    m_actionCode = code;
    if (m_playerFlags[g_tileKindMagic] == 0 && static_cast<u32>((code - 0x12f)) <= 0x1a) {
        switch (code) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                code = 0x12f;
                break;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146:
                code = 0x130;
                break;
            case 0x131:
            case 0x135:
            case 0x136:
            case 0x137:
            case 0x13b:
            case 0x13c:
            case 0x13d:
            case 0x141:
            case 0x142:
            case 0x143:
            case 0x147:
            case 0x148:
            case 0x149:
                code = 0x131;
                break;
        }
    }
    {
        CPlaneRender* grid = g_gameReg->m_world->m_level->m_mainPlane;
        i32* cell = &grid->m_tileGrid[grid->m_colOffsets[m_tileY] + m_tileX];
        if (*cell == code) {
            return 0;
        }
        *cell = code;
        g_gameReg->m_tileGrid->ComputeCellFlags(m_tileX, m_tileY, code);
        return 1;
    }
}

// ===========================================================================
// CTileActionEvent::Process  (0x112ee0) - __thiscall, ret 4
// ===========================================================================
// @early-stop
// 918-byte two-jump-table dispatch wall (outer remap switch on m_actionCode + inner
// brick-color switch on the derived effect code, four shl-5/add-0x10 coordinate
// scalings under heavy register pressure ebp=this/ebx=arg/esi=newCode/edi=effect).
// Logic complete + decoded; byte-match deferred to the final sweep.
//
// Outer switch(m_actionCode): derive `effect` (edi, 0=none) and the canonical re-fire code
// `newCode` (esi). First-half: fire the per-effect game action on the brick arg.
// Second-half (always): if the tile is on-screen, spawn the brick-break sprite and
// pick its colored break animation by `effect`. Finally re-fire SetActionCode with
// `newCode` if it changed; return (newCode == 0x12d).
RVA(0x00112ee0, 0x35e)
i32 CTileActionEvent::Process(i32 arg) {
    i32 newCode = m_actionCode;
    i32 effect = 0;
    switch (m_actionCode) {
        case 0x12f:
            newCode = 0x12d;
            break;
        case 0x130:
            newCode = 0x12f;
            break;
        case 0x131:
            newCode = 0x130;
            break;
        case 0x132:
            effect = 0x132;
            newCode = 0x12d;
            break;
        case 0x133:
            newCode = 0x132;
            break;
        case 0x134:
            effect = 0x132;
            newCode = 0x12f;
            break;
        case 0x135:
            newCode = 0x133;
            break;
        case 0x136:
            newCode = 0x134;
            break;
        case 0x137:
            effect = 0x132;
            newCode = 0x130;
            break;
        case 0x138:
            effect = 0x138;
            newCode = 0x12d;
            break;
        case 0x139:
            newCode = 0x138;
            break;
        case 0x13a:
            effect = 0x138;
            newCode = 0x12f;
            break;
        case 0x13b:
            newCode = 0x139;
            break;
        case 0x13c:
            newCode = 0x13a;
            break;
        case 0x13d:
            effect = 0x138;
            newCode = 0x130;
            break;
        case 0x13e:
            effect = 0x13e;
            if (arg != 0) {
                break;
            }
            newCode = 0x12d;
            break;
        case 0x13f:
            newCode = 0x13e;
            break;
        case 0x140:
            effect = 0x13e;
            if (arg != 0) {
                break;
            }
            newCode = 0x12f;
            break;
        case 0x141:
            newCode = 0x13f;
            break;
        case 0x142:
            newCode = 0x140;
            break;
        case 0x143:
            effect = 0x13e;
            if (arg != 0) {
                break;
            }
            newCode = 0x130;
            break;
        case 0x144:
            effect = 0x144;
            newCode = 0x12d;
            break;
        case 0x145:
            newCode = 0x144;
            break;
        case 0x146:
            effect = 0x144;
            newCode = 0x12f;
            break;
        case 0x147:
            newCode = 0x145;
            break;
        case 0x148:
            newCode = 0x146;
            break;
        case 0x149:
            effect = 0x144;
            newCode = 0x130;
            break;
    }

    CGrunt* brick = reinterpret_cast<CGrunt*>(arg);
    if (effect != 0 && brick != 0) {
        if (effect == 0x132) {
            brick->LoadGruntTypeTable(0, 1, 0, 0);
            brick->m_entranceActive = 0;
        } else if (effect == 0x138) {
            g_gameReg->m_cmdGrid->CombatCue((m_tileX << 5) + 0x10, (m_tileY << 5) + 0x10, 1, 2, -1);
        } else if (effect == 0x13e) {
            i32 px = (m_tileX << 5) + 0x10;
            i32 py = (m_tileY << 5) + 0x10;
            if (px < g_gameReg->m_viewOriginR && px >= g_gameReg->m_viewOriginL
                && py < g_gameReg->m_viewOriginB && py >= g_gameReg->m_viewOriginT
                && g_gameReg->m_world->m_soundRegistry->m_emitGate == 0) {
                LeafCue* snd = static_cast<LeafCue*>(g_gameReg->m_world->m_soundRegistry->Lookup_05b7e0(
                    "GRUNTZ_NORMALGRUNT_IMPACTMM3"
                ));
                if (snd != 0) {
                    snd->PlayIfElapsed(static_cast<i32>(g_sndCueTag), 0, 0, 0);
                }
            }
            if (brick->m_tileOwnerHi == 5) {
                m_playerFlags[0] = 1;
                m_playerFlags[1] = 1;
                m_playerFlags[2] = 1;
                m_playerFlags[3] = 1;
                SetActionCode(m_actionCode);
                return 0;
            }
            m_playerFlags[brick->m_tileOwnerHi] = 1;
            SetActionCode(m_actionCode);
            return 0;
        } else if (effect == 0x144) {
            g_gameReg->m_cmdGrid
                ->LoadExplosionSprites((m_tileX << 5) + 0x10, (m_tileY << 5) + 0x10, -1, 2);
        }
    }

    i32 px = (m_tileX << 5) + 0x10;
    i32 py = (m_tileY << 5) + 0x10;
    if (px < g_gameReg->m_viewOriginR && px >= g_gameReg->m_viewOriginL
        && py < g_gameReg->m_viewOriginB && py >= g_gameReg->m_viewOriginT) {
        CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup
                               ->CreateSprite(0, px, py, 0xcf84f, "Particlez", 0x40003);
        if (spr != 0) {
            spr->ApplyLookupGeometry("GAME_BRICKBREAK", 0);
            // Inner dense byte-mapped switch on (effect - 0x132) -> the colored break
            // animation; effect 0x138->RED, 0x13d->BLUE, 0x142->GOLD, the remaining
            // mapped slots->BLACK, anything off-table->default GAME_BRICKBREAK (which
            // also sets the +0x8 uncached flag). The exact slot-0 (effect 0x132) and
            // 0x144 color assignments are the deferred byte-match residual.
            switch (effect) {
                case 0x138:
                    spr->ApplyName("GAME_REDBRICKBREAK");
                    break;
                case 0x13d:
                    spr->ApplyName("GAME_BLUEBRICKBREAK");
                    break;
                case 0x142:
                    spr->ApplyName("GAME_GOLDBRICKBREAK");
                    break;
                default:
                    if (effect >= 0x133 && effect <= 0x144) {
                        spr->ApplyName("GAME_BLACKBRICKBREAK");
                        break;
                    }
                    spr->ApplyName("GAME_BRICKBREAK");
                    if (spr->m_layer == 0) {
                        spr->m_flags |= 0x10000;
                    }
                    break;
            }
        }
    }

    if (newCode != m_actionCode) {
        SetActionCode(newCode);
    }
    return newCode == 0x12d;
}

RVA(0x00113420, 0x1f2)
i32 CTileActionEvent::MorphByTool(i32 toolId, i32 playerSlot) {
    if (toolId == 0x22) {
        switch (m_actionCode) {
            case 0x12f:
                m_actionCode = 0x130;
                break;
            case 0x132:
                m_actionCode = 0x133;
                break;
            case 0x138:
                m_actionCode = 0x139;
                break;
            case 0x13e:
                m_actionCode = 0x13f;
                break;
            case 0x144:
                m_actionCode = 0x145;
                break;
            case 0x130:
                m_actionCode = 0x131;
                break;
            case 0x133:
                m_actionCode = 0x135;
                break;
            case 0x134:
                m_actionCode = 0x136;
                break;
            case 0x139:
                m_actionCode = 0x13b;
                break;
            case 0x13a:
                m_actionCode = 0x13c;
                break;
            case 0x13f:
                m_actionCode = 0x141;
                break;
            case 0x140:
                m_actionCode = 0x142;
                break;
            case 0x145:
                m_actionCode = 0x147;
                break;
            case 0x146:
                m_actionCode = 0x148;
                break;
            default:
                return 0;
        }
    } else if (toolId == 0x23) {
        switch (m_actionCode) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                m_actionCode = 0x134;
                break;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146:
                m_actionCode = 0x137;
                break;
            default:
                return 0;
        }
    } else if (toolId == 0x24) {
        switch (m_actionCode) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                m_actionCode = 0x13a;
                break;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146:
                m_actionCode = 0x13d;
                break;
            default:
                return 0;
        }
    } else if (toolId == 0x26) {
        switch (m_actionCode) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                m_actionCode = 0x146;
                break;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146:
                m_actionCode = 0x149;
                break;
            default:
                return 0;
        }
    } else if (toolId == 0x25) {
        switch (m_actionCode) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                m_actionCode = 0x140;
                break;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146:
                m_actionCode = 0x143;
                break;
            default:
                return 0;
        }
    }

    m_playerFlags[0] = 0;
    m_playerFlags[1] = 0;
    m_playerFlags[2] = 0;
    m_playerFlags[3] = 0;
    if (playerSlot == 5) {
        m_playerFlags[0] = 1;
        m_playerFlags[1] = 1;
        m_playerFlags[2] = 1;
        m_playerFlags[3] = 1;
    } else {
        m_playerFlags[playerSlot] = 1;
    }
    SetActionCode(m_actionCode);
    return 1;
}

RVA(0x00113860, 0x3b)
i32 CTileTriggerSwitchLogic::ValidateByType(CSerialArchive* s, i32 mode, i32 a3, i32 a4) {
    if (s == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (!SaveState(s)) {
                return 0;
            }
            break;
        case 7:
            if (!LoadState(s)) {
                return 0;
            }
            break;
    }
    return 1;
}

RVA(0x001138b0, 0xb4)
i32 CTileTriggerSwitchLogic::SaveState(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    ar->Write(&m_08, 4);
    ar->Write(&m_key0c, 4);
    ar->Write(&m_key1, 4);
    ar->Write(&m_linkGate, 4);
    ar->Write(&m_18, 4);
    ar->Write(&m_1c, 4);
    ar->Write(&m_initGate, 4);
    ar->Write(&m_28, 4);
    i32* p = m_block;
    i32 n = 24;
    do {
        ar->Write(p, 4);
        p++;
    } while (--n);
    return 1;
}

RVA(0x001139a0, 0xb4)
i32 CTileTriggerSwitchLogic::LoadState(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_08, 4);
    s->Read(&m_key0c, 4);
    s->Read(&m_key1, 4);
    s->Read(&m_linkGate, 4);
    s->Read(&m_18, 4);
    s->Read(&m_1c, 4);
    s->Read(&m_initGate, 4);
    s->Read(&m_28, 4);
    i32* p = m_block;
    for (i32 i = 0; i < 24; i++) {
        s->Read(p, 4);
        p++;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::ValidateByType - the 0x9c family's save/load dispatcher.
// Returns 0 if the archive is null; type 4 saves (Serialize), type 7 loads
// (Deserialize); any other type passes (returns 1).
//
// RE-HOMED from CTileTriggerSwitchLogic (0x8c): CTileTriggerFactory::Build calls it
// (ILT 0x1abe) at 0x117aa7 on a freshly-`new`ed 0x9c CTileTriggerLogic. Its two callees
// were modeled as `__stdcall TileSwitchCheckType4/7(void* obj)` free functions - but retail
// emits `push eax; call <rel32>` with ECX UNTOUCHED, i.e. they are __thiscall methods run on
// `this` (0x291e -> Serialize @0x113ae0, 0x3102 -> Deserialize @0x113c10). Same bytes; the
// free-function spelling only matched because `this` happened to still be live in ecx.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~93%): switch-case ordering + the two calls match; retail keeps arg1 in eax
// (returns it as the null-zero, push eax for the callees) vs our ecx + explicit xor in the
// null block. Entry-block register only.
RVA(0x00113a90, 0x3b)
i32 CTileTriggerLogic::ValidateByType(void* archive, i32 type, i32 a3, i32 a4) {
    if (archive == 0) {
        return 0;
    }
    switch (type) {
        case 4:
            if (Serialize(static_cast<CSerialArchive*>(archive)) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Deserialize(static_cast<CSerialArchive*>(archive)) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::Serialize  (the type-4 save ValidateByType dispatches to)
// Returns 0 if the stream is null or the active game-manager (g_gameReg+0x30) is
// null; otherwise transfers the scalar fields then the 24-dword m_block through
// the stream's Write slot and returns 1.
//
// RE-HOMED off the invented `CTileTriggerLogic` (see TileGridCommand.h @identity-TODO):
// ValidateByType reaches it with `this` in ecx, and that `this` is a 0x9c CTileTriggerLogic
// straight off the allocation site. Fields are the same offsets under this class's names
// (m_block -> m_block, m_dutyOn kept).
// ---------------------------------------------------------------------------
RVA(0x00113ae0, 0xe8)
i32 CTileTriggerLogic::Serialize(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_tileX, 4);
    s->Write(&m_tileY, 4);
    s->Write(&m_10, 4);
    s->Write(&m_14, 4);
    s->Write(&m_18, 4);
    s->Write(&m_initGate, 4);
    s->Write(&m_dutyOnSpan, 4);
    s->Write(&m_leadInSpan, 4);
    s->Write(&m_dutyOffSpan, 4);
    s->Write(&m_tileToken, 4);
    s->Write(&m_dutyOn, 4);
    s->Write(&m_startClock, 4);
    i32* p = m_block;
    for (i32 i = 0; i < 24; i++) {
        s->Write(p, 4);
        p++;
    }
    return 1;
}

RVA(0x00113c10, 0xe8)
i32 CTileTriggerLogic::Deserialize(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_tileX, 4);
    s->Read(&m_tileY, 4);
    s->Read(&m_10, 4);
    s->Read(&m_14, 4);
    s->Read(&m_18, 4);
    s->Read(&m_initGate, 4);
    s->Read(&m_dutyOnSpan, 4);
    s->Read(&m_leadInSpan, 4);
    s->Read(&m_dutyOffSpan, 4);
    s->Read(&m_tileToken, 4);
    s->Read(&m_dutyOn, 4);
    s->Read(&m_startClock, 4);
    i32* p = m_block;
    for (i32 i = 0; i < 24; i++) {
        s->Read(p, 4);
        p++;
    }
    return 1;
}

RVA(0x00113d40, 0x6f)
i32 CGiantRockLogic::ApplyByType(void* archive, i32 type, i32 a3, i32 a4) {
    if (archive == 0) {
        return 0;
    }
    if (ValidateByType(archive, type, a3, a4) == 0) {
        return 0;
    }
    switch (type) {
        case 4:
            if (SerializeMatrix(static_cast<CSerialArchive*>(archive)) == 0) {
                return 0;
            }
            break;
        case 7:
            if (DeserializeMatrix(static_cast<CSerialArchive*>(archive)) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGiantRockLogic::SerializeMatrix
// Streams two header dwords (+0xc0, +0xc4) then the 3x3 dword matrix (+0x9c..) via
// the stream's Write slot.  Returns 0 if the stream or the active game-manager
// (g_gameReg+0x30) is null, else 1.
//
// RE-HOMED from CTileTriggerSwitchLogic. These +0xc0/+0xc4 writes are what made the old
// owner's array run to m_block[38] and overrun its 0x8c allocation - the contradiction that
// blocked the layout fix. They are CGiantRockLogic's own tail (0x9c base + 0x24 matrix + 8).
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~95%): whole body byte-identical; retail pins this->esi /
// stream->edi (pushes all 4 callee regs, then loads args) vs our this->edi /
// stream->esi (arg load interleaved with the pushes). Reg-pair swap only.
RVA(0x00113dd0, 0x7b)
i32 CGiantRockLogic::SerializeMatrix(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_c0, 4);
    s->Write(&m_c4, 4);
    i32* p = m_matrix;
    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Write(p, 4);
            p++;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGiantRockLogic::DeserializeMatrix (0x113e70) - the READ mirror of SerializeMatrix:
// streams two header dwords (+0xc0, +0xc4) then the 3x3 dword matrix (+0x9c..) via the
// stream's Read slot. Returns 0 if the stream or the active game-manager (g_gameReg+0x30)
// is null, else 1. This is the type-7 (load) apply ApplyByType dispatches to (thunk 0x3cd3).
// @early-stop
// esi/edi regalloc wall (~95%, same as SerializeMatrix): whole body byte-identical;
// retail pins this->esi / stream->edi vs our this->edi / stream->esi. Reg-pair swap.
RVA(0x00113e70, 0x7b)
i32 CGiantRockLogic::DeserializeMatrix(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_c0, 4);
    s->Read(&m_c4, 4);
    i32* p = m_matrix;
    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Read(p, 4);
            p++;
        }
    }
    return 1;
}

RVA(0x00113f10, 0x3b)
i32 CTileActionEvent::Serialize(void* ar, i32 mode, i32 a3, i32 a4) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (SerializeFields(ar) == 0) {
                return 0;
            }
            break;
        case 7:
            if (DeserializeFields(ar) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

RVA(0x00113f60, 0xa2)
i32 CTileActionEvent::SerializeFields(void* ar) {
    CSerialArchive* a = static_cast<CSerialArchive*>(ar);
    if (a == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    a->Write(&m_actionCode, 4);
    a->Write(&m_tileX, 4);
    a->Write(&m_tileY, 4);
    a->Write(&m_c, 4);
    a->Write(&m_10, 4);
    a->Write(&m_playerFlags[0], 4);
    a->Write(&m_playerFlags[1], 4);
    a->Write(&m_playerFlags[2], 4);
    a->Write(&m_playerFlags[3], 4);
    return 1;
}

RVA(0x00114040, 0xa2)
i32 CTileActionEvent::DeserializeFields(void* ar) {
    CSerialArchive* a = static_cast<CSerialArchive*>(ar);
    if (a == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    a->Read(&m_actionCode, 4);
    a->Read(&m_tileX, 4);
    a->Read(&m_tileY, 4);
    a->Read(&m_c, 4);
    a->Read(&m_10, 4);
    a->Read(&m_playerFlags[0], 4);
    a->Read(&m_playerFlags[1], 4);
    a->Read(&m_playerFlags[2], 4);
    a->Read(&m_playerFlags[3], 4);
    return 1;
}
