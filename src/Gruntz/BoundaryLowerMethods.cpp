// BoundaryLowerMethods.cpp - small leaf methods recovered from the engine_boundary
// backlog (lower half, RVA < 0x133370). RTTI cannot attribute these COMDAT-folded
// methods, so the owning class names here are placeholders; only the OFFSETS +
// code bytes are load-bearing. Unmodeled engine callees/globals are declared
// NO-body so their rel32/DIR32 operands reloc-mask. Defined in retail-RVA order.
#include <Ints.h>
#include <rva.h>

// ===========================================================================
// 0x0213a0 - virtual-base field getter: read the field at +0x04 of the virtual
// base whose displacement lives in the vbtable's second slot. __thiscall.
// ===========================================================================
struct C213a0 {
    i32 Get();
};
RVA(0x000213a0, 0xa)
i32 C213a0::Get() {
    i32* vb = *(i32**)this;
    i32 disp = vb[1];
    return *(i32*)((char*)this + disp + 4);
}

// ===========================================================================
// 0x0464e0 - type-id -> entry resolver (the projectile/act fast-range + Find +
// grow-on-miss lookup; same shape as TypeKeyColl's TypeResolve). __thiscall(key).
// ===========================================================================
struct CKSlimeColl464 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (ret 0xc)
};
extern void* g_projActCache;       // 0x6bf464 (pinned in CStaticHazard.cpp)
extern void* g_projActAllocResult; // 0x6bf428 (pinned in CVoiceTrigger.cpp)
extern "C" i32 ProjActAlloc();     // 0x16d990
struct CTypeColl464 {
    void* m_0;            // +0x00
    CKSlimeColl464* m_4;  // +0x04
    i32 m_8;              // +0x08  lo
    i32 m_c;              // +0x0c  hi
    char* m_10;           // +0x10  base
    i32 m_14;             // +0x14  miss result
    i32 m_18;             // +0x18  stride
    char pad1c[0x20 - 0x1c];
    i32 m_20;             // +0x20
    i32 Find(i32 key, i32 z); // 0x16da80
    void* Resolve(i32 key);
};
// @early-stop
// esi/edi regalloc wall: cl assigns this->esi, key->edi; retail swaps (key->esi,
// this->edi). Full fast-range/Find/grow logic + offsets byte-faithful (same shape as
// TypeKeyColl::TypeResolve); the esi/edi assignment is not source-steerable.
RVA(0x000464e0, 0x74)
void* CTypeColl464::Resolve(i32 key) {
    m_20 = 0;
    if (key >= m_8 && key <= m_c) {
        return m_10 + (key - m_8) * m_18;
    }
    if (Find(key, 0)) {
        return m_10 + (key - m_8) * m_18;
    }
    void* item = g_projActCache;
    g_projActAllocResult = (void*)ProjActAlloc();
    m_4->Insert(this, item, 0xc);
    return (void*)m_14;
}

// ===========================================================================
// 0x050ca0 - dispatch then reset the +0x1a0/+0x1a4 pair. __thiscall(arg).
// ===========================================================================
struct C50ca0 {
    char pad0[0x1a0];
    i32 m_1a0; // +0x1a0
    i32 m_1a4; // +0x1a4
    void Method(i32 a, i32 b, i32 c, i32 d); // 0x3bd9
    void M(i32 arg);
};
RVA(0x00050ca0, 0x2b)
void C50ca0::M(i32 arg) {
    Method(arg, 0, 0, 0);
    m_1a0 = -1;
    m_1a4 = 0;
}

// ===========================================================================
// 0x077dc0 - cell setter: m_20[ m_24[idx] + base ] = value. __thiscall(3 args).
// ===========================================================================
struct C77dc0 {
    char pad0[0x20];
    i32* m_20; // +0x20
    i32* m_24; // +0x24
    void Set(i32 base, i32 idx, i32 value);
};
RVA(0x00077dc0, 0x1d)
void C77dc0::Set(i32 base, i32 idx, i32 value) {
    m_20[m_24[idx] + base] = value;
}

