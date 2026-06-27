// LobbySync.cpp - CLobbySync, the multiplayer-lobby grunt-sync object owned by
// CLobbyObjB (ctor 0xbf000).  Holds 4 channel slots (CCluster0c, 0x64 each) at
// +0x20, an 0x80-entry id map at +0x1b0, and an 0x80-entry grunt-record table
// (stride 0x410) at +0x3b0.  Drives per-tick reconciliation of grunt selections
// across the net session.  The slot class CCluster0c is modeled in Cluster0c.cpp;
// here we only declare the slot methods we call (declared-only -> reloc-masked).
#include <Ints.h>
#include <rva.h>

extern "C" void* memcpy(void* d, const void* s, unsigned int n);

// --- CNetMgr (only the slot we call) -------------------------------------------
struct CNetMgr {
    i32 SetData(i32 a, i32 b, i32 c, i32 d, i32 e);  // 0x178fc0
};

// --- The per-slot descriptor pointed to by m_0c --------------------------------
struct SlotInfo {
    char pad00[4];
    i32 m_04;   // +0x04
    char pad08[0x18 - 0x08];
    i32 m_18;   // +0x18
    char pad1c[0x2c - 0x1c];
    i32 m_2c;   // +0x2c
};

// --- A grunt record in the +0x3b0 table (stride 0x410) -------------------------
struct GruntRec {
    i32 m_00;         // +0x00
    i32 m_04;         // +0x04
    unsigned char m_08;  // +0x08
    char pad09[3];
    i32 m_0c;         // +0x0c payload length
    char m_10[0x410 - 0x10];  // +0x10 payload
};

// --- The per-channel slot (0x64 bytes); full model lives in Cluster0c.cpp. ----
struct CCluster0c {
    i32 m_00;          // +0x00 slot type (3 = active)
    i32 m_04;          // +0x04 flag (0 = local)
    i32 m_08;          // +0x08 counter / CNetMgr* (overloaded)
    SlotInfo* m_0c;    // +0x0c descriptor
    i32 m_10;          // +0x10
    i32 m_14;          // +0x14
    i32 m_18;          // +0x18
    char pad1c[0x4c - 0x1c];
    char m_4c[0x58 - 0x4c];  // +0x4c sub-object
    char m_58[0x5c - 0x58];  // +0x58 sub-object
    char pad5c[0x64 - 0x5c];

    void Init();                         // 0xc0c20 (CCluster0c::Init)
    i32 M_c0fd0(void* p, i32 v);         // 0xc0fd0
    void M_c1230(i32* a, i32* b);        // 0xc1230
    i32 M_c12b0(i32 v);                  // 0xc12b0
    void M_c11b0(i32 v);                 // 0xc11b0
    i32 M_c0c70(i32 a, void* b, i32 c);  // 0xc0c70
    i32 M_bfc70(i32 seq, GruntRec* rec, i32 flag, i32 slot, i32 gruntId); // 0xbfc70
};

// --- The game/session manager pointed to by CLobbySync::m_04 -------------------
struct CGameMgr {
    void LoadMenuSelectSprite(void* m);    // 0xba620
    void OnPlayerLeft(void* p);            // 0xba3b0
    void ResetPlayerCommands(void* p);     // 0xbcf20
    i32 HandleControlMsg(void* m, i32 a);  // 0xba1a0
};

// --- A control/command message (CLobbySync messages) ---------------------------
struct LobbyMsg {
    i32 m_00;   // +0x00 type
    i32 m_04;   // +0x04
    i32 m_08;   // +0x08
};

struct CLobbySync {
    i32 m_00;
    CGameMgr* m_04;     // +0x04
    CNetMgr* m_08;      // +0x08 CNetMgr*
    SlotInfo* m_0c;     // +0x0c
    i32 m_10;           // +0x10
    i32 m_14;           // +0x14
    i32 m_18;           // +0x18
    i32 m_1c;           // +0x1c modulus
    CCluster0c m_20[4]; // +0x20 .. +0x1b0
    i32 m_1b0[0x80];    // +0x1b0 .. +0x3b0
    GruntRec m_3b0[0x80];  // +0x3b0 grunt-record table

    // my targets
    i32 Advance();                                // 0xc01d0
    void Reconcile();                             // 0xc00f0
    i32 SendBatch();                              // 0xbfd40
    i32 SendAll();                                // 0xbfb40
    i32 Dispatch(i32 a, LobbyMsg* b, i32 c);      // 0xbf700
    i32 DispatchMsg(LobbyMsg* m, i32 arg2);       // 0xbf7c0
    // siblings (declared-only -> reloc-masked)
    CCluster0c* M_c00a0(i32 a);                   // 0xc00a0
    i32 M_c0290(i32 v);                           // 0xc0290
    i32 SendOne(CCluster0c* s, i32 v);            // 0xbfeb0
};

// 0xc01d0
RVA(0x000c01d0, 0x8c)
i32 CLobbySync::Advance() {
    i32 next10 = m_10 + 1;
    i32 next18 = m_18 + 1;
    if (next10 % m_1c != 0) {
        m_10 = next10;
        return 1;
    }
    Reconcile();
    if (!M_c0290(next18))
        return 0;
    CCluster0c* s = m_20;
    i32 n = 4;
    do {
        if (s && s->m_00 == 3 && s->m_04 == 0)
            s->M_c11b0(m_18 - 4);
        s++;
    } while (--n);
    m_10 = next10;
    m_18 = next18;
    m_14 = 0;
    return 1;
}

