# RoseV Language

RoseV is a separate all-in-one source language for Unity mods.

It is not C# syntax with a new file extension. RoseV source files use `.rosev`, are parsed by the native RoseV compiler, and currently emit generated C# because Unity mod loaders ultimately load managed assemblies.

## Goal

RoseV is meant to feel like C# made easier for Unity modding:

- Fewer symbols.
- Fewer required concepts before the first working mod.
- Explicit lifecycle events.
- Friendly commands for logging, settings, keyboard checks, scene events, and Unity escape code.
- Built-in import packs for C#/.NET, Unity, MelonLoader, BepInEx, RoseMod, Harmony, Il2CppInterop, JSON, and common optional Unity packages.
- Full C# member and statement blocks when friendly RoseV syntax is not enough.
- Native C, C++, and x64 assembly companion declarations.
- Generated C# that advanced users can inspect.

## Why It Emits C#

Unity mod loaders normally load .NET assemblies:

- MelonLoader loads managed mod DLLs.
- BepInEx loads managed plugin DLLs.
- RoseMod loads managed MelonLoader-style and BepInEx-style code.

Because of that, RoseV's first backend is:

```text
.rosev source -> native RoseV compiler -> generated C# -> DLL
```

RoseV remains separate because the language parser, syntax, and compiler are native C/C++/ASM code. The C# is the current Unity backend output.

## Design Reasons

RoseV syntax is based on common barriers in beginner programming and end-user programming:

- People often struggle before seeing a first visible result.
- Syntax punctuation causes many early failures.
- Framework lifecycle rules are hidden.
- Error messages often mention compiler internals instead of user intent.
- Beginners often do not know where to put code.
- People can lose confidence when a tiny punctuation error blocks everything.

RoseV answers those problems with:

- `when load`, `when update`, and `when scene_loaded`.
- No semicolons.
- Small fixed command set.
- Line-based diagnostics.
- Generated C# for transparency.
- IDE snippets for common patterns.

## Hello World

```rosev
rosev "Hello RoseV" id "com.example.hello" version "1.0.0" author "Me"
namespace MyMods
class HelloRoseV

when load {
  say "Hello from {mod}"
}
```

## Metadata

Every file should start with metadata:

```rosev
rosev "Mod Name" id "com.author.modname" version "1.0.0" author "Author"
namespace MyMods
class MyMod
```

This becomes:

- A generated C# namespace.
- A generated class name.
- RoseMod DevKit metadata.

## Loader Targets

RoseV can mark intended targets:

```rosev
use unity
use melonloader
use bepinex
use rosemod
```

These are declarations of intent. Actual loader entrypoint wrappers are still part of the final DLL build path.

## Imports

RoseV supports imports:

```rosev
import csharp
import unity.core
import unity.ai
import unity.ui
import unity.scene
import unity.inputsystem
import unity.textmeshpro
import melonloader.core
import melonloader.utils
import bepinex.core
import bepinex.config
import bepinex.logging
import rosemod
import harmony
import il2cpp.runtime
import il2cpp.injection
import json.linq
import UnityEngine.InputSystem when UNITY_INPUT_SYSTEM
import static System.Math
```

This generates:

```csharp
using System;
using System.Collections.Generic;
#if UNITY_REFERENCES
using UnityEngine;
#endif
```

Important built-in packs:

- `import all`: common C#/.NET plus the basic Unity, MelonLoader, BepInEx, and RoseMod packs.
- `import everything`: opt-in shortcut for C#/.NET plus the broad Unity, MelonLoader, BepInEx, RoseMod, Harmony, Il2CppInterop, standard assets, and JSON packs.
- `import csharp` or `import dotnet`: common .NET namespaces.
- `import unity.core`: `UnityEngine` under `#if UNITY_REFERENCES`.
- `import unity.ai`: `UnityEngine.AI`.
- `import unity.ui`: `UnityEngine.UI` plus EventSystems.
- `import unity.scene`: scene management.
- `import unity.inputsystem`: Input System namespaces under `#if UNITY_INPUT_SYSTEM`.
- `import unity.textmeshpro`: `TMPro` under `#if UNITY_TEXTMESHPRO`.
- `import unity.xr`, `import unity.timeline`, `import unity.vfx`, `import unity.video`, `import unity.tilemaps`, `import unity.addressables`, and similar aliases import those specific optional Unity APIs.
- `import melonloader.core`: MelonLoader.
- `import melonloader.utils`: MelonLoader.Utils.
- `import bepinex.core`, `import bepinex.config`, `import bepinex.logging`, `import bepinex.mono`, and `import bepinex.il2cpp`: individual BepInEx namespaces.
- `import rosemod`: RoseMod DevKit.
- `import harmony`: HarmonyLib under `#if HARMONY`.
- `import il2cpp.runtime`, `import il2cpp.attributes`, `import il2cpp.injection`, and `import il2cpp.arrays`: individual Il2CppInterop namespaces.
- `import json.core` and `import json.linq`: individual Newtonsoft.Json namespaces.
- `import Some.Namespace when SYMBOL`: generic conditional import for anything RoseV does not have a named alias for.

RoseV always emits `using System;` once, then de-duplicates generated imports.

