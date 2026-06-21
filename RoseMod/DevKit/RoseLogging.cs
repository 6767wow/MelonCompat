namespace RoseMod.DevKit;

public interface IRoseLogger
{
    void Write(RoseLogLevel level, string message, Exception? exception = null);
}

public sealed class RoseConsoleLogger : IRoseLogger
{
    private readonly string name;

    public RoseConsoleLogger(string name)
    {
        this.name = string.IsNullOrWhiteSpace(name) ? "RoseMod" : name;
    }

    public void Write(RoseLogLevel level, string message, Exception? exception = null)
    {
        var prefix = $"[{DateTime.Now:HH:mm:ss.fff}] [{name}] [{level}]";
        Console.WriteLine($"{prefix} {message}");
        if (exception is not null)
            Console.WriteLine(exception);
    }
}

public sealed class RoseLog
{
    private readonly IRoseLogger logger;

    public RoseLog(IRoseLogger logger)
    {
        this.logger = logger;
    }

    public void Debug(string message) => logger.Write(RoseLogLevel.Debug, message);
    public void Info(string message) => logger.Write(RoseLogLevel.Info, message);
    public void Warning(string message) => logger.Write(RoseLogLevel.Warning, message);
    public void Error(string message) => logger.Write(RoseLogLevel.Error, message);
    public void Error(Exception exception, string message) => logger.Write(RoseLogLevel.Error, message, exception);
}
