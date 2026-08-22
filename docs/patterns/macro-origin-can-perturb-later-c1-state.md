# Macro-origin metadata can perturb later C1 state even when the expansion is token-equivalent

- **confidence** c10
- **tags** `cpp:macro` `cpp:inline` `cpp:statement` | `asm:call` | `topic:codegen-idiom` `topic:front-end-state` `topic:negative-control`

## Symptom

Replacing a repeated statement body with a macro leaves that function exact, but a
later function in the same TU changes its inline/call set. Removing an optional
semicolon from the macro invocation does not restore the later function, even though
the visible expanded statements are otherwise the same.

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

## Interpretation and reverse use

The exact internal C1 mechanism is not recovered. The controlled result implies that
macro-expansion origin or associated source-location state reaches the front-end handle
stream even when C2 receives an equivalent local statement body.

Use a macro as a code-preserving fallback only after checking later functions in the
same TU. If a sensitive later caller moves, keep the earlier sites textual or place the
macro-backed population after that caller when ownership and source order already allow
it. Never add inert macro uses to steer codegen: this pattern is a warning and a
localization tool, not permission for disposable TU-state probes in retained source.

The positive control is 49 sprite-retirement expansions across seven Grunt TUs: after
the six pre-`StepArrivalDrop` sites were excluded, the macro form introduced no fresh
MAX regression.
