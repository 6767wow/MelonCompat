namespace RoseMod.DevKit;

public sealed class RoseEventBus
{
    private readonly Dictionary<string, List<Action<RoseEvent>>> handlers = new(StringComparer.OrdinalIgnoreCase);

    public IDisposable Subscribe(string topic, Action<RoseEvent> handler)
    {
        if (!handlers.TryGetValue(topic, out var list))
        {
            list = new List<Action<RoseEvent>>();
            handlers[topic] = list;
        }

        list.Add(handler);
        return new Subscription(() => list.Remove(handler));
    }

    public void Publish(string topic, object? payload = null)
    {
        if (!handlers.TryGetValue(topic, out var list))
            return;

        var ev = new RoseEvent(topic, payload);
        foreach (var handler in list.ToArray())
            handler(ev);
    }

    private sealed class Subscription : IDisposable
    {
        private Action? dispose;

        public Subscription(Action dispose)
        {
            this.dispose = dispose;
        }

        public void Dispose()
        {
            var action = dispose;
            dispose = null;
            action?.Invoke();
        }
    }
}

public sealed class RoseEvent
{
    public RoseEvent(string topic, object? payload)
    {
        Topic = topic;
        Payload = payload;
    }

    public string Topic { get; }
    public object? Payload { get; }
}
