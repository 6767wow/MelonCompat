# Full Bug History

This page is the long-form bug history for MelonCompat and RoseMod. It keeps repeated reports grouped by unique failure instead of pretending each fix landed perfectly on the first try.

## 1. Original MelonCompat Runtime Failure

The first concrete runtime problem was that a MelonLoader mod worked in real MelonLoader but did not behave correctly through the BepInEx compatibility layer.

Observed behavior:

- The mod loaded far enough to run some code.
- Baldi could not kill the player correctly.
- The expected game behavior did not match real MelonLoader.

Meaning:

- The facade compiled, but lifecycle callbacks, Unity event timing, or patched game methods were not equivalent enough.
- Compatibility had to be tested against real gameplay, not just assembly load success.

Documentation status:

- This became the baseline rule: a build is not compatible until a real mod behaves correctly in-game.

## 2. Missing MelonLoader Facade Surface

Early real-mod tests exposed missing MelonLoader APIs.

Common failure:

```text
Could not load type MelonLoader.Utils.MelonEnvironment
```

Meaning:

- The mod referenced public MelonLoader API types that the compatibility facade did not define.
- Mods compiled against `MelonLoader.dll` resolve by assembly name and public type names.

Fix direction:

- Keep the compatibility assembly named `MelonLoader`.
- Match the MelonLoader 0.7.3 assembly identity.
- Add `MelonLoader.Utils`, environment helpers, attributes, logging, preferences, and other common public types.

Status:

- Partially addressed by `MelonLoaderApi/`.
- Still a class of bugs whenever a mod uses a public or internal MelonLoader detail that is not covered yet.

## 3. Installer Had to Become a Real EXE

The request moved from a plugin DLL to a releaseable installer.

Requirements added:

- Build a Windows EXE installer.
- Support Mono and IL2CPP Unity games.
- Target MelonLoader mods from 0.5.7 through 0.7.3.
- Detect installed Steam Unity games.
- Show game icons, platform, backend, BepInEx status, MelonLoader status, and RoseMod status.

Problems found:

- A raw DLL was not user-friendly.
- The installer had to understand Unity backends and installed loaders.
- It had to avoid installing the normal MelonCompat shim when BepInEx was missing unless the user approved BepInEx installation.

Status:

- The Tauri GUI scans games and calls the CLI backend.
- The CLI backend handles the actual installation logic.

## 4. BepInEx Missing Detection

The normal MelonCompat shim depends on BepInEx.

Bug:

- The installer originally needed stricter rules when BepInEx was missing.

Expected behavior:

- Do not silently install a BepInEx-dependent shim into a game that has no BepInEx.
- Ask whether to install BepInEx first.
- Launch the game once after BepInEx installation so BepInEx can initialize.
- Continue installing the shim after the first run.

Status:

- `InstallerEngine.InstallAsync` validates BepInEx for shim installs.
- The GUI asks before installing BepInEx and before running the first game launch.

## 5. MelonLoader Removal and Migration

Problem:

- A game might already have MelonLoader installed.
- Installing MelonCompat or RoseMod over it could conflict with bootstrap files and mod folders.

Expected behavior:

- Detect MelonLoader.
- Ask before removing it.
- Optionally migrate old `Mods` DLLs into the compatible destination.

Status:

- MelonLoader detection and migration are handled by `MelonLoaderInstall`.
- The GUI asks before removal and migration.

## 6. Theme and Logo Iterations

UI issues:

- The installer initially looked too close to the MelonLoader installer.
- The theme had to become its own gray MelonCompat/RoseMod theme.
- Several logo images were supplied.
- Earlier logo processing made the eyes gray instead of white.
- Later logos replaced the earlier ones.
- RoseMod needed its own logo next to the MelonCompat logo.

Status:

- The Tauri UI uses a gray theme.
- Game icons are taken from Steam where possible.
- MelonCompat and RoseMod logos are shown together in the README and GUI assets.

## 7. Release ZIP and Source Folder

Requirement:

- Provide a release ZIP for users.
- Provide a source-code folder/ZIP for GitHub upload.

Problems:

- Generated release folders could include stale names or stale build outputs if not refreshed.
- Source ZIPs had to avoid including machine-only junk while keeping real project files.

