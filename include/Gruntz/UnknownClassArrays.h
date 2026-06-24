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
    i32 Method_025c20();
    void Clear_02ade0();
    i32 Method_02c0a0(i32, i32);
    i32 Method_030530(i32);
    i32 Method_0305b0(i32, i32, i32);
    i32 Method_02bfc0(i32, void*, i32, i32);
    i32 Method_02ed90(i32);
    i32 Serialize_02b420(void*);
    i32 Method_030730(i32, i32, i32, i32);
    void* Method_030f20(void*, i32, i32);
    i32 winapi_0267c0_IntersectRect_PtInRect();
    i32 winapi_02a570_IntersectRect(i32);
    i32 winapi_02ab80_PtInRect(i32, i32, i32, i32);
    i32 winapi_02ae00_IntersectRect(i32, i32);
    i32 winapi_02c140_IntersectRect_PtInRect(i32);
    i32 winapi_02dfa0_IntersectRect(i32, i32, i32, i32);
    i32 winapi_02e3a0_PtInRect(i32);
    i32 winapi_031ca0_IntersectRect(i32);
    i32 winapi_032060_IntersectRect(i32);

    i32 m_000;                  // +0x000  (vtbl-slot / first dword; some methods test/clear it)
    void* m_004;                // +0x004  level/board object pointer
    char* m_008;                // +0x008  grid base: a cell array, 0x3c-byte stride
    void* m_00c;                // +0x00c  board/tile-map object pointer
    char m_pad010[0x18 - 0x10]; // +0x010  (untouched by ctor)
    i32 m_018;                  // +0x018  = 0  (current cell index)
    i32 m_01c;                  // +0x01c  = 1
    i32 m_020;                  // +0x020  = 0x40
    i32 m_024;                  // +0x024  = 0x40
    i32 m_028;                  // +0x028  = 0x40
    i32 m_02c;                  // +0x02c  = 0x32
    i32 m_030;                  // +0x030  = 0x32
    i32 m_034;                  // +0x034  (serialized)
    i32 m_038;                  // +0x038  (serialized)
    i32 m_03c;                  // +0x03c  (serialized)
    i32 m_040;                  // +0x040  (serialized)
    i32 m_044;                  // +0x044  (serialized)
    i32 m_048;                  // +0x048  = 0
    i32 m_04c;                  // +0x04c  = 0
    i32 m_050;                  // +0x050  = 0
    i32 m_054;                  // +0x054  = 0
    i32 m_058;                  // +0x058  = 0
    i32 m_05c;                  // +0x05c  = 0
    i32 m_060;                  // +0x060  (serialized)
    i32 m_064;                  // +0x064  (serialized)
    i32 m_068;                  // +0x068  (serialized)
    i32 m_06c;                  // +0x06c  (serialized)
    i32 m_070;                  // +0x070  (serialized)
    i32 m_074;                  // +0x074  = 0x19
    i32 m_078;                  // +0x078  = 0
    i32 m_07c;                  // +0x07c  = 0
    i32 m_080;                  // +0x080  = 0
    i32 m_084;                  // +0x084  = 0
    i32 m_088;                  // +0x088  = 0x32
    i32 m_08c;                  // +0x08c  = 5
    i32 m_090;                  // +0x090  = 5
    i32 m_094;                  // +0x094  = 8
    i32 m_098;                  // +0x098  = 8
    i32 m_09c;                  // +0x09c  = 0x7d0
    i32 m_0a0;                  // +0x0a0  = 0x7d0
    i32 m_0a4;                  // +0x0a4  = 6
    i32 m_0a8;                  // +0x0a8  = 0x32
    i32 m_0ac;                  // +0x0ac  = 8
    i32 m_0b0;                  // +0x0b0  = 8
    i32 m_0b4;                  // +0x0b4  = 0x3e8
    i32 m_0b8;                  // +0x0b8  = 0x7d0
    i32 m_0bc;                  // +0x0bc  = 0x3e8
    i32 m_0c0;                  // +0x0c0  = 0xa
    i32 m_0c4;                  // +0x0c4  = 0xbb8
    i32 m_0c8;                  // +0x0c8  = 0x7530
    i32 m_0cc;                  // +0x0cc  = 0xbb8
    i32 m_0d0;                  // +0x0d0  (serialized, 8 B with m_0d4)
    i32 m_0d4;                  // +0x0d4
    i32 m_0d8;                  // +0x0d8  (serialized)
    CPtrArray m_0dc;            // +0x0dc  CPtrArray  (m_pData@+0xe0, m_nSize@+0xe4)
    CPtrArray m_0f0;            // +0x0f0  CPtrArray  (m_pData@+0xf4, m_nSize@+0xf8)
    CDWordArray m_104;          // +0x104  CDWordArray
    CDWordArray m_118;          // +0x118  CDWordArray
    i32 m_12c;                  // +0x12c  (serialized, 4-dword inline array)
    i32 m_130;                  // +0x130
    i32 m_134;                  // +0x134
    i32 m_138;                  // +0x138
    i32 m_13c;                  // +0x13c  = 0
    i32 m_140;                  // +0x140  = 0
    i32 m_144;                  // +0x144
    i32 m_148;                  // +0x148  (cleared by 02c0a0)
    i32 m_14c;                  // +0x14c  (serialized)
};

#endif // SRC_GRUNTZ_UNKNOWNCLASSARRAYS_H
