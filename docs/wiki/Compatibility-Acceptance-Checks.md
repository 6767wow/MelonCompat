# Compatibility Acceptance Checks

This page defines what "working" means for MelonCompat and RoseMod. It exists because many failures looked like success at first: the game launched, but the mod did not actually behave like it did in the real loader.

## Basic Loader Check

A loader check passes only when:

1. The game starts.
2. The loader starts.
3. The loader writes logs.
4. The selected mod/plugin is discovered.
5. The selected mod/plugin is not skipped.
6. Initialization succeeds.

This is the lowest bar. It does not prove gameplay compatibility.

## MelonLoader Mod Check

For a MelonLoader-style mod:

1. The DLL is in the right folder.
2. The `MelonLoader` facade resolves.
3. `MelonInfo` metadata loads.
4. The mod type inherits `MelonBase`.
5. The mod instance is created.
6. `OnInitializeMelon` runs.
7. Harmony autopatches run if the mod expects them.
8. Scene callbacks run.
9. Update callbacks run.
10. Visible gameplay behavior matches real MelonLoader.

Failure examples:

- Missing title screen.
- Character behavior missing.
- Baldi freezes after day one.
- Crash after jumpscare.
- "Loaded 0 melon(s)" when a mod DLL is present.

## BepInEx Plugin Check

For a BepInEx-style plugin:

1. The DLL is in `RoseMod/BepInExPlugins`.
2. The correct BepInEx facade assembly resolves.
3. Plugin metadata loads.
4. The plugin base class is recognized.
5. Mono plugins are attached as Unity components when needed.
6. IL2CPP plugins get their `Load` method called.
7. BepInEx logging works.
8. BepInEx config calls work.
9. Harmony patches run.
10. Visible gameplay behavior matches real BepInEx.

## Patcher Check

For patchers:

1. The DLL is in `RoseMod/Patchers`.
2. Legacy path `BepInEx/patchers` resolves when required.
3. The patcher is discovered.
4. Target assemblies are identified.
5. Patch methods run.
6. Patched outputs are written when Cecil rewriting is used.
7. The target assembly is patched before Unity loads it, if required.

Failure examples:

- Patcher exists but no log entry appears.
- Patcher runs but game behavior does not change.
- Patched assembly is written but not active during the current launch.

## IL2CPP Check

For IL2CPP games:

1. Native bootstrap starts.
2. CoreCLR host starts.
3. Il2CppInterop initializes before mod loading.
4. `RoseMod.Il2CppFixes` installs.
5. `UnityEngine.CoreModule.dll` interop resolves.
6. `Assembly-CSharp.dll` interop resolves if the mod uses game classes.
7. Class injection works for mod-defined IL2CPP types.
8. Native detours do not crash.

Failure examples:

- `Il2CppInteropRuntime is not yet initialized`.
- `UnityEngine.CoreModule` missing.
- `Assembly-CSharp` missing.
- ClassInjector type initializer failure.
- Native crash after scene transition.

## Mono Check

For Mono games:

1. Mono host starts.
2. Unity managed assemblies resolve from `_Data/Managed`.
3. `BaseUnityPlugin` facades work for BepInEx-style plugins.
4. MelonLoader facade callbacks work.
5. Plugin serialization fallback is active for BepInEx 5 style plugins.

Failure examples:

- BepInEx 5 plugin loads but Unity refuses to clone/serialize plugin types.
- Plugin logs but does not affect the game.
- Game-specific API references fail.

## Baldi Helps Granny Check

This mod became the main real-world compatibility test.

Expected signs:

1. Mod DLL is discovered.
2. It is not skipped.
3. Initialization does not throw.
4. Custom title screen appears.
5. Baldi and Granny behavior appears.
6. Day transition does not freeze Baldi.
7. Jumpscare does not crash the game.

If any of these fail, the loader is not compatible with that mod yet, even if the game launches.

## Baldi's Basics Plus Check

This game exposed separate BepInEx/RoseMod installer and patcher issues.

Checks:

1. Game is detected as Baldi's Basics Plus, not Granny.
2. Backend is detected as Mono.
3. Existing BepInEx 5 state is detected.
4. RoseMod install works even if BepInEx removal is denied.
5. `RoseMod/Patchers` is used for patchers.
6. Legacy `BepInEx/patchers` path works for older tools.
7. Program Compatibility Assistant does not confuse install status.

## Granny Check

This game exposed IL2CPP and crash issues.

Checks:

1. Game is detected as Granny.
2. Backend is detected as IL2CPP.
3. Native bootstrap starts.
4. IL2CPP interop is present or prepared.
5. The game does not instantly crash.
6. RoseMod logs contain managed startup.
7. Mods do not crash after jumpscare or scene change.

## "No Skip" Rule

Do not mark compatibility as done when logs contain:

```text
Skipped <mod>
Loaded 0 melon(s)
Failed to initialize
Failed to resolve assembly
Unity frame callback bridge is unavailable
```

Some warnings can be survivable, but each one must be explained.

## Release Acceptance

A release is acceptable only when:

1. Source builds.
2. GUI launches.
3. CLI doctor mode works.
4. Normal shim install works in a BepInEx 6 test game.
5. RoseMod install works in a Mono test game.
6. RoseMod install works in an IL2CPP test game.
7. Logs are written.
8. No required mod is silently skipped.
9. The release ZIP and source tree match the same commit.
10. Wiki/README mention known limitations honestly.
