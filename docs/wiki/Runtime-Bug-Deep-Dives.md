# Runtime Bug Deep Dives

This page explains the runtime bugs in the order a loaded game usually hits them.

## Startup Stage

### Native Bootstrap Does Not Start

Symptoms:

- No `RoseMod/Logs/RoseMod.native.log`.
- No `RoseMod/Logs/RoseMod.log`.
- The game starts normally or crashes before managed code.

Likely causes:

- `winhttp.dll` was not placed beside the game executable.
- The game does not import WinHTTP.
- Another loader owns the active bootstrap.
- Antivirus or permissions blocked the native DLL.

Main files:

- `Native/RoseMod.Native/src/dllmain.cpp`
- `Native/RoseMod.Native/src/winhttp_proxy.cpp`
- `Native/RoseMod.Native/src/paths.cpp`

Expected flow:

1. Unity loads `winhttp.dll`.
2. `DllMain` starts `RoseModStartupThread`.
3. RoseMod detects game root and backend.
4. RoseMod forwards WinHTTP calls to the real Windows DLL.
5. RoseMod starts Mono host or CoreCLR host.

### Backend Detection Fails

Symptoms:

```text
Unity backend could not be detected.
```

Likely causes:

- The game layout is unusual.
- `_Data/Managed` does not match normal Mono layout.
- IL2CPP files are missing or renamed.

Fix direction:

- Improve `Native/RoseMod.Native/src/paths.cpp`.
- Improve `Installer/UnityGameDetector`.
- Allow explicit backend override in CLI when detection is wrong.

## Managed Runtime Stage

### RoseMod Managed Runtime Does Not Start

Symptoms:

- Native log exists.
- Managed log is missing or stops before banner.

Likely causes:

- `RoseMod/Core/RoseMod.Core.dll` missing.
- CoreCLR runtime files missing for IL2CPP.
- Mono host path wrong for Mono game.

Main files:

- `RoseMod/Core/RoseModEntrypoint.cs`
- `RoseMod/Core/RoseModRuntime.cs`
- `Native/RoseMod.Native/src/coreclr_host.cpp`
- `Native/RoseMod.Native/src/mono_host.cpp`

### Trusted Platform Assemblies Missing

Symptoms:

```text
Failed to resolve assembly: 'System.Runtime, Version=6.0.0.0'
```

Why it matters:

- Cecil fixups and some runtime reflection paths need .NET platform assemblies.
- Without them, namespace fixups or facade patching can fail before mods load.

Fix implemented:

- `RoseModRuntime.ConfigureTrustedPlatformAssemblies` registers DLLs under the bundled `dotnet` folder.

## Assembly Resolution Stage

### UnityEngine.CoreModule Missing

Symptoms:

```text
Could not load file or assembly 'UnityEngine.CoreModule'
```

Cause:

- The mod references Unity assemblies, but RoseMod cannot resolve them.

Fix direction:

1. Index game managed assemblies.
2. Index `RoseMod/Core`.
3. Index `RoseMod/interop`.
4. Index `RoseMod/Il2CppAssemblies`.
5. Copy or generate IL2CPP interop when needed.

Main files:

- `RoseMod/Core/RoseModAssemblyResolver.cs`
- `Installer/RoseModInteropPreparer.cs`

### Assembly-CSharp Interop Missing

Symptoms:

```text
Assembly-CSharp interop assembly was not found
```

Why it matters:

- Mods that touch game classes need generated game interop.
- Namespace rewriting cannot fix game class references without the target interop assembly.

Fix direction:

- Populate `RoseMod/interop/Assembly-CSharp.dll`.
- Import existing `BepInEx/interop`.
- Generate interop during install when possible.

## MelonLoader Mod Stage

### Mod Instantiates But Initialization Fails

Symptoms:

```text
OnInitializeMelon failed
Skipped <mod> because melon initialization failed
```

Common causes:

- Missing facade type.
- Missing Unity or game interop.
- Il2CppInterop not initialized.
- Mod calls loader-specific internals not covered by RoseMod.

Main files:

- `RoseMod/Core/RoseModMelonBridge.cs`
- `BepInExCompat/MelonAssemblyLoader.cs`
- `MelonLoaderApi/MelonBase.cs`

