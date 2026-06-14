# MelonCompat

MelonCompat is a BepInEx 6 compatibility shim for running supported MelonLoader mods in Unity games.

It supports both Unity backends:

- Mono games
- IL2CPP games

It targets MelonLoader mod DLLs made for MelonLoader 0.5.7 through 0.7.3. This is a compatibility layer, not the full MelonLoader runtime, so some mods that depend on deeper MelonLoader internals may still need fixes.

## For Players

Download the release zip, extract it, and run:

```text
MelonCompat Installer 0.7.3.exe
```

The installer scans Steam libraries for Unity games and shows each game's icon, platform, Unity backend, BepInEx status, and MelonLoader status.

Basic flow:

1. Select a Unity game.
2. Click `Melon DLLs` if you want to choose specific MelonLoader mod DLLs.
3. Click `Install`.
4. If BepInEx is missing, the installer asks before downloading and installing the matching BepInEx 6 Mono or IL2CPP package.
5. After installing BepInEx, it launches the game once. Close the game after it reaches the menu so the installer can continue.
6. The installer then adds the MelonCompat shim and copies selected MelonLoader mod DLLs into `BepInEx/plugins/MelonLoaderMods`.

If MelonLoader is already installed, the installer asks before removing it. If you accept, DLLs from the old MelonLoader `Mods` folder are migrated into `BepInEx/plugins/MelonLoaderMods` before MelonLoader files are deleted.

## What Gets Installed

For the selected game, MelonCompat installs:

```text
BepInEx/plugins/MelonLoader.dll
BepInEx/plugins/Mono.Cecil.dll
BepInEx/plugins/MelonLoaderMods/*.dll
```

The `MelonLoader.dll` name is intentional. MelonLoader mods reference an assembly named `MelonLoader`, so the shim uses that identity while running under BepInEx.

## Command Line

The release zip also includes a CLI backend:

```powershell
cli/MelonCompatInstaller.exe --game "D:\Games\GameName\GameName.exe" --install-bepinex --run-game-before-shim --melon "C:\Mods\ExampleMelon.dll" --yes
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

## Compatibility

Supported MelonLoader mod range:

- MelonLoader 0.5.7
- MelonLoader 0.6.x
- MelonLoader 0.7.x through 0.7.3

Supported Unity/BepInEx targets:

- BepInEx 6 Mono
- BepInEx 6 IL2CPP

Implemented compatibility surface:

- `MelonInfo`, `MelonGame`, `MelonProcess`, priority, color, platform, and version attributes
- `MelonMod` and `MelonPlugin` discovery from DLLs in `BepInEx/plugins`
- `OnEarlyInitializeMelon`, `OnInitializeMelon`, `OnApplicationStart`, `OnLateInitializeMelon`
- Unity frame callbacks: `OnUpdate`, `OnFixedUpdate`, `OnLateUpdate`, `OnGUI`
- Scene callbacks: `OnSceneWasLoaded`, `OnSceneWasInitialized`, `OnSceneWasUnloaded`
- Basic managed `MelonCoroutines.Start` and `Stop`
- BepInEx-backed `MelonLogger` output
- Harmony `PatchAll` against loaded melon assemblies
- Harmony patch target fixups for BepInEx IL2CPP interop assemblies that expose game types without the `Il2Cpp.` namespace
- Minimal in-memory `MelonPreferences` and `MelonPrefs`

## Limits

MelonCompat is not a complete replacement for MelonLoader. Mods can still fail if they require exact MelonLoader lifecycle ordering, native hooks, generated interop details, internal MelonLoader classes, or behavior that is not implemented by this shim.

Legacy BepInEx 5 installs are detected but not used. The installer expects BepInEx 6 for both Mono and IL2CPP games.

## For Coders

Build requirements:

- .NET SDK 8 or newer
- Node.js and npm
- Windows for the packaged Electron installer

Build everything:

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

Build outputs:

```text
bin/Release/net6.0/MelonLoader.dll              # IL2CPP shim
bin/Mono/Release/net6.0/MelonLoader.dll         # Mono shim
dist/installer/MelonCompatInstaller.exe         # CLI backend
dist/electron/MelonCompat Installer 0.7.3.exe   # Electron GUI portable installer
```

Source layout:

```text
BepInExCompat/        Shared shim implementation
MelonLoaderApi/       MelonLoader API facade types
Installer/            CLI installer and install engine
ElectronInstaller/    Electron GUI wrapper
```

BepInEx packages are downloaded from BepInEx bleeding-edge builds because BepInEx 6 Unity Mono and IL2CPP packages are distributed there.
