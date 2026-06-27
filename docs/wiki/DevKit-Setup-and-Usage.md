# RoseMod.DevKit Setup and Usage

This page explains how to install `RoseMod.DevKit` in Visual Studio 2022, make a C# mod with one shared core class, add loader-specific wrappers for BepInEx or MelonLoader, and understand what the DevKit does at runtime.

## What RoseMod.DevKit Is

`RoseMod.DevKit` is a small C# SDK for Unity mods. It does not replace BepInEx or MelonLoader. Instead, it gives your mod one shared class that can be called by BepInEx Mono, BepInEx IL2CPP, or MelonLoader wrapper classes.

The core idea is:

```text
Loader entrypoint -> RoseMod.DevKit wrapper -> your shared RoseModBase class
```

This avoids trying to make one C# class inherit from `MelonMod`, `BaseUnityPlugin`, and `BasePlugin` at the same time. C# cannot inherit from all of those loader base classes at once, so the DevKit uses composition:

- your real mod logic goes in one `RoseModBase` class
- each loader gets a tiny wrapper class
- the wrapper forwards lifecycle callbacks to your shared mod class

## What The Package Contains

The release package is:

```text
RoseMod.DevKit.0.2.1.nupkg
```

It contains:

- `RoseMod.DevKit.dll`
- `RoseModEntry<TMod>` lifecycle forwarding helpers
- `RoseModHost` for loading and tracking your shared mod instance
- `RoseModBase` for shared lifecycle methods
- `RoseConfig` for simple config files
- `RoseLog` for loader-independent logging
- `RoseEventBus` for simple in-process events
- `buildTransitive` adapter source for optional BepInEx and MelonLoader wrapper base classes

## Step 1. Download The Package

Download `RoseMod.DevKit.0.2.1.nupkg` from the release page:

```text
https://github.com/6767wow/MelonCompat-and-RoseMod/releases/tag/v0.8.2-devkit
```

Do not unzip the file. If you see a `.nuspec` file, you opened the package archive. Go back and use the original `.nupkg` file.

## Step 2. Put The Package In A Local NuGet Folder

Create a local folder for NuGet packages:

```text
C:\Users\<YourUserName>\LocalNuGet\RoseMod
```

Copy the package into that folder so the path looks like this:

```text
C:\Users\<YourUserName>\LocalNuGet\RoseMod\RoseMod.DevKit.0.2.1.nupkg
```

The folder you use as a NuGet source must directly contain the `.nupkg`. Do not select your mod project folder unless the `.nupkg` is directly inside it.

To check from PowerShell:

```powershell
Get-ChildItem "$env:USERPROFILE\LocalNuGet\RoseMod" -Filter "*.nupkg"
```

You should see:

```text
RoseMod.DevKit.0.2.1.nupkg
```

## Step 3. Add The Local NuGet Source In Visual Studio 2022

Open Visual Studio 2022.

Go to:

```text
Tools -> NuGet Package Manager -> Package Manager Settings -> Package Sources
```

Click the `+` button and add:

```text
Name: RoseMod Local
Source: C:\Users\<YourUserName>\LocalNuGet\RoseMod
```

Click `Update` if Visual Studio shows an update button, then click `OK`.

## Step 4. Install The DevKit Package

Right-click your mod project and open:

```text
Manage NuGet Packages
```

At the top-right, change `Package source` to:

```text
RoseMod Local
```

Search:

```text
RoseMod.DevKit
```

Install version:

```text
0.2.1
```

If Visual Studio still says `No packages found`, use Package Manager Console.

Open:

```text
Tools -> NuGet Package Manager -> Package Manager Console
```

Run:

```powershell
Install-Package RoseMod.DevKit -Version 0.2.1 -Source "$env:USERPROFILE\LocalNuGet\RoseMod"
```

If that fails, verify the package is actually in the source folder:

```powershell
Get-ChildItem "$env:USERPROFILE\LocalNuGet\RoseMod" -Filter "RoseMod.DevKit*.nupkg"
```

