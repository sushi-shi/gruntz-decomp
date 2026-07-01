// FaderElemSetup.cpp - CFaderElem::Apply (0x17f5e0), a fader-element setup method
// in the CFaderMgr family (caller CFaderMgr::Add 0x17d9c0).  Copies a config
// source's fields into the element and allocates its per-frame work array.
#include <Ints.h>
#include <rva.h>

extern "C" void* RezAlloc(i32 n); // 0x1b9b46

struct FaderSrc {
    char pad00[0x18];
    i32 m_18; // +0x18 frame count
};

struct FaderArg {
    char pad00[4];
    i32 m_04;       // +0x04
    FaderSrc* m_08; // +0x08
    i32 m_0c;       // +0x0c
    i32 m_10;       // +0x10
    i32 m_14;       // +0x14
};

struct CFaderElem {
    char pad00[0x20];
    i32 m_20;       // +0x20
    i32 m_24;       // +0x24
    FaderSrc* m_28; // +0x28
    char pad2c[0x38 - 0x2c];
    i32 m_38;       // +0x38
    FaderSrc* m_3c; // +0x3c
    i32 m_40;       // +0x40
    i32 m_44;       // +0x44
    i32 m_48;       // +0x48
    i32* m_4c;      // +0x4c per-frame array

    i32 Apply(FaderArg* s); // 0x17f5e0
};

// @early-stop
// regalloc/scheduling tie (~90%): logic byte-exact; retail's ecx/edx assignment
// for the m_3c reload + s->m_10 store schedule differs from this cl's.
// 0x17f5e0
RVA(0x0017f5e0, 0x7d)
i32 CFaderElem::Apply(FaderArg* s) {
    i32 a = s->m_04;
    if (!a) {
        a = m_24;
    }
    m_38 = a;
    if (s->m_08) {
        m_3c = s->m_08;
    } else {
        m_3c = m_28;
    }
    m_40 = s->m_0c;
    m_44 = s->m_10;
    m_20 = 0;
    m_48 = s->m_14;
    m_4c = (i32*)RezAlloc(m_3c->m_18 << 2);
    for (i32 i = 0; i < m_3c->m_18; i++) {
        m_4c[i] = 0;
    }
    return 1;
}

// class metadata (SIZE sweep, D-G)
SIZE_UNKNOWN(FaderSrc);
SIZE_UNKNOWN(FaderArg);
SIZE_UNKNOWN(CFaderElem);

