package zserio.extension.cpp;

import zserio.extension.cpp.types.NativeArrayTraits;

/**
 * FreeMarker template data for array traits.
 */
public final class ArrayTraitsTemplateData
{
    public ArrayTraitsTemplateData(NativeArrayTraits nativeTraits)
    {
        this.nativeTraits = nativeTraits;
    }

    public String getName()
    {
        return nativeTraits.getFullName();
    }

    public boolean getIsTemplated()
    {
        return nativeTraits.isTemplated();
    }

    public boolean getRequiresElementFixedBitSize()
    {
        return nativeTraits.requiresElementFixedBitSize();
    }

    public boolean getRequiresElementDynamicBitSize()
    {
        return nativeTraits.requiresElementDynamicBitSize();
    }

    public boolean getRequiresElementFactory()
    {
        return nativeTraits.requiresElementFactory();
    }

    private final NativeArrayTraits nativeTraits;
}
