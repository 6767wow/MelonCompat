using System.ComponentModel;
using System.Globalization;

namespace RoseMod.DevKit;

public sealed class RoseConfig
{
    private readonly Dictionary<string, string> values = new(StringComparer.OrdinalIgnoreCase);
    private readonly List<IRoseConfigEntry> entries = new();

    public RoseConfig(string filePath)
    {
        FilePath = filePath;
        Load();
    }

    public string FilePath { get; }

    public RoseConfigEntry<T> Bind<T>(string section, string key, T defaultValue, string description = "")
    {
        var fullKey = NormalizeKey(section, key);
        var value = values.TryGetValue(fullKey, out var raw)
            ? ConvertValue(raw, defaultValue)
            : defaultValue;

        var entry = new RoseConfigEntry<T>(section, key, value, defaultValue, description);
        entries.Add(entry);
        return entry;
    }

    public void Save()
    {
        Directory.CreateDirectory(Path.GetDirectoryName(FilePath) ?? Directory.GetCurrentDirectory());
        using var writer = new StreamWriter(FilePath, append: false);
        writer.WriteLine("# RoseMod DevKit config");
        foreach (var entry in entries.OrderBy(entry => entry.Section, StringComparer.OrdinalIgnoreCase)
            .ThenBy(entry => entry.Key, StringComparer.OrdinalIgnoreCase))
        {
            if (!string.IsNullOrWhiteSpace(entry.Description))
                writer.WriteLine("# " + entry.Description);
            writer.WriteLine($"{NormalizeKey(entry.Section, entry.Key)}={entry.SerializedValue}");
        }
    }

    private void Load()
    {
        if (!File.Exists(FilePath))
            return;

        foreach (var line in File.ReadAllLines(FilePath))
        {
            var trimmed = line.Trim();
            if (trimmed.Length == 0 || trimmed.StartsWith("#", StringComparison.Ordinal))
                continue;

            var equals = trimmed.IndexOf('=');
            if (equals <= 0)
                continue;

            values[trimmed.Substring(0, equals).Trim()] = trimmed.Substring(equals + 1).Trim();
        }
    }

    private static string NormalizeKey(string section, string key)
    {
        return $"{section.Trim()}.{key.Trim()}";
    }

    private static T ConvertValue<T>(string value, T fallback)
    {
        try
        {
            if (typeof(T).IsEnum)
                return (T)Enum.Parse(typeof(T), value, ignoreCase: true);

            var converter = TypeDescriptor.GetConverter(typeof(T));
            if (converter.CanConvertFrom(typeof(string)))
                return (T?)converter.ConvertFromInvariantString(value) ?? fallback;

            return (T)Convert.ChangeType(value, typeof(T), CultureInfo.InvariantCulture);
        }
        catch
        {
            return fallback;
        }
    }
}

public interface IRoseConfigEntry
{
    string Section { get; }
    string Key { get; }
    string Description { get; }
    string SerializedValue { get; }
}

public sealed class RoseConfigEntry<T> : IRoseConfigEntry
{
    internal RoseConfigEntry(string section, string key, T value, T defaultValue, string description)
    {
        Section = section;
        Key = key;
        Value = value;
        DefaultValue = defaultValue;
        Description = description;
    }

    public string Section { get; }
    public string Key { get; }
    public T Value { get; set; }
    public T DefaultValue { get; }
    public string Description { get; }
    public string SerializedValue => Convert.ToString(Value, CultureInfo.InvariantCulture) ?? string.Empty;
}
