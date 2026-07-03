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
#include <Gruntz/CFaderSubtypes.h> // the six concrete subtypes (declarations)

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
    m_timerA = a;
    m_timerB = b;
}

// ===========================================================================
// 0x17e780 - CFader::Set2c(v): store the manager's shared Set2c value.
// ===========================================================================
RVA(0x0017e780, 0xa)
void CFader::Set2c(i32 v) {
    m_set2cArg = v;
}

// ===========================================================================
// CFader17e940 - a CFader subtype (ctor 0x17e940, size 0x6c) that embeds a nested
// polymorphic sub-object at +0x58 (its own vftable 0x5f07d8 + four zeroed fields).
// ATYPICAL vptr order: the member sub-object is constructed (its vptr + fields)
// BEFORE the subtype's own primary vftable 0x5f07c0 is stamped -- MSVC5's ctor
// order is base ctors, MEMBER ctors, own vptr, body (cf. CFader::CFader building
// m_cache before its own stamp). cl inlines the member ctor, so the member vptr +
// field zeros fall between the CFader base ctor call and the sunk own-vptr stamp.
// (Class declaration in <Gruntz/CFaderSubtypes.h>.)
// ===========================================================================
RVA(0x0017e940, 0x27)
CFader17e940::CFader17e940() {}

// ===========================================================================
// CFaderSine - the case-"3" / jump-index-2 fader subtype (CFaderMgr::Add allocates
// 0x7d5c bytes for it). Its motion virtuals (0x17ff30 / 0x180400) override the two
// CFader pure virtuals (slots 1/2); slots 3/4 are inherited. Real polymorphic now:
// the empty ~CFaderSine stamps ??_7CFaderSine then tail-calls ~CFader, and cl emits
// ??_7CFaderSine (slots reloc-mask the 0x5f0848 target). Name is descriptive.
// ===========================================================================
// (Class declaration in <Gruntz/CFaderSubtypes.h>.)
// ===========================================================================
// 0x17fdb0 - CFaderSine(): chain CFader::CFader, stamp ??_7CFaderSine, zero the
// two subtype fields. cl auto-emits the base ctor call + the implicit vptr stamp
// (reloc-masked vs 0x5f0848); the subtype virtuals are declared (not defined
// here) only to make the class concrete + size its vtable.
// ===========================================================================
RVA(0x0017fdb0, 0x1a)
CFaderSine::CFaderSine() {
    m_50 = 0;
    m_4c = 0;
}

// ===========================================================================
// 0x17fdf0 - ~CFaderSine(): stamp the subtype vftable, then tail-call ~CFader().
// ===========================================================================
RVA(0x0017fdf0, 0xb)
CFaderSine::~CFaderSine() {}

// ===========================================================================
// CFaderFlat - the fader subtype whose ctor (0x17f530) only clears m_4c. Its
// vftable is 0x5f07f8. Same modeling as CFaderSine.
// ===========================================================================
// (Class declaration in <Gruntz/CFaderSubtypes.h>.)
// ===========================================================================
RVA(0x0017f530, 0x19)
CFaderFlat::CFaderFlat() {
    m_4c = 0;
}

// ===========================================================================
// CFader180410 - subtype ctor 0x180410: clears m_40. vftable 0x5f0870.
// ===========================================================================
// (Class declaration in <Gruntz/CFaderSubtypes.h>; size 0x206c pinned from the
// CFaderMgr::Add new(0x206c) allocation.)
// ===========================================================================
RVA(0x00180410, 0x19)
CFader180410::CFader180410() {
    m_40 = 0;
}

// ===========================================================================
// CFader17f9a0 - subtype ctor 0x17f9a0: m_44/m_40/m_50 = 0, m_48 = 1. vftable
// 0x5f0810.
// ===========================================================================
// (Class declaration in <Gruntz/CFaderSubtypes.h>; size 0x5c pinned from the
// CFaderMgr::Add new(0x5c) allocation.)
// ===========================================================================
RVA(0x0017f9a0, 0x24)
CFader17f9a0::CFader17f9a0() {
    m_44 = 0;
    m_40 = 0;
    m_50 = 0;
    m_48 = 1;
}

// ===========================================================================
// CFader1816c0 - subtype ctor 0x1816c0 (size 0x494): zeroes m_478/m_44/m_48/m_4c/
// m_488/m_48c and the CFader base field m_20. vftable 0x5f0890.
// ===========================================================================
// (Class declaration in <Gruntz/CFaderSubtypes.h>.)
// ===========================================================================
RVA(0x001816c0, 0x32)
CFader1816c0::CFader1816c0() {
    m_478 = 0;
    m_44 = 0;
    m_48 = 0;
    m_4c = 0;
    m_488 = 0;
    m_48c = 0;
    m_20 = 0;
}