// ===========================================================================
// 0x08e880 - debug command hook: if the +0x2c sub-object's state slot (vtbl +0x10)
// reports 3, register the DEBUG_SETSKILL command. __thiscall, returns 0.
// ===========================================================================
struct CState8e {
    virtual i32 v0();
    virtual i32 v1();
    virtual i32 v2();
    virtual i32 v3();
    virtual i32 GetState(); // slot +0x10
};
extern void Lab401947(); // 0x401947 (code address passed as a ptr; reloc-masked)
struct C8e880 {
    char pad0[0x2c];
    CState8e* m_2c; // +0x2c
    void Cmd2bb7(const char* name, void* fn, i32 n); // 0x2bb7
    i32 M();
};
RVA(0x0008e880, 0x27)
i32 C8e880::M() {
    if (m_2c->GetState() == 3) {
        Cmd2bb7("DEBUG_SETSKILL", (void*)&Lab401947, 1);
    }
    return 0;
}

// ===========================================================================
// 0x0915d0 / 0x091620 - guarded dispatch: when +0x48 and +0x14 are live and the
// +0x48 sub's +0x1c probe (0x138f60) succeeds, hand (const, arg) to its handler
// (0x138fd0). __thiscall(arg). The two differ only in the constant (0 vs 0x64).
// ===========================================================================
struct CInner915 {
    i32 Probe138f60();           // 0x138f60
    void Do138fd0(i32 a, i32 b); // 0x138fd0
};
struct CMid915 {
    char pad0[0x1c];
    CInner915* m_1c; // +0x1c
};
struct C915d0 {
    char pad0[0x14];
    void* m_14; // +0x14
    char pad18[0x48 - 0x18];
    CMid915* m_48; // +0x48
    void M0(void* arg);
    void M64(void* arg);
};
RVA(0x000915d0, 0x3f)
void C915d0::M0(void* arg) {
    if (m_48 == 0) {
        return;
    }
    if (m_14 == 0) {
        return;
    }
    i32 ok;
    if (m_48->m_1c != 0) {
        ok = m_48->m_1c->Probe138f60();
    } else {
        ok = 0;
    }
    if (ok == 0) {
        return;
    }
    if (m_48->m_1c == 0) {
        return;
    }
    m_48->m_1c->Do138fd0(0, (i32)arg);
}
RVA(0x00091620, 0x3f)
void C915d0::M64(void* arg) {
    if (m_48 == 0) {
        return;
    }
    if (m_14 == 0) {
        return;
    }
    i32 ok;
    if (m_48->m_1c != 0) {
        ok = m_48->m_1c->Probe138f60();
    } else {
        ok = 0;
    }
    if (ok == 0) {
        return;
    }
    if (m_48->m_1c == 0) {
        return;
    }
    m_48->m_1c->Do138fd0(0x64, (i32)arg);
}

// ===========================================================================
// 0x099ba0 - ctor: build the +0x04 sub-object (0x1b4867, arg 0xa), seed +0x20 = 0,
// +0x24 = -1, +0x00 = 0; return this. __thiscall.
// ===========================================================================
struct CSub99ba0 {
    char pad0[0x1c];
    i32 m_1c; // +0x1c (== owner +0x20)
    i32 m_20; // +0x20 (== owner +0x24)
    void Init(i32 n); // 0x1b4867
};
struct C99ba0 {
    void* m_0;       // +0x00
    CSub99ba0 m_sub; // +0x04
    C99ba0* Ctor();
};
RVA(0x00099ba0, 0x29)
C99ba0* C99ba0::Ctor() {
    CSub99ba0* sub = &m_sub;
    sub->Init(0xa);
    sub->m_1c = 0;
    sub->m_20 = -1;
    m_0 = 0;
    return this;
}

