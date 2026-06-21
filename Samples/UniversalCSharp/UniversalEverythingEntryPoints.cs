using RoseMod.DevKit;

#if MELONLOADER
using MelonLoader;

[assembly: MelonInfo(typeof(RoseMod.Samples.UniversalCSharp.UniversalEverythingMelon), "Universal CSharp Everything Sample", "1.0.0", "RoseMod")]
[assembly: MelonGame(null, null)]
#endif

#if BEPINEX_MONO
using BepInEx;
using BepInEx.Unity.Mono;
#endif

#if BEPINEX_IL2CPP
using BepInEx;
using BepInEx.Unity.IL2CPP;
#endif

#if UNITY_REFERENCES
using UnityEngine;
using UnityEngine.SceneManagement;
#endif

namespace RoseMod.Samples.UniversalCSharp;

internal static class UniversalEverythingBootstrap
{
    internal static UniversalEverythingMod Load(RoseLoaderKind loader)
    {
        return RoseModHost.Load<UniversalEverythingMod>(loader);
    }
}

#if MELONLOADER
public sealed class UniversalEverythingMelon : MelonMod
{
    public override void OnInitializeMelon() => UniversalEverythingBootstrap.Load(RoseLoaderKind.MelonLoader);
    public override void OnApplicationStart() => RoseModHost.Start<UniversalEverythingMod>();
    public override void OnUpdate() => RoseModHost.Update<UniversalEverythingMod>();
    public override void OnFixedUpdate() => RoseModHost.FixedUpdate<UniversalEverythingMod>();
    public override void OnLateUpdate() => RoseModHost.LateUpdate<UniversalEverythingMod>();
    public override void OnGUI() => RoseModHost.Gui<UniversalEverythingMod>();
    public override void OnSceneWasLoaded(int buildIndex, string sceneName) => RoseModHost.SceneLoaded<UniversalEverythingMod>(buildIndex, sceneName);
    public override void OnSceneWasUnloaded(int buildIndex, string sceneName) => RoseModHost.SceneUnloaded<UniversalEverythingMod>(buildIndex, sceneName);
    public override void OnApplicationQuit() => RoseModHost.ApplicationQuit<UniversalEverythingMod>();
    public override void OnDeinitializeMelon() => RoseModHost.Unload<UniversalEverythingMod>();
}
#endif

#if BEPINEX_MONO
[BepInPlugin("com.rosemod.samples.universal-csharp", "Universal CSharp Everything Sample", "1.0.0")]
public sealed class UniversalEverythingBepInExMono : BaseUnityPlugin
{
    private void Awake()
    {
        UniversalEverythingBootstrap.Load(RoseLoaderKind.BepInEx);
#if UNITY_REFERENCES
        SceneManager.sceneLoaded += OnSceneLoaded;
        SceneManager.sceneUnloaded += OnSceneUnloaded;
#endif
    }

    private void Start() => RoseModHost.Start<UniversalEverythingMod>();
    private void Update() => RoseModHost.Update<UniversalEverythingMod>();
    private void FixedUpdate() => RoseModHost.FixedUpdate<UniversalEverythingMod>();
    private void LateUpdate() => RoseModHost.LateUpdate<UniversalEverythingMod>();
    private void OnGUI() => RoseModHost.Gui<UniversalEverythingMod>();
    private void OnApplicationQuit() => RoseModHost.ApplicationQuit<UniversalEverythingMod>();

    private void OnDestroy()
    {
#if UNITY_REFERENCES
        SceneManager.sceneLoaded -= OnSceneLoaded;
        SceneManager.sceneUnloaded -= OnSceneUnloaded;
#endif
        RoseModHost.Unload<UniversalEverythingMod>();
    }

#if UNITY_REFERENCES
    private static void OnSceneLoaded(Scene scene, LoadSceneMode mode) => RoseModHost.SceneLoaded<UniversalEverythingMod>(scene.buildIndex, scene.name);
    private static void OnSceneUnloaded(Scene scene) => RoseModHost.SceneUnloaded<UniversalEverythingMod>(scene.buildIndex, scene.name);
#endif
}
#endif

#if BEPINEX_IL2CPP
[BepInPlugin("com.rosemod.samples.universal-csharp", "Universal CSharp Everything Sample", "1.0.0")]
public sealed class UniversalEverythingBepInExIl2Cpp : BasePlugin
{
    public override void Load()
    {
        UniversalEverythingBootstrap.Load(RoseLoaderKind.BepInEx);
#if UNITY_REFERENCES
        AddComponent<UniversalEverythingIl2CppBehaviour>();
#endif
    }

    public override bool Unload()
    {
        RoseModHost.Unload<UniversalEverythingMod>();
        return true;
    }
}

#if UNITY_REFERENCES
public sealed class UniversalEverythingIl2CppBehaviour : MonoBehaviour
{
    private void Awake()
    {
        SceneManager.sceneLoaded += OnSceneLoaded;
        SceneManager.sceneUnloaded += OnSceneUnloaded;
    }

    private void Start() => RoseModHost.Start<UniversalEverythingMod>();
    private void Update() => RoseModHost.Update<UniversalEverythingMod>();
    private void FixedUpdate() => RoseModHost.FixedUpdate<UniversalEverythingMod>();
    private void LateUpdate() => RoseModHost.LateUpdate<UniversalEverythingMod>();
    private void OnGUI() => RoseModHost.Gui<UniversalEverythingMod>();
    private void OnApplicationQuit() => RoseModHost.ApplicationQuit<UniversalEverythingMod>();

    private void OnDestroy()
    {
        SceneManager.sceneLoaded -= OnSceneLoaded;
        SceneManager.sceneUnloaded -= OnSceneUnloaded;
    }

    private static void OnSceneLoaded(Scene scene, LoadSceneMode mode) => RoseModHost.SceneLoaded<UniversalEverythingMod>(scene.buildIndex, scene.name);
    private static void OnSceneUnloaded(Scene scene) => RoseModHost.SceneUnloaded<UniversalEverythingMod>(scene.buildIndex, scene.name);
}
#endif
#endif
