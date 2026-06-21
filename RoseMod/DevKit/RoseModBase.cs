namespace RoseMod.DevKit;

public abstract class RoseModBase
{
    private RoseModContext? context;

    public RoseModContext Context => context ?? throw new InvalidOperationException("The Rose mod has not been attached to a context yet.");
    public RoseLog Log => Context.Logger;
    public RoseConfig Config => Context.Config;
    public RoseEventBus Events => Context.Events;

    internal void Attach(RoseModContext context)
    {
        this.context = context;
    }

    public virtual void OnLoad()
    {
    }

    public virtual void OnStart()
    {
    }

    public virtual void OnUpdate()
    {
    }

    public virtual void OnFixedUpdate()
    {
    }

    public virtual void OnLateUpdate()
    {
    }

    public virtual void OnSceneLoaded(int buildIndex, string sceneName)
    {
    }

    public virtual void OnSceneUnloaded(int buildIndex, string sceneName)
    {
    }

    public virtual void OnGui()
    {
    }

    public virtual void OnApplicationQuit()
    {
    }

    public virtual void OnUnload()
    {
    }
}
