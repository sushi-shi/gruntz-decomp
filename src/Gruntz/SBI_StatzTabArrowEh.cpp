#define SBI_DTOR_CHAIN // enable the inline base-dtor bodies down the chain
#include <rva.h>
#include <Ints.h>
#include <Gruntz/SBI_ImageSetAni.h> // canonical chain up to CSBI_ImageSetAni + CSBI_StatzTabArrow

RVA(0x001048f0, 0xa9)
CSBI_StatzTabArrow::~CSBI_StatzTabArrow() {
    ResetCounters();
}