### Missing Title Screen or Gameplay Changes

Symptoms:

- The mod says it loaded.
- The expected title screen does not appear.
- Game behavior does not match real MelonLoader.

Likely causes:

- Harmony patches did not apply.
- Scene callbacks fired too early or too late.
- Update loop callbacks are missing.
- Asset bundle or resource path differs.
- Game class references were unresolved or rewritten incorrectly.

Acceptance rule:

- For Baldi Helps Granny, the title screen is a real compatibility test, not a cosmetic detail.

## BepInEx Plugin Stage

### BaseUnityPlugin Activation Fails

Symptoms:

- BepInEx plugin DLL is discovered but no behavior appears.

Likely causes:

- `BaseUnityPlugin` facade mismatch.
- Unity component creation failed.
- Plugin type serialization failed.
- Plugin expects exact BepInEx chainloader state.

Main files:

- `RoseMod/Core/RoseModBepInExBridge.cs`
- `RoseMod/BepInExFacade/CoreFacade.cs`
- `RoseMod/BepInExFacade/UnityMonoFacade.cs`

### ManualLogSource Mismatch

Symptoms:

```text
Object of type 'System.String' cannot be converted to type 'BepInEx.Logging.ManualLogSource'
```

Cause:

- Reflection bridge passed the wrong argument type to a facade/API method.

Fix:

- Match real BepInEx logging surface more closely.

## Patcher Stage

### Patcher Folder Not Found

Symptoms:

- A patcher exists under `RoseMod/Patchers`.
- A tool or plugin still expects `BepInEx/patchers`.

Fix:

- Create a junction from `BepInEx/patchers` to `RoseMod/Patchers` when no real BepInEx install owns the folder.
- If junction creation fails, create a real compatibility folder.

Main files:

- `RoseMod/Core/RoseModPaths.cs`
- `Installer/InstallerEngine.cs`

### Cecil Patcher Cannot Replace Already Loaded Assembly

Symptoms:

- Patcher runs.
- Output is written to `RoseMod/PatchedAssemblies`.
- The live game assembly does not change during that launch.

Cause:

- Some Cecil patchers must run before Unity loads the target assembly.

Status:

- RoseMod can run patchers, but it cannot always time-travel and replace an assembly Unity already loaded.

## Callback Stage

### Unity Frame Callback Bridge Unavailable

Symptoms:

```text
Unity frame callback bridge is unavailable; keeping scene callbacks and trying fallback update bridge: Method unstripping failed
```

Cause:

- The preferred frame method could not be patched or unstripped.

Impact:

- Mods that depend on `OnUpdate`, `OnFixedUpdate`, or scene timing may behave incorrectly.

Fallbacks:

- Scene callbacks remain active.
- Managed Harmony callback pumping is attempted.
- Injected event pump can be enabled for debugging.

## Crash Stage

### Crash After Jumpscare, Day Transition, or Scene Change

Symptoms:

- The game runs for a while and then closes.
- The last visible event is a jumpscare, scene transition, or day change.

Likely causes:

- Native detour/trampoline crash.
- Harmony patch unsafe for the Unity version.
- Missing callback cleanup.
- Managed object destroyed while mod still references it.

Debug steps:

1. Check `RoseMod/Logs/RoseMod.native.log`.
2. Check `RoseMod/Logs/RoseMod.log`.
3. Compare the last loaded patch/mod.
4. Disable suspicious patch guards only for debugging.
5. Re-test the same mod in real MelonLoader or real BepInEx.

## Performance Stage

### Loading Screen Hangs

Symptoms:

- The game says "Not Responding" while loading.
- Startup eventually continues, or it crashes.

Likely causes:

- Interop generation or import.
- Recursive assembly indexing.
- Patcher scanning.
- Reflection over many game assemblies.

Fix direction:

- Cache and narrow assembly indexes.
- Defer non-critical work.
- Log phase timings.
- Avoid repeated scans of the same folders.

## Logging Rule

For every runtime bug, collect both logs:

```text
RoseMod/Logs/RoseMod.native.log
RoseMod/Logs/RoseMod.log
```

For normal MelonCompat shim mode, collect:

```text
BepInEx/LogOutput.log
```

No single log file is enough for native plus managed startup bugs.
