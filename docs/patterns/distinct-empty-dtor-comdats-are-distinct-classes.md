# N distinct empty `ret` destructor COMDATs mean N distinct CLASSES

tags: cpp:dtor cpp:eh cpp:class cpp:inline | asm:ret asm:jmp | topic:identity topic:correctness
symptoms: several members that our headers give ONE type, whose unwind funclets
reach DIFFERENT 1-byte `c3` addresses; `eh_band --census` row `ours ??1Foo@@QAE@XZ
-> -` repeated over several groups; a retail address that is a lone `c3` fenced by
`cc` linker fill on both sides
confidence: 10/10

cl emits **one destructor COMDAT per type**, and the retail link has no
`/OPT:ICF`, so two identical `ret`-only destructors are never folded. Therefore
each distinct empty-destructor ADDRESS the unwind funclets reach is a distinct
CLASS. When one modelled type serves several members and their funclets land on
different `c3`s, the model has CONFLATED several classes into one — split it.

```cpp
// The shared, ops-carrying head (ONE copy of every operation in the image).
struct DSoundList { DSoundLink* m_head; DSoundLink* m_tail; /* Insert*/Unlink */ };

// One trivial typed wrapper per element type - each gets its own `ret` COMDAT.
struct DSoundBufferList : public DSoundList { ~DSoundBufferList() {} };
struct DSoundVoiceList  : public DSoundList { ~DSoundVoiceList()  {} };
struct DSoundCloneList  : public DSoundList { ~DSoundCloneList()  {} };
```
```asm
; ??0SoundDevice ctor: two 8-byte zero-init members, two EH states
mov  DWORD PTR [esi+0x4],edi     ; state 0 -> funclet jmps 0x1364e0
mov  DWORD PTR [esi+0x8],edi
mov  DWORD PTR [esi+0xc],edi     ; state 1 -> funclet jmps 0x1364f0
mov  DWORD PTR [esi+0x10],edi
; 0x1364e0: c3      <- DIFFERENT addresses, so DIFFERENT types
; 0x1364f0: c3
```

## Reading the retail side

`eh_band --census` prints `retail -` when the funclet's target carries no name in
the Model, and `FUN_<va>` when it goes through an ILT thunk. Both mean
**an unpinned COMDAT**, not a missing function. Resolve the `e9` chain by hand and
read the body:

| body at the end of the chain | what it is | pin |
|---|---|---|
| `c3` | `~X() {}` written inline in a header | `RVA_COMPGEN(<rva>, 0x1, ??1X@@QAE@XZ)` |
| `e9 <rel32>` to a real dtor/`Close` | an inline `~X() { OneCall(); }` | `RVA_COMPGEN(<rva>, 0x5, ??1X@@QAE@XZ)` |

`0xcc` fill on BOTH sides of the body proves it is its own linker contribution,
so it belongs to no neighbour's band: the pin only has to live in a `.cpp` whose
base obj actually defines that symbol (`llvm-objdump -t` the base obj, look for a
non-`sec 0` row), placed in the file's ascending-RVA order. It is emitted in every
TU whose funclets take its address, so several `.cpp`s are legal hosts; pick the
one nearest the retail address.

## Evidence

`DSoundList` was modelling THREE retail classes at once. `??0/??1SoundDevice`'s
funclets destroy `this+0x4` through 0x1364e0 and `this+0xc` through 0x1364f0;
`??0/??1DSoundCloneInst`'s destroy `this+0x58` through 0x135ba0. Three separate
1-byte COMDATs, so three classes — while `InsertHead` 0x1390e0 / `Unlink` 0x1391e0
exist in exactly ONE copy each, shared by DirectSoundMgr, SoundDevice,
SoundStream, CSymParser, CHashBase and CWwdGrid, which puts the operations on a
common base and the destructor on the typed wrapper. Splitting the type and
pinning the three dtors took all four `directsoundmgr` groups to
`unwind-identical`.

Same shape, same wave: `??1RegistryHelper@Utils@@QAE@XZ` at 0x201f0 (an
`e9` to `?Close@RegistryHelper@Utils@@QAEXXZ`, 3 groups),
`??1CMotionState@@QAE@XZ` at 0x58ba0 (`c3`, 2 groups) and
`??1PlayerLatency@@QAE@XZ` at 0x832e0 (`c3`, 2 groups) were all merely unpinned;
one `RVA_COMPGEN` line each closed them. Still open under this rule:
`CHash` (`include/Bute/Hash.h`) is ONE modelled type where retail has TWO, at
0x139dd0 and 0x139ed0 — both 5-byte `jmp 0x184a40`.

variants: [eh-funclet-names-the-type.md](eh-funclet-names-the-type.md),
[eh-funclet-band-owns-the-inline-dtor-comdat.md](eh-funclet-band-owns-the-inline-dtor-comdat.md)
