#include <rva.h>

#include <Bute/ButeTail.h>

#include <Crypto/Blowfish.h>

RVA(0x0016f680, 0x3)
CButeTail::CButeTail() {}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0016f690, 0x18)
CButeTail::CButeTail(const char* key) {
    InitializeBlowfish(key, 4);
}

RVA(0x0016f6b0, 0x1)
CButeTail::~CButeTail() {}
