#if ROSEMOD_MELONLOADER
using MelonLoader;
#endif

#if ROSEMOD_BEPINEX_MONO
using BepInEx.Unity.Mono;
#endif

#if ROSEMOD_BEPINEX_IL2CPP
using BepInEx.Unity.IL2CPP;
#endif

#if ROSEMOD_UNITY_REFERENCES
using UnityEngine.SceneManagement;
#endif

namespace RoseMod.DevKit
{
#if ROSEMOD_MELONLOADER
    public abstract class RoseMelonMod<TMod> : MelonMod
        where TMod : RoseModBase, new()
    {
        public override void OnInitializeMelon() => RoseModEntry<TMod>.Load(RoseLoaderKind.MelonLoader);
        public override void OnApplicationStart() => RoseModEntry<TMod>.Start();
        public override void OnUpdate() => RoseModEntry<TMod>.Update();
        public override void OnFixedUpdate() => RoseModEntry<TMod>.FixedUpdate();
        public override void OnLateUpdate() => RoseModEntry<TMod>.LateUpdate();
        public override void OnGUI() => RoseModEntry<TMod>.Gui();
        public override void OnSceneWasLoaded(int buildIndex, string sceneName) => RoseModEntry<TMod>.SceneLoaded(buildIndex, sceneName);
        public override void OnSceneWasUnloaded(int buildIndex, string sceneName) => RoseModEntry<TMod>.SceneUnloaded(buildIndex, sceneName);
        public override void OnApplicationQuit() => RoseModEntry<TMod>.ApplicationQuit();
        public override void OnDeinitializeMelon() => RoseModEntry<TMod>.Unload();
    }
#endif

#if ROSEMOD_BEPINEX_MONO
    public abstract class RoseBepInExMonoPlugin<TMod> : BaseUnityPlugin
        where TMod : RoseModBase, new()
    {
        protected virtual void Awake()
        {
            RoseModEntry<TMod>.Load(RoseLoaderKind.BepInEx, backend: RoseUnityBackend.Mono);
#if ROSEMOD_UNITY_REFERENCES
            SceneManager.sceneLoaded += OnSceneLoaded;
            SceneManager.sceneUnloaded += OnSceneUnloaded;
#endif
        }

        protected virtual void Start() => RoseModEntry<TMod>.Start();
        protected virtual void Update() => RoseModEntry<TMod>.Update();
        protected virtual void FixedUpdate() => RoseModEntry<TMod>.FixedUpdate();
        protected virtual void LateUpdate() => RoseModEntry<TMod>.LateUpdate();
        protected virtual void OnGUI() => RoseModEntry<TMod>.Gui();
        protected virtual void OnApplicationQuit() => RoseModEntry<TMod>.ApplicationQuit();

        protected virtual void OnDestroy()
        {
#if ROSEMOD_UNITY_REFERENCES
            SceneManager.sceneLoaded -= OnSceneLoaded;
            SceneManager.sceneUnloaded -= OnSceneUnloaded;
#endif
            RoseModEntry<TMod>.Unload();
        }

#if ROSEMOD_UNITY_REFERENCES
        private static void OnSceneLoaded(Scene scene, LoadSceneMode mode) => RoseModEntry<TMod>.SceneLoaded(scene.buildIndex, scene.name);
        private static void OnSceneUnloaded(Scene scene) => RoseModEntry<TMod>.SceneUnloaded(scene.buildIndex, scene.name);
#endif
    }
#endif

#if ROSEMOD_BEPINEX_IL2CPP
    public abstract class RoseBepInExIl2CppPlugin<TMod> : BasePlugin
        where TMod : RoseModBase, new()
    {
        public override void Load() => RoseModEntry<TMod>.Load(RoseLoaderKind.BepInEx, backend: RoseUnityBackend.Il2Cpp);

        public override bool Unload()
        {
            RoseModEntry<TMod>.Unload();
            return true;
        }
    }
#endif
}
