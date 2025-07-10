#ifndef MINI_TEST_H
#define MINI_TEST_H

#include <string>

class MiniTest
{
public:
    MiniTest() = default;
    ~MiniTest() = default;

    bool runAllTests();

private:
    bool testInnerSerialization();
    bool testOuterSerialization();
    bool testMostOuterSerialization();
    bool testEdgeCases();

    void reportResult(const std::string& testName, bool passed);
    
    int m_totalTests = 0;
    int m_passedTests = 0;
};

#endif // MINI_TEST_H