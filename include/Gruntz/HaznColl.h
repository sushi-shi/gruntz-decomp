// HaznColl.h - the single shared address-view of the engine coordinate/activation-
// registry collection (a _zvec-based registry). The SAME archetype is instantiated
// as the static-hazard registry (@0x64e3d0), the timebomb registry (@0x64c780), and
// the shared name registry (@0x6bf650), and range-registered from the boundary thunk
// pool. The slow coordinate lookup and range-register are the real _zvec / CZDArray
// methods, called through a cast: `(reinterpret_cast<_zvec*>(&g))->GrowTo` (0x16da80) and
// `((CZDArrayDerived*)&g)->Construct` (0x8710). This struct is now a pure
// address-holder for the DATA-pinned globals; offsets + code bytes are load-bearing.
//
// Previously duplicated as CTBombColl (TBombColl.h) and CHaznColl (HaznColl.h) - two
// byte-identical struct views of the one archetype, dual-binding g_nameReg@0x6bf650.
// Both folded into CCoordColl (the old names removed). The former placeholder Find /
// RegisterRange methods (comment-only "bindings" that never bound, leaving their
// calls UNBOUND) were dissolved onto the real _zvec::GrowTo / CZDArrayDerived cast.
#ifndef GRUNTZ_GRUNTZ_CHAZNCOLL_H
#define GRUNTZ_GRUNTZ_CHAZNCOLL_H

#include <rva.h>

struct CCoordColl {};

#endif // GRUNTZ_GRUNTZ_CHAZNCOLL_H
