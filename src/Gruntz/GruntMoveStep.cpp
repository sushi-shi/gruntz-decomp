#include <Mfc.h>
#include <Gruntz/Grunt.h>
#include <rva.h>

#include <Ints.h>
#include <stdlib.h> // rand (CRT PRNG, reloc-masked) for TileSwitch29af0

RVA(0x00029af0, 0x3b)
void __stdcall TileSwitch29af0(CGrunt* g, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    if (a4) {
        rand();
    }
    if (a5) {
        rand();
    }
    g->TileSwitch(a2, a3, 0, 0x9c7, 0, 0);
}
