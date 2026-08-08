# A `je` that lands PAST the call but BEFORE the trailing stores means those stores are outside the guard
tags: cpp:branch cpp:if cpp:local | asm:jcc asm:mov | topic:codegen-idiom
symptoms: je lands mid-block | parameter slot reused as a temp | mov [esp+N],eax then mov eax,[esp+N] over a call

The guarded block ends with a call followed by a run of plain assignments, and
retail's failing branch does **not** jump over the whole block - its `je` target
is the first instruction *after* the call. Those assignments are unconditional
in the source; only the call is guarded.

```cpp
// what the je target proves - NOT `if (eq) { Call(); a = 1; b = 1; }`
if (eq) {
    ConsiderArrival(0);
}
fresh = 1;
defer = 1;
```
```asm
  test   cl,cl
  je     0x4f13f          ; <- lands BELOW the call, ABOVE mov eax,1
  push   0x0
  mov    ecx,esi
  call   ConsiderArrival
0x4f13f:
  mov    eax,0x1
  mov    DWORD PTR [esp+0x24],eax    ; fresh = 1   (both paths)
  mov    DWORD PTR [esp+0x2c],eax    ; defer = 1   (both paths)
```

**Corollary that finds it without reading branch targets.** Because the trailing
store is unconditional, the variable is *dead* everywhere above it, so cl homes
an unrelated temp in that variable's stack slot - including an incoming
**parameter** slot. Seeing a temp spilled to `[esp+0x24]` when `[esp+0x24]` is
parameter 2 is not a delinker artifact; it says parameter 2 is overwritten
unconditionally later on that path. Our version, with the assignments inside the
guard, kept the parameter live and had to allocate a fresh slot (it picked the
dead `variant` slot at `[esp+0x28]` instead).

Steerable. `CGrunt::LoadGruntTypeTable` 0x4dd50, ten toy-pickup arms: 87.12 ->
90.20 (with the sibling `AI_NONE` fix). Both `fresh=1` and `fresh=1;defer=1`
variants collapse to one shared block each, which is why there are only two such
tails in retail for ten arms.
