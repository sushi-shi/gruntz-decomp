#include <rva.h>

#include <Gruntz/SBI_ImageSetAni.h>
#include <Ints.h>

RVA_COMPGEN(0x001048c0, 0x1e, ??_GCSBI_StatzTabArrow@@UAEPAXI@Z)
RVA(0x001048f0, 0xa9)
CSBI_StatzTabArrow::~CSBI_StatzTabArrow() {
    Reset();
}
