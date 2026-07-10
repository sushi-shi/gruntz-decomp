#include <rva.h>
// <Mfc.h> for the /GX EH frame helpers + CObject-style base. The grid's bucket
// list helpers, the engine allocator, and the vector ctor/dtor iterators are all
// reloc-masked engine externs (no bodies here).
#include <Mfc.h>

#include <Gruntz/WwdGrid.h>

// --- reloc-masked engine externs -------------------------------------------

// The engine bucket-array deallocator path: the MSVC vector dtor iterator
// (__ehvec_dtor, __stdcall - callee-clean, NO `add esp` after the call) over the
// element dtor, then RezFree of the backing block. (The ctor's alloc + element
// ctor fall out of `new BucketHead[n]`; see array-new-cookie-ehvec-ctor.md.)
extern "C" void RezFree(void* p);
extern "C" void __stdcall EhVecDtor_11f640(void* arr, u32 elemSize, i32 count, void* dtor);
extern "C" void DNameNodeDtor_191d10(); // the element dtor, passed by address (reloc-masked)
extern "C" double log(double);
extern "C" double pow(double, double);

// ===========================================================================
// 0x1682a0 - ~CWwdGrid: stamp own vtable, free the bucket array; the base dtor
// (folded inline) then restores the parent vtable. /GX frame from the base
// subobject teardown.
// ===========================================================================
// Real polymorphic now: cl emits the implicit ??_7CWwdGrid own-vptr stamp in the
// ENTRY state (stamp-first, == retail), then FreeBuckets, then the ~CObject
// grand-base re-stamp folds in. /GX frame from the destructible base subobject.
// (eh-dtor-implicit-vptr-stamp-first.md.)
RVA(0x001682a0, 0x46)
CWwdGrid::~CWwdGrid() {
    FreeBuckets();
}

// 0x168c10 - a SECOND, un-COMDAT-folded copy of ~CWwdGrid (byte-identical to 0x1682a0:
// stamp 0x5f0328, FreeBuckets, fold CObject) that retail emitted from a different TU
// (MSVC5 has no ICF). Co-located with CWwdGrid; kept a distinct placeholder identity
// (C168c10) because name-injectivity forbids two CWwdGrid::~CWwdGrid at two RVAs.
struct Sev168c10 {
    virtual ~Sev168c10();
};
SIZE_UNKNOWN(Sev168c10);
inline Sev168c10::~Sev168c10() {}
struct C168c10 : Sev168c10 {
    virtual ~C168c10() OVERRIDE;
};
SIZE_UNKNOWN(C168c10);
RELOC_VTBL(C168c10, 0x001f0328); // duplicate ~CWwdGrid copy; stamp reloc-masks 0x1f0328
RVA(0x00168c10, 0x46)
C168c10::~C168c10() {
    ((CWwdGrid*)this)->FreeBuckets();
}

// ===========================================================================
// Homed from src/Stub/GapFunctions.cpp (matcher-5): the gap tool had merged the
// BucketHead `??_E` (0x191720, size 0x50) with the CWwdGrid span-ctor forwarder
// (0x191770, size 0x8d) into one 0xdd-byte span - now split.
// ===========================================================================
// 0x191720 = BucketHead's `vector deleting destructor' (??_E): the COMPILER-GENERATED
// vector deleting destructor for BucketHead, emitted from the `new BucketHead[m_cellCount]`
// in the ctor above (the array-cookie alloc + ehvec ctor/dtor pair). Not a hand-written
// method: cl auto-emits ??_EBucketHead@@QAEPAXI@Z; @rva-symbol names it at this RVA so the
// delinker pairs the retail orphan (a zero-ref COMDAT; FreeBuckets inlines its own ehvec).
// @rva-symbol: ??_EBucketHead@@QAEPAXI@Z 0x00191720 0x50

