#include <rva.h>

#include <Mfc.h>

#include <Gruntz/Grunt.h>
#include <Ints.h>

#include <stdlib.h>

RVA(0x00029af0, 0x3b)

void __stdcall TileSwitch(CGrunt* g, i32 col, i32 row, i32 burnRandA, i32 burnRandB, i32 unused) {
    if (burnRandA) {
        rand();
    }
    if (burnRandB) {
        rand();
    }
    g->TileSwitch(col, row, 0, 0x9c7, 0, 0);
}
