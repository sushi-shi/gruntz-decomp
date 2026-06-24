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

// CRT rand() (RVA 0x11fee0); used by the grid-scan helpers to pick a random
// neighbour. External, reloc-masked.
extern "C" i32 rand(void);
// CRT abs(); inlined by MSVC5 to the branchless cdq/xor/sub sequence.
extern "C" i32 abs(i32);

// The WwdGameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). It
// fronts an array of per-level records (0x238-byte stride = the
// UnknownClassInCGruntzMgr sub-objects); only the two fields Method_025c20 reads
// are named. Reloc-masked DATA. A struct (mangles `U`) gives the retail name.
struct WwdGameReg {
    char m_pad000[0x164];
    i32 m_164; // +0x164  per-level "loaded" flag
    char m_pad168[0x170 - 0x168];
    i32 m_170; // +0x170  per-level "active" flag
    char m_pad174[0x238 - 0x174];
};

// The per-element refresh method (RVA ~0x021906), a __thiscall on the array
// bundle taking one int. Modeled as a method on a tiny helper laid over `this`
// so the `mov ecx,this; push 0; call` falls out, reloc-masked (no body).
struct ElementRefresher {
    void Refresh(i32 index); // ~0x021906
};

// The argument object of Method_02bfc0: a polymorphic unit whose vtable carries
// two pair-emit slots at +0x2c (index 11) / +0x30 (index 12). The slot call is a
// __thiscall indirect (mov eax,[obj]; call [eax+slot]); modeled with real
// virtuals so the right convention falls out (the __thiscall keyword is
// unspellable on a fn-ptr under MSVC5, see docs/patterns/dummy-virtual-slots.md).
struct EmitArg {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void Emit2c(void* coord, i32 count); // +0x2c (index 11)
    virtual void Emit30(void* coord, i32 count); // +0x30 (index 12)
};

// The serializer/archive the Serialize method writes into: a polymorphic object
// whose vtable carries a Write(void* buf, uint count) slot at +0x30 (index 12).
// The slot call is a __thiscall indirect (mov edx,[ar]; call [edx+0x30]); modeled
// with real virtuals so the convention falls out (see dummy-virtual-slots.md).
struct Serializer {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void Write(const void* buf, u32 count); // +0x30 (index 12)
};

// A free helper (RVA 0x01146a) that validates a kind-7 EmitArg; nonzero result
// means "skip the emit". __stdcall (callee pops its one arg — no add esp at the
// call site). External, reloc-masked.
i32 __stdcall Validate_01146a(EmitArg*);

// The kind-4 validator (RVA 0x022040), a __thiscall method on the array bundle.
// Modeled as a method on a tiny helper laid over `this`, so the `mov ecx,this;
// call` lowers cleanly and reloc-masks (external, no body).
struct Kind4Validator {
    i32 Validate(EmitArg*); // 0x022040
};

// A CGrunt unit (the huge game-object the grid stores). Modeled by raw offset:
// it is a separate, far larger class; only the offsets these methods touch are
// named. m_2d4 = a state slot, m_2d8 = a mode, m_2f0/m_2f4 = a target coord.
struct GridUnit {
    char m_pad000[0x10];
    void* m_010; // +0x010  UnitLevel* (board geometry)
    char m_pad014[0x170 - 0x14];
    i32 m_170; // +0x170  primary anim/state id
    i32 m_174; // +0x174  packed x (>>5)
    i32 m_178; // +0x178  packed y (>>5)
    char m_pad17c[0x19c - 0x17c];
    i32 m_19c; // +0x19c  secondary anim/state id (used when m_170 > 0x16)
    char m_pad1a0[0x1ec - 0x1a0];
    i32 m_1ec; // +0x1ec
    i32 m_1f0; // +0x1f0
    char m_pad1f4[0x250 - 0x1f4];
    i32 m_250; // +0x250
    i32 m_254; // +0x254
    i32 m_258; // +0x258  (packed coord pair base)
    char m_pad25c[0x2d4 - 0x25c];
    i32 m_2d4; // +0x2d4
    i32 m_2d8; // +0x2d8  mode
    char m_pad2dc[0x2e0 - 0x2dc];
    i32 m_2e0; // +0x2e0
    i32 m_2e4; // +0x2e4
    i32 m_2e8; // +0x2e8
    i32 m_2ec; // +0x2ec
    i32 m_2f0; // +0x2f0
    i32 m_2f4; // +0x2f4
    char m_pad2f8[0x31c - 0x2f8];
    char m_31c[0x320 - 0x31c]; // +0x31c  occupied-coords list object base
    void* m_320;               // +0x320  list head (node->next at +0)
    void* m_324;               // +0x324
    i32 m_328;                 // +0x328  list count/flag
};

