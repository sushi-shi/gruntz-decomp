// GruntEntranceArrival.cpp - CGrunt entrance/arrival per-tick step handlers
// (RunAct-dispatched; the g_reg_644af0 activation-registry PMF targets in the
// 0x62840..0x65c20 RVA band). Each advances the grunt's entrance geometry sub-
// player (m_154->m_1a0.Advance_15c360((u32)g_defaultGeo)) and, once that sub-
// player is armed-but-not-running (m_1a0.m_28 != 0 && m_1a0.m_20 == 0), commits
// the grunt's arrival: snap/commit the last tile through the tile-mgr, then a
// board-occupancy gate (g_pGameRegistry->m_tileGrid) chooses among tile-commit,
// arrival-drop, entrance-reset and geometry-reset tails. Only offsets + code
// bytes are load-bearing.
#include <Gruntz/Grunt.h>
#include <Gruntz/AniElement.h> // CAniElement::m_records (active-descriptor frame array)
#include <Gruntz/InGameIcon.h> // CInGameIcon::PlaceAt (the tile-icon the map resolves)
#include <rva.h>

// The default geometry-source token (g_defaultGeo @0x6bf3bc; defined in
// SpriteResource.cpp, reloc-masked here) each step arms the entrance sub-player with.
extern i32 g_defaultGeo;
// g_645588 (the running game clock) is declared by MovingLogic.h via Grunt.h.

// The geometry sub-player's m_20/m_28 live PAST the player's own m_1b4, so they
// are read via raw offsets off &player->m_1a0 (same as FinishEntranceMove) to keep
// cl on one `add eax,0x1a0` base.

