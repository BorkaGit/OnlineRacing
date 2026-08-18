# OnlineRacing

OnlineRacing is a compact Unreal Engine 5.8 C++ portfolio project: a 2-4 player arcade race built on Chaos Vehicles and a listen-server architecture.

The goal is a finished vertical slice that demonstrates Unreal C++, multiplayer gameplay architecture, vehicle telemetry, and vehicle audio. Core gameplay and networking live in C++; Blueprints are used for asset setup, UI layout, animation, tuning, and presentation.

## Current milestone

The project currently has a playable race foundation with a server-controlled AI opponent and a functional first vehicle-audio pass:

- one Chaos Vehicles sports car with Enhanced Input;
- a project-specific race map at `/Game/Maps/Lvl_Race`;
- replicated race phases: `Waiting`, `Countdown`, `Racing`, and `Finished`;
- a server-timed countdown synchronized through `GameState`;
- vehicle input locking until the race starts;
- ordered checkpoints and lap progression validated by the server;
- server-authoritative respawn at the last confirmed checkpoint;
- authoritative finish times and positions assigned in crossing order;
- replicated per-player race progress and finish data stored in `PlayerState`;
- a replicated results snapshot stored in `GameState`;
- deterministic indexed grid starts shared by human and AI racers;
- a server-spawned Chaos vehicle AI that follows a closed driving spline, predicts upcoming turns, controls speed, and requests recovery when stuck;
- vehicle telemetry for speed, RPM, gear, inputs, wheel contact, slip, skid, and the dominant physical surface;
- MotoSynth engine audio driven by smoothed Chaos RPM and throttle-derived load;
- idle, tire roll, drift, handbrake, wind, gear-shift, and collision audio layers;
- a debug HUD with telemetry, race state, network roles, net mode, and ping;
- PIE listen-server gameplay with multiple players.

![Gameplay](assets/gameplay.gif)

The original Vehicle Template time-trial variant remains in the repository as reference content. The multiplayer race uses separate project-specific C++ classes and assets.

## Not implemented yet

- Online Subsystem Null session creation, discovery, join, leave, and travel;
- Steam lobbies and friend invitations;
- surface-dependent tire sounds; physical-material detection is already implemented;
- audio values in the debug HUD and an Audio Insights profiling pass;
- final results-screen and HUD presentation polish;
- systematic network-emulation validation of Chaos vehicle control;
- automated race-rule tests;
- packaged-build validation and demonstration video.

## Architecture

| Type | Responsibility |
| --- | --- |
| `AOnlineRacingPawn` | Owns the Chaos vehicle, cameras, local vehicle input, input locking, and authoritative teleport/reset operations. |
| `UOnlineRacingVehicleTelemetryComponent` | Reads post-physics Chaos state and exposes a stable game-thread telemetry snapshot. It does not own race rules. |
| `UOnlineRacingVehicleAudioComponent` | Converts local vehicle telemetry into MotoSynth and MetaSound parameters for engine, tire, wind, transmission, and impact presentation. |
| `AOnlineRacingAIController` | Drives a server-controlled Chaos vehicle using spline targets, upcoming-turn prediction, speed control, and stuck recovery. |
| `AOnlineRacingDrivingLine` | Owns the closed spline used as the AI racing trajectory and debug reference. |
| `AOnlineRacingMatchGameMode` | Owns server-only race orchestration, checkpoint validation, lap progression, countdown start, finish timing and ordering, and respawn selection. |
| `AOnlineRacingMatchGameState` | Replicates public race phase, synchronized timing, race configuration, and the ordered results snapshot. |
| `AOnlineRacingMatchPlayerState` | Replicates per-player lap, expected checkpoint, last confirmed checkpoint, finish position, and finish time. |
| `AOnlineRacingCheckpoint` | Defines an ordered overlap volume and a respawn transform; reports vehicle crossings to server race logic. |
| `AOnlineRacingGridStart` | Marks a deterministic indexed grid position available to a human or AI controller. |
| `AOnlineRacingPlayerController` | Coordinates local UI and input state and sends respawn requests to the server. |
| `UOnlineRacingCountdownWidget` | Presents countdown values and `GO`; it never starts or advances the race. |
| `UOnlineRacingResultsWidget` | Presents replicated finish positions and times; Blueprint may replace the fallback text with styled rows. |
| `UOnlineRacingDebugWidget` | Presents telemetry and replicated networking/race state for development. |

### Authority model

- `GameMode` exists only on the server and is the source of truth for race progression.
- Clients cannot submit lap counts, checkpoint indices, finish state, or respawn transforms.
- A checkpoint crossing is accepted only during the `Racing` phase and only when its index matches the player's expected checkpoint.
- Finish time is calculated from synchronized server timestamps; the server assigns each finishing position exactly once.
- The server selects the respawn transform from the player's last validated checkpoint.
- Shared race state is replicated through `GameState`; persistent player progress is replicated through `PlayerState`.
- UI reads state and sends requests but never decides gameplay outcomes.

### Race flow

```text
Map BeginPlay
    -> discover and validate checkpoint and grid-start indices
    -> spawn the configured server-controlled AI racers
    -> initialize GameState and PlayerStates
    -> Countdown (server end timestamp is replicated)
    -> Racing (vehicle input enabled, checkpoints accepted)
        -> record each racer's server finish time and position
    -> Finished (after all active racers finish)
```

## Vehicle telemetry

`UOnlineRacingVehicleTelemetryComponent` ticks in `TG_PostPhysics` so its snapshot is updated after the Chaos vehicle simulation for the frame.

Available values:

