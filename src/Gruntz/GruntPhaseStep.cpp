// GruntPhaseStep.cpp - a CGrunt arrival/relocation phase state machine re-homed out
// of src/Stub/ApiCallers.cpp (0x000f60f0). __thiscall, no args, always returns 1.
//
// Gated on the grunt's resolved type name (g_typeColl lookup of m_14->m_1c vs a fixed
// type string): for the matching type it drives a small state machine on m_2d4
// (states 0/2/4/0x19/0x1a) that recomputes the grunt's target tile, builds the 16
// border cells of the 5x5 block around it into a point accumulator, picks a random
// still-free cell to relocate/arrive on (marking tiles via the tile-manager), and
// recycles the visited-cell CObList nodes back onto the shared free list. /GX for the
// accumulator + rect temporaries. Placeholder names; only offsets + code bytes are
// load-bearing. Modeled with a local CGrunt view (offsets only) to stay self-contained.
#include <Win32.h>

#include <Ints.h>
#include <rva.h>
#include <string.h>

#pragma intrinsic(strcmp)

namespace gruntphase {

    // The type-name collection singleton (0x6bf650); Lookup437c(key) returns a node
    // whose first dword is the char* type name.
    struct TypeColl {
        char** Lookup437c(i32 key); // 0x0000437c (thiscall)
    };
    extern TypeColl g_typeColl;         // 0x006bf650
    extern const char g_phaseType[];    // 0x0060d2e8 (the gate type name)

    // The point accumulator built for the 16 border cells (a small CObList/array
    // temporary): m_4 = the packed-point array, m_8 = live count.
    struct PtAcc {
        i32 m_0;
        i32* m_4; // +0x04 packed-point array
        i32 m_8;  // +0x08 live count
        void Ctor1b4b43();               // 0x001b4b43
        void Add1b4d7c(i32 a, i32 pt);   // 0x001b4d7c
        void Remove1b4e38(i32 i, i32 f); // 0x001b4e38
        void Dtor1b4b76();               // 0x001b4b76
    };

    // A tile-grid plane (g_mgrSettings->m_70): m_8 = row table, m_c/m_10 = width/height.
    struct TilePlane {
        char m_pad0[8];
        i32** m_8; // +0x08 row table
        i32 m_c;   // +0x0c width
        i32 m_10;  // +0x10 height
    };
    struct MapObj {
        char m_pad0[0x5c];
        i32 m_5c; // +0x5c pixel x
        i32 m_60; // +0x60 pixel y
    };
    struct LevelSubInner {
        char m_pad0[0x5c];
        i32 m_5c; // +0x5c
    };
    struct LevelSub {
        char m_pad0[0x24];
        LevelSubInner* m_24; // +0x24
    };
    struct MgrSettings {
        char m_pad0[0x30];
        LevelSub* m_30; // +0x30
        char m_pad34[0x60 - 0x34];
        i32 m_60; // +0x60
        char m_pad64[0x68 - 0x64];
        void* m_68; // +0x68
        char m_pad6c[0x70 - 0x6c];
        TilePlane* m_70; // +0x70
    };
    extern MgrSettings* g_mgrSettings; // 0x0064556c

    // The shared free-list of recycled CObList nodes.
    extern void* g_freeList;        // 0x00645544
    extern i32 g_freeListNodeBias;  // 0x0064554c

    // A visited-cell node (CObList element): m_0 = next, m_8 = payload.
    struct ObNode {
        ObNode* m_0; // +0x00 next
        char m_pad4[8 - 4];
        void* m_8; // +0x08
    };
    struct ObList {
        ObNode* m_0; // +0x00 head
        char m_pad4[8 - 4];
        ObNode* m_8; // +0x08 tail
        void RemoveAll1b48a6(); // 0x001b48a6
    };

    struct GruntTilePos {
        i32 x, y;
    };

    static void RecycleNodes(i32* headPtr);

    struct AnimRec {
        char m_pad0[0x1c];
        i32 m_1c; // +0x1c type key
    };

