# An inline phase return can replace a shared `goto` edge
tags: cpp:inline cpp:return cpp:goto cpp:loop | asm:jmp asm:jcc | topic:codegen-idiom topic:source-shape
symptoms: several paths jump to one continuation or failure tail; direct positive-gate nesting adds comparisons; early returns would be natural inside a named helper; the caller and retail retain the same shared destination block
confidence: 9/10

A forward `goto` is sometimes evidence for a missing source boundary rather than
an authored label. If several paths mean "this phase is finished" or "this
attempt failed" and all target one caller-owned block, extract the complete
phase into a small `inline` helper. An early `return` from the expanded helper
can become the same edge to the caller's continuation, so VC5 can preserve the
retail CFG without an explicit source-level goto.

## Controlled A/B evidence

`CMenuState::Render` 0x0a0750 starts with actor polling, performs six
priority-ordered controller scans, then always updates and draws the menu.

* Baseline: six `goto tail` sites, 0x1d0 bytes, 152 instructions, 12 calls,
  31 branches, one return, 18 relocations, 100%.
* Negative control: replace each hit with `break` and nest the next scan under
  `c == n`. The source is correct but VC5 retains new comparisons: 0x1dd bytes,
  76.710526%.
* Exact form: move all six scans into the private inline
  `CMenuState::HandleControllerInput`; use an early `return` after each
  action; call it immediately before the update/draw tail. `/Ob1` expands it to
  the original 0x1d0-byte function at 100%, with no helper symbol. All 36
  compared `menustate` functions retain their baseline scores.

`CSplashState::Render` 0x0f9920 has the two-condition form "pressed button OR
countdown expired":

```cpp
if (!IsAdvanceRequested() && m_splashCountdownMs) {
    return 1;
}
PostTransition();
```

The private inline `CSplashState::IsAdvanceRequested` method keeps the
byte-width actor test and folds its hit return into the transition edge. The
result stays exact at 0x108 bytes; every compared `splashstate` symbol is
unchanged.

`CMoviePlayer::Frame` 0x17caa0 is the lifetime-sensitive form. A direct `break`
keeps the Lock result live and scores 95.2%, while an `if` plus `do/while` peels
the retry body and scores 74.6%. Moving the complete Lock/Restore/decode phase
into the private inline `CMoviePlayer::DecodeFrame` lets restore failure return
from the expanded method. The result remains exact at 0x13b bytes, and every
compared `ddpagemgr` symbol is unchanged. This corrects the stronger "goto is the only
spelling" reading of
[retry-loop-bail-while-goto-no-peel.md](retry-loop-bail-while-goto-no-peel.md):
it is the only matching spelling when the retry loop stays directly in the
caller, but not when a real inline phase boundary is considered.

`CGruntVoice::UpdateIndicator` 0x11a8e0 is the shared-failure form. Its two
positioning modes have distinct success exits, but three lookup failures enter
one indicator-hide tail. Extracting one private inline method per mode lets
the failed lookup paths return `false`; the caller retains two positive success
returns and owns the single hide block. The three `goto stopped` sites disappear
while the function stays exact at 0x198 bytes, and neither method emits a
symbol. Declaring the private methods also moves the unchanged `CVoiceTrigger`
constructor from 92.4417% to 98.25% through header/TU state; no other
`gruntvoice` function score moves. Using one method for both modes would erase
source-visible mode boundaries, so it was not the tested shape.

`CStatusBarMgr::ActivateSlot` 0x10b930 combines a shared-failure tail, an
auto-slot search, and two large duplicated success bodies. A private
`CStatusBarMgr::ActivateReadySlot` predicate owns all three parts when `-1`
means "find the first ready slot": a normal `for`/`break` scan returns `false`
on exhaustion, then the method performs the cursor-frame test, the
already-attested nested `HiCueTimed` inline call, and the slot
state/notification writes. The sentinel arm in the caller directly returns the
method's `b32`. This removes the three `goto notActivated` sites, the
`goto slotFound` site, and the duplicated body without adding nesting or an
out-of-line symbol.

The fully structured form deliberately trades current byte identity for humane
source while retaining the historical 100% proof. It keeps retail's exact
0x1a7-byte extent, eight calls, and all 17 ordered relocation targets, but emits
133 rather than 137 instructions, 18 rather than 17 branches, and three rather
than four returns, scoring 91.77037%. A separate scan helper recovered retail's
scan prefix and distinct exhaustion return, but made C2 merge the two
notification success tails and dropped one call. Reference versus pointer
output, helper declaration/definition order, named results, direct returns,
notification wrappers, guard polarity, and `for`/`while`/infinite-loop
spellings did not close the residue. The old label remains evidence for VC5's
exact exit factoring, not a reason to retain the last source-level `goto`.

## Reverse-use checklist

Use this hypothesis only when the label marks the end or failure of a cohesive
operation:

1. The destination is either the statement immediately after that operation or
   a caller-owned state update. It is not ownership rollback, a switch-table
   identity target, or multi-step resource cleanup.
2. Every jump can become a helper return without returning from the outer API.
3. Keep widths, local reuse, scan priority, call order, and lazy member/global
   loads unchanged inside the helper.
4. Confirm the helper is fully expanded: call-set equality alone is insufficient;
   no out-of-line helper symbol may remain.
5. Compare the entire containing TU because adding a helper can perturb C1 state
   outside the edited function.

Do not apply this to source-attested `goto fail` cleanup, partial exit-merging,
or `default: goto tail` jump-table preservation. A search hit with a distinct
loop-exhaustion exit needs separate proof: a sentinel helper can be clear and
correct, but VC5 may not preserve the label form's exact exit factoring.
