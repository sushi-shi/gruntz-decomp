# The EH funclet that calls an out-of-line destructor names the COMDAT's OWNER TU

tags: cpp:dtor cpp:eh cpp:inline cpp:class | asm:call | topic:identity topic:eh
symptoms: a `~CFoo()` pinned out-of-line in the class's own .cpp; a *consumer*
TU that has a `CFoo` stack local emits `call ??1CFoo@@QAE@XZ` on its normal exit
path where retail expands the member teardown inline; `@identity-TODO` notes on
a whole run of 0x21xxx-band COMDATs saying only "belongs to another compiland"
confidence: 10/10

A destructor whose only out-of-line callers are **EH unwind thunks** is an
INLINE destructor, and the COMDAT belongs to the TU whose funclets call it — not
to the class's own .cpp.

`gruntz sema xref <dtor-rva>` is the whole test. If the callers are all
`jmp 0x001dXXXX FUN_005dXXXX [retail] (via 1t)` addresses that sit in one narrow
band, look up which function pushes a handler from that band
(`sema disasm <fn>` → `push 0x005dXXXX` in the prologue). That function's TU is
the owner: cl expands the destructor on the normal path and emits ONE
out-of-line copy for the unwind funclets.

```cpp
// header - the real shape
class CButeMgr {
    ~CButeMgr() {}          // inline; members do the work
};
// the consumer TU
RVA_COMPGEN(0x000213c0, 0x14c, ??1CButeMgr@@QAE@XZ)
```

The whole COMDAT *run* moves together: the inline dtor drags the member
destructors, their `??_G` scalar-deleting dtors and the `??_E` vtable thunks
with it. Verify by building — `build/gen/labels/<unit>.csv` will list every one
of them under the new unit if the attribution is right, and the labels-manifest
gate will report the matching per-unit LOST/GAINED (acknowledge with
`GRUNTZ_LABELS_ACK=1` and commit `config/labels_manifest.tsv`).

## Evidence

`??1CButeMgr@@QAE@XZ` (0x213c0) and `??1CBSecStream@@UAE@XZ` (0x21570) had their
only callers in `0x5d9322`/`0x5d9556`/`0x5d9561` — the same handler band as
`CChatBoxOwner::ProcessCheatInput`'s `push 0x5d9474`. That TU has the
`CButeMgr bute;` stack local. Moving all five pins (0x212e0 `??_GzPTree`,
0x21310 `??1zPTree`, 0x213c0 `??1CButeMgr`, 0x21570 `??1CBSecStream`, 0x21600
`??_EzPTree`) from `ButeMgr.cpp` to `ChatBoxOwner.cpp` and making `~CButeMgr()`
inline: cl emitted all five in `chatboxowner.obj`, each **100%**, and
ProcessCheatInput went **62.32 → 76.01** on that change alone. Five
`@identity-TODO`s resolved.

variants: [interleaved-comdat-methods.md](interleaved-comdat-methods.md),
[base-ctor-pinned-out-of-line-costs-every-derived-ctor.md](base-ctor-pinned-out-of-line-costs-every-derived-ctor.md)
