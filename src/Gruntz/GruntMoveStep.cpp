// GruntMoveStep.cpp - the random-gated tile-switch dispatch TileSwitch29af0 (0x029af0),
// a COMDAT-pooled leaf called by CGruntMover::Step. Step itself (0x031610) was re-homed
// to src/Gruntz/BattlezMapConfig.cpp (waveP): its retail birth position is inside that
// TU's 0x29a30 interval. This unit retains only the 0x29af0 leaf (its own attribution is
// out of scope this batch).
#include <Mfc.h>
#include <Gruntz/Grunt.h>
#include <rva.h>

#include <Ints.h>
#include <stdlib.h> // rand (CRT PRNG, reloc-masked) for TileSwitch29af0

// 0x29af0 (re-homed from src/Stub/BoundaryMisc.cpp): a random-gated tile-switch
// dispatch called by CGruntMover::Step. Consumes up to two CRT rand() draws (gated by
// a4/a5) then dispatches CGrunt_TileSwitch(a2, a3, 0, 0x9c7, 0, 0); the a1 receiver is
// loaded into ecx but ignored by the __stdcall callee. __stdcall, 6 args (ret 0x18).
RVA(0x00029af0, 0x3b)
void __stdcall TileSwitch29af0(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    if (a4) {
        rand();
    }
    if (a5) {
        rand();
    }
    CGrunt_TileSwitch(a2, a3, 0, 0x9c7, 0, 0);
}
