#include <rva.h>

#include <Crypto/Blowfish.h>

#include <Crypto/BlowfishPi.h>
#include <Crypto/CryptMgr.h>
#include <Ints.h>

#include <iostream.h>
#include <memory.h>
#include <string.h>

DATA(0x0021aeb0)
u32 g_bfP[18] = BF_PI_P_INIT;
DATA(0x0021aef8)
u32 g_bfS[4][256] = BF_PI_S_INIT;
DATA(0x0021bef8)
u32 g_bfInitP[18] = BF_PI_P_INIT;
DATA(0x0021bf40)
u32 g_bfInitS[4][256] = BF_PI_S_INIT;

#define BF_ENC(LL, R, P)                                                                           \
    (LL ^= (P),                                                                                    \
     LL ^=                                                                                         \
     (((g_bfS[0][(R) >> 24] + g_bfS[1][((R) >> 16) & 0xff]) ^ g_bfS[2][((R) >> 8) & 0xff])         \
      + g_bfS[3][(R) & 0xff]))

RVA(0x0016f6c0, 0x12)
void CCryptMgr::SetKey(const char* key) {
    InitializeBlowfish(key, sizeof(key));
}

RVA(0x0016f6e0, 0x76)
void CCryptMgr::Encrypt(istream& src, ostream& dst) {
    int last = 0;
    char buf[8];

    while (!src.eof()) {
        memset(buf, 0, 8);
        src.read(buf, 8);
        last = src.gcount();
        void* left = buf;
        void* right = &buf[4];
        Blowfish_encipher(static_cast<u32*>(left), static_cast<u32*>(right));
        dst.write(buf, 8);
    }
    dst.put(static_cast<char>(last));
}

RVA(0x0016f760, 0x82)
void CCryptMgr::Decrypt(istream& in, ostream& out) {
    int count = 0;
    char buf[8];
    char previous[8];
    bool first = true;

    while (!in.eof()) {
        in.read(buf, 8);
        count = in.gcount();
        if (count == 1) {
            count = static_cast<int>(buf[0]);
        }
        if (!first) {
            out.write(previous, count);
        } else {
            first = false;
        }
        void* left = buf;
        void* right = &buf[4];
        Blowfish_decipher(static_cast<u32*>(left), static_cast<u32*>(right));
        memcpy(previous, buf, 8);
    }
}

RVA(0x0016f7f0, 0x47b)
void Blowfish_encipher(u32* xl, u32* xr) {
    u32 l = *xl;
    u32 r = *xr;

    l ^= g_bfP[0];
    BF_ENC(r, l, g_bfP[1]);
    BF_ENC(l, r, g_bfP[2]);
    BF_ENC(r, l, g_bfP[3]);
    BF_ENC(l, r, g_bfP[4]);
    BF_ENC(r, l, g_bfP[5]);
    BF_ENC(l, r, g_bfP[6]);
    BF_ENC(r, l, g_bfP[7]);
    BF_ENC(l, r, g_bfP[8]);
    BF_ENC(r, l, g_bfP[9]);
    BF_ENC(l, r, g_bfP[10]);
    BF_ENC(r, l, g_bfP[11]);
    BF_ENC(l, r, g_bfP[12]);
    BF_ENC(r, l, g_bfP[13]);
    BF_ENC(l, r, g_bfP[14]);
    BF_ENC(r, l, g_bfP[15]);
    BF_ENC(l, r, g_bfP[16]);
    r ^= g_bfP[17];

    *xr = l;
    *xl = r;
}

RVA(0x0016fc70, 0x48e)
void Blowfish_decipher(u32* xl, u32* xr) {
    u32 l = *xl;
    u32 r = *xr;

    l ^= g_bfP[17];
    BF_ENC(r, l, g_bfP[16]);
    BF_ENC(l, r, g_bfP[15]);
    BF_ENC(r, l, g_bfP[14]);
    BF_ENC(l, r, g_bfP[13]);
    BF_ENC(r, l, g_bfP[12]);
    BF_ENC(l, r, g_bfP[11]);
    BF_ENC(r, l, g_bfP[10]);
    BF_ENC(l, r, g_bfP[9]);
    BF_ENC(r, l, g_bfP[8]);
    BF_ENC(l, r, g_bfP[7]);
    BF_ENC(r, l, g_bfP[6]);
    BF_ENC(l, r, g_bfP[5]);
    BF_ENC(r, l, g_bfP[4]);
    BF_ENC(l, r, g_bfP[3]);
    BF_ENC(r, l, g_bfP[2]);
    BF_ENC(l, r, g_bfP[1]);
    r ^= g_bfP[0];

    *xl = r;
    *xr = l;
}

RVA(0x00170100, 0x104)
i16 InitializeBlowfish(const char* key, i16 keybytes) {
    i16 i, j;
    u32 data, datal, datar;

    for (i = 0; i < 18; i++) {
        g_bfP[i] = g_bfInitP[i];
    }
    memcpy(g_bfS, g_bfInitS, sizeof(g_bfS));

    j = 0;
    for (i = 0; i < 18; i++) {

        data = static_cast<u32>(static_cast<u8>(key[j])) << 24;
        data |= static_cast<u32>(static_cast<u8>(key[(j + 1) % keybytes])) << 16;
        data |= static_cast<u32>(static_cast<u8>(key[(j + 2) % keybytes])) << 8;
        data |= static_cast<u32>(static_cast<u8>(key[(j + 3) % keybytes]));
        g_bfP[i] ^= data;
        j = (j + 4) % keybytes;
    }

    datal = 0;
    datar = 0;
    for (i = 0; i < 18; i += 2) {
        Blowfish_encipher(&datal, &datar);
        g_bfP[i] = datal;
        g_bfP[i + 1] = datar;
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 256; j += 2) {
            Blowfish_encipher(&datal, &datar);
            g_bfS[i][j] = datal;
            g_bfS[i][j + 1] = datar;
        }
    }
    return 0;
}
