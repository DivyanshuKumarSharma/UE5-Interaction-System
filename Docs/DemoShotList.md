# 30-Second Demo: Shot List

Capture at 1920x1080, 60 fps. Hide the editor UI (play in a New Editor Window, or Standalone).
Two-player shots: Net Mode *Play As Listen Server*, 2 players, then arrange the windows side by side
and record the whole desktop region.

| # | Time | Shot | On screen | Caption (optional) |
|---|---|---|---|---|
| 1 | 0:00–0:03 | Wide establishing shot of the test level: door, lever, locked door, lamp, coins | Camera slowly turns toward the first door | **Interaction System, UE 5.6 C++** |
| 2 | 0:03–0:07 | Walk up to the door. The outline and the `E  Open Door` prompt appear. Press E; the door swings away from the player. | Outline + prompt anchored above the door | Focus · highlight · prompt |
| 3 | 0:07–0:10 | Walk around to the other side and close it, then open it again from that side; it swings the other way. | Verb flips Open ↔ Close | Swings away from whoever opens it |
| 4 | 0:10–0:14 | Look at the coin pouch. Hold E halfway, release (the ring resets), then hold to completion. The pouch disappears. | Progress ring partial, then full | Hold-to-interact, cancel on release |
| 5 | 0:14–0:18 | Look at the locked door (greyed `Locked Door`, no key shown). Turn to the lever, pull it: the locked door opens and the lamp turns on. | Lever handle flips | Actors talk through interfaces |
| 6 | 0:18–0:21 | Hold E on the Blueprint lamp to turn it off. | Lamp toggles | Blueprint-only interactable, no C++ |
| 7 | 0:21–0:26 | Split screen, two players: client opens a door, the server window sees it swing; server collects a coin, it vanishes for the client. | Two PIE windows side by side | Server-authoritative · RepNotify |
| 8 | 0:26–0:30 | `Interaction.Debug 2` on: trace lines, sweep sphere, server reach sphere and line-of-sight lines. End on a title card. | Debug drawing | `github.com/<you>/InteractionLab` |

## Tips

- Set `r.ScreenPercentage 100` and turn off motion blur in the PostProcessVolume for crisp
  outlines in the recording.
- Pick a high-contrast outline color (warm yellow on the grey prototype grid reads well).
- For shot 7, put the server and client windows close to each other so both doors are in frame.
- Record each shot separately and cut on actions (button press), not on idle moments.
