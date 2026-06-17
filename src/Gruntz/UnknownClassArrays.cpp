// UnknownClassArrays.cpp - nested array bundle inside CGruntzMgr (tomalla-
// derived, from C:\Proj\Gruntz\GruntzMgr.cpp).  Two methods:
//   ctor       @0x024dc0 (344 B)  - SEH-framed: constructs 4 MFC containers
//                                    (2x CPtrArray, 2x CDWordArray), seeds ~40
//                                    scalar config fields with magic constants.
//   FreeArrays @0x025ca0 (191 B)  - returns array elements to a global free
//                                    list, then calls SetSize(0,-1) on each of
//                                    the 4 MFC containers to release storage.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

// --- MFC container placeholders ---
class CPtrArray {
public:
    CPtrArray();                            // @0x1b4f0b
    ~CPtrArray();                           // (EH unwind)
    void SetSize(int nNewSize, int nGrowBy); // @0x1b4f75
    char _raw[0x14 - 4];                   // 0x14 incl vptr
};

class CDWordArray {
public:
    CDWordArray();                          // @0x1b4b43
    ~CDWordArray();                         // (EH unwind)
    void SetSize(int nNewSize, int nGrowBy); // @0x1b4bad
    char _raw[0x14 - 4];                   // 0x14 incl vptr
};

// --- Global free-list state (from the engine's memory-pool allocator) ---
// @data: 0x245544
extern void *g_freeListHead;                // VA 0x645544
// @data: 0x24554c
extern int   g_freeListBase;                // VA 0x64554c  (subtracted from element addr)

// ---------------------------------------------------------------------------
// UnknownClassArrays - reconstructed from the matched ctor + FreeArrays.
// Fields drawn from structure/game/cgruntzmgr.h; only the offsets touched
// by the matched methods are modeled below.  NO vtable.
// ---------------------------------------------------------------------------
class UnknownClassArrays {
public:
    UnknownClassArrays();
    void FreeArrays();

    char  _pad000[0x18];                     // +0x000
    int   m_018;                             // +0x018  = 0
    int   m_01C;                             // +0x01C  = 1
    int   m_020;                             // +0x020  = 0x40
    int   m_024;                             // +0x024  = 0x40
    int   m_028;                             // +0x028  = 0x40
    int   m_02C;                             // +0x02C  = 0x32
    int   m_030;                             // +0x030  = 0x32
    char  _pad034[0x48 - 0x34];             // +0x034
    int   m_048;                             // +0x048  = 0
    int   m_04C;                             // +0x04C  = 0
    int   m_050;                             // +0x050  = 0
    int   m_054;                             // +0x054  = 0
    int   m_058;                             // +0x058  = 0
    int   m_05C;                             // +0x05C  = 0
    char  _pad060[0x74 - 0x60];             // +0x060
    int   m_074;                             // +0x074  = 0x19
    int   m_078;                             // +0x078  = 0
    int   m_07C;                             // +0x07C  = 0
    int   m_080;                             // +0x080  = 0
    int   m_084;                             // +0x084  = 0
    int   m_088;                             // +0x088  = 0x32
    int   m_08C;                             // +0x08C  = 5
    int   m_090;                             // +0x090  = 5
    int   m_094;                             // +0x094  = 8
    int   m_098;                             // +0x098  = 8
    int   m_09C;                             // +0x09C  = 0x7d0
    int   m_0A0;                             // +0x0A0  = 0x7d0
    int   m_0A4;                             // +0x0A4  = 6
    int   m_0A8;                             // +0x0A8  = 0x32
    int   m_0AC;                             // +0x0AC  = 8
    int   m_0B0;                             // +0x0B0  = 8
    int   m_0B4;                             // +0x0B4  = 0x3e8
    int   m_0B8;                             // +0x0B8  = 0x7d0
    int   m_0BC;                             // +0x0BC  = 0x3e8
    int   m_0C0;                             // +0x0C0  = 0xa
    int   m_0C4;                             // +0x0C4  = 0xbb8
    int   m_0C8;                             // +0x0C8  = 0x7530
    int   m_0CC;                             // +0x0CC  = 0xbb8
    char  _pad0D0[0x0C];                    // +0x0D0
    // NOTE: FreeArrays reads array metadata at +0xe0 (data ptr), +0xe4 (count)
    // for the first CPtrArray's internal state.  These lie inside the CPtrArray
    // _raw padding and are accessed by the loop, not by named fields.
    CPtrArray  m_ptrArray1;                 // +0x0DC
    CPtrArray  m_ptrArray2;                 // +0x0F0
    CDWordArray m_dwordArray1;              // +0x104
    CDWordArray m_dwordArray2;              // +0x118
    char  _pad12C[0x10];                    // +0x12C
    int   m_13C;                            // +0x13C  = 0
    int   m_140;                            // +0x140  = 0
};                                          // 0x144