// ===========================================================================
// 0x09a420 - walk the +0x04 linked list; for each node clear the back-pointer's
// +0x04 (node->m_8->m_4 = 0). __thiscall.
// ===========================================================================
struct CBack9a420 {
    char pad0[4];
    i32 m_4; // +0x04
};
struct CNode9a420 {
    CNode9a420* m_next; // +0x00
    char pad4[8 - 4];
    CBack9a420* m_8; // +0x08
};
struct C9a420 {
    char pad0[4];
    CNode9a420* m_head; // +0x04
    void Clear();
};
RVA(0x0009a420, 0x1c)
void C9a420::Clear() {
    CNode9a420* p = m_head;
    if (p == 0) {
        return;
    }
    do {
        CNode9a420* node = p;
        p = node->m_next;
        CBack9a420* b = node->m_8;
        if (b != 0) {
            b->m_4 = 0;
        }
    } while (p != 0);
}

// ===========================================================================
// 0x09cab0 - out-param wrapper: call the +0x10 sub's method (0x1b8008) with a
// zeroed local and return the filled local. __thiscall(arg).
// ===========================================================================
struct CSub9cab0 {
    void Get(i32 a, i32* out); // 0x1b8008
};
struct C9cab0 {
    char pad0[0x10];
    CSub9cab0 m_10; // +0x10
    i32 M(i32 arg);
};
RVA(0x0009cab0, 0x23)
i32 C9cab0::M(i32 arg) {
    i32 local = 0;
    m_10.Get(arg, &local);
    return local;
}

// ===========================================================================
// 0x0b4c40 - dispatch a 4-arg action (0x3035); on success, when arg2 == 8, arm the
// +0x10 sub-object (+0x58 = 1, +0x50 = arg2, +0x54 = 0x80). __thiscall, ret 0x10.
// ===========================================================================
struct CSubB4 {
    char pad0[0x50];
    i32 m_50; // +0x50
    i32 m_54; // +0x54
    i32 m_58; // +0x58
};
struct C0b4c40 {
    char pad0[0x10];
    CSubB4* m_10; // +0x10
    i32 Dispatch3035(i32 a, i32 b, i32 c, i32 d); // 0x3035
    i32 Handle(i32 a1, i32 a2, i32 a3, i32 a4);
};
RVA(0x000b4c40, 0x4b)
i32 C0b4c40::Handle(i32 a1, i32 a2, i32 a3, i32 a4) {
    if (!Dispatch3035(a1, a2, a3, a4)) {
        return 0;
    }
    if (a2 == 8) {
        CSubB4* s = m_10;
        s->m_58 = 1;
        s->m_50 = a2;
        s->m_54 = 0x80;
    }
    return 1;
}

// ===========================================================================
// 0x0bd450 - init: run the base ctor (0x3625) then open the "c:\gruntz.log" log
// (0x1983). __thiscall.
// ===========================================================================
struct Cbd450 {
    void Base3625();                 // 0x3625
    void OpenLog1983(const char* s); // 0x1983
    void Init();
};
RVA(0x000bd450, 0x16)
void Cbd450::Init() {
    Base3625();
    OpenLog1983("c:\\gruntz.log");
}

// ===========================================================================
// 0x0cef50 - teardown: flush the +0x04 owner's +0xc8 sub (0x1b9c69); when +0x1c0
// is live, run the +0x0c->+0x04 close (0x158d20 -> 0x158e40) and dispatch +0x04's
// 0x201d(3). __thiscall, returns 1.
// ===========================================================================
struct CSubC8 {
    void M1b9c69(); // 0x1b9c69
};
struct CObjC {
    void M201d(i32 a); // 0x201d
};
struct CInnerC {
    i32 M158d20();  // 0x158d20
    void M158e40(); // 0x158e40
};
struct CMidC {
    char pad0[4];
    CInnerC* m_4; // +0x04
};
struct Ccef50 {
    char pad0[4];
    char* m_4; // +0x04
    char pad8[0xc - 8];
    CMidC* m_c; // +0x0c
    char pad10[0x1c0 - 0x10];
    i32 m_1c0; // +0x1c0
    i32 M();
};
RVA(0x000cef50, 0x46)
i32 Ccef50::M() {
    ((CSubC8*)(m_4 + 0xc8))->M1b9c69();
    if (m_1c0 != 0) {
        if (m_c->m_4->M158d20() != 0) {
            m_c->m_4->M158e40();
        }
        ((CObjC*)m_4)->M201d(3);
    }
    return 1;
}

