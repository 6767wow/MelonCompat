# Source Code Walkthrough

This page explains the source tree step by step. It is written for someone who opens the repo and wants to know what each major piece does before editing it.

## 1. Repository Root

Important root files:

```text
MelonLoader.BepInExCompat.csproj
MelonLoader.BepInExCompat.Mono.csproj
RoseMod.Core.csproj
RoseMod.MelonLoader.csproj
RoseMod.BepInEx.Core.csproj
RoseMod.BepInEx.Unity.Mono.csproj
RoseMod.BepInEx.Unity.IL2CPP.csproj
RoseMod.BepInEx5.Mono.csproj
RoseMod.Il2CppFixes.csproj
NuGet.config
README.md
```

The root project files build the facade assemblies and runtime assemblies. The repo intentionally has multiple project files because different loader modes need different assembly names and reference sets.

## 2. Normal MelonCompat Shim

Source folder:

```text
BepInExCompat/
```

Purpose:

- Runs inside BepInEx.
- Exposes a compatibility loader for MelonLoader mods.
- Scans the configured plugin/mod folder.
- Loads mod DLLs through the MelonLoader facade.
- Maps MelonLoader-style logging and lifecycle calls onto the BepInEx-hosted environment.

Key files:

```text
BepInExCompat/Plugin.cs
BepInExCompat/Plugin.Mono.cs
BepInExCompat/MelonAssemblyLoader.cs
BepInExCompat/CompatAssemblyResolver.cs
BepInExCompat/InteropNamespaceRewriter.cs
BepInExCompat/ClassInjectorCompatibilityPatches.cs
BepInExCompat/CompatUnityDriver.cs
BepInExCompat/MelonEventPumpBehaviour.cs
BepInExCompat/UnityPhysicsCompatibility.cs
```

Step-by-step flow:

1. BepInEx loads the shim plugin.
2. The shim installs assembly resolution support.
3. It scans `BepInEx/plugins/MelonLoaderMods`.
4. `MelonAssemblyLoader` reads Melon metadata.
5. It checks process/platform attributes.
6. It instantiates `MelonBase` subclasses.
7. It calls initialization lifecycle methods.
8. It applies Harmony autopatches where possible.
9. It logs loaded mod metadata.
10. Unity callbacks are pumped through compatibility drivers.

## 3. MelonLoader API Facade

Source folder:

```text
MelonLoaderApi/
```

Purpose:

- Builds an assembly named `MelonLoader.dll`.
- Provides the public types many MelonLoader mods reference.
- Targets MelonLoader 0.5.7 through 0.7.3 compatibility where practical.

Key files:

```text
MelonLoaderApi/MelonBase.cs
MelonLoaderApi/Attributes.cs
MelonLoaderApi/Logging.cs
MelonLoaderApi/Preferences.cs
MelonLoaderApi/MelonUtils.cs
MelonLoaderApi/MelonEnvironmentCompat.cs
MelonLoaderApi/ExtendedCompatibilityFacade.cs
MelonLoaderApi/HarmonyLegacyFacade.cs
MelonLoaderApi/MelonEventPump.cs
MelonLoaderApi/BuildInfo.cs
```

Step-by-step role:

1. The mod asks the CLR to resolve `MelonLoader`.
2. RoseMod or MelonCompat supplies this facade assembly.
3. Attribute types let metadata load.
4. `MelonBase` gives mods their base class and lifecycle methods.
5. Logging types redirect output into RoseMod or BepInEx logs.
6. Preference APIs provide basic config compatibility.
7. Utility/environment types satisfy common loader queries.

Important rule:

- If a real mod fails with `TypeLoadException` or `MissingMethodException`, this folder is usually the first place to check.

## 4. RoseMod Standalone Runtime

Source folder:

```text
RoseMod/Core/
```

Purpose:

- Runs without BepInEx.
- Starts the managed compatibility environment.
- Loads MelonLoader-style mods and BepInEx-style plugins.
- Coordinates logging, paths, interop, patchers, serialization fallback, and callback bridges.

Key files:

```text
RoseMod/Core/RoseModEntrypoint.cs
RoseMod/Core/RoseModRuntime.cs
RoseMod/Core/RoseModPaths.cs
RoseMod/Core/RoseModAssemblyResolver.cs
RoseMod/Core/RoseModConsole.cs
RoseMod/Core/RoseModLog.cs
RoseMod/Core/RoseModMelonBridge.cs
RoseMod/Core/RoseModBepInExBridge.cs
RoseMod/Core/RoseModPatcherBridge.cs
RoseMod/Core/RoseModIl2CppInteropHost.cs
RoseMod/Core/RoseModIl2CppFixBridge.cs
RoseMod/Core/RoseModSerializationFallback.cs
RoseMod/Core/RoseModUnityThreadBridge.cs
RoseMod/Core/RoseModStartupOptions.cs
```

Startup order:

1. `RoseModEntrypoint` is called by the native host.
2. `RoseModStartupOptions` describes game root and backend.
3. `RoseModPaths` resolves all folders under the game root.
4. `RoseModConsole` configures console visibility and console output.
5. `RoseModLog` starts file logging.
6. `RoseModAssemblyResolver` indexes assemblies.
7. `RoseModIl2CppInteropHost` initializes IL2CPP support when needed.
8. `RoseModIl2CppFixBridge` installs compatibility fixes.
9. `RoseModPatcherBridge` loads patchers.
10. `RoseModSerializationFallback` installs plugin serialization support.
11. `RoseModMelonBridge` loads MelonLoader-style mods.
12. `RoseModBepInExBridge` loads BepInEx-style plugins.

