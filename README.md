# OnlineRacing

OnlineRacing is a compact Unreal Engine 5.8 C++ portfolio project: a 2-4 player arcade race built on Chaos Vehicles and a listen-server architecture.

The goal is a finished vertical slice that demonstrates Unreal C++, multiplayer gameplay architecture, vehicle telemetry, and vehicle audio. Core gameplay and networking live in C++; Blueprints are used for asset setup, UI layout, animation, tuning, and presentation.

## Current milestone

The project currently has a validated Online Subsystem Null flow and a configured Steam backend leading from the main menu to a listen-server race, together with server-controlled AI and a functional vehicle-audio pass. The checked-in configuration selects Steam by default; cross-account Steam validation is the current networking milestone.

- a main menu with host, session search, and join controls;
- a `GameInstanceSubsystem` that creates, discovers, joins, destroys, and replaces online sessions through the active Online Subsystem;
- enabled `OnlineSubsystemSteam` and `SteamSockets` plugins, Steam lobby-oriented session settings, development App ID `480`, and a Steam Sockets game NetDriver;
- listen-server lobby travel for the host and resolved-address client travel for joining players;
- a replicated lobby with host assignment, per-player Ready state, aggregate player counts, and a server-validated start request;
- seamless server travel from `/Game/Maps/Lvl_Lobby` to `/Game/Maps/Lvl_Race` after every player is ready;
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
- telemetry-driven rear-wheel Niagara smoke with wheel-contact validation;
- a debug HUD with telemetry, race state, network roles, net mode, and ping;
- PIE listen-server gameplay with multiple players.

![Gameplay](assets/gameplay.gif)

The original Vehicle Template time-trial variant remains in the repository as reference content. The multiplayer race uses separate project-specific C++ classes and assets.

## Not implemented yet

- end-to-end Steam validation with separate Steam accounts and machines;
- Steam friend-invitation presentation and invite acceptance flow;
- a complete leave-session and return-to-menu user flow;
- session advertisement updates and an explicit late-join policy once a race starts;
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
| `UOnlineRacingSessionSubsystem` | Owns Online Subsystem session creation, discovery, joining, destruction, error reporting, and lobby travel coordination. |
| `AOnlineRacingMainMenuPlayerController` | Creates the local session-menu UI and configures menu input. |
| `AOnlineRacingLobbyGameMode` | Owns server-only lobby membership, host assignment, Ready validation, start eligibility, and travel to the match. |
| `AOnlineRacingLobbyGameState` | Replicates public lobby counts, start eligibility, and match-travel state. |
| `AOnlineRacingLobbyPlayerController` | Sends owned Ready and start requests to the server and coordinates the local lobby UI. |
| `AOnlineRacingMatchGameMode` | Owns server-only race orchestration, checkpoint validation, lap progression, countdown start, finish timing and ordering, and respawn selection. |
| `AOnlineRacingMatchGameState` | Replicates public race phase, synchronized timing, race configuration, and the ordered results snapshot. |
| `AOnlineRacingPlayerState` | Replicates lobby identity/Ready state and per-player race progress, finish position, and finish time across travel. |
| `AOnlineRacingCheckpoint` | Defines an ordered overlap volume and a respawn transform; reports vehicle crossings to server race logic. |
| `AOnlineRacingGridStart` | Marks a deterministic indexed grid position available to a human or AI controller. |
| `AOnlineRacingMatchPlayerController` | Coordinates local race UI and input state and sends respawn requests to the server. |
| `UOnlineRacingVehicleEffectsComponent` | Converts wheel-contact and slip/skid telemetry into local Niagara tire-smoke presentation. |
| `UOnlineRacingCountdownWidget` | Presents countdown values and `GO`; it never starts or advances the race. |
| `UOnlineRacingResultsWidget` | Presents replicated finish positions and times; Blueprint may replace the fallback text with styled rows. |
| `UOnlineRacingDebugWidget` | Presents telemetry and replicated networking/race state for development. |

### Authority model

- `SessionSubsystem` coordinates Online Subsystem operations and travel, but does not own lobby or race rules.
- `LobbyGameMode` accepts Ready/start requests only on the server and calculates whether the match may start.
- `GameMode` exists only on the server and is the source of truth for race progression.
- Clients cannot submit lap counts, checkpoint indices, finish state, or respawn transforms.
- A checkpoint crossing is accepted only during the `Racing` phase and only when its index matches the player's expected checkpoint.
- Finish time is calculated from synchronized server timestamps; the server assigns each finishing position exactly once.
- The server selects the respawn transform from the player's last validated checkpoint.
- Shared race state is replicated through `GameState`; persistent player progress is replicated through `PlayerState`.
- UI reads state and sends requests but never decides gameplay outcomes.

### Race flow

