# `call ds:[??_7Class+N]` in a ctor/dtor: the virtual call arrived by INLINING, not from the body

tags: cpp:dtor cpp:ctor cpp:virtual cpp:inline | asm:call | topic:codegen-idiom topic:eh
symptoms: retail has `ff 15 <abs>` = `call DWORD PTR ds:0x<vtable+N>` right after the ctor/dtor's own `mov [this],??_7Class`, ours has `e8 <rel32>` = a direct `call ?Virt@Class@@UAEXXZ`; body otherwise byte-identical, function one byte short, ~97%
confidence: 9/10

cl5 resolves a virtual call written **directly in a ctor/dtor body** statically —
the dynamic type is pinned, so `Reset();` becomes a plain `call ?Reset@…`. It does
**not** re-apply that when the call arrives through INLINING: an in-class member
whose body calls a virtual on its own `this` keeps the virtual dispatch, and the
optimizer then constant-propagates the vptr the ctor/dtor just stored, collapsing
`mov eax,[this]; call [eax+N]` into a single absolute-indirect `call ds:[??_7…+N]`.

So an absolute-indirect call through the class's OWN vtable inside its own
ctor/dtor is a **fingerprint**: the destructor body did not name that virtual — it
called an inline member that did.

```cpp
// ours - `Reset()` in the dtor body is devirtualized: e8 <rel32>
class CFileMemBase {
    virtual ~CFileMemBase() { Reset(); }
    virtual void Close();               // out-of-line
    virtual void Reset();               // vtable slot 3 (+0xc)
};

// retail - the dtor calls the INLINE Close(), whose Reset() stays virtual:
class CFileMemBase {                    // -> ff 15 [??_7CFileMemBase@@6B@+0xc]
    virtual ~CFileMemBase() { Close(); }
    virtual void Close() { Reset(); }   // in-class => inlined into the dtor
    virtual void Reset();
};
```

Read the slot index straight off the operand: `ds:0x5efe74 - ??_7CFileMemBase@@6B@
(0x5efe68) = 0xc` = slot 3, and `vtable_hierarchy --class` names slot 3. That
names the virtual the inline member called, which in turn tells you what the
one-line inline member must be.

STEERABLE. `??1CFileMemBase` 0x1578b0 97.0 -> **100 EXACT** and `??1CFileMem`
0x157980 97.8 -> **100 EXACT** (both dtors call `Close()`; `Close()` becomes an
in-class `{ Reset(); }` for base and derived). `?Close@…` keeps a standalone
COMDAT (it is in the vtable) — pin it with `RVA_COMPGEN`.

**Cost to expect:** making a virtual inline MOVES its COMDAT. An out-of-line
virtual is emitted by the TU that defines it; an inline one is emitted by whatever
TU emits the class vtable (i.e. the TU with a ctor/dtor). `?Close@CFileMem@@UAEXXZ`
moved ddrawsubmgr -> ddrawsurfacemgr, so the `RVA_COMPGEN` pin moves with it and
`config/labels_manifest.tsv` needs the one-line transfer (`GRUNTZ_LABELS_ACK=<unit>`,
committed). If retail's placement for that COMDAT sits inside the OLD unit's span,
that is a TU-completeness finding (retail's compiland also instantiated the class),
not a refutation of the inline model.

related: [inline-base-dtor-folds-into-leaves.md](inline-base-dtor-folds-into-leaves.md),
[derived-dtor-inlines-only-in-class-base-dtors.md](derived-dtor-inlines-only-in-class-base-dtors.md)
(/O2 implies /Ob1: only in-class bodies inline, so this fingerprint is always an
in-class member).
