# A `?:` with a null arm lays its blocks in CONDITION polarity — an `if/else` does not

tags: cpp:ternary cpp:branch | asm:jcc asm:xor asm:jmp | topic:codegen-idiom
symptoms: the two arms of a null-guarded ternary are emitted in the opposite order from
  retail — base `je <zero>; <call arm>; jmp <join>; xor eax,eax` vs retail
  `jne <call arm>; xor eax,eax; jmp <join>; <call arm>` — with identical instructions on
  both sides; inverting the equivalent `if/else` changes nothing
confidence: 9/10

For a conditional expression, cl5 emits the FALSE arm first when the condition is written
so the false arm is the fall-through, i.e. the polarity you write is the polarity you get:

```cpp
POSITION pos = head != NULL ? head->GetHeadPosition() : NULL;
//   je <zero>;  mov eax,[eax+4];  jmp <join>;  xor eax,eax

POSITION pos = head == NULL ? NULL : head->GetHeadPosition();
//   jne <call>; xor eax,eax;      jmp <join>;  mov eax,[eax+4]     <- retail
```

`CPlay::LoadWarlordSprites` @0xd65d0: the list walk is not `if (!head) return 0;` at all —
retail yields a null POSITION and falls through the loop. Writing it as the ternary got the
instruction set right (95.16 → 94.60, worse) and the `== NULL ? NULL :` polarity got the
block order right (→ 95.26).

**The knob is specific to `?:`.** The same choice written as `if/else` is canonicalised:
on `CStatusBarMgr::Deserialize` @0x109520, both

```cpp
if (found && obj != NULL) { m8 = …; } else { m8 = 0; }
if (!found || obj == NULL) { m8 = 0; } else { m8 = …; }
```

emit the compute arm first, and retail emits the zero arm first — so an `if/else` residue
of this shape is a different (still-open) mechanism, not this one.

related: negated-condition-far-block.md, map-lookup-ternary-ifconverts.md,
minmax-clamp-if-widen-not-ternary.md
