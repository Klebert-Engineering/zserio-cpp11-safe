#include <fstream>
#include <regex>

#include "gtest/gtest.h"

class DeprecatedAttributeTest : public ::testing::Test
{
protected:
    bool matchInFile(const std::string& fileName, const std::regex& lineRegex)
    {
        std::ifstream file(fileName.c_str());

        bool isPresent = false;
        std::string line;
        while (std::getline(file, line))
        {
            if (std::regex_search(line, lineRegex))
            {
                isPresent = true;
                break;
            }
        }
        file.close();

        return isPresent;
    }

    static const char* const ERROR_LOG_PATH;
};

const char* const DeprecatedAttributeTest::ERROR_LOG_PATH =
        "zserio/deprecated_attribute/src/DeprecatedAttribute-stamp/DeprecatedAttribute-build-"
#if defined(DEPRECATED_ATTRIBUTE_TEST_CHECK_WARNINGS) && DEPRECATED_ATTRIBUTE_TEST_CHECK_WARNINGS == 1
        "out"
#else
        "err"
#endif
        ".log";

TEST_F(DeprecatedAttributeTest, checkWarnings)
{
#if defined(DEPRECATED_ATTRIBUTE_TEST_CHECK_WARNINGS) && DEPRECATED_ATTRIBUTE_TEST_CHECK_WARNINGS != 0
    ASSERT_FALSE(matchInFile(ERROR_LOG_PATH, std::regex("Unknown warning to check matchInFile method!")));
    ASSERT_TRUE(matchInFile(
            ERROR_LOG_PATH, std::regex("DeprecatedAttribute\\.cpp.*15.*81.*warning.*FIVE.*deprecated")))
            << "Warning not found in '" << ERROR_LOG_PATH << "'!";
#endif
}
