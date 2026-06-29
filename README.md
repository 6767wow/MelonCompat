# MelonCompat and RoseMod/RoseMod.DevKit

MelonCompat is a C# compatibility shim for running supported MelonLoader mods under BepInEx 6.

RoseMod.DevKit is the C# package for writing a shared Unity mod core and calling it from MelonLoader or BepInEx entrypoints. RoseMod is no longer a custom language, bundled sample mod, native bootstrap, or installer payload.

## Projects

- `RoseMod.DevKit.csproj`: packable C# SDK for shared mod lifecycle, logging, config, metadata, and loader detection.
- `MelonLoader.BepInExCompat.csproj`: IL2CPP BepInEx 6 shim that exposes a `MelonLoader.dll` facade.
- `MelonLoader.BepInExCompat.Mono.csproj`: Mono BepInEx 6 shim that exposes a `MelonLoader.dll` facade.
- `RoseMod.BepInEx.*.csproj`: C# BepInEx facade assemblies used by the compatibility surface.
- `RoseMod.MelonLoader.csproj`: C# MelonLoader facade assembly used by the compatibility surface.
- `CompatVerifier/`: Cecil-based verifier for facade coverage.

## C# Package

Build the DevKit package:

```powershell
dotnet pack RoseMod.DevKit.csproj -c Release
```

The package contains:

- `RoseMod.DevKit.dll`
- transitive C# adapter source for MelonLoader and BepInEx wrappers
- shared lifecycle helpers through `RoseModEntry<TMod>` and `RoseModHost`

Consumer projects enable wrapper source with project-wide build constants:

```xml
<PropertyGroup>
  <DefineConstants>$(DefineConstants);ROSEMOD_MELONLOADER</DefineConstants>
</PropertyGroup>
```

Use `ROSEMOD_BEPINEX_MONO`, `ROSEMOD_BEPINEX_IL2CPP`, and `ROSEMOD_UNITY_REFERENCES` the same way for BepInEx builds. Do not rely on file-local `#define` lines for these symbols; the adapter source is compiled as a separate file.

## Shared Mod Core

```csharp
using RoseMod.DevKit;

[RoseModMetadata("com.example.coolmod", "Cool Mod", "1.0.0", "You")]
public sealed class CoolMod : RoseModBase
{
    public override void OnLoad()
    {
        Log.Info($"{Context.Metadata} loaded through {Context.Loader}.");
    }

    public override void OnUpdate()
    {
        // Shared update logic for MelonLoader and BepInEx wrappers.
    }
}
```

## MelonLoader Wrapper

```csharp
using MelonLoader;
using RoseMod.DevKit;

[assembly: MelonInfo(typeof(CoolMelonEntry), "Cool Mod", "1.0.0", "You")]
[assembly: MelonGame(null, null)]

public sealed class CoolMelonEntry : RoseMelonMod<CoolMod>
{
}
```

## BepInEx Mono Wrapper

```csharp
using BepInEx;
using RoseMod.DevKit;

[BepInPlugin("com.example.coolmod", "Cool Mod", "1.0.0")]
public sealed class CoolBepInExMonoEntry : RoseBepInExMonoPlugin<CoolMod>
{
}
```

## BepInEx IL2CPP Wrapper

```csharp
using BepInEx;
using RoseMod.DevKit;

[BepInPlugin("com.example.coolmod", "Cool Mod", "1.0.0")]
public sealed class CoolBepInExIl2CppEntry : RoseBepInExIl2CppPlugin<CoolMod>
{
}
```

For IL2CPP frame and scene callbacks, add a concrete Unity behaviour in the mod project and call `RoseModEntry<CoolMod>.Update()`, `SceneLoaded(...)`, and the other lifecycle helpers from that behaviour.

## MelonCompat Shim

Build the BepInEx-powered MelonLoader facade:

```powershell
dotnet build MelonLoader.BepInExCompat.csproj -c Release
dotnet build MelonLoader.BepInExCompat.Mono.csproj -c Release
```

Install the matching output as `BepInEx/plugins/MelonLoader.dll`, then place MelonLoader mods under:

```text
BepInEx/plugins/MelonLoaderMods
```

The `MelonLoader.dll` assembly name is intentional because MelonLoader mods reference that assembly identity.

## Build

Managed build commands:

```powershell
dotnet build RoseMod.DevKit.csproj -c Release
dotnet pack RoseMod.DevKit.csproj -c Release
dotnet build MelonLoader.BepInExCompat.csproj -c Release
dotnet build MelonLoader.BepInExCompat.Mono.csproj -c Release
dotnet build RoseMod.BepInEx.Core.csproj -c Release
dotnet build RoseMod.BepInEx.Unity.Mono.csproj -c Release
dotnet build RoseMod.BepInEx.Unity.IL2CPP.csproj -c Release
dotnet build RoseMod.MelonLoader.csproj -c Release
dotnet build CompatVerifier/CompatVerifier.csproj -c Release
```

`RoseMod.MelonLoader.csproj` and the IL2CPP shim need Unity/game interop assemblies. Pass `GameInteropPath` when the defaults do not exist:

```powershell
dotnet build RoseMod.MelonLoader.csproj -c Release /p:GameInteropPath="D:\Games\Game\BepInEx\interop"
```

## Compatibility

MelonCompat targets MelonLoader mods built for MelonLoader 0.5.7 through 0.7.3 and BepInEx 6 Mono or IL2CPP games. It is still a compatibility facade, not the full MelonLoader runtime.

## Documentation

The trimmed wiki source starts at [`docs/wiki/Home.md`](docs/wiki/Home.md).

For step-by-step Visual Studio setup, local NuGet source setup, wrapper classes, lifecycle callbacks, and troubleshooting, read [`docs/wiki/DevKit-Setup-and-Usage.md`](docs/wiki/DevKit-Setup-and-Usage.md).