// ---------------------------------------------------------------------------
// UnknownClassArrays::UnknownClassArrays  @0x024dc0
// SEH-framed (/GX).  Constructs the four MFC containers via member initialiser
// list, then seeds ~40 scalar fields with their magic startup values (binding
// sizes, interval durations, mode flags, etc.).  The first four fields at
// +0x78/+0x7c/+0x80/+0x84 are zeroed BEFORE the constructor initialiser run,
// not by the body — they likely serve as constructor-cleanup EH state flags
// that the /GX compiler emits automatically.
//
// @address: 0x024dc0
// @size:    0x158
// ---------------------------------------------------------------------------
UnknownClassArrays::UnknownClassArrays()
    : m_ptrArray1()
    , m_ptrArray2()
    , m_dwordArray1()
    , m_dwordArray2()
{
    m_078 = 0;
    m_080 = 0;
    m_07C = 0;
    m_084 = 0;
    m_018 = 0;
    m_01C = 1;
    m_020 = 0x40;
    m_024 = 0x40;
    m_028 = 0x40;
    m_08C = 5;
    m_090 = 5;
    m_02C = 0x32;
    m_030 = 0x32;
    m_094 = 8;
    m_098 = 8;
    m_0AC = 8;
    m_0B0 = 8;
    m_0B4 = 0x3e8;
    m_0BC = 0x3e8;
    m_088 = 0x32;
    m_0A8 = 0x32;
    m_048 = 0;
    m_054 = 0;
    m_050 = 0;
    m_058 = 0;
    m_05C = 0;
    m_04C = 0;
    m_0C4 = 0xbb8;
    m_0CC = 0xbb8;
    m_13C = 0;
    m_140 = 0;
    m_09C = 0x7d0;
    m_0A0 = 0x7d0;
    m_0A4 = 6;
    m_0B8 = 0x7d0;
    m_0C0 = 0xa;
    m_0C8 = 0x7530;
    m_074 = 0x19;
}

// ---------------------------------------------------------------------------
// UnknownClassArrays::FreeArrays  @0x025ca0
// For each of the two CPtrArrays (+0xdc / +0xf0), iterates over their stored
// element pointers (accessed at +0xe0/+0xec = data ptr, +0xe4/+0xf0 = count)
// and returns each non-null element to the engine's global free list
// (g_freeListHead @0x645544, g_freeListBase @0x64554c).  Then calls
// SetSize(0, -1) on all four containers and clears m_13C.
//
// @address: 0x025ca0
// @size:    0xbf
// ---------------------------------------------------------------------------
void UnknownClassArrays::FreeArrays()
{
    // Return array-1 elements to the free list.  m_ptrArray1's internal data
    // is a pointer array at +0x04 (= the CPtrArray _raw start), accessible
    // through the _raw padding.  In C++ we access fields via raw offset.
    int count1 = *(int *)((char *)&m_ptrArray1 + 0x8);      // _count @ +0xe4
    int **data1 = *(int ***)((char *)&m_ptrArray1 + 0x4);   // _data  @ +0xe0
    for (int i = 0; i < count1; i++) {
        int *p = data1[i];
        if (p != 0) {
            int *adjusted = (int *)((char *)p - (int)g_freeListBase);
            *adjusted = (int)g_freeListHead;
            g_freeListHead = adjusted;
        }
    }

    m_ptrArray1.SetSize(0, -1);

    // Return array-2 elements to the free list.
    int count2 = *(int *)((char *)&m_ptrArray2 + 0x8);      // _count @ +0xf8
    int **data2 = *(int ***)((char *)&m_ptrArray2 + 0x4);   // _data  @ +0xf4
    int base = g_freeListBase;
    for (int j = 0; j < count2; j++) {
        int *p = data2[j];
        int *adjusted = (int *)((char *)p - base);
        *adjusted = (int)g_freeListHead;
        g_freeListHead = adjusted;
    }

    m_ptrArray2.SetSize(0, -1);
    m_dwordArray1.SetSize(0, -1);
    m_dwordArray2.SetSize(0, -1);
    m_13C = 0;
}
