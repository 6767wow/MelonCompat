# Universal C# SDK Sample

This sample is the normal C# path.

It uses:

- `RoseMod.DevKit`
- one shared mod class: `UniversalEverythingMod`
- a MelonLoader entrypoint wrapper
- a BepInEx Mono entrypoint wrapper
- a BepInEx IL2CPP entrypoint wrapper
- Unity callbacks behind `UNITY_REFERENCES`

The important idea is composition:

```text
Loader-specific wrapper -> RoseModHost -> UniversalEverythingMod
```

That avoids trying to inherit from `MelonMod`, `BaseUnityPlugin`, and `BasePlugin` at the same time, which C# cannot do.

## Defines

Build with the defines that match the target:

```text
MELONLOADER
BEPINEX_MONO
BEPINEX_IL2CPP
UNITY_REFERENCES
```

For a real release, build separate DLLs per loader/backend unless your reference set is designed to include all facade assemblies safely.

## Why This SDK Exists

RoseV is the beginner language. The C# SDK is for normal C# modders who want one clean lifecycle but still need MelonLoader and BepInEx compatibility.

The SDK gives:

- one lifecycle
- one logger
- one simple config API
- one event bus
- one shared mod class
- small wrapper classes for each loader
