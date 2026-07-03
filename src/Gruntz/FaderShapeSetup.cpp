// FaderShapeSetup.cpp - a CFaderMgr-family screen-fader subclass init (0x1817e0).
// __thiscall, called by CFaderMgr::Add (0x17d9c0). Resolves three equal-sized
// source surfaces (m_surfA/m_surfB/m_surfC) from the config arg (defaulting to the fader's
// own m_defaultSurfA/m_defaultSurfB), validates their dimensions match, builds an acos warp table
// (m_warpTable) and a sin highlight ramp (m_highlightRamp), resolves the shade table via the
// embedded CShadeTableCache (file/array/flash), then allocates per-column pitch
// tables (m_pitchA/m_pitchB/m_pitchC) and a scratch line (m_scratchLine). Field names are placeholders;
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
    i32 m_width;  // +0x18 width
    i32 m_height; // +0x1c height
    i32 m_pitch;  // +0x20 row pitch
    char p24[0xa8 - 0x24];
    i32 m_format; // +0xa8 format
    char pac[0xb0 - 0xac];
    i32 m_colStride; // +0xb0 column stride
};

// The +0x28 config arg's palette holder.
struct FInitPal {
    char p00[0xc];
    void* m_palBase; // +0x0c palette base
};

// The config arg (CFaderMgr::Add's pInit).
struct FInit {
    char p00[0x4];
    FShadeSurf* m_surfA;   // +0x04 surface A override
    FShadeSurf* m_surfB;   // +0x08 surface B override
    FShadeSurf* m_surfC;   // +0x0c surface C override
    i32 m_rampSize;        // +0x10 ramp size
    i32 m_mode;            // +0x14 mode (1..3)
    i32 m_18;              // +0x18
    i32 m_gate;            // +0x1c gate
    void* m_prebuiltTable; // +0x20 prebuilt shade table (pointer)
    char* m_tableName;     // +0x24 table file/array name
    FInitPal* m_flashPal;  // +0x28 flash palette source
};

struct CFaderShape {
    char p00[0x4];
    ShadeCache m_cache;         // +0x04 cache subobject (0x18)
    void* m_shadeTable;         // +0x1c resolved shade table
    i32 m_20;                   // +0x20
    FShadeSurf* m_defaultSurfA; // +0x24 default surface A
    FShadeSurf* m_defaultSurfB; // +0x28 default surface B
    char p2c[0x30 - 0x2c];
    i32 m_30; // +0x30
    char p34[0x38 - 0x34];
    FShadeSurf* m_surfA; // +0x38 surface A
    FShadeSurf* m_surfB; // +0x3c surface B
    FShadeSurf* m_surfC; // +0x40 surface C
    i32* m_pitchA;       // +0x44 A column pitch table
    i32* m_pitchB;       // +0x48 B column pitch table
    i32* m_pitchC;       // +0x4c C column pitch table
    i32 m_mode;          // +0x50 mode
    i32 m_54;            // +0x54
    i32 m_rampSize;      // +0x58 ramp size
    i32 m_active;        // +0x5c active flag
    i32 m_heightA;       // +0x60 A height
    i32 m_widthA;        // +0x64 A width
    i32 m_heightB;       // +0x68 B height
    i32 m_widthB;        // +0x6c B width
    i32 m_heightC;       // +0x70 C height
    i32 m_widthC;        // +0x74 C width
    char p78[0x478 - 0x78];
    i32* m_warpTable; // +0x478 acos warp table
    char p47c[0x488 - 0x47c];
    void* m_scratchLine; // +0x488 scratch line
    u8* m_highlightRamp; // +0x48c sin highlight ramp

    i32 Setup(FInit* pInit); // 0x1817e0
};

