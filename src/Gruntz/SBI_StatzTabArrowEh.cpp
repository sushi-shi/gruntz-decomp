#include <rva.h>

#include <Gruntz/SBI_ImageSetAni.h>
#include <Ints.h>

RVA(0x001048f0, 0xa9)
CSBI_StatzTabArrow::~CSBI_StatzTabArrow() {
    Reset();
}
