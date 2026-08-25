# A flag-guarded ~CString that is NEVER constructed is a release-dead `TRACE()`

- **Confidence:** 10/10
- **Tags:** `cpp:eh` `cpp:dtor` `cpp:class` `cpp:branch` | `asm:test` `asm:and` `asm:call` | `topic:codegen-idiom`
- **Seen:** `CAreaMgr::LoadObject{Image,Sound,Anim}Resources` 0x0009a510 / 0x0009a910 /
  0x0009ac20 (90.94 / 85.78 / 85.78 -> 100.00 EXACT), `CDDrawChildGroup::DeserializeObjects`
  0x0015b0e0 (74.55 -> 96.17). These are the ONLY four sites in the whole image.

## Symptom

Retail's frame is a few dwords larger than the recompile and carries two locals the
source has no account of: a **dword zeroed at entry and never written again**, and a
**`CString` that is destroyed but never constructed**:

```
   9a93a:  mov  DWORD PTR [esp+0x1c],ebx      ; flag := 0   <- only write, ever
   ...
   9aafe-: mov  al,BYTE PTR [esp+0x1c]
           test al,0x1                         ; flag & 1  -- provably false
           je   skip
           mov  eax,DWORD PTR [esp+0x1c]
           lea  ecx,[esp+0x24]                 ; a CString with no ctor anywhere
           and  al,0xfe                        ; flag &= ~1
           mov  DWORD PTR [esp+0x1c],eax
           call ??1CString@@QAE@XZ
   skip:
```

Screen for it with `and al,0xfe` (`24 fe`) preceded within ~10 bytes by
`lea ecx,[esp+..]` (`8d 4c 24`). Note the flag is never SET: the image contains no
`or <flag>,1` for any of these.

## Why

That is MSVC's **destructor flag** — one bit per object whose construction is
conditional relative to its destruction point. MFC 4.2 (`msvc/include/AFX.H:342`)
defines, in **release**:

```c
#define TRACE   1 ? (void)0 : ::AfxTrace
```

So `TRACE("%s\n", (LPCTSTR)e->GetName())` parses as a ternary whose `AfxTrace` arm is
**statically dead**. The front end has already allocated the argument's `CString`
temporary and its flag bit; the optimizer then deletes the ctor, the call and the
`or flag,1` — but the flag-guarded cleanup survives. `TRACE0`..`TRACE3` expand to
nothing, so only bare `TRACE` leaves this.

A **runtime** `if` around the TRACE survives too, and shows up as an extra test in
front of the flag test (`CDDrawChildGroup::DeserializeObjects`: `test bl,1` on the
`LogicTypeId` argument, then `test BYTE PTR [esp+0x10],1`).

## The fix

Put the `TRACE` back, at the statement position where the guarded dtor sits:

```cpp
registry->LoadFromTree(handle, const_cast<char*>((LPCTSTR)e->GetName()), "_");
TRACE("%s\n", static_cast<LPCTSTR>(e->GetName()));   // release-dead
e->m_flag = 1;
```

The argument must build **exactly one class temporary** — a by-value getter, a
`CString(x)` copy, a concatenation. Everything else about the arm (the format string,
which function was called) is unrecoverable, because the arm is gone; only "one
CString temporary lived here" is byte-proven. Say so at the site.

Trap: `TRACE("%s\n", strFoo)` (a `CString` straight through `...`) is what MFC code
usually writes and cl accepts it, but **clang rejects passing a non-POD through an
ellipsis**, which kills `gruntz`'s label/IR pass ("clang -emit-llvm produced no IR").
Use an explicit `(LPCTSTR)`-cast temporary instead.

## Related

- [`trylevel-bracket-is-an-raii-guard-object`](trylevel-bracket-is-an-raii-guard-object.md)
- [`two-identical-blocks-with-different-eh-states`](two-identical-blocks-with-different-eh-states.md)
