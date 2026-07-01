// ClassUnknown43.cpp - 0x0f7d90 is a per-tick Update on the ClassUnknown_43
// manager: it mirrors a coord pair
// (m_17c/m_180 -> m_300/m_304), early-outs when m_198 is clear, otherwise pulls a
// peer object from m_260 and either re-syncs its position via g_mgrSettings->m_68
// or (after a >1000 throttle) re-snaps + screen-bounds-checks before firing
// g_mgrSettings->m_60. Field names are placeholders (offsets are load-bearing);
// engine callees + g_mgrSettings are external (reloc-masked).
#include <rva.h>
#include <Mfc.h>

class ClassUnknown_43;

struct Box5c {
    char pad[0x5c];
    int m_5c; // 0x5c
    int m_60; // 0x60
};

struct Obj260 {
    ClassUnknown_43* Get253b(ClassUnknown_43* self);
};

struct Rect4 {
    int left;   // 0x0
    int top;    // 0x4
    int right;  // 0x8
    int bottom; // 0xc
};
struct Reg5c {
    char pad[0x40];
    Rect4 rect; // 0x40
};
struct Reg24 {
    char pad[0x5c];
    Reg5c* m_5c; // 0x5c
};
struct Reg30 {
    char pad[0x24];
    Reg24* m_24; // 0x24
};
struct MgrObj {
    void Func3030(int a, int b, int c, int d);
    void Func39f4(void* a, int b, int c, int d, int e, int f);
};
struct MgrReg {
    char pad00[0x30];
    Reg30* m_30; // 0x30
    char pad34[0x60 - 0x34];
    MgrObj* m_60; // 0x60
    char pad64[0x68 - 0x64];
    MgrObj* m_68; // 0x68
};

DATA(0x0024556c)
extern "C" MgrReg* g_mgrSettings; // 0x64556c

class ClassUnknown_43 {
public:
    char pad00[0x10];
    Box5c* m_10; // 0x10
    char pad14[0x17c - 0x14];
    int m_17c; // 0x17c
    int m_180; // 0x180
    char pad184[0x198 - 0x184];
    void* m_198; // 0x198
    char pad19c[0x1ec - 0x19c];
    int m_1ec; // 0x1ec
    int m_1f0; // 0x1f0
    char pad1f4[0x1fc - 0x1f4];
    int m_1fc; // 0x1fc
    char pad200[0x248 - 0x200];
    int m_248; // 0x248
    char pad24c[0x260 - 0x24c];
    Obj260* m_260; // 0x260
    char pad264[0x2d0 - 0x264];
    int m_2d0; // 0x2d0
    int m_2d4; // 0x2d4
    char pad2d8[0x2ec - 0x2d8];
    unsigned m_2ec; // 0x2ec
    char pad2f0[0x300 - 0x2f0];
    int m_300; // 0x300
    int m_304; // 0x304
    char pad308[0x390 - 0x308];
    int m_390; // 0x390

    int Func1e97(int a, int b);
    int Func1014(int a, int b);
    void Func1640(int a, int b, int c, int d, int e, int f);
    int Update_0f7d90();
};

// @early-stop
// ~98%: logic byte-exact except the Func1640 (0x1640) arg-setup, where MSVC5
// pins m_248 in ecx + reuses edi for b->m_60 while the recompile picks
// eax/ecx/edx; pure regalloc selection in the push sequence, not steerable.
RVA(0x000f7d90, 0x171)
int ClassUnknown_43::Update_0f7d90() {
    m_300 = m_17c;
    m_304 = m_180;
    if (m_198 == 0) {
        m_2d0 = 5;
        m_2d4 = 0;
        m_2ec = 0;
        return 1;
    }
    ClassUnknown_43* p = m_260->Get253b(this);
    if (p == 0) {
        return 1;
    }
    if (p->m_1fc == 0) {
        return 1;
    }
    Box5c* a = p->m_10;
    if (a->m_5c == p->m_17c && a->m_60 == p->m_180 && Func1e97(a->m_5c, a->m_60)) {
        Box5c* b = p->m_10;
        g_mgrSettings->m_68->Func3030(m_1ec, m_1f0, b->m_5c, b->m_60);
        return 1;
    }
    if (m_2ec <= 0x3e8) {
        return 1;
    }
    if (Func1014(p->m_1ec, p->m_1f0)) {
        Box5c* b = p->m_10;
        Func1640(b->m_5c >> 5, b->m_60 >> 5, 0, m_248, 1, 0);
        m_2ec = 0;
        if (m_390 == 0) {
            return 1;
        }
        Box5c* c = m_10;
        MgrReg* g = g_mgrSettings;
        int y = c->m_60;
        int x = c->m_5c;
        Rect4* r = &g->m_30->m_24->m_5c->rect;
        if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) {
            g->m_60->Func39f4(this, 0x366, -1, 0, -1, -1);
        }
    }
    m_390 = 0;
    return 1;
}

SIZE_UNKNOWN(Box5c);
SIZE_UNKNOWN(MgrObj);
SIZE_UNKNOWN(MgrReg);
SIZE_UNKNOWN(Obj260);
SIZE_UNKNOWN(Rect4);
SIZE_UNKNOWN(Reg24);
SIZE_UNKNOWN(Reg30);
SIZE_UNKNOWN(Reg5c);
