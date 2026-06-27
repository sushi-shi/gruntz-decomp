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
    m_table = 0;
    m_flag = 1;
}

// ===========================================================================
// 0x17e4a0 - CFader::~CFader(): restore the fader vftable, FindRemove the cached
// table from the cache when armed, then destruct the cache subobject (0x14de50).
// The destructible m_cache member forces the /GX EH frame.
// ===========================================================================
// Real polymorphic now: cl emits the implicit ??_7CFader vptr re-stamp in the
// ENTRY state (stamp-first, == retail), and the destructible m_cache member folds
// in to supply the /GX frame. (eh-dtor-implicit-vptr-stamp-first.md sub-case 1.)
RVA(0x0017e4a0, 0x69)
CFader::~CFader() {
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
// 0x7d5c bytes for it). Its motion virtuals (0x17ff30 / 0x180400) override the two
// CFader pure virtuals (slots 1/2); slots 3/4 are inherited. Real polymorphic now:
// the empty ~CFaderSine stamps ??_7CFaderSine then tail-calls ~CFader, and cl emits
// ??_7CFaderSine (slots reloc-mask the 0x5f0848 target). Name is descriptive.
// ===========================================================================
class CFaderSine : public CFader {
public:
    virtual ~CFaderSine(); // 0x17fdf0
    virtual void v1();     // slot 1 -> 0x17ff30 (overrides CFader pure)
    virtual void v2();     // slot 2 -> 0x180400 (overrides CFader pure)
};

// ===========================================================================
// 0x17fdf0 - ~CFaderSine(): stamp the subtype vftable, then tail-call ~CFader().
// ===========================================================================
RVA(0x0017fdf0, 0xb)
CFaderSine::~CFaderSine() {}
