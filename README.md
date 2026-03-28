# RacingGame

Multiplayer kart racing game built with Unreal Engine 5 and the Chaos Vehicle system.

[![Gameplay
  Video](https://img.youtube.com/vi/bk4r0aSvFvA/hqdefault.jpg?v=2)](https://www.youtube.com/watch?v=bk4r0aSvFvA)

  https://www.youtube.com/watch?v=bk4r0aSvFvA
  
## Tech Stack

| | |
|---|---|
| **Engine** | Unreal Engine 5.7 (C++) |
| **Vehicle Physics** | Chaos Vehicles (`UChaosWheeledVehicleMovementComponent`) |
| **Input** | Enhanced Input System |
| **Networking** | UE Replication, Steam Sockets |
| **Online** | OnlineSubsystemSteam, AdvancedSessions, AdvancedSteamSessions |
| **Rendering** | Custom material shaders (radial fill, GPU-driven minimap) |

## Architecture

The project follows a clean interface-driven design. Two core interfaces decouple gameplay systems:

- **IInputGameInterface** — abstracts vehicle input so the controller never references the pawn class directly. The controller translates Enhanced Input actions into interface calls (`InputMove`, `InputBrake`, `InputHandbrake`, `InputLookAround`, `InputToggleCamera`), and the pawn implements them. This means any pawn class that implements the interface can be driven by the same controller — useful for swapping vehicle types or adding AI.

- **IGameInfoInterface** — broadcasts race events (`OnLapCompleted`, `OnRaceFinished`, `OnAllPlayersFinished`, `OnPlayerConnected`, `OnLapDataUpdated`) without tight coupling between GameMode, GameState, Controller, and Pawn. Each class only implements the events it cares about.

This separation makes it straightforward to extend race logic, add new vehicle types, or introduce AI controllers without touching existing code.

## Networking

All race state lives in `ARacingGameState` and replicates automatically.

| What | How | Why |
|---|---|---|
| Race logic (laps, standings, finish) | Server-authoritative | Prevents cheating; single source of truth |
| Vehicle input (steering, throttle, brake) | `Server, Unreliable` RPC | A dropped packet is immediately replaced by the next one |
| UI events (lap complete, finish, countdown) | `Client, Reliable` RPC | No widget should ever be missed |
| Player color | `ReplicatedUsing` | Triggers `OnPlayerColorReady` on clients the moment it arrives |
| Lap data array | `ReplicatedUsing` | Clients receive updated standings and fire `OnLapDataUpdated` |

The GameMode only runs on the server. It assigns spawn points, distributes colors, and initiates countdowns. The PlayerController handles client-side UI and forwards input to the pawn through the input interface.

## Steam Integration

Multiplayer runs on top of Steam via `OnlineSubsystemSteam`, `SteamSockets`, and the third-party `AdvancedSessions` / `AdvancedSteamSessions` plugins. Session creation, search, and join go through the Steam backend — no dedicated server or manual IP entry required.

Player identity is pulled directly from the Steam account: the player's **Steam nickname** is used as the in-game display name, and their **Steam avatar** is fetched and shown in the UI. This keeps the lobby and race HUD consistent with what players see in their Steam friends list.

## Game Flow

1. **Lobby** — Players connect. `PostLogin` assigns each player a random color from the available pool and notifies all clients via `OnPlayerConnected`.
2. **Pre-race** — The host sees a pre-start widget with a "Start" button. Other players see a waiting screen. The host triggers `BeginCountdown`.
3. **Countdown** — A 5-second countdown runs on each client. Input is blocked until the final second. The pre-start widget is removed on "GO".
4. **Race** — Players drive. Each time a kart crosses the lap trigger in the correct direction, the server records the lap time, updates standings, and notifies the driver's controller. A lap-completed widget appears for 3 seconds.
5. **Finish** — When a player completes all laps, they receive a finish widget and their input is disabled. The server records their finish position and total time.
6. **All finished** — Once every player has finished, all clients receive `OnAllPlayersFinished`. After a short delay, race widgets are hidden and a final results screen is shown with cursor enabled.

## Systems in Detail

### Race Management

**GameMode** (`ARacingGameMode`)
- Sets default classes: `AKartPawn`, `ARacingGameState`, `ARacingPlayerState`.
- On `InitGameState`, reads `TotalLaps` from `URacingGameInstance` (persists across map travel).
- Caches and sorts `PlayerStart` actors by name for deterministic spawn order.
- Assigns each player a sequential kart number and a random color from a designer-defined palette.
- `BeginCountdown` iterates all player controllers and triggers the countdown RPC.

**GameState** (`ARacingGameState`)
- Owns the replicated `TArray<FPlayerLapData>` — the single source of truth for race standings.
- `RegisterLapTime` records the time, updates best/last lap, increments the lap counter, and checks for race completion.
- `CancelLastLap` allows undoing the last lap (useful for debugging or penalty systems) — recalculates best time from remaining laps.
- Provides sorted accessors: `GetSortedByBestTime` (leaderboard) and `GetSortedByCurrentLap` (race position).
- `BroadcastLapDataUpdated` notifies all controllers so UI can refresh.

**PlayerState** (`ARacingPlayerState`)
- Holds the replicated `PlayerColor`. When it replicates, `OnRep_PlayerColor` calls `OnPlayerColorReady` on the owning pawn so materials/widgets can update.

### Lap Trigger

`ARacingLapTrigger` uses two `UBoxComponent` volumes placed sequentially along the track:

1. **EntryBox** — when a kart enters, it is added to `EnteredActors`.
2. **ExitBox** — when a kart exits:
   - If the kart is in `EnteredActors` (correct direction): lap is completed, timestamp is recorded, and `RegisterLapTime` or `StartRace` is called on GameState.
   - If the kart is NOT in `EnteredActors` (wrong direction): it is added to `WrongExitActors`. If it then hits EntryBox, the wrong-direction pass is cancelled.

This dual-box approach reliably detects direction without raycasts or dot-product checks.

The trigger also exposes `GetLastPassTime` and `GetTimeSinceLastPass` for Blueprint use (e.g. showing a live lap timer in the HUD).

### Vehicle (KartPawn)

Built on `AWheeledVehiclePawn` with `UChaosWheeledVehicleMovementComponent`.

**Physics & Input**
- Automatic braking logic: when the player presses the opposite direction (forward while going backward or vice versa), the pawn brakes first until speed drops below 5 cm/s, then switches gear and applies throttle. This prevents jarring gear changes at speed.
- Separate brake input (e.g. left trigger on gamepad) sets brake directly without auto-braking interference.
- Handbrake input for drifting.
- All vehicle input is replicated to the server via `ServerUpdateVehicleInput` (Unreliable).

**Cameras**
- **First-person (hood cam)** — attached via `USpringArmComponent` with rotation lag. Look-around is clamped to configurable yaw/pitch limits. The driver's head bone is hidden in this mode.
- **Third-person (chase cam)** — rear spring arm with free yaw rotation and clamped pitch. The head bone is visible.
- Toggle between cameras via `InputToggleCamera`. Camera state resets on switch.

**Driver head rotation** — in first-person mode, the head rotation is sent to the server (`ServerUpdateHeadRotation`, Unreliable) and replicated to all clients, so other players see the driver looking around.
![Gameplay](Docs/HeadRotation.gif)

**Body offset** — a filtered acceleration vector (`BodyOffset`) is computed each tick from longitudinal and centrifugal acceleration. This is exposed to Blueprint for procedural body lean animations (the driver leans into turns and brakes).
![Gameplay](Docs/BodyRotation.gif)

**Nickname widget** — a world-space `UUserWidget` is created in `BeginPlay` and positioned each tick at `NicknamePoint`. It scales inversely with camera distance (configurable reference distance, min/max scale) and hides when off-screen. Cleaned up in `EndPlay`.
<img width="2559" height="1383" alt="image" src="https://github.com/user-attachments/assets/04551823-32ac-4c2e-9af4-e03b1cdfee4f" />


### Minimap (MapLibrary)

A GPU-driven minimap system with zero per-dot draw calls.
<img width="422" height="365" alt="image" src="https://github.com/user-attachments/assets/e48b2508-72e1-484f-b92b-7b9a8476d631" />

**How it works:**
1. `CreateMapTexture` creates a 64x2 transient `PF_B8G8R8A8` texture (nearest filtering, no sRGB, no compression).
2. `UpdateMapPositions` iterates actors of a given class, normalizes their positions relative to a center point and world radius, and writes them into the texture:
   - **Row 0** — position data: R = normalized X, G = normalized Y, B = active flag (255), A = unused.
   - **Row 1** — per-actor color: RGB from `IMapDotColorInterface::GetMapDotColor()` if implemented, otherwise the provided default color.
3. A custom HLSL node in the material loops over all 64 pixels, samples position and color, and draws an icon (or circle) at each active dot's location.
<img width="1969" height="762" alt="image" src="https://github.com/user-attachments/assets/1791a92c-03bc-4312-854b-c28d0ac5d08a" />
<img width="1385" height="794" alt="image" src="https://github.com/user-attachments/assets/05cd7586-e279-4d50-8a89-7b6e60078a84" />


**Features:**
- Optional yaw rotation (`bRotateWithYaw`) — the map rotates so the player always faces up.
- Coordinate swizzle (Y→X, -X→Y) maps UE's X-forward convention to screen-space Y-up.
- Per-actor colors via the `IMapDotColorInterface` — each actor can return its own color (e.g. team color from PlayerState).
- The material supports custom icons instead of circles by sampling a texture at local UV coordinates relative to each dot.

### Speed Widget (Material Shader)

A radial speedometer built entirely in the UE material editor — no Blueprint tick needed for the visual fill.

<img width="319" height="301" alt="image" src="https://github.com/user-attachments/assets/78c9aa07-45c8-4944-aaa9-0ecd3dd1a281" />

**How it works:**
- Three textures compose the gauge: `T_SpeedWidget_Empty` (background), `T_SpeedWidget_Full` (filled arc), and `T_SpeedWidget_Arrow` (needle).
- A scalar parameter `Progress` (0–1) drives everything. It is remapped to the angular range 0.11–0.7 via `RemapValueRange` to match the gauge arc.
- **Radial fill:** UVs are converted to polar coordinates (`atan2` of centered UVs, divided by 2π, offset and wrapped with `frac`). A `Step` node compares the pixel's angle against `ProgressClamped`, producing a hard mask that blends the Full texture over the Empty texture.
- **Needle rotation:** The arrow texture UVs are rotated with a standard 2D rotation matrix (`X' = X·cos − Y·sin`, `Y' = X·sin + Y·cos`). The rotation angle is derived from `ProgressClamped`, so the needle tracks the fill.
- **Color gradient:** The needle color lerps from green `(0,1,0)` to red `(1,0,0)` based on `Progress` — low speed is green, high speed is red. The tinted color is masked by the arrow texture and added to the base gauge on the Final Color output.
- **Opacity:** The empty texture's alpha channel combined with the arrow mask, scaled by 0.3, feeds the Opacity Override — keeping the gauge semi-transparent over the HUD.
<img width="1220" height="1038" alt="image" src="https://github.com/user-attachments/assets/7513a7a2-5a62-4dbd-8c5a-1b597b36c9d6" />

### Spline Tool

`ASplineTool` is an editor and runtime tool for building track geometry along a spline.

![SplineTool](Docs/SplineTool.gif)

**Deformable mesh mode:**
- Divides the spline into segments of configurable length (`SegmentLength`).
- Creates a `USplineMeshComponent` per segment, with tangents clamped to segment length for smooth bends.
- Configurable scale (`MeshScale`) and optional last-segment stretching (`bScaleLastToFit`) to fill the remaining spline length.

**Instanced mesh mode:**
- Uses `UHierarchicalInstancedStaticMeshComponent` (HISM) for batched rendering.
- Places instances at regular intervals (`InstanceSpacing`) along the spline.
- Each instance can be offset (`InstanceOffset`) and rotated (`InstanceRotationOffset`) in spline-local space.
- Configurable scale per instance (`InstanceScale`).

Both modes rebuild in `OnConstruction` (editor preview) and `BeginPlay` (runtime).

### Player Controller

`AKartPlayerController` bridges Enhanced Input with gameplay:

**Input handling:**
- Binds Enhanced Input actions (Move, Brake, Handbrake, LookAround, ToggleCamera) in `SetupInputComponent`.
- All input is blocked until `bRaceStarted` is true (except camera look-around).
- Input values are forwarded to the pawn through `IInputGameInterface` — the controller never casts to `AKartPawn`.

**UI management:**
- Manages five widget layers: PreStart, DefaultGame (HUD), LapCompleted, Finish, AllFinished.
- Widgets are created lazily and reused.
- LapCompleted widget auto-hides after 3 seconds via timer.
- AllFinished widget appears after a 3-second delay, hides all other widgets, and switches to UI-only input with cursor.

**Countdown:**
- `ClientStartCountdown` starts a 1-second repeating timer from 5.
- `CountdownValue` is exposed as `BlueprintReadOnly` for the countdown widget to read.
- Input mode switches to game-only on the last tick, and the race starts on zero.

## Project Structure

```
Source/RacingGame/
  Interfaces/
    InputGameInterface       Vehicle input abstraction
    GameInfoInterface        Race event broadcast
    MapDotColorInterface     Per-actor minimap color
  Race/
    RacingGameMode           Server-side race orchestration
    RacingGameState          Replicated race standings
    RacingPlayerState        Replicated player color
    RacingLapTrigger         Directional lap detection
  Vehicles/
    KartPawn                 Chaos vehicle with cameras and input
  GameSetup/
    KartPlayerController     Enhanced Input + UI management
    RacingGameInstance       Persistent settings (total laps)
  Tools/
    SplineTool               Spline-based track geometry
    MapLibrary               GPU-driven minimap texture
```
