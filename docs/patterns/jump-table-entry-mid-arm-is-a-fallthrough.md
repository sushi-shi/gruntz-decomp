# A jump-table entry pointing MID-arm is a source `case` fall-through
tags: cpp:switch cpp:branch | asm:jmp asm:add | topic:codegen-idiom
symptoms: the recompile emits one arm per `case` but retail's switch body has FEWER arms than the source has cases; the missing cases' jump-table entries are addresses INSIDE another arm rather than that arm's first instruction; every `[esp+N]` after the switch shifts and the function sits 55-70%
confidence: 9/10

## Symptom

`gruntz sema disasm <rva>` shows a dense jump table (`dec eax / cmp eax,N / ja
<default> / jmp [eax*4+<tbl>]`) whose arms are visibly short of one per case:

```
   52c98:  add ebx,0x20      ; NE
   52c9b:  add edi,-0x20
   52c9e:  jmp <join>
   ...
   52cba:  add ebx,-0x20     ; NW
   52cbd:  add edi,-0x20
   52cc0:  jmp <join>
```

Eight `case`s in the source, six arms in retail. Reading the table settles it —
**dump the entries, do not eyeball the arms**:

```
idx 0 (DIR_NORTH)     -> 0x452cbd   <- the SECOND instruction of the NW arm
idx 5 (DIR_SOUTH)     -> 0x452cb0   <- the SECOND instruction of the SW arm
```

An entry that lands on an address which is not an arm's first instruction is not
a compiler tail-merge you have to reproduce indirectly: it is the direct image of
a **source fall-through**, `case NW: x -= 0x20; /* fall through */ case N: y -= 0x20; break;`.

## Reading the table

```python
# rva of the table comes straight out of the `jmp DWORD PTR [eax*4+0x452e24]`
tbl = read(0x452e24, 4 * n)
for i in range(n):
    print(i, hex(struct.unpack_from('<I', tbl, i * 4)[0]))
```

Then pair each entry with the arm that CONTAINS it. Every entry that is not an
arm head names the pair `(containing case, this case)` and the order inside the
arm names which one comes first in the source.

## Why it is worth doing first

The arms are cheap to re-spell and the switch is usually at the top of the
function, so getting it wrong shifts the whole body. `CGrunt::ClaimSwitchTile`
0x52c70 had eight independent arms where retail has six; restoring the two
fall-throughs took it 67.95 -> 60.16 on the *current* score but moved the frame
from `sub esp,8` to `sub esp,0x10` (retail's) and the arm layout to retail's
exactly - the instruction count went 118 -> 126 against retail's 144. Judge this
one by the arm layout and the frame, not the percent.

## Related

- [`empty-switch-arms-fold-into-default-and-kill-the-jump-table`](empty-switch-arms-fold-into-default-and-kill-the-jump-table.md)
  - the other reason a table has the wrong number of live entries.
- [`instruction-count-mismatch-finds-the-real-bug`](instruction-count-mismatch-finds-the-real-bug.md)
  - the count screen that sends you here.
