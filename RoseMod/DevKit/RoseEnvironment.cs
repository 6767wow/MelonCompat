namespace RoseMod.DevKit;

public static class RoseEnvironment
{
    public static RoseLoaderKind DetectLoader()
    {
        var loader = RoseLoaderKind.Unknown;
        foreach (var assembly in AppDomain.CurrentDomain.GetAssemblies())
        {
            var name = assembly.GetName().Name ?? string.Empty;
            if (name.Equals("RoseMod.Core", StringComparison.OrdinalIgnoreCase))
                loader |= RoseLoaderKind.RoseMod;
            else if (name.Equals("MelonLoader", StringComparison.OrdinalIgnoreCase))
                loader |= RoseLoaderKind.MelonLoader;
            else if (name.StartsWith("BepInEx", StringComparison.OrdinalIgnoreCase))
                loader |= RoseLoaderKind.BepInEx;
            else if (name.StartsWith("UnityEngine", StringComparison.OrdinalIgnoreCase))
                loader |= RoseLoaderKind.Unity;
        }

        return loader == RoseLoaderKind.Unknown ? RoseLoaderKind.Unity : loader;
    }

    public static RoseUnityBackend DetectBackend()
    {
        var explicitBackend = System.Environment.GetEnvironmentVariable("ROSEMOD_BACKEND");
        if (Enum.TryParse(explicitBackend, ignoreCase: true, out RoseUnityBackend backend) && backend != RoseUnityBackend.Unknown)
            return backend;

        foreach (var assembly in AppDomain.CurrentDomain.GetAssemblies())
        {
            var name = assembly.GetName().Name ?? string.Empty;
            if (name.StartsWith("Il2Cpp", StringComparison.OrdinalIgnoreCase)
                || name.Equals("Il2CppInterop.Runtime", StringComparison.OrdinalIgnoreCase))
            {
                return RoseUnityBackend.Il2Cpp;
            }
        }

        return RoseUnityBackend.Unknown;
    }

    public static string GameRoot
    {
        get
        {
            var explicitRoot = System.Environment.GetEnvironmentVariable("ROSEMOD_GAME_ROOT");
            return string.IsNullOrWhiteSpace(explicitRoot)
                ? Directory.GetCurrentDirectory()
                : Path.GetFullPath(explicitRoot);
        }
    }
}
