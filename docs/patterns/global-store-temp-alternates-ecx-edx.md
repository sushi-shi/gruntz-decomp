# Named RHS snapshots steer VC5's global-member receiver/value phase

**Tags:** `cpp:global` `cpp:assign` | `asm:mov` | `topic:wall` `topic:regalloc`
**Confidence:** 10/10

## Symptom

One instruction pair differs and it is only the scratch register:

```
base:    mov edx,[g_gameReg]  ...  mov [edx+0x100],eax
target:  mov ecx,[g_gameReg]  ...  mov [ecx+0x100],eax
```

Everything else in the function - including other reloads of the same global,
some of which use edx in retail too - may be semantically identical. It looks like
an unsteerable regalloc coin flip. It is deterministic, and a source-visible RHS
snapshot can be the lever even when C2 eliminates the local completely.

## Cause

The first micro-replica established that direct global-member stores draw address
temporaries from a repeatable ecx/edx phase. That observation was real, but the
original conclusion - that the phase could only be changed by adding or removing
an upstream store - was too narrow. VC5's front end also gives a named RHS local a
distinct value-creation tuple. C2 can eliminate the local while retaining the
changed receiver/value ordering and register phase.

The original store-count A/B remains a useful control:

| stores present | 0x118 | 0x100 | 0x124 |
|---|---|---|---|
| as reconstructed         | ecx | **edx** | ecx |
| delete the 0x118 store   |  -  | **ecx** | ecx |
| insert a 3rd store first | ecx / edx | **ecx** | edx |

The production A/B on `ApplyGameOptions` is decisive. The hand transcription
loaded saved globals directly at their consumers and scored 82.65%. Adding the
ordinary typed snapshots in source order produced this monotonic composition:

| source state | fuzzy | first corrected region |
|---|---:|---|
| direct global consumers | 82.65% | baseline |
| `b32 easyMode = g_savedEasyMode` | 97.84% | entry, resolution/audio order, first member store |
| plus `b32 voiceEnabled = g_savedVoiceEnabled` | 98.24% | voice-enabled member store |
| plus `i32 voiceVolume = g_savedVoiceVolume` | **100%** | remaining call chain and tail |

Every state retained 51 instructions, five calls, six branches, one return, and
27 relocations. The final 0xd3-byte body is normalized-byte exact. A named receiver
pointer was byte-identical to the 82.65% baseline, so the lever is specifically the
RHS value census and creation order, not caching `g_gameReg`.

## What this means when you hit it

When calls, branches, mnemonics, stores, and relocation multisets agree but a chain
of direct global loads alternates among eax/ecx/edx differently, reconstruct the
semantic RHS snapshot census before calling it a coin. Start at the first divergent
assignment, give the saved value its authentic width, and compose the next local
from that state. Do not infer that every global needs a local: in this function the
sound, music, MIDI-volume, and scroll globals remained direct consumers at exact.

The store-count phase is still a secondary hypothesis when the local census is
already evidenced and exhausted. A cached receiver pointer is a separate lever and
must be measured separately.

## Evidence

`src/Gruntz/VideoConfig.cpp` @0x036be0, controlled production builds under pinned
MSVC 5.0 SP3 `/O2 /MT`; 82.65 -> 97.84 -> 98.24 -> 100. The earlier 40-line
micro-replica remains evidence for the deterministic address-temp phase, but the
exact production closure supersedes its claimed lack of a local lever.

`ReadMenuOptionsDialog` @0x036a30 has a superficially similar residue but is not
proved to share this exact missing-local cause.