// ===========================================================================
// 0x0d5e20 - forward an arg through two virtuals (vtbl +0x3c then +0x40).
// __thiscall(arg).
// ===========================================================================
struct Cd5e20 {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15(void* a); // slot +0x3c
    virtual void v16(void* a); // slot +0x40
    void M(void* arg);
};
RVA(0x000d5e20, 0x1b)
void Cd5e20::M(void* arg) {
    v15(arg);
    v16(arg);
}

// ===========================================================================
// 0x0db200 - swap the +0x08 holder to `arg`: no-op when already equal, else
// validate (0x11f9), toggle old off / new on (0x3bbb) and store. __thiscall.
// ===========================================================================
extern "C" i32 Check11f9(void* p);          // 0x11f9
extern "C" void Toggle3bbb(void* p, i32 f); // 0x3bbb
struct Cdb200 {
    char pad0[8];
    void* m_8; // +0x08
    i32 M(void* arg);
};
RVA(0x000db200, 0x51)
i32 Cdb200::M(void* arg) {
    if (m_8 == arg) {
        return 1;
    }
    if (Check11f9(arg)) {
        Toggle3bbb(m_8, 1);
        Toggle3bbb(arg, 0);
        m_8 = arg;
        return 1;
    }
    return 0;
}

// ===========================================================================
// 0x0db2f0 - finalize: when +0x20 is live, run the +0x38 teardown (0x40c5) iff
// +0x14 is clear, then reset +0x20. __thiscall, returns 1/0.
// ===========================================================================
struct CSubdb2f0 {
    void Teardown40c5(); // 0x40c5
};
struct Cdb2f0 {
    char pad0[0x14];
    i32 m_14; // +0x14
    char pad18[0x20 - 0x18];
    i32 m_20; // +0x20
    char pad24[0x38 - 0x24];
    CSubdb2f0 m_38; // +0x38
    i32 M();
};
RVA(0x000db2f0, 0x2b)
i32 Cdb2f0::M() {
    if (m_20 == 0) {
        return 0;
    }
    if (m_14 == 0) {
        m_38.Teardown40c5();
    }
    m_20 = 0;
    return 1;
}

// ===========================================================================
// 0x0db750 - "LEVEL" config sync: on first call (arg==0) probe the +0x0c owner's
// +0x2c config for "LEVEL" (0x152c50); set it (0x1527d0), then resolve the +0x28
// parser entry (0x13bae0) and, if found, bind it back (0x152ad0). __thiscall.
// ===========================================================================
struct CCfgdb {
    void* Get152c50(const char* key);                     // 0x152c50
    void Set1527d0(const char* key, void* v);             // 0x1527d0
    void Bind152ad0(void* val, const char* key, void* v); // 0x152ad0
};
struct CHolderdb {
    char pad0[0x2c];
    CCfgdb* m_2c; // +0x2c
};
struct CParserdb {
    i32 Resolve13bae0(void* arg); // 0x13bae0
};
DATA(0x0020b588)
extern u8 g_dat60b588; // 0x60b588 (new pin)
DATA(0x00213054)
extern u8 g_dat613054; // 0x613054 (new pin)
struct Cdb750 {
    char pad0[0xc];
    CHolderdb* m_c; // +0x0c
    char pad10[0x28 - 0x10];
    CParserdb* m_28; // +0x28
    i32 M(void* arg);
};
RVA(0x000db750, 0x70)
i32 Cdb750::M(void* arg) {
    if (m_c == 0) {
        return 0;
    }
    if (arg == 0) {
        if (m_c->m_2c->Get152c50("LEVEL") != 0) {
            return 1;
        }
    }
    m_c->m_2c->Set1527d0("LEVEL", &g_dat60b588);
    void* e = (void*)m_28->Resolve13bae0(&g_dat613054);
    if (e == 0) {
        return 0;
    }
    m_c->m_2c->Bind152ad0(e, "LEVEL", &g_dat60b588);
    return 1;
}

