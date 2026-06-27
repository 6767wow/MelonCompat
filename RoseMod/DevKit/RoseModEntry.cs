namespace RoseMod.DevKit;

public static class RoseModEntry<TMod>
    where TMod : RoseModBase, new()
{
    public static TMod Load(
        RoseLoaderKind entryLoader,
        RoseModMetadata? metadata = null,
        IRoseLogger? logger = null,
        RoseUnityBackend backend = RoseUnityBackend.Unknown,
        string? gameRoot = null)
    {
        return RoseModHost.Load<TMod>(entryLoader, metadata, logger, backend, gameRoot);
    }

    public static void Start() => RoseModHost.Start<TMod>();
    public static void Update() => RoseModHost.Update<TMod>();
    public static void FixedUpdate() => RoseModHost.FixedUpdate<TMod>();
    public static void LateUpdate() => RoseModHost.LateUpdate<TMod>();
    public static void Gui() => RoseModHost.Gui<TMod>();
    public static void ApplicationQuit() => RoseModHost.ApplicationQuit<TMod>();
    public static void SceneLoaded(int buildIndex, string sceneName) => RoseModHost.SceneLoaded<TMod>(buildIndex, sceneName);
    public static void SceneUnloaded(int buildIndex, string sceneName) => RoseModHost.SceneUnloaded<TMod>(buildIndex, sceneName);
    public static void Unload() => RoseModHost.Unload<TMod>();
    public static bool TryGet(out TMod mod) => RoseModHost.TryGet(out mod);
}
