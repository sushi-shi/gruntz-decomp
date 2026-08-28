#include <rva.h>

#include <Crypto/Blowfish.h>
#include <Crypto/CryptMgr.h>

RVA(0x0016f680, 0x3)
CCryptMgr::CCryptMgr() {}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0016f690, 0x18)
CCryptMgr::CCryptMgr(char* key) {
    InitializeBlowfish(key, sizeof(key));
}

RVA(0x0016f6b0, 0x1)
CCryptMgr::~CCryptMgr() {}
