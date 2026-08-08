# A by-value `CString` return is written `return nrv;` in EVERY arm, and the no-work arm falls through

tags: cpp:return cpp:branch cpp:eh cpp:string | asm:push asm:jmp asm:jcc | topic:codegen-idiom
symptoms: a function returning a class BY VALUE (NRV) sits far below its neighbours; retail pushes
the NRV local in BOTH arms of an `if` with the copy-ctor `call` shared after the join
(`lea ecx,[esp+N]; push ecx; jmp <join>` against `...; push edx; <join>: call ??0CString@@QAE@ABV0@@Z`),
while the recompile pushes it once after the arms; `jcc_sieve` shows one inverted `jcc` and
`rets N -> N+1`
confidence: 9/10

## The shape

`CSpawnEntry::GetTail` @0x9a830 returns `CString` by value. Retail:

```asm
    cmp   ecx,0x8
    jg    0x9a890            ; len > 8  -> the assigning arm
    lea   ecx,[esp+0xc]      ; the NO-WORK arm is the FALL-THROUGH
    push  ecx
    jmp   0x9a8a2
0x9a890:
    add   eax,0x8
    lea   ecx,[esp+0xc]
    push  eax
    call  ??4CString@@QAEABV0@PBD@Z   ; tmp = m_name + 8
    lea   edx,[esp+0xc]
    push  edx
0x9a8a2:
    mov   esi,[esp+0x28]     ; the NRV slot
    mov   ecx,esi
    call  ??0CString@@QAE@ABV0@@Z     ; ONE copy-ctor, shared
```

Two `push <&tmp>` and one `call` is cl's own tail-merge of **two complete `return tmp;`
statements** — the same reading as
[two-arm CALL](positive-gate-enables-shrink-wrap.md) (§ "Two-arm CALL"), here applied to the
implicit copy-construction of a named return value.

## The lever, in two halves — both are needed

```cpp
// 66.38 - one return, one push, cl merges the exits
if (len > 8) {
    tmp = static_cast<const char*>(m_name) + 8;
}
return tmp;

// 92.64 - the return duplicated per arm; arms still in the WRONG order
if (len > 8) {
    tmp = static_cast<const char*>(m_name) + 8;
    return tmp;
}
return tmp;

// 100.00 EXACT - and the NO-WORK arm made the fall-through
if (len <= 8) {
    return tmp;
}
tmp = static_cast<const char*>(m_name) + 8;
return tmp;
```

The second half is read straight off the branch: retail's `jg <assign>` means the *assigning* arm
is the jump TARGET, so the source must test the inverse and let the do-nothing arm fall through.
`gruntz sema disasm <rva> --branches --diff` names it (`#1 base jle -> target jg`).

## Why it is not just "the positive gate"

The `return 0`-family levers in `positive-gate-enables-shrink-wrap.md` are about **merging**
epilogues (`b_ret > t_ret`). This is the opposite: `b_ret < t_ret`, and the fix DUPLICATES a
return. It works here because the duplicated thing is a `push` + a shared `call`, not an epilogue —
the /GX unwind tail (`~CString` on the NRV temp, `mov fs:0,ecx`, the pops) is still emitted once.

related: positive-gate-enables-shrink-wrap.md, identical-return-epilogue-tailmerge.md,
if-body-owns-the-fallthrough.md
