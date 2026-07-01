// RingBuffer.cpp - a ring-buffer "consume" + a forwarding shim (anonymous owners).
//
//  0x186910 - copy min(src.count, dst.count) bytes from the source ring (+0x1c)
//             into the destination, advance both read pointers / counters, and
//             rewind the source read pointer to its base once it drains.
//  0x186180 - __cdecl forwarder inserting fixed parameters (8,0xf,8,0) into a
//             call to the 8-arg worker at 0x1861b0.
#include <rva.h>
#include <string.h>

struct RingSrc {
    char pad0[8];
    char* m_08;    // +0x08 base
    char* m_0c;    // +0x0c read ptr
    unsigned m_10; // +0x10 count
};
struct RingCtx {
    char pad0[0xc];
    char* m_0c;    // +0x0c write ptr
    unsigned m_10; // +0x10 free count
    unsigned m_14; // +0x14 used count
    char pad18[0x1c - 0x18];
    RingSrc* m_1c; // +0x1c source ring
};

// 0x186910
RVA(0x00186910, 0x72)
void Unmatched_186910(RingCtx* c) {
    unsigned n = c->m_1c->m_10;
    if (n > c->m_10) {
        n = c->m_10;
    }
    if (n) {
        memcpy(c->m_0c, c->m_1c->m_0c, n);
        c->m_0c += n;
        c->m_1c->m_0c += n;
        c->m_14 += n;
        c->m_10 -= n;
        c->m_1c->m_10 -= n;
        if (c->m_1c->m_10 == 0) {
            c->m_1c->m_0c = c->m_1c->m_08;
        }
    }
}

// 0x186180
extern "C" int FUN_1861b0(int, int, int, int, int, int, int, int); // 0x1861b0
RVA(0x00186180, 0x25)
int Unmatched_186180(int a0, int a1, int a2, int a3) {
    return FUN_1861b0(a0, a1, 8, 0xf, 8, 0, a2, a3);
}

// Class metadata (hosted at .cpp EOF).
SIZE_UNKNOWN(RingCtx); // ring-buffer context partial view
SIZE_UNKNOWN(RingSrc); // source-ring partial view
