#ifndef SRC_BUTE_BUTETAIL_H
#define SRC_BUTE_BUTETAIL_H

#include <rva.h>

struct CButeTail {
    CButeTail();

    ~CButeTail();

    // All THREE crypto entry points are __thiscall members of this tag struct:
    // every retail call site (Save 0x171640, ProcessCheatInput 0x205c0, Run
    // 0x83450) loads ecx with a CButeTail lvalue before the call. For InitKey
    // that lvalue is `mov ecx,0x6454e7` in Run - g_buteMgr + 0x10f, i.e.
    // g_buteMgr.m_crypt - and `lea ecx,[esp+0x14b]` in ProcessCheatInput, the
    // same slot its inlined ~CButeMgr later hands to ??1CButeTail. The 18-byte
    // body is indistinguishable from a __stdcall free function because `this`
    // is unused; only the call sites say which it is.
    void InitKey(const char* key);
    void Decode(class istream* in, class ostream* out);
    void Encode(class istream* src, class ostream* dst);
};

#endif // SRC_BUTE_BUTETAIL_H