// A {x, y} coordinate pair (a list node's +0x8 payload).
struct Coord {
    i32 m_x; // +0x00
    i32 m_y; // +0x04
};

// One occupied-coord list node: ->next at +0, ->coord at +8.
struct CoordNode {
    CoordNode* m_next; // +0x00
    char m_pad04[0x08 - 0x04];
    Coord* m_coord; // +0x08
};

// A candidate spawn-point node: {x, y} at +0 / +4. Lives in the per-level
// record's candidate array (rec->m_04[r]).
struct Candidate {
    i32 m_x; // +0x00
    i32 m_y; // +0x04
};

// A map tile: 0x1c-byte record; its first byte carries flag bits
// (bit 2 = 0x4 = "blocked"/special).
struct Tile {
    u8 m_flags; // +0x00
    char m_pad01[0x1c - 1];
};

// The board/tile map held at this->m_00c: m_rows is a row-pointer table; a row
// is a Tile array indexed by x. m_w / m_h are the in-bounds limits.
struct Board {
    char m_pad00[0x08];
    Tile** m_rows; // +0x08  rows[y][x] -> Tile
    i32 m_w;       // +0x0c  x bound
    i32 m_h;       // +0x10  y bound
};

// A level/board geometry object held on a unit at +0x10: its +0x5c / +0x60 carry
// a packed (x<<5)/(y<<5) coordinate.
struct UnitLevel {
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
};

// ---- Reloc-masked engine globals --------------------------------------------
// The intrusive freelist head: a singly-linked list of recycled coord-pair nodes
// (node->next at +0). Shared with CBattlezMapConfig's allocator (which pulls nodes
// off it); FreeArrays pushes them back. Referenced as data (DIR32).
DATA(0x00245544)
extern void* g_freeList;

// The element<->node bias subtracted from a stored element pointer to recover its
// freelist node header (the allocator hands out node + bias; recycle reverses it).
DATA(0x0024554c)
extern i32 g_freeListNodeBias;

