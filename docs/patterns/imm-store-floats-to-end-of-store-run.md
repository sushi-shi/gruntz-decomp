# A lone IMMEDIATE store floats to the end of a same-register store RUN — an inlined member-object method ENDS the run

tags: cpp:member cpp:inline cpp:ctor | asm:mov | topic:codegen-idiom topic:scheduling
symptoms: `mov [esi+N],0xf` last instead of Nth, `mov dword ptr [reg+disp32],imm32`,
"immediate-float scheduling wall", "cl floats the imm store to the tail of the store cluster"
confidence: 9/10
variants: i64-zero-store-batching-reveals-subobjects.md

A run of `mov [this+off],<same reg>` stores with ONE `mov [this+off],imm32` among them:
cl5 emits every register store in source order and sinks the immediate store to the **end
of the run**. Reordering the source does nothing — position 1, position N and position last
all produce the identical output. Retail keeps the immediate in source position, which means
retail's run was **shorter**: something ended it right after the immediate store. The thing
that ends a run without emitting an instruction is an **inlined method call on an embedded
sub-object**.

```cpp
// before - one flat run; cl sinks `m_comboSel = 0xf` past all four trailing edi stores
m_focusX = 0;  m_focusY = 0;  m_comboSel = 0xf;
m_doneFlag = 0;  m_030 = 0;  m_latency.m_avg = 0;  m_latency.m_count = 0;

// after - PlayerLatency gets the method it obviously owns; the inline call splits the run
struct PlayerLatency { i32 m_avg, m_count; void Clear() { m_avg = 0; m_count = 0; } };
m_focusX = 0;  m_focusY = 0;  m_comboSel = 0xf;
m_doneFlag = 0;  m_030 = 0;  m_latency.Clear();
```

```asm
target: mov [esi+0x224],edi | mov [esi+0x228],0xf | mov [esi+0x2c],edi | ... | mov [esi+0x230],edi
base:   mov [esi+0x224],edi | mov [esi+0x2c],edi  | ... | mov [esi+0x230],edi | mov [esi+0x228],0xf
```

STEERABLE. Corroborate before inventing the method: a SIBLING function whose run is cut
short by a call already matches (`GruntzPlayer::SeedForSlot` @0x0da870 has the same nine
stores but a `GetDefaultName` call after the seventh, and was already 100%). Evidence:
`GruntzPlayer::Clear` 94.65, `::Reset` 94.88, `GruntzPlayer::GruntzPlayer` 97.33 → **all
three 100% EXACT** from one `PlayerLatency::Clear()`; all three had been filed
"immediate-float scheduling wall … reordering / hoisting to a local does not flip it".