// ---------------------------------------------------------------------------
// CGrunt::StepEntranceRelatchA()   @0x62840   (ret 0)
// Advance the entrance geometry. When the sub-player is armed-but-not-running:
// (re)create the HUD stat sprites on arrival, re-latch the "A"(idle) anim set,
// reload the grunt type table, then (board-block-bit set) commit the arrival move,
// else clamp the HUD scroll. When NOT armed-but-running: the toy-break timer path -
// once the toy timer has elapsed and the entrance is unstamped and the sub-player
// just went ready, retire the toy-time HUD sprite, drive the TOY-BREAK geometry +
// re-stamp the cell frame, and fire the on-screen spawn cue; otherwise tick the
// coord list / update the arrival.
RVA(0x00062840, 0x25d)
i32 CGrunt::StepEntranceRelatchA() {
    i32 ready = m_154->m_1a0.Advance_15c360((u32)g_defaultGeo);
    char* sub = (char*)&m_154->m_1a0;
    if (*(i32*)(sub + 0x28) != 0 && *(i32*)(sub + 0x20) == 0) {
        if (m_arrived != 0) {
            CreateHealthSprite();
            CreateStaminaSprite();
            CreateToySprite();
        }
        m_prevAnimSetNode = m_14->m_1c;
        m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeA);
        LoadGruntTypeTable(m_19c, 1, 0, 0);
        m_entranceActive = 0;
        CGameRegistry* g = g_pGameRegistry;
        CTileGrid* grid = g->m_tileGrid;
        i32 tx = m_lastTilePxX >> 5;
        i32 ty = m_lastTilePxY >> 5;
        i32 flags;
        if ((u32)tx >= (u32)grid->m_c || (u32)ty >= (u32)grid->m_10) {
            flags = 1;
        } else {
            flags = ((i32*)grid->m_8[ty])[tx * 7];
        }
        if (flags & 0x80) {
            SetEntrancePos(1, 1);
            m_tileMgr->CommitArrivalMove(this, m_lastTilePxX, m_lastTilePxY);
            return 0;
        }
        CGruntHud* h = m_10;
        i32 v = h->m_60 + 0x186a0;
        if (h->m_74 != v) {
            h->m_74 = v;
            h->m_8 |= 0x20000;
        }
        return 0;
    }
    // sub-player armed-but-still-running: the toy-break timer path.
    i64 diff = (i64)(u32)g_645588 - *(i64*)&m_toyClockLo;
    if (diff >= *(i64*)&m_toyDurationLo && m_entranceStamped == 0 && ready == 1) {
        if (m_toyTimeSprite != 0) {
            m_toyTimeSprite->m_8 |= 0x10000;
            m_toyTimeSprite = 0;
        }
        m_prevEntranceDesc = m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseToyBreak);
        CAniElement* desc = m_154->m_1b4;
        CAnimElem* elem = desc->m_records.m_nSize > 0 ? (CAnimElem*)*desc->m_records.m_pData : 0;
        i32 frame = elem->m_14;
        char* nm = ((CString*)&m_448)->GetBuffer(0);
        m_154->CacheFrameIndexed(nm, frame);
        m_entranceStamped = 1;
        CGruntHud* h = m_10;
        CGameRegistry* g = g_pGameRegistry;
        i32 x = h->m_5c;
        i32 y = h->m_60;
        CCueRect* r = (CCueRect*)(g->m_world->m_24->m_5c + 0x40);
        if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) {
            g->m_cueSink->CueSpawn(this, 0xc, -1, -1, -1);
        }
        return 0;
    }
    ClearSubA();
    if (ready == 1) {
        UpdateArrival(0, 0);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::StepArrivalCommitA()   @0x65300   (ret 0)
// Advance the entrance geometry; once the sub-player is armed-but-not-running:
// if the grunt is dead (m_health <= 0) clear the entrance-commit flag and set the
// tile back, else clear the entrance-active flag and test the head tile's spawn-
// block bit (0x80): set -> commit the arrival move; clear + (m_358==0 && m_35c!=0)
// -> arrival-drop; else m_entranceReason==0x14 -> reset the entrance anim; else
// reset the geometry.
RVA(0x00065300, 0x148)
i32 CGrunt::StepArrivalCommitA() {
    m_154->m_1a0.Advance_15c360((u32)g_defaultGeo);
    char* sub = (char*)&m_154->m_1a0;
    if (*(i32*)(sub + 0x28) == 0 || *(i32*)(sub + 0x20) != 0) {
        return 0;
    }
    if (m_health <= 0) {
        m_entranceCommitted = 0;
        m_tileMgr->SetTile(m_tileOwnerHi, m_tileOwnerLo, 1, m_370);
        return 0;
    }
    m_entranceActive = 0;
    CGameRegistry* g = g_pGameRegistry;
    CTileGrid* grid = g->m_tileGrid;
    i32 tx = m_lastTilePxX >> 5;
    i32 ty = m_lastTilePxY >> 5;
    i32 flags;
    if ((u32)tx >= (u32)grid->m_c || (u32)ty >= (u32)grid->m_10) {
        flags = 1;
    } else {
        flags = ((i32*)grid->m_8[ty])[tx * 7];
    }
    if (flags & 0x80) {
        SetEntrancePos(1, 1);
        m_tileMgr->CommitArrivalMove(this, m_lastTilePxX, m_lastTilePxY);
        return 0;
    }
    if (m_358 == 0 && m_35c != 0) {
        StepArrivalDrop(m_commitPxX, m_commitPxY, 0, -1, 1, 0);
        return 0;
    }
    if (m_entranceReason == 0x14) {
        ResetEntranceAnimation(1, 0, 0);
        return 0;
    }
    ResetGeometry();
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::StepArrivalCommitB()   @0x654b0   (ret 0)
// Sibling of StepArrivalCommitA: once armed-but-not-running, unconditionally snap
// to the last tile + commit the arrival move, then the same health/board-gated
// tail (health<=0 -> set tile back; else block-bit set -> return; else drop or
// reset the geometry).
RVA(0x000654b0, 0x130)
i32 CGrunt::StepArrivalCommitB() {
    m_154->m_1a0.Advance_15c360((u32)g_defaultGeo);
    char* sub = (char*)&m_154->m_1a0;
    if (*(i32*)(sub + 0x28) == 0 || *(i32*)(sub + 0x20) != 0) {
        return 0;
    }
    m_entranceActive = 0;
    SnapToLastTile(1);
    SetEntrancePos(1, 1);
    m_tileMgr->CommitArrivalMove(this, m_lastTilePxX, m_lastTilePxY);
    if (m_health <= 0) {
        m_entranceCommitted = 0;
        m_tileMgr->SetTile(m_tileOwnerHi, m_tileOwnerLo, 1, m_370);
        return 0;
    }
    CGameRegistry* g = g_pGameRegistry;
    CTileGrid* grid = g->m_tileGrid;
    i32 tx = m_lastTilePxX >> 5;
    i32 ty = m_lastTilePxY >> 5;
    i32 flags;
    if ((u32)tx >= (u32)grid->m_c || (u32)ty >= (u32)grid->m_10) {
        flags = 1;
    } else {
        flags = ((i32*)grid->m_8[ty])[tx * 7];
    }
    if (flags & 0x80) {
        return 0;
    }
    if (m_358 == 0 && m_35c != 0) {
        StepArrivalDrop(m_commitPxX, m_commitPxY, 0, -1, 1, 0);
        return 0;
    }
    ResetGeometry();
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::StepEntranceRelatchB()   @0x65c20   (ret 0)
// Advance the entrance geometry (and, when it is still running, drive the tile-mgr
// I-pose load). Once the sub-player is armed-but-not-running: (re)create the HUD
// stat sprites on arrival, re-latch the "D" anim set, commit the coord toggle, then
// two tile-board reads: if the head tile carries the lose-item bit (0x2000000) play
// the lose-item anim; then resolve the tile's placed object (cell dword +8) through
// the sprite factory's key->object map and, when found, place its in-game icon at
// the grunt's owner cell; else clear the tile's object slot + the 0x40000 flag bit.
// @early-stop
// complete reconstruction (all logic + control flow); ~73% plateau. Residue is the
// deep board-cell index regalloc across the TWO tile-board reads + the cell-clear
// (the strength-reduced tx*0x1c / ty*4 running-pointer forms), the reloaded-`g`
// (0x64556c) scheduling around the lose-item call, and the reloc-masked map-lookup /
// PlaceAt / cue operands. A register-allocation/scheduling wall, not a shape error.
RVA(0x00065c20, 0x1d5)
i32 CGrunt::StepEntranceRelatchB() {
    i32 ready = m_154->m_1a0.Advance_15c360((u32)g_defaultGeo);
    if (ready > 0) {
        m_tileMgr->Load6(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            ready
        );
    }
    char* sub = (char*)&m_154->m_1a0;
    if (*(i32*)(sub + 0x28) == 0 || *(i32*)(sub + 0x20) != 0) {
        return 0;
    }
    m_entranceActive = 0;
    if (m_arrived != 0) {
        CreateHealthSprite();
        CreateStaminaSprite();
        CreateToySprite();
    }
    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(g_codeD);
    OnCoordCommit(m_coordToggle);
    CGameRegistry* g = g_pGameRegistry;
    CTileGrid* grid = g->m_tileGrid;
    i32 tx = m_lastTilePxX >> 5;
    i32 ty = m_lastTilePxY >> 5;
    i32 f1;
    if ((u32)tx >= (u32)grid->m_c || (u32)ty >= (u32)grid->m_10) {
        f1 = 1;
    } else {
        f1 = ((i32*)grid->m_8[ty])[tx * 7];
    }
    if (f1 & 0x2000000) {
        BuildGruntLoseItemAnimation();
        g = g_pGameRegistry;
    }
    grid = g->m_tileGrid;
    void* cellObj;
    if ((u32)tx >= (u32)grid->m_c || (u32)ty >= (u32)grid->m_10) {
        cellObj = 0;
    } else {
        cellObj = (void*)((i32*)grid->m_8[ty])[tx * 7 + 2];
    }
    if (cellObj == 0) {
        return 0;
    }
    void* found = 0;
    void* result = 0;
    if (g->m_world->m_8->m_objMap.Lookup(cellObj, found)) {
        result = found;
    }
    if (result != 0) {
        void* aux = *(void**)((char*)result + 0x7c);
        CInGameIcon* icon = *(CInGameIcon**)((char*)aux + 0x18);
        icon->PlaceAt(m_tileOwnerHi, m_tileOwnerLo);
        return 0;
    }
    grid = g_pGameRegistry->m_tileGrid;
    if ((u32)tx < (u32)grid->m_c && (u32)ty < (u32)grid->m_10) {
        ((i32*)grid->m_8[ty])[tx * 7 + 2] = 0;
        ((i32*)grid->m_8[ty])[tx * 7] &= ~0x40000;
    }
    return 0;
}
