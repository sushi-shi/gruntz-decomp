// GameChecksum.cpp - the multiplayer game-state signature accumulator
// (C:\Proj\Gruntz). Walks the 60-entry object table (this->m_4->m_4->m_68,
// indexed 0x1c..0x108 step 4, in 4 batches of 15) and folds a large set of each
// object's fields plus the global sync salt (g_frameTime @0x645588) and a per-object
// switch term into a running 32-bit signature. The folded object-table entry is the
// real CGrunt (its m_10 is the CGruntHud geometry source), typed via <Gruntz/Grunt.h>.
#include <Ints.h>
#include <rva.h>
#include <stdlib.h>       // rand (0x11fee0), the trailing signature component
#include <Gruntz/Grunt.h> // the real CGrunt object-table entry + its CGruntHud* m_10
// g_frameTime (the per-frame sync salt, 0x645588) comes from <Gruntz/MovingLogic.h>
// via Grunt.h as `extern "C" u32 g_frameTime` - re-declaring it here clashed (C2371).

struct CGameSyncSig {
    i32 ComputeSignature();
};

// fold every team object's fields into a 32-bit game-state signature.
//
// @early-stop
// integer-sum reassociation/scheduling wall (topic:wall topic:regalloc, ~58%):
// every field offset, the nested 15x4 loop, the value->value+2 switch jump table,
// the g_frameTime salt and the rand() term are all byte-faithful, but retail
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
            CGrunt* obj = *(CGrunt**)(base + off);
            if (obj != 0) {
                CGruntHud* sub = obj->m_10;
                sum += obj->m_entranceCell.reason + obj->m_stamina + obj->m_toyTime + obj->m_health
                       + sub->m_60 + sub->m_74 + sub->m_5c + obj->m_lastTilePxX
                       + obj->m_lastTilePxY;
                i32 n = obj->m_entranceReason;
                i32 d = n;
                if (n > 0x16) {
                    d = obj->m_19c;
                }
                sum +=
                    obj->m_198 + obj->m_entranceCommitted + obj->m_entranceActive + obj->m_224 + d;
                i32 v = obj->m_entranceReason - 1;
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
                sum += obj->m_arrivalPhase + obj->m_358 + obj->m_combatActive + obj->m_neighborValid
                       + obj->m_poweredUp + g_frameTime + r;
                sum += rand();
            }
            off += 4;
        } while (--cnt);
    } while (off < 0x10c);
    return sum;
}

SIZE_UNKNOWN(CGameSyncSig);