Unity is not a separate syntax. Unity mod code is still C# that imports Unity assemblies, just like MelonLoader mods. The Baldi Helps Granny source uses normal C# imports such as `using UnityEngine;`, `using UnityEngine.AI;`, `using UnityEngine.UI;`, `using MelonLoader;`, `using MelonLoader.Utils;`, `using Il2CppInterop.Runtime;`, and `using HarmonyLib;`. RoseV's import packs generate that style when the matching references and symbols exist.

## Settings

Settings are persistent config entries:

```rosev
setting showHud bool true "Show the HUD"
setting tickRate int 300 "Frames between messages"
setting label string "RoseV" "HUD text"
```

Supported setting types:

- `bool`
- `int`
- `long`
- `short`
- `byte`
- `float`
- `double`
- `string`
- `object`
- aliases: `boolean`, `integer`, `number`, `text`, `any`

## Fields

Fields are persistent values on the generated mod class:

```rosev
field updateCount int = 0
field enabled bool = true
field label string = "RoseV"
```

Use fields when a value needs to survive between updates.

Use `let` when a value is only needed inside one event or function.

## Events

Events describe when code runs.

```rosev
when load { }
when start { }
when update { }
when fixed_update { }
when late_update { }
when scene_loaded scene { }
when scene_unloaded scene { }
when gui { }
when quit { }
when unload { }
```

## Logging

```rosev
say "Normal message"
warn "Warning message"
error "Error message"
```

Text replacements:

```text
{loader}
{backend}
{game}
{mod}
{version}
{scene}
{buildIndex}
```

## Variables

Local variables:

```rosev
let shouldRun = true
let name = "Rose"
let amount = 5
```

Assignments:

```rosev
set shouldRun = false
set name = "New Name"
```

Increment:

```rosev
add updateCount 1
sub updateCount 1
mul updateCount 2
div updateCount 2
```

## Conditions

```rosev
if shouldRun is true {
  say "Running"
}

if updateCount atleast 300 {
  say "300 updates"
}
```

Friendly comparison words:

- `is` becomes `==`
- `isnt` becomes `!=`
- `more` becomes `>`
- `less` becomes `<`
- `atleast` becomes `>=`
- `atmost` becomes `<=`

## Functions

Reusable functions use `make`:

```rosev
make announce {
  say "Function called"
}

when load {
  call announce
}
```

Generated C#:

```csharp
private void announce()
{
    Log.Info(...);
}
```

## Timing

Run code every N update calls:

```rosev
when update {
  every 300 {
    say "Still alive"
  }
}
```

Repeat a block immediately:

```rosev
repeat 3 {
  say "Runs three times"
}
```

Normal loop and guarded failure handling:

```rosev
while updateCount less 10 {
  add updateCount 1
}

try {
  say "If this throws, RoseV logs the exception."
}
```

## Keyboard

```rosev
when update {
  key F8 {
    warn "F8 pressed"
  }
}
```

This requires Unity references in the generated C# build.

## Full C# Inside RoseV

RoseV supports the whole C# language through generated C# blocks. This is how RoseV can stay simple without blocking advanced Unity or loader code.

Switch syntax with `synvert`:

```rosev
when load {
  say "RoseV mode"
  synvert = csharp
  var list = new List<string> { "Unity", "MelonLoader", "BepInEx" };
  foreach (var value in list)
  {
    Log.Info(value);
  }
  synvert = rosev
  say "Back in RoseV mode"
}
```

These are all C# syntax modes:

```rosev
synvert = csharp
synvert = cs
synvert = unity
synvert = melonloader
synvert = bepinex
synvert = il2cpp
synvert = harmony
```

`synvert = unity` does not create a different language. It means "write normal C# here while working with Unity APIs."

```rosev
cs "Console.WriteLine(\"raw C#\");"

csharp {
  var values = new List<string> { "Unity", "MelonLoader", "BepInEx", "RoseMod" };
  foreach (var value in values)
  {
    Log.Info(value);
  }
}
```

Add fields, properties, methods, nested classes, or helper code to the generated class:

```rosev
members {
  private int fullCSharpCounter;

  private void FullCSharpHelper()
  {
    fullCSharpCounter++;
    Log.Info($"Full C# helper ran {fullCSharpCounter} time(s).");
  }
}

when load {
  csharp {
    FullCSharpHelper();
  }
}
```

Unity-guarded one-line C# is still available:

```rosev
unity "UnityEngine.Debug.Log(\"raw Unity call\");"
```

`unity` blocks are wrapped in:

```csharp
#if UNITY_REFERENCES
...
#endif
```

## Native C, C++, and Assembly

Declare native companions:

```rosev
native c "Native/RoseVNativeSample.c" as RoseVNativeSample
native cpp "Native/RoseVNativeSample.cpp" as RoseVNativeCpp
native asm "Native/RoseVNativeSample.asm" as RoseVNativeAsm
```

Call exported native functions:

```rosev
make optionalNativePing {
  native call RoseVNativeSample.RoseVNativePing
}
```

The alias should match the DLL name without `.dll`. The generated C# uses P/Invoke, so C++ exports should use `extern "C"`.

## Current Limitations

RoseV now has a path to the whole C# language, but it still does not yet have:

- Full expression parsing.
- User-defined parameters for functions.
- Lists/dictionaries as first-class syntax.
- Automatic final DLL packaging.
- Automatic loader wrapper generation.
- Rich type checking before C# generation.
- Automatic native DLL compilation.

Those can be added later without changing the basic event/action style.
