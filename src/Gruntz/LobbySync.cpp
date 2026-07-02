// LobbySync.cpp - CLobbySync, the multiplayer-lobby grunt-sync object owned by
// CLobbyObjB (ctor 0xbf000).  Holds 4 channel slots (CCluster0c, 0x64 each) at
// +0x20, an 0x80-entry id map at +0x1b0, and an 0x80-entry grunt-record table
// (stride 0x410) at +0x3b0.  Drives per-tick reconciliation of grunt selections
// across the net session.  The slot class CCluster0c is modeled in Cluster0c.cpp;
// here we only declare the slot methods we call (declared-only -> reloc-masked).
#include <Ints.h>
#include <rva.h>
#include <Rez/RezMgr.h>
#include <Globals.h>

extern "C" void* memcpy(void* d, const void* s, unsigned int n);
extern "C" void* memset(void* d, int c, unsigned int n);

// --- The networking endpoint reached through CNetMgr::m_18.  Its dispatch table
// at +0 holds __stdcall fn ptrs (the object is the explicit 1st arg). ----------
struct CNetEndpoint;
struct CNetEndpointVtbl {
    char pad00[0x44];
    i32(__stdcall* Recv)(CNetEndpoint*, i32, i32*); // +0x44
    char pad48[0x64 - 0x48];
    i32(__stdcall* Read)(CNetEndpoint*, i32*, i32*, i32, void*, i32*); // +0x64
};
struct CNetEndpoint {
    CNetEndpointVtbl* vtbl;
};

// --- CNetMgr (only the slots we call) ------------------------------------------
struct CNetMgr {
    char pad00[0x18];
    CNetEndpoint* m_18;                             // +0x18 endpoint
    i32 SetData(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x178fc0
};

// --- A per-channel serializable grunt object (m_1b0 table element). -----------
struct CSyncObj {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual i32 Serialize(char* buf, i32 max); // slot 8 -> vtbl+0x20
};
void NoopSync(CSyncObj* p); // 0xbfb20 (empty)

// --- The recycled-node free pool drained at the tail of Reset (real MFC CPtrList
// from <Rez/RezMgr.h> -> <Mfc.h>; RemoveTail 0x1b4a27 / GetCount inline @+0xc). ---
DATA(0x0024aca8)
extern CPtrList g_pool; // 0x64aca8

// The "[end]"-tagged broadcast scratch buffer for the receive loop (0x800 B).

// --- The per-slot descriptor pointed to by m_0c --------------------------------
struct SlotInfo {
    char pad00[4];
    i32 m_04; // +0x04
    char pad08[0x18 - 0x08];
    i32 m_18; // +0x18
    char pad1c[0x2c - 0x1c];
    i32 m_2c; // +0x2c
};

// --- A grunt record in the +0x3b0 table (stride 0x410) -------------------------
struct GruntRec {
    i32 m_00;           // +0x00
    i32 m_04;           // +0x04
    unsigned char m_08; // +0x08
    char pad09[3];
    i32 m_0c;                // +0x0c payload length
    char m_10[0x410 - 0x10]; // +0x10 payload
};

// --- The per-channel slot (0x64 bytes); full model lives in Cluster0c.cpp. ----
struct CCluster0c {
    i32 m_00;       // +0x00 slot type (3 = active)
    i32 m_04;       // +0x04 flag (0 = local)
    i32 m_08;       // +0x08 counter / CNetMgr* (overloaded)
    SlotInfo* m_0c; // +0x0c descriptor
    i32 m_10;       // +0x10
    i32 m_14;       // +0x14
    i32 m_18;       // +0x18
    i32 m_1c;       // +0x1c
    char pad20[0x4c - 0x20];
    char m_4c[0x58 - 0x4c]; // +0x4c sub-object
    char m_58[0x5c - 0x58]; // +0x58 sub-object
    char pad5c[0x64 - 0x5c];