// ===========================================================================
// 0x0ea170 - 2-bit selector over a +0x38 virtual: pick one of four fixed arg
// tuples by (arg1!=0, arg2!=0). __thiscall(arg1, arg2).
// ===========================================================================
struct Cea170 {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void Dispatch(i32 a, i32 b, i32 c, i32 d, i32 e); // slot +0x38
    void M(i32 a1, i32 a2);
};
RVA(0x000ea170, 0x5c)
void Cea170::M(i32 a1, i32 a2) {
    if (a1 == 0) {
        if (a2 == 0) {
            Dispatch(1, -1, 0, 0, -1);
        } else {
            Dispatch(-1, -1, -1, 0, -1);
        }
    } else {
        if (a2 == 0) {
            Dispatch(4, -1, 0, 0, -1);
        } else {
            Dispatch(-1, -1, 1, 0, -1);
        }
    }
}

// ===========================================================================
// 0x0eb970 - Serialize: when the manager is live, transfer +0x3c via arg1's
// vtbl +0x30 (mode 4) or +0x2c (mode 7), then chain the base serializer (0x3ca1)
// normalized to a bool. __thiscall(ar, mode, a3, a4). ret 0x10.
// ===========================================================================
struct CArchiveEb {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void Xfer2c(void* p, i32 n); // slot +0x2c
    virtual void Xfer30(void* p, i32 n); // slot +0x30
};
struct CMgrEb {
    char pad0[0x30];
    void* m_30; // +0x30
};
extern "C" CMgrEb* g_mgrSettings;
struct Ceb970 {
    char pad0[0x3c];
    i32 m_3c; // +0x3c
    i32 Base3ca1(CArchiveEb* ar, i32 a2, i32 a3, i32 a4); // 0x3ca1
    i32 Serialize(CArchiveEb* ar, i32 mode, i32 a3, i32 a4);
};
// @early-stop
// block-layout wall: the mode==4 Xfer30 branch lands inline (jne-skip) but retail
// floats it to the tail (forward je). All transfers, the base-chain call and the
// neg/sbb/neg bool are byte-faithful.
RVA(0x000eb970, 0x72)
i32 Ceb970::Serialize(CArchiveEb* ar, i32 mode, i32 a3, i32 a4) {
    if (ar == 0) {
        return 0;
    }
    if (g_mgrSettings->m_30 == 0) {
        return 0;
    }
    if (mode == 4) {
        ar->Xfer30(&m_3c, 4);
    } else if (mode == 7) {
        ar->Xfer2c(&m_3c, 4);
    }
    return Base3ca1(ar, mode, a3, a4) != 0;
}

// ===========================================================================
// 0x0fa150 - release the four owned blits (+0x160/+0x164/+0x14/+0x18) through the
// +0x0c owner's +0x1c allocator (0x142160), then clear +0x3c. __thiscall.
// ===========================================================================
struct CFreer142160 {
    void Free(void* p); // 0x142160
};
struct CMidFa {
    char pad0[0x1c];
    CFreer142160* m_1c; // +0x1c
};
struct Cfa150 {
    char pad0[0xc];
    CMidFa* m_c; // +0x0c
    char pad10[0x14 - 0x10];
    void* m_14; // +0x14
    void* m_18; // +0x18
    char pad1c[0x3c - 0x1c];
    i32 m_3c; // +0x3c
    char pad40[0x160 - 0x40];
    void* m_160; // +0x160
    void* m_164; // +0x164
    void Cleanup();
};
// @early-stop
// cmp-operand-order wall: retail emits cmp val,edi (val vs the zeroed edi); cl emits
// cmp edi,val. Same semantics, 1 byte per guard. All four frees + offsets byte-faithful.
RVA(0x000fa150, 0x74)
void Cfa150::Cleanup() {
    if (m_c != 0) {
        if (m_160 != 0) {
            m_c->m_1c->Free(m_160);
            m_160 = 0;
        }
        if (m_164 != 0) {
            m_c->m_1c->Free(m_164);
            m_164 = 0;
        }
        if (m_14 != 0) {
            m_c->m_1c->Free(m_14);
            m_14 = 0;
        }
        if (m_18 != 0) {
            m_c->m_1c->Free(m_18);
            m_18 = 0;
        }
    }
    m_3c = 0;
}

