# OnlineRacing

OnlineRacing is a compact Unreal Engine 5.8 C++ portfolio project: a 2-4 player arcade race built on Chaos Vehicles and a listen-server architecture.

The goal is a finished vertical slice that demonstrates Unreal C++, multiplayer gameplay architecture, vehicle telemetry, and vehicle audio. Core gameplay and networking live in C++; Blueprints are used for asset setup, UI layout, animation, tuning, and presentation.

## Current milestone

The project currently has a playable race foundation:

- one Chaos Vehicles sports car with Enhanced Input;
- a project-specific race map at `/Game/Maps/Lvl_Race`;
- replicated race phases: `Waiting`, `Countdown`, `Racing`, and `Finished`;
- a server-timed countdown synchronized through `GameState`;
- vehicle input locking until the race starts;
- ordered checkpoints and lap progression validated by the server;
- server-authoritative respawn at the last confirmed checkpoint;
- replicated per-player race progress stored in `PlayerState`;
- vehicle telemetry for speed, RPM, gear, inputs, wheel contact, slip, and skid;
- a debug HUD with telemetry, race state, network roles, net mode, and ping;
- PIE listen-server gameplay with multiple players.

The original Vehicle Template time-trial variant remains in the repository as reference content. The multiplayer race uses separate project-specific C++ classes and assets.

## Not implemented yet

- finish timestamps, finish order, and a results table;
- Online Subsystem Null session creation, discovery, join, leave, and travel;
- Steam lobbies and friend invitations;
- production vehicle audio;
- surface-dependent tire audio and impact classification;
- automated race-rule tests;
- packaged-build validation and demonstration video.

## Architecture

| Type | Responsibility |
| --- | --- |
| `AOnlineRacingPawn` | Owns the Chaos vehicle, cameras, local vehicle input, input locking, and authoritative teleport/reset operations. |
| `UOnlineRacingVehicleTelemetryComponent` | Reads post-physics Chaos state and exposes a stable game-thread telemetry snapshot. It does not own race rules. |
| `AOnlineRacingRaceGameMode` | Owns server-only race orchestration, checkpoint validation, lap progression, countdown start, finish detection, and respawn selection. |
| `AOnlineRacingRaceGameState` | Replicates public race phase, synchronized countdown end time, lap count, and checkpoint count. |
| `AOnlineRacingRacePlayerState` | Replicates per-player lap, expected checkpoint, last confirmed checkpoint, and finished state. |
| `AOnlineRacingRaceCheckpoint` | Defines an ordered overlap volume and a respawn transform; reports vehicle crossings to server race logic. |
| `AOnlineRacingPlayerController` | Coordinates local UI and input state and sends respawn requests to the server. |
| `UOnlineRacingRaceCountdownWidget` | Presents countdown values and `GO`; it never starts or advances the race. |
| `UOnlineRacingDebugWidget` | Presents telemetry and replicated networking/race state for development. |

### Authority model

- `GameMode` exists only on the server and is the source of truth for race progression.
- Clients cannot submit lap counts, checkpoint indices, finish state, or respawn transforms.
- A checkpoint crossing is accepted only during the `Racing` phase and only when its index matches the player's expected checkpoint.
- The server selects the respawn transform from the player's last validated checkpoint.
- Shared race state is replicated through `GameState`; persistent player progress is replicated through `PlayerState`.
- UI reads state and sends requests but never decides gameplay outcomes.

### Race flow

```text
Map BeginPlay
    -> discover and validate checkpoint indices
    -> initialize GameState and PlayerStates
    -> Countdown (server end timestamp is replicated)
    -> Racing (vehicle input enabled, checkpoints accepted)
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

Telemetry is presentation data for UI, debugging, and the future audio system. It is not used as authoritative proof of race progress.

## Content

| Asset | Purpose |
| --- | --- |
| `/Game/Maps/Lvl_Race` | Current multiplayer race map and default editor/game map. |
| `/Game/Core/BP_RaceGameMode` | Blueprint defaults for the C++ race mode. |
| `/Game/CheckPoints/BP_RaceCheckpoint` | Configurable checkpoint presentation and collision asset. |
| `/Game/UI/WBP_DebugHUD` | Debug telemetry and networking display. |
| `/Game/UI/WBP_RaceCountdown` | Countdown and race-start presentation. |

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
6. The debug HUD shows the local race state, progress, telemetry, roles, and ping.

For meaningful network testing, use separate server/client worlds and PIE network emulation. Split-screen alone is not sufficient validation.

## Roadmap

1. **Complete race results** - record authoritative finish times, assign positions, replicate results, and display the final table.
2. **Validate local multiplayer** - test 2-4 players, late joins, disconnects, listen-server behavior, and Chaos movement under simulated latency.
3. **Online Subsystem Null** - implement session create/find/join/leave and listen-server travel in a project subsystem.
4. **Steam** - switch the same session flow to Steam lobbies and add friend invitations.
5. **Vehicle audio** - build the RPM/load engine model, gear shifts, skid, impacts, surfaces, wind, and audio debugging.
6. **Polish and profile** - use network emulation, Unreal Insights, stat commands, failure handling, and packaged-build tests.
7. **Portfolio delivery** - add architecture diagrams, screenshots, known limitations, and a short demonstration video.

## Scope boundaries

The vertical slice intentionally excludes open-world systems, multiple vehicle classes, economy, garage progression, complex tuning, dedicated servers, full matchmaking, PCG tracks, and vehicle destruction.

## Repository policy

Source-of-truth project files under `Source/`, `Config/`, and `Content/` are tracked. Generated folders such as `Binaries/`, `DerivedDataCache/`, `Intermediate/`, and `Saved/` are excluded by `.gitignore`.

Unreal binary assets should use Git LFS before the repository is distributed extensively.
