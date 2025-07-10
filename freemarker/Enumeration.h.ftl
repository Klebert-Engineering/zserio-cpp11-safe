<#include "FileHeader.inc.ftl">
<#include "DocComment.inc.ftl">
<@file_header generatorDescription/>

<@include_guard_begin package.path, name/>

<@runtime_version_check generatorVersion/>

#include <array>

<#function has_deprecated_items items>
    <#list items as item>
        <#if item.isDeprecated>
            <#return true>
        </#if>
    </#list>
    <#return false>
</#function>
<#if has_deprecated_items(items)>
#include <zserio/DeprecatedAttribute.h>
</#if>
#include <zserio/Enums.h>
#include <zserio/BitStreamReader.h>
#include <zserio/BitStreamWriter.h>
<#if usedInPackedArray>
#include <zserio/DeltaContext.h>
</#if>
<#if !bitSize??>
#include <zserio/BitSizeOfCalculator.h>
</#if>
<#if withTypeInfoCode>
<@type_includes types.typeInfo/>
    <#if withReflectionCode>
<@type_includes types.reflectablePtr/>
    </#if>
</#if>
<@system_includes headerSystemIncludes/>
<@user_includes headerUserIncludes/>
<@namespace_begin package.path/>

<#if withCodeComments && docComments??>
<@doc_comments docComments/>
</#if>
enum class ${name} : ${underlyingTypeInfo.typeFullName}
{
<#list items as item>
    <#if withCodeComments && item.docComments??>
    <@doc_comments item.docComments, 1/>
    </#if>
    ${item.name}<#if item.isDeprecated> ZSERIO_DEPRECATED</#if> = ${item.value}<#if item?has_next>,</#if>
</#list>
};
<@namespace_end package.path/>
<@namespace_begin ["zserio"]/>

// This is full specialization of enumeration traits and methods for ${name} enumeration.
template <>
struct EnumTraits<${fullName}>
{
    static constexpr ::std::array<const char*, ${items?size}> names =
    {{
<#list items as item>
        "${item.name}"<#if item?has_next>,</#if>
</#list>
    }};

    static constexpr ::std::array<${fullName}, ${items?size}> values =
    {{
<#list items as item>
        ${item.fullName}<#if item?has_next>,</#if>
</#list>
    }};

    static constexpr const char* enumName = "${name}";
<#if underlyingTypeInfo.arrayTraits.isTemplated && underlyingTypeInfo.arrayTraits.requiresElementDynamicBitSize>

    class ZserioElementBitSize
    {
    public:
        static uint8_t get();
    };
</#if>
};
<#if withTypeInfoCode>

template <>
const ${types.typeInfo.name}& enumTypeInfo<${fullName}, ${types.allocator.default}>();
    <#if withReflectionCode>

template <>
${types.reflectablePtr.name} enumReflectable(${fullName} value, const ${types.allocator.default}& allocator);
    </#if>
</#if>

template <>
size_t enumToOrdinal<${fullName}>(${fullName} value);

template <>
${fullName} valueToEnum<${fullName}>(
        typename ::std::underlying_type<${fullName}>::type rawValue);

template <>
uint32_t enumHashCode<${fullName}>(${fullName} value);
<#if usedInPackedArray>

template <>
void initPackingContext<::zserio::DeltaContext, ${fullName}>(::zserio::DeltaContext& context, ${fullName} value);
</#if>

template <>
size_t bitSizeOf<${fullName}>(${fullName} value);
<#if usedInPackedArray>

template <>
size_t bitSizeOf<::zserio::DeltaContext, ${fullName}>(::zserio::DeltaContext& context, ${fullName} value);
</#if>
<#if withWriterCode>

template <>
size_t initializeOffsets<${fullName}>(size_t bitPosition, ${fullName} value);
    <#if usedInPackedArray>

template <>
size_t initializeOffsets<::zserio::DeltaContext, ${fullName}>(::zserio::DeltaContext& context, size_t bitPosition,
        ${fullName} value);
    </#if>
</#if>

template <>
${fullName} read<${fullName}>(::zserio::BitStreamReader& in);
<#if usedInPackedArray>

template <>
${fullName} read<${fullName}, ::zserio::DeltaContext>(::zserio::DeltaContext& context, ::zserio::BitStreamReader& in);
</#if>
<#if withWriterCode>

template <>
void write<${fullName}>(::zserio::BitStreamWriter& out, ${fullName} value);
    <#if usedInPackedArray>

template <>
void write<::zserio::DeltaContext, ${fullName}>(::zserio::DeltaContext& context, ::zserio::BitStreamWriter& out,
        ${fullName} value);
    </#if>
</#if>
<@namespace_end ["zserio"]/>

<@include_guard_end package.path, name/>
