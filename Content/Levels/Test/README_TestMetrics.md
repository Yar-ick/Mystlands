# L_Test movement lab

Open `/Game/Levels/Test/L_Test` and Play from PlayerStart. The original map is preserved at `/Game/Levels/Test/Archive/L_Test_BeforeMetrics`.

The arena is 140 x 110 m. Its safety floor is at world Z = 100 cm. Height labels are relative to that floor. Actors are organized under `Metrics` in the Outliner and prefixed `TEST_` for easy editing.

| Area | Measurements | Use |
| --- | --- | --- |
| Speed and distance | Two 50 m lanes; ticks every 5 m | Walking, strafing, acceleration, stopping, ability travel distance |
| Step height | 0.20, 0.40, 0.45, 0.50, 0.80 m | Compare stepping and jumping across the current step threshold |
| Stairs | 0.20 m risers; 0.35 m treads | Repeated step transitions and camera stability |
| Clearance gates | Widths 0.60, 0.80, 1.00, 1.50 m; respective clear heights 1.60, 1.80, 2.00, 2.40 m | Capsule fit, low ceilings, camera obstruction |
| Slopes | 15, 30, 44, 46, 60 degrees; 10 m horizontal run | Walking near and beyond the current slope limit |
| Jump gaps | 1, 2, 3, 4, 5 m; 16 m run-ups; 12 m landing pads | Standing/running jumps, air control, edge cases |
| Jump-up blocks | 0.50, 1.00, 1.40, 1.60, 2.00 m | Maximum jump height and landing tolerance |
| Vertical course | Platforms +3, +6, +10, +16 m | Vine-hook attraction, boosting, upward aiming, descending and landing |
| Walking access | Four stair flights to +16 m, 0.20 m risers and 0.40 m treads | Repeat high-to-low tests without needing a successful hook sequence |

Jump-gap run-ups have individual access stairs with 0.20 m risers and 0.40 m treads. Missed jumps land on the safety floor. The two existing hook actors are retained and repositioned; four additional attractors and two additional boosters use the existing Blueprint assets and their default behavior.

## Current BP_Player baseline

- Walk speed: 600 cm/s (6 m/s).
- Step height: 45 cm.
- Walkable floor angle: 45 degrees.
- Capsule: radius 35 cm, half height 90 cm (0.70 m wide, 1.80 m tall).
- Jump Z velocity: 700 cm/s; gravity scale 1.75; air control 0.35.
- Constant-speed reference: 10 m in 1.67 s, 50 m in 8.33 s. These are reference calculations, not automatic timing telemetry.

Green step surfaces are at or below the current step limit. Orange slopes are below the walkable-angle limit. Red surfaces indicate boundary cases beyond a baseline limit. Purple identifies jump tests, cyan identifies movement lanes and the vertical course.

## Suggested checks

1. Walk both speed lanes in both directions; compare acceleration, braking, strafing and ability distance against the tick marks.
2. Approach each step without jumping, then jump. Check the 45/50 cm transition.
3. Try each gate and watch capsule clearance and camera movement.
4. Walk up and down each slope; compare 44 and 46 degrees.
5. Try each gap from standing and from the full run-up. Compare the jump-up blocks separately.
6. Traverse the vertical platforms with vine hooks and boosters. Use the north stairs to repeat high-to-low landings.

## Verification performed

- Saved and reloaded L_Test and its supporting assets.
- Checked the five step heights, ramp slope, four platform heights, clear gap geometry and stair connector heights through world traces.
- Started Play-In-Editor and confirmed BP_Player spawned in walking mode on the Z = 100 cm safety floor.
- In PIE, placed the player over the highest platform and confirmed a stable walking floor hit at Z = 1700 cm, with no capsule penetration.
- Decorative markings use a separate cube mesh with no collision shapes, so they do not change the movement surfaces. Confirmed the spawn-pad marking does not affect the player's floor collision.

Full input-driven traversal of every gap and hook sequence remains a playtest. Static navigation bounds cover the enlarged arena; rebuild navigation in the editor when changing geometry for AI tests.