Status:

- Release and source folders were generated under `dist/release` and `dist/github-source`.
- GitHub source is now the main repo.

## 8. README Was Too Developer-Only

Bug:

- The README originally targeted coders more than players.

Required clarification:

- Consumers need install steps.
- Coders need build/source notes.
- The compatibility range is MelonLoader 0.5.7 through 0.7.3.
- Mono and IL2CPP are both intended targets.

Status:

- README now has consumer and coder sections.
- The wiki now carries the long-form docs.

## 9. UniWork Request and Optional Tab

The project expanded into a standalone framework request.

Requirement:

- Build a framework like both MelonLoader and BepInEx.
- It should be optional in the installer.
- It should be able to load MelonLoader-style mods and BepInEx-style plugins.
- It should not replace the normal MelonCompat shim path.

Initial name:

- UniWork.

Final name:

- RoseMod.

Status:

- The GUI has a RoseMod path.
- RoseMod is documented as experimental, not a complete clone of both loaders.

## 10. No Console Showing

Several stages had console-window confusion.

Reports:

- No UniWork/RoseMod console appears on launch.
- Later the console should not show by default.
- The console window should be selectable when debugging.
- When launching games during tests, the game should not keep stealing the screen.

Meaning:

- Runtime logging cannot rely only on a visible console.
- Logs must be written to files.
- Debug console visibility has to be controlled by options.

Status:

- RoseMod writes `RoseMod/Logs/RoseMod.native.log` and `RoseMod/Logs/RoseMod.log`.
- Console visibility is controlled by runtime behavior and environment/config options.
- Tauri backend process uses no-window execution for backend CLI calls.

## 11. Console Style and Colored Logs

Requirement:

- Errors should be red.
- Warnings should be yellow.
- Info should look closer to MelonLoader.
- Mod info should look like MelonLoader's mod listing.

Related user-supplied logging utility:

- A simple ANSI color logging helper was provided.
- It influenced the RoseMod logging style.

Status:

- RoseMod logging uses colored categories when the output target supports ANSI colors.
- File logs remain plain text for readability.

## 12. UnityEngine.CoreModule Missing

Observed IL2CPP error:

```text
Could not load file or assembly 'UnityEngine.CoreModule, Version=0.0.0.0'
```

Cause:

- The mod referenced Unity assemblies that were not available through RoseMod's resolver.

Fix direction:

- Index game managed assemblies.
- Index RoseMod core assemblies.
- For IL2CPP games, prepare or import generated interop assemblies into `RoseMod/interop`.

Status:

- RoseMod resolver indexes more paths.
- Installer includes interop preparation/import steps.

## 13. Assembly-CSharp Interop Missing

Observed warning:

```text
Assembly-CSharp interop assembly was not found
```

Meaning:

- Game-type namespace fixups cannot run without the game interop assembly.
- Mods that reference game classes may fail or behave incorrectly.

Status:

- RoseMod has an interop folder and preparer.
- The warning still means the selected game's generated game interop is missing or incomplete.

## 14. Il2CppInterop Runtime Not Initialized

Observed error:

```text
Il2CppInteropRuntime is not yet initialized. Call Il2CppInteropRuntime.Create and Il2CppInteropRuntime.Start first.
```

Cause:

- A mod called Il2CppInterop APIs before RoseMod had started the IL2CPP runtime host.

Fix direction:

- Initialize Il2CppInterop before loading melons.
- Install RoseMod Il2CppInterop compatibility fixes first.

Status:

- `RoseModIl2CppInteropHost` and `RoseModIl2CppFixBridge` run before mod loading.

## 15. DispatchProxy Sealed Proxy Error

Observed error:

```text
The base type 'UniWorkDetourProviderProxy' cannot be sealed.
```

Cause:

- The early detour provider proxy shape was invalid for `DispatchProxy`.

Fix direction:

- Do not use a sealed proxy class.
- Reduce reliance on this proxy path in the later native/CoreCLR design.

Status:

- This belongs to the old UniWork design and is documented as a retired design issue.

## 16. System.Runtime 6.0 Resolution Failure

