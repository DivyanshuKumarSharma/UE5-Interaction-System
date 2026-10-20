# Case Study: A Drop-in, Server-Authoritative Interaction System (UE 5.6, C++)

> Draft. Update the "Problems" section with anything found during editor and multiplayer testing.

## What I built

A reusable interaction framework for Unreal Engine 5.6, packaged as a runtime plugin. Any actor
becomes interactable by implementing one interface. The player gets focus detection, an outline
highlight, a prompt that follows the target (`E  Open Door`), and press or hold-to-interact with a
progress ring. The whole thing is server-authoritative, with RepNotify-driven state, verified with
two players in PIE (listen server and dedicated server).

I shipped it with three C++ examples that show different kinds of interaction (a door that swings
away from whoever opens it, a pickup that feeds an inventory, and a lever that drives other actors
through a second interface) and one Blueprint-only interactable to show designers can extend the
system without C++.

## The challenge

Interaction looks simple, but it sits where input, rendering, UI, physics queries and networking
meet. I wanted something I could drop into another project unchanged, so it had to:

- work for any actor, including ones that already have a parent class or exist only in Blueprint;
- stay cheap: no per-frame traces for every pawn on the server;
- handle per-object hold durations, which Enhanced Input's Hold trigger can't express;
- never trust the client, while staying tolerant of normal latency;
- keep UI, gameplay and game-specific content decoupled.

## Approach

**Interface + component.** `IInteractable` (five BlueprintNativeEvents) defines *what* can be
interacted with; `UInteractionComponent` on the pawn defines *who* interacts. Neither needs a
specific base class, so a Blueprint lamp and a C++ door go through exactly the same code path.

**Cheap, accurate focus.** Only the locally controlled player traces, on a throttled tick that is
enabled and disabled by possession events. A precise line trace runs first, then a small sphere
sweep clamped to the line's hit point, so aim is forgiving but can't reach through walls. A
dedicated trace channel gives level designers independent control over what blocks interaction.

**Server authority without punishing lag.** The client sends a request; the server re-checks
availability, reach to the target's bounds with a tolerance, line of sight from the pawn's eyes,
and for holds, that its own clock saw the hold last long enough. Failed checks are logged, not
kicked, because they're almost always latency.

**Minimal replication.** The component replicates nothing. Each interactable replicates only its
outcome. The door sends one float with a RepNotify, and every machine animates locally; doors and
levers stay net-dormant until they change.

**Designer-friendly UI.** A C++ `UUserWidget` base class handles behavior; designers build the look
in a Widget Blueprint via `BindWidget`. The hold ring is a small custom Slate widget that paints
its own arc, so it needs no material and appears in the UMG palette.

## Real problems and how I solved them

1. **The third-person camera broke reach.** Tracing from the camera meant the 400 cm spring arm ate
   most of the interaction range, and objects between the camera and the character could be focused.
   I start the ray where the camera ray passes closest to the pawn instead, so reach is measured from
   the character. In first person that offset is about 0, so the same code works for both.

2. **UHT generated `#include "abCharacter.h"`.** After I added a `Source/InteractionLab/Interaction`
   folder to the public include paths, the build failed in the character's generated code. UHT
   computes a header's include path by plain *string* prefix matching, and `.../Interaction` is a
   string prefix of `.../InteractionLabCharacter.h`, so it stripped "InteractionL". Renaming the
   folder to `Interactables` fixed it. Lesson: when generated code is wrong, read the generated file.

3. **Interface defaults collided with generated ones (C2084).** UHT emits default `_Implementation`
   bodies for interface BlueprintNativeEvents that you don't declare. I declared them explicitly in
   the interface, which also let me choose sensible defaults (e.g. `CanInteract` returns true) so C++
   implementers override only what they need.

4. **Focus on a destroyed actor.** *(Caught in code review.)* If another player collects the pickup
   you're looking at, the weak pointer silently becomes null and the prompt would stay on screen. An
   explicit `bHasFocus` flag distinguishes "nothing focused" from "focused thing was destroyed", so
   listeners always hear focus was lost.

5. **Two players, one pickup, same frame.** *(Caught in code review.)* The second request can
   reference an actor that's already being destroyed. Server validation now rejects actors that are
   invalid or `IsActorBeingDestroyed()`.

6. **A hidden prompt never came back.** *(Caught in code review.)* Hiding the prompt when its anchor
   went off screen would have broken it permanently: Slate doesn't paint Hidden widgets, and UMG
   ticks during paint, so it would never tick again to un-hide. I fade it via `ColorAndOpacity`
   instead.

7. **Per-object hold durations.** Enhanced Input's Hold trigger has one threshold per input asset.
   I moved hold timing into the component (a one-shot timer, plus a polled progress value for the
   UI), driven by `FInteractionData::HoldDuration`.

<!-- Add problems found during editor / PIE testing here. -->

## What I learned

- The value of separating *who interacts*, *what is interactable* and *how it looks*: each part can
  change without touching the others.
- How Unreal routes RPCs (owning connection, net-addressable actors), and why RepNotify fires
  automatically for Blueprint variables but must be called by hand on the server in C++.
- Where soft references actually pay off (UI classes on servers, cross-actor references in World
  Partition) and where they're just noise.
- Practical knowledge of the toolchain: UHT code generation, include paths, and building from the
  command line while keeping Live Coding out of the way.

## What I'd improve

- **Client-side prediction:** play an immediate local cue on press and reconcile with the server.
- **Component-level interactables:** multiple interaction points per actor.
- **Input-device-aware prompts:** use CommonUI to switch key glyphs when a gamepad is used.
- **Automated tests:** functional tests for validation rules (reach, line of sight, hold time) and
  a Gauntlet multiplayer smoke test.
- **Push-model replication / Iris:** mark dirty explicitly instead of polling properties.
- **Gameplay Tags** for interaction types and requirements (e.g. "requires key: Red").