    void Init();                        // 0xc0c20 (CCluster0c::Init)
    i32 M_c0fd0(void* p, i32 v);        // 0xc0fd0
    void M_c1230(i32* a, i32* b);       // 0xc1230
    i32 M_c12b0(i32 v);                 // 0xc12b0
    void M_c11b0(i32 v);                // 0xc11b0
    i32 M_c0c70(i32 a, void* b, i32 c); // 0xc0c70
    i32 M_bfc70(i32 seq, GruntRec* rec, i32 flag, i32 slot, i32 gruntId); // 0xbfc70
    void ClearList();      // 0xc12e0 (recycle list at +0x20 to pool)
    void InitSub3c();      // 0xbf120 (zero +0x3c..0x48)
    void InitNeg(void* p); // 0xc10a0 (3 dwords of *p = -1)
};

// --- The game/session manager pointed to by CLobbySync::m_04. This is NOT the
// WAP32::CGameMgr base class (0x2c) nor a full CGruntzMgr view - it is a partial
// net/session sink whose reloc-masked callees live in the 0xba000+ net-game code
// (LoadMenuSelectSprite etc.). Named distinctly so it does not masquerade as the
// engine base CGameMgr; only the +0x564 offset + the reloc-masked call shapes are
// load-bearing. -----------------------------------------------------------------
struct CSessionMgr {
    char pad000[0x564];
    i32 m_564;                            // +0x564 net busy flag
    void LoadMenuSelectSprite(void* m);   // 0xba620
    void OnPlayerLeft(void* p);           // 0xba3b0
    void ResetPlayerCommands(void* p);    // 0xbcf20
    i32 HandleControlMsg(void* m, i32 a); // 0xba1a0
    void WriteTag(const char* tag);       // 0xbd4a0 (no-op stub)
};

// --- A control/command message (CLobbySync messages) ---------------------------
struct LobbyMsg {
    i32 m_00; // +0x00 type
    i32 m_04; // +0x04
    i32 m_08; // +0x08
};

struct CLobbySync {
    i32 m_00;
    CSessionMgr* m_04;    // +0x04
    CNetMgr* m_08;        // +0x08 CNetMgr*
    SlotInfo* m_0c;       // +0x0c
    i32 m_10;             // +0x10
    i32 m_14;             // +0x14
    i32 m_18;             // +0x18
    i32 m_1c;             // +0x1c modulus
    CCluster0c m_20[4];   // +0x20 .. +0x1b0
    i32 m_1b0[0x80];      // +0x1b0 .. +0x3b0
    GruntRec m_3b0[0x80]; // +0x3b0 grunt-record table