// The WwdGameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c).
DATA(0x0024556c)
extern WwdGameReg* g_gameReg;

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
// UnknownClassArrays::Method_025c20  @0x025c20
// If the current level's WwdGameReg record is not-yet-loaded but active, refresh
// every element of the first CPtrArray (m_0dc). Returns 1 unconditionally.
// ===========================================================================
RVA(0x00025c20, 0x55)
i32 UnknownClassArrays::Method_025c20() {
    if (g_gameReg[m_018].m_164 == 0 && g_gameReg[m_018].m_170 != 0) {
        for (i32 i = 0; i < m_0dc.GetSize(); i++) {
            ((ElementRefresher*)this)->Refresh(0);
        }
    }
    return 1;
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
    i32 i;
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

// @confidence: low
// @source: winapi:IntersectRect;PtInRect
// @stub
RVA(0x000267c0, 0x281d)
i32 UnknownClassArrays::winapi_0267c0_IntersectRect_PtInRect() {
    return 0;
}

// @confidence: low
// @source: winapi:IntersectRect
// @stub
RVA(0x0002a570, 0x4c6)
i32 UnknownClassArrays::winapi_02a570_IntersectRect(i32) {
    return 0;
}

// @confidence: low
// @source: winapi:PtInRect
// @stub
RVA(0x0002ab80, 0x15e)
i32 UnknownClassArrays::winapi_02ab80_PtInRect(i32, i32, i32, i32) {
    return 0;
}

// ===========================================================================
// UnknownClassArrays::Clear_02ade0  @0x02ade0
// Single-store setter: zero the first dword. (mov [ecx],0; ret)
// ===========================================================================
RVA(0x0002ade0, 0x7)
void UnknownClassArrays::Clear_02ade0() {
    m_000 = 0;
}

// @confidence: low
// @source: winapi:IntersectRect
// @stub
RVA(0x0002ae00, 0x42e)
i32 UnknownClassArrays::winapi_02ae00_IntersectRect(i32, i32) {
    return 0;
}

// ===========================================================================
// UnknownClassArrays::Serialize_02b420  @0x02b420
// Stream every config scalar (then the four growable arrays + the inline 4-dword
// block) into the archive via its Write(buf, count) vtable slot. Each array
// section writes the element count first, then each element (the two CPtrArrays'
// elements are 8-byte payloads; the two CDWordArrays' are 4-byte dwords).
// ===========================================================================
RVA(0x0002b420, 0x419)
i32 UnknownClassArrays::Serialize_02b420(void* arArg) {
    Serializer* ar = (Serializer*)arArg;
    if (ar == 0) {
        return 0;
    }
    ar->Write(&m_000, 4);
    ar->Write(&m_018, 4);
    ar->Write(&m_01c, 4);
    ar->Write(&m_020, 4);
    ar->Write(&m_024, 4);
    ar->Write(&m_028, 4);
    ar->Write(&m_02c, 4);
    ar->Write(&m_030, 4);
    ar->Write(&m_034, 4);
    ar->Write(&m_038, 4);
    ar->Write(&m_03c, 4);
    ar->Write(&m_040, 4);
    ar->Write(&m_044, 4);
    ar->Write(&m_048, 4);
    ar->Write(&m_054, 4);
    ar->Write(&m_050, 4);
    ar->Write(&m_058, 4);
    ar->Write(&m_04c, 4);
    ar->Write(&m_05c, 4);
    ar->Write(&m_060, 4);
    ar->Write(&m_064, 4);
    ar->Write(&m_068, 4);
    ar->Write(&m_06c, 4);
    ar->Write(&m_070, 4);
    ar->Write(&m_074, 4);
    ar->Write(&m_088, 4);
    ar->Write(&m_08c, 4);
    ar->Write(&m_090, 4);
    ar->Write(&m_094, 4);
    ar->Write(&m_098, 4);
    ar->Write(&m_09c, 4);
    ar->Write(&m_0a0, 4);
    ar->Write(&m_0a4, 4);
    ar->Write(&m_0a8, 4);
    ar->Write(&m_0ac, 4);
    ar->Write(&m_0b0, 4);
    ar->Write(&m_0b4, 4);
    ar->Write(&m_0b8, 4);
    ar->Write(&m_0bc, 4);
    ar->Write(&m_0c0, 4);
    ar->Write(&m_0c4, 4);
    ar->Write(&m_0c8, 4);
    ar->Write(&m_0cc, 4);
    ar->Write(&m_0d0, 8);
    ar->Write(&m_0d8, 4);
    ar->Write(&m_13c, 4);
    ar->Write(&m_140, 4);
    ar->Write(&m_144, 4);
    ar->Write(&m_148, 4);
    ar->Write(&m_14c, 4);

    u32 i;
    u32 n = m_104.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        DWORD v = m_104[i];
        ar->Write(&v, 4);
    }

    n = m_118.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        DWORD v = m_118[i];
        ar->Write(&v, 4);
    }

    i32* p = &m_12c;
    for (i32 k = 0; k < 4; k++) {
        ar->Write(p, 4);
        p++;
    }

    n = m_0f0.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        ar->Write(m_0f0[i], 8);
    }

    n = m_0dc.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        ar->Write(m_0dc[i], 8);
    }
    return 1;
}

// ===========================================================================
// UnknownClassArrays::Method_02bfc0  @0x02bfc0
// Validate an EmitArg by kind (4 or 7); on success, dispatch through its vtable
// to emit a {x,y} pair into the bundle's m_078/m_080 scratch via slot +0x2c
// (kind 7) or +0x30 (kind 4).
// ===========================================================================
// @early-stop
// branch-layout wall (~81%): logic + both reloc-masked validate calls + the
// vtable-slot dispatch are byte-exact, but retail lays the kind==4 arm out of
// line in both if-ladders (cmp 4; je <fwd>) where MSVC5 keeps our first arm
// inline (cmp 4; jne). No steerable source spelling found (switch would add a
// jump table). Deferred to the final sweep.
RVA(0x0002bfc0, 0x8a)
i32 UnknownClassArrays::Method_02bfc0(i32 objArg, void* kindArg, i32, i32) {
    EmitArg* obj = (EmitArg*)objArg;
    i32 kind = (i32)(i32)kindArg;
    if (kind == 4) {
        if (((Kind4Validator*)this)->Validate(obj) != 0) {
            return 0;
        }
    } else if (kind == 7) {
        if (Validate_01146a(obj) != 0) {
            return 0;
        }
    }
    char* scratch = (char*)this + 0x78;
    if (kind == 4) {
        obj->Emit30(scratch, 8);
        scratch += 8;
        obj->Emit30(scratch, 8);
    } else if (kind == 7) {
        obj->Emit2c(scratch, 8);
        scratch += 8;
        obj->Emit2c(scratch, 8);
    }
    return 1;
}

