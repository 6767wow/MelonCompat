# RoseV

RoseV is a separate all-in-one coding language for Unity mods.

It is not C# syntax. The compiler is a native C/C++/x64 assembly tool located in:

```text
RoseV/Native
```

The first backend emits C# because Unity mod loaders ultimately need managed entrypoints for MelonLoader, BepInEx, and RoseMod. RoseV is still its own source language: `.rosev` files are parsed by the native compiler and translated into normal C# that uses `RoseMod.DevKit`.

The design rule is: simple RoseV commands for common work, built-in imports for Unity and loaders, full C# blocks when you need the complete language, and native companion hooks for C/C++/x64 assembly DLLs.

## Why RoseV Syntax Is Simple

RoseV is designed around common reasons people do not start or continue coding:

- Too much syntax before anything happens.
- Confusing compiler errors.
- Hidden lifecycle rules.
- Too many ways to do the same simple thing.
- Fear of breaking the project.
- Weak mental model of "when does my code run?"
- Setup friction before the first visible result.

RoseV avoids that by making the Unity mod lifecycle explicit:

```rosev
when load {
  say "Loaded {mod}"
}

when update {
  every 300 {
    say "Still running on {loader}"
  }
}
```

No semicolons. No inheritance. No manual logger setup. No loader-specific base class in the RoseV file.

## Current Output

RoseV currently compiles to one C# class:

```text
RoseV source -> native RoseV compiler -> C# class using RoseMod.DevKit
```

That generated class can be used by C# wrappers for:

- RoseMod
- MelonLoader
- BepInEx Mono
- BepInEx IL2CPP

## Build the Native Compiler

From a Visual Studio developer shell:

```bat
msbuild RoseV\Native\RoseV.Native.vcxproj /p:Configuration=Release /p:Platform=x64
```

Output:

```text
bin/RoseV/Release/RoseV.exe
```

## Compile a RoseV File

```bat
bin\RoseV\Release\RoseV.exe compile Samples\RoseV\UniversalEverything.rosev -o Samples\RoseV\Generated\UniversalEverything.generated.cs
```

## Basic Syntax

Metadata:

```rosev
rosev "My Mod" id "com.example.mymod" version "1.0.0" author "Me"
namespace MyMods
class MyMod
import csharp
import unity.core
import melonloader.core
import rosemod
```

Built-in imports:

```rosev
import csharp
import dotnet
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
import everything
import static System.Math
```

Import the pieces you actually use. `import everything` still exists as a shortcut, but it is intentionally not the normal example because most mods do not need every optional Unity package. `import unity everything` is another shortcut if you specifically want the wide Unity namespace pack.

Unity is still C# in RoseV. The imports match real MelonLoader/BepInEx mod style: you write C# that uses `UnityEngine`, `MelonLoader`, `Il2CppInterop`, `HarmonyLib`, and game assemblies when those references exist.

Feature targets:

```rosev
use unity
use melonloader
use bepinex
use rosemod
```

Settings:

```rosev
setting showHud bool true "Show the HUD label"
setting tickRate int 300 "How often to print update messages"
field updateCount int = 0
field score double = 0.0
```

Supported field and setting types:

```text
bool, int, long, short, byte, float, double, string, object
boolean, integer, number, text, any
```

Events:

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

Actions:

```rosev
say "info message"
warn "warning message"
error "error message"
emit "topic" "payload"
let counter = 0
set counter = 10
add counter 1
sub counter 1
mul counter 2
div counter 2
call helper
if counter atleast 10 { }
repeat 3 { }
while counter less 10 { }
try { }
return
throw "Something failed"
every 300 { }
key F8 { }
unity "UnityEngine.Debug.Log(\"raw Unity C# escape hatch\");"
cs "Console.WriteLine(\"raw C# escape hatch\");"
```

Reusable functions:

```rosev
make helper {
  say "Helper was called"
}

when load {
  call helper
}
```

RoseV is meant to feel like easier C#:

- `import System` becomes `using System;`.
- `let count = 0` becomes a local variable.
- `field count int = 0` becomes a persistent class field.
- `set count = 10` assigns a value.
- `add count 1` increments a number.
- `sub`, `mul`, and `div` handle basic numeric changes.
- `if count atleast 10 { }` becomes a normal `if`.
- `repeat`, `while`, and `try` become normal C# control flow.
- `make helper { }` becomes a generated private method.
- `call helper` calls that method.
- `cs "..."` and `csharp { ... }` embed full generated C#.
- `members { ... }` adds full C# fields, methods, properties, classes, and helper code to the generated mod class.
- `unity "..."` is a compatibility shortcut for a C# statement that should only compile when Unity references are present.

## Full C# Inside RoseV

RoseV is not trying to reimplement Roslyn overnight. Instead, full C# is part of RoseV through raw generated C# blocks.

Class-level C#:

```rosev
members {
  private int fullCSharpCounter;

  private void FullCSharpHelper()
  {
    fullCSharpCounter++;
    Log.Info($"Full C# helper ran {fullCSharpCounter} time(s).");
  }
}
```

Event/function-level C#:

```rosev
when load {
  synvert = csharp
  var list = new List<string> { "Unity", "MelonLoader", "BepInEx", "RoseMod" };
  foreach (var item in list)
  {
    Log.Info(item);
  }
  synvert = rosev
}
```

You can also use a braced switch:

```rosev
when load {
  csharp {
    var list = new List<string> { "Unity", "MelonLoader", "BepInEx", "RoseMod" };
    foreach (var item in list)
    {
      Log.Info(item);
    }
  }
}
```

`synvert = unity`, `synvert = melonloader`, `synvert = bepinex`, `synvert = il2cpp`, and `synvert = harmony` are C# syntax modes too. They are labels for the API surface you are working with, not different languages.

That means one `.rosev` file can contain the easy RoseV commands and the whole C# language where advanced code needs it.

## Native C, C++, and Assembly Companions

RoseV can declare native companion source files and generate P/Invoke bridges:

```rosev
native c "Native/RoseVNativeSample.c" as RoseVNativeSample
native cpp "Native/RoseVNativeSample.cpp" as RoseVNativeCpp
native asm "Native/RoseVNativeSample.asm" as RoseVNativeAsm

make optionalNativePing {
  native call RoseVNativeSample.RoseVNativePing
}
```

Build the native file into `RoseVNativeSample.dll` and place it beside the managed mod DLL. Keep Unity object work in C#/RoseV unless you have a specific native reason.

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

## Design References

RoseV's design is based on programming-education and end-user programming research themes, not on a claim that one syntax can solve every learning problem.

Useful references:

- Natural Programming work by Myers, Pane, and others: https://www.cs.cmu.edu/~NatProg/
- Pane and Myers natural programming overview: https://faculty.washington.edu/ajko/papers/Myers2004NaturalProgramming.pdf
- Novice syntax-error reduction with GRAIL: https://www.ppig.org/files/2000-PPIG-12th-mciver.pdf
- Enhanced novice error messages: https://cs.brown.edu/people/sk/Publications/Papers/Published/mfk-measur-effect-error-msg-novice-sigcse/paper.pdf
- Programming anxiety and self-efficacy review: https://mural.maynoothuniversity.ie/id/eprint/10306/1/SB-Role-2016.pdf

RoseV uses those themes this way:

- Natural event words instead of framework class names.
- Small fixed vocabulary.
- Line-numbered parser errors.
- No semicolon/brace soup outside block boundaries.
- Visible lifecycle events.
- Generated C# so advanced users can inspect what happened.
