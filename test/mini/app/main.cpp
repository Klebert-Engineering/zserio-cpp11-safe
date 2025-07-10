#include <iostream>
#include <cstdlib>
#include "MiniTest.h"

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "Zserio C++11-Safe Extension Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    MiniTest tester;
    bool success = tester.runAllTests();
    
    if (success)
    {
        std::cout << "\nAll tests passed!" << std::endl;
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout << "\nSome tests failed!" << std::endl;
        return EXIT_FAILURE;
    }
}