## Step 5. Add Loader References

`RoseMod.DevKit` gives you the shared SDK. Your project still needs references for the loader you are building against.

If Visual Studio cannot find BepInEx packages, add the BepInEx NuGet feed:

```text
Tools -> NuGet Package Manager -> Package Manager Settings -> Package Sources
```

Add:

```text
Name: BepInEx
Source: https://nuget.bepinex.dev/v3/index.json
```

### BepInEx Mono

For a BepInEx 6 Mono plugin, install or reference:

```text
BepInEx.Unity.Mono
```

Package Manager Console example:

```powershell
Install-Package BepInEx.Unity.Mono -Version 6.0.0-be.764
```

### BepInEx IL2CPP

For a BepInEx 6 IL2CPP plugin, install or reference:

```text
BepInEx.Unity.IL2CPP
```

Package Manager Console example:

```powershell
Install-Package BepInEx.Unity.IL2CPP -Version 6.0.0-be.764
```

IL2CPP mods also usually need references to generated game interop assemblies such as:

```text
BepInEx\interop\UnityEngine.CoreModule.dll
BepInEx\interop\Assembly-CSharp.dll
```

### MelonLoader

For a MelonLoader mod, reference the `MelonLoader.dll` used by the target game/mod loader.

If you are building against a downloaded MelonLoader folder, add a reference to:

```text
MelonLoader.dll
```

The DevKit does not ship MelonLoader itself.

## Step 6. Enable The Correct Build Constants

The DevKit package includes optional wrapper source through NuGet `buildTransitive` files. Those wrapper classes only compile when you enable the matching build constants in the project.

Open your `.csproj` and add the constants for your target.

For BepInEx Mono:

```xml
<PropertyGroup>
  <DefineConstants>$(DefineConstants);ROSEMOD_BEPINEX_MONO;ROSEMOD_UNITY_REFERENCES</DefineConstants>
</PropertyGroup>
```

For BepInEx IL2CPP:

```xml
<PropertyGroup>
  <DefineConstants>$(DefineConstants);ROSEMOD_BEPINEX_IL2CPP</DefineConstants>
</PropertyGroup>
```

For MelonLoader:

```xml
<PropertyGroup>
  <DefineConstants>$(DefineConstants);ROSEMOD_MELONLOADER</DefineConstants>
</PropertyGroup>
```

Use project-wide constants in the `.csproj`. Do not rely on file-local `#define` lines for these symbols, because the DevKit adapter source is compiled as a separate file.

## Step 7. Write The Shared Mod Class

Create a C# file named `MyMod.cs`.

```csharp
using RoseMod.DevKit;

namespace MyCoolMod;

[RoseModMetadata(
    "com.example.mycoolmod",
    "My Cool Mod",
    "1.0.0",
    "YourName",
    Description = "A shared C# mod core that can run from multiple loaders.")]
public sealed class MyMod : RoseModBase
{
    private RoseConfigEntry<bool>? enabled;
    private RoseConfigEntry<int>? logEveryFrames;
    private int frameCount;

    public override void OnLoad()
    {
        enabled = Config.Bind("General", "Enabled", true, "Turns the mod on or off.");
        logEveryFrames = Config.Bind("General", "LogEveryFrames", 300, "How often to log from OnUpdate.");

        Log.Info($"{Context.Metadata} loaded.");
        Log.Info($"Loader: {Context.Loader}");
        Log.Info($"Backend: {Context.Backend}");
        Log.Info($"Game root: {Context.GameRoot}");
    }

    public override void OnStart()
    {
        Log.Info("Unity Start reached.");
    }

    public override void OnUpdate()
    {
        if (enabled?.Value != true)
            return;

        frameCount++;
        var rate = Math.Max(1, logEveryFrames?.Value ?? 300);
        if (frameCount % rate == 0)
            Log.Info($"Update heartbeat: {frameCount}");
    }

    public override void OnSceneLoaded(int buildIndex, string sceneName)
    {
        Log.Info($"Scene loaded: {sceneName} ({buildIndex})");
    }

    public override void OnApplicationQuit()
    {
        Log.Info("Application quitting.");
    }

    public override void OnUnload()
    {
        Config.Save();
        Log.Info("Unloaded.");
    }
}
```