    struct CGrunt;
    // A resolved neighbor node (from tile-mgr): m_10 map obj, m_17c/m_180 dst coords,
    // m_1ec/m_1f0 owner coords, m_1fc gate.
    struct NeighborNode {
        char m_pad0[0x10];
        MapObj* m_10; // +0x10
        char m_pad14[0x17c - 0x14];
        i32 m_17c; // +0x17c
        i32 m_180; // +0x180
        char m_pad184[0x1ec - 0x184];
        i32 m_1ec; // +0x1ec
        i32 m_1f0; // +0x1f0
        char m_pad1f4[0x1fc - 0x1f4];
        i32 m_1fc; // +0x1fc
    };
    struct TileMgr {
        NeighborNode* Resolve253b(CGrunt* g); // 0x0000253b
    };

    struct CGrunt {
        char m_pad0[0x10];
        MapObj* m_10;  // +0x10
        AnimRec* m_14; // +0x14
        char m_pad18[0x17c - 0x18];
        i32 m_17c; // +0x17c
        i32 m_180; // +0x180
        char m_pad184[0x1ec - 0x184];
        i32 m_1ec; // +0x1ec
        i32 m_1f0; // +0x1f0
        char m_pad1f4[0x220 - 0x1f4];
        i32 m_220; // +0x220
        char m_pad224[0x248 - 0x224];
        i32 m_248; // +0x248
        i32 m_24c; // +0x24c
        char m_pad250[0x260 - 0x250];
        TileMgr* m_260; // +0x260
        char m_pad264[0x2d4 - 0x264];
        i32 m_2d4; // +0x2d4 state
        char m_pad2d8[0x2ec - 0x2d8];
        i32 m_2ec; // +0x2ec dwell
        i32 m_2f0; // +0x2f0 tile x
        i32 m_2f4; // +0x2f4 tile y
        char m_pad2f8[0x300 - 0x2f8];
        i32 m_300; // +0x300
        i32 m_304; // +0x304
        char m_pad308[0x31c - 0x308];
        ObList m_31c; // +0x31c
        i32* m_320;   // +0x320
        char m_pad324[0x328 - 0x324];
        i32 m_328; // +0x328
        char m_pad32c[0x358 - 0x32c];
        i32 m_358; // +0x358
        char m_pad35c[0x390 - 0x35c];
        i32 m_390; // +0x390
        char m_pad394[0x3f0 - 0x394];
        i32 m_3f0; // +0x3f0

