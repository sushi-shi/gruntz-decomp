// GruntChargeStep.cpp - CGrunt::ChargeStep (0x0ef6b0) re-homed from
// src/Stub/Discovered.cpp. The per-frame "pursue / charge the target grunt"
// behavior step: a three-state machine (m_2d4 = scan / move / arrived) over the
// grunt manager's tile table (m_260), issuing move/attack commands and re-arming
// a random-wander fallback. /base - no destructible locals. Self-contained CGrunt
// view (own TU) so the shared Grunt.h/CGrunt model stays untouched; only offsets +
// the reloc-masked engine callees (ILT thunks) are load-bearing.
#include <Ints.h>
#include <rva.h>

// A grunt world-position sub-object (this->m_10 and target->m_10). +0x5c/+0x60 are
// the pixel coords; +0x134..+0x140 the random-wander bounding rect.
struct PosObj {
    char pad0[0x5c];
    i32 m_5c; // +0x5c  x
    i32 m_60; // +0x60  y
    char pad64[0x134 - 0x64];
    i32 m_134; // +0x134 wander minX
    i32 m_138; // +0x138 wander minY
    i32 m_13c; // +0x13c wander maxX
    i32 m_140; // +0x140 wander maxY
};

struct Grunt;

// The grunt manager tile table at this->m_260: Find(grunt) returns the target and
// the +0x1c entry table is a 15-wide grid of grunt slots.
struct GruntTable {
    Grunt* Find(Grunt* g); // 0x40253b (thiscall, 1 arg)
    char pad0[0x1c];
    Grunt* entries[1]; // +0x1c (indexed [m_2f4 + m_2f0*15])
};

// The g_64556c game-manager singleton chain this step walks.
struct MgrGrid {
    char pad0[0x30];
    i32 m_30; // +0x30 (grid dim base, +0x40'd)
};
struct MgrSubB {
    char pad0[0x5c];
    MgrGrid* m_5c; // +0x5c
};
struct MgrSubA {
    char pad0[0x24];
    MgrSubB* m_24; // +0x24
};
struct MgrBounds {
    char pad0[0xc];
    u32 m_c;  // +0x0c  world width
    u32 m_10; // +0x10  world height
};
struct MgrNotifySink {
    // 0x4039f4 (thiscall): the "grunt engaged" notify (id 0x366).
    void Notify(Grunt* g, i32 id, i32 a, i32 b, i32 c, i32 d);
};
struct GameMgr {
    char pad0[0x30];
    MgrSubA* m_30; // +0x30
    char pad34[0x60 - 0x34];
    MgrNotifySink* m_60; // +0x60
    char pad64[0x70 - 0x64];
    MgrBounds* m_70; // +0x70
};
extern "C" {
    DATA(0x0064556c)
    extern GameMgr* g_64556c;
}

// __cdecl line-of-sight probe (0x401127 via ILT): (destTile, x, y) -> reachable.
i32 GruntLos1127(i32 dest, i32 x, i32 y);

// The CRT rand (0x11fee0).
extern "C" i32 rand();

struct Grunt {
    char pad0[0x10];
    PosObj* m_10; // +0x10
    char pad14[0x17c - 0x14];
    i32 m_17c; // +0x17c  x
    i32 m_180; // +0x180  y
    char pad184[0x1e4 - 0x184];
    i32 m_1e4; // +0x1e4
    char pad1e8[0x1ec - 0x1e8];
    i32 m_1ec; // +0x1ec  (target home x)
    i32 m_1f0; // +0x1f0  (target home y)
    char pad1f4[0x1fc - 0x1f4];
    i32 m_1fc; // +0x1fc  (target alive flag)
    char pad200[0x218 - 0x200];
    i32 m_218; // +0x218
    i32 m_21c; // +0x21c
    i32 m_220; // +0x220
    char pad224[0x244 - 0x224];
    i32 m_244; // +0x244
    i32 m_248; // +0x248
    char pad24c[0x260 - 0x24c];
    GruntTable* m_260; // +0x260
    char pad264[0x2d4 - 0x264];
    i32 m_2d4; // +0x2d4  charge state (0=scan,1=move,2=arrived)
    char pad2d8[0x2ec - 0x2d8];
    i32 m_2ec; // +0x2ec  step timer (ms)
    i32 m_2f0; // +0x2f0  target tile row
    i32 m_2f4; // +0x2f4  target tile col
    char pad2f8[0x300 - 0x2f8];
    i32 m_300; // +0x300  saved x
    i32 m_304; // +0x304  saved y
    char pad308[0x318 - 0x308];
    i32 m_318; // +0x318
    char pad31c[0x328 - 0x31c];
    i32 m_328; // +0x328  max wander distance
    char pad32c[0x3f0 - 0x32c];
    i32 m_3f0; // +0x3f0  charge cooldown counter

