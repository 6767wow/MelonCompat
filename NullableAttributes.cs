namespace System.Runtime.CompilerServices;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Method | AttributeTargets.Interface | AttributeTargets.Delegate)]
internal sealed class NullableContextAttribute : Attribute
{
    public NullableContextAttribute(byte flag)
    {
    }
}

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Method | AttributeTargets.Interface | AttributeTargets.Delegate | AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Event | AttributeTargets.Parameter | AttributeTargets.ReturnValue | AttributeTargets.GenericParameter)]
internal sealed class NullableAttribute : Attribute
{
    public NullableAttribute(byte flag)
    {
    }

    public NullableAttribute(byte[] flags)
    {
    }
}