// @early-stop
// x87 scheduling wall (~40-50%): the surface resolution, the equal-dimension
// validation, the per-column pitch tables (m_pitchA/m_pitchB/m_pitchC) and the m_scratchLine scratch
// allocation match, but the two transcendental ramp loops are not source-steerable
// - retail's fild/fxch/fstp stack-slot reuse over `acos((i-r)/r)*r` (m_warpTable) and
// `0x10 - sin(i/r*PI)*-32` (m_highlightRamp), plus the MFC CString temp the file-name
// AddFromArray path builds, diverge. Same family as CFaderRadial::Build (0x17fa40).
RVA(0x001817e0, 0x315)
i32 CFaderShape::Setup(FInit* pInit) {
    i32 i;
    this->m_20 = 0;
    if (pInit == 0) {
        return 0;
    }

    this->m_surfA = pInit->m_surfA ? pInit->m_surfA : this->m_defaultSurfA;
    this->m_surfB = pInit->m_surfB ? pInit->m_surfB : this->m_defaultSurfB;
    if (this->m_surfA == 0) {
        return 0;
    }
    if (this->m_surfB == 0) {
        return 0;
    }
    this->m_surfC = pInit->m_surfC ? pInit->m_surfC : this->m_surfB;

    if (!this->m_cache.Init()) {
        return 0;
    }

    this->m_heightA = this->m_surfA->m_height;
    this->m_widthA = this->m_surfA->m_width;
    this->m_heightB = this->m_surfB->m_height;
    this->m_widthB = this->m_surfB->m_width;
    this->m_heightC = this->m_surfC->m_height;
    this->m_widthC = this->m_surfC->m_width;
    if (this->m_heightA != this->m_heightB) {
        return 0;
    }
    if (this->m_widthA != this->m_widthB) {
        return 0;
    }
    if (this->m_heightA != this->m_heightC) {
        return 0;
    }
    if (this->m_widthA != this->m_widthC) {
        return 0;
    }
    if (this->m_heightC != this->m_heightB) {
        return 0;
    }
    if (this->m_widthC != this->m_widthB) {
        return 0;
    }

    if (pInit->m_mode == 0) {
        return 0;
    }
    if ((u32)pInit->m_mode >= 4) {
        return 0;
    }
    this->m_mode = pInit->m_mode;
    this->m_54 = pInit->m_18;
    this->m_rampSize = pInit->m_rampSize;

    if (this->m_mode == 1 || this->m_mode == 2) {
        if (this->m_heightA < (i32)((double)this->m_rampSize * 3.141592653589793)) {
            return 0;
        }
    }

    this->m_warpTable = (i32*)RezAlloc(this->m_rampSize * 8);
    for (i = 0; i < 2 * this->m_rampSize; i++) {
        this->m_warpTable[i] =
            (i32)(acos(((float)i - (float)this->m_rampSize) / (float)this->m_rampSize)
                  * (float)this->m_rampSize);
    }

    this->m_active = pInit->m_gate;
    if (this->m_surfA->m_format != 8) {
        this->m_active = 0;
    }

    if (this->m_active != 0) {
        if (pInit->m_prebuiltTable) {
            this->m_30 = 0;
            this->m_shadeTable = pInit->m_prebuiltTable;
        } else if (_access(pInit->m_tableName, 0) == 0) {
            this->m_shadeTable = this->m_cache.AddFromArray(pInit->m_tableName);
            if (this->m_shadeTable == 0) {
                this->m_active = 0;
            }
        } else {
            this->m_shadeTable =
                this->m_cache.FlashTable(pInit->m_flashPal->m_palBase, 0x20, 0x20, 0x32, 0xc8);
        }

        i32 m = this->m_rampSize << 1;
        this->m_highlightRamp = (u8*)RezAlloc(m);
        for (i = 0; i < m; i++) {
            i32 t = (i32)(sin((float)i / (float)this->m_rampSize * 3.14f) * -32.0);
            this->m_highlightRamp[i] = (u8)(0x10 - t);
        }
    }

    this->m_pitchA = (i32*)RezAlloc(this->m_widthA * 4);
    this->m_pitchB = (i32*)RezAlloc(this->m_widthB * 4);
    this->m_pitchC = (i32*)RezAlloc(this->m_widthC * 4);
    for (i = 0; i < this->m_widthA; i++) {
        this->m_pitchA[i] = this->m_surfA->m_pitch * i;
        this->m_pitchB[i] = this->m_surfB->m_pitch * i;
        this->m_pitchC[i] = this->m_surfC->m_pitch * i;
    }

    i32 mx = (this->m_widthA > this->m_heightA) ? this->m_widthA : this->m_heightA;
    this->m_scratchLine = RezAlloc(this->m_surfA->m_colStride * mx);
    return 1;
}

SIZE_UNKNOWN(ShadeCache);
SIZE_UNKNOWN(FShadeSurf);
SIZE_UNKNOWN(FInitPal);
SIZE_UNKNOWN(FInit);
SIZE_UNKNOWN(CFaderShape);
