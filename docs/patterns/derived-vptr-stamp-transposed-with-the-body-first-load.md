# The last-vptr stamp transposed with the ctor body's first member load is a scheduler wall
tags: cpp:ctor cpp:vtable cpp:member | asm:mov | topic:wall
symptoms: a derived ctor at 99.5-99.7% whose ONLY diff is one adjacent swap - retail `mov [esi],<vftable>` then `mov eax,[esi+0x38]`, ours the reverse; every other instruction identical
confidence: 9/10

In a `CUserLogic` + `CWapX` leaf constructor the most-derived vptr is stamped between the last
base-ctor store and the body's first statement:

```asm
mov  [esi+0x3c],eax          ; CWapX::m_animWorker - last base store
mov  [esi],<??_7CWayPoint>   ; retail stamps HERE
mov  eax,[esi+0x38]          ; body: m_wwdObject
```

cl 5 puts the body's load one slot earlier and the stamp one slot later. The position of the
stamp is correct in both (it is between the bases and the body); only the two adjacent
instructions are transposed, which objdiff scores as ~0.3%.

**It is not source-steerable.** An 18-cell hand-authored Cartesian on `CWayPoint::CWayPoint`
(`0xae3f0`) - six body spellings (`m_wwdObject->m_stateFlags |= F`, `this->`-qualified, a named
receiver local, an explicit read-modify-write, a value local, a receiver local plus a value
local) crossed with three mem-init spellings - produced **byte-identical output in all 18**.
Rewriting `CWapX`'s constructor from body assignments to a member-init list is also byte-identical
(and the body-assignment form is the documented dev shape - see
`ctor-vptr-interleave-vs-spelled-out-init.md`).

Family, all with this single residue and nothing else: `CWayPoint` `0xae3f0`, `CGuardPoint`
`0xae5f0`, `CLevelTime` `0x9b8b0` (99.68 each), `CWarpStonePad` `0x10d650` (99.65),
`CTileTriggerTransition` `0x10faf0` (99.57). Mark them `@early-stop` and stop; do not re-run the
spelling chase.
