namespace RoseMod.DevKit;

public sealed class RoseModContext
{
    public RoseModContext(
        RoseModMetadata metadata,
        RoseLoaderKind loader,
        RoseUnityBackend backend,
        string gameRoot,
        IRoseLogger logger)
    {
        Metadata = metadata;
        Loader = loader;
        Backend = backend;
        GameRoot = gameRoot;
        UserDataPath = Path.Combine(gameRoot, "RoseMod", "UserData", metadata.Id);
        Logger = new RoseLog(logger);
        Config = new RoseConfig(Path.Combine(UserDataPath, metadata.Id + ".cfg"));
        Events = new RoseEventBus();
        Services = new RoseServices();
    }

    public RoseModMetadata Metadata { get; }
    public RoseLoaderKind Loader { get; }
    public RoseUnityBackend Backend { get; }
    public string GameRoot { get; }
    public string UserDataPath { get; }
    public RoseLog Logger { get; }
    public RoseConfig Config { get; }
    public RoseEventBus Events { get; }
    public RoseServices Services { get; }

    public bool IsRunningOn(RoseLoaderKind loader) => Loader.HasFlag(loader);
}
