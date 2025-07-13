#include <cstdlib>
#include <iostream>
#include <vector>

#include "minizs/Inner.h"
#include "minizs/MostOuter.h"
#include "minizs/Outer.h"
#include "zserio/SerializeUtil.h"
#include "CountingMemoryResource.h"
#include "zserio/pmr/PolymorphicAllocator.h"
#include "zserio/pmr/Vector.h"
#include "zserio/pmr/String.h"

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "Zserio C++11-Safe Mini Schema Demo" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;

  // Setup counting memory resource for tracking
  minizs::CountingMemoryResource countingResource(100 * 1024 * 1024); // 100MB limit for demo
  auto* previousDefault = zserio::pmr::setDefaultResource(&countingResource);

  std::cout << "Memory tracking enabled" << std::endl;
  std::cout << "Initial memory usage: " << countingResource.getCurrentMemory() << " bytes" << std::endl;
  std::cout << std::endl;

  // Create PMR allocator - using the same type as generated code
  zserio::pmr::PropagatingPolymorphicAllocator<> allocator(&countingResource);

  // Step 1: Create Inner objects
  std::cout << "1. Creating Inner objects..." << std::endl;
  zserio::pmr::vector<minizs::Inner> inners(allocator);

    // Create 3 Inner objects
    for (uint8_t i = 0; i < 3; ++i) {
      minizs::Inner inner(allocator);
      zserio::pmr::string key("item_", allocator);
      key += std::to_string(i);
      inner.setKey(key);
      inner.setValue(10 + i * 5); // values: 10, 15, 20
      inners.push_back(inner);
      std::cout << "   - Inner[" << static_cast<int>(i) << "]: key='"
                << inner.getKey()
                << "', value=" << static_cast<int>(inner.getValue())
                << std::endl;
    }

    // Step 2: Create and fill Outer with Inner objects
    std::cout << "\n2. Creating Outer with Inner objects..." << std::endl;
    minizs::Outer outer(inners, allocator);
    outer.initialize(3); // Initialize with the number of inner elements
    std::cout << "   - Outer initialized with " << inners.size()
              << " Inner objects" << std::endl;

    // Step 3: Create and fill MostOuter with Outer
    std::cout << "\n3. Creating MostOuter..." << std::endl;
    minizs::MostOuter mostOuter(allocator);
    mostOuter.setNumOfInner(3);
    mostOuter.setOuter(outer);
    std::cout << "   - MostOuter: numOfInner="
              << static_cast<int>(mostOuter.getNumOfInner()) << std::endl;

    // Step 4: Serialize MostOuter using zserio::serialize
    std::cout << "\n4. Serializing MostOuter..." << std::endl;
    const auto serializedResult = zserio::serialize(mostOuter, allocator);
    if (!serializedResult.isSuccess())
    {
        std::cerr << "\nERROR: Serialization failed with error code: "
                  << static_cast<int>(serializedResult.getError()) << std::endl;
        return EXIT_FAILURE;
    }
    const auto serializedData = serializedResult.getValue();
    std::cout << "   - Serialized to " << serializedData.getByteSize()
              << " bytes" << std::endl;

    // Step 5: Deserialize MostOuter using zserio::deserialize
    std::cout << "\n5. Deserializing MostOuter..." << std::endl;
    const auto deserializeResult =
        zserio::deserialize<minizs::MostOuter>(serializedData, allocator);
    if (!deserializeResult.isSuccess())
    {
        std::cerr << "\nERROR: Deserialization failed with error code: "
                  << static_cast<int>(deserializeResult.getError()) << std::endl;
        return EXIT_FAILURE;
    }
    const auto deserializedMostOuter = deserializeResult.getValue();
    std::cout << "   - Deserialized successfully" << std::endl;

    // Step 6: Verify the deserialized data
    std::cout << "\n6. Verifying deserialized data..." << std::endl;
    std::cout << "   - numOfInner: "
              << static_cast<int>(deserializedMostOuter.getNumOfInner())
              << std::endl;

    const auto &deserializedInners =
        deserializedMostOuter.getOuter().getInner();
    std::cout << "   - Number of Inner objects: " << deserializedInners.size()
              << std::endl;

    bool dataMatches = true;
    for (size_t i = 0; i < deserializedInners.size(); ++i) {
      const auto &inner = deserializedInners[i];
      std::cout << "   - Inner[" << i << "]: key='" << inner.getKey()
                << "', value=" << static_cast<int>(inner.getValue())
                << std::endl;

      // Verify the data matches
      zserio::pmr::string expected("item_", allocator);
      expected += std::to_string(i);
      if (inner.getKey() != expected ||
          inner.getValue() != 10 + i * 5) {
        dataMatches = false;
      }
    }

    // Print memory statistics
    std::cout << "\n7. Memory Usage Statistics:" << std::endl;
    std::cout << "   - Current memory: " << countingResource.getCurrentMemory() << " bytes" << std::endl;
    std::cout << "   - Peak memory: " << countingResource.getPeakMemory() << " bytes" << std::endl;
    std::cout << "   - Total allocated: " << countingResource.getTotalAllocated() << " bytes" << std::endl;
    std::cout << "   - Allocations: " << countingResource.getAllocationCount() << std::endl;
    std::cout << "   - Deallocations: " << countingResource.getDeallocationCount() << std::endl;

    // Restore previous default resource
    zserio::pmr::setDefaultResource(previousDefault);

    std::cout << "\n========================================" << std::endl;
    if (dataMatches && deserializedMostOuter.getNumOfInner() == 3 &&
        deserializedInners.size() == 3) {
      std::cout << "SUCCESS: All data verified correctly!" << std::endl;
      std::cout << "========================================" << std::endl;
      return EXIT_SUCCESS;
    } else {
      std::cout << "FAILED: Data verification failed!" << std::endl;
      std::cout << "========================================" << std::endl;
      return EXIT_FAILURE;
    }
}
