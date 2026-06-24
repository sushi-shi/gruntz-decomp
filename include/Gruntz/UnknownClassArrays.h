// UnknownClassArrays.h - the (tomalla-named) config/array bundle whose ctor,
// dtor, and FreeArrays method byte-match retail. The class owns four growable MFC
// arrays - two CPtrArray (+0xdc / +0xf0) and two CDWordArray (+0x104 / +0x118) -
// followed by a block of scalar config fields the ctor seeds with magic startup
// values. FreeArrays recycles the two CPtrArrays' element pointers onto a global
// intrusive freelist, then empties all four arrays.
//
// The array types are FIXED by their retail RTTI/vtable records:
//   +0xdc / +0xf0   vtable s_CPtrArray  @0x5ec2dc  -> CPtrArray
//   +0x104 / +0x118 vtable s_CDWordArray@0x5ec29c  -> CDWordArray
// Both are the real MFC afxcoll classes (0x14 B: vptr@0, m_pData@+4, m_nSize@+8,
// m_nMaxSize@+0xc, m_nGrowBy@+0x10).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine). The class is referenced from
// CBattlezMapConfig (the Battlez map-config loader operates on this same layout).
#ifndef SRC_GRUNTZ_UNKNOWNCLASSARRAYS_H
#define SRC_GRUNTZ_UNKNOWNCLASSARRAYS_H
#include <rva.h>

#include <Mfc.h> // CPtrArray, CDWordArray (real afxcoll, 0x14 layout)

class UnknownClassArrays {
public:
    UnknownClassArrays();
    ~UnknownClassArrays();
    void FreeArrays();
    int winapi_0267c0_IntersectRect_PtInRect();
    int winapi_02a570_IntersectRect(int);
    int winapi_02ab80_PtInRect(int, int, int, int);
    int winapi_02ae00_IntersectRect(int, int);
    int winapi_02c140_IntersectRect_PtInRect(int);
    int winapi_02dfa0_IntersectRect(int, int, int, int);
    int winapi_02e3a0_PtInRect(int);
    int winapi_031ca0_IntersectRect(int);
    int winapi_032060_IntersectRect(int);

    char m_pad000[0x18];          // +0x000  (untouched by ctor)
    int m_018;                    // +0x018  = 0
    int m_01c;                    // +0x01c  = 1
    int m_020;                    // +0x020  = 0x40
    int m_024;                    // +0x024  = 0x40
    int m_028;                    // +0x028  = 0x40
    int m_02c;                    // +0x02c  = 0x32
    int m_030;                    // +0x030  = 0x32
    char m_pad034[0x48 - 0x34];   // +0x034  (untouched)
    int m_048;                    // +0x048  = 0
    int m_04c;                    // +0x04c  = 0
    int m_050;                    // +0x050  = 0
    int m_054;                    // +0x054  = 0
    int m_058;                    // +0x058  = 0
    int m_05c;                    // +0x05c  = 0
    char m_pad060[0x74 - 0x60];   // +0x060  (untouched)
    int m_074;                    // +0x074  = 0x19
    int m_078;                    // +0x078  = 0
    int m_07c;                    // +0x07c  = 0
    int m_080;                    // +0x080  = 0
    int m_084;                    // +0x084  = 0
    int m_088;                    // +0x088  = 0x32
    int m_08c;                    // +0x08c  = 5
    int m_090;                    // +0x090  = 5
    int m_094;                    // +0x094  = 8
    int m_098;                    // +0x098  = 8
    int m_09c;                    // +0x09c  = 0x7d0
    int m_0a0;                    // +0x0a0  = 0x7d0
    int m_0a4;                    // +0x0a4  = 6
    int m_0a8;                    // +0x0a8  = 0x32
    int m_0ac;                    // +0x0ac  = 8
    int m_0b0;                    // +0x0b0  = 8
    int m_0b4;                    // +0x0b4  = 0x3e8
    int m_0b8;                    // +0x0b8  = 0x7d0
    int m_0bc;                    // +0x0bc  = 0x3e8
    int m_0c0;                    // +0x0c0  = 0xa
    int m_0c4;                    // +0x0c4  = 0xbb8
    int m_0c8;                    // +0x0c8  = 0x7530
    int m_0cc;                    // +0x0cc  = 0xbb8
    char m_pad0d0[0xdc - 0xd0];   // +0x0d0  (untouched)
    CPtrArray m_0dc;              // +0x0dc  CPtrArray  (m_pData@+0xe0, m_nSize@+0xe4)
    CPtrArray m_0f0;              // +0x0f0  CPtrArray  (m_pData@+0xf4, m_nSize@+0xf8)
    CDWordArray m_104;            // +0x104  CDWordArray
    CDWordArray m_118;            // +0x118  CDWordArray
    char m_pad12c[0x13c - 0x12c]; // +0x12c  (untouched)
    int m_13c;                    // +0x13c  = 0
    int m_140;                    // +0x140  = 0
};

#endif // SRC_GRUNTZ_UNKNOWNCLASSARRAYS_H
