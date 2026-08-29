#include <rva.h>

#include <Crypto/Blowfish.h>

#include <Crypto/BlowfishPi.h>
#include <Crypto/CryptMgr.h>
#include <Ints.h>

#include <iostream.h>
#include <memory.h>

#define bf_N 16

DATA(0x0021aeb0)
static u32 bf_P[bf_N + 2] = BF_PI_P_INIT;
DATA(0x0021aef8)
static u32 bf_S[4][256] = BF_PI_S_INIT;
DATA(0x0021bef8)
static u32 bf_P_Orig[bf_N + 2] = BF_PI_P_INIT;
DATA(0x0021bf40)
static u32 bf_S_Orig[4][256] = BF_PI_S_INIT;

union aword {
    u32 word;
    u8 byte[4];
    struct {
        unsigned int byte3 : 8;
        unsigned int byte2 : 8;
        unsigned int byte1 : 8;
        unsigned int byte0 : 8;
    } w;
};

#define S(x, i) (bf_S[i][x.w.byte##i])
#define bf_F(x) (((S(x, 0) + S(x, 1)) ^ S(x, 2)) + S(x, 3))
#define ROUND(a, b, n) (a.word ^= bf_F(b) ^ bf_P[n])

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
    union aword Xl;
    union aword Xr;

    Xl.word = *xl;
    Xr.word = *xr;

    Xl.word ^= bf_P[0];
    ROUND(Xr, Xl, 1);
    ROUND(Xl, Xr, 2);
    ROUND(Xr, Xl, 3);
    ROUND(Xl, Xr, 4);
    ROUND(Xr, Xl, 5);
    ROUND(Xl, Xr, 6);
    ROUND(Xr, Xl, 7);
    ROUND(Xl, Xr, 8);
    ROUND(Xr, Xl, 9);
    ROUND(Xl, Xr, 10);
    ROUND(Xr, Xl, 11);
    ROUND(Xl, Xr, 12);
    ROUND(Xr, Xl, 13);
    ROUND(Xl, Xr, 14);
    ROUND(Xr, Xl, 15);
    ROUND(Xl, Xr, 16);
    Xr.word ^= bf_P[17];

    *xr = Xl.word;
    *xl = Xr.word;
}

// Retail's mirror functions use different cl 5.0 register schemes. The surviving
// source includes string.h before both bodies; the reconstructed TU needs the same
// declaration boundary here because its preceding class-method composition differs.
#include <string.h>

RVA(0x0016fc70, 0x48e)
void Blowfish_decipher(u32* xl, u32* xr) {
    union aword Xl;
    union aword Xr;

    Xl.word = *xl;
    Xr.word = *xr;

    Xl.word ^= bf_P[17];
    ROUND(Xr, Xl, 16);
    ROUND(Xl, Xr, 15);
    ROUND(Xr, Xl, 14);
    ROUND(Xl, Xr, 13);
    ROUND(Xr, Xl, 12);
    ROUND(Xl, Xr, 11);
    ROUND(Xr, Xl, 10);
    ROUND(Xl, Xr, 9);
    ROUND(Xr, Xl, 8);
    ROUND(Xl, Xr, 7);
    ROUND(Xr, Xl, 6);
    ROUND(Xl, Xr, 5);
    ROUND(Xr, Xl, 4);
    ROUND(Xl, Xr, 3);
    ROUND(Xr, Xl, 2);
    ROUND(Xl, Xr, 1);
    Xr.word ^= bf_P[0];

    *xl = Xr.word;
    *xr = Xl.word;
}

RVA(0x00170100, 0x104)
i16 InitializeBlowfish(const char* key, i16 keybytes) {
    i16 i;
    i16 j;
    u32 data;
    u32 datal;
    u32 datar;
    union aword temp;

    memcpy(bf_P, bf_P_Orig, (bf_N + 2) * sizeof(u32));
    memcpy(bf_S, bf_S_Orig, 4 * 256 * sizeof(u32));

    j = 0;
    for (i = 0; i < bf_N + 2; ++i) {
        temp.word = 0;
        temp.w.byte0 = static_cast<u8>(key[j]);
        temp.w.byte1 = static_cast<u8>(key[(j + 1) % keybytes]);
        temp.w.byte2 = static_cast<u8>(key[(j + 2) % keybytes]);
        temp.w.byte3 = static_cast<u8>(key[(j + 3) % keybytes]);
        data = temp.word;
        bf_P[i] = bf_P[i] ^ data;
        j = (j + 4) % keybytes;
    }

    datal = 0x00000000;
    datar = 0x00000000;
    for (i = 0; i < bf_N + 2; i += 2) {
        Blowfish_encipher(&datal, &datar);
        bf_P[i] = datal;
        bf_P[i + 1] = datar;
    }
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 256; j += 2) {
            Blowfish_encipher(&datal, &datar);
            bf_S[i][j] = datal;
            bf_S[i][j + 1] = datar;
        }
    }
    return 0;
}
