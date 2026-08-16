# The last-vptr stamp transposed with the ctor body's first member load — BROKEN 2026-08-16
tags: cpp:ctor cpp:vtable cpp:member cpp:inline | asm:mov | topic:codegen-idiom
symptoms: a derived ctor at 99.5-99.7% whose ONLY diff is one adjacent swap - retail `mov [esi],<vftable>` then `mov eax,[esi+0x38]`, ours the reverse; every other instruction identical
confidence: 10/10

In a `CUserLogic` + `CWapX` leaf constructor the most-derived vptr is stamped between the last
base-ctor store and the body's first statement:

```asm
mov  [esi+0x3c],eax          ; CWapX::m_animWorker - last base store
mov  [esi],<??_7CWayPoint>   ; retail stamps HERE
mov  eax,[esi+0x38]          ; body: m_wwdObject
```

cl 5 puts the body's load one slot earlier and the stamp one slot later.

**This is NOT a wall. The fix is in
[ctor-body-first-statement-is-an-inline-member.md](ctor-body-first-statement-is-an-inline-member.md):**
make the body's first statement an inline member of the class that OWNS the receiver
pointer (`CWapX::Hide()`, not `m_wwdObject->Hide()`), so the receiver load sits inside the
expansion and is no longer a hoist candidate. `CWayPoint` `0xae3f0`, `CGuardPoint` `0xae5f0`,
`CLevelTime` `0x9b8b0`, `CWarpStonePad` `0x10d650` and `CTileTriggerTransition` `0x10faf0` —
the five this file used to park — are all **100.000 EXACT**.

## Why the old 18-cell Cartesian missed it

Every cell kept the statement WRITTEN OUT in the ctor body (six body spellings x three
mem-init spellings): a named receiver local, an explicit read-modify-write, `this->`, a
value local, and the mem-init permutations. None of them moves the load out of the ctor's
own scheduling region, so all 18 were correctly byte-identical. The search space was
"how do I spell this statement", and the answer was "this statement is not in this
function".

Read that as the general lesson: a Cartesian over spellings of ONE statement cannot find a
lever that consists of the statement being somewhere else.