    // reloc-masked engine callees (ILT thunks; thiscall on this):
    i32 AtTile(i32 x, i32 y);                               // 0x403c4c
    i32 IsBusy(i32 a);                                      // 0x403d5a
    void StopMove(i32 a, i32 b, i32 c);                     // 0x40136b
    i32 CanReach(i32 x, i32 y);                             // 0x401014
    void FaceTarget(i32 a, i32 b, i32 c, i32 d);            // 0x40302b
    void MoveTo6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x4014e2
    i32 Attack(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);   // 0x401640
    void Snap(i32 a, i32 b);                                // 0x401401

    i32 ChargeStep(); // 0x0ef6b0
};

// ---------------------------------------------------------------------------
// @early-stop
// shared-tail / stack-spill structural wall (~21%, topic:scheduling topic:wall):
// complete + logically faithful (validated against the Ghidra decomp), but retail
// reserves a 0xc-byte frame and spills the target grunt's coords to fixed temps
// [esp+0x14]/[esp+0x18]/[esp+0x28] which feed a SHARED FaceTarget(0x40302b) tail
// block jumped to from two states (llvm-objdump -dr base vs target: frame is
// `sub esp,0xc` retail vs `push ecx` here). Reproducing that shared block +
// spill layout needs a leaf-first goto-structured redo; deferred to the final
// sweep. All member offsets + reloc-masked engine callees are modeled.
RVA(0x000ef6b0, 0x61d)
i32 Grunt::ChargeStep() {
    m_300 = m_17c;
    m_304 = m_180;
    Grunt* g = m_260->Find(this);
    i32 hitGate = 0;
    if (g != 0) {
        PosObj* gp = g->m_10;
        if (gp->m_5c == g->m_17c && gp->m_60 == g->m_180 && AtTile(gp->m_5c, gp->m_60)) {
            hitGate = 1;
        }
    }

    if (m_220 != 0) {
        if (m_21c != 0) {
            m_21c = 0;
            return 1;
        }
        if (m_218 == 0) {
            if (m_3f0 < 100) {
                if (hitGate == 0 && m_220 != 0 && m_21c == 0) {
                    m_1e4 = 0;
                    m_218 = 0;
                    m_21c = 0;
                    m_220 = 0;
                    StopMove(1, 0, 0);
                    return 1;
                }
            } else {
                if (IsBusy(1) == 0 && (hitGate == 0 || g != 0) && m_220 != 0 && m_21c == 0) {
                    m_1e4 = 0;
                    m_218 = 0;
                    m_21c = 0;
                    m_220 = 0;
                    StopMove(1, 0, 0);
                    return 1;
                }
            }
        }
        return 1;
    }

    // ---- m_220 == 0: the charge state machine ----
    if (m_2d4 == 0) {
        // scan for a target on the wander tile
        if (g == 0) {
            goto arrive0;
        }
        if (hitGate != 0) {
            if (m_3f0 >= 100) {
                PosObj* gp = g->m_10;
                if (gp->m_5c == g->m_17c && gp->m_60 == g->m_180 && AtTile(gp->m_60, gp->m_5c)) {
                    FaceTarget(g->m_1ec, g->m_1f0, g->m_17c, g->m_180);
                    return 1;
                }
            }
        }
        if (g != 0 && m_2ec > 500) {
            if (CanReach(g->m_1ec, g->m_1f0) == 0) {
                return 1;
            }
            if (Attack(g->m_10->m_5c >> 5, g->m_10->m_60 >> 5, 0, m_248, 1, 0) != 0) {
                Snap(1, 1);
                m_2f0 = g->m_1ec;
                m_2f4 = g->m_1f0;
                m_2d4 = 1;
                PosObj* mp = m_10;
                GameMgr* mgr = g_64556c;
                i32 los = GruntLos1127(mgr->m_30->m_24->m_5c->m_30 + 0x40, mp->m_5c, mp->m_60);
                if (los != 0) {
                    mgr->m_60->Notify(this, 0x366, -1, 0, -1, -1);
                }
            }
            m_2ec = 0;
            return 1;
        }
    arrive0:
        if (m_244 == 0 && m_318 != 0 && m_2ec > 3000) {
            PosObj* mp = m_10;
            i32 baseX = mp->m_134;
            i32 spanX = mp->m_13c - baseX;
            spanX = spanX < 0 ? -spanX : spanX;
            i32 baseY = mp->m_138;
            i32 spanY = mp->m_140 - baseY;
            spanY = spanY < 0 ? -spanY : spanY;
            if (spanX != 0) {
                baseX += rand() % spanX;
            }
            if (spanY != 0) {
                baseY += rand() % spanY;
            }
            GameMgr* mgr = g_64556c;
            if ((u32)baseX < mgr->m_70->m_c && (u32)baseY < mgr->m_70->m_10) {
                Attack(baseX, baseY, 0, m_248, 1, 0);
            }
            if (m_328 != 0) {
                if (spanX <= spanY) {
                    spanX = spanY;
                }
                if (spanX < m_328) {
                    Snap(1, 1);
                }
            }
            m_2ec = 0;
        }
    } else if (m_2d4 == 1) {
        // moving to the arrival tile
        Grunt* t = m_260->entries[m_2f4 + m_2f0 * 0xf];
        Grunt* cur = m_260->Find(this);
        if (cur != 0 && cur != t) {
            m_2f0 = -1;
            m_2d4 = 0;
            m_2f4 = -1;
            return 1;
        }
        if (t == 0 || t->m_1fc == 0 || CanReach(t->m_1ec, t->m_1f0) == 0) {
            m_2d4 = 0;
            return 1;
        }
        if (m_2ec > 500) {
            MoveTo6(t->m_17c, t->m_180, 0, m_248, 1, 0);
            m_2ec = 0;
        }
        if (m_220 == 0 && m_3f0 >= 100 && AtTile(t->m_10->m_60, t->m_10->m_5c) != 0
            && t->m_10->m_5c == t->m_17c && t->m_10->m_60 == t->m_180) {
            FaceTarget(t->m_1ec, t->m_1f0, t->m_17c, t->m_180);
            m_2d4 = 2;
            return 1;
        }
    } else if (m_2d4 == 2) {
        // arrived: re-check target then hold
        if (m_220 != 0) {
            Grunt* t = m_260->entries[m_2f4 + m_2f0 * 0xf];
            if (t == 0) {
                goto rearm2;
            }
            if (CanReach(t->m_1ec, t->m_1f0) == 0) {
                goto rearm2;
            }
            if (t->m_1fc == 0) {
                goto rearm2;
            }
            if (m_21c != 0 || m_218 != 0 || m_3f0 < 100) {
                return 1;
            }
            if (AtTile(t->m_10->m_60, t->m_10->m_5c) == 0) {
                goto rearm2;
            }
            if (t->m_10->m_5c != t->m_17c || t->m_10->m_60 != t->m_180) {
                goto rearm2;
            }
            FaceTarget(t->m_1ec, t->m_1f0, t->m_17c, t->m_180);
            return 1;
        rearm2:
            m_2d4 = 1;
            m_2ec = 0x1f4;
            return 1;
        } else {
            m_2d4 = 1;
            m_2ec = 0x1f4;
            return 1;
        }
    }
    return 1;
}

SIZE_UNKNOWN(GameMgr);
SIZE_UNKNOWN(Grunt);
SIZE_UNKNOWN(GruntTable);
SIZE_UNKNOWN(MgrBounds);
SIZE_UNKNOWN(MgrGrid);
SIZE_UNKNOWN(MgrNotifySink);
SIZE_UNKNOWN(MgrSubA);
SIZE_UNKNOWN(MgrSubB);
SIZE_UNKNOWN(PosObj);
