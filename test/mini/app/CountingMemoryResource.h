#ifndef COUNTING_MEMORY_RESOURCE_H_INC
#define COUNTING_MEMORY_RESOURCE_H_INC

#include <cstddef>
#include <atomic>
#include <algorithm>

#include "zserio/pmr/MemoryResource.h"
#include "zserio/Result.h"
#include "zserio/ErrorCode.h"

namespace minizs
{

/**
 * Example counting memory resource for functional safety applications.
 * 
 * This is a basic implementation that:
 * - Tracks total allocated memory
 * - Tracks current memory usage
 * - Tracks peak memory usage
 * - Can enforce a maximum memory limit
 * - Uses simple allocation with minimal overhead
 * 
 * WARNING: This is a simple example implementation for demonstration purposes.
 * Production code needs more sophisticated features:
 * - Memory fragmentation prevention (e.g., memory pools with fixed-size blocks)
 * - Guaranteed allocation strategies (pre-allocated pools)
 * - Thread-safe allocation/deallocation with deterministic timing
 * - Deterministic allocation times (no searching for free blocks)
 * - Advanced memory corruption detection (guard pages, checksums)
 * - Alignment guarantees for all platforms
 * - Certified for specific functional safety standards (ISO 26262, DO-178C, etc.)
 */
class CountingMemoryResource : public ::zserio::pmr::MemoryResource
{
public:
    /**
     * Constructor.
     * 
     * \param maxMemory Maximum allowed memory in bytes (0 = unlimited)
     */
    explicit CountingMemoryResource(size_t maxMemory = 0) noexcept :
            m_maxMemory(maxMemory),
            m_currentMemory(0),
            m_peakMemory(0),
            m_totalAllocated(0),
            m_allocationCount(0),
            m_deallocationCount(0)
    {}

    /**
     * Get current memory usage in bytes.
     */
    size_t getCurrentMemory() const noexcept
    {
        return m_currentMemory.load(std::memory_order_relaxed);
    }

    /**
     * Get peak memory usage in bytes.
     */
    size_t getPeakMemory() const noexcept
    {
        return m_peakMemory.load(std::memory_order_relaxed);
    }

    /**
     * Get total allocated memory in bytes (includes deallocated memory).
     */
    size_t getTotalAllocated() const noexcept
    {
        return m_totalAllocated.load(std::memory_order_relaxed);
    }

    /**
     * Get number of allocations.
     */
    size_t getAllocationCount() const noexcept
    {
        return m_allocationCount.load(std::memory_order_relaxed);
    }

    /**
     * Get number of deallocations.
     */
    size_t getDeallocationCount() const noexcept
    {
        return m_deallocationCount.load(std::memory_order_relaxed);
    }

    /**
     * Reset statistics (does not affect allocated memory).
     */
    void resetStatistics() noexcept
    {
        m_peakMemory.store(m_currentMemory.load(std::memory_order_relaxed), std::memory_order_relaxed);
        m_totalAllocated.store(m_currentMemory.load(std::memory_order_relaxed), std::memory_order_relaxed);
        m_allocationCount.store(0, std::memory_order_relaxed);
        m_deallocationCount.store(0, std::memory_order_relaxed);
    }

private:
    // Header placed before each allocation to track size and detect corruption
    struct AllocationHeader
    {
        static constexpr uint32_t MAGIC_START = 0xDEADBEEF;
        static constexpr uint32_t MAGIC_END = 0xCAFEBABE;
        
        uint32_t magicStart;
        size_t size;
        size_t alignment;
        uint32_t magicEnd;
    };

