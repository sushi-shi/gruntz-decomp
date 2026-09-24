# A late parser local reuses a dead argument home and restores the EH receiver slot

tags: cpp:local cpp:scope cpp:constructor cpp:inline | asm:mov asm:push | topic:regalloc topic:eh topic:source-shape

The string constructor of `zBitVec` (0x16d3a0) had the same extent, calls,
control flow, constants and ordered referents as retail, but scored 95.66782%.
The first difference was the saved constructor receiver's stack offset, not
the parser arithmetic: base saved it at `[esp+0x18]` during argument setup;
retail used `[esp+0x14]`. Both allocated eight local bytes.

The maximum parsed index was initialized at function entry in base, before
checking for a null string and resolving the default capacity. Retail delayed
its zero until after those guards, just before the first `isspace` call.
There it occupied the dead incoming string-argument home, while the actual
string cursor already lived in ESI. The baseline instead allocated a separate
local home. This is an initialization/lifetime clue, not permission to cast an
argument slot or introduce padding.

Controlled builds in the real TypeKeyColl TU with VC5 `/O2 /MT`:

| Source composition | Primary result | Extent / instructions | Calls / branches / returns / relocations |
| --- | ---: | --- | --- |
| Expanded storage selectors and error tails; early maximum initialization | 95.66782% | 0x344 / 289 | 24 / 48 / 1 / 34 |
| Restore the source-attested `body()` selection helper at both bit writes | 95.66782% | unchanged | unchanged |
| Also use the existing `handle` helper at the three error tails | 95.66782% | unchanged | unchanged |
| Keep both helper layers; initialize the maximum after the two entry guards | 100% | unchanged | unchanged |

The successful change is ordinary source: move `i32 maxv = 0;` immediately
before the token-cursor declaration, after the null/default-size checks.
The compiler chooses argument-home reuse itself. No signature, class layout,
parser loop, allocation, error identity, or constructor ownership changes.
The helper-only controls isolate the local-lifetime change as the closing
lever; they do not establish that those helpers are necessary for exactness.

The single unwind action also becomes identical:
`*[ebp-0x14] -> zErrHandling::~zErrHandling`. It is not an independent funclet
fix. The parent now has the authentic receiver home, which decides its cleanup
operand. Primary-function matching alone would not prove that result.

For reverse use, compare the first zero store with the parameter's last live
use. A same-sized frame plus a shifted EH receiver home can indicate an
unjustified early local lifetime. Test the declaration/initialization boundary
in humane source, keeping all supported helper layers intact, then audit both
the parent and cleanup action.

This does not overturn the negative control for `CPlay::ValidateLevelTiles`
in [the EH slot-shift pattern](eh-slot-shift-measures-the-parents-local-homing.md):
moving that function's initialization was flat. It disproves generalizing that
result to another constructor with independently observed argument-home reuse.
