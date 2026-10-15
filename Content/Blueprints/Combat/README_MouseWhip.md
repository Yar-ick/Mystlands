# Mouse-controlled vine whip prototype

Open `/Game/Levels/Test/L_Test`, press Play, and walk to the three red training targets east of the spawn area (world X = 4000–5000 cm, Y = 1800 cm).

- Hold **LMB** to lock the current camera angle and enter whip control.
- Move the mouse horizontally and vertically to move the right arm and swing the vine.
- Release **LMB** to restore camera control.
- A moving whip can damage each enemy at most once every **0.5 seconds**. Keeping a stationary whip on an enemy does not deal repeated damage.
- Pause and upgrade menus, death, and disabling the prototype release the camera lock.
- Restart PIE to reset destroyed training targets.

## Implementation

The player accumulates mouse yaw and pitch around the locked view. The animation Blueprint uses a Two Bone IK control on `Hand_R` to pose the right arm, with a stable elbow target and a blended return to locomotion.

The player also checks the physical `LeftMouseButton` state each tick, so a shared skill-cancel mapping cannot prevent hold activation. Releasing the button, showing a menu cursor, death, or disabling the prototype ends whip control. The key pin uses the canonical `LeftMouseButton` value.

Normal camera look applies the existing sensitivity coefficient and world delta seconds. The missing time multiplier was restored after the initial prototype caused excessive camera sensitivity.

The player's existing Niagara component, using `/Game/VFX/ParticleSystems/Player/Vine/FXS_VineWhip`, is the weapon's only visual. During mouse control, its existing `Velocity` parameter follows the controlled aim. The usual Niagara behavior resumes when LMB is released.

`BP_MouseVineWhip` provides collision only. Eight invisible swept capsule colliders approximate the whip path from the animated right hand, with a small sag and an interpolated tip. These colliders do not read Niagara particle positions. Capsule overlap callbacks catch swept contacts; continuing contacts are checked each tick.

A cooldown map is kept on the same whip actor across LMB presses, keyed by the enemy's level-unique object name. Angular movement gates damage, and a static-world line-of-sight trace rejects hits through walls. Damage uses the existing `SwordDamage` upgrade curve and GAS damage function, including the existing Thick Hide response.

The cylinder mesh components and their material have been removed. Niagara remains visible during mouse control; no replacement weapon geometry is rendered.

## Tuning

Edit Class Defaults in the following assets:

| Asset | Settings |
| --- | --- |
| `/Game/Blueprints/BP_Player` | `MouseWhipEnabled`, `MouseWhipSensitivity`, `MouseWhipMaxYaw`, `MouseWhipMinPitch`, `MouseWhipMaxPitch` |
| `/Game/Blueprints/Combat/BP_MouseVineWhip` | `HitCooldown`, `WhipLength`, `WhipRadius`, `TipBlendSpeed`, `MinimumAngularSpeed`, `bDrawCollision` |

Defaults: 6 m vine length, 7 cm collision radius, 0.5 s per-enemy cooldown, sensitivity 1.5, yaw ±75 degrees, pitch -55/+65 degrees relative to the locked camera. Enable `bDrawCollision` to show the matching capsules during play.

Set `MouseWhipEnabled` false to use the original LMB animation combo. Normal look input, movement, existing skill assets, and the player's skeleton remain integrated with the existing project.

## Verification

The reusable `/Game/Blueprints/Combat/Tests/BP_MouseWhipTestDriver` injects Enhanced Input actions in PIE. It is saved as a test asset and is not placed in L_Test.

Verified:
- Actual capsule contact reduced an initialized enemy's health from 150 to 130.
- Sixteen repeated hit calls during the cooldown left health at 130.
- Another moving contact after the cooldown reduced health to 110.
- Health remained unchanged after input and aim motion stopped, including after the cooldown expired.
- Camera rotation remained fixed during mouse-driven arm control.
- Releasing the injected LMB action cleared the look lock; subsequent look input rotated the camera normally.
- The right-hand pose points forward. A PIE visual check confirmed that the original Niagara whip stays visible during mouse control, with zero static mesh components and eight capsule components on the collision actor.
- The level and all affected Blueprint assets were saved.

Input-fix regression: an injected look vector `(2, 1)` changed camera yaw by about 1.46 degrees and pitch by about -0.73 degrees, compared with 175 and -87.5 degrees before the fix. The camera stayed locked during injected whip input and unlocked on release. Automated input injection does not exercise the physical mouse button; the physical-button path was checked for its canonical key value and tick wiring.

Blueprint backups from before this prototype are in `Saved/Codex/MouseWhip/Backup_20261014_234733`.