```text
Main Menu
    -> host creates a session and opens Lobby as a listen server
       OR client discovers a session, joins it, and travels to Lobby
    -> server assigns the lobby host and tracks Ready state
    -> all players Ready
    -> host requests match start
    -> seamless server travel to Race

Race map BeginPlay
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
| `/Game/Maps/Lvl_MainMenu` | Packaged-game entry map containing the session menu. |
| `/Game/Maps/Lvl_Lobby` | Listen-server lobby used before match travel. |
| `/Game/Maps/Lvl_Race` | Multiplayer race map and editor startup map. |
| `/Game/Core/MainMenu/BP_MainMenuGameMode` | Main-menu GameMode and local controller defaults. |
| `/Game/Core/Lobby/BP_LobbyGameMode` | Lobby rules, match-map reference, and lobby controller defaults. |
| `/Game/Core/Race/BP_MatchGameMode` | Blueprint asset wiring and tuning for the C++ match mode. |
| `/Game/Core/Race/BP_GridStart` | Indexed grid-start marker used by human and AI racers. |
| `/Game/Core/Race/BP_RacingLine` | Closed spline that defines the AI driving line. |
| `/Game/AI/BP_RacingAIController` | Tuned Blueprint defaults for the C++ vehicle AI controller. |
| `/Game/CheckPoints/BP_RaceCheckpoint` | Configurable checkpoint presentation and collision asset. |
| `/Game/UI/WBP_DebugHUD` | Debug telemetry and networking display. |
| `/Game/UI/WBP_RaceCountdown` | Countdown and race-start presentation. |
| `/Game/UI/WBP_RaceResults` | Replicated race-results presentation. |
| `/Game/UI/WBP_SessionMenu` | Host, search, join, status, and selected-session presentation. |
| `/Game/UI/WBP_Lobby` | Ready state, lobby counts, host controls, and travel status. |
| `/Game/Audio/Vehicles/Metasounds/SportsCar/MS_Veh_Sport_Ip` | Persistent MetaSound vehicle loop used alongside MotoSynth. |

## Running the project

### Requirements

- Unreal Engine 5.8 with the Chaos Vehicles plugin;
- Visual Studio 2022 with Desktop development with C++ and Game development with C++ workloads;
- Windows 64-bit toolchain.
- a running Steam client and a logged-in Steam account for Steam backend tests.

### Setup

1. Clone the repository.
2. Generate project files from `OnlineRacing.uproject` if required.
3. Build `OnlineRacingEditor` using `Development Editor | Win64`.
4. Open `OnlineRacing.uproject`.
5. Launch from `/Game/Maps/Lvl_MainMenu` to test the complete session flow, or open `/Game/Maps/Lvl_Race` for direct race iteration.

### Backend status

- Online Subsystem Null has been used to validate the local create/find/join and listen-server travel flow.
- The checked-in `DefaultEngine.ini` currently selects `DefaultPlatformService=Steam`.
- Steam uses development App ID `480` (Spacewar) and `SteamSocketsNetDriver`; it must be replaced with the project's own App ID before release.
- The last successful Null test does not count as Steam validation. A Steam run should report `Backend=STEAM`, `LAN=0`, and `SteamSocketsNetDriver` in the log.

Restart the editor after changing Online Subsystem plugins or configuration. Multiple PIE windows commonly share one Steam identity, so meaningful Steam testing should use separate processes with separate Steam accounts, preferably on separate machines.

### Session and lobby test

Use separate server/client windows. The complete expected flow is:

1. In the first window, select **Host**. The session is created and the host opens `Lvl_Lobby` as a listen server.
2. In the second window, select **Refresh**, select the discovered session, and select **Join**.
3. Both players enter the lobby. Each player toggles **Ready**.
4. When the minimum player count is met and every player is ready, the host selects **Start**.
5. The server performs seamless travel to `Lvl_Race`; connected clients follow the host.

### Race validation

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

1. **Validate Steam end to end** - verify initialization, create/find/join, lobby travel, seamless match travel, and cleanup with separate Steam accounts using App ID `480`.
2. **Harden the session flow** - test 2-4 players, repeated host/join/destroy cycles, failed joins, disconnects, late joins, and return-to-menu behavior.
3. **Define the match session lifecycle** - stop or update session advertisement when the race starts and decide how reconnects and late joins are handled.
4. **Steam invitations** - expose the Steam invite UI and handle joining through an accepted invitation.
5. **Network and audio validation** - test Chaos control under latency/packet loss, expose audio debug values, and profile with Audio Insights.
6. **Race-rule tests** - add focused automation tests for checkpoint sequencing, lap completion, finish ordering, and invalid progress.
7. **Portfolio delivery** - replace the development App ID, validate Development and Shipping packaged builds, finish UI polish, document known limitations, and record a short demonstration video.

## Scope boundaries

The vertical slice intentionally excludes open-world systems, multiple vehicle classes, economy, garage progression, complex tuning, dedicated servers, full matchmaking, PCG tracks, and vehicle destruction.

## Repository policy

Source-of-truth project files under `Source/`, `Config/`, and `Content/` are tracked. Generated folders such as `Binaries/`, `DerivedDataCache/`, `Intermediate/`, and `Saved/` are excluded by `.gitignore`.

Unreal binary assets should use Git LFS before the repository is distributed extensively.
