// NameVec.h - the scratch name-vec (zDArray<CString> @ 0x6bf650, ?g_buteNameVec):
// the bute registration path caches each interned name string here. An empty derived
// view over zDArray (only the base layout is load-bearing); folded from the
// per-TU redeclarations in the icon/text/animation/wormhole registration TUs.
// This is a DIFFERENT facet of the shared 0x6bf650 registry than the type-name
// lookup facet (CTypeColl/CTypeKeyColl) - a CString vector, not the id->node map -
// so it stays a distinct class (do not union with CTypeColl).
#ifndef GRUNTZ_GRUNTZ_NAMEVEC_H
#define GRUNTZ_GRUNTZ_NAMEVEC_H

#include <Wap32/ZVec.h> // zDArray

struct NameVec : public zDArray {};

#endif // GRUNTZ_GRUNTZ_NAMEVEC_H