Observed warning:

```text
Failed to resolve assembly: 'System.Runtime, Version=6.0.0.0'
```

Cause:

- Cecil namespace fixups were running without a complete .NET reference set.

Fix direction:

- Register trusted platform assemblies.
- Index RoseMod core and bundled .NET assemblies.

Status:

- `RoseModRuntime.ConfigureTrustedPlatformAssemblies` handles the CoreCLR trusted platform assembly list.

## 17. ManualLogSource Argument Mismatch

Observed error:

```text
Object of type 'System.String' cannot be converted to type 'BepInEx.Logging.ManualLogSource'
```

Cause:

- Bridge invocation passed a string where a BepInEx-style logger object was expected.

Fix direction:

- Keep BepInEx logging facade constructor and property behavior close to real BepInEx.
- Do not guess logger argument shapes in reflection calls.

Status:

- Fixed in the bridge/facade wiring.

## 18. Mod Title Screen Missing

Observed behavior:

- The Baldi Helps Granny mod should show a custom title screen.
- The compatibility layer launched the game but did not reproduce the expected title screen.

Meaning:

- Loading the DLL was not enough.
- Scene lifecycle callbacks, asset loading, Harmony patch application, or game class interop were still not matching the real loader.

Status:

- This remains a compatibility acceptance test: the custom title screen is a visible proof that the mod lifecycle and patches are working.

## 19. Skipping Too Much

Repeated report:

```text
you gotta make it so it skips nothing
```

Meaning:

- Loader code that avoids errors by skipping unknown DLLs, unknown patchers, missing callbacks, or failed mod init can hide the real issue.

Fix direction:

- Log exactly what was skipped and why.
- Prefer supporting the missing API/path over silently ignoring it.
- Keep skips only for known dependency DLLs and clearly incompatible assemblies.

Status:

- The docs now call out skip warnings as compatibility bugs unless the skipped file is a known support assembly.

## 20. Unity Frame Callback Bridge Unavailable

Observed warning:

```text
Unity frame callback bridge is unavailable; keeping scene callbacks and trying fallback update bridge: Method unstripping failed
```

Meaning:

- The preferred Unity update hook could not be installed.
- This can break mods that depend on frequent update callbacks.

Fix direction:

- Keep scene callbacks.
- Use fallback managed callback pumping.
- Add an opt-in injected event pump for debugging.

Status:

- RoseMod documents fallback behavior and debug options.

## 21. Game Crashes After Jumpscare or Scene Change

Observed behavior:

- The game crashed after a jumpscare.
- Later reports included freezing after day one.

Likely causes:

- Harmony patch against fragile game/Unity code.
- Native trampoline crash in Unity 6 IL2CPP.
- Lifecycle callback order mismatch.
- Missing or incomplete game interop.

Status:

- RoseMod includes crash-guard logic for known unsafe patch patterns.
- This still requires per-mod validation because a loader cannot prove game-specific patch safety ahead of time.

## 22. Game Slow or Not Responding During Loading

Observed behavior:

- The game stopped responding for a while on the loading screen.
- In-game performance was slow.

Likely causes:

- Synchronous interop scanning.
- Heavy assembly indexing.
- Patcher work on the startup thread.
- Repeated reflection scans.

Fix direction:

- Cache indexes.
- Avoid repeated full recursive scans.
- Delay optional work where possible.
- Log timing for long phases.

Status:

- Some indexing was tightened, but this remains a performance-sensitive area.

## 23. Baldi's Basics Plus Access Denied Removing BepInEx

Observed installer error:

```text
Access to the path '...\BepInEx' is denied.
```

Context:

- The selected game was Baldi's Basics Plus under `C:\Program Files (x86)\Steam\steamapps\common`.
- RoseMod install succeeded far enough to install bootstrap/runtime, then failed while trying to move `BepInEx`.

Fix direction:

- Treat BepInEx removal as optional cleanup.
- Continue RoseMod install when removal fails.
- Advise running as Administrator only if the user really wants the folder moved.

Status:

- Documented as installer behavior. Permission failures should not block core RoseMod installation.

## 24. Program Compatibility Assistant Popup

Observed behavior:

