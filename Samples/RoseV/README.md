# RoseV Sample

`UniversalEverything.rosev` shows the beginner-friendly RoseV syntax.

It uses:

- `rosev` metadata
- `use unity`
- `use melonloader`
- `use bepinex`
- `use rosemod`
- settings
- lifecycle events
- scene events
- update events
- timed events with `every`
- keyboard input with `key`
- raw Unity escape code with `unity`

Compile it with:

```bat
bin\RoseV\Release\RoseV.exe compile Samples\RoseV\UniversalEverything.rosev -o Samples\RoseV\Generated\UniversalEverything.generated.cs
```

The generated C# class should be paired with loader wrappers like the ones in:

```text
Samples/UniversalCSharp
```

RoseV is intentionally small. The idea is that a new modder can write:

```rosev
when load {
  say "my mod loaded"
}
```

before learning about C# inheritance, Unity base classes, MelonLoader attributes, or BepInEx plugin metadata.