    // my targets
    i32 Advance();                           // 0xc01d0
    void Reconcile();                        // 0xc00f0
    i32 SendBatch();                         // 0xbfd40
    i32 SendAll();                           // 0xbfb40
    i32 Dispatch(i32 a, LobbyMsg* b, i32 c); // 0xbf700
    i32 DispatchMsg(LobbyMsg* m, i32 arg2);  // 0xbf7c0
    void Reset();                            // 0xbf000
    i32 Poll(i32 delta);                     // 0xbf5a0
    i32 Tick();                              // 0xbf9e0
    // siblings (declared-only -> reloc-masked)
    CCluster0c* M_c00a0(i32 a);        // 0xc00a0
    i32 M_c0290(i32 v);                // 0xc0290
    i32 SendOne(CCluster0c* s, i32 v); // 0xbfeb0
    i32 Checksum();                    // 0xc0590
    CSyncObj* GetSlotPtr(i32 v);       // 0xc0430
};

void ReportError(const char* file, i32 line, i32 code, i32 extra); // 0x1776a0

RVA(0x000c01d0, 0x8c)
i32 CLobbySync::Advance() {
    i32 next10 = m_10 + 1;
    i32 next18 = m_18 + 1;
    if (next10 % m_1c != 0) {
        m_10 = next10;
        return 1;
    }
    Reconcile();
    if (!M_c0290(next18)) {
        return 0;
    }
    CCluster0c* s = m_20;
    i32 n = 4;
    do {
        if (s && s->m_00 == 3 && s->m_04 == 0) {
            s->M_c11b0(m_18 - 4);
        }
        s++;
    } while (--n);
    m_10 = next10;
    m_18 = next18;
    m_14 = 0;
    return 1;
}

RVA(0x000c00f0, 0xaf)
void CLobbySync::Reconcile() {
    i32 withFlag = 0;
    i32 withoutFlag = 0;
    CCluster0c* base = m_20;
    {
        CCluster0c* s = base;
        i32 n = 4;
        do {
            if (s) {
                i32 type = s->m_00;
                if (type == 3 && s->m_04 != 0) {
                    withFlag++;
                }
                if (type == 3 && s->m_04 == 0) {
                    withoutFlag++;
                }
            }
            s++;
        } while (--n);
    }
    if (withoutFlag == 0) {
        CCluster0c* s = base;
        i32 n = 4;
        do {
            if (s && s->m_00 == 3) {
                s->Init();
                SlotInfo* p = s->m_0c;
                s->m_00 = 1;
                p->m_2c = 1;
            }
            s++;
        } while (--n);
    } else if (withFlag != 0) {
        CCluster0c* s = base;
        i32 n = 4;
        do {
            if (s && s->m_00 == 3 && s->m_04 != 0 && m_18 > s->m_08 + 2) {
                s->Init();
                SlotInfo* p = s->m_0c;
                s->m_00 = 1;
                p->m_2c = 1;
            }
            s++;
        } while (--n);
    }
}

RVA(0x000bfd40, 0x116)
i32 CLobbySync::SendBatch() {
    i32 count = 0;
    CCluster0c* s = m_20;
    i32 n = 4;
    do {
        if (s && s->m_00 == 3 && s->m_04 == 0) {
            i32 t = m_18 + 2;
            if (m_14 == 0 && (m_10 + 1) % m_1c == 0) {
                if (SendOne(s, t)) {
                    count++;
                }
            }
            i32 v = m_18 + 1;
            if (s->m_18 < v && s->M_c0fd0(&s->m_58, v) == 0) {
                if (SendOne(s, v)) {
                    count++;
                }
            }
            v = m_18;
            if (s->m_18 < v && s->M_c0fd0(&s->m_58, v) == 0) {
                if (SendOne(s, v)) {
                    count++;
                }
            }
            v = m_18 - 1;
            if (s->m_18 < v && s->M_c0fd0(&s->m_58, v) == 0) {
                if (SendOne(s, v)) {
                    count++;
                }
            }
            v = m_18 - 2;
            if (s->m_18 < v && s->M_c0fd0(&s->m_58, v) == 0) {
                if (SendOne(s, v)) {
                    count++;
                }
            }
        }
        s++;
    } while (--n);
    return count;
}

// @early-stop
// regalloc tie (~93%): logic byte-exact, retail keeps obj in eax / reads the flag
// byte into cl; cl's MSVC spills obj to ecx then reads flag into al.
RVA(0x000bf700, 0x82)
i32 CLobbySync::Dispatch(i32 a, LobbyMsg* b, i32 c) {
    if (!b) {
        return 0;
    }
    if (a == 0) {
        return DispatchMsg(b, c);
    }
    CCluster0c* obj = M_c00a0(a);
    if (!obj) {
        return 0;
    }
    obj->m_10 = 0;
    CCluster0c* target = obj;
    unsigned char* p = (unsigned char*)b;
    if (!(p[0] & 0x80) && (p[0] & 1)) {
        target = &m_20[p[1]];
        if (!target) {
            return 0;
        }
    }
    return target->M_c0c70(a, b, c);
}

// @early-stop
// jump-table-placement wall (docs/patterns/switch-jumptable-separate-comdat.md):
// code bytes byte-identical (proven llvm-objdump -dr base vs target); MSVC emits
// the 0xff-byte index table + jump table as separate $L symbols while the delinker
// folds them into the fn symbol, so the table region can't pair.
RVA(0x000bf7c0, 0x95)
i32 CLobbySync::DispatchMsg(LobbyMsg* m, i32 arg2) {
    if (!m) {
        return 0;
    }
    switch (m->m_00) {
        case 3:
            m_04->LoadMenuSelectSprite(m);
            return 1;
        case 5:
            if (m->m_04 == 1) {
                void* p = (void*)m->m_08;
                m_04->OnPlayerLeft(p);
                m_04->ResetPlayerCommands(p);
                return 1;
            }
            return 1;
        case 49:
            return m_04->HandleControlMsg(m, arg2);
        case 257:
            return m_04->HandleControlMsg(m, arg2);
        default:
            return 1;
    }
}

// @early-stop
// regalloc/spill wall (~67%): logic correct, retail spills `this` (dead slot) +
// caches &m_20[0]; this cl allocates the slot pointers differently.
RVA(0x000bfb40, 0xe2)
i32 CLobbySync::SendAll() {
    i32 count = 0;
    CCluster0c* outer = m_20;
    for (i32 oi = 0; oi < 4; oi++) {
        if (outer && outer->m_00 == 3 && outer->m_04 != 0) {
            i32 lo, hi;
            outer->M_c1230(&lo, &hi);
            CCluster0c* inner = m_20;
            i32 in = 4;
            do {
                if (inner && inner->m_00 == 3 && inner->m_04 == 0) {
                    for (i32 v = lo; v <= hi; v++) {
                        i32 r = outer->M_c12b0(v);
                        if (r) {
                            i32 flag = (v == hi) ? 3 : 1;
                            if (m_20[0].M_bfc70(v, (GruntRec*)r, flag, oi, inner->m_0c->m_18)) {
                                count++;
                            }
                        }
                    }
                }
                inner++;
            } while (--in);
        }
        outer++;
    }
    return count;
}

// The two outgoing-packet staging buffers (BSS).  Ghidra split each into one
// DAT symbol per written field offset, so we declare each offset as its own
// labeled extern for the abs stores to reloc-pair.

// @early-stop
// regalloc cascade (~84%): logic byte-exact; ecx/edx + esi/eax allocation for the
// modulo index and arg differ, plus the M_c0fd0 sibling (Boundary_0c0fd0) reloc.
RVA(0x000bfeb0, 0xfa)
i32 CLobbySync::SendOne(CCluster0c* slot, i32 val) {
    if (!slot) {
        return 0;
    }
    if (val < 0) {
        return 1;
    }
    unsigned char flags = 0;
    i32 m14 = slot->m_14;
    if (slot->M_c0fd0(&slot->m_4c, m14 + 2)) {
        flags = 0x10;
    }
    if (slot->M_c0fd0(&slot->m_4c, m14 + 3)) {
        flags |= 0x20;
    }
    gB_flag = flags;
    gB_val = val;
    i32 idx = val % 0x80;
    GruntRec* entry = &m_3b0[idx];
    gB_m14 = slot->m_14;
    gB_e04 = entry->m_04;
    gB_e08 = entry->m_08;
    memcpy(&gB_data, entry->m_10, entry->m_0c);
    return m_08->SetData(m_0c->m_04, slot->m_0c->m_18, 0, (i32)&gB_flag, entry->m_0c + 0xe) == 0;
}

// @early-stop
// one-register regalloc tie (~90%): byte-identical except retail holds `seq` in
// esi where this cl holds it in ecx (mov esi/ecx,[esp+0x10]); non-steerable.
// 0xbfc70  (CCluster0c method: this == the channel base slot)
RVA(0x000bfc70, 0x9c)
i32 CCluster0c::M_bfc70(i32 seq, GruntRec* rec, i32 flag, i32 slot, i32 gruntId) {
    if (!rec) {
        return 0;
    }
    if (seq < 0) {
        return 1;
    }
    gA_seq = seq;
    gA_flag = (unsigned char)flag;
    gA_slot = (unsigned char)slot;
    gA_e04 = rec->m_04;
    gA_e08 = rec->m_08;
    memcpy(&gA_data, rec->m_10, rec->m_0c);
    return ((CNetMgr*)m_08)->SetData(m_0c->m_04, gruntId, 0, (i32)&gA_flag, rec->m_0c + 0xf) == 0;
}

// @early-stop
// regalloc/scheduling wall (~85%): instruction sequence byte-faithful, but retail
// centers the per-slot store base at slot+8 (keeping edi=slot for the thiscall
// `this`) and spills the loop counter to [esp+0x10]; this cl uses one pointer
// (esi=slot) + counter in edi.  Same store order/values; callees+pool reloc-masked.
// 0xbf000  Reset: recycle each channel slot, clear the id-map + record table,
// then drain the recycled-node free pool.
RVA(0x000bf000, 0xd5)
void CLobbySync::Reset() {
    m_00 = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    m_1c = 1;
    CCluster0c* s = m_20;
    i32 n = 4;
    do {
        s->m_04 = 0;
        s->m_08 = 0;
        s->m_00 = 0;
        s->m_0c = 0;
        s->m_10 = 0;
        s->m_14 = 0;
        s->m_18 = 0;
        s->m_1c = 0;
        s->ClearList();
        s->InitSub3c();
        s->InitNeg(&s->m_4c);
        s->InitNeg(&s->m_58);
        s++;
    } while (--n);
    for (i32 j = 0; j < 0x80; j++) {
        m_1b0[j] = 0;
    }
    GruntRec* r = m_3b0;
    i32 k = 0x80;
    do {
        r->m_00 = 0;
        r->m_08 = 0;
        r->m_0c = 0;
        r->m_04 = 0;
        r++;
    } while (--k);
    while (g_pool.GetCount() != 0) {
        void* p = g_pool.RemoveTail();
        if (p) {
            RezFree(p);
        }
    }
}

// @early-stop
// regalloc cascade (~63%): logic byte-faithful; retail pins `this` in edi, hoists
// the slot `== 3` test constant into esi, and parks `len` in the reused incoming
// arg slot (3 distinct stack locals).  This cl pins `this` in esi and coalesces a
// stack local into a register; the consequent register renames cascade the body.
// 0xbf5a0  Poll: advance active slots by `delta`, then drain the endpoint's
// incoming packet queue, dispatching foreign packets.
RVA(0x000bf5a0, 0x110)
i32 CLobbySync::Poll(i32 delta) {
    CCluster0c* s = m_20;
    i32 n = 4;
    do {
        if (s->m_00 == 3) {
            s->m_10 += delta;
        }
        s++;
    } while (--n);

    i32 avail;
    if (m_0c == 0) {
        avail = 0;
    } else {
        i32 got;
        CNetEndpoint* ep = m_08->m_18;
        i32 r = ep->vtbl->Recv(ep, m_0c->m_04, &got);
        avail = (r == 0) ? got : 0;
    }

    i32 a = 0;
    i32 received = 0;
    while (avail > 0 && m_04->m_564 == 0) {
        i32 len = 0x800;
        i32 chan = m_0c->m_04;
        CNetEndpoint* ep = m_08->m_18;
        i32 st = ep->vtbl->Read(ep, &a, &chan, 1, g_649858, &len);
        if (st != 0) {
            ReportError("c:\\proj\\incs\\netmgr.h", 0x141, st, 0);
            if (st != 0) {
                break;
            }
        }
        received++;
        avail--;
        if (a != m_0c->m_04) {
            Dispatch(a, (LobbyMsg*)g_649858, len);
        }
    }
    return received;
}

// @early-stop
// regalloc tie (~85%): instruction sequence byte-identical except retail holds
// `this` in ebp where this cl holds it in ebx (cascading esi/edi/ebx renames);
// the callee-save register pick is function-specific and non-steerable.
// 0xbf9e0  Tick: at the reconcile boundary, snapshot every channel's grunt
// state into the current record, broadcast, then flush the pending batches.
RVA(0x000bf9e0, 0xfe)
i32 CLobbySync::Tick() {
    if (m_14 == 0 && (m_10 + 1) % m_1c == 0) {
        i32 seq = m_18 + 2;
        GruntRec* rec = &m_3b0[seq % 0x80];
        rec->m_00 = seq;
        rec->m_0c = 0;
        rec->m_08 = 0;
        rec->m_04 = Checksum();
        char* payload = rec->m_10;
        i32 next = seq + 1;
        for (i32 t = seq * m_1c; t < next * m_1c; t++) {
            CSyncObj* obj = GetSlotPtr(t);
            if (obj) {
                NoopSync(obj);
                rec->m_08++;
                payload += obj->Serialize(payload, (char*)rec - payload + 0x410);
            }
        }
        m_04->WriteTag("[end]\n");
        rec->m_0c = (i32)(payload - (char*)rec - 0x10);
        m_14 = 1;
    }
    return SendBatch() + SendAll();
}

SIZE_UNKNOWN(CLobbySync);
SIZE_UNKNOWN(CNetEndpoint);
SIZE_UNKNOWN(CNetEndpointVtbl);
SIZE_UNKNOWN(CSyncObj);
SIZE_UNKNOWN(GruntRec);
SIZE_UNKNOWN(LobbyMsg);
SIZE_UNKNOWN(SlotInfo);
