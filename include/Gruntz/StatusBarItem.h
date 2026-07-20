#ifndef STATUSBARITEM_H
#define STATUSBARITEM_H

#include <Gruntz/SerialArchive.h> // CSerialArchive (== the real CFileMemBase) - the slot-1 arg
#include <Ints.h>
#include <rva.h>

class CStatusBarMgr;      // the owning status-bar manager (Setup arg1 / m_2c)
class CDDrawSurfaceMgr;   // the config host (Setup arg2 / m_24)

struct SbiRect {
    i32 m_0; // +0x00 (rel +0x14)
    i32 m_4; // +0x04 (rel +0x18)
    i32 m_8; // +0x08 (rel +0x1c)
    i32 m_c; // +0x0c (rel +0x20)
    SbiRect() {}
    SbiRect(i32 l, i32 t, i32 r, i32 b) : m_0(l), m_4(t), m_8(r), m_c(b) {}
};
SIZE_UNKNOWN(SbiRect);

class CStatusBarItem {
public:
    // The ctor is INLINE in every builder/ctor-side TU (so the derived CSBI_RectOnly
    // folds it). The standalone complete-object ctor COMDAT (0x1005d0) is labeled by
    // src/Gruntz/StatusBarItem.cpp, which #defines SBI_ITEM_OWN_CTOR to take the
    // out-of-line declaration + supply the RVA-keyed body (mirrors SbiDtorChain.h's
    // SBI_OWN_*_DTOR device); NOT a second class.
#ifdef SBI_ITEM_OWN_CTOR
    CStatusBarItem();
#else
    CStatusBarItem() {
        m_enabled = 0;
        m_kind = 0;
        m_24 = 0;
        m_28 = 0;
    }
#endif
    virtual ~CStatusBarItem();
    // vtable slot 1 (thunk 0x1848 -> 0x10bfc0): the family serialize. Transfers the six
    // base-region fields (m_4..m_rect14, then +0x28) through the archive; `kind` selects
    // the leg (7 = read, 4 = write - the arms every override switches on).
    //
    // THE SIGNATURE IS 4-ARG, PROVEN BY THE BYTES. Every slot-1 body in this hierarchy
    // ends `ret 0x10` (0x10bfc0, 0xe6e40, 0xe74f0, 0xe8520, 0xe8e00, 0xe9a30, 0xea990,
    // ...) = 4 stack args + __thiscall. The old `virtual i32 SbiVfunc0()` that stood here
    // was a FABRICATED 0-arg placeholder: it emitted a bare `ret`, and because the two
    // real bodies wired onto it (CSBI_GruntMachine 0xe8e00 / CSBI_SideTab 0xe9a30) were
    // virtual, vtable_slot_binding could not see the defect - it only checks virtuality,
    // never the signature. Wiring a body to the right slot under the wrong signature is
    // still wrong; the gate went green while the bytes stayed broken.
    //
    // Each override tail-chains its base leg with a QUALIFIED call (retail: `call 0x1848`
    // + `neg/sbb/neg` = normalise to 0/1), which is itself the proof that those bodies'
    // `this` is a CStatusBarItem: an unqualified call would be recursion.
    virtual i32 SerializeFields(CSerialArchive* ar, i32 kind, i32 a, i32 b); // 0x10bfc0
    // vtable slot 2 (0x100660): the base 10-arg setup - bails (returns 0) if the object
    // id (a2) or owner (a1) is null, else stores the eight live args into the base-region
    // fields (the last two args are ABI-accepted but unused). CSBI_RectOnly overrides it
    // (0xe86e0) to additionally mark m_4 = 1 (active).
    // Args 5..8 are ONE by-value SbRect, and the CALLER proves it: BuildStatusBarTabs
    // (0xffde0) materializes them with `sub esp,0x10; mov ecx,esp; mov [ecx],<value>;
    // mov [ecx+4],..; mov [ecx+8],..; mov [ecx+0xc],..` - the struct is built DIRECTLY in
    // the outgoing arg frame from freshly-computed values, never from a named local.
    // Callee-side the two spellings are ABI-identical (10 dwords, same `ret`), so only the
    // call site can tell them apart. The sibling builder view <Gruntz/SbiTabzDialogViews.h>
    // already had it right - its slot-11 Setup takes `SbiRect rc` by value and its call
    // sites pass an INLINE TEMPORARY, `SbRect(cx - 0x5e, cy - 0x3c, ...)`. That temporary
    // is the whole trick: a named local makes cl materialize the struct and copy it; an
    // inline temporary makes it build the struct in place, which is what retail does.
    virtual i32 Setup(CStatusBarMgr* owner, CDDrawSurfaceMgr* host, i32 a3, i32 a4, SbiRect rc, i32 a9, i32 a10); // slot 2
    virtual void SbiSlot3();                                                        // slot 3
    virtual void SbiSlot4();                                                        // slot 4
    virtual void SbiSlot5();                                                        // slot 5
    // slots 6..9 (0x100530/0x100550/0x100570/0x100590): base defaults - each is
    // `xor eax,eax; ret 0xc` => i32-return, 3 stack args, `return 0`. No SBI leaf
    // overrides them. Out-of-line default bodies in SBI_RectOnly.cpp.
    virtual i32 SbiSlot6(i32, i32, i32); // slot 6  0x100530
    virtual i32 Click1c(i32 a, i32 b, i32 c); // slot 7 (+0x1c)  0x100550  click handler A
    virtual i32 SbiSlot8(i32, i32, i32); // slot 8  0x100570
    virtual i32 Click24(i32 a, i32 b, i32 c); // slot 9 (+0x24)  0x100590  click handler B
    virtual void SetSubtype();           // slot 10

    // Semantic names recovered from the ex-CSbiRect reader view (the hit-test code
    // reads these same slots as enabled/kind/cmd/tab + the x/y span rect).
    i32 m_enabled; // +0x04  enabled gate (Setup seeds 0/1)
    i32 m_kind;    // +0x08  widget kind tag
    i32 m_cmd;     // +0x0c  Setup arg3: command id
    i32 m_tab;     // +0x10  Setup arg4: owning tab index
    // +0x14..0x20: a 4-int sub-block (a RECT-like record) that Setup fills through
    // a single base pointer (lea &m_14; [+0]/[+4]/[+8]/[+c]).
    SbiRect m_rect14; // +0x14  Setup args 5..8
    class CDDrawSurfaceMgr* m_24; // +0x24  Setup arg2: the config host (surface mgr)
    i32 m_28;         // +0x28
    class CStatusBarMgr* m_2c;    // +0x2c  Setup arg1: the owning status-bar mgr

    // Member teardown run by the inline destructor of the CHAIN-DTOR device below
    // (reloc-masked extern; the retail standalone body is 0x10bfa0).
    void DtorStatus(); // 0x10bfa0
};
SIZE_UNKNOWN(CStatusBarItem);

#if defined(SBI_DTOR_CHAIN) && !defined(SBI_ITEM_OWN_DTOR)
inline CStatusBarItem::~CStatusBarItem() {
    DtorStatus();
}
#endif

#endif // STATUSBARITEM_H
