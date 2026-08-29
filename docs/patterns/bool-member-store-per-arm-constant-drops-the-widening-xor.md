# Storing a comparison into a `bool` member: the per-arm constant form drops the widening `xor`

tags: cpp:branch cpp:local | asm:sete asm:xor asm:cmp asm:mov | topic:codegen-idiom
symptoms: retail materializes a byte flag with a bare `cmp r32,K; sete cl; mov [this+N],cl`
(often with an unrelated byte store scheduled between the `cmp` and the `sete`, inside the
flags window), while ours emits `xor ecx,ecx; cmp; sete cl; mov` — one extra `xor`, and the
neighbouring store sinks below the setcc; semdiff shows a single exclusive `xor` and equal
everything else
confidence: 8/10

## Mechanism

`m_flag = (delta == '\n');` computes an INT boolean and truncates it into the byte member,
so cl 5.0 zero-extends the scratch (`xor ecx,ecx`) before the `sete cl`. Writing the two
byte CONSTANTS per arm gives the if-converter a byte-width value from the start, and the
`xor` disappears; the setcc target register may then also reuse the byte register that
already held the member's OLD value (R6 byte class):

```cpp
// ours (widened):                      // retail (byte-width):
m_countLine = delta == '\n';            if (delta == '\n') {
                                            m_countLine = true;
                                        } else {
                                            m_countLine = false;
                                        }
```

A `bool nl = m_countLine; ... nl = delta == '\n'; m_countLine = nl;` reuse local is
byte-neutral (copy-propagated) — the lever is the per-arm constants, not the local.

## Production

`CButeMgr::ConsumeChar` 0x170390 (butemgr): 88.67 -> **100.00 EXACT** on this change alone.
The unrelated `m_currentChar` byte store also moved back up between the `cmp` and the `sete`,
matching retail's flags-window schedule, without any separate steering.

related: return-bool-via-local-setcc.md (the return-tail variant),
guard-result-zero-per-arm-not-an-initializer.md (per-arm materialization family)
