// m2_GameChecksum.cpp - the multiplayer game-state signature accumulator
// (C:\Proj\Gruntz). Walks the 60-entry object table (this->m_4->m_4->m_68,
// indexed 0x1c..0x108 step 4, in 4 batches of 15) and folds a large set of each
// object's fields plus the global sync salt (g_645588 @0x645588) and a per-object
// switch term into a running 32-bit signature. Deliberate raw-offset reads keep the
// codegen naming-independent over the opaque object layout; offsets are load-bearing.
#include <Ints.h>
#include <rva.h>

#define I32AT(p, off) (*(i32*)((char*)(p) + (off)))

extern "C" i32 g_645588;       // 0x645588 (the per-frame sync salt)
extern "C" i32 ChecksumTail(); // 0x11fee0 (the trailing signature component)

struct CGameSyncSig {
    i32 ComputeSignature();
};

// 0xc0590 - fold every team object's fields into a 32-bit game-state signature.
//
// @early-stop
// integer-sum reassociation/scheduling wall (topic:wall topic:regalloc, ~58%):
// every field offset, the nested 15x4 loop, the value->value+2 switch jump table,
// the g_645588 salt and the ChecksumTail() term are all byte-faithful, but retail
// spills m_17c/m_180 to locals (a 0x10-byte frame) which reschedules the long field
// sum (adjacent independent loads land in a different order, e.g. 0x3f0/0x3f4 and
// 0x60/0x74 swapped). MSVC freely reassociates the integer +, so the order is not
// source-steerable here. Complete + correct; deferred to the final sweep.
RVA(0x000c0590, 0x1c3)
i32 CGameSyncSig::ComputeSignature() {
    i32 sum = 0;
    i32 off = 0x1c;
    do {
        i32 cnt = 15;
        do {
            char* base = *(char**)(*(char**)(*(char**)((char*)this + 4) + 4) + 0x68);
            char* obj = *(char**)(base + off);
            if (obj != 0) {
                char* sub = *(char**)(obj + 0x10);
                sum += I32AT(obj, 0x444) + I32AT(obj, 0x3f0) + I32AT(obj, 0x3f4) + I32AT(obj, 0x3ec)
                       + I32AT(sub, 0x60) + I32AT(sub, 0x74) + I32AT(sub, 0x5c) + I32AT(obj, 0x17c)
                       + I32AT(obj, 0x180);
                i32 n = I32AT(obj, 0x170);
                i32 d = n;
                if (n > 0x16) {
                    d = I32AT(obj, 0x19c);
                }
                sum += I32AT(obj, 0x198) + I32AT(obj, 0x1fc) + I32AT(obj, 0x1e4) + I32AT(obj, 0x224)
                       + d;
                i32 v = I32AT(obj, 0x170) - 1;
                i32 r;
                if ((u32)v > 0x15) {
                    r = 0x17;
                } else {
                    switch (v) {
                        case 0:
                            r = 2;
                            break;
                        case 1:
                            r = 3;
                            break;
                        case 2:
                            r = 4;
                            break;
                        case 3:
                            r = 5;
                            break;
                        case 4:
                            r = 6;
                            break;
                        case 5:
                            r = 7;
                            break;
                        case 6:
                            r = 8;
                            break;
                        case 7:
                            r = 9;
                            break;
                        case 8:
                            r = 0xa;
                            break;
                        case 9:
                            r = 0xb;
                            break;
                        case 0xa:
                            r = 0xc;
                            break;
                        case 0xb:
                            r = 0xd;
                            break;
                        case 0xc:
                            r = 0xe;
                            break;
                        case 0xd:
                            r = 0xf;
                            break;
                        case 0xe:
                            r = 0x10;
                            break;
                        case 0xf:
                            r = 0x11;
                            break;
                        case 0x10:
                            r = 0x12;
                            break;
                        case 0x11:
                            r = 0x13;
                            break;
                        case 0x12:
                            r = 0x14;
                            break;
                        case 0x13:
                            r = 0x15;
                            break;
                        case 0x14:
                            r = 0x16;
                            break;
                        case 0x15:
                            r = 0x17;
                            break;
                        default:
                            r = 0x17;
                            break;
                    }
                }
                sum += I32AT(obj, 0x450) + I32AT(obj, 0x358) + I32AT(obj, 0x218) + I32AT(obj, 0x21c)
                       + I32AT(obj, 0x220) + g_645588 + r;
                sum += ChecksumTail();
            }
            off += 4;
        } while (--cnt);
    } while (off < 0x10c);
    return sum;
}
