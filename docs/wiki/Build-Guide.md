# Build Guide

## Requirements

- .NET SDK 8 or newer.
- NuGet sources from `NuGet.config`.
- Unity/game interop assemblies when building IL2CPP facade projects.

## DevKit Package

```powershell
dotnet build RoseMod.DevKit.csproj -c Release
dotnet pack RoseMod.DevKit.csproj -c Release
```

The package output is written under:

```text
bin/RoseMod/DevKit/Release
```

## Compatibility Shims

```powershell
dotnet build MelonLoader.BepInExCompat.csproj -c Release
dotnet build MelonLoader.BepInExCompat.Mono.csproj -c Release
```

For IL2CPP projects, pass `GameInteropPath` when the default local game path is not available:

```powershell
dotnet build MelonLoader.BepInExCompat.csproj -c Release /p:GameInteropPath="D:\Games\Game\BepInEx\interop"
```

## Facades

```powershell
dotnet build RoseMod.BepInEx.Core.csproj -c Release
dotnet build RoseMod.BepInEx.Unity.Mono.csproj -c Release
dotnet build RoseMod.BepInEx.Unity.IL2CPP.csproj -c Release
dotnet build RoseMod.MelonLoader.csproj -c Release /p:GameInteropPath="D:\Games\Game\BepInEx\interop"
```

## Verifier

```powershell
dotnet build CompatVerifier/CompatVerifier.csproj -c Release
dotnet run --project CompatVerifier/CompatVerifier.csproj -c Release
```
