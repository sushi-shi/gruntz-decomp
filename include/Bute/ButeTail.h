#ifndef SRC_BUTE_BUTETAIL_H
#define SRC_BUTE_BUTETAIL_H

#include <rva.h>

struct CButeTail {
    CButeTail();

    ~CButeTail();

    // Both crypto entry points are __thiscall members of this tag struct: every
    // retail call site (Save 0x171640, ProcessCheatInput 0x205c0, Run 0x83450)
    // loads ecx with a CButeTail lvalue before the call.
    void Decode(class istream* in, class ostream* out);
    void Encode(class istream* src, class ostream* dst);
};
SIZE(0x1);

#endif // SRC_BUTE_BUTETAIL_H
