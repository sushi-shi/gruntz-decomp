// BuildGruntzCrcInfo.cpp - BuildGruntzCrcInfo (0xbf1d0). A diagnostic dump:
// __thiscall(this) walks the level's flat grunt array (this->m_4->m_4->m_68,
// indexed 0x1c..0x108 by 4) and, for each present grunt, formats a
// "[p=%d][g=%d][health=%d]...[rnd=%d]" line (18 fields incl. a per-grunt
// type->weapon-id switch and a random nonce) and appends it to a CString seeded
// with "crc info for all gruntz:\n----...". The assembled report is handed to the
// sink (this->m_4->WriteLog).
//
// /GX EH-framed: the local CString (its ~CString must run on the normal + unwind
// exits) gives the body the exception frame, so it lives in an `eh` unit. Only
// offsets / strings / code bytes are load-bearing; the CString ops, wsprintfA,
// the rand nonce and WriteLog are reloc-masked engine calls.
#include <rva.h>

#include <Win32.h> // wsprintfA

// The engine's MFC CString (statically linked); reloc-masked engine calls:
//   CString(const char*) = 0x1b9d4c   ~CString() = 0x1b9cde   += = 0x1ba0c8
class CString {
public:
    CString(const char* s);     // 0x1b9d4c
    ~CString();                 // 0x1b9cde
    void Concat(const char* s); // 0x1ba0c8 (operator+= LPCSTR)
    char* m_pchData;            // +0x00
};

// A grunt record: only the dumped fields are named (sparse, raw offsets).
struct CrcGrunt {
    char m_pad00[0x10];
    struct CrcGruntPos* m_10; // +0x10  (x/y at +0x5c/+0x60)
    char m_pad14[0x170 - 0x14];
    i32 m_170; // +0x170  type id (switch input)
    char m_pad174[0x198 - 0x174];
    i32 m_198; // +0x198  toy
    i32 m_19c; // +0x19c  tool override (type > 0x16)
    char m_pad1a0[0x218 - 0x1a0];
    i32 m_218; // +0x218  ia
    i32 m_21c; // +0x21c  qat
    i32 m_220; // +0x220  iic
    i32 m_224; // +0x224  da
    char m_pad228[0x358 - 0x228];
    i32 m_358; // +0x358  iad
    char m_pad35c[0x3ec - 0x35c];
    i32 m_3ec; // +0x3ec  health
    i32 m_3f0; // +0x3f0  stm
    i32 m_3f4; // +0x3f4  ttl
    char m_pad3f8[0x444 - 0x3f8];
    i32 m_444; // +0x444  dir
    char m_pad448[0x450 - 0x448];
    i32 m_450; // +0x450  qax
};
struct CrcGruntPos {
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c  x
    i32 m_60; // +0x60  y
};
struct CrcLevelHolder {
    char m_pad00[0x68];
    void* m_68; // +0x68  flat grunt-pointer array base
};
struct CrcSink {
    char m_pad00[0x4];
    CrcLevelHolder* m_4;          // +0x04
    void WriteLog(char* s, i32 z); // FUN_00101af0 __thiscall
};
struct CrcOwner {
    char m_pad00[0x4];
    CrcSink* m_4; // +0x04
    void BuildGruntzCrcInfo();
};

extern "C" char g_emptyString[]; // 0x6293f4 (== "")
// FUN_0011fee0 __cdecl, 0-arg: the per-grunt random nonce ("rnd").
extern i32 GruntCrcRand();

// @source: decomp-xref
// @early-stop
// /GX EH-state megafunction wall: the body is reconstructed in full (the seeded
// CString, the flat grunt-array double walk, the 0x16-case type->weapon switch,
// the 18-field wsprintf line + two appends, the WriteLog handoff and the
// destructor). The byte residual is the documented /GX exception-state numbering +
// jump-table base reloc typing around the CString temp (cf. the sibling
// megafunctions RollingBall/TerrainTileLoader): not source-steerable. See
// docs/patterns/big-seh-fuzzy-desync.md + eh-state-numbering-base.md.
RVA(0x000bf1d0, 0x249)
void CrcOwner::BuildGruntzCrcInfo() {
    char szLine[0x100];
    szLine[0] = g_emptyString[0];
    memset(szLine + 1, 0, sizeof(szLine) - 1);

    CString info("crc info for all gruntz:\n------------------------\n");

    i32 arrOff = 0x1c;
    for (i32 player = 0; arrOff < 0x10c; player++) {
        for (i32 g = 0; g < 0xf; g++, arrOff += 4) {
            CrcGrunt* grunt = *(CrcGrunt**)((char*)m_4->m_4->m_68 + arrOff);
            if (grunt == 0) {
                continue;
            }
            i32 rnd = GruntCrcRand();
            i32 type = grunt->m_170;
            i32 wp;
            switch (type) {
                case 1: wp = 2; break;
                case 2: wp = 9; break;
                case 3: wp = 0xe; break;
                case 4: wp = 6; break;
                case 5: wp = 0xb; break;
                case 6: wp = 0x13; break;
                case 7: wp = 0x11; break;
                case 8: wp = 0xf; break;
                case 9: wp = 5; break;
                case 10: wp = 0x15; break;
                case 0xb: wp = 7; break;
                case 0xc: wp = 0x10; break;
                case 0xd: wp = 8; break;
                case 0xe: wp = 0xa; break;
                case 0xf: wp = 0xd; break;
                case 0x10: wp = 4; break;
                case 0x11: wp = 0x14; break;
                case 0x12: wp = 0x12; break;
                case 0x13: wp = 0x16; break;
                case 0x15: wp = 3; break;
                case 0x16: wp = 0xc; break;
                default: wp = 0x17; break;
            }
            i32 tool = type;
            if (type > 0x16) {
                tool = grunt->m_19c;
            }
            wsprintfA(szLine, "[p=%d][g=%d][health=%d][x=%d][y=%d][dir=%d][stm=%d][ttl=%d][tool=%d]"
                              "[toy=%d][da=%d][wp=%d][iic=%d][qat=%d][qax=%d][ia=%d][iad=%d][rnd=%d]\n",
                      player, g, grunt->m_3ec, grunt->m_10->m_5c, grunt->m_10->m_60, grunt->m_444,
                      grunt->m_3f0, grunt->m_3f4, tool, grunt->m_198, grunt->m_224, wp,
                      grunt->m_220, grunt->m_21c, grunt->m_450, grunt->m_218, grunt->m_358, rnd);
            info.Concat("\n");
            info.Concat(szLine);
        }
    }
    m_4->WriteLog(info.m_pchData, 0);
}