// ===========================================================================
// 0x104c80 - release the +0x34 blit through the +0x24 owner's +0x1c allocator
// (0x142160) and clear it. __thiscall.
// ===========================================================================
struct CMid104 {
    char pad0[0x1c];
    CFreer142160* m_1c; // +0x1c
};
struct C104c80 {
    char pad0[0x24];
    CMid104* m_24; // +0x24
    char pad28[0x34 - 0x28];
    void* m_34; // +0x34
    void Free();
};
RVA(0x00104c80, 0x1f)
void C104c80::Free() {
    if (m_34 != 0) {
        m_24->m_1c->Free(m_34);
        m_34 = 0;
    }
}

// ===========================================================================
// 0x104dd0 - lazy-create the StatusBarSprite: clamp +0x24/+0x28 to the manager's
// screen bounds, then build it via the +0x0c factory (0x1597b0). __thiscall.
// ===========================================================================
struct CMgr104 {
    char pad0[0x8c];
    i32 m_8c; // +0x8c
    i32 m_90; // +0x90
};
struct CFactory104 {
    void* Create(i32 a, i32 b, i32 c, i32 d, const char* name, i32 f); // 0x1597b0
};
struct CHolder104 {
    char pad0[8];
    CFactory104* m_8; // +0x08
};
struct C104dd0 {
    char pad0[8];
    void* m_8;       // +0x08
    CHolder104* m_c; // +0x0c
    char pad10[0x24 - 0x10];
    i32 m_24; // +0x24
    i32 m_28; // +0x28
    i32 Create();
};
// @early-stop
// scheduling wall: retail computes m_8c-0x22 via lea eax,[ecx-0x22] and loads m_24
// late; cl uses sub + an earlier m_24 load. Clamp logic, the factory call and the
// StatusBarSprite literal are byte-faithful.
RVA(0x00104dd0, 0x6b)
i32 C104dd0::Create() {
    if (m_8 != 0) {
        return 0;
    }
    CMgr104* mg = (CMgr104*)g_mgrSettings;
    i32 a = mg->m_8c - 0x22;
    i32 d = mg->m_90;
    if (m_24 > a) {
        m_24 = a;
    }
    if (m_28 > d - 9) {
        m_28 = d - 0x22;
    }
    m_8 = m_c->m_8->Create(0, m_24, m_28, 0xf4240, "StatusBarSprite", 1);
    return m_8 != 0;
}

// ===========================================================================
// 0x10bbe0 - getter: return +0x4cc when +0x528 is clear; else the active cell
// (+0x534[+0x52c]) when the +0x538 count exceeds the index, else 0. __thiscall.
// ===========================================================================
struct C10bbe0 {
    char pad0[0x4cc];
    i32 m_4cc; // +0x4cc
    char pad4d0[0x528 - 0x4d0];
    i32 m_528; // +0x528
    i32 m_52c; // +0x52c
    char pad530[0x534 - 0x530];
    i32** m_534; // +0x534
    i32 m_538;   // +0x538
    i32 M();
};
RVA(0x0010bbe0, 0x34)
i32 C10bbe0::M() {
    if (m_528 == 0) {
        return m_4cc;
    }
    if (m_538 > 0 && m_538 > m_52c) {
        return *m_534[m_52c];
    }
    return 0;
}