    void* doAllocate(size_t bytes, size_t alignment) override
    {
        // Add space for header
        const size_t headerSize = sizeof(AllocationHeader);
        const size_t totalSize = headerSize + bytes + sizeof(uint32_t); // Extra magic at end
        
        // Check memory limit
        if (m_maxMemory > 0)
        {
            size_t currentMem = m_currentMemory.load(std::memory_order_relaxed);
            if (currentMem + totalSize > m_maxMemory)
            {
                // In production, this should return nullptr or use a Result type
                // For now, we'll just return nullptr
                return nullptr;
            }
        }
        
        // Allocate with extra space for alignment and to store original pointer
        // We need space for: original pointer + alignment padding + header + data + end magic
        const size_t extraSpace = sizeof(void*) + alignment - 1;
        void* rawPtr = ::operator new(extraSpace + totalSize);
        if (rawPtr == nullptr)
        {
            return nullptr;
        }
        
        // Calculate aligned address for header (after space for original pointer)
        uintptr_t rawAddr = reinterpret_cast<uintptr_t>(rawPtr) + sizeof(void*);
        uintptr_t alignedAddr = (rawAddr + alignment - 1) & ~(alignment - 1);
        
        // Store original pointer just before header
        void** originalPtrStorage = reinterpret_cast<void**>(alignedAddr - sizeof(void*));
        *originalPtrStorage = rawPtr;
        
        // Place header
        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(alignedAddr);
        header->magicStart = AllocationHeader::MAGIC_START;
        header->size = bytes;
        header->alignment = alignment;
        header->magicEnd = AllocationHeader::MAGIC_END;
        
        // Place end magic
        uint32_t* endMagic = reinterpret_cast<uint32_t*>(
            reinterpret_cast<uint8_t*>(header) + headerSize + bytes);
        *endMagic = AllocationHeader::MAGIC_END;
        
        // Update statistics
        m_currentMemory.fetch_add(totalSize, std::memory_order_relaxed);
        m_totalAllocated.fetch_add(totalSize, std::memory_order_relaxed);
        m_allocationCount.fetch_add(1, std::memory_order_relaxed);
        
        // Update peak
        size_t current = m_currentMemory.load(std::memory_order_relaxed);
        size_t peak = m_peakMemory.load(std::memory_order_relaxed);
        while (current > peak && !m_peakMemory.compare_exchange_weak(peak, current, std::memory_order_relaxed))
        {
            // Loop until we update peak or current is no longer greater
        }
        
        // Return user pointer (after header)
        return reinterpret_cast<uint8_t*>(header) + headerSize;
    }

    void doDeallocate(void* storage, size_t bytes, size_t alignment) override
    {
        if (storage == nullptr)
        {
            return;
        }
        
        // Get header
        const size_t headerSize = sizeof(AllocationHeader);
        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(
            reinterpret_cast<uint8_t*>(storage) - headerSize);
        
        // Verify magic (basic corruption detection)
        if (header->magicStart != AllocationHeader::MAGIC_START ||
            header->magicEnd != AllocationHeader::MAGIC_END)
        {
            // Memory corruption detected
            // In production, this should trigger a safety action
            return;
        }
        
        // Verify size matches
        if (header->size != bytes || header->alignment != alignment)
        {
            // Size mismatch - possible corruption or programming error
            // In production, this should trigger a safety action
            return;
        }
        
        // Check end magic
        uint32_t* endMagic = reinterpret_cast<uint32_t*>(
            reinterpret_cast<uint8_t*>(storage) + bytes);
        if (*endMagic != AllocationHeader::MAGIC_END)
        {
            // Buffer overflow detected
            // In production, this should trigger a safety action
            return;
        }
        
        // Update statistics
        const size_t totalSize = headerSize + bytes + sizeof(uint32_t);
        m_currentMemory.fetch_sub(totalSize, std::memory_order_relaxed);
        m_deallocationCount.fetch_add(1, std::memory_order_relaxed);
        
        // Clear magic values
        header->magicStart = 0;
        header->magicEnd = 0;
        *endMagic = 0;
        
        // Retrieve original allocation pointer
        void** originalPtrStorage = reinterpret_cast<void**>(
            reinterpret_cast<uintptr_t>(header) - sizeof(void*));
        void* originalPtr = *originalPtrStorage;
        
        // Free the original allocation
        ::operator delete(originalPtr);
    }

    bool doIsEqual(const ::zserio::pmr::MemoryResource& other) const noexcept override
    {
        return this == &other;
    }

private:
    size_t m_maxMemory;
    std::atomic<size_t> m_currentMemory;
    std::atomic<size_t> m_peakMemory;
    std::atomic<size_t> m_totalAllocated;
    std::atomic<size_t> m_allocationCount;
    std::atomic<size_t> m_deallocationCount;
};

} // namespace minizs

#endif // COUNTING_MEMORY_RESOURCE_H_INC