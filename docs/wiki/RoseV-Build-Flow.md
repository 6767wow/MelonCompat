# RoseV Build Flow

RoseV source is not placed directly in a game folder.

The flow is:

```text
YourMod.rosev
  -> RoseV native compiler
  -> generated C#
  -> optional native companion DLLs
  -> dotnet/MSBuild
  -> YourMod.dll
  -> game mod folder
```

## Step 1. Write RoseV

Example:

```rosev
rosev "My Mod" id "com.example.mymod" version "1.0.0" author "Me"
namespace MyMods
class MyMod

import csharp
import unity.core
import melonloader.core
import rosemod

when load {
  say "{mod} loaded"
  csharp {
    Log.Info("Full C# can live in this RoseV file too.");
  }
}
```

## Step 2. Compile to C#

```text
bin/RoseV/Release/RoseV.exe compile MyMod.rosev -o MyMod.generated.cs
```

Output:

```text
MyMod.generated.cs
```

## Step 3. Build a DLL

The generated file is compiled with a normal C# project that references:

```text
RoseMod.DevKit.dll
```

If the generated file uses Unity, MelonLoader, BepInEx, Harmony, Il2Cpp, or optional Unity package APIs, the C# project must also reference the matching assemblies and define the matching symbols:

```text
UNITY_REFERENCES
MELONLOADER
BEPINEX_MONO
BEPINEX_IL2CPP
HARMONY
IL2CPP_REFERENCES
NEWTONSOFT_JSON
UNITY_UI
UNITY_INPUT_SYSTEM
UNITY_TEXTMESHPRO
UNITY_XR
UNITY_TIMELINE
```

Import only what the mod uses. `import everything` exists as an opt-in shortcut, but individual imports are cleaner and make missing references easier to understand. If you define a symbol, you must reference the matching assembly set.

For loader-specific final DLLs, add the correct wrapper:

- MelonLoader wrapper.
- BepInEx Mono wrapper.
- BepInEx IL2CPP wrapper.
- RoseMod direct/shared wrapper.

The sample project shows the basic generated class build:

```text
Samples/RoseV/RoseVGeneratedSample.csproj
```

The C# universal wrapper sample is here:

```text
Samples/UniversalCSharp
```

## Optional Native Companions

RoseV can declare native files:

```rosev
native c "Native/MyNativeCode.c" as MyNativeCode
native cpp "Native/MyNativeCode.cpp" as MyNativeCpp
native asm "Native/MyNativeAsm.asm" as MyNativeAsm
```

That generates C# P/Invoke declarations. You still build each native source into a DLL yourself:

```text
MyNativeCode.dll
MyNativeCpp.dll
MyNativeAsm.dll
```

Place native DLLs beside the managed mod DLL. Do not put heavy Unity-object logic in native code; use native companions for focused work that really needs C/C++/ASM.

## Step 4. Put the DLL in the Right Folder

RoseMod MelonLoader-style mod:

```text
Game/RoseMod/MelonMods/YourMod.dll
```

RoseMod BepInEx-style plugin:

```text
Game/RoseMod/BepInExPlugins/YourMod.dll
```

Real MelonLoader:

```text
Game/Mods/YourMod.dll
```

Real BepInEx:

```text
Game/BepInEx/plugins/YourMod.dll
```

## Current Important Detail

RoseV currently generates the shared mod class, not a complete one-click final DLL with all loader wrappers automatically generated.

That means the next planned build step is:

```text
RoseV project package -> generated C# + selected wrapper -> final DLL
```

Until that is implemented, use the sample wrapper code from:

```text
Samples/UniversalCSharp
```

## Why This Is Split

The split keeps the language simple:

- RoseV stays easy to read.
- Generated C# is inspectable.
- C# wrappers handle loader-specific details.
- The same RoseV logic can be reused across MelonLoader, BepInEx, and RoseMod.
