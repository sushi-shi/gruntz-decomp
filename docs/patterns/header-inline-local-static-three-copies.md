# One RNG definition with shared game state and private library states

tags: cpp:inline cpp:static cpp:linkage cpp:namespace msvc5:common | topic:method topic:identity
symptoms: one utility function (CREDITZ-printed GetRandomNumber) but the
image holds THREE guard/seed pairs with a caller partition exactly on
module boundaries (game band / wwd lib / fader lib)
confidence: 10/10 for the retained compiler/linker behavior; original include scope unresolved

## Retained implementation

`include/Utils/RandomNumber.inl` contains the only `GetRandomNumber` definition,
with the credits body unchanged. `GameRand.h` includes it at global scope.
`WwdFactoryObject.cpp` and `FaderEffects.cpp` each include it inside an anonymous
namespace. Both invented RNG member definitions were removed. The fader's range
wrapper is a free inline function in that private scope; it has no instance
state and no reason to claim a `CFaderSine` receiver. Moving it out of the
class preserves both callers' instruction bytes and ordered referents.

This models the observed shared/private state boundary without assigning RNG
ownership to the animation or fader class. The local scopes are a reconstruction
of that boundary, not a claim that the original source used this exact include
arrangement. No calling conventions, class layouts, or RNG arithmetic changed.

The actual 293-object candidate links with zero unresolved externals and zero
duplicate-symbol warnings, without `/FORCE`. Its map contains exactly three RNG
guard/seed pairs as COMMONs, all at different addresses. In this link:

| State | Guard VA | Seed VA |
|---|---|---|
| Animation | 0x006da874 | 0x006da880 |
| Fader | 0x006da884 | 0x006da890 |
| Game | 0x006da894 | 0x006da8a0 |

These are candidate placement facts, not retail RVA claims. Each guard-to-seed
delta is +12, matching the two library pairs in retail without filler storage.
The library callers preserve their prior instruction bytes and ordered
referents. Animation `Rng2Next` remains exact; fader ApplyInit/RenderFrame retain
their 96.5000/94.6626 MAX and their existing register/scheduling residues.

## Receiver-independent wrappers still need a caller ABI audit

The fader range adapter uses only its two integer arguments and the private
RNG. Converting it from `CFaderSine::GetRandom` to a free inline `GetRandom`
in the same private scope preserves ApplyInit's 300 bytes/8 relocations and
RenderFrame's 1224 bytes/24 relocations exactly after namespace normalization.
There is no emitted range-helper body or receiver-bearing call to preserve.

Animation's `Rng2Next` is a different case. Retail `CAniAdvanceCursor::Advance`
loads the animation-record pointer with `mov ecx,edi` at both 0x15c5f6 and
0x15c624, immediately before its calls to 0x15cbe0. The callee immediately
overwrites CL and never reads the incoming receiver, but the callers still
support a receiver-bearing ABI. The exact source-facing method name remains
inferred; absence of receiver use does not disprove a method boundary.

Two actual-TU controls replaced `dd->Rng2Next()` with the private free inline:
one kept its definition at the top, the other used a forward declaration and
included the same body at the old standalone's source position. VC5 expanded
both sites in both forms and emitted no standalone RNG. The caller grew from
1456 to 1576 bytes against retail's 1436-byte extent. Neither control recovers
the two receiver-bearing calls, so neither was retained. This is not evidence
for duplicated RNG arithmetic: the kept method forwards to the one shared body.

## Stable attribution of VC5 anonymous-namespace state

VC5's namespace token is `?%<absolute source path><compiler nonce>@`. Rebuilding
the same TU changes the nonce, and worktrees change the prefix. Neither belongs
in a committed pin. `core.msvc_names.anonymous_namespaces` canonicalizes only
tokens whose declaration path is a repository source `.cpp`/`.cxx`/`.cc`:
`?A0x` followed by the first 16 hex digits of SHA-256 of its `src/...` path.
The enclosing source path and function identity remain distinct. Tokens naming
headers are left untouched because one header can instantiate in several TUs.