This class is the part you keep shared between loaders.

## Step 8. Add A BepInEx Mono Wrapper

For BepInEx Mono, create `MyModBepInExMono.cs`:

```csharp
using BepInEx;
using RoseMod.DevKit;

namespace MyCoolMod;

[BepInPlugin("com.example.mycoolmod", "My Cool Mod", "1.0.0")]
public sealed class MyModBepInExMono : RoseBepInExMonoPlugin<MyMod>
{
}
```

Build the project and copy the output DLL to:

```text
BepInEx\plugins\MyCoolMod.dll
```

## Step 9. Add A BepInEx IL2CPP Wrapper

For BepInEx IL2CPP, create `MyModBepInExIl2Cpp.cs`:

```csharp
using BepInEx;
using RoseMod.DevKit;

namespace MyCoolMod;

[BepInPlugin("com.example.mycoolmod", "My Cool Mod", "1.0.0")]
public sealed class MyModBepInExIl2Cpp : RoseBepInExIl2CppPlugin<MyMod>
{
}
```

Copy the output DLL to:

```text
BepInEx\plugins\MyCoolMod.dll
```

The base IL2CPP wrapper calls `OnLoad()` and `OnUnload()`. For Unity frame callbacks in IL2CPP, add your own Unity behaviour in the mod project and call the static `RoseModEntry<MyMod>` methods from it:

```csharp
using RoseMod.DevKit;
using UnityEngine;

namespace MyCoolMod;

public sealed class MyModIl2CppBehaviour : MonoBehaviour
{
    private void Start() => RoseModEntry<MyMod>.Start();
    private void Update() => RoseModEntry<MyMod>.Update();
    private void FixedUpdate() => RoseModEntry<MyMod>.FixedUpdate();
    private void LateUpdate() => RoseModEntry<MyMod>.LateUpdate();
    private void OnGUI() => RoseModEntry<MyMod>.Gui();
    private void OnApplicationQuit() => RoseModEntry<MyMod>.ApplicationQuit();
}
```

How you attach that behaviour depends on the target game and the BepInEx IL2CPP APIs available to your project.

## Step 10. Add A MelonLoader Wrapper

For MelonLoader, create `MyModMelonLoader.cs`:

```csharp
using MelonLoader;
using RoseMod.DevKit;

namespace MyCoolMod;

[assembly: MelonInfo(typeof(MyModMelonLoader), "My Cool Mod", "1.0.0", "YourName")]
[assembly: MelonGame(null, null)]

public sealed class MyModMelonLoader : RoseMelonMod<MyMod>
{
}
```

Copy the output DLL to the game's MelonLoader mods folder:

```text
Mods\MyCoolMod.dll
```

## Step 11. Build And Test

In Visual Studio:

```text
Build -> Build Solution
```

Or from PowerShell:

```powershell
dotnet build -c Release
```

After copying the built DLL to the loader's plugin/mod folder, launch the game and check the loader console or log.

You should see a log line like:

```text
My Cool Mod 1.0.0 by YourName loaded.
```

## How The Lifecycle Works

Your shared mod class inherits from `RoseModBase`.

The main callbacks are:

- `OnLoad()`: called once when the loader wrapper initializes the mod.
- `OnStart()`: called from Unity `Start` or loader equivalent.
- `OnUpdate()`: called every Unity frame when the wrapper supports it.
- `OnFixedUpdate()`: called from Unity fixed update.
- `OnLateUpdate()`: called from Unity late update.
- `OnSceneLoaded(int buildIndex, string sceneName)`: called when a scene loads.
- `OnSceneUnloaded(int buildIndex, string sceneName)`: called when a scene unloads.
- `OnGui()`: called from Unity IMGUI.
- `OnApplicationQuit()`: called when the game quits.
- `OnUnload()`: called when the wrapper unloads or is destroyed.

