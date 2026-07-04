// LevelSync.cpp - 0x1084d0, the big game-container Sync/Serialize (the level-state
// object). __thiscall(stream, op, p4, p5), ret 0x10, /GX EH frame. `op` selects the
// direction: 4 = write (stream slot +0x30), 7 = read (stream slot +0x2c), 8 = a
// pre-pass that resets the manager (g_mgrSettings+0x2c). On read it lazily news the
// +0x54c child (operator new(0x40) + ctor, back-linked at child+0x3c), forwards the
// shared sync (0x402306), streams a block of inline 8-byte fields, then walks every
// owned sub-object array and calls each element's Serialize (vtable slot 1).
//
// The container is a ~0x580-byte field bag whose members are dword-indexed exactly
// as retail addresses them; modeling each of the ~80 fields as a named member buys
// nothing (offsets are the only load-bearing fact), so it is one i32[] view + typed
// stream / sub-object vtable structs for the dispatches. External engine helpers are
// reloc-masked.
#include <Ints.h>
#include <Gruntz/CGameRegistry.h>

#include <rva.h>

// The stream/archive object: a vtable whose +0x2c slot (#11) is Read(buf,n) and +0x30
// (#12) is Write(buf,n). Modeled as an abstract class so the slots land by index.
struct SyncStream {
    virtual void v0() = 0;
    virtual void v1() = 0;
    virtual void v2() = 0;
    virtual void v3() = 0;
    virtual void v4() = 0;
    virtual void v5() = 0;
    virtual void v6() = 0;
    virtual void v7() = 0;
    virtual void v8() = 0;
    virtual void v9() = 0;
    virtual void v10() = 0;
    virtual void Read(void* buf, i32 n) = 0;  // slot 11 / +0x2c
    virtual void Write(void* buf, i32 n) = 0; // slot 12 / +0x30
};

// An owned serializable sub-object: vtable slot 1 (+0x4) is its Serialize.
struct SyncSub {
    virtual void v0() = 0;
    virtual i32 Serialize(SyncStream* s, i32 op, i32 p4, i32 p5) = 0; // slot 1 / +0x4
};

// The lazily-allocated +0x54c child (operator new(0x40) + ctor 0x401271).
class CLevelSync; // owner (defined below); m_3c back-links to it
struct CLevelSyncChild {
    char pad[0x3c];
    CLevelSync* m_3c; // +0x3c back-link to the owner (= `this` in Sync)
    CLevelSyncChild();
};

// The game-registry singleton (g_gameReg, 0x64556c): op-8 resets its +0x2c manager.
struct MgrReset {
    void Reset(); // 0x403d55 (__thiscall)
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

class CLevelSync {
public:
    i32 Sync(SyncStream* s, i32 op, i32 p4, i32 p5);

    // Reloc-masked engine helpers (this-methods unless noted):
    i32 PreWriteValidate(SyncStream* s);                  // 0x4016b8
    i32 PreReadValidate(SyncStream* s);                   // 0x402b53
    void SubResetA();                                     // 0x402b8a
    void SubResetB();                                     // 0x402d5b
    i32 ChildSync(SyncStream* s, i32 op, i32 p4, i32 p5); // 0x402306 (child __thiscall)
    void PostBlockFixup();                                // 0x403a08
    void Finalize();                                      // 0x40125d