The COMMON manifest uses that canonical name. The data-manifest owner lookup,
comparison copies, and raw-referent resolver use the same rule. Real compiler
objects and linker inputs retain their original names. Name collisions within
an object, or the same source-file anonymous COMMON in multiple base units,
are errors rather than opportunities to merge states. The graph includes the
name module in every affected consumer's dependencies.

`AnonymousNamespaceControls` exercises actual COFF normalization with live
relocations, checks the COMMON-owner and raw-referent consumers, proves that
two TUs stay distinct, and rejects collisions. Its negative control disables
normalization in the consumer and verifies that two rebuilds no longer agree.
The source-name corpus also checks injectivity across every real base object.

## The retail facts

| copy | guard | seed | guard/seed refs; functions / reconstructed TUs |
|---|---|---|---|
| A | 0x2c127d | 0x2c1288 | 57/54 refs; 16 functions / 12 TUs |
| B | 0x2c278c | 0x2c2798 | 2/2 refs; Rng2Next / 1 TU |
| C | 0x2c279c | 0x2c27a8 | 6/8 refs; ApplyInit and RenderFrame / 1 TU |

B and C are the LAST allocations in .bss (the band ends at 0x2c27ac);
all three pairs are guard-BEFORE-seed. The reconstructed COMMON identities are
not original symbol evidence. Placement in zero-filled image storage does not
independently prove COFF storage class or original TU boundaries.

## The original probes (cl 5.0 SP3 + era link, 2026-08-22)

1. `__inline int GetRandomNumber()` (external) in two TUs: each obj emits
   `?holdrand@?1??GetRandomNumber@@YAHXZ@4JA` COMMON(4) + `??_B...@51`
   COMMON(1). Linking both objs -> exactly ONE guard + ONE seed in the map,
   class `<common>`, guard first. Same-name COMMONs FOLD across objs (and
   libs). This excludes ordinary external inline under one enclosing identity.
2. `static __inline` (internal): each obj emits `_?holdrand@...$S168` and a
   guard `_?$S1@...$S170` in the TU's OWN section, seed@+0 and guard@+4.
   This particular form does not reproduce retail's seed/guard arrangement.
   It does not exclude other internal-linkage forms.

## Counterexamples and actual-TU controls, 2026-09-05

The old inference that three state identities require three source revisions
or authored bodies is false. The credits give the body but not its enclosing
scope or compile flags. Controlled tests at `5c87009ad`, in worktree
`codex/random-linkage`, used cl 11.00.7022 and link 5.10.7303 with `/O2 /MT`.
No executable was run. Link tests retained every caller with `/OPT:NOREF`;
archive tests used unique `/INCLUDE` entries to force every member's extraction.

| Form, compiled in three TUs | COFF and final-link result |
|---|---|
| One ordinary external inline header | One COMMON pair; still one when the objects are archived separately. |
| `static inline` | Three section-local pairs, each seed@0/guard@4. |
| Inline in anonymous namespace | Three uniquely named COMMON pairs, each linked guard before its seed. Internal linkage does not imply section-local storage. |
| Same external header under `/Gd`, `/Gz`, `/Gr` | Three COMMON pairs with enclosing signatures `YAHXZ`, `YGHXZ`, `YIHXZ`; direct-object and archive links both preserve them. Body text is unchanged. |
| `extern "C"` inline | One COMMON pair, enclosing identity `@@9`. |
| Same nonstatic member inline | One class-qualified COMMON pair. |
| Same static member inline | One class-qualified COMMON pair, with a different guard spelling. |
| Free inline template instantiated with int, long, char | One pair in this VC5 control: the local-static names omit the unused template argument. |
| Unmarked free definition under `/Ob2` | Section-local seed/guard, not external-inline COMMONs. |

The decisive composition writes the RNG once in `random.h`. Two game callers
include it globally; two library callers include that same file inside separate
anonymous namespaces, after the SDK declarations:

```cpp
#include <windows.h>
#include <mmsystem.h>
namespace {
#include "random.h"
}
```