// @early-stop
// 0x191770 = a __thiscall(this; x0,y0,x1,y1) helper that derives the cell sizes
// (cellW = |x1-x0|/5, cellH = |y1-y0|/5 via the 0x66666667 /5 magic-divide) and
// tail-calls CWwdGrid::CWwdGrid(x0,y0,x1,y1,cellW,cellH) @0x1915c0 on `this` (a
// re-init / 4-arg ctor overload). Body parked: invoking a ctor as a function on an
// existing object (and CWwdGrid being abstract - the pure OnFound - forbids placement
// new) is not expressible in MSVC5 C++, so the delegating call cannot be regenerated.
RVA(0x00191770, 0x8d)
i32 Gap_191770(void) {
    return 0;
}

// ===========================================================================
// 0x191800 - FreeBuckets: if allocated, run the vector dtor over the node array
// and release the backing block; then clear the alloc-OK flag.
// ===========================================================================
RVA(0x00191800, 0x39)
void CWwdGrid::FreeBuckets() {
    if (m_allocated) {
        if (m_buckets) {
            i32* raw = (i32*)m_buckets - 1; // count cookie precedes the array
            EhVecDtor_11f640(m_buckets, 0x8, *raw, (void*)DNameNodeDtor_191d10);
            RezFree(raw);
        }
        m_allocated = 0;
    }
}

// ===========================================================================
// 0x191840 - Add(region): compute the region's cell index from its pixel
// position, cache the owning bucket, link it in, bump the count.
// ===========================================================================
RVA(0x00191840, 0x48)
i32 CWwdGrid::Add(WwdRegion* r) {
    i32 col = (r->m_y - m_minY) >> m_shiftX;
    i32 row = (r->m_x - m_minX) >> m_shiftY;
    BucketHead* bucket = m_buckets + (col * m_cols + row);
    r->m_bucket = bucket;
    bucket->AddNode_1390e0(r);
    ++m_count;
    return 1;
}

// ===========================================================================
// 0x191890 - Remove(region): unlink from its cached bucket, clear it, dec count.
// ===========================================================================
RVA(0x00191890, 0x24)
void CWwdGrid::Remove(WwdRegion* r) {
    r->m_bucket->Unlink_1391e0(r);
    r->m_bucket = 0;
    --m_count;
}

// ===========================================================================
// 0x1918c0 - Query(x0,y0,x1,y1,doRemove): walk the cells overlapping the rect;
// for each region truly inside the rect, fire the virtual callback (and, when
// doRemove, unlink it first). Returns how many fired.
// ===========================================================================
// @early-stop
// 99.7% - entropy tail: body byte-identical bar a `lea ebp,[ecx*8+0]` vs
// `[8*ecx]` addressing-mode encoding + one [esp] reload-pair load-order swap
// (scheduling). No source diff closes it. (Wrapping the loop in `if(colA<=colB)`
// instead of an early `return fired` was the real fix: 95%->99.7%, killing the
// duplicate early-return epilogue - retail jg's to the shared exit.)
RVA(0x001918c0, 0x1a2)
i32 CWwdGrid::Query(i32 a0, i32 a1, i32 a2, i32 a3, i32 doRemove) {
    i32 fired = 0;
    if (a0 > m_maxX) {
        return 0;
    }
    if (a2 < m_minX) {
        return 0;
    }
    if (a1 > m_maxY) {
        return 0;
    }
    if (a3 < m_minY) {
        return 0;
    }
    if (a0 < m_minX) {
        a0 = m_minX;
    }
    if (a2 > m_maxX) {
        a2 = m_maxX;
    }
    if (a1 < m_minY) {
        a1 = m_minY;
    }
    if (a3 > m_maxY) {
        a3 = m_maxY;
    }
    i32 colA = (a1 - m_minY) >> m_shiftX;
    i32 rowA = (a0 - m_minX) >> m_shiftY;
    i32 colB = (a3 - m_minY) >> m_shiftX;
    i32 rowB = (a2 - m_minX) >> m_shiftY;
    i32 base = colA * m_cols + rowA;
    if (colA <= colB) {
        i32 colN = colB - colA + 1;
        do {
            if (rowA <= rowB) {
                i32 rowN = rowB - rowA + 1;
                i32 idx = base;
                do {
                    WwdRegion* r = m_buckets[idx].m_head;
                    while (r) {
                        i32 x = r->m_x;
                        WwdRegion* next = r->m_next;
                        if (x >= a0 && r->m_y >= a1 && x <= a2 && r->m_y <= a3) {
                            if (doRemove) {
                                m_buckets[idx].Unlink_1391e0(r);
                                r->m_bucket = 0;
                                --m_count;
                            }
                            OnFound(r);
                            ++fired;
                        }
                        r = next;
                    }
                    ++idx;
                } while (--rowN);
            }
            base += m_cols;
        } while (--colN);
    }
    return fired;
}

