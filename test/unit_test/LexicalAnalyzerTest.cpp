#include "LexicalAnalyzerTest.hpp"

#include <cinttypes>
#include <cstdio>

#include "fkYAML/LexicalAnalyzer.hpp"

int LexicalAnalyzerTest::ScanSignedDecimalIntegerTest()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer("-1234");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::SIGNED_INT_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::SIGNED_INT_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    int64_t scanned_value = lexer.GetSignedInt();
    if (scanned_value != -1234)
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%d, actual=%" PRId64 "\n", -1234, scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanUnsignedDecimalIntegerTest()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer("5678");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::UNSIGNED_INT_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::UNSIGNED_INT_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    uint64_t scanned_value = lexer.GetUnsignedInt();
    if (scanned_value != 5678ULL)
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%" PRIu64 ", actual=%" PRIu64 "\n", 5678ULL, scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanOctalIntegerTest()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer("0o437");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::UNSIGNED_INT_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::UNSIGNED_INT_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    uint64_t scanned_value = lexer.GetUnsignedInt();
    if (scanned_value != 287ULL)
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%" PRIu64 ", actual=%" PRIu64 "\n", 287ULL, scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanHexadecimalIntegerTest()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer("0xA04F");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::UNSIGNED_INT_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::UNSIGNED_INT_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    uint64_t scanned_value = lexer.GetUnsignedInt();
    if (scanned_value != 41039ULL)
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%" PRIu64 ", actual=%" PRIu64 "\n", 41039ULL, scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanSignedFloatNumberTest()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer("-1.234");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    double scanned_value = lexer.GetFloatNumber();
    if (scanned_value != -1.234)
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%lf, actual=%lf\n", -1.234, scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanUnsignedFloatNumberTest()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer("567.8");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    double scanned_value = lexer.GetFloatNumber();
    if (scanned_value != 567.8)
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%lf, actual=%lf\n", 567.8, scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanFloatNumberWithExponentTest()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer("9.8e-3");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    double scanned_value = lexer.GetFloatNumber();
    if (scanned_value != 9.8e-3)
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%lf, actual=%lf\n", 9.8e-3, scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanPositiveInfinityTest1()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer(".inf");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    double scanned_value = lexer.GetFloatNumber();
    if (!std::isinf(scanned_value))
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%lf, actual=%lf\n", std::numeric_limits<double>::infinity(), scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanPositiveInfinityTest2()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer(".Inf");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    double scanned_value = lexer.GetFloatNumber();
    if (!std::isinf(scanned_value))
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%lf, actual=%lf\n", std::numeric_limits<double>::infinity(), scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanPositiveInfinityTest3()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer(".INF");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    double scanned_value = lexer.GetFloatNumber();
    if (!std::isinf(scanned_value))
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%lf, actual=%lf\n", std::numeric_limits<double>::infinity(), scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanNegativeInfinityTest1()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer("-.inf");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    double scanned_value = lexer.GetFloatNumber();
    if (!std::isinf(scanned_value))
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%lf, actual=%lf\n", -std::numeric_limits<double>::infinity(), scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanNegativeInfinityTest2()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer("-.Inf");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    double scanned_value = lexer.GetFloatNumber();
    if (!std::isinf(scanned_value))
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%lf, actual=%lf\n", -std::numeric_limits<double>::infinity(), scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanNegativeInfinityTest3()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer("-.INF");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    double scanned_value = lexer.GetFloatNumber();
    if (!std::isinf(scanned_value))
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%lf, actual=%lf\n", -std::numeric_limits<double>::infinity(), scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanNaNTest1()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer(".nan");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    double scanned_value = lexer.GetFloatNumber();
    if (!std::isnan(scanned_value))
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%lf, actual=%lf\n", std::nan(""), scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanNaNTest2()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer(".NaN");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    double scanned_value = lexer.GetFloatNumber();
    if (!std::isnan(scanned_value))
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%lf, actual=%lf\n", std::nan(""), scanned_value);
        return -1;
    }

    return 0;
}

int LexicalAnalyzerTest::ScanNaNTest3()
{
    fkyaml::LexicalAnalyzer lexer;
    lexer.SetInputBuffer(".NAN");

    fkyaml::LexicalTokenType ret = fkyaml::LexicalTokenType::END_OF_BUFFER;
    try
    {
        ret = lexer.GetNextToken();
    }
    catch (const fkyaml::Exception& e)
    {
        std::fprintf(stderr, "fkYAML internal error: %s\n", e.what());
        return -1;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "unexpected error: %s\n", e.what());
        return -1;
    }

    if (ret != fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE)
    {
        std::fprintf(
            stderr,
            "Invalid scan result for signed integers. expect=%d, actual=%d\n",
            static_cast<int>(fkyaml::LexicalTokenType::FLOAT_NUMBER_VALUE),
            static_cast<int>(ret));
        return -1;
    }

    double scanned_value = lexer.GetFloatNumber();
    if (!std::isnan(scanned_value))
    {
        std::fprintf(stderr, "Invalid scanned value. expectated=%lf, actual=%lf\n", std::nan(""), scanned_value);
        return -1;
    }

    return 0;
}
