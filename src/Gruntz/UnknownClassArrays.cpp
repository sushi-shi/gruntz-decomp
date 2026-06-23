// UnknownClassArrays.cpp - the ctor / dtor / FreeArrays of the (tomalla-named)
// config-array bundle. The class owns four growable MFC arrays - two CPtrArray
// (+0xdc / +0xf0) and two CDWordArray (+0x104 / +0x118) - and a block of scalar
// config fields. See <Gruntz/UnknownClassArrays.h> for the layout and the array
// type derivation from the retail RTTI/vtable records.
//
//   ctor       @0x024dc0 (0x158 B)  - /GX EH frame: member-constructs the four
//                                      arrays (try-level advances per member),
//                                      then seeds ~40 scalar fields with magic
//                                      startup constants.
//   dtor       @0x024f80 (0x7d  B)  - /GX: calls FreeArrays(), then auto-destructs
//                                      the four arrays in reverse (try-level 3..-1).
//   FreeArrays @0x025ca0 (0xbf  B)  - recycles the two CPtrArrays' element pointers
//                                      onto the global intrusive freelist, then
//                                      SetSize(0,-1) on all four arrays.
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing.
// ---------------------------------------------------------------------------
#include <rva.h>

#include <Gruntz/UnknownClassArrays.h>

// ---- Reloc-masked engine globals --------------------------------------------
// The intrusive freelist head: a singly-linked list of recycled coord-pair nodes
// (node->next at +0). Shared with CBattlezMapConfig's allocator (which pulls nodes
// off it); FreeArrays pushes them back. Referenced as data (DIR32).
DATA(0x00245544)
extern void* g_freeList;

// The element<->node bias subtracted from a stored element pointer to recover its
// freelist node header (the allocator hands out node + bias; recycle reverses it).
DATA(0x0024554c)
extern int g_freeListNodeBias;

// ===========================================================================
// UnknownClassArrays::UnknownClassArrays  @0x024dc0
// Member-constructs the four arrays (CPtrArray x2, CDWordArray x2) - the /GX
// compiler frames the ctor and advances the EH try-level after each constructed
// member - then seeds the scalar config block. Returns `this`.
// ===========================================================================
RVA(0x00024dc0, 0x158)
UnknownClassArrays::UnknownClassArrays()
    // The four 0x78..0x87 fields are member-init-list initializations (NOT body
    // assignments): the /GX compiler schedules them into the array-construction
    // region (some land before the first array ctor), which is what retail does -
    // modeling them as body stores drops the ctor ~28%. VC5 emits the init list in
    // DECLARATION order regardless of the order written here.
    : m_078(0), m_07c(0), m_080(0), m_084(0) {
    m_018 = 0;
    m_01c = 1;
    m_020 = 0x40;
    m_024 = 0x40;
    m_028 = 0x40;
    m_08c = 5;
    m_090 = 5;
    m_02c = 0x32;
    m_094 = 8;
    m_098 = 8;
    m_0ac = 8;
    m_0b0 = 8;
    m_030 = 0x32;
    m_0b4 = 0x3e8;
    m_0bc = 0x3e8;
    m_088 = 0x32;
    m_0a8 = 0x32;
    m_048 = 0;
    m_054 = 0;
    m_050 = 0;
    m_058 = 0;
    m_05c = 0;
    m_04c = 0;
    m_0c4 = 0xbb8;
    m_0cc = 0xbb8;
    m_13c = 0;
    m_140 = 0;
    m_09c = 0x7d0;
    m_0a0 = 0x7d0;
    m_0a4 = 6;
    m_0b8 = 0x7d0;
    m_0c0 = 0xa;
    m_0c8 = 0x7530;
    m_074 = 0x19;
}

// ===========================================================================
// UnknownClassArrays::~UnknownClassArrays  @0x024f80
// Calls FreeArrays() (covered by the full unwind, try-level 3), then the compiler
// auto-destructs the four arrays in reverse construction order (+0x118, +0x104,
// +0xf0, +0xdc), lowering the try-level after each.
// ===========================================================================
RVA(0x00024f80, 0x7d)
UnknownClassArrays::~UnknownClassArrays() {
    FreeArrays();
}

// ===========================================================================
// UnknownClassArrays::FreeArrays  @0x025ca0
// For each non-null element of the two CPtrArrays (+0xdc, +0xf0), recover its
// freelist node (element - bias), push it onto g_freeList. Loop 1 guards on a
// non-null element; loop 2 does not (the retail asymmetry). Then SetSize(0,-1)
// empties all four arrays and m_13c is cleared.
// ===========================================================================
RVA(0x00025ca0, 0xbf)
void UnknownClassArrays::FreeArrays() {
    int i;
    for (i = 0; i < m_0dc.GetSize(); i++) {
        void* p = m_0dc[i];
        if (p != 0) {
            void** node = (void**)((char*)p - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_0dc.SetSize(0, -1);

    for (i = 0; i < m_0f0.GetSize(); i++) {
        void** node = (void**)((char*)m_0f0[i] - g_freeListNodeBias);
        *node = g_freeList;
        g_freeList = node;
    }
    m_0f0.SetSize(0, -1);

    m_104.SetSize(0, -1);
    m_118.SetSize(0, -1);
    m_13c = 0;
}