The wrapper classes call `RoseModEntry<TMod>`, which forwards to `RoseModHost`. `RoseModHost` creates your shared mod instance, attaches a `RoseModContext`, catches callback exceptions, and prevents duplicate instances of the same mod class.

## Context, Logging, And Config

Inside your `RoseModBase` class, use:

```csharp
Context.Metadata
Context.Loader
Context.Backend
Context.GameRoot
Log.Info("message")
Config.Bind("Section", "Key", defaultValue, "description")
Events.Publish("topic", payload)
```

Config files are saved under:

```text
<GameRoot>\RoseMod\UserData\<mod id>\<mod id>.cfg
```

Call `Config.Save()` in `OnUnload()` or after changing values.

## Event Bus Example

Use `Events` when different parts of your mod need to talk without hard references:

```csharp
private IDisposable? subscription;

public override void OnLoad()
{
    subscription = Events.Subscribe("example.ready", ev =>
    {
        Log.Info("Event received: " + ev.Payload);
    });

    Events.Publish("example.ready", "hello");
}

public override void OnUnload()
{
    subscription?.Dispose();
}
```

## Service Container Example

Use `Context.Services` for shared objects:

```csharp
public sealed class ScoreTracker
{
    public int Score { get; set; }
}

public override void OnLoad()
{
    var tracker = Context.Services.GetOrAdd(() => new ScoreTracker());
    tracker.Score += 10;
}
```

## Troubleshooting

### Visual Studio says no packages found

The selected package source is not the folder that directly contains the `.nupkg`.

Check:

```powershell
Get-ChildItem "$env:USERPROFILE\LocalNuGet\RoseMod" -Filter "*.nupkg"
```

Then install from that exact folder:

```powershell
Install-Package RoseMod.DevKit -Version 0.2.1 -Source "$env:USERPROFILE\LocalNuGet\RoseMod"
```

### You see a `.nuspec` file

You opened or extracted the `.nupkg`. NuGet needs the original `.nupkg` file. Download the package again and do not extract it.

### `RoseBepInExMonoPlugin` does not exist

Your project is missing the build constant:

```xml
<DefineConstants>$(DefineConstants);ROSEMOD_BEPINEX_MONO;ROSEMOD_UNITY_REFERENCES</DefineConstants>
```

Also make sure the project references `BepInEx.Unity.Mono`.

### `RoseBepInExIl2CppPlugin` does not exist

Your project is missing the build constant:

```xml
<DefineConstants>$(DefineConstants);ROSEMOD_BEPINEX_IL2CPP</DefineConstants>
```

Also make sure the project references `BepInEx.Unity.IL2CPP`.

### `RoseMelonMod` does not exist

Your project is missing the build constant:

```xml
<DefineConstants>$(DefineConstants);ROSEMOD_MELONLOADER</DefineConstants>
```

Also make sure the project references `MelonLoader.dll`.

### Unity types cannot be found

Add references to the target game's Unity assemblies, usually from:

```text
<Game>\BepInEx\interop
<Game>\<GameName>_Data\Managed
```

The exact folder depends on whether the game is Mono or IL2CPP.

### The mod loads but update methods do not run

Check that you are using the wrapper that forwards update callbacks for your loader. BepInEx Mono and MelonLoader wrappers forward common Unity callbacks. BepInEx IL2CPP may need a Unity behaviour that calls `RoseModEntry<TMod>.Update()` and related methods.

## Recommended Project Layout

```text
MyCoolMod/
|-- MyCoolMod.csproj
|-- MyMod.cs
|-- MyModBepInExMono.cs
|-- MyModBepInExIl2Cpp.cs
|-- MyModMelonLoader.cs
```

For a release, it is usually cleaner to build separate projects or configurations per loader, because each loader needs different references and build constants.
