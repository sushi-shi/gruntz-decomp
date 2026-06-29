// GruntArrivalStep.cpp - CGrunt::StepArrivalDefenseAlt (@0xf1c70), the powered-up
// arrival-defender variant. Trace mis-attributed to "ClassUnknown_42"; every member
// offset (m_248/m_260/m_220/m_21c/m_3f0/m_2d4/m_2f0/m_2f4/m_300/m_304/m_174..m_180)
// and every engine helper (GetOccupant, RectContains, FindGridNeighbor, CommitNeighbor,
// StepArrivalDrop, GruntInRadius, the on-screen CueA) is a CGrunt member/method, so it
// is re-homed here as a CGrunt method (the devs' true shape). Field names are
// placeholders; offsets + code bytes are load-bearing. Engine callees + g_pGameRegistry
// are external (reloc-masked).
#include <Gruntz/Grunt.h>
#include <rva.h>

extern CGameRegistry* g_pGameRegistry; // ?g_gameReg@@3PAUWwdGameRegZ@@A

// @early-stop
// arrival-defender regalloc/redundant-recheck wall (~big body): the prologue (m_248
// dirty stamp, GetOccupant settle + RectContains in-range latch), the powered-up release
// gate (FindGridNeighbor + the m_220/m_21c/m_218/m_1e4 clear-state with its cached
// ecx=m_220/eax=m_21c re-reads), and every m_2d4 (0/1/2/3) case (the grid-occupant
// CommitNeighbor commits, the 4-way StepArrivalDrop tile-walk toward m_defenderX/Y, the
// on-screen CueA 0x366) are byte-faithful in shape/offsets/symbols/constants. Residue:
// retail caches m_220/m_21c in callee-saved regs across the GetOccupant call and folds
// the switch-bound 3 into the m_2d4=3 store (ebx pin); structured C++ re-reads the members
// + materializes the immediate, permuting the register pins across the redundant arrival
// re-checks. Source-invariant (the documented regalloc/recheck wall); deferred to the
// final sweep.
RVA(0x000f1c70, 0x60d)
i32 CGrunt::StepArrivalDefenseAlt() {
    m_arrivalFlags |= 0x40000;
    CGrunt* occ = m_tileMgr->GetOccupant(this);
    i32 inRange = 0;
    if (occ != 0 && occ->m_10->m_5c == occ->m_lastTilePxX && occ->m_10->m_60 == occ->m_lastTilePxY
        && RectContains(occ->m_10->m_5c, occ->m_10->m_60) != 0) {
        inRange = 1;
    }

    if (m_poweredUp != 0) {
        if (m_neighborValid != 0) {
            m_neighborValid = 0;
            return 1;
        }
        if (*(i32*)((char*)this + 0x218) != 0) {
            goto tail;
        }
        if (m_stamina >= 0x64) {
            if (FindGridNeighbor(1) != 0) {
                goto tail;
            }
            if (inRange != 0 && occ == 0) {
                goto tail;
            }
            if (m_poweredUp == 0) {
                goto tail;
            }
            if (m_neighborValid != 0) {
                goto tail;
            }
        } else {
            if (inRange != 0) {
                goto tail;
            }
            if (m_poweredUp == 0) {
                goto tail;
            }
            if (m_neighborValid != 0) {
                goto tail;
            }
        }
        m_entranceActive = 0;
        *(i32*)((char*)this + 0x218) = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        Stub_062e10(1, 0, 0);
        return 1;
    }

    switch (m_2d4) {
        case 0: {
            CGrunt* o = m_tileMgr->GetOccupant(this);
            if (o != 0) {
                if (m_poweredUp != 0) {
                    goto tail;
                }
                if (m_stamina >= 0x64 && o->m_10->m_5c == o->m_lastTilePxX
                    && o->m_10->m_60 == o->m_lastTilePxY
                    && RectContains(o->m_10->m_5c, o->m_10->m_60) != 0) {
                    CommitNeighbor(
                        o->m_tileOwnerHi,
                        o->m_tileOwnerLo,
                        o->m_lastTilePxX,
                        o->m_lastTilePxY
                    );
                    return 1;
                }
            }
            if (m_poweredUp != 0) {
                goto tail;
            }
            if (m_lastTilePxX != m_entrancePxX || m_lastTilePxY != m_entrancePxY) {
                goto tail;
            }
            {
                i32 tx = m_lastTilePxX >> 5;
                i32 ty = m_lastTilePxY >> 5;
                i32 gx = m_defenderX >> 5;
                i32 gy = m_defenderY >> 5;
                if (tx < gx) {
                    if (ty < gy) {
                        StepArrivalDrop(
                            m_lastTilePxX + 0x40,
                            m_lastTilePxY,
                            0,
                            m_arrivalFlags,
                            1,
                            0
                        );
                        return 1;
                    }
                    if (ty > gy) {
                        StepArrivalDrop(
                            m_lastTilePxX,
                            m_lastTilePxY - 0x40,
                            0,
                            m_arrivalFlags,
                            1,
                            0
                        );
                        return 1;
                    }
                    goto resetState;
                }
                if (tx > gx) {
                    if (ty < gy) {
                        StepArrivalDrop(
                            m_lastTilePxX,
                            m_lastTilePxY + 0x40,
                            0,
                            m_arrivalFlags,
                            1,
                            0
                        );
                        return 1;
                    }
                    if (ty > gy) {
                        StepArrivalDrop(
                            m_lastTilePxX - 0x40,
                            m_lastTilePxY,
                            0,
                            m_arrivalFlags,
                            1,
                            0
                        );
                        return 1;
                    }
                }
                goto resetState;
            }
        }

        case 1: {
            CGrunt* o = *(CGrunt**)((char*)m_tileMgr + (m_2f4 + 15 * m_2f0) * 4 + 0x1c);
            CGrunt* g = m_tileMgr->GetOccupant(this);
            if (g != 0 && g != o) {
                m_2f0 = -1;
                m_2d4 = 0;
                m_2f4 = -1;
                return 1;
            }
            if (o == 0) {
                goto resetState;
            }
            if (o->m_entranceCommitted == 0) {
                goto resetState;
            }
            if (GruntInRadius(o->m_tileOwnerHi, o->m_tileOwnerLo) == 0) {
                goto resetState;
            }
            if (GruntInRadius(m_2f0, m_2f4) == 0) {
                goto resetState;
            }
            StepArrivalDrop(o->m_lastTilePxX, o->m_lastTilePxY, 0, m_arrivalFlags, 1, 0);
            if (m_poweredUp != 0) {
                goto tail;
            }
            if (m_stamina < 0x64) {
                goto tail;
            }
            if (RectContains(o->m_10->m_5c, o->m_10->m_60) == 0) {
                goto tail;
            }
            if (o->m_10->m_5c != o->m_lastTilePxX) {
                goto tail;
            }
            if (o->m_10->m_60 != o->m_lastTilePxY) {
                goto tail;
            }
            CommitNeighbor(o->m_tileOwnerHi, o->m_tileOwnerLo, o->m_lastTilePxX, o->m_lastTilePxY);
            m_2d4 = 2;
            return 1;
        }

        case 2:
            m_2d4 = 0;
            return 1;

        case 3: {
            StepArrivalDrop(m_defenderX - 0x20, m_defenderY - 0x20, 0, m_arrivalFlags, 1, 0);
            if (m_10->m_5c == m_defenderX - 0x20 && m_10->m_60 == m_defenderY - 0x20) {
                m_2d4 = 0;
                return 1;
            }
            CGrunt* o = m_tileMgr->GetOccupant(this);
            if (o == 0) {
                goto tail;
            }
            if (m_poweredUp == 0 && m_stamina >= 0x64 && o->m_10->m_5c == o->m_lastTilePxX
                && o->m_10->m_60 == o->m_lastTilePxY
                && RectContains(o->m_10->m_5c, o->m_10->m_60) != 0) {
                CommitNeighbor(
                    o->m_tileOwnerHi,
                    o->m_tileOwnerLo,
                    o->m_lastTilePxX,
                    o->m_lastTilePxY
                );
                m_2d4 = 2;
            }
            if (GruntInRadius(o->m_tileOwnerHi, o->m_tileOwnerLo) == 0) {
                goto tail;
            }
            m_2f0 = o->m_tileOwnerHi;
            m_2f4 = o->m_tileOwnerLo;
            m_2d4 = 1;
            {
                CGruntHud* h = m_10;
                i32 x = h->m_5c;
                i32 y = h->m_60;
                i32* rect = (i32*)(g_pGameRegistry->m_30->m_24->m_5c + 0x40);
                if (x < rect[2] && x >= rect[0] && y < rect[3] && y >= rect[1]) {
                    g_pGameRegistry->m_60->CueA(this, 0x366, -1, 0, -1, -1);
                }
            }
            goto tail;
        }

        default:
            goto tail;
    }

resetState:
    m_2d4 = 3;
    return 1;

tail:
    return 1;
}
