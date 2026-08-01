#define SBI_DTOR_CHAIN
#include <rva.h>
#include <Ints.h>
#include <Gruntz/SBI_ImageSetAni.h>

RVA(0x001048f0, 0xa9)
CSBI_StatzTabArrow::~CSBI_StatzTabArrow() {
    Reset();
}