// ===========================================================================
// UnknownClassArrays::Method_02c0a0  @0x02c0a0
// Mark a unit as "state 3" with a value, then count how many OTHER units in the
// current cell-row are also state 3 and record that count on the unit.
//   grid row = m_008 + m_018*0x3c, the 15-entry unit array starts at +0x1c.
// ===========================================================================
RVA(0x0002c0a0, 0x78)
i32 UnknownClassArrays::Method_02c0a0(i32 unitArg, i32 value) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_2d4 == 3) {
        return 1;
    }
    m_148 = 0;
    unit->m_2d4 = 3;
    unit->m_2e4 = value;
    void** units = (void**)m_008 + m_018 * 15 + 7;
    i32 count = 0;
    for (i32 k = 0; k < 15; k++) {
        GridUnit* p = (GridUnit*)units[k];
        if (p != 0 && unit != p && p->m_2d4 == 3) {
            count++;
        }
    }
    unit->m_2e0 = count;
    return 1;
}

// @confidence: low
// @source: winapi:IntersectRect;PtInRect
// @stub
RVA(0x0002c140, 0x3e7)
i32 UnknownClassArrays::winapi_02c140_IntersectRect_PtInRect(i32) {
    return 0;
}

// @confidence: low
// @source: winapi:IntersectRect
// @stub
RVA(0x0002dfa0, 0x325)
i32 UnknownClassArrays::winapi_02dfa0_IntersectRect(i32, i32, i32, i32) {
    return 0;
}

// @confidence: low
// @source: winapi:PtInRect
// @stub
RVA(0x0002e3a0, 0x7e1)
i32 UnknownClassArrays::winapi_02e3a0_PtInRect(i32) {
    return 0;
}

// ===========================================================================
// UnknownClassArrays::Method_02ed90  @0x02ed90
// One-arg predicate that always returns 0. (xor eax,eax; ret 4)
// ===========================================================================
RVA(0x0002ed90, 0x5)
i32 UnknownClassArrays::Method_02ed90(i32) {
    return 0;
}

