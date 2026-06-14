# MelonLoader BepInEx IL2CPP Compatibility Shim

This project builds BepInEx 6 compatibility shims named `MelonLoader.dll`.
The assembly name is intentional: MelonLoader mods reference an assembly named `MelonLoader`, so this shim uses that identity and exposes a practical MelonLoader 0.7.3 API facade.

## Build

```powershell
dotnet build MelonLoader.BepInExCompat.csproj -c Release
dotnet build MelonLoader.BepInExCompat.Mono.csproj -c Release
dotnet build Installer/MelonCompatInstaller.csproj -c Release
dotnet publish Installer/MelonCompatInstaller.csproj -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -o dist/installer
dotnet publish Installer/MelonCompatInstaller.csproj -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -o ElectronInstaller/backend
pushd ElectronInstaller
npm install
npm run build
popd
```

Output:

```text
bin/Release/net6.0/MelonLoader.dll              # IL2CPP shim
bin/Mono/Release/net6.0/MelonLoader.dll         # Mono shim
dist/installer/MelonCompatInstaller.exe         # self-contained Windows installer
dist/electron/MelonCompat Installer 0.7.3.exe   # Electron GUI portable installer
```

## Installer

The Electron installer uses a neutral gray UI with the MelonCompat logo. It scans Steam libraries for Unity games, shows each game icon, platform, Unity backend, BepInEx status, and MelonLoader status.

If BepInEx is missing, the GUI asks before downloading and extracting the matching BepInEx 6 Unity Mono or IL2CPP package. It then launches the game once, waits for the game to close, and installs the MelonCompat shim after BepInEx has had its first run.

If MelonLoader is detected, the GUI asks before removing it. When removal is accepted, DLLs from the MelonLoader `Mods` folder are migrated into `BepInEx/plugins/MelonLoaderMods` before the MelonLoader files are deleted.

Manual CLI installs are still available through the backend installer:

```powershell
dist/installer/MelonCompatInstaller.exe --game "D:\Games\GameName\GameName.exe" --install-bepinex --run-game-before-shim --melon "C:\Mods\ExampleMelon.dll" --yes
```

Useful options:

```text
--backend <auto|mono|il2cpp>
--install-bepinex          Download and install BepInEx 6 when missing.
--bepinex-zip <zip>        Install BepInEx 6 from a local zip.
--run-game-before-shim     Launch the game once before installing the shim after a BepInEx install.
--remove-melonloader       Remove an existing MelonLoader install.
--migrate-melon-mods       Migrate DLLs from MelonLoader/Mods before removal.
--force-payload            Replace existing BepInEx/plugins/MelonLoader.dll.
--dry-run                  Detect and print the plan without writing files.
```

BepInEx packages are downloaded from BepInEx bleeding-edge builds because BepInEx 6 Unity Mono and IL2CPP packages are distributed there.

The BepInEx console/log should show lines like:

```text
Loaded MelonMod: Example Mod v1.0.0 by Author -> ExampleMod.dll
```

## Supported

- Best-effort MelonLoader 0.5.7-0.7.3 assembly/API facade with assembly identity `MelonLoader, Version=0.7.3.0`
- BepInEx 6 Mono and BepInEx 6 IL2CPP backend-specific shim builds
- `MelonInfo`, `MelonGame`, `MelonProcess`, priority, color, platform, version attributes
- `MelonMod` / `MelonPlugin` discovery from DLLs in `BepInEx/plugins`
- `OnEarlyInitializeMelon`, `OnInitializeMelon`, `OnApplicationStart`, `OnLateInitializeMelon`
- Unity frame callbacks: `OnUpdate`, `OnFixedUpdate`, `OnLateUpdate`, `OnGUI`
- Scene callbacks: `OnSceneWasLoaded`, `OnSceneWasInitialized`, `OnSceneWasUnloaded`
- Basic managed `MelonCoroutines.Start` / `Stop` support for frame-driven enumerators
- BepInEx-backed `MelonLogger` output
- Harmony `PatchAll` against loaded melon assemblies
- Harmony patch target fixups for BepInEx IL2CPP interop assemblies that expose game types without the `Il2Cpp.` namespace
- Minimal in-memory `MelonPreferences` / `MelonPrefs`

## Limits

This is a compatibility shim, not the full MelonLoader runtime. It requires BepInEx 6; legacy BepInEx 5 Mono installs are detected but not used. Coroutine support is a managed frame-pumped scheduler and does not reproduce every Unity coroutine yield instruction. Mods that depend on deeper MelonLoader internals, native hook behavior, generated interop details, or exact loader lifecycle ordering may still need more compatibility work.