    i32 m[0x160];
};

// @early-stop
// /GX EH serialize: ~50 vtable-slot sub-object Serialize calls + an inline field
// block + the lazy operator-new/ctor child path. Logic reconstructed faithfully
// (direction split, the per-array sub walks, the child new+back-link, the finalize),
// but the EH state numbering of the new/ctor unwind region + the deep spill schedule
// across the ~80 member reloads do not reproduce instruction-for-instruction.
// Final-sweep candidate (eh-state-numbering + serialize-reload regalloc walls).
RVA(0x001084d0, 0x96c)
i32 CLevelSync::Sync(SyncStream* s, i32 op, i32 p4, i32 p5) {
    if (s == 0) {
        return 0;
    }
    if (op == 4) {
        if (PreWriteValidate(s) == 0) {
            return 0;
        }
    } else if (op == 7) {
        if (PreReadValidate(s) == 0) {
            return 0;
        }
    } else if (op == 8) {
        ((MgrReset*)g_gameReg->m_curState)->Reset();
        if (m[0] == 0) {
            SubResetA();
            SubResetB();
        }
    }

    if (m[0x153] == 0) {
        i32 tmp = 0;
        if (op == 4) {
            s->Write(&tmp, 4);
        } else if (op == 7) {
            s->Read(&tmp, 4);
            if (tmp != 0) {
                CLevelSyncChild* c = new CLevelSyncChild();
                m[0x153] = (i32)c;
                c->m_3c = this;
            }
        }
    } else {
        i32 tmp = 1;
        if (op == 4) {
            s->Write(&tmp, 4);
        }
    }

    if (m[0x153] != 0) {
        if (((CLevelSync*)m[0x153])->ChildSync(s, op, p4, p5) == 0) {
            return 0;
        }
    }

    if (op == 4) {
        s->Write(&m[0x134], 8);
        s->Write(&m[0x136], 8);
    } else if (op == 7) {
        s->Read(&m[0x134], 8);
        s->Read(&m[0x136], 8);
    }
    if (op == 4) {
        s->Write(&m[0x13c], 8);
        s->Write(&m[0x13e], 8);
    } else if (op == 7) {
        s->Read(&m[0x13c], 8);
        s->Read(&m[0x13e], 8);
    }
    if (op == 4) {
        s->Write(&m[200], 8);
        s->Write(&m[0xca], 8);
    } else if (op == 7) {
        s->Read(&m[200], 8);
        s->Read(&m[0xca], 8);
    }
    if (op == 4) {
        s->Write(&m[0xce], 8);
        s->Write(&m[0xd0], 8);
    } else if (op == 7) {
        s->Read(&m[0xce], 8);
        s->Read(&m[0xd0], 8);
    }
    if (op == 4) {
        s->Write(&m[0x158], 8);
        s->Write(&m[0x15a], 8);
    } else if (op == 7) {
        s->Read(&m[0x158], 8);
        s->Read(&m[0x15a], 8);
    }

    i32* p = &m[0x8a];
    i32 n = 5;
    do {
        if (op == 4) {
            s->Write(p, 8);
            s->Write(p + 2, 8);
        } else if (op == 7) {
            s->Read(p, 8);
            s->Read(p + 2, 8);
        }
        p += 6;
        n--;
    } while (n != 0);

    n = 3;
    p = &m[0xb2];
    do {
        if (op == 4) {
            s->Write(p, 8);
            s->Write(p + 2, 8);
        } else if (op == 7) {
            s->Read(p, 8);
            s->Read(p + 2, 8);
        }
        p += 6;
        n--;
    } while (n != 0);

    i32 outer = 3;
    p = &m[0xe0];
    do {
        n = 4;
        do {
            if (op == 4) {
                s->Write(p, 8);
                s->Write(p + 2, 8);
            } else if (op == 7) {
                s->Read(p, 8);
                s->Read(p + 2, 8);
            }
            p += 6;
            n--;
        } while (n != 0);
        outer--;
    } while (outer != 0);

    if (op == 4) {
        s->Write(&m[0xa8], 8);
        s->Write(&m[0xaa], 8);
    } else if (op == 7) {
        s->Read(&m[0xa8], 8);
        s->Read(&m[0xaa], 8);
    }
    if (op == 7 && m[0] != 2) {
        PostBlockFixup();
    }

#define SER(idx)                                                                                   \
    if (SyncSub* _o = (SyncSub*)m[idx]) {                                                          \
        if (_o->Serialize(s, op, p4, p5) == 0)                                                     \
            return 0;                                                                              \
    }

    {
        i32 i = 0;
        i32* q = &m[99];
        do {
            if (SyncSub* a = (SyncSub*)q[-0xf]) {
                if (a->Serialize(s, op, p4, p5) == 0) {
                    return 0;
                }
            }
            if (SyncSub* b = (SyncSub*)*q) {
                if (b->Serialize(s, op, p4, p5) == 0) {
                    return 0;
                }
            }
            i++;
            q++;
        } while (i < 0xf);
    }
    {
        i32 i = 0;
        i32* q = &m[0x81];
        do {
            if (SyncSub* a = (SyncSub*)*q) {
                if (a->Serialize(s, op, p4, p5) == 0) {
                    return 0;
                }
            }
            i++;
            q++;
        } while (i < 5);
    }
    {
        i32 i = 0;
        i32* q = &m[0xc2];
        do {
            if (SyncSub* a = (SyncSub*)*q) {
                if (a->Serialize(s, op, p4, p5) == 0) {
                    return 0;
                }
            }
            i++;
            q++;
        } while (i < 3);
    }
    {
        i32 row = 0;
        i32* base = &m[0x126];
        do {
            i32 i = 0;
            i32* q = base;
            do {
                if (SyncSub* a = (SyncSub*)*q) {
                    if (a->Serialize(s, op, p4, p5) == 0) {
                        return 0;
                    }
                }
                i++;
                q++;
            } while (i < 4);
            row++;
            base += 4;
        } while (row < 3);
    }
    {
        i32 i = 0;
        i32* q = &m[0x187];
        do {
            if (SyncSub* a = (SyncSub*)*q) {
                if (a->Serialize(s, op, p4, p5) == 0) {
                    return 0;
                }
            }
            i++;
            q++;
        } while (i < 4);
    }

    SER(0x72)
    SER(0x73)
    SER(0x74)
    SER(0x75)
    SER(0x76)
    SER(0x77)
    SER(0x78)
    SER(0x79)
    SER(0x7a)
    SER(0x7b)
    SER(0x7c)
    SER(0x7c)
    SER(0x7d)
    SER(0x7e)
    SER(0x7f)
    SER(0x80)
    SER(0x86)
    SER(0x87)
    SER(0xd2)
    SER(0xd9)
    SER(0xda)
    SER(0xdb)
    SER(0xdc)
    SER(0x138)
    SER(0x140)
    SER(0x15c)
#undef SER

    Finalize();
    return 1;
}

SIZE_UNKNOWN(CLevelSync);
SIZE_UNKNOWN(CLevelSyncChild);
SIZE_UNKNOWN(MgrReset);
SIZE_UNKNOWN(SyncStream);
SIZE_UNKNOWN(SyncSub);
