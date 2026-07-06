// Cluster0c.cpp - three methods of an anonymous class living in the 0x0c00xx
// cluster: a field-init (0xc0c20) that zeroes a span of members then constructs
// two embedded sub-objects at +0x4c/+0x58, a two-step driver (0xc2a50), and a
// teardown (0xc5240) that releases+frees the +0x60 child then runs a final method.
#include <rva.h>

extern "C" void RezFree(void*); // 0x1b9b82

// The +0x60 owned child: dtor @0xc5280 (Release IS ~CNetThing); TU-local view of the
// real header-less CNetThing (netthingdtor unit).
struct CNetThing {
    ~CNetThing();
};

struct CCluster0c {
    char pad00[4];
    int m_04; // +0x04
    int m_08; // +0x08
    char pad0c[0x10 - 0x0c];
    int m_10; // +0x10
    int m_14; // +0x14
    int m_18; // +0x18
    char pad1c[0x3c - 0x1c];
    int m_3c;               // +0x3c
    int m_40;               // +0x40
    int m_44;               // +0x44
    int m_48;               // +0x48
    char m_4c[0x58 - 0x4c]; // +0x4c sub-object
    char m_58[0x60 - 0x58]; // +0x58 sub-object
    CNetThing* m_60;        // +0x60 owned child

    void Init12e0();        // 0xc12e0
    void Init10a0(void* p); // 0xc10a0
    void M_c2ab0(int n);    // 0xc2ab0
    void M_c40b0();         // 0xc40b0
    void Destroy_1bbb7c();  // 0x1bbb7c

    void Init();    // 0xc0c20
    void Run();     // 0xc2a50
    void Cleanup(); // 0xc5240
};

// 0xc0c20
// @early-stop
// regalloc tie-break wall (~71%): logic byte-identical, but retail rematerializes
// the zero constant in eax (a second `xor eax,eax` after the Init12e0 call) while
// this toolchain's cl hoists 0 into a callee-saved edi across the call (push/pop
// edi + every zero store's modrm reg field 46->7e).  Proven non-steerable: 3
// spellings (separate / chained / temp-var) x 5 opt flags (/O2,/O1,/O2 /Oy-,/Ox,
// /Og /Os) x 3 callee conventions all emit the edi hoist - a cl-build heuristic
// delta, not a source shape.
RVA(0x000c0c20, 0x3f)
void CCluster0c::Init() {
    m_04 = 0;
    m_08 = 0;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    Init12e0();
    m_3c = 0;
    m_40 = 0;
    m_44 = 0;
    m_48 = 0;
    Init10a0(&m_4c);
    Init10a0(&m_58);
}

// 0xc2a50
RVA(0x000c2a50, 0x13)
void CCluster0c::Run() {
    M_c2ab0(2);
    M_c40b0();
}

// 0xc5240
RVA(0x000c5240, 0x2c)
void CCluster0c::Cleanup() {
    CNetThing* p = m_60;
    if (p) {
        p->~CNetThing();
        RezFree(p);
        m_60 = 0;
    }
    Destroy_1bbb7c();
}

SIZE_UNKNOWN(CCluster0c);
SIZE_UNKNOWN(CNetThing);
