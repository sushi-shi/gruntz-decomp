// FaderShapeSetup.cpp - a CFaderMgr-family screen-fader subclass init (0x1817e0).
// __thiscall, called by CFaderMgr::Add (0x17d9c0). Resolves three equal-sized
// source surfaces (m_38/m_3c/m_40) from the config arg (defaulting to the fader's
// own m_24/m_28), validates their dimensions match, builds an acos warp table
// (m_478) and a sin highlight ramp (m_48c), resolves the shade table via the
// embedded CShadeTableCache (file/array/flash), then allocates per-column pitch
// tables (m_44/m_48/m_4c) and a scratch line (m_488). Field names are placeholders;
// offsets + code bytes are the load-bearing fact.
#include <Ints.h>
#include <math.h> // acos -> __CIacos, sin -> fsin (intrinsics at /O2 /Oi)
#include <rva.h>

extern "C" void* RezAlloc(i32 n);                   // 0x1b9b46
extern "C" int _access(const char* path, int mode); // 0x193900 CRT

// The embedded shade-table cache subobject (CFader base, at this+0x04).
struct ShadeCache {
    i32 Init();                                                // 0x14dec0
    void* AddFromArray(char* name);                            // 0x14f6c0
    void* FlashTable(void* pal, i32 nA, i32 nB, i32 s, i32 e); // 0x14df40
    char pad[0x18];
};

// A source surface: dims at +0x18/+0x1c, row pitch +0x20, format gate +0xa8,
// column stride +0xb0.
struct FShadeSurf {
    char p00[0x18];
    i32 m_18; // +0x18 width
    i32 m_1c; // +0x1c height
    i32 m_20; // +0x20 row pitch
    char p24[0xa8 - 0x24];
    i32 m_a8; // +0xa8 format
    char pac[0xb0 - 0xac];
    i32 m_b0; // +0xb0 column stride
};

// The +0x28 config arg's palette holder.
struct FInitPal {
    char p00[0xc];
    void* m_0c; // +0x0c palette base
};

// The config arg (CFaderMgr::Add's pInit).
struct FInit {
    char p00[0x4];
    FShadeSurf* m_04; // +0x04 surface A override
    FShadeSurf* m_08; // +0x08 surface B override
    void* m_0c;       // +0x0c surface C override
    i32 m_10;         // +0x10 ramp size
    i32 m_14;         // +0x14 mode (1..3)
    i32 m_18;         // +0x18
    i32 m_1c;         // +0x1c gate
    i32 m_20;         // +0x20 prebuilt table
    char* m_24;       // +0x24 table file/array name
    FInitPal* m_28;   // +0x28 flash palette source
};

struct CFaderShape {
    char p00[0x4];
    ShadeCache m_04;  // +0x04 cache subobject (0x18)
    void* m_1c;       // +0x1c resolved shade table
    i32 m_20;         // +0x20
    FShadeSurf* m_24; // +0x24 default surface A
    FShadeSurf* m_28; // +0x28 default surface B
    char p2c[0x30 - 0x2c];
    i32 m_30; // +0x30
    char p34[0x38 - 0x34];
    FShadeSurf* m_38; // +0x38 surface A
    FShadeSurf* m_3c; // +0x3c surface B
    FShadeSurf* m_40; // +0x40 surface C
    i32* m_44;        // +0x44 A column pitch table
    i32* m_48;        // +0x48 B column pitch table
    i32* m_4c;        // +0x4c C column pitch table
    i32 m_50;         // +0x50 mode
    i32 m_54;         // +0x54
    i32 m_58;         // +0x58 ramp size
    i32 m_5c;         // +0x5c active flag
    i32 m_60;         // +0x60 A height
    i32 m_64;         // +0x64 A width
    i32 m_68;         // +0x68 B height
    i32 m_6c;         // +0x6c B width
    i32 m_70;         // +0x70 C height
    i32 m_74;         // +0x74 C width
    char p78[0x478 - 0x78];
    i32* m_478; // +0x478 acos warp table
    char p47c[0x488 - 0x47c];
    void* m_488; // +0x488 scratch line
    u8* m_48c;   // +0x48c sin highlight ramp

    i32 Setup(FInit* pInit); // 0x1817e0
};

