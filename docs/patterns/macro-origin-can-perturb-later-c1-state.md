# Macro-origin metadata can perturb later C1 state even when the expansion is token-equivalent

- **confidence** c10
- **tags** `cpp:macro` `cpp:inline` `cpp:statement` | `asm:call` | `topic:codegen-idiom` `topic:front-end-state` `topic:negative-control`

## Symptom

Replacing a repeated statement body with a macro leaves that function exact, but a
later function in the same TU changes its inline/call set. Removing an optional
semicolon from the macro invocation does not restore the later function, even though
the visible expanded statements are otherwise the same. A header which only defines
the candidate macros can also perturb the first function below it: the definition
boundary itself must be scoped like any other front-end-state input.

## Controlled evidence

`CGrunt::ClearAllSprites` contains six copies of the sprite-retirement body:

```cpp
if (m_selectedSprite) {
    m_selectedSprite->m_flags |= 0x10000;
    m_selectedSprite = NULL;
}
```

Replacing all six with `HIDE_AND_CLEAR_GRUNT_SPRITE(member)` kept
`ClearAllSprites` at 100.000%. It nevertheless moved the later
`CGrunt::StepArrivalDrop` from 32.302708% to 0.000%. Removing the invocation
semicolons did not change that result. Moving the macro uses below
`StepArrivalDrop`, while retaining the same macro for four later sites in the TU,
restored 32.302708% and the MAX gate.

The downstream divergence was an inline/call-set change: retail retains four
`CPtrList::RemoveHead` calls, while the perturbed base retained three. The earlier
function's emitted bytes are therefore not a sufficient control for a macro rewrite.

The exhaustive inline campaign supplied a second control. After the affected sites
were restored to their previous direct spelling, these functions remained below
their banked score solely while the new definition header still preceded them:

- `CGrunt::StepArrivalDrop`: 32.302708% to 0.000%, with the coordinate, movement,
  and sort-key macro headers at the top of `Grunt.cpp`;
- `CSpriteRef::Build`: 100.0000% to 99.8157%, with `PixelFormatMacros.h` included
  but no pixel macro left in the function;
- `CGrunt::StepDiggerBehavior`: 83.8685% to 83.8486%, after every new invocation in
  the function had been restored;
- `CDDSurface::ShadeRect`: 76.9909% to 76.9636%, while the pixel-format definitions
  and their earlier uses preceded it.

Removing the unused definition headers in the one-function cases, and moving the
required definitions below `StepArrivalDrop` and `ShadeRect`, restored all four
banked scores in the same full build. Later macro-backed sites remained compiled and
the MAX gate returned to zero fresh regressions.

A 2026-08-30 reconstruction of the `StepArrivalDrop` control narrowed the required
composition further. Moving only the coordinate-recycle, movement, sort-key, and
tile-coordinate definition headers back below the function left it at 0.0000% and
still omitted the fourth emitted `CPtrList::RemoveHead` site. Restoring the two later
powered-state and sprite definition boundaries as well returned the unchanged body to
32.2933%. The perturbation therefore belongs to the cumulative C1 definition-boundary
state, not to any one macro family; when reversing a hoist, restore the complete
historical boundary population before judging the A/B.

## Interpretation and reverse use

The exact internal C1 mechanism is not recovered. The controlled result implies that
macro-expansion origin or associated source-location state reaches the front-end handle
stream even when C2 receives an equivalent local statement body.

Use a macro as a code-preserving fallback only after checking the function containing
the first definition boundary and every later function in the same TU. If a sensitive
caller moves, keep the earlier sites textual or place both the definition boundary and
the macro-backed population after that caller when ownership and source order already
allow it. Never add inert macro uses to steer codegen: this pattern is a warning and a
localization tool, not permission for disposable TU-state probes in retained source.

The positive control is 49 sprite-retirement expansions across seven Grunt TUs: after
the six pre-`StepArrivalDrop` sites were excluded, the macro form introduced no fresh
MAX regression.

## Retained structural exceptions

The definition-against-every-function campaign found two cases where moving the macro
boundary cannot be separated from the proven relationship:

- `CDDrawWorkerHost::Build` has a 100.000% standalone COMDAT, two expanded bodies in
  `Read` and `InitGeometry`, and two real retail calls from `CGameLevel`. A canonical
  member inline expands all four uses and removes the COMDAT. The two-entity macro form
  preserves those shapes but moves the later `CDDrawWorkerHost::Draw` from 85.6551% to
  81.5202%.
- `CGameLevel::ReleaseChildren` remains 100.000% while its consolidated expansion moves
  `Unload` from 100.0000% to 99.8824%. Calls, branches, referents, and semantic store set
  are unchanged; only the independent-store schedule differs.

These do not license inert steering. They narrow the reverse-use rule: first try a real
inline, exact-expansion macro, and source-order scoping; if the mixed retail call/expansion
shape still requires the boundary, retain a humane structural macro only with the
downstream effect measured, documented, historical MAX preserved, and the bank explicitly
adjudicated. A useful abstraction need not be duplicated merely to protect current fuzzy.

A third A/B demonstrates why macro fallback remains the preferred final lever.
`CWwdSpriteObject::SetAnimation` is standalone at 100.000% and fully expanded inside
the exact `SetAnimationByName`. A narrow real inline kept both exact but moved the later
exact `CDDrawWorker::GetMemoryUsage` to 99.9623%; the token-preserving macro restored all
three to 100.000%.
