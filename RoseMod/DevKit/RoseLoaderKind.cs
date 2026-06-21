namespace RoseMod.DevKit;

[Flags]
public enum RoseLoaderKind
{
    Unknown = 0,
    RoseMod = 1,
    MelonLoader = 2,
    BepInEx = 4,
    Unity = 8
}

public enum RoseUnityBackend
{
    Unknown,
    Mono,
    Il2Cpp
}

public enum RoseLogLevel
{
    Debug,
    Info,
    Warning,
    Error
}