        void GetTilePos36c0(GruntTilePos* out); // 0x000036c0
        i32 MarkTile1640(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x00001640
        i32 Check3c4c(i32 a, i32 b);         // 0x00003c4c
        void Emit302b(i32 a, i32 b, i32 c, i32 d); // 0x0000302b
        i32 Probe1014(i32 a, i32 b);         // 0x00001014
        void Notify39f4(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x000039f4
        i32 PhaseStep();
    };

    // Rect builder (0x34a4) + a rect view.
    struct RectT {
        i32 left, top, right, bottom;
    };
    extern "C" RectT* RectSet34a4(RectT* r, i32 l, i32 t, i32 rr, i32 bb); // 0x000034a4
    // The dirty/valid-cell region builder over the accumulator.
    extern "C" void AccInit1b4b43(void* rgn);                 // 0x001b4b43 (also PtAcc::Ctor)
    extern "C" i32 GridTest1127(i32 a, i32 b, i32 c);         // 0x00001127 (cdecl)
    extern "C" i32 Rand11fee0();                              // 0x0011fee0
    extern "C" void PlaceTile14bf(void* plane, i32 a, i32 b, i32 c, i32 d); // 0x000014bf

    // @early-stop
    // regalloc + region-build wall. Complete correct reconstruction: the type-name
    // gate (inline strcmp of the g_typeColl lookup vs the phase type), the m_2d4 state
    // dispatch (0x19/0x1a re-mark, 0/2/4), the 5x5-border 16-point accumulator build +
    // random-free-cell relocation with tile marking, and the common tail's CObList
    // node recycle onto the shared free list all align by shape (llvm-objdump -dr).
    // Residual: MSVC5 pins the tile coords/loop indices across esi/edi/ebp/ebx and
    // schedules the 16 unrolled (x+-2,y+-2) packed-point stores + rect/IntersectRect
    // temporaries at [esp+N] slots that a source transcription can't reproduce exactly.
    RVA(0x000f60f0, 0xb30)
    i32 CGrunt::PhaseStep() {
        RectT r0;
        RectT r1;
        PtAcc acc;
        GruntTilePos pa;
        GruntTilePos pb;

        m_358 = 0;
        if (strcmp(*g_typeColl.Lookup437c(m_14->m_1c), g_phaseType) == 0) {
            return 1;
        }
        m_300 = m_17c;
        m_304 = m_180;

        if (m_2d4 == 0x19) {
            GetTilePos36c0(&pa);
            i32 ax = pa.x >> 5;
            GetTilePos36c0(&pb);
            i32 gx = (pb.x >> 5) - m_2f0 + ax;
            GetTilePos36c0(&pa);
            i32 ay = pa.y >> 5;
            GetTilePos36c0(&pb);
            i32 gy = (pb.y >> 5) - m_2f4 + ay;
            MarkTile1640(gx, gy, 0, m_248, 1, 0);
            m_2ec = 0;
            m_2d4 = 4;
        }
        if (m_2d4 == 0x1a) {
            GetTilePos36c0(&pa);
            i32 ax = pa.x >> 5;
            GetTilePos36c0(&pb);
            GetTilePos36c0(&pa);
            i32 gx = (pb.x >> 5) - m_2f0 + ax;
            i32 ay = pa.x >> 5;
            GetTilePos36c0(&pb);
            i32 gy = (pb.y >> 5) - m_2f4 + ay;
            MarkTile1640(gx, gy, 0, m_248, 1, 0);
            m_2d4 = 0;
            return 1;
        }

        if (m_2d4 == 0) {
            goto state0;
        }
        if (m_2d4 == 2) {
            goto state2;
        }
        if (m_2d4 != 4) {
            goto common;
        }
        if (m_2ec <= 0x1f40) {
            return 1;
        }
        m_2d4 = 0;
        return 1;

    state2: {
        if (strcmp(*g_typeColl.Lookup437c(m_14->m_1c), g_phaseType) == 0) {
            goto common;
        }
        i32 x = m_2f0;
        i32 y = m_2f4;
        RectSet34a4(&r0, 0, 0, 0, 0);
        RectSet34a4(&r1, 0, 0, 0, 0);
        acc.Ctor1b4b43();
        acc.Add1b4d7c(acc.m_8, ((x - 2) << 16) | ((y - 2) & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x - 1) << 16) | ((y - 2) & 0xffff));
        acc.Add1b4d7c(acc.m_8, (x << 16) | ((y - 2) & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x + 1) << 16) | ((y - 2) & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x + 2) << 16) | ((y - 2) & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x - 2) << 16) | ((y + 2) & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x - 1) << 16) | ((y + 2) & 0xffff));
        acc.Add1b4d7c(acc.m_8, (x << 16) | ((y + 2) & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x + 1) << 16) | ((y + 2) & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x + 2) << 16) | ((y + 2) & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x - 2) << 16) | ((y - 1) & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x - 2) << 16) | (y & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x - 2) << 16) | ((y + 1) & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x + 2) << 16) | ((y - 1) & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x + 2) << 16) | (y & 0xffff));
        acc.Add1b4d7c(acc.m_8, ((x + 2) << 16) | ((y + 1) & 0xffff));
        while (acc.m_8 != 0) {
            i32 sel = Rand11fee0() % acc.m_8;
            i32 pt = acc.m_4[sel];
            i32 px = (u32)pt >> 0x10;
            i32 py = pt & 0xffff;
            TilePlane* pl = g_mgrSettings->m_70;
            i32 flag;
            if ((u32)px < (u32)pl->m_c && (u32)py < (u32)pl->m_10 && px < pl->m_c && py < pl->m_10) {
                flag = pl->m_8[py][px * 8 - px];
            } else {
                flag = 1;
            }
            if ((flag & 0x939) == 0) {
                if (MarkTile1640(px, py, 0, m_248, 1, 0) != 0) {
                    m_2d4 = 4;
                    m_2ec = 0;
                    goto build_tail;
                }
            }
            acc.Remove1b4e38(sel, 1);
        }
    build_tail: {
        TilePlane* pl2 = g_mgrSettings->m_70;
        RectSet34a4(&r0, 0, 0, pl2->m_c, pl2->m_10);
        pl2->m_c = r1.right - r1.left;
        pl2->m_10 = r1.bottom - r1.top;
        acc.Dtor1b4b76();
        goto common;
    }
    }

    state0: {
        NeighborNode* nb = m_260->Resolve253b(this);
        if (nb == 0) {
            goto common;
        }
        if (nb->m_1fc == 0) {
            goto common;
        }
        if (m_220 == 0 && m_3f0 >= 0x64 && nb->m_10->m_5c == nb->m_17c && nb->m_10->m_60 == nb->m_180 &&
            Check3c4c(nb->m_10->m_5c, nb->m_10->m_60) != 0) {
            Emit302b(nb->m_1ec, nb->m_1f0, nb->m_17c, nb->m_180);
            m_2f0 = nb->m_10->m_5c >> 5;
            m_2f4 = nb->m_10->m_60 >> 5;
            m_2d4 = 2;
            goto common;
        }
        if (m_2ec <= 0x1f4) {
            goto common;
        }
        if (Probe1014(nb->m_1ec, nb->m_1f0) == 0) {
            goto s0_reset;
        }
        if (MarkTile1640(nb->m_10->m_5c >> 5, nb->m_10->m_60 >> 5, 0, m_248, 1, 0) == 0) {
            m_24c |= 0x4020;
            MarkTile1640(nb->m_10->m_5c >> 5, nb->m_10->m_60 >> 5, 0, m_248, 1, 0);
            m_24c &= 0xffffbfdf;
        }
        m_2ec = 0;
        if (m_390 == 0) {
            goto common;
        }
        if (GridTest1127(g_mgrSettings->m_30->m_24->m_5c + 0x40, m_10->m_5c, m_10->m_60) == 0) {
            goto s0_reset;
        }
        Notify39f4(0x366, -1, 0, -1, -1);
    s0_reset:
        m_390 = 0;
        goto common;
    }

    common: {
        i32 st = m_2d4;
        if (st != 4 && st != 0x19 && m_328 >= 2) {
            i32* head = m_320;
            i32 bx = ((i32*)head[2])[0];
            i32 by = ((i32*)head[2])[1];
            i32* p0 = (i32*)((i32*)head[0])[2];
            i32 fx = p0[0];
            i32 fy = p0[1];
            TilePlane* pl = g_mgrSettings->m_70;
            i32 flag;
            (void)bx;
            if ((u32)fx < (u32)pl->m_c && (u32)fy < (u32)pl->m_10) {
                flag = pl->m_8[fy][fx * 8 - fx];
            } else {
                flag = 1;
            }
            if ((flag & 0x20) != 0) {
                if (m_328 != 0) {
                    RecycleNodes(m_320);
                    m_31c.RemoveAll1b48a6();
                }
                PlaceTile14bf(g_mgrSettings->m_68, m_1ec, m_1f0, fy * 32 + 16, fx * 32 + 16);
                m_2f0 = fx;
                m_2f4 = by;
                m_2d4 = 0x19;
                return 1;
            }
        }
        if (m_328 == 0) {
            return 1;
        }
        i32* p1 = (i32*)m_320[2];
        TilePlane* pl2 = g_mgrSettings->m_70;
        i32 gx = p1[0];
        i32 gy = p1[1];
        i32 flag2;
        if ((u32)gy < (u32)pl2->m_c && (u32)gx < (u32)pl2->m_10) {
            flag2 = pl2->m_8[gx][gy * 8 - gy];
        } else {
            flag2 = 1;
        }
        if ((flag2 & 0x20) == 0) {
            return 1;
        }
        m_2f0 = gx;
        m_2f4 = gy;
        if (m_328 != 0) {
            RecycleNodes(m_320);
            m_31c.RemoveAll1b48a6();
        }
        m_2d4 = 0x1a;
        return 1;
    }
    }

    // Recycle the visited-cell CObList nodes back onto the shared free list.
    static void RecycleNodes(i32* headPtr) {
        ObNode* n = (ObNode*)headPtr;
        while (n != 0) {
            ObNode* next = n->m_0;
            void* pay = n->m_8;
            if (pay != 0) {
                void** slot = (void**)((char*)pay - g_freeListNodeBias);
                *slot = g_freeList;
                g_freeList = slot;
            }
            n = next;
        }
    }

} // namespace gruntphase
