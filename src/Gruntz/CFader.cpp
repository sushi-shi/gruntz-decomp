// CFader.cpp - the CFaderMgr element base class + one concrete subtype's
// destructor (tracer placeholder MallocCtor_17fdb0, keyed off the 0x17fdb0 subtype
// ctor). CFader is the polymorphic base of the six screen-fader subtypes the
// CFaderMgr::Add factory allocates; each fader owns a CShadeTableCache color-table
// cache at +0x04 plus the manager-primed timing fields, and a GetTickCount
// busy-wait helper.
//
// Methods in ascending retail-RVA order. Field names are placeholders; offsets +
// code bytes are load-bearing. The CShadeTableCache ctor/dtor/FindRemove (0x14de30
// / 0x14de50 / 0x14fb80) and operator new/delete are external/reloc-masked; the
// fader subtype vftables are stamped as reloc-masked DIR32 data.
#include <Gruntz/CFader.h>

#include <rva.h>
#include <Win32.h> // GetTickCount / DWORD (pure-Win32 fader; reloc-masked import)

// ===========================================================================
// 0x17e450 - CFader::CFader(): build the cache subobject (0x14de30), stamp the
// fader vftable, zero m_table, and arm the teardown flag.
// ===========================================================================
RVA(0x0017e450, 0x23)
CFader::CFader() {
    m_vptr = &g_faderVtbl;
    m_table = 0;
    m_flag = 1;
}

// ===========================================================================
// 0x17e4a0 - CFader::~CFader(): restore the fader vftable, FindRemove the cached
// table from the cache when armed, then destruct the cache subobject (0x14de50).
// The destructible m_cache member forces the /GX EH frame.
// ===========================================================================
// @early-stop
// /GX EH-state wall (docs/patterns/eh-dtor-vptr-stamp-vs-trylevel-order.md +
// eh-state-numbering-base.md): body byte-identical; residue is (1) the vptr stamp
// scheduled after the m_table load vs retail's stamp-first, and (2) the
// __ehfuncinfo state id (push 0xb vs 0x0) - a fresh single-EH-fn TU can't
// reproduce retail's EH-state index. Not source-steerable. ~88%.
RVA(0x0017e4a0, 0x69)
CFader::~CFader() {
    m_vptr = &g_faderVtbl;
    if (m_table && m_flag) {
        m_cache.FindRemove(m_table);
        m_table = 0;
    }
}

// ===========================================================================
// 0x17e510 - CFader::Wait(delay): spin on GetTickCount() until at least `delay`
// ms have elapsed since the call. __thiscall (ecx=this, unused by the body).
// ===========================================================================
RVA(0x0017e510, 0x23)
void CFader::Wait(i32 delay) {
    DWORD target = GetTickCount() + delay;
    while (GetTickCount() < target) {
    }
}

// ===========================================================================
// 0x17e760 - CFader::SetTimers(a, b): store the manager's shared timing pair.
// ===========================================================================
RVA(0x0017e760, 0x11)
void CFader::SetTimers(i32 a, i32 b) {
    m_24 = a;
    m_28 = b;
}

// ===========================================================================
// 0x17e780 - CFader::Set2c(v): store the manager's shared Set2c value.
// ===========================================================================
RVA(0x0017e780, 0xa)
void CFader::Set2c(i32 v) {
    m_2c = v;
}

// ===========================================================================
// CFaderSine - the case-"3" / jump-index-2 fader subtype (CFaderMgr::Add allocates
// 0x7d5c bytes for it). Its motion virtual (0x17ff30) drives a sine-curve scroll
// over a large embedded table; only its destructor is reconstructed here (the
// ctor/motion virtuals live in sibling TUs). The subtype vftable (0x5f0848) is
// stamped manually as reloc-masked DIR32 data - a transitional workaround while
// the subtype's vtable contents are unmodeled (declaring virtuals here would emit
// a divergent ??_7). Name is descriptive (no RTTI / naming string survives).
// ===========================================================================
DATA(0x001f0848)
extern void* g_faderSineVtbl; // 0x5f0848 - the CFaderSine vftable

class CFaderSine : public CFader {
public:
    ~CFaderSine(); // 0x17fdf0
};

// ===========================================================================
// 0x17fdf0 - ~CFaderSine(): restore the subtype vftable, then tail-call ~CFader().
// ===========================================================================
RVA(0x0017fdf0, 0xb)
CFaderSine::~CFaderSine() {
    m_vptr = &g_faderSineVtbl;
}
