// TypeColl2.h - the insert facet of the shared type-name registry (@0x6bf650) the
// projectile/action registrars reach: Insert (0x16d850, __thiscall ret 0xc) rebuilds
// a collection slot. Redeclared identically in the ProjActRegistry / ActReg4
// registrars; folded here. NO-body -> the call reloc-masks. Kept as its own header
// (not merged into CTypeColl.h) so it is visible only to the two registrar TUs that
// use it - adding it to the widely-included CTypeColl.h perturbs unrelated large
// functions via MSVC's per-TU type-table (codegen leaks across functions).
#ifndef GRUNTZ_GRUNTZ_CTYPECOLL2_H
#define GRUNTZ_GRUNTZ_CTYPECOLL2_H

#include <Ints.h>

struct CTypeColl2 {};

#endif // GRUNTZ_GRUNTZ_CTYPECOLL2_H
