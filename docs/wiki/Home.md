# MelonCompat Wiki

MelonCompat has two C# surfaces:

- `RoseMod.DevKit`: a packable C# SDK for one shared mod core with MelonLoader and BepInEx entrypoints.
- `MelonLoader.BepInExCompat`: a BepInEx 6 shim that exposes a `MelonLoader.dll` facade for supported MelonLoader mods.

## Current Scope

- C# only.
- No custom RoseV language.
- No bundled demo mod.
- No native C++ bootstrap or installer payload.
- BepInEx 6 Mono and IL2CPP shim projects remain.
- MelonLoader 0.5.7 through 0.7.3 facade types remain.

## Pages

- [Build Guide](Build-Guide.md)