- Windows showed "This program might not have installed correctly."

Cause:

- Windows heuristics can flag installer-like EXEs that do not use a normal Windows installer manifest/flow.

Fix direction:

- Use app manifest metadata.
- Make install success clear in the GUI/backend output.
- Prefer Tauri GUI for consumer install.

Status:

- Documented as a Windows UX issue, not a loader runtime failure.

## 25. BepInEx Patchers Were Missing

Problem:

- BepInEx-style patchers had no proper place in RoseMod.

Expected behavior:

- Add `RoseMod/Patchers`.
- Support common BepInEx 5 patcher entry patterns.
- Support older mods/tools that expect `BepInEx/patchers`.

Status:

- `RoseMod/Patchers` exists.
- `BepInEx/patchers` compatibility view is created when no real BepInEx install owns the folder.

## 26. Patcher Redirect Shortcut Did Not Work

Issue:

- A normal shortcut is not enough because tools and patchers expect a real folder path.

Fix direction:

- Use a directory junction where possible.
- Fall back to creating a real compatibility folder.

Status:

- `RoseModPaths.EnsureLegacyPatcherView` and installer logic attempt a junction first.

## 27. BepInEx 5 Plugin Serialization Failure

Observed from Baldi's Basics Plus style mods:

- Some BepInEx 5 Mono mods depend on plugin `MonoBehaviour` types being serializable or cloneable by Unity.

Fix direction:

- Add a serialization fallback for plugin-defined `MonoBehaviour` types.
- Patch Unity clone/instantiate paths enough to copy managed fields when Unity does not serialize them normally.

Status:

- `RoseModSerializationFallback` is part of the startup chain.

## 28. Build Warnings and "Skipped Necessary Things"

Problem:

- Build output and runtime logs showed warnings that represented real missing compatibility, not harmless noise.

Rule added:

- A warning is not harmless if it means a required callback, interop assembly, facade type, or patcher path failed.

Status:

- The wiki now separates harmless warnings from compatibility blockers.

## 29. Build Reference Bugs

Known build failures:

```text
BepInEx.BaseUnityPlugin does not exist
PluginInfo.Type does not exist
Cannot implicitly convert type 'object' to 'BepInEx.BaseUnityPlugin'
Unity physics types missing
```

Causes:

- MSBuild resolving stale generated payload DLLs before fresh facade outputs.
- Facade type shapes drifting away from expected BepInEx API shapes.
- Missing Unity reference assemblies.

Status:

- Build guide documents the reference order and required Unity assemblies.

## 30. Electron to Tauri Rewrite

Problem:

- The installer started as Electron-style UI work, but the request changed to Tauri.

Fix direction:

- Keep the frontend app model.
- Replace the shell with Tauri/Rust.
- Keep C# CLI backend for install operations.

Status:

- `TauriInstaller/` is the current GUI.
- Old `ElectronInstaller/` is historical/stale and should not be treated as the current UI.

## 31. GitHub, Release, and Wiki Issues

Problems:

- GitHub CLI was not installed.
- The repo moved from `MelonCompat` to `MelonCompat-and-RoseMod`.
- The GitHub Wiki could not be pushed until the first page was created in the browser.

Status:

- Repo docs were pushed.
- The real GitHub Wiki was initialized and then populated with the full page set.

## 32. Rename From UniWork to RoseMod

Requirement:

- Rename the framework from UniWork to RoseMod.
- Put the RoseMod logo next to MelonCompat.
- Keep historical references only where documenting the old name.

Status:

- Current source uses RoseMod names.
- Wiki history still mentions UniWork where it explains old logs and old errors.

## Compatibility Lesson

The main lesson is that "loads without crashing" is not enough. A compatibility framework needs to pass each of these levels:

1. The game starts.
2. The loader starts.
3. Required support assemblies resolve.
4. The mod assembly resolves.
5. The mod instantiates.
6. The mod initialization callback runs.
7. Harmony or patcher changes apply.
8. Scene/update callbacks run in the expected order.
9. Assets and game classes resolve.
10. The visible mod behavior matches the real loader.

For Baldi Helps Granny, the custom title screen and gameplay behavior are the practical acceptance checks.
