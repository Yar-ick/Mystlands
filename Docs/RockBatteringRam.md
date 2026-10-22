# Rock battering ram

`GA_Player_RockBatteringRam` is a defaults-only Blueprint child of the native
`UGA_Player_RockBatteringRam`. Its graphs are empty and it declares no Blueprint
variables. Tune the inherited **Ram** categories in Class Defaults: ground movement,
damage and knockback, dive height and speed, impact limits, gameplay effects, VFX,
sounds, camera shakes, and FOV.

Scalable values retain `CT_RockBatteringRam`. The native ability reads the existing
player `RockBatteringRamAttributeLevel` struct for individual upgrade levels, with
the GAS ability level as a fallback. Ground damage is elapsed rush time multiplied
by `BaseGroundDamagePerSecond` (15 by default) and the scalable upgrade
`GroundDamageMultiplier` (1 at the first upgrade level). Air values scale with activation height in meters and
are clamped to the configured limits. Steering is expressed in degrees per second.
The minimum dive height defaults to 200 cm.

To stop either mode, use `SendGameplayEventToActor` on the player:

- Event tag: `Ability.Skill.RockBatteringRam`
- Payload `InstigatorTags`: `Ability.Skill.Cancel`

The ability's native `WaitGameplayEvent` task listens on the avatar's ASC. The cancel
command is carried in `InstigatorTags` because GAS replaces the payload's `EventTag`
with the routed event tag. Both tags are declared in `GameplayTags.h/.cpp`.

`AC_Player_SkillSystem.CancelAbility` uses this event. Player steering reads the
existing horizontal input value; neither the player nor the skill system stores a
ram instance. Cooldown debugging queries active effects through
`UIGGameplayAbility::GetCooldownTimeRemainingForActor`.

Electric Blast, Rock Armor scattering, and Vine Trap wall combinations are absent
from the ram. The unused `BP_ElectricBlastSphere` asset is removed. Electric Jump's
independent `RockScattering` feature is retained.

Run `Automation RunTests Mystlands.Abilities.RockBatteringRam` in the editor console
to check lifecycle behavior, player/skill-system Blueprint integration, and animation
state transitions after a collision-free timeout, both stopped and still moving.
