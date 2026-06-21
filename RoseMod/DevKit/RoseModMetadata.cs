using System.Reflection;

namespace RoseMod.DevKit;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Assembly, AllowMultiple = false)]
public sealed class RoseModMetadataAttribute : Attribute
{
    public RoseModMetadataAttribute(string id, string name, string version, string author = "")
    {
        Id = id;
        Name = name;
        Version = version;
        Author = author;
    }

    public string Id { get; }
    public string Name { get; }
    public string Version { get; }
    public string Author { get; }
    public string Description { get; set; } = string.Empty;
}

public sealed class RoseModMetadata
{
    public RoseModMetadata(string id, string name, string version, string author = "", string description = "")
    {
        Id = string.IsNullOrWhiteSpace(id) ? "unknown.mod" : id;
        Name = string.IsNullOrWhiteSpace(name) ? "Unknown Mod" : name;
        Version = string.IsNullOrWhiteSpace(version) ? "0.0.0" : version;
        Author = author ?? string.Empty;
        Description = description ?? string.Empty;
    }

    public string Id { get; }
    public string Name { get; }
    public string Version { get; }
    public string Author { get; }
    public string Description { get; }

    public static RoseModMetadata FromType(Type modType)
    {
        var attribute = modType.GetCustomAttributes(typeof(RoseModMetadataAttribute), inherit: false)
            .OfType<RoseModMetadataAttribute>()
            .FirstOrDefault()
            ?? modType.Assembly.GetCustomAttributes(typeof(RoseModMetadataAttribute))
                .OfType<RoseModMetadataAttribute>()
                .FirstOrDefault();

        if (attribute is not null)
            return new RoseModMetadata(attribute.Id, attribute.Name, attribute.Version, attribute.Author, attribute.Description);

        var assemblyName = modType.Assembly.GetName();
        var version = assemblyName.Version?.ToString() ?? "0.0.0";
        return new RoseModMetadata(modType.FullName ?? modType.Name, assemblyName.Name ?? modType.Name, version);
    }

    public override string ToString()
    {
        return string.IsNullOrWhiteSpace(Author)
            ? $"{Name} {Version}"
            : $"{Name} {Version} by {Author}";
    }
}
