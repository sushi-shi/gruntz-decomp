# The out-param `lea`/zero-store slot is NOT source-reachable - two in-tree EXACT controls prove the spelling

tags: cpp:local cpp:call msvc5:mfc | asm:lea asm:mov | topic:wall topic:scheduling
symptoms: a `Map.Lookup(key, out)` site at 96-98% whose ENTIRE residue is that retail emits
`lea r,[out]` before `mov [out],0` (or sinks the store one further slot into the argument
pushes) and cl emits them the other way round; identical size, identical instruction
multiset, `walls diagnose` says REGALLOC/SCHEDULING
confidence: 9/10 (2 EXACT controls, 4 measured negative controls)

```cpp
CObject* found = 0;
m_spriteMgrHolder->m_workerMap->m_map1.Lookup(name, found);
if (found) { return 1; }
```
```asm
; retail 0xe2d32                      ; cl
lea  eax,[esp+0x50]                   mov  DWORD PTR [esp+0x50],0x0
mov  DWORD PTR [esp+0x50],0x0         lea  eax,[esp+0x50]
mov  ecx,DWORD PTR [ecx+0x18]         mov  ecx,DWORD PTR [ecx+0x18]
push eax                              push eax
```

## Why the spelling is already right

Two functions in the tree use the SAME three-line spelling and are **100.000 EXACT**:
`CDDrawChildGroup::AttachSprite` 0x159830 (`CObject* tmpl_ob = 0;` + a two-level receiver
chain, `lea` then store, exactly retail's order) and `CGruntzSoundZ::FindBank` 0x138730.
So the source is not the variable: cl reaches retail's order from this spelling whenever the
surrounding block lets it. The affected sites are `CSpriteRefTable::LoadGruntzPalette`
0xe2d10, `CSpriteRefTable::Add` 0xe2890 (retail sinks the store one slot FURTHER, past the
second `push`) and `CGrunt::EnsureStruckSlot` 0x57b70 (two slots). Both orders cost the
same 5 Pentium issue slots - the `mov [esp+disp8],imm32` carries both a displacement and an
immediate, so it is unpairable either way - which is why the tie exists at all.

## Negative controls (measured, all removed)

| probe | result |
|---|---|
| declare the out-local above the early-return guard | 96.923 -> **95.831** (the store moves into the entry block and the frame grows 0x40 -> 0x44; retail keeps the local in the dead `src` parameter home slot) |
| hoist the receiver into its own local (`CDDrawWorkerMapSmall* wm = ...`) | neutral to the byte |
| declare the `char buf[0x40]` before the out-local | neutral to the byte |
| TU declaration-count sweep, N = 0..15 throwaway prototypes above the first project include | **dead flat**: `LoadGruntzPalette` 96.9231 and `Add` 97.5000 to four decimals at every N |

So the lever of `declaration-count-window-steers-regalloc.md` is unavailable in this TU too,
and the class is C2-anchored. Park it; do not re-derive these four probes.

related: [declaration-count-window-steers-regalloc.md](declaration-count-window-steers-regalloc.md),
[animation-switch-pair-is-one-inline-member.md](animation-switch-pair-is-one-inline-member.md)
