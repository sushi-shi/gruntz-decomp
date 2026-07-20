#include <rva.h>
#include <string.h>

RVA(0x0017f500, 0x23)
void __stdcall ZeroRecords(void* dst, int count) {
    memset(dst, 0, count * 0x28);
}
