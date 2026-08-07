# Byte reads addressed at `[esp+param]` are a VIEW of the parameter, not a union copy

- **confidence** c9
- **tags** `cpp:param` `cpp:local` `cpp:union` `cpp:int` | `asm:mov` | `topic:codegen-idiom` `topic:mis-model`
- **first seen** `UnpackTag` @0x0013b970, 82.31 -> **100.00 EXACT**

## Symptom

Retail picks individual bytes out of a 4-byte scalar parameter by addressing the
parameter's own home slot — including a 16-bit load that covers two of them at
once — and the recompile instead materialises a local copy plus a callee-saved
register:

```
; retail                                   ; recompile
mov  cx,WORD PTR [esp+0x6]                  mov  ecx,DWORD PTR [esp+0x4]
xor  eax,eax                                push ebx
test ch,ch                                  mov  bl,BYTE PTR [esp+0xb]
...                                         xor  eax,eax
test cl,cl                                  test bl,bl
mov  ecx,DWORD PTR [esp+0x4]                mov  DWORD PTR [esp+0xc],ecx   <- the copy
test ch,ch                                  ...
...
mov  cl,BYTE PTR [esp+eax+0x3]              mov  cl,BYTE PTR [esp+eax+0xb]
```

The tells are (a) an extra callee-saved push the retail prologue does not have,
(b) a store of the incoming parameter into a fresh slot, and (c) every byte
displacement shifted by the size of that slot plus the push.

## Cause

A union local — the obvious way to read an enum/int's bytes without a cast —

```cpp
DwordBytes tagBytes;          // union { u32 m_value; u8 m_bytes[4]; }
tagBytes.m_value = IDX(tag);
```

is a genuine **copy**. cl5 will not coalesce it with the parameter's home slot
(both have their address taken), so the frame grows and the byte reads move.
Indexing the union's `m_bytes` array through a pointer (`u8* tb = tagBytes.m_bytes;`)
makes no difference — the copy is already committed.

## The source

View the parameter in place. `&param` is an implicit conversion to `const void*`
and `static_cast` takes it the rest of the way, so no `reinterpret_cast` is needed
(and the `reinterpret_casts` ratchet stays flat):

```cpp
// The tag's four characters, most significant byte first. Byte-evidenced: retail
// addresses the PARAMETER's own slot, so this is a view of `tag`, never a copy.
const u8* tb = static_cast<const u8*>(static_cast<const void*>(&tag));
```

With the copy gone, cl5 also fuses the two adjacent byte tests into the single
`mov cx,WORD PTR [esp+0x6]` + `test ch,ch` / `test cl,cl` pair on its own — that
fusion is a *consequence* of reading the home slot, not a separate lever.

## Related

- `narrowing-arg-needs-an-int-local.md` — the mirror case at a call site: a byte
  ARGUMENT read straight from memory takes a narrow load; binding it to an `int`
  local first restores retail's full-dword push.
