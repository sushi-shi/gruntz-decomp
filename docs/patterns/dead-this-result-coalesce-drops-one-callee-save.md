# A byte-sized condition prevents coalescing onto dead `this`

tags: cpp:local cpp:return cpp:method cpp:bool | asm:push asm:mov asm:pop asm:neg | topic:regalloc topic:codegen-idiom
symptoms: calls, branches, returns, and relocations agree; the recompile is two bytes
shorter because it omits `push ebp`/`pop ebp`; retail keeps one call-crossing result in
EBP while the recompile reuses the now-dead `this` register for a later boolean
confidence: 10/10 (controlled exact closure)

**Corrected 2026-09-23:** the allocation-only conclusion below was premature.
The local's storage width, not an invented extra lifetime, closes this function.

`CMulti::CreateHostSessionAndPlayer` (`0x000b8b10`) is the calibration case. Both sides
emit 11 calls, four branches, one return, and 22 relocations. Retail is 113
instructions/`0x175` bytes and the recompile is 111 instructions/`0x173` bytes.

The values account for the whole delta:

| role | retail | recompile |
|---|---|---|
| `this` | ESI | ESI |
| group-enumeration result, live across later calls | EBP | EBX |
| player color, live across `GetName` | EDI | EDI |
| `RegisterLocalPlayer(...) == 0` | EBX | ESI after `this` dies |

Retail therefore saves EBP; the recompile coalesces the final failure flag onto ESI
immediately after its last `this` use and needs no EBP. The remaining instructions are
the same computation with those roles recolored.

Two earlier controls did not test the relevant type:

- declaring `failed` at function entry and assigning it at the call is byte-identical;
- copying `enumResult` into a result local and conditionally nulling it changes the
  skeleton to five branches and 108 instructions, moving away from retail.

The missed discriminator is the instruction AFTER the temporary's destructor:
retail uses `neg bl`; the reconstruction used `neg eax` on the copied dword
result. The expression `RegisterLocalPlayer(...) == 0` produces a boolean, but
the source stored it in a `b32` local. VC5 could therefore keep that local in
ESI, whose low byte is not addressable on this target.

```cpp
bool failed = RegisterLocalPlayer(hostPlayer->GetName(), hostColor, -1, m_localPlayerId) == 0;
return failed ? NULL : enumResult;
```

Changing only `b32` to `bool` gives the flag EBX, moves the earlier session
result to EBP, restores its push/pop, and reproduces every instruction and all
22 ordered relocation offsets, identities and addends: **93.1982 -> 100.0**,
373 bytes and 113 decoded instruction rows. The prior replacement of the
hand-expanded empty-string buffer initialization with `char buf[0x100] = ""`
was byte-flat; it is not the cause of this closure.

Reverse-use rule: when an apparently spare callee-save is missing, map the
values AND inspect each one's consumer width across intervening calls. A
byte test or byte negation excludes ESI/EDI/EBP, even when the definition uses
dword `neg/sbb/inc`. Establish boolean normalization versus a raw byte from
the full producer/consumer pair. A dead-receiver coalesce and flat declaration
tests do not prove an allocation-only wall. Never add fake uses to force the
save.
