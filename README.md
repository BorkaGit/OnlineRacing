# Online Racing

Online Racing is a compact Unreal Engine 5 C++ portfolio project: a 2–4 player arcade race built with Chaos Vehicles and a listen-server architecture.

The goal is a polished, finished vertical slice rather than a large game. Gameplay architecture, networking, race validation, telemetry, and vehicle audio are implemented primarily in C++. Blueprints are reserved for asset configuration, UI layout, animation, and presentation.

## Current status

The repository currently contains the Unreal Engine Vehicle C++ Template baseline.

Available now:

- Chaos Vehicles sports car and off-road example;
- Enhanced Input for steering, throttle, brake, handbrake, camera, and reset;
- chase and interior-style cameras;
- basic speed and gear UI;
- template time-trial example with track gates, laps, and local split-screen support;
- template maps and vehicle assets.

The existing time-trial code is reference material, not the final multiplayer race architecture. Session management, replicated race state, server-authoritative checkpoints, telemetry, and the production audio system have not been implemented yet.

## Target vertical slice

- One playable Chaos Vehicle;
- one compact race track;
- 2–4 players through a listen server;
- Online Subsystem Null for local development, followed by Steam;
- lobby creation, discovery, join, and Steam friend invitations;
- replicated pre-race countdown;
- server-validated checkpoints, laps, finish order, and results;
- respawn at the last server-confirmed checkpoint;
- vehicle telemetry and network debug HUD;
- RPM- and load-driven vehicle audio with gears, tire skid, impacts, surfaces, and wind;
- packaged demonstration build, architecture documentation, and short gameplay video.

## Scope

This project intentionally excludes:

- open-world gameplay;
- multiple vehicle classes;
- progression, economy, or garage systems;
- complex tuning;
- dedicated servers;
- full matchmaking;
- procedural track generation;
- vehicle destruction.

## Architecture

Existing template classes will be kept only where they provide a useful vehicle foundation. The target responsibilities are:

| Type | Responsibility |
| --- | --- |
| `AOnlineRacingPawn` | Chaos Vehicle ownership, player input, cameras, and vehicle-facing commands |
| `UVehicleTelemetryComponent` | Stable snapshot of speed, RPM, gear, driver input, and skid data |
| `UVehicleAudioComponent` | Vehicle audio state and parameter updates driven by telemetry |
| `UVehicleDataAsset` | Designer-editable handling and audio configuration for the single vehicle |
| `ARaceGameMode` | Server-only race rules, player spawning, validation, and result assignment |
| `ARaceGameState` | Replicated race phase, countdown timing, and public results |
| `ARacePlayerState` | Replicated per-player lap, checkpoint, finish, and timing state |
| `ARaceCheckpoint` | Ordered checkpoint volume that reports crossings to server race logic |
| `USessionSubsystem` | Online Subsystem session lifecycle independent of menu widgets |
| UI | Presentation of replicated state; no race authority or session rules |

### Network authority

- The owning client produces vehicle input and displays local UI/debug information.
- The server owns race progression, checkpoint validation, respawn transforms, countdown state, and final results.
- `GameMode` exists only on the server.
- Replicated public race state belongs in `GameState`.
- Replicated player progress belongs in `PlayerState`, so it survives Pawn replacement.
- Widgets and audio components consume state but never decide race outcomes.

## Development roadmap

1. **Vehicle foundation** — keep one sports car, stabilize handling and reset, add telemetry and a debug HUD.
2. **Race rules** — implement race phases, countdown, ordered checkpoints, laps, finish, results, and checkpoint respawn.
3. **Local multiplayer** — validate authority and replication with 2–4 PIE clients using Online Subsystem Null.
4. **Steam sessions** — create, find, join, leave, travel, and invite through Steam.
5. **Vehicle audio** — engine RPM/load model, gear shifts, skid, impacts, surfaces, wind, and audio debugging.
6. **Polish and profiling** — network emulation, stat commands, Unreal Insights, failure handling, and packaged-build tests.
7. **Portfolio delivery** — architecture notes, screenshots, known limitations, and a short demonstration video.

## Getting started

Prerequisites:

- the Unreal Engine 5 installation associated with `OnlineRacing.uproject`;
- Visual Studio 2022 with the C++ game-development and Unreal Engine components;
- Windows 64-bit toolchain.

Setup:

1. Clone the repository.
2. Right-click `OnlineRacing.uproject` and generate Visual Studio project files if required.
3. Build the `OnlineRacingEditor` target in `Development Editor | Win64`.
4. Open `OnlineRacing.uproject`.
5. Run `/Game/VehicleTemplate/Maps/VehicleBasic` in Play In Editor.

The current default map and game mode still point to the original Vehicle Template assets. They will be replaced by project-specific race assets during the vehicle-foundation milestone.

## Repository policy

Tracked source-of-truth files include:

- `Source/`;
- `Config/`;
- `Content/`;
- `OnlineRacing.uproject`;
- project documentation.

Generated build products, local IDE state, caches, logs, and saved editor data are excluded by `.gitignore`.

Unreal binary assets (`.uasset` and `.umap`) should be stored with Git LFS before the repository is published or shared extensively.
