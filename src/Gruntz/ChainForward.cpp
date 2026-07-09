// ChainForward.cpp - 0x114fa0: chase a +0x30->+0x4->+0x18->+0x2c pointer chain
// off arg2 and, if intact, forward (that leaf + all six args) to 0x114ff0.
#include <Ints.h>
#include <rva.h>

struct Chain2c {
    char m_pad0[0x2c];
    void* m_2c; // +0x2c
};
struct Chain18 {
    char m_pad0[0x14];
    Chain2c* m_14; // +0x14  (the 0x114f50 sibling chases this field)
    Chain2c* m_18; // +0x18
};
struct Chain4 {
    char m_pad0[4];
    Chain18* m_4; // +0x04
};
struct ChainRoot {
    char m_pad0[0x30];
    Chain4* m_30; // +0x30
};

extern "C" void Chain114ff0(void* leaf, i32 p1, ChainRoot* p2, i32 p3, i32 p4, i32 p5, i32 p6);

// ChainForward14 (0x114f50) - the +0x14 sibling of ChainForward below: chase the
// SAME p2->m_30->m_4 chain but off the +0x14 field, and forward the intact leaf +
// all six args to SaveScreenshot (0x114ff0).
// @early-stop
// same return-merge codegen-selection wall (~88%) as ChainForward below: the two
// null-guard `jne; ret` exits are byte-identical but cl /O2 merges them into one
// shared `je ret`. Logic 100% correct; deferred (same non-steerable heuristic).
RVA(0x00114f50, 0x3e)
void ChainForward14(i32 p1, ChainRoot* p2, i32 p3, i32 p4, i32 p5, i32 p6) {
    Chain2c* c = p2->m_30->m_4->m_14;
    if (c) {
        void* leaf = c->m_2c;
        if (leaf) {
            Chain114ff0(leaf, p1, p2, p3, p4, p5, p6);
        }
    }
}

// @early-stop
// return-merge codegen-selection wall (88%): the pointer-chain chase, the two null
// guards, and the 7-arg forward to 0x114ff0 are byte-identical; the only residual
// is that retail emits each null-guard exit as an inline `jne next; ret` (3 separate
// rets) while cl /O2 merges both guards into one `je shared_ret`. A cl tail-
// duplication heuristic (not flag- or source-steerable: nested-if and /Oy- both
// tested, no change). Logic 100% correct; deferred.
RVA(0x00114fa0, 0x3e)
void ChainForward(i32 p1, ChainRoot* p2, i32 p3, i32 p4, i32 p5, i32 p6) {
    Chain2c* c = p2->m_30->m_4->m_18;
    if (c) {
        void* leaf = c->m_2c;
        if (leaf) {
            Chain114ff0(leaf, p1, p2, p3, p4, p5, p6);
        }
    }
}

SIZE_UNKNOWN(Chain18);
SIZE_UNKNOWN(Chain2c);
SIZE_UNKNOWN(Chain4);
SIZE_UNKNOWN(ChainRoot);
