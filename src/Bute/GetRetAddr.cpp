#include <rva.h>

RVA(0x0016d990, 0x3)
__declspec(naked) void* GetRetAddr() {
    __asm {
        pop  eax
        push eax
        ret
    }
}
