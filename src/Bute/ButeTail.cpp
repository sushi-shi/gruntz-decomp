#include <StdAfx.h>

#include <Ints.h>

#include <Crypto/Blowfish.h>
#include <Crypto/CryptMgr.h>

CCryptMgr::CCryptMgr() {}

CCryptMgr::CCryptMgr(char* key) {
    InitializeBlowfish(key, sizeof(key));
}

CCryptMgr::~CCryptMgr() {}
