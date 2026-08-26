#include <rva.h>

#include <Mfc.h>

#include <stdarg.h>
#include <stdio.h>

RVA(0x0013e2c0, 0x21)
void ActiveWait(u32 milliseconds) {
    DWORD target = timeGetTime() + milliseconds;
    while (timeGetTime() < target)
        ;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013e2f0, 0x32)
void DebugTrace(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    OutputDebugStringA(buf);
}
