# Claw Design Bible names relevant to Gruntz

The 100-page *Claw Design Bible* by Kevin Hawkins contains literal registration
code on PDF pages 92–95, not merely prose. The four pages contain 140
registrations through the `ADD`, `ADD_SMALL`, `ADD_BIG`, and `ADD_MSL` macros.
That is direct evidence that Monolith developers represented object factories
with compact registration macros in this engine family.

The following names occur both in that literal and Gruntz's retail registry:

| Claw literal | Gruntz registry |
|---|---|
| `AniCycle` | `AniCycle` |
| `SingleFrameMessage` | `SingleFrameMessage` |
| `DoNothing` | `DoNothing` |
| `BehindCandy` | `BehindCandy` |
| `FrontCandy` | `FrontCandy` |
| `SimpleAnimation` | `SimpleAnimation` |
| `MenuSparkle` | `MenuSparkle` |

Claw spells two animated variants `BehindAniCandy` and `FrontAniCandy`; Gruntz
contains `BehindCandyAni` and `FrontCandyAni`. This is evidence of name
evolution across titles, not permission to rename the Gruntz classes.

`GameObjectLogicTypes.cpp`'s current literal registry is already exact at
1.00 RVA `0xa3b0`. The design bible strengthens the macro/inline prior for
future reconstruction but does not justify perturbing exact source. Nor do
Claw's macro categories translate directly to Gruntz's numeric flags:
`AniCycle`, `DoNothing`, `BehindCandy`, and `FrontCandy` use `ADD_SMALL` in
Claw, while `SimpleAnimation` and `MenuSparkle` use `ADD`; all of those current
Gruntz registrations use flag 2.

Source: Kevin Hawkins, *Claw Design Bible*, pp. 92–95,
<https://www.gamedevs.org/uploads/monolith-claw.pdf>. Audited PDF SHA-256:
`5a294b842714e04a8775777243ea560d358c705725a827932ed99b28833bdd59`.
