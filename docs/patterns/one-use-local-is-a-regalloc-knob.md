# A single-use local is a REGISTER knob — a named local dies at its last use, an inline sub-expression does not

**Tags:** cpp:local | asm:mov | topic:codegen-idiom topic:regalloc

## Symptom

The instruction stream is byte-identical to retail except that one scratch
register is different — and the difference is *upstream* of the instruction that
looks wrong: cl loaded some member into `eax`, and then had to put the next value
in `edx` because it still considers `eax` occupied.

```
retail:  mov eax,[esi+0x48]      ; m_sound
         mov ecx,[eax+0x1c]      ; ->m_pCurrent   (receiver)
         mov eax,[esp+0x8]       ; the arg — eax REUSED
base:    mov eax,[esi+0x48]
         mov ecx,[eax+0x1c]
         mov edx,[esp+0x8]       ; the arg — eax still "live"
```

## Why

cl5 keeps a **memory-value cache**: a member read written inline as a
sub-expression stays parked in its register as a CSE candidate, so that register
is not free for the next temp. A **named local** has a precise live range and its
register is released at its last use.

So a one-use local is a knob **in both directions**, and you must try both:

- **ADD a local** to free the register earlier
  (`CGruntzSoundZ* snd = m_sound; if (snd->m_pCurrent) snd->m_pCurrent->SetVolume(0, ms);`)
- **DELETE a local** so the value becomes a cache-parked temp instead
  (`g_gameReg->m_cmdGrid->m_units[...]` rather than `CGruntzMgr* reg = g_gameReg;`)

The same knob also selects *how many* registers a pointer chain consumes: with the
middle link bound to a local, cl chases the chain in ONE register
(`mov eax,[ecx+0x1c]; mov eax,[eax]`) instead of spending a fresh one per link —
see [pointer-chain-hoist-intermediate-local](pointer-chain-hoist-intermediate-local.md).

## Evidence (all 2026-07-28, each the SOLE residue of an otherwise byte-exact fn)

| function | edit | result |
|---|---|---|
| `CGruntzMgr::MuteMusicIfActive` 0x0915d0 | **add** `CGruntzSoundZ* snd` for the second read | 99.62 → **100** |
| `CGruntzMgr::RestoreMusicVolumeIfActive` 0x091620 | same | 99.62 → **100** |
| `CGruntzMgr::SetRunState` 0x092340 | **add** `i32 run = m_soundEnabled;` for the global store (buys the 5-byte `a3` accumulator form) | 99.62 → **100** |
| `CGruntzMgr::ExitModalUI` 0x0903f0 | **add** `CDDrawPtrCollections* pc` (middle link) — fixed **three** vtable-call scratch registers at once | 99.57 → **100** |
| `CGruntzMgr::EnterModalUI` 0x08ef10 | same one-line edit | ~93 → **100** (had been filed an esi/edi wall) |
| `CGruntHealthSprite::HealthUpdate` 0x07f180 | **delete** `CGruntzMgr* reg = g_gameReg;` | 99.64 → **100** (had been filed a zero-register-pinning wall) |
| `CGrunt::StepArrivalCommitA` 0x065300 | **delete** `CGruntzMgr* g = g_gameReg;` (also fixed the tx/ty emission order) | 99.70 → **100** |

Note the last two: the sibling `CGrunt::StepArrivalCommitB`, which *keeps* the
local, is 100% — the right spelling depends on the register state the preceding
statements leave behind, so **decide it per call site, by experiment**, not by a
tree-wide rule.
