# An EBP frame + `push ebp` inside a loop means the span loop was `__asm`

tags: cpp:asm cpp:loop | asm:push asm:or asm:inc | topic:codegen-idiom
symptoms: a /O2 function keeps a real frame pointer (`push ebp; mov ebp,esp; sub esp,N`,
  locals addressed `[ebp-N]`, params `[ebp+N]`) where every neighbouring function is
  ESP-relative; inside its innermost loop retail does `push ebp` ... `pop ebp` to free one
  more register; the loop body uses `or ax,ax` instead of `test ax,ax`, `inc edi` twice
  instead of `add edi,2`, and reads all its inputs from GLOBALS that the C code stored one
  statement earlier; after the loop the compiler reloads every cached value
confidence: 9/10

## Mechanism

cl 5.0 disables frame-pointer omission for **any** function containing an `__asm` block, so
a single hand-written loop forces `push ebp; mov ebp,esp` on the whole function and makes
every local/parameter `[ebp+-N]`. It also drops all register caches across the block, which
is why the compiler-generated code right after the loop re-reads globals and params.

The give-aways that the loop is hand-written, not compiled: assembler-programmer idioms cl
never picks (`or ax,ax`, paired `inc`), `push ebp` used as a scratch spill inside a loop
that does not touch memory through ebp, and the surrounding C code marshalling every loop
input into a **global** (the classic way to hand an `__asm` block its state).

## The fix

Transcribe it. MSVC inline asm resolves C locals and globals by name, so the block reads
exactly like the retail listing:

```cpp
g_rasterDestPtr = Span16(g_rasterDestRow) + rx;
__asm {
    mov  edi, g_rasterDestPtr
    mov  esi, g_warpTexBase
    mov  ebx, g_warpU
    mov  edx, g_warpV
    mov  ecx, span          // a C local
    push ebp
    mov  ebp, g_warpVStep
lp: mov  eax, edx
    add  edx, ebp
    and  eax, g_warpUMask
    or   eax, ebx
    shr  eax, 0eh
    add  ebx, g_warpUStep
    mov  ax, word ptr [esi + eax * 2]
    mov  word ptr [edi], ax
    inc  edi
    inc  edi
    dec  ecx
    jne  lp
    pop  ebp
}
```

Evidence: `WarpTextureBlit` 53.7 -> 71.5 from replacing three C span loops with their asm.

## Related

The same function also proved the `_ftol` half: a `static i32 helper(double)` wrapper around
`(i32)v` compiles to a real `sub esp,8; fstp qword [esp]; call helper; add esp,8` per site,
where the plain `static_cast<i32>(expr)` leaves the value on the x87 stack and emits the bare
`call __ftol` retail has. Never wrap a float->int conversion in a helper.
