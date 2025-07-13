#include <fstream>

#include "zserio/FileUtil.h"
#include "zserio/StringView.h"
#include "zserio/Result.h"
#include "zserio/ErrorCode.h"

namespace zserio
{

Result<void> writeBufferToFile(const uint8_t* buffer, size_t bitSize, BitsTag, const std::string& fileName) noexcept
{
    std::ofstream stream(fileName.c_str(), std::ofstream::binary | std::ofstream::trunc);
    if (!stream)
    {
        return Result<void>::error(ErrorCode::FileOpenFailed);
    }

    const size_t byteSize = (bitSize + 7) / 8;
    if (!stream.write(reinterpret_cast<const char*>(buffer), static_cast<std::streamsize>(byteSize)))
    {
        return Result<void>::error(ErrorCode::FileWriteFailed);
    }

    return Result<void>::success();
}

Result<BitBuffer> readBufferFromFile(const std::string& fileName) noexcept
{
    std::ifstream stream(fileName.c_str(), std::ifstream::binary);
    if (!stream)
    {
        return Result<BitBuffer>::error(ErrorCode::FileOpenFailed);
    }

    (void)stream.seekg(0, stream.end);
    const std::streampos fileSize = stream.tellg();
    (void)stream.seekg(0);

    if (static_cast<int>(fileSize) == -1)
    {
        return Result<BitBuffer>::error(ErrorCode::FileSeekFailed);
    }

    const size_t sizeLimit = std::numeric_limits<size_t>::max() / 8;
    if (static_cast<uint64_t>(fileSize) > sizeLimit)
    {
        return Result<BitBuffer>::error(ErrorCode::BufferSizeExceeded);
    }

    // Create BitBuffer with factory method to handle allocation failure
    auto bitBufferResult = BitBuffer::create(static_cast<size_t>(fileSize) * 8);
    if (bitBufferResult.isError())
    {
        return Result<BitBuffer>::error(bitBufferResult.getError());
    }
    BitBuffer bitBuffer = std::move(bitBufferResult.getValue());

    if (!stream.read(reinterpret_cast<char*>(bitBuffer.getBuffer()),
                static_cast<std::streamsize>(bitBuffer.getByteSize())))
    {
        return Result<BitBuffer>::error(ErrorCode::FileReadFailed);
    }

    return Result<BitBuffer>::success(std::move(bitBuffer));
}

} // namespace zserio
