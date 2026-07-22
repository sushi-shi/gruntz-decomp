#ifndef GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
#define GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H

#include <Ints.h>
#include <rva.h>

#include <Wap32/ZVec.h> // the canonical _zdvec allocating base (ex-CZArray2D)

// Its own 1-slot vtable. VTBL names the datum ??_7CTypeCollRuntime@@6B@ (a typed vtable,
// not the ex-DATA `void* const CTypeCollRuntime_vtbl` global). The real dtor below (the
// only member the devs wrote) makes this the vtable's key TU, so cl emits the ??_7 + the
// `??_G' scalar-deleting destructor (0x16ea20) here and auto-stamps the vptr.
VTBL(CTypeCollRuntime, 0x001f04e4);
class CTypeCollRuntime : public _zdvec {
public:
    virtual ~CTypeCollRuntime() OVERRIDE;
};
SIZE_UNKNOWN(); // _zdvec base (0x24) + no own fields; size not pinned

#endif // GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