// ===========================================================================
// 0x112bf0 - decrement the active grid cell (manager-owned plane) and re-publish
// it through the manager's +0x70 notifier (0x33f0). __thiscall, returns 1.
// ===========================================================================
struct CGridData {
    char pad0[0x20];
    i32* cells; // +0x20
    i32* rows;  // +0x24
};
struct CGridHolder {
    char pad0[0x5c];
    CGridData* m_5c; // +0x5c
};
struct CGridOuter {
    char pad0[0x24];
    CGridHolder* m_24; // +0x24
};
struct CHandler112 {
    void Notify(i32 a, i32 b, i32 c); // 0x33f0
};
struct CMgr112 {
    char pad0[0x30];
    CGridOuter* m_30; // +0x30
    char pad34[0x70 - 0x34];
    CHandler112* m_70; // +0x70
};
struct C112bf0 {
    char pad0[8];
    i32 m_8; // +0x08
    i32 m_c; // +0x0c
    char pad10[0x14 - 0x10];
    i32 m_14; // +0x14
    i32 M();
};
// @early-stop
// strength-reduction wall: cl materializes m_c<<2 (shl ecx,2) and reuses scale-1
// addressing; retail keeps m_c in ecx and uses *4 scaled addressing in both cell
// stores. Logic/offsets byte-faithful; the shift vs scaled-index pick is not steerable.
RVA(0x00112bf0, 0x5e)
i32 C112bf0::M() {
    CMgr112* mg = (CMgr112*)g_mgrSettings;
    CGridData* g = mg->m_30->m_24->m_5c;
    i32 v = g->cells[g->rows[m_c] + m_8] - 1;
    CGridData* g2 = mg->m_30->m_24->m_5c;
    g2->cells[g2->rows[m_c] + m_8] = v;
    mg->m_70->Notify(m_8, m_c, v);
    m_14 = 0;
    return 1;
}

// ===========================================================================
// 0x113860 - mode gate: validate `obj` against mode (4 -> 0x4499, 7 -> 0x1893),
// passing through otherwise. __stdcall(obj, mode, a3, a4) ret 0x10.
// ===========================================================================
extern i32 __stdcall Func1893(void* p); // 0x1893
extern i32 __stdcall Func4499(void* p); // 0x4499
// @early-stop
// regalloc wall: retail keeps obj in eax (so the obj==0 return 0 is free); cl pins
// it in ecx and adds xor eax. switch(mode) recovers the case layout (93%); the eax
// vs ecx pick is not steerable.
RVA(0x00113860, 0x3b)
i32 __stdcall Gate113860(void* obj, i32 mode, i32 a3, i32 a4) {
    if (obj == 0) {
        return 0;
    }
    switch (mode) {
    case 4:
        if (!Func4499(obj)) {
            return 0;
        }
        break;
    case 7:
        if (!Func1893(obj)) {
            return 0;
        }
        break;
    }
    return 1;
}

// ===========================================================================
// 0x113e70 - Serialize: when the manager is live, transfer +0xc0/+0xc4 and the
// nine +0x9c.. dwords (3x3) through arg1's vtbl +0x2c. __thiscall(ar) ret 4.
// ===========================================================================
struct CArchive113 {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void Xfer(void* p, i32 n); // slot +0x2c
};
struct CMgr113 {
    char pad0[0x30];
    void* m_30; // +0x30
};
struct C113e70 {
    char pad0[0x9c];
    i32 m_9c[9]; // +0x9c .. +0xbc
    i32 m_c0;    // +0xc0
    i32 m_c4;    // +0xc4
    i32 Serialize(CArchive113* ar);
};
// @early-stop
// esi/edi regalloc wall: cl assigns ar->esi, this->edi; retail swaps (this->esi,
// ar->edi). The two header transfers + the 3x3 nested-loop transfer are byte-faithful.
RVA(0x00113e70, 0x7b)
i32 C113e70::Serialize(CArchive113* ar) {
    if (ar == 0) {
        return 0;
    }
    if (((CMgr113*)g_mgrSettings)->m_30 == 0) {
        return 0;
    }
    ar->Xfer(&m_c0, 4);
    ar->Xfer(&m_c4, 4);
    i32* p = m_9c;
    i32 i = 3;
    do {
        i32 j = 3;
        do {
            ar->Xfer(p, 4);
            p++;
        } while (--j);
    } while (--i);
    return 1;
}

