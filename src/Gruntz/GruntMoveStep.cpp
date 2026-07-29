#include <Mfc.h>
#include <Gruntz/Grunt.h>
#include <rva.h>

#include <Ints.h>
#include <stdlib.h> // rand (CRT PRNG, reloc-masked) for TileSwitch

RVA(0x00029af0, 0x3b)
// The two flag args each BURN one CRT rand() draw and discard it (a PRNG-sequence
// alignment device), which is all the retail bytes claim - hence the literal names.
void __stdcall TileSwitch(CGrunt* g, i32 col, i32 row, i32 burnRandA, i32 burnRandB, i32 unused) {
    if (burnRandA) {
        rand();
    }
    if (burnRandB) {
        rand();
    }
    g->TileSwitch(col, row, 0, 0x9c7, 0, 0);
}
