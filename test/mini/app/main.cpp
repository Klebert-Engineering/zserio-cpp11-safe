#include <cstdlib>
#include <iostream>
#include <vector>

#include "minizs/Inner.h"
#include "minizs/MostOuter.h"
#include "minizs/Outer.h"
#include "zserio/SerializeUtil.h"

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "Zserio C++11-Safe Mini Schema Demo" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;

  try {
    // Step 1: Create Inner objects
    std::cout << "1. Creating Inner objects..." << std::endl;
    std::vector<minizs::Inner> inners;

    // Create 3 Inner objects
    for (uint8_t i = 0; i < 3; ++i) {
      minizs::Inner inner;
      inner.setKey("item_" + std::to_string(i));
      inner.setValue(10 + i * 5); // values: 10, 15, 20
      inners.push_back(inner);
      std::cout << "   - Inner[" << static_cast<int>(i) << "]: key='"
                << inner.getKey()
                << "', value=" << static_cast<int>(inner.getValue())
                << std::endl;
    }

    // Step 2: Create and fill Outer with Inner objects
    std::cout << "\n2. Creating Outer with Inner objects..." << std::endl;
    minizs::Outer outer(inners);
    outer.initialize(3); // Initialize with the number of inner elements
    std::cout << "   - Outer initialized with " << inners.size()
              << " Inner objects" << std::endl;

    // Step 3: Create and fill MostOuter with Outer
    std::cout << "\n3. Creating MostOuter..." << std::endl;
    minizs::MostOuter mostOuter;
    mostOuter.setNumOfInner(3);
    mostOuter.setOuter(outer);
    std::cout << "   - MostOuter: numOfInner="
              << static_cast<int>(mostOuter.getNumOfInner()) << std::endl;

    // Step 4: Serialize MostOuter using zserio::serialize
    std::cout << "\n4. Serializing MostOuter..." << std::endl;
    const auto serializedData = zserio::serialize(mostOuter);
    std::cout << "   - Serialized to " << serializedData.getByteSize()
              << " bytes" << std::endl;

    // Step 5: Deserialize MostOuter using zserio::deserialize
    std::cout << "\n5. Deserializing MostOuter..." << std::endl;
    const auto deserializedMostOuter =
        zserio::deserialize<minizs::MostOuter>(serializedData);
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
      if (inner.getKey() != "item_" + std::to_string(i) ||
          inner.getValue() != 10 + i * 5) {
        dataMatches = false;
      }
    }

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
  } catch (const std::exception &e) {
    std::cerr << "\nERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
