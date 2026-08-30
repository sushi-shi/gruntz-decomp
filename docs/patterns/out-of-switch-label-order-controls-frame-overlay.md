# Out-of-switch label order controls C1 frame overlay

tags: cpp:goto cpp:local cpp:switch cpp:scope | asm:sub-esp asm:jmp | topic:codegen-idiom topic:regalloc
symptoms: calls, branches, returns and referents agree, but the base frame is larger than retail;
  a switch dispatches with `goto` to disjoint labelled bodies defined later in the function; moving
  declarations or adding scopes inside one body does not recover retail's slot overlay
confidence: 9/10

## The signal

A `switch` whose cases only jump to labelled blocks has two independent source orders:

1. the order of the `case` labels, which shapes the dispatcher; and
2. the physical order of the labelled block definitions, which reaches C1's local census and
   frame-slot packing even when C2 later emits the same branch skeleton.

Do not assume that matching the dispatcher settles the second order. When the labelled bodies own
different aggregate locals, their physical definition order can decide which lifetimes VC5 overlays.

## Calibration

`CGrunt::StepTimeBomberBehavior` (`0x0f60f0`) dispatches `AISTATE_SEEK` to `state0` and
`AISTATE_ATTACK` to `state2`. The reconstruction originally defined `state0` first. After the
authentic `MakeRect` restoration, the base still reserved a `0x5c` frame against retail's `0x44`.
The call set, branch skeleton, returns and ordered referents already agreed.

Moving the complete `state2` definition before `state0`, without changing the case order or either
body, made the frame exact at `0x44`, moved the first divergence from the prologue at `+0x17` to
`+0x72`, and moved fuzzy from about `89.10` to `89.18`. An inner brace around `state2`'s `RECT` and
grid pointer was byte-flat; swapping declarations inside the mirror helper was also byte-flat. The
lever was the order of the complete labelled source regions, not a smaller local spelling.

This is an exploratory-descent pattern. Earlier helper and rectangle restorations changed the local
census without closing the frame by themselves; they were the correct base on which the label-order
change became effective.

## Reverse use

Use this only when retail evidence already supports all of the following:

- a switch or if-chain reaches disjoint blocks through explicit labels;
- the call set and branch/return skeleton agree;
- the frame mismatch is attributable to locals owned by different labelled bodies; and
- retail's physical body order can be identified from its blocks and call sites.

Reorder whole labelled definitions as a disposable A/B, then compare the frame and the first real
divergence. Do not reorder cases merely to perturb C1, and do not split one semantic body into fake
labels. A jump-table switch whose cases contain their bodies directly is the separate
`switch-arm-emission-follows-source-order` pattern.

related: switch-arm-emission-follows-source-order.md,
switch-arm-locals-overlay-only-when-scoped.md,
surviving-source-lineage-restores-typed-layers-and-order.md
