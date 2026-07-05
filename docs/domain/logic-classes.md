# WWD `Logic:` names → our `CUserLogic` / `CTileLogic` classes

Every WWD object carries a `Logic:` string (the `editor/*.html` "Logic pages"). At load
the engine maps that string to a concrete `CUserLogic`-derived leaf. This is the
game-object hierarchy our reconstruction already models; the table below ties the
community docs to our classes and to the `LogicTypeId` tag (the 6-byte `GetTypeTag`
accessor, `include/Gruntz/LogicTypeId.h`).

Hierarchy: `CUserLogic` → `CTileLogic` → the leaves. `GetTypeTag` returns a unique
per-leaf id the engine's logic dispatch keys off.

| Site `Logic:` page | Our class | Defined in | `LogicTypeId` tag |
|---|---|---|---|
| `CoveredPowerUp` | `CCoveredPowerup` | `src/Gruntz/UserLogic.cpp` (+ `CCoveredPowerupLogic` factory, `TileTriggerFactory.cpp`, `??0…@0x112240`) | — |
| `InGameIcon` | `CInGameIcon` | `include/Gruntz/InGameIcon.h` | — |
| `GruntStartingPoint` | `CGruntStartingPoint` | `src/Gruntz/GruntStartingPoint.cpp` | — |
| `GruntCreationPoint` | `CGruntCreationPoint` | `src/Gruntz/GruntCreationPoint.cpp` | — |
| `GruntPuddlez` | `CGruntPuddle` | (roster) | — |
| `Teleporterz` | `CTeleporter` | `include/Gruntz/Teleporter.h` | `LOGIC_TELEPORTER 0x3fc` |
| `WarpstonePad` / wormhole | `CWarpStonePad`, `CWormhole` | `UserLogic.cpp`, `include/Gruntz/Wormhole.h` | — |
| `RainCloud` | `CRainCloud` | `src/Gruntz/GameObjectCtors.cpp` | — |
| `UFO` | `CUFO` | `src/Gruntz/GameObjectCtors.cpp` | — |
| `WayPoint` | `CWayPoint` | `include/Gruntz/WayPoint.h` | `LOGIC_WAYPOINT 0x420` |
| `ObjectDropper` | `CObjectDropper` | `include/Gruntz/ObjectDropper.h` | `LOGIC_OBJECTDROPPER 0x40f` |
| `Spotlight` | `CSpotLight` | `src/Gruntz/SpotLight.cpp` | — |
| `GiantRock` | `CGiantRock` | `src/Gruntz/UserLogic.cpp` | — |
| `Brickz` | `CBrickz` | `include/Gruntz/CBrickz.h` | `LOGIC_BRICKZ 0x409` |
| `ExitTrigger` | `CExitTrigger` | `include/Gruntz/ExitTrigger.h` | `LOGIC_EXITTRIGGER 0x3f7` |
| `CheckPointTrigger` | `CCheckpointTrigger` | `include/Gruntz/CheckpointTrigger.h` | — |
| `SecretLevelTrigger` | `CSecretLevelTrigger` | `include/Gruntz/SecretLevelTrigger.h` | — |
| `TileTrigger` | `CTileTrigger` | (roster) | — |
| `VoiceTrigger` (Appendix D) | `CVoiceTrigger` | `include/Gruntz/VoiceTrigger.h` | `LOGIC_VOICETRIGGER 0x426` |
| `FortressFlag` | `CFortressFlag` | `include/Gruntz/FortressFlag.h` | `LOGIC_FORTRESSFLAG 0x427` |
| `GuardPoint` | `CGuardPoint` | `include/Gruntz/GuardPoint.h` | `LOGIC_GUARDPOINT 0x42a` |
| `StaticHazardz` | `CStaticHazard` | `include/Gruntz/StaticHazard.h` | `LOGIC_STATICHAZARD 0x416` |
| `Hazardz` (path) | `CPathHazard` | `src/Gruntz/UserLogic.cpp` | `LOGIC_PATHHAZARD 0x425` |
| `ToobSpikez` | `CToobSpikez` | `include/Gruntz/ToobSpikez.h` | `LOGIC_TOOBSPIKEZ 0x418` |
| `BehindCandy(Ani)` / EyeCandy | `CBehindCandy(Ani)`, `CEyeCandy(Ani)` | `include/Gruntz/{BehindCandy,EyeCandy}.h` | `LOGIC_BEHINDCANDY 0x3f0`, `…ANI 0x3f3`, `LOGIC_EYECANDY 0x3f1`, `…ANI 0x3f4` |
| `RollingBallz` | `CRollingBall` | (roster) | — |
| `LevelTime` | `CLevelTime` | (roster) | — |
| (ActionArea) | `CActionArea` | `include/Gruntz/ActionArea.h` | `LOGIC_ACTIONAREA 0x423` |
| (Particlez) | `CParticlez` | (roster) | `LOGIC_PARTICLEZ 0x41c` |

## The per-Grunt indicator sprites (also `CTileLogic` leaves)

The status sprites floating over a Grunt are logic leaves too — several already carry a
`LogicTypeId`:

| Indicator | Class | `LogicTypeId` |
|---|---|---|
| Stamina bar | `CGruntStaminaSprite` | `LOGIC_GRUNTSTAMINASPRITE 0x410` |
| Toy-play timer | `CGruntToyTimeSprite` | `LOGIC_GRUNTTOYTIMESPRITE 0x411` |
| Wingz-flight timer | `CGruntWingzTimeSprite` | `LOGIC_GRUNTWINGZTIMESPRITE 0x417` |
| Health bar | `CGruntHealthSprite` | — |
| Powerup indicator | `CGruntPowerupSprite` | — (act-handler table over `[2000,2010]`) |
| Selected marker | `CGruntSelectedSprite` | — |
| Toy indicator | `CGruntToySprite` | — |

## Notes / worklist

- Tags for `CoveredPowerup`, `InGameIcon`, `GruntStartingPoint`, `GruntCreationPoint`,
  `RainCloud`, `UFO`, `SpotLight`, `GiantRock`, `SecretLevelTrigger`, `CheckpointTrigger`,
  `WarpStonePad`, `Wormhole` are **not yet in `LogicTypeId.h`** — pin their `GetTypeTag`
  RVAs and add the enumerators (matching-neutral; each is a `mov eax,<id>; ret`).
- The unpinned `LOGIC_TAG_3EF/41D/428/429` tags in `LogicTypeId.h` (owner classes
  COMDAT-folded) are candidates to attribute to the still-tagless Logic pages above.
- The `Logic:` string → factory dispatch is the WWD loader's class-registry
  (`src/Gruntz/TileTriggerFactory.cpp`, `WwdObjMgrFactories.cpp`) — a good place to
  confirm each string maps to the class in this table.
</content>
