# An out-parameter helper that loads the pointer into EAX at every arm RETURNS it

tags: cpp:return cpp:param | asm:mov asm:ret asm:push | topic:codegen-idiom

symptoms: a `void f(T* out)` modelled correctly (right arms, right constants) plateaus
  in the low 80s; retail loads the out-pointer into **eax** at the top of every arm and
  leaves it there across the `ret N`, and spends a callee-saved register (`push esi`)
  on a value your version happily keeps in eax

confidence: 9/10

## The tell

Two signals together, not either alone:

1. **eax holds the out-pointer at every `ret`.** A `void` callee is free to use any
   scratch register for the destination; cl picks `ecx` (`mov ecx,[esp+4]`) when eax is
   busier. Retail choosing eax at *every* exit is the return-value register being fed.
2. **An extra callee-saved push appears.** Reserving eax for the return value pushes one
   more live value into `esi`/`edi`, so a leaf that should need no saves grows a
   `push esi`/`pop esi` pair. That pair is the cheapest thing to spot in a `--diff`.

```asm
; retail                                  ; our `void` version
mov edx,[ecx+0x17c]                       mov eax,[ecx+0x17c]
push esi                                  mov edx,[ecx+0x180]
mov esi,[ecx+0x180]                       mov ecx,[ecx+0x444]
mov ecx,[ecx+0x444]                       dec ecx
lea eax,[ecx-0x1]                         cmp ecx,0x7
...                                       ...
mov eax,[esp+0x8]   ; <- out, in EAX      mov ecx,[esp+0x4]   ; <- out, in ECX
mov [eax],edx                             mov [ecx],eax
mov [eax+0x4],esi                         mov [ecx+0x4],edx
pop esi                                   ret 0x4
ret 0x4
```

The fix is the signature, not the body: `T* f(T* out) { ...; return out; }`.

## Why this is safe to change

The `RVA()` binding uses OUR mangled name on BOTH sides (the Model feeds the
synthetic PDB), so `?f@C@@QAEXPAH@Z` -> `?f@C@@QAEPAHPAH@Z` re-binds transparently.
Check the call sites only for source compatibility — an ignored return value needs no
edit.

## Evidence and the counter-examples

`CGrunt::EntranceTileOffset` @0x56f80 (gruntcombat): **81.65 -> 97.89** on the signature
alone; the residual is one swapped store pair in the merged NORTHWEST/default arm.
`CGrunt::SetArrivalTarget` @0x52ed0 gave the same tell in scalar form — retail
materialises `mov eax,1` for a member store that could have used an immediate, i.e. the
constant is CSE'd with `return 1`.

**Both signals are required.** Tested and REJECTED in the same session:
`CGrunt::ConsiderArrival` (82.37 -> 63.37) and `CVoiceManager::ClearVoiceIndicatorSlots`
(82.00 -> 62.00) — both end with a zero in eax, but that zero is a loop/NULL constant
that merely survives to the `ret`, and neither shows the extra callee-saved push. A
trailing `xor eax,eax` on its own proves nothing.
