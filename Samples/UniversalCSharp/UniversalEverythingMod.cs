using RoseMod.DevKit;

#if UNITY_REFERENCES
using UnityEngine;
#endif

namespace RoseMod.Samples.UniversalCSharp;

[RoseModMetadata(
    "com.rosemod.samples.universal-csharp",
    "Universal CSharp Everything Sample",
    "1.0.0",
    "RoseMod",
    Description = "One shared C# mod core with MelonLoader and BepInEx entrypoints.")]
public sealed class UniversalEverythingMod : RoseModBase
{
    private RoseConfigEntry<bool>? showHud;
    private RoseConfigEntry<int>? tickRate;
    private int ticks;
    private IDisposable? sampleSubscription;

    public override void OnLoad()
    {
        showHud = Config.Bind("General", "ShowHud", true, "Draw a small Unity IMGUI status label.");
        tickRate = Config.Bind("General", "TickRate", 300, "Frames between heartbeat log messages.");
        sampleSubscription = Events.Subscribe("sample.ping", ev => Log.Info("Event bus received: " + ev.Payload));

        Log.Info($"{Context.Metadata} loaded through {Context.Loader}.");
        Log.Info("This one core class can be called from MelonLoader, BepInEx Mono, BepInEx IL2CPP, or RoseMod.");
        Events.Publish("sample.ping", "OnLoad");
    }

    public override void OnStart()
    {
        Log.Info("Unity start callback reached.");
    }

    public override void OnSceneLoaded(int buildIndex, string sceneName)
    {
        Log.Info($"Scene loaded: {sceneName} ({buildIndex}).");
    }

    public override void OnUpdate()
    {
        ticks++;
        var rate = Math.Max(1, tickRate?.Value ?? 300);
        if (ticks % rate == 0)
            Log.Info($"Update heartbeat on {Context.Loader}. Frame bucket: {ticks}.");

#if UNITY_REFERENCES
        if (Input.GetKeyDown(KeyCode.F8))
            Log.Warning("F8 pressed from the universal C# SDK sample.");
#endif
    }

    public override void OnGui()
    {
#if UNITY_REFERENCES
        if (showHud?.Value ?? true)
            GUI.Label(new Rect(20, 48, 620, 24), "Universal C# SDK sample on " + Context.Loader);
#endif
    }

    public override void OnApplicationQuit()
    {
        Log.Info("Application quit callback reached.");
    }

    public override void OnUnload()
    {
        sampleSubscription?.Dispose();
        Config.Save();
        Log.Info("Universal sample unloaded.");
    }
}