## 5. RoseMod Paths

Runtime folder layout is controlled by:

```text
RoseMod/Core/RoseModPaths.cs
```

Installed layout:

```text
RoseMod/Core
RoseMod/MelonMods
RoseMod/BepInExPlugins
RoseMod/Patchers
RoseMod/interop
RoseMod/Il2CppAssemblies
RoseMod/UserData
RoseMod/UserLibs
RoseMod/Logs
```

This file also creates the legacy patcher compatibility view:

```text
BepInEx/patchers
```

when no real BepInEx install owns the folder.

## 6. BepInEx Facades

Source folder:

```text
RoseMod/BepInExFacade/
```

Purpose:

- Provide BepInEx API names for BepInEx-style plugins.
- Avoid requiring real BepInEx at runtime.

Key files:

```text
RoseMod/BepInExFacade/CoreFacade.cs
RoseMod/BepInExFacade/UnityMonoFacade.cs
RoseMod/BepInExFacade/UnityIl2CppFacade.cs
```

Facade assemblies:

```text
BepInEx.Core.dll
BepInEx.Unity.Mono.dll
BepInEx.Unity.IL2CPP.dll
BepInEx.dll
```

These are compatibility surfaces, not full BepInEx source.

## 7. RoseMod Il2Cpp Fixes

Source folder:

```text
RoseMod/Il2CppFixes/
```

Purpose:

- Apply compatibility fixes around Il2CppInterop behavior.
- Reduce crashes or missing-member failures from mods that use Il2CppInterop APIs.

Important file:

```text
RoseMod/Il2CppFixes/RoseModIl2CppInteropFixes.cs
```

## 8. Native C++ Bootstrap

Source folder:

```text
Native/RoseMod.Native/
```

Purpose:

- Build `winhttp.dll`.
- Load beside the game executable.
- Proxy calls to the real Windows WinHTTP DLL.
- Start Mono host for Mono games.
- Start CoreCLR host for IL2CPP games.

Key files:

```text
Native/RoseMod.Native/src/dllmain.cpp
Native/RoseMod.Native/src/winhttp_proxy.cpp
Native/RoseMod.Native/src/paths.cpp
Native/RoseMod.Native/src/native_log.cpp
Native/RoseMod.Native/src/mono_host.cpp
Native/RoseMod.Native/src/coreclr_host.cpp
Native/RoseMod.Native/src/winhttp_exports.def
```

Step-by-step native flow:

1. Game loads local `winhttp.dll`.
2. `DllMain` starts a background startup thread.
3. Startup detects game root and backend.
4. WinHTTP proxy is initialized.
5. Native log is written.
6. Mono or CoreCLR host starts.
7. Managed RoseMod entrypoint is invoked.

## 9. Installer CLI Backend

Source folder:

```text
Installer/
```

Purpose:

- Detect games and loaders.
- Install the normal MelonCompat shim.
- Install RoseMod.
- Install BepInEx when approved.
- Remove/migrate MelonLoader when approved.
- Import or prepare IL2CPP interop.
- Print a clear install plan.

Key files:

```text
Installer/Program.cs
Installer/InstallerEngine.cs
Installer/BepInExPackageInstaller.cs
Installer/MelonLoaderInstall.cs
Installer/RoseModInteropPreparer.cs
Installer/app.manifest
```

CLI flow:

1. Parse command-line options.
2. Detect Unity game.
3. Print install plan.
4. Run doctor mode if requested.
5. Confirm unless `--yes`.
6. Execute shim or RoseMod install.
7. Report log paths and installed paths.

## 10. Tauri GUI

Source folder:

```text
TauriInstaller/
```

Purpose:

- Provide the consumer installer GUI.
- Scan installed Steam Unity games.
- Show icons/platform/backend/loader state.
- Let users pick MelonLoader DLLs.
- Call the CLI backend.

Important files:

```text
TauriInstaller/src/index.html
TauriInstaller/src/renderer.js
TauriInstaller/src/styles.css
TauriInstaller/src-tauri/src/main.rs
TauriInstaller/src-tauri/tauri.conf.json
```

GUI flow:

1. Frontend calls `scan_games`.
2. Rust/Tauri scans Steam libraries and manual game selection.
3. User selects a game.
4. User installs MelonCompat or RoseMod.
5. Tauri runs the C# backend with no backend console window.
6. Backend output is streamed into the GUI log panel.

## 11. Compatibility Verifier

Source folder:

```text
CompatVerifier/
```

Purpose:

- Check facade coverage against expected APIs and real mod references.
- Catch missing public type/member coverage before manual runtime testing.

Important file:

```text
CompatVerifier/Program.cs
```

Rule:

- The verifier is useful, but a passing verifier does not prove real gameplay compatibility.

## 12. Generated Output Folders

Common generated folders:

```text
bin/
obj/
dist/
Installer/Payload/
TauriInstaller/backend/
TauriInstaller/dist/
```

Important:

- `bin` and `obj` are build products.
- `dist` contains packaged release/source artifacts.
- `Installer/Payload` contains files embedded or copied into installer outputs.
- Generated folders can cause stale reference bugs if MSBuild accidentally resolves DLLs from them.

## Editing Rules

When fixing compatibility:

1. Start from the real error text.
2. Identify whether it is a facade, resolver, lifecycle, patcher, interop, native, or installer bug.
3. Patch the narrow source area.
4. Build the affected projects.
5. Test with a real mod.
6. Check both managed and native logs.
7. Update the wiki if the bug adds a new known failure mode.
