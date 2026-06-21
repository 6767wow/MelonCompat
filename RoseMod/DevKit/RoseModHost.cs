namespace RoseMod.DevKit;

public static class RoseModHost
{
    private static readonly Dictionary<Type, RoseModBase> LoadedMods = new();

    public static T Load<T>(
        RoseLoaderKind entryLoader,
        RoseModMetadata? metadata = null,
        IRoseLogger? logger = null,
        RoseUnityBackend backend = RoseUnityBackend.Unknown,
        string? gameRoot = null)
        where T : RoseModBase, new()
    {
        var type = typeof(T);
        if (LoadedMods.TryGetValue(type, out var existing))
            return (T)existing;

        metadata ??= RoseModMetadata.FromType(type);
        var detectedLoader = RoseEnvironment.DetectLoader();
        var detectedBackend = backend == RoseUnityBackend.Unknown ? RoseEnvironment.DetectBackend() : backend;
        gameRoot ??= RoseEnvironment.GameRoot;
        logger ??= new RoseConsoleLogger(metadata.Name);

        var context = new RoseModContext(metadata, detectedLoader | entryLoader, detectedBackend, gameRoot, logger);
        var mod = new T();
        mod.Attach(context);
        LoadedMods[type] = mod;
        Invoke(mod, nameof(RoseModBase.OnLoad), mod.OnLoad);
        return mod;
    }

    public static void Start<T>() where T : RoseModBase => Invoke<T>(mod => mod.OnStart(), nameof(RoseModBase.OnStart));
    public static void Update<T>() where T : RoseModBase => Invoke<T>(mod => mod.OnUpdate(), nameof(RoseModBase.OnUpdate));
    public static void FixedUpdate<T>() where T : RoseModBase => Invoke<T>(mod => mod.OnFixedUpdate(), nameof(RoseModBase.OnFixedUpdate));
    public static void LateUpdate<T>() where T : RoseModBase => Invoke<T>(mod => mod.OnLateUpdate(), nameof(RoseModBase.OnLateUpdate));
    public static void Gui<T>() where T : RoseModBase => Invoke<T>(mod => mod.OnGui(), nameof(RoseModBase.OnGui));
    public static void ApplicationQuit<T>() where T : RoseModBase => Invoke<T>(mod => mod.OnApplicationQuit(), nameof(RoseModBase.OnApplicationQuit));

    public static void SceneLoaded<T>(int buildIndex, string sceneName)
        where T : RoseModBase
    {
        Invoke<T>(mod => mod.OnSceneLoaded(buildIndex, sceneName), nameof(RoseModBase.OnSceneLoaded));
    }

    public static void SceneUnloaded<T>(int buildIndex, string sceneName)
        where T : RoseModBase
    {
        Invoke<T>(mod => mod.OnSceneUnloaded(buildIndex, sceneName), nameof(RoseModBase.OnSceneUnloaded));
    }

    public static void Unload<T>()
        where T : RoseModBase
    {
        var type = typeof(T);
        if (!LoadedMods.TryGetValue(type, out var mod))
            return;

        Invoke(mod, nameof(RoseModBase.OnUnload), mod.OnUnload);
        mod.Context.Config.Save();
        LoadedMods.Remove(type);
    }

    public static bool TryGet<T>(out T mod)
        where T : RoseModBase
    {
        if (LoadedMods.TryGetValue(typeof(T), out var value) && value is T typed)
        {
            mod = typed;
            return true;
        }

        mod = null!;
        return false;
    }

    private static void Invoke<T>(Action<T> callback, string callbackName)
        where T : RoseModBase
    {
        if (!LoadedMods.TryGetValue(typeof(T), out var mod) || mod is not T typed)
            return;

        Invoke(typed, callbackName, () => callback(typed));
    }

    private static void Invoke(RoseModBase mod, string callbackName, Action callback)
    {
        try
        {
            callback();
        }
        catch (Exception ex)
        {
            mod.Log.Error(ex, $"{callbackName} failed.");
        }
    }
}
