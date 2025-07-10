#include "MiniTest.h"
#include <iostream>
#include <memory>
#include <vector>

#include "minizs/Inner.h"
#include "minizs/Outer.h"
#include "minizs/MostOuter.h"
#include "zserio/BitStreamWriter.h"
#include "zserio/BitStreamReader.h"
#include "zserio/SerializeUtil.h"

bool MiniTest::runAllTests()
{
    std::cout << "Running Mini Zserio C++11 Tests...\n" << std::endl;
    
    testInnerSerialization();
    testOuterSerialization();
    testMostOuterSerialization();
    testEdgeCases();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Total tests: " << m_totalTests << std::endl;
    std::cout << "Passed: " << m_passedTests << std::endl;
    std::cout << "Failed: " << (m_totalTests - m_passedTests) << std::endl;
    std::cout << "========================================" << std::endl;
    
    return m_totalTests == m_passedTests;
}

bool MiniTest::testInnerSerialization()
{
    try
    {
        // Create an Inner object
        minizs::Inner inner;
        inner.setKey("test_key");
        inner.setValue(42);
        
        // Serialize
        std::vector<uint8_t> buffer(1024);  // Create a buffer
        zserio::BitStreamWriter writer(buffer.data(), buffer.size());
        inner.write(writer);
        
        // Deserialize
        zserio::BitStreamReader reader(buffer.data(), writer.getBitPosition(), zserio::BitsTag());
        minizs::Inner innerRead(reader);
        
        // Verify
        bool passed = (innerRead.getKey() == "test_key" && innerRead.getValue() == 42);
        reportResult("Inner serialization", passed);
        return passed;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception in testInnerSerialization: " << e.what() << std::endl;
        reportResult("Inner serialization", false);
        return false;
    }
}

bool MiniTest::testOuterSerialization()
{
    try
    {
        // Create Outer with 3 Inner objects
        const uint8_t numInners = 3;
        
        std::vector<minizs::Inner> inners;
        for (uint8_t i = 0; i < numInners; ++i)
        {
            minizs::Inner inner;
            inner.setKey("key_" + std::to_string(i));
            inner.setValue(i * 10);
            inners.push_back(inner);
        }
        minizs::Outer outer(inners);
        outer.initialize(numInners);
        
        // Serialize
        std::vector<uint8_t> buffer(4096);  // Create a larger buffer for the array
        zserio::BitStreamWriter writer(buffer.data(), buffer.size());
        outer.write(writer);
        
        // Deserialize
        zserio::BitStreamReader reader(buffer.data(), writer.getBitPosition(), zserio::BitsTag());
        minizs::Outer outerRead(reader, numInners);
        
        // Verify
        bool passed = true;
        const auto& readInners = outerRead.getInner();
        if (readInners.size() != numInners)
        {
            passed = false;
        }
        else
        {
            for (size_t i = 0; i < numInners; ++i)
            {
                if (readInners[i].getKey() != "key_" + std::to_string(i) ||
                    readInners[i].getValue() != i * 10)
                {
                    passed = false;
                    break;
                }
            }
        }
        
        reportResult("Outer serialization", passed);
        return passed;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception in testOuterSerialization: " << e.what() << std::endl;
        reportResult("Outer serialization", false);
        return false;
    }
}

bool MiniTest::testMostOuterSerialization()
{
    try
    {
        // Create MostOuter
        minizs::MostOuter mostOuter;
        mostOuter.setNumOfInner(2);
        
        // Create Outer with 2 Inner objects
        std::vector<minizs::Inner> inners;
        for (uint8_t i = 0; i < 2; ++i)
        {
            minizs::Inner inner;
            inner.setKey("nested_" + std::to_string(i));
            inner.setValue(100 + i);
            inners.push_back(inner);
        }
        minizs::Outer outer(inners);
        outer.initialize(2);  // Initialize with the number of inner elements
        mostOuter.setOuter(outer);
        
        // Serialize using SerializeUtil
        const auto buffer = zserio::serialize(mostOuter);
        
        // Deserialize using SerializeUtil
        const auto mostOuterRead = zserio::deserialize<minizs::MostOuter>(buffer);
        
        // Verify
        bool passed = true;
        if (mostOuterRead.getNumOfInner() != 2)
        {
            passed = false;
        }
        else
        {
            const auto& readInners = mostOuterRead.getOuter().getInner();
            for (size_t i = 0; i < 2; ++i)
            {
                if (readInners[i].getKey() != "nested_" + std::to_string(i) ||
                    readInners[i].getValue() != 100 + i)
                {
                    passed = false;
                    break;
                }
            }
        }
        
        reportResult("MostOuter serialization", passed);
        return passed;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception in testMostOuterSerialization: " << e.what() << std::endl;
        reportResult("MostOuter serialization", false);
        return false;
    }
}

bool MiniTest::testEdgeCases()
{
    try
    {
        bool allPassed = true;
        
        // Test 1: Empty array
        {
            minizs::Outer outer(std::vector<minizs::Inner>{});
            outer.initialize(0);
            
            std::vector<uint8_t> buffer(1024);
            zserio::BitStreamWriter writer(buffer.data(), buffer.size());
            outer.write(writer);
            
            zserio::BitStreamReader reader(buffer.data(), writer.getBitPosition(), zserio::BitsTag());
            minizs::Outer outerRead(reader, 0);
            
            bool passed = outerRead.getInner().empty();
            reportResult("Edge case: empty array", passed);
            allPassed &= passed;
        }
        
        // Test 2: Max uint8 value
        {
            minizs::Inner inner;
            inner.setKey("max_value");
            inner.setValue(255);
            
            std::vector<uint8_t> buffer(1024);
            zserio::BitStreamWriter writer(buffer.data(), buffer.size());
            inner.write(writer);
            
            zserio::BitStreamReader reader(buffer.data(), writer.getBitPosition(), zserio::BitsTag());
            minizs::Inner innerRead(reader);
            
            bool passed = (innerRead.getValue() == 255);
            reportResult("Edge case: max uint8", passed);
            allPassed &= passed;
        }
        
        // Test 3: Long string
        {
            minizs::Inner inner;
            std::string longKey(100, 'x');
            inner.setKey(longKey);
            inner.setValue(123);
            
            std::vector<uint8_t> buffer(1024);
            zserio::BitStreamWriter writer(buffer.data(), buffer.size());
            inner.write(writer);
            
            zserio::BitStreamReader reader(buffer.data(), writer.getBitPosition(), zserio::BitsTag());
            minizs::Inner innerRead(reader);
            
            bool passed = (innerRead.getKey() == longKey && innerRead.getValue() == 123);
            reportResult("Edge case: long string", passed);
            allPassed &= passed;
        }
        
        return allPassed;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception in testEdgeCases: " << e.what() << std::endl;
        reportResult("Edge cases", false);
        return false;
    }
}

void MiniTest::reportResult(const std::string& testName, bool passed)
{
    m_totalTests++;
    if (passed)
    {
        m_passedTests++;
        std::cout << "[PASS] " << testName << std::endl;
    }
    else
    {
        std::cout << "[FAIL] " << testName << std::endl;
    }
}