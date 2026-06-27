// DiscoveredSmall.cpp - trace-discovered small leaf methods re-homed from
// src/Stub/Discovered.cpp (matcher-1). Each ClassUnknown_N is a placeholder
// class whose true name is unknown; only the OFFSETS + code bytes are
// load-bearing. These are the self-contained (no/one-reloc) leaves: simple
// ctors / inits / free-list ops / a key-compare helper.
#include <Ints.h>
#include <rva.h>

// The engine __cdecl deallocator (operator delete; reloc-masked rel32). 0x1b9b82.
extern "C" void RezFree(void* p);

// The severus-worker teardown grand-base vtable (0x5e8cb4); stamped by address.
extern void* g_severusWorkerDtorVtbl;

// ---------------------------------------------------------------------------
// ClassUnknown_28 @0x0311b0 - a typed free-list push: subtract the node's link
// offset (m_c) from the freed pointer, chain it onto the head (m_4). __thiscall,
// 1 stack arg.
// ---------------------------------------------------------------------------
class ClassUnknown_28 {
public:
    void Push(char* p);
    i32 m_0;
    void* m_4; // free-list head
    i32 m_8;
    i32 m_c; // node link offset
};
RVA(0x000311b0, 0x14)
void ClassUnknown_28::Push(char* p) {
    char* node = p - m_c;
    *(void**)node = m_4;
    m_4 = node;
}

// ---------------------------------------------------------------------------
// ClassUnknown_29 @0x029a30 - a list iterator advance: read the current node
// (*it), step the cursor to its link (*cur), return a pointer into the current
// node's payload (cur+8). __stdcall, 1 stack arg.
// ---------------------------------------------------------------------------
RVA(0x00029a30, 0x10)
void* __stdcall ClassUnknown_29_advance(void** it) {
    char* cur = (char*)*it;
    *it = *(void**)cur;
    return cur + 8;
}

// ---------------------------------------------------------------------------
// ClassUnknown_14 @0x029ac0 - 4-field initializer (ctor returning this).
// __thiscall, 4 stack args, ret 0x10.
// ---------------------------------------------------------------------------
class ClassUnknown_14 {
public:
    ClassUnknown_14(i32 a, i32 b, i32 c, i32 d);
    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 m_c;
};
RVA(0x00029ac0, 0x20)
ClassUnknown_14::ClassUnknown_14(i32 a, i32 b, i32 c, i32 d) {
    m_0 = a;
    m_4 = b;
    m_8 = c;
    m_c = d;
}

// ---------------------------------------------------------------------------
// ClassUnknown_21 @0x15b2b0 - zero three fields (ctor returning this).
// ---------------------------------------------------------------------------
class ClassUnknown_21 {
public:
    ClassUnknown_21();
    char m_pad0[0x8];
    i32 m_8;
    i32 m_c;
    char m_pad10[0x18 - 0x10];
    i32 m_18;
};
RVA(0x0015b2b0, 0xe)
ClassUnknown_21::ClassUnknown_21() {
    m_c = 0;
    m_8 = 0;
    m_18 = 0;
}

// ---------------------------------------------------------------------------
// ClassUnknown_22 @0x15b270 - seed two fields (ctor returning this).
// ---------------------------------------------------------------------------
class ClassUnknown_22 {
public:
    ClassUnknown_22();
    char m_pad0[0x8];
    i32 m_8;
    char m_pad0c[0x20 - 0xc];
    i32 m_20;
};
RVA(0x0015b270, 0x11)
ClassUnknown_22::ClassUnknown_22() {
    m_8 = (i32)0x80000000;
    m_20 = -1;
}

// ---------------------------------------------------------------------------
// ClassUnknown_39 @0x148d10 - free two owned heap blocks (m_c, m_20) if set.
// __thiscall, void.
// ---------------------------------------------------------------------------
class ClassUnknown_39 {
public:
    void FreeBuffers();
    char m_pad0[0xc];
    void* m_c;
    char m_pad10[0x20 - 0x10];
    void* m_20;
};
RVA(0x00148d10, 0x25)
void ClassUnknown_39::FreeBuffers() {
    if (m_c) {
        RezFree(m_c);
    }
    if (m_20) {
        RezFree(m_20);
    }
}

// ---------------------------------------------------------------------------
// ClassUnknown_46 @0x1591b0 - severus-worker base init: seed m_4=-1, zero
// m_8/m_c/m_10, stamp the grand-base dtor vptr. A void METHOD (keeps this in
// ecx, eax=0; no mov eax,ecx) - see vptr-stamp-void-init-not-ctor.
// ---------------------------------------------------------------------------
class ClassUnknown_46 {
public:
    void BaseInit();
    void* m_vptr;
    i32 m_4;
    i32 m_8;
    i32 m_c;
    i32 m_10;
};
RVA(0x001591b0, 0x19)
void ClassUnknown_46::BaseInit() {
    m_4 = -1;
    m_10 = 0;
    m_8 = 0;
    m_c = 0;
    m_vptr = &g_severusWorkerDtorVtbl;
}

// ---------------------------------------------------------------------------
// ClassUnknown_49 @0x1397a0 - teardown: free m_0; then free m_38 unless the
// m_10 target is live (m_10 && m_10->m_48 != 0); then clear nine fields.
// __thiscall, void. (The two `if (m_38) free` arms tail-merge to one call.)
// ---------------------------------------------------------------------------
struct Obj49Target {
    char m_pad[0x48];
    i32 m_48;
};
class ClassUnknown_49 {
public:
    void Teardown();
    void* m_0;
    i32 m_4;
    i32 m_8;
    i32 m_c;
    Obj49Target* m_10;
    i32 m_14;
    i32 m_18;
    char m_pad1c[0x30 - 0x1c];
    i32 m_30;
    char m_pad34[0x38 - 0x34];
    void* m_38;
};
RVA(0x001397a0, 0x57)
void ClassUnknown_49::Teardown() {
    if (m_0) {
        RezFree(m_0);
    }
    if (m_10 != 0) {
        if (m_10->m_48 == 0) {
            if (m_38) {
                RezFree(m_38);
            }
        }
    } else {
        if (m_38) {
            RezFree(m_38);
        }
    }
    m_0 = 0;
    m_4 = 0;
    m_8 = 0;
    m_c = 0;
    m_38 = 0;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    m_30 = 0;
}

// ---------------------------------------------------------------------------
// ClassUnknown_8 @0x16e480 - the bit-level common-prefix length of two byte
// keys: 8 per matching leading byte, plus the trailing-zero-bit count of the
// first differing pair's xor. __cdecl.
// ---------------------------------------------------------------------------
RVA(0x0016e480, 0x3e)
i32 KeyPrefixBits_16e480(const char* a, const char* b) {
    i32 n = 0;
    while (*a == *b) {
        n += 8;
        ++a;
        ++b;
    }
    i32 x = *a;
    x ^= *b;
    i32 c = 0;
    while (!(x & 1)) {
        x >>= 1;
        ++c;
    }
    return c + n;
}