The linked map contains exactly three COMMON pairs: one shared game pair and
one per library caller. Named `Wwd` and `Fader` include scopes are a separate
positive control with the same partition. The scope names and include placement
are hypotheses, not recovered source. These small maps have guard-to-seed +4;
they prove sharing and linkage, not retail's complete intervening-data layout.
No filler data or placement directives were introduced.

The real `worldsoundset`, `attractstate`, `wwdfactoryobject`, and `fadereffects`
TUs were then compiled after replacing both member RNG bodies with calls to
the same `GameRand.h` definition. External inline, static inline, and anonymous-
namespace inline were tested separately. All three forms preserve baseline
instruction bytes after masking relocation fields, sizes, and relocation counts
in all six selected callers:

| Caller | Bytes | Relocations |
|---|---:|---:|
| CGruntzMgr::Rand | 70 | 5 |
| CRandomAmbientSound::InitCycleTiming | 225 | 9 |
| CAttract::EnterState | 425 | 26 |
| CAniRecordView::Rng2Next | 70 | 5 |
| CFaderSine::ApplyInit | 300 | 8 |
| CFaderSine::RenderFrame | 1224 | 24 |

This is instruction equivalence to baseline, not strict retail exactness.
The animation wrapper is exact; fader MAX remains 96.5000/94.6626. The alternatives
change ordered state identities: plain external inline collapses the three
states, while uniform static or anonymous inclusion splits the shared game
state across TUs. Both fail retail sharing under the current TU model. An
original unity build is unsupported, but reconstructed TU boundaries alone
cannot disprove it.

A whole-file `/Gd` game, `/Gz` animation, `/Gr` fader composition leaves five
selected bodies instruction-identical but changes fader ApplyInit from 300 to
296 bytes. It calls fastcall ScatterSamples, whereas retail pushes four
arguments and performs `add esp,0x10` after the call at 0x17ff0e. This rejects
that unqualified flag change against the surrounding ABI. It remains a valid
counterexample to the logical claim about duplicated source; no calling-
convention overrides were added to evade the negative control.

Artifacts: `build/random-linkage/`, `build/random-real-tu/`, and
`build/random-scopes/` contain generated callers, objects, maps, and
`results.json`. Drivers are `build/random_linkage_probe.py`,
`build/random_real_tu_probe.py`, and `build/random_scope_probe.py`. The real-TU
driver restores the original headers in a `finally` block. These are disposable
experiments, not proposed source or generated state to commit.

## Surviving LithTech search

An exhaustive `git grep` of public revision
`845119cd2da5bca34e8d87ee867f9b7cf999636c` for `GetRandomNumber`, `holdrand`,
`214013`, and `2531011` found only the arithmetic in
[FEAR WeaponPath.h](https://github.com/jsj2008/lithtech/blob/845119cd2da5bca34e8d87ee867f9b7cf999636c/FEAR/Shared/WeaponPath.h#L237).
That implementation identifies the Win32 rand.c algorithm and uses an object
seed; it does not recover Gruntz's static-storage declaration.

[Blood2 ClientUtilities.cpp](https://github.com/jsj2008/lithtech/blob/845119cd2da5bca34e8d87ee867f9b7cf999636c/Blood2/ClientShellDLL/ClientUtilities.cpp#L18)
and Shogo's RiotCommonUtilities.cpp preserve the GetRandom overload family and
identify its origin as `gamework.h`. Blood2 ObjectUtilities.h also contains the
closed-range helper as an inline. These support shared utility ancestry, but
none contains the credits function's local static. No gamework.h survives in
the searched tree. No new RNG owner was recovered.

## Reuse

First census actual guard and seed referents without assuming generated names
are original. Separate body identity, state sharing, storage class, and source
scope. Link every caller, including forced archive members. Test anonymous and
named scopes and calling conventions before declaring duplicated bodies
necessary, and inspect surrounding calls when changing flags. Preserve ordered
state identities even when instruction bytes agree.

Do not restore the removed CAniRecordView/CFaderSine RNG members to name the
states. Their original ownership was never established. Preserve the state
boundary through the shared definition and explicit local include scopes.