- absolute forward speed in km/h;
- engine RPM and normalized RPM;
- current gear;
- throttle, brake, steering, and handbrake input;
- number of wheels in contact;
- wheel slipping and skidding flags;
- maximum slip and skid magnitudes in cm/s.
- dominant physical surface selected from the contacting wheel with the highest suspension force.

Telemetry is presentation data for UI, debugging, AI observation, and audio. It is not used as authoritative proof of race progress.

## Vehicle audio

`UOnlineRacingVehicleAudioComponent` ticks after vehicle telemetry and owns the local presentation path. Audio is generated independently on each machine and is not replicated as gameplay state.

Implemented layers:

- MotoSynth RPM mapped logarithmically from the configured Chaos engine range;
- smoothed throttle-derived engine load controlling volume and low-pass filtering;
- a MetaSound vehicle loop containing speed-dependent idle pitch and level, tire roll, drifting, handbrake skids, and wind;
- one-shot upshift audio;
- logarithmically scaled collision impacts with a per-vehicle cooldown.

Physical surfaces currently affect vehicle handling through their friction settings. Surface-dependent tire sound is intentionally deferred so it does not delay multiplayer and session work.

### Deferred audio TODO: physical surfaces

- keep `EPhysicalSurface` detection in vehicle telemetry as the presentation input;
- add independent smoothed MetaSound weights rather than a two-surface blend parameter;
- validate Asphalt and Gravel first, then extend the same path to Grass, Mud, and Ice if suitable recordings are available;
- apply surface selection to tire-roll and skid layers without changing engine or wind audio;
- verify transitions at surface boundaries and profile the additional looping voices in Audio Insights.

## Content

| Asset | Purpose |
| --- | --- |
| `/Game/Maps/Lvl_Race` | Current multiplayer race map and default editor/game map. |
| `/Game/Core/BP_RaceGameMode` | Blueprint defaults for the C++ race mode. |
| `/Game/Core/BP_GridStart` | Indexed grid-start marker used by human and AI racers. |
| `/Game/Core/BP_RacingLine` | Closed spline that defines the AI driving line. |
| `/Game/AI/BP_RacingAIController` | Tuned Blueprint defaults for the C++ vehicle AI controller. |
| `/Game/CheckPoints/BP_RaceCheckpoint` | Configurable checkpoint presentation and collision asset. |
| `/Game/UI/WBP_DebugHUD` | Debug telemetry and networking display. |
| `/Game/UI/WBP_RaceCountdown` | Countdown and race-start presentation. |
| `/Game/UI/WBP_RaceResults` | Replicated race-results presentation. |
| `/Game/Audio/Vehicles/Metasounds/SportsCar/MS_Veh_Sport_Ip` | Persistent MetaSound vehicle loop used alongside MotoSynth. |

## Running the project

### Requirements

- Unreal Engine 5.8 with the Chaos Vehicles plugin;
- Visual Studio 2022 with Desktop development with C++ and Game development with C++ workloads;
- Windows 64-bit toolchain.

### Setup

1. Clone the repository.
2. Generate project files from `OnlineRacing.uproject` if required.
3. Build `OnlineRacingEditor` using `Development Editor | Win64`.
4. Open `OnlineRacing.uproject`.
5. Open `/Game/Maps/Lvl_Race` if it is not already loaded.

### Multiplayer PIE test

In the Play dropdown, open Multiplayer Options and use:

- Number of Players: `2`;
- Net Mode: `Play As Listen Server`;
- separate server and client windows.

Expected behavior:

1. Both vehicles are held in place during the countdown.
2. Driving input becomes active at `GO`.
3. Checkpoints crossed before `Racing` are ignored.
4. Checkpoint `0` acts as the start/finish line; after spawning, racers must cross `1`, continue in contiguous index order, and return to `0` to complete a lap.
5. Reset respawns the vehicle at its last server-confirmed checkpoint.
6. A finished racer's driving input is disabled while the remaining racers continue.
7. After all racers finish, every client receives the same ordered results table.
8. The debug HUD shows the local race state, progress, finish data, telemetry, roles, and ping.

For meaningful network testing, use separate server/client worlds and PIE network emulation. Split-screen alone is not sufficient validation.

## Roadmap

1. **Close the current audio pass** - expose the implemented audio values in the debug HUD and run an Audio Insights profiling pass. Surface-dependent tire sounds remain a deferred TODO.
2. **Validate local multiplayer** - test 2-4 racers with AI, late joins, disconnects, listen-server behavior, and Chaos movement under network emulation.
3. **Online Subsystem Null** - implement session create/find/join/leave and listen-server travel in a `GameInstanceSubsystem`.
4. **Steam** - reuse the session flow for Steam lobbies, joining, and friend invitations.
5. **Race-rule tests** - add focused automation tests for checkpoint sequencing, lap completion, finish ordering, and invalid progress.
6. **Final polish** - finish results/HUD presentation, remove noisy debug defaults, and profile game, network, and audio paths.
7. **Portfolio delivery** - validate a packaged build, document architecture and known limitations, and record a short demonstration video.

## Scope boundaries

The vertical slice intentionally excludes open-world systems, multiple vehicle classes, economy, garage progression, complex tuning, dedicated servers, full matchmaking, PCG tracks, and vehicle destruction.

## Repository policy

Source-of-truth project files under `Source/`, `Config/`, and `Content/` are tracked. Generated folders such as `Binaries/`, `DerivedDataCache/`, `Intermediate/`, and `Saved/` are excluded by `.gitignore`.

Unreal binary assets should use Git LFS before the repository is distributed extensively.