// ===========================================================================
// UnknownClassArrays::Method_030530  @0x030530
// Returns 1 if ANY occupied coordinate of `unit` lands on a board tile whose
// flag byte has bit 0x4 set; else 0. Bails to 0 if the unit has no coord list.
// ===========================================================================
RVA(0x00030530, 0x56)
i32 UnknownClassArrays::Method_030530(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_328 == 0) {
        return 0;
    }
    CoordNode* node = (CoordNode*)unit->m_320;
    if (node == 0) {
        return 0;
    }
    Tile** rows = ((Board*)m_00c)->m_rows;
    while (node != 0) {
        CoordNode* cur = node;
        node = node->m_next;
        Coord* c = cur->m_coord;
        i32 y = c->m_y;
        i32 x = c->m_x;
        if (rows[y][x].m_flags & 4) {
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// UnknownClassArrays::Method_0305b0  @0x0305b0
// Scan the current cell-row for any OTHER unit that occupies coordinate
// (arg1, arg2): either via a "blocked tile" hit on the unit's occupied-coord
// list, via the unit's own packed coord (m_174/m_178 >> 5), or via its level
// geometry (m_010->m_5c/m_60 >> 5). Returns 1 on the first hit, else 0.
// ===========================================================================
// @early-stop
// regalloc wall (~46%): logic byte-exact at the head (prologue + the three
// early-out compares match). Retail spills `this` to a stack slot and keeps it
// in esi (reloading inside the inner loop), and orders the two stack locals
// (counter / cell-ptr) opposite to MSVC5's choice here; we keep `this` live in
// ebx. The divergence cascades through every register operand. No steerable
// spelling found. Deferred to the final sweep.
RVA(0x000305b0, 0x121)
i32 UnknownClassArrays::Method_0305b0(i32 selfUnit, i32 qx, i32 qy) {
    void** units = (void**)m_008 + m_018 * 15 + 7;
    for (i32 i = 0; i < 15; i++) {
        GridUnit* unit = (GridUnit*)units[i];
        if (unit == 0) {
            continue;
        }
        if (unit == (GridUnit*)selfUnit) {
            continue;
        }
        if (unit->m_2d8 == 0xb) {
            continue;
        }
        if (unit->m_328 != 0 && unit->m_320 != 0) {
            Board* board = (Board*)m_00c;
            CoordNode* node = (CoordNode*)unit->m_320;
            while (node != 0) {
                CoordNode* cur = node;
                node = node->m_next;
                Coord* c = cur->m_coord;
                i32 x = c->m_x;
                i32 y = c->m_y;
                i32 tile;
                if ((u32)x < (u32)board->m_w && (u32)y < (u32)board->m_h) {
                    tile = ((i32*)board->m_rows[y])[x * 7];
                } else {
                    tile = 1;
                }
                if ((tile & 4) && x == qx && y == qy) {
                    return 1;
                }
            }
        }
        if ((unit->m_174 >> 5) == qx && (unit->m_178 >> 5) == qy) {
            return 1;
        }
        UnitLevel* lvl = (UnitLevel*)unit->m_010;
        if ((lvl->m_5c >> 5) == qx && (lvl->m_60 >> 5) == qy) {
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// UnknownClassArrays::Method_030730  @0x030730
// Cell-claim scan: for the (cellX,cellY) source unit, walk the 15 unit slots of
// the CURRENT cell-row (m_018) and, for each candidate whose mode is 3 (or a
// 2/3-of-the-time random pick) and whose per-level record lands within distance
// 0x19 of the candidate's geometry, claim it - mark mode 3 / state 2, stamp the
// target coord (cellX,cellY) and seed m_250 = 0xd87.
// ===========================================================================
// @early-stop
// regalloc wall (~88%): logic byte-exact. Retail pins `this` in edi and SPILLS a
// copy to [esp+0x10] so it can reuse edi as scratch for this->m_004 (mov edi,
// [edi+0x4]) in the distance block, reloading it after; MSVC5 here keeps `this`
// live in one callee-saved reg + loads m_004 into a fresh one, reserving 0x8 of
// locals (no spill slot) vs retail's 0xc. Cascades through the cellX/cellY
// reg-vs-memory operand choice. No steerable spelling found; final sweep.
RVA(0x00030730, 0x1da)
i32 UnknownClassArrays::Method_030730(i32 cellX, i32 cellY, i32, i32) {
    if (m_000 == 0) {
        return 0;
    }
    if (cellX == m_018) {
        return 1;
    }
    GridUnit* src = ((GridUnit**)(m_008 + cellX * 0x3c + 0x1c))[cellY];
    if (src == 0) {
        return 0;
    }
    if (src->m_258 == 0x36) {
        return 0;
    }
    if (src->m_2d8 == 4) {
        i32 sx = src->m_2f0;
        i32 sy = src->m_2f4;
        if (sx == m_018) {
            return 0;
        }
    }
    for (i32 i = 0; i < 15; i++) {
        GridUnit* u = ((GridUnit**)(m_008 + m_018 * 0x3c + 0x1c))[i];
        if (u == 0) {
            continue;
        }
        i32 ok = 1;
        if (u->m_2d8 == 3) {
            i32 ux = u->m_2f0;
            i32 uy = u->m_2f4;
            if (ux == cellX && uy == cellY) {
                ok = 0;
            }
        }
        if (u->m_2d8 == 3) {
            i32 ux = u->m_2f0;
            i32 uy = u->m_2f4;
            if (!(ux == cellX && uy == cellY) && (rand() % 3) != 0) {
                ok = 0;
            }
        }
        if (ok == 0) {
            continue;
        }
        UnitLevel* lvl = (UnitLevel*)u->m_010;
        i32 lx = lvl->m_5c >> 5;
        i32 ly = lvl->m_60 >> 5;
        if (u->m_2d8 == 4 && u->m_2e8 != -1) {
            char* rec = (char*)m_004 + u->m_2e8 * 0x238;
            i32 dx = *(i32*)(rec + 0x258) - lx;
            i32 dy = *(i32*)(rec + 0x25c) - ly;
            dx = abs(dx);
            dy = abs(dy);
            if (dx * dx + dy * dy > 0x19) {
                ok = 0;
            }
        }
        if (ok == 0) {
            continue;
        }
        u->m_2f0 = cellX;
        u->m_2d8 = 3;
        u->m_2f4 = cellY;
        u->m_2d4 = 2;
        u->m_250 = 0xd87;
        u->m_254 = 0;
    }
    return 1;
}

// ===========================================================================
// UnknownClassArrays::Method_030f20  @0x030f20
// Pick a spawn coordinate for `unit` from the per-level record's candidate list
// (index `kind`, 0..3): start at a random candidate and walk forward (mod count)
// looking for one not already occupied by any unit in the current cell-row; on
// success write it to `out`. Out-of-range `kind` or empty list falls back to the
// unit's own (>>5) geometry. Returns `out`.
// ===========================================================================
// @early-stop
// regalloc wall (~58%): logic byte-exact (count==0 / final-rand tail-merge, the
// found-in-loop early return, the 15-slot collision scan all reproduced). Retail
// re-reads `unit` from the stack arg and spills the candidate count to a stack
// slot; MSVC5 here caches `unit` and pins count in edi, which cascades the inner
// collision loop's register operands (load-then-test vs memory-compare on
// u->m_328, cand coord regs). No steerable spelling found; final sweep.
RVA(0x00030f20, 0x16d)
void* UnknownClassArrays::Method_030f20(void* out, i32 unitArg, i32 kind) {
    Coord* o = (Coord*)out;
    GridUnit* unit = (GridUnit*)unitArg;
    if (kind < 0 || kind >= 4) {
        UnitLevel* lvl = (UnitLevel*)unit->m_010;
        o->m_x = lvl->m_5c >> 5;
        o->m_y = lvl->m_60 >> 5;
        return o;
    }
    char* rec = (char*)m_004 + kind * 0x238 + 0x278;
    UnitLevel* lvl = (UnitLevel*)unit->m_010;
    i32 rx = lvl->m_5c >> 5;
    i32 ry = lvl->m_60 >> 5;
    i32 count = *(i32*)(rec + 0x8);
    if (count != 0) {
        i32 r = rand() % count;
        i32 k = 0;
        if (count > 0) {
            Candidate** arr = *(Candidate***)(rec + 0x4);
            char* grid = m_008;
            i32 cell = m_018;
            for (;;) {
                Candidate* cand = arr[r];
                i32 cx = cand->m_x;
                i32 cy = cand->m_y;
                i32 ok = 1;
                GridUnit** row = (GridUnit**)(grid + cell * 0x3c + 0x1c);
                for (i32 j = 15; j != 0; j--) {
                    GridUnit* u = *row;
                    if (u != 0 && u->m_328 != 0) {
                        i32* node = *(i32**)((char*)u->m_324 + 0x8);
                        if (node[0] == cx && node[1] == cy) {
                            ok = 0;
                        }
                    }
                    row++;
                }
                if (ok != 0) {
                    o->m_x = cx;
                    o->m_y = cy;
                    return o;
                }
                r = (r + 1) % count;
                k++;
                if (k >= count) {
                    break;
                }
            }
        }
        r = rand() % count;
        Candidate* cand = (*(Candidate***)(rec + 0x4))[r];
        rx = cand->m_x;
        ry = cand->m_y;
    }
    o->m_x = rx;
    o->m_y = ry;
    return o;
}

// @confidence: low
// @source: winapi:IntersectRect
// @stub
RVA(0x00031ca0, 0x2f2)
i32 UnknownClassArrays::winapi_031ca0_IntersectRect(i32) {
    return 0;
}

// @confidence: low
// @source: winapi:IntersectRect
// @stub
RVA(0x00032060, 0x7bd)
i32 UnknownClassArrays::winapi_032060_IntersectRect(i32) {
    return 0;
}
