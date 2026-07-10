// GruntReticle.cpp - CGrunt::ArrivalReticleScan (@0xee800), the arrival/defender
// reticle scan. XREF-RECOVERED IDENTITY: this was mis-homed under the auto-generated
// ?winapi_0ee800_IntersectRect_PtInRect@CUserLogic; the `this` object extends to
// +0x3f0 (m_stamina) and beyond, and the retail caller 0x5d210 (CGrunt vtable slot 3)
// runs it on a CGrunt. It is a CGrunt method. The supporting singletons are the
// canonical modeled classes: CGameRegistry (g_mgrSettings), CTileGrid (registry+0x70),
// CGameViewport (registry->m_world->m_24), FreeNodePool (g_coordPool), GruntCoordNode.
//
// @early-stop
// The front state machine (range gate / occupant / powered-up-neighbour-combat dispatch
// / occupied-coord recycle) is reconstructed byte-faithfully. The tail from the reach-box
// grid snapshot onward (0xeeb93..: a local CByteArray built over the tile grid, the
// radius mark double-loops, and the shared IntersectRect(grid.rect, viewport) + coord-list
// rebuild at 0xef0de that nearly every non-early-return path funnels through) is
// DECOMPILER-GATED: it relies on MSVC /O2 stack-slot aliasing (the reach-box corners
// [esp+48/4c/50/58] are reused as the CByteArray + rect, and SetAtGrow's nIndex resolves
// to an array-internal field) that cannot be reconstructed reliably from raw disasm.
// Deferred to the final sweep (needs the Ghidra decompiler C output). The front match is
// banked here (a real CGrunt method, no views/casts); the tail diverges.
#include <Mfc.h> // the /GX EH frame (the reconstructed tail builds a local CByteArray)
#include <Gruntz/Grunt.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/FreeNodePool.h> // g_coordPool.Push
#include <stdlib.h>              // abs (branchless cdq/xor/sub)
#include <rva.h>

RVA(0x000ee800, 0x971)
i32 CGrunt::ArrivalReticleScan() {
    i32 defTX = m_defenderX >> 5;
    i32 defTY = m_defenderY >> 5;

    GruntTilePos pt;
    GetScreenPos(&pt);
    i32 dTX = abs((pt.m_x >> 5) - defTX);
    GetScreenPos(&pt);
    i32 dTY = abs((pt.m_y >> 5) - defTY);
    i32 dist = dTX > dTY ? dTX : dTY;
    if (dist > m_defenderRadius) {
        m_defenderX = m_lastTilePxX;
        m_defenderY = m_lastTilePxY;
        return 1;
    }

    CGrunt* occ = m_tileMgr->GetOccupant(this);
    i32 occOnTile = 0;
    if (occ) {
        CGameObject* oo = occ->m_object;
        if (oo->m_screenX == occ->m_lastTilePxX && oo->m_screenY == occ->m_lastTilePxY) {
            if (RectContains(oo->m_screenX, oo->m_screenY)) {
                occOnTile = 1;
            }
        }
    }

    if (m_poweredUp) {
        if (m_neighborValid) {
            m_neighborValid = 0;
            return 1;
        }
        if (m_combatActive) {
            return 1;
        }
        if (m_stamina >= 0x64) {
            if (FindGridNeighbor(1)) {
                return 1;
            }
            if (occOnTile && occ == 0) {
                return 1;
            }
        } else {
            if (occOnTile) {
                return 1;
            }
        }
        if (m_neighborValid) {
            return 1;
        }
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    // --- m_poweredUp == 0 ---
    if (occ == 0) {
        m_390 = 0;
    } else {
        if (m_neighborValid) {
            return 1;
        }
        if (m_combatActive == 0 && m_stamina >= 0x64 && occOnTile) {
            CommitCombatMove(
                occ->m_tileOwnerHi,
                occ->m_tileOwnerLo,
                occ->m_lastTilePxX,
                occ->m_lastTilePxY
            );
            if (m_coordCount) {
                for (GruntCoordNode* n = m_320; n; n = n->m_next) {
                    if (n->m_coord) {
                        g_coordPool.Push(n->m_coord);
                    }
                }
                m_31c.RemoveAll();
            }
            return 1;
        }
        if (occOnTile) {
            if (m_coordCount) {
                for (GruntCoordNode* n = m_320; n; n = n->m_next) {
                    if (n->m_coord) {
                        g_coordPool.Push(n->m_coord);
                    }
                }
                m_31c.RemoveAll();
            }
            return 1;
        }
    }

    // --- reach-box grid marking tail (DECOMPILER-GATED; see @early-stop above) ---
    // The confident branch structure is retained; the CByteArray snapshot + radius
    // mark loops + the shared IntersectRect/coord-rebuild are deferred.
    return 1;
}