// 0xc00f0
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
                if (type == 3 && s->m_04 != 0)
                    withFlag++;
                if (type == 3 && s->m_04 == 0)
                    withoutFlag++;
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

// 0xbfd40
RVA(0x000bfd40, 0x116)
i32 CLobbySync::SendBatch() {
    i32 count = 0;
    CCluster0c* s = m_20;
    i32 n = 4;
    do {
        if (s && s->m_00 == 3 && s->m_04 == 0) {
            i32 t = m_18 + 2;
            if (m_14 == 0 && (m_10 + 1) % m_1c == 0) {
                if (SendOne(s, t))
                    count++;
            }
            i32 v = m_18 + 1;
            if (s->m_18 < v && s->M_c0fd0(&s->m_58, v) == 0) {
                if (SendOne(s, v))
                    count++;
            }
            v = m_18;
            if (s->m_18 < v && s->M_c0fd0(&s->m_58, v) == 0) {
                if (SendOne(s, v))
                    count++;
            }
            v = m_18 - 1;
            if (s->m_18 < v && s->M_c0fd0(&s->m_58, v) == 0) {
                if (SendOne(s, v))
                    count++;
            }
            v = m_18 - 2;
            if (s->m_18 < v && s->M_c0fd0(&s->m_58, v) == 0) {
                if (SendOne(s, v))
                    count++;
            }
        }
        s++;
    } while (--n);
    return count;
}

// @early-stop
// regalloc tie (~93%): logic byte-exact, retail keeps obj in eax / reads the flag
// byte into cl; cl's MSVC spills obj to ecx then reads flag into al.
// 0xbf700
RVA(0x000bf700, 0x82)
i32 CLobbySync::Dispatch(i32 a, LobbyMsg* b, i32 c) {
    if (!b)
        return 0;
    if (a == 0)
        return DispatchMsg(b, c);
    CCluster0c* obj = M_c00a0(a);
    if (!obj)
        return 0;
    obj->m_10 = 0;
    CCluster0c* target = obj;
    unsigned char* p = (unsigned char*)b;
    if (!(p[0] & 0x80) && (p[0] & 1)) {
        target = &m_20[p[1]];
        if (!target)
            return 0;
    }
    return target->M_c0c70(a, b, c);
}

// @early-stop
// jump-table-placement wall (docs/patterns/switch-jumptable-separate-comdat.md):
// code bytes byte-identical (proven llvm-objdump -dr base vs target); MSVC emits
// the 0xff-byte index table + jump table as separate $L symbols while the delinker
// folds them into the fn symbol, so the table region can't pair.
// 0xbf7c0
RVA(0x000bf7c0, 0x95)
i32 CLobbySync::DispatchMsg(LobbyMsg* m, i32 arg2) {
    if (!m)
        return 0;
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
// 0xbfb40
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
                            if (m_20[0].M_bfc70(v, (GruntRec*)r, flag, oi, inner->m_0c->m_18))
                                count++;
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
DATA(0x0024a058) extern unsigned char gB_flag;  // 0x64a058
DATA(0x0024a059) extern i32 gB_val;             // 0x64a059
DATA(0x0024a05d) extern i32 gB_m14;             // 0x64a05d
DATA(0x0024a061) extern i32 gB_e04;             // 0x64a061
DATA(0x0024a065) extern unsigned char gB_e08;   // 0x64a065
DATA(0x0024a066) extern unsigned char gB_data;  // 0x64a066

DATA(0x0024a8a8) extern unsigned char gA_flag;  // 0x64a8a8
DATA(0x0024a8a9) extern unsigned char gA_slot;  // 0x64a8a9
DATA(0x0024a8aa) extern i32 gA_seq;             // 0x64a8aa
DATA(0x0024a8b2) extern i32 gA_e04;             // 0x64a8b2
DATA(0x0024a8b6) extern unsigned char gA_e08;   // 0x64a8b6
DATA(0x0024a8b7) extern unsigned char gA_data;  // 0x64a8b7

// @early-stop
// regalloc cascade (~84%): logic byte-exact; ecx/edx + esi/eax allocation for the
// modulo index and arg differ, plus the M_c0fd0 sibling (Boundary_0c0fd0) reloc.
// 0xbfeb0
RVA(0x000bfeb0, 0xfa)
i32 CLobbySync::SendOne(CCluster0c* slot, i32 val) {
    if (!slot)
        return 0;
    if (val < 0)
        return 1;
    unsigned char flags = 0;
    i32 m14 = slot->m_14;
    if (slot->M_c0fd0(&slot->m_4c, m14 + 2))
        flags = 0x10;
    if (slot->M_c0fd0(&slot->m_4c, m14 + 3))
        flags |= 0x20;
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
    if (!rec)
        return 0;
    if (seq < 0)
        return 1;
    gA_seq = seq;
    gA_flag = (unsigned char)flag;
    gA_slot = (unsigned char)slot;
    gA_e04 = rec->m_04;
    gA_e08 = rec->m_08;
    memcpy(&gA_data, rec->m_10, rec->m_0c);
    return ((CNetMgr*)m_08)->SetData(m_0c->m_04, gruntId, 0, (i32)&gA_flag, rec->m_0c + 0xf) == 0;
}
