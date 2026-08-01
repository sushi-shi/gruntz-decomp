#define SBI_DTOR_CHAIN
#define SBI_OWN_RECTONLY_DTOR
#include <rva.h>
#include <Ints.h>
#include <Gruntz/SBI_Image.h>

DATA_SYMBOL(0x00001bd1, 0x0, ?Reset@CSBI_RectOnly@@UAEXXZ)
RVA(0x00100700, 0x55)
CSBI_RectOnly::~CSBI_RectOnly() {
    Reset();
}

RVA_COMPGEN(0x00100780, 0xb, ??1CStatusBarItem@@UAE@XZ)