// ===========================================================================
// 0x191a70 - Clear: unlink and reset every bucket's list, return how many cells
// held something; zero the count.
// ===========================================================================
RVA(0x00191a70, 0x57)
i32 CWwdGrid::Clear() {
    i32 nonEmpty = 0;
    for (i32 i = 0; i < m_cellCount; ++i) {
        WwdRegion* r = m_buckets[i].m_head;
        while (r) {
            m_buckets[i].Unlink_1391e0(r);
            r->m_bucket = 0;
            ++nonEmpty;
            r = m_buckets[i].m_head;
        }
    }
    m_count = 0;
    return nonEmpty;
}

// ===========================================================================
// 0x1915c0 - ctor: normalize the rect, compute log2 cell shifts + power-of-two
// cell sizes, derive the cols/rows/cell-count, allocate the bucket array
// (count cookie + vector-constructed 8-byte heads). /GX frame from the alloc EH.
// ===========================================================================
// @early-stop
// ~74%: three stacked walls, all logic byte-faithful. (1) the log2/pow x87 path
// (fldln2/fld 2.0/fyl2x/fdiv/__ftol/__CIpow) has a non-steerable FP-stack
// schedule (docs/patterns/x87-fp-stack-schedule.md). (2) the real polymorphic
// CObject subobject shifts the /GX __ehfuncinfo state-id base
// (docs/patterns/eh-state-numbering-base.md) - the trade that took the dtor to
// 100% (real-virtual implicit stamp-first). (3) the 4 rect-field stores fuse to a
// `lea edx,[esi+0x28]` pointer-block in retail but stay direct member stores here
// (regalloc choice). The implicit ctor vptr-stamp is now compiler-emitted (the
// abstract base's __purecall vtable), matching retail's polymorphic construction.
RVA(0x001915c0, 0x15d)
CWwdGrid::CWwdGrid(i32 x0, i32 y0, i32 x1, i32 y1, i32 cellW, i32 cellH) {
    m_count = 0;
    m_minX = x0;
    m_minY = y0;
    m_maxX = x1;
    m_maxY = y1;
    i32 lox = x0, hix = x1;
    if (x1 < x0) {
        lox = x1;
        hix = x0;
    }
    i32 loy = y0, hiy = y1;
    if (y1 < y0) {
        loy = y1;
        hiy = y0;
    }
    m_width = hix - lox;
    m_height = hiy - loy;
    m_shiftY = (i32)(log((double)cellW) / log(2.0));
    m_shiftX = (i32)(log((double)cellH) / log(2.0));
    m_cellH = (i32)pow(2.0, (double)m_shiftY);
    m_cellW = (i32)pow(2.0, (double)m_shiftX);
    m_cols = m_width / m_cellH + 1;
    m_rows = m_height / m_cellW + 1;
    m_cellCount = m_rows * m_cols;
    BucketHead* arr = new BucketHead[m_cellCount];
    m_buckets = arr;
    if (arr) {
        m_allocated = 1;
    }
}
