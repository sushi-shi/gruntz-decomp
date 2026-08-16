# A `sub/neg/sbb/not/and` mask means the second test was NOT short-circuited - split the `&&`

tags: cpp:if cpp:ternary cpp:operator | asm:sbb asm:neg asm:not asm:and | topic:codegen-idiom
symptoms: your branch COUNT is one higher than retail's, and where you emit a `cmp`/`jcc`
retail has a branchless `sub eax,K / neg eax / sbb eax,eax / not eax / and eax,reg`
confidence: 9/10

`sbb eax,eax` after a `neg` materialises 0 or -1 from the flags, and `not`+`and` turns that
into "the value, or 0". So

```asm
  call  DWORD PTR [eax+0x20]   ; GetClassId()
  mov   ecx,DWORD PTR [esp+0x18]
  sub   eax,0x5
  neg   eax
  sbb   eax,eax
  not   eax
  and   eax,ecx
```

is `(GetClassId() == 5) ? p : 0` compiled **with no branch**. cl 5.0 only reaches for that
form when the test is a value-producing expression it can flatten - a bare ternary. Put the
same test inside an `&&` and cl must short-circuit it, so it becomes a second `cmp`/`jcc`
and the branch count goes up by one:

```cpp
// TWO branches - cl must short-circuit
void* obj = (looked != NULL && static_cast<CGameObject*>(looked)->GetClassId() == CLASSID_SERIALREF)
                ? looked : 0;

// ONE branch + retail's mask
void* obj;
if (looked == NULL) {
    obj = 0;
} else {
    obj = (static_cast<CGameObject*>(looked)->GetClassId() == CLASSID_SERIALREF) ? looked : 0;
}
```

`CTriggerMgr::Load` 0x7abc0 **87.32 -> 89.50**, and its branch count went 36 -> retail's 35.

Two details that both mattered:

* The **outer** null test stays a branch in retail too, so do not try to make the whole
  thing branchless - only the inner one flattens.
* Spelling the null arm as the `if` side (`if (looked == NULL) obj = 0; else ...`) rather
  than as an initialiser (`void* obj = 0; if (looked != NULL) ...`) is what gives it its own
  block; the initialiser form hoists the `xor eax,eax` above the compare and flips the
  branch polarity.

**Where to look for it:** `gruntz walls diagnose <rva>` reporting
"BRANCH COUNTS DIFFER" with us one HIGHER, plus a `sbb` count mismatch between the two
sides. A one-line sieve over a unit's functions - compare `grep -c sbb` on `--target` vs
`--base` - finds them; across the 29 sub-100 functions of the trigger cluster this was the
only site, so it is precise rather than noisy.

## Two comparisons combined as values use bitwise `&`, not short-circuit `&&`

A second form has no selected pointer and may have no literal `and` instruction. Retail
can materialize each comparison as 0/1 and combine them only for flags:

```asm
  xor   ecx,ecx
  cmp   esi,edx
  sete  cl
  cmp   edx,eax
  sbb   eax,eax
  neg   eax
  test  eax,ecx
  je    done
```

That is eager value evaluation:

```cpp
if ((m_poweredUp == 0) & (static_cast<u32>(m_dwell) > 1000)) {
    // ...
}
```

The behavior agrees with `&&` when both operands are side-effect-free, but the compiler
IR does not: `&&` emits two short-circuit branches, while `&` emits both boolean values
and one final branch. In `CGrunt::StepDiggerBehavior` at `0x000f36a0`, this spelling
removed the candidate-only branch, made the block/return counts agree at 85/5, and raised
current-source MAX from 62.6290% to 63.3527%. The two remaining branch-count differences
are separately identified redundant guards in the low-stamina arm.
