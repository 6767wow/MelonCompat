namespace RoseMod.DevKit;

public sealed class RoseServices
{
    private readonly Dictionary<Type, object> services = new();

    public void Add<T>(T service)
        where T : class
    {
        services[typeof(T)] = service;
    }

    public bool TryGet<T>(out T service)
        where T : class
    {
        if (services.TryGetValue(typeof(T), out var value) && value is T typed)
        {
            service = typed;
            return true;
        }

        service = null!;
        return false;
    }

    public T GetOrAdd<T>(Func<T> factory)
        where T : class
    {
        if (TryGet<T>(out var service))
            return service;

        service = factory();
        Add(service);
        return service;
    }
}