// ===========================================================================
// 0x114ec0 - straight 6-arg forwarder to 0x21c1. __cdecl.
// ===========================================================================
extern "C" void Func21c1(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6); // 0x21c1
RVA(0x00114ec0, 0x27)
void Fwd114ec0(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    Func21c1(a1, a2, a3, a4, a5, a6);
}

// ===========================================================================
// 0x114f00 - guarded forwarder: resolve a2->m_30->m_4->m_10->m_2c and, when live,
// forward it plus the six args to 0x267b. __cdecl(6 args).
// ===========================================================================
struct CObj114f {
    char pad0[0x2c];
    void* m_2c; // +0x2c
};
struct CMid114f {
    char pad0[0x10];
    CObj114f* m_10; // +0x10
};
struct CHolder114f {
    char pad0[4];
    CMid114f* m_4; // +0x04
};
struct CArg114f {
    char pad0[0x30];
    CHolder114f* m_30; // +0x30
};
extern "C" void Func267b(void* v, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6); // 0x267b
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns): cl shares one pop;ret
// tail across the two null guards; retail emits the inline ret at each site. Deref
// chain + 6-arg re-push forward are byte-faithful.
RVA(0x00114f00, 0x3e)
void Fwd114f00(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    CObj114f* obj = ((CArg114f*)a2)->m_30->m_4->m_10;
    if (obj == 0) {
        return;
    }
    if (obj->m_2c == 0) {
        return;
    }
    Func267b(obj->m_2c, a1, a2, a3, a4, a5, a6);
}

// ===========================================================================
// 0x1181d0 - bounds-grow: reject when the new (+0x04,+0x08) pair does not exceed
// the +0xb8 box; else store it, notify (0x3661) and stash +0xd4. __thiscall(3).
// ===========================================================================
struct CBox118 {
    void* m_0;
    u32 m_4;
    u32 m_8;
};
extern "C" void Func3661(CBox118* p); // 0x3661
struct C1181d0 {
    char pad0[0xb8];
    CBox118 m_b8; // +0xb8
    char padd4[0xd4 - 0xb8 - 0xc];
    i32 m_d4; // +0xd4
    i32 Update(i32 a1, i32 a2, i32 a3);
};
RVA(0x001181d0, 0x70)
i32 C1181d0::Update(i32 a1, i32 a2, i32 a3) {
    if (a1 == 0) {
        return 0;
    }
    if (a2 == 0) {
        return 0;
    }
    CBox118* b = &m_b8;
    if (b == 0) {
        return 0;
    }
    if (b->m_4 > a1) {
        return 0;
    }
    if (b->m_4 == a1 && b->m_8 < a2) {
        return 0;
    }
    b->m_4 = a1;
    b->m_8 = a2;
    Func3661(b);
    m_d4 = a3;
    return 1;
}

// ===========================================================================
// 0x118260 - copy-if-grow: reject when the source box does not exceed +0xb8;
// else copy the 7-dword box in and stash +0xd4. __thiscall(src, arg2) ret 8.
// ===========================================================================
struct CRect118 {
    void* m_0;
    u32 m_4;
    u32 m_8;
    char pad[0x1c - 0xc]; // 7 dwords total
};
struct C118260 {
    char pad0[0xb8];
    CRect118 m_b8; // +0xb8 (7 dwords, ends at 0xd4)
    i32 m_d4;      // +0xd4
    i32 Update(CRect118* src, i32 arg2);
};
RVA(0x00118260, 0x63)
i32 C118260::Update(CRect118* src, i32 arg2) {
    if (src == 0) {
        return 0;
    }
    CRect118* dst = &m_b8;
    if (dst == 0) {
        return 0;
    }
    if (dst->m_4 > src->m_4) {
        return 0;
    }
    if (dst->m_4 == src->m_4 && dst->m_8 < src->m_8) {
        return 0;
    }
    *dst = *src;
    m_d4 = arg2;
    return 1;
}