// @early-stop
// x87 scheduling wall (~40-50%): the surface resolution, the equal-dimension
// validation, the per-column pitch tables (m_44/m_48/m_4c) and the m_488 scratch
// allocation match, but the two transcendental ramp loops are not source-steerable
// - retail's fild/fxch/fstp stack-slot reuse over `acos((i-r)/r)*r` (m_478) and
// `0x10 - sin(i/r*PI)*-32` (m_48c), plus the MFC CString temp the file-name
// AddFromArray path builds, diverge. Same family as CFaderRadial::Build (0x17fa40).
RVA(0x001817e0, 0x315)
i32 CFaderShape::Setup(FInit* pInit) {
    i32 i;
    this->m_20 = 0;
    if (pInit == 0) {
        return 0;
    }

    this->m_38 = pInit->m_04 ? pInit->m_04 : this->m_24;
    this->m_3c = pInit->m_08 ? pInit->m_08 : this->m_28;
    if (this->m_38 == 0) {
        return 0;
    }
    if (this->m_3c == 0) {
        return 0;
    }
    this->m_40 = pInit->m_0c ? (FShadeSurf*)pInit->m_0c : this->m_3c;

    if (!this->m_04.Init()) {
        return 0;
    }

    this->m_60 = this->m_38->m_1c;
    this->m_64 = this->m_38->m_18;
    this->m_68 = this->m_3c->m_1c;
    this->m_6c = this->m_3c->m_18;
    this->m_70 = this->m_40->m_1c;
    this->m_74 = this->m_40->m_18;
    if (this->m_60 != this->m_68) {
        return 0;
    }
    if (this->m_64 != this->m_6c) {
        return 0;
    }
    if (this->m_60 != this->m_70) {
        return 0;
    }
    if (this->m_64 != this->m_74) {
        return 0;
    }
    if (this->m_70 != this->m_68) {
        return 0;
    }
    if (this->m_74 != this->m_6c) {
        return 0;
    }

    if (pInit->m_14 == 0) {
        return 0;
    }
    if ((u32)pInit->m_14 >= 4) {
        return 0;
    }
    this->m_50 = pInit->m_14;
    this->m_54 = pInit->m_18;
    this->m_58 = pInit->m_10;

    if (this->m_50 == 1 || this->m_50 == 2) {
        if (this->m_60 < (i32)((double)this->m_58 * 3.141592653589793)) {
            return 0;
        }
    }

    this->m_478 = (i32*)RezAlloc(this->m_58 * 8);
    for (i = 0; i < 2 * this->m_58; i++) {
        this->m_478[i] =
            (i32)(acos(((float)i - (float)this->m_58) / (float)this->m_58) * (float)this->m_58);
    }

    this->m_5c = pInit->m_1c;
    if (this->m_38->m_a8 != 8) {
        this->m_5c = 0;
    }

    if (this->m_5c != 0) {
        if (pInit->m_20) {
            this->m_30 = 0;
            this->m_1c = (void*)pInit->m_20;
        } else if (_access(pInit->m_24, 0) == 0) {
            this->m_1c = this->m_04.AddFromArray(pInit->m_24);
            if (this->m_1c == 0) {
                this->m_5c = 0;
            }
        } else {
            this->m_1c = this->m_04.FlashTable(pInit->m_28->m_0c, 0x20, 0x20, 0x32, 0xc8);
        }

        i32 m = this->m_58 << 1;
        this->m_48c = (u8*)RezAlloc(m);
        for (i = 0; i < m; i++) {
            i32 t = (i32)(sin((float)i / (float)this->m_58 * 3.14f) * -32.0);
            this->m_48c[i] = (u8)(0x10 - t);
        }
    }

    this->m_44 = (i32*)RezAlloc(this->m_64 * 4);
    this->m_48 = (i32*)RezAlloc(this->m_6c * 4);
    this->m_4c = (i32*)RezAlloc(this->m_74 * 4);
    for (i = 0; i < this->m_64; i++) {
        this->m_44[i] = this->m_38->m_20 * i;
        this->m_48[i] = this->m_3c->m_20 * i;
        this->m_4c[i] = this->m_40->m_20 * i;
    }

    i32 mx = (this->m_64 > this->m_60) ? this->m_64 : this->m_60;
    this->m_488 = RezAlloc(this->m_38->m_b0 * mx);
    return 1;
}

SIZE_UNKNOWN(ShadeCache);
SIZE_UNKNOWN(FShadeSurf);
SIZE_UNKNOWN(FInitPal);
SIZE_UNKNOWN(FInit);
SIZE_UNKNOWN(CFaderShape);

