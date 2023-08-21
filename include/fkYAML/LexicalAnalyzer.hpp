/**
 * @file LexicalAnalyzer.hpp
 * @brief Implementation of the lexical analyzer for YAML documents.
 *
 * Copyright (c) 2023 fktn
 * Distributed under the MIT License (https://opensource.org/licenses/MIT)
 */

#ifndef FK_YAML_LEXICAL_ANALIZER_HPP_
#define FK_YAML_LEXICAL_ANALIZER_HPP_

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>

#include "fkYAML/Exception.hpp"

/**
 * @namespace fkyaml
 * @brief namespace for fkYAML library.
 */
namespace fkyaml
{

/**
 * @enum LexicalTokenType
 * @brief Definition of lexical token types.
 */
enum class LexicalTokenType
{
    END_OF_BUFFER,         //!< the end of input buffer.
    KEY_SEPARATOR,         //!< the key separater `:`
    VALUE_SEPARATOR,       //!< the value separater `,`
    ANCHOR_PREFIX,         //!< the character for anchor prefix `&`
    ALIAS_PREFIX,          //!< the character for alias prefix `*`
    COMMENT_PREFIX,        //!< the character for comment prefix `#`
    DIRECTIVE_PREFIX,      //!< the character for directive prefix `%`
    SEQUENCE_BLOCK_PREFIX, //!< the character for sequence block prefix `- `
    SEQUENCE_FLOW_BEGIN,   //!< the character for sequence flow begin `[`
    SEQUENCE_FLOW_END,     //!< the character for sequence flow end `]`
    MAPPING_FLOW_BEGIN,    //!< the character for mapping begin `{`
    MAPPING_FLOW_END,      //!< the character for mapping end `}`
    NULL_VALUE,            //!< a null value found. use GetNull() to get a value.
    BOOLEAN_VALUE,         //!< a boolean value found. use GetBoolean() to get a value.
    SIGNED_INT_VALUE,      //!< a signed integer value found. use GetSignedInt() to get a value.
    UNSIGNED_INT_VALUE,    //!< an unsigned integer value found. use GetUnsignedInt() to get a value.
    FLOAT_NUMBER_VALUE,    //!< a float number value found. use GetFloatNumber() to get a value.
    STRING_VALUE,          //!< the character for string begin `"` or any character except the above ones
};

/**
 * @class LexicalAnalyzer
 * @brief A class which lexically analizes YAML formatted inputs.
 */
class LexicalAnalyzer
{
private:
    using char_traits_type = std::char_traits<char>;
    using char_int_type = typename char_traits_type::int_type;

    /**
     * @struct Position
     * @brief Information set of analyzed data counters.
     */
    struct Position
    {
        //!< The total read char counts from the input buffer.
        size_t total_read_char_counts = 0;
        //!< The total read line counts.
        size_t total_read_line_counts = 0;
        //!< The total read char counts in the current line.
        size_t read_char_counts_in_line = 0;
        //!< The total char counts in the previous line.
        size_t prev_char_counts_in_line = 0;
    };

public:
    /**
     * @brief Construct a new LexicalAnalyzer object.
     */
    LexicalAnalyzer() = default;

public:
    /**
     * @brief Set an input buffer to be analyzed by this LexicalAnalyzer.
     *
     * @param[in] input_buffer An input buffer to be analyzed.
     */
    void SetInputBuffer(const char* const input_buffer)
    {
        if (!input_buffer || input_buffer[0] == '\0')
        {
            throw Exception("The input buffer for lexical analysis is nullptr or empty.");
        }
        m_input_buffer = input_buffer;
    }

    /**
     * @brief Get the next lexical token type by scanning the left of the input buffer.
     *
     * @return LexicalTokenType The next lexical token type.
     */
    LexicalTokenType GetNextToken()
    {
        if (m_input_buffer.empty())
        {
            throw Exception("The next token is required before an input buffer is set.");
        }

        SkipWhiteSpaces();

        const char& current = RefCurrentChar();

        if (isdigit(current))
        {
            return ScanNumber();
        }

        switch (current)
        {
        case '\0':
        case s_EOF: // end of input buffer
            return LexicalTokenType::END_OF_BUFFER;
        case ':': // key separater
            if (GetNextChar() != ' ')
            {
                throw Exception("At least one half-width space is required after a key separater(:).");
            }
            return LexicalTokenType::KEY_SEPARATOR;
        case ',': // value separater
            GetNextChar();
            return LexicalTokenType::VALUE_SEPARATOR;
        case '&': // anchor prefix
            return LexicalTokenType::ANCHOR_PREFIX;
        case '*': // alias prefix
            return LexicalTokenType::ALIAS_PREFIX;
        case '#': // comment prefix
            ScanComment();
            return LexicalTokenType::COMMENT_PREFIX;
        case '%': // directive prefix
            return LexicalTokenType::DIRECTIVE_PREFIX;
        case '-':
            if (RefNextChar() == ' ')
            {
                return LexicalTokenType::SEQUENCE_BLOCK_PREFIX;
            }
            return ScanNumber();
        case '[': // sequence flow begin
            GetNextChar();
            return LexicalTokenType::SEQUENCE_FLOW_BEGIN;
        case ']': // sequence flow end
            GetNextChar();
            return LexicalTokenType::SEQUENCE_FLOW_END;
        case '{': // mapping flow begin
            GetNextChar();
            return LexicalTokenType::MAPPING_FLOW_BEGIN;
        case '}': // mapping flow end
            GetNextChar();
            return LexicalTokenType::MAPPING_FLOW_END;
        case '@':
            throw Exception("Any token cannot start with at(@). It is a reserved indicator for YAML.");
        case '`':
            throw Exception("Any token cannot start with grave accent(`). It is a reserved indicator for YAML.");
        case '\"':
        case '\'':
            return ScanString();
        case '~':
            m_value_buffer = current;
            return LexicalTokenType::NULL_VALUE;
        case '+':
            return ScanNumber();
        case '.': {
            const std::string tmp_str = m_input_buffer.substr(m_position_info.total_read_char_counts, 4);

            if (tmp_str == ".inf" || tmp_str == ".Inf" || tmp_str == ".INF")
            {
                m_value_buffer = tmp_str;
                for (int i = 0; i < 4; ++i)
                {
                    GetNextChar();
                }
                return LexicalTokenType::FLOAT_NUMBER_VALUE;
            }

            if (tmp_str == ".nan" || tmp_str == ".NaN" || tmp_str == ".NAN")
            {
                m_value_buffer = tmp_str;
                for (int i = 0; i < 4; ++i)
                {
                    GetNextChar();
                }
                return LexicalTokenType::FLOAT_NUMBER_VALUE;
            }

            return ScanString();
        }
        case 'F':
        case 'f': {
            const std::string tmp_str = m_input_buffer.substr(m_position_info.total_read_char_counts, 5);
            // YAML specifies that only these words represent the boolean value `false`.
            // See "10.3.2. Tag Resolution" section in https://yaml.org/spec/1.2.2/
            if (tmp_str == "false" || tmp_str == "False" || tmp_str == "FALSE")
            {
                m_value_buffer = tmp_str;
                for (int i = 0; i < 5; ++i)
                {
                    GetNextChar();
                }
                return LexicalTokenType::BOOLEAN_VALUE;
            }
            return ScanString();
        }
        case 'N':
        case 'n': {
            const std::string tmp_str = m_input_buffer.substr(m_position_info.total_read_char_counts, 4);
            // YAML specifies that these words and a tilde represent a null value.
            // Tildes are already checked above, so no check is needed here.
            // See "10.3.2. Tag Resolution" section in https://yaml.org/spec/1.2.2/
            if (tmp_str == "null" || tmp_str == "Null" || tmp_str == "NULL")
            {
                m_value_buffer = tmp_str;
                for (int i = 0; i < 4; ++i)
                {
                    GetNextChar();
                }
                return LexicalTokenType::NULL_VALUE;
            }
            return ScanString();
        }
        case 'T':
        case 't': {
            const std::string tmp_str = m_input_buffer.substr(m_position_info.total_read_char_counts, 4);
            // YAML specifies that only these words represent the boolean value `true`.
            // See "10.3.2. Tag Resolution" section in https://yaml.org/spec/1.2.2/
            if (tmp_str == "true" || tmp_str == "True" || tmp_str == "TRUE")
            {
                m_value_buffer = tmp_str;
                for (int i = 0; i < 4; ++i)
                {
                    GetNextChar();
                }
                return LexicalTokenType::BOOLEAN_VALUE;
            }
            return ScanString();
        }
        default:
            return ScanString();
        }
    }

    /**
     * @brief Convert from string to null and get the converted value.
     *
     * @return std::nullptr_t A null value converted from one of the followings: "null", "Null", "NULL", "~".
     */
    std::nullptr_t GetNull() const
    {
        if (m_value_buffer.empty())
        {
            throw Exception("Value storage is empty.");
        }

        if (m_value_buffer == "null" || m_value_buffer == "Null" || m_value_buffer == "NULL" || m_value_buffer == "~")
        {
            return nullptr;
        }

        throw Exception("Invalid request for a null value.");
    }

    /**
     * @brief Convert from string to boolean and get the converted value.
     *
     * @return true  A string token is one of the followings: "true", "True", "TRUE".
     * @return false A string token is one of the followings: "false", "False", "FALSE".
     */
    bool GetBoolean() const
    {
        if (m_value_buffer.empty())
        {
            throw Exception("Value storage is empty.");
        }

        if (m_value_buffer == "true" || m_value_buffer == "True" || m_value_buffer == "TRUE")
        {
            return true;
        }

        if (m_value_buffer == "false" || m_value_buffer == "False" || m_value_buffer == "FALSE")
        {
            return false;
        }

        throw Exception("Invalid request for a boolean value.");
    }

    /**
     * @brief Convert from string to signed integer and get the converted value.
     *
     * @return int64_t A signed integer value converted from the source string.
     */
    int64_t GetSignedInt() const
    {
        if (m_value_buffer.empty())
        {
            throw Exception("Value storage is empty.");
        }

        char* endptr = nullptr;
        const auto tmp_val = std::strtoll(m_value_buffer.data(), &endptr, 0);

        if (endptr != m_value_buffer.data() + m_value_buffer.size())
        {
            throw Exception("Failed to convert a string to a signed integer.");
        }

        // NOLINTNEXTLINE(google-runtime-int)
        if ((tmp_val == std::numeric_limits<long long>::min() || tmp_val == std::numeric_limits<long long>::max()) &&
            errno == ERANGE)
        {
            ;
            throw Exception("Range error on converting from a string to a signed integer.");
        }

        const auto value_int = static_cast<int64_t>(tmp_val);
        if (value_int != tmp_val)
        {
            throw Exception("Failed to convert from long long to int64_t.");
        }
        return value_int;
    }

    /**
     * @brief Convert from string to unsigned integer and get the converted value.
     *
     * @return uint64_t An unsigned integer value converted from the source string.
     */
    uint64_t GetUnsignedInt() const
    {
        if (m_value_buffer.empty())
        {
            throw Exception("Value storage is empty.");
        }

        char* endptr = nullptr;
        const auto tmp_val = std::strtoull(m_value_buffer.data(), &endptr, 0);

        if (endptr != m_value_buffer.data() + m_value_buffer.size())
        {
            throw Exception("Failed to convert a string to an unsigned integer.");
        }

        // NOLINTNEXTLINE(google-runtime-int)
        if (tmp_val == std::numeric_limits<unsigned long long>::max() && errno == ERANGE)
        {
            throw Exception("Range error on converting from a string to an unsigned integer.");
        }

        const auto value_int = static_cast<uint64_t>(tmp_val);
        if (value_int != tmp_val)
        {
            throw Exception("Failed to convert from unsigned long long to uint64_t.");
        }
        return value_int;
    }

    /**
     * @brief Convert from string to float number and get the converted value.
     *
     * @return double A float number value converted from the source string.
     */
    double GetFloatNumber() const
    {
        if (m_value_buffer.empty())
        {
            throw Exception("Value storage is empty.");
        }

        if (m_value_buffer == ".inf" || m_value_buffer == ".Inf" || m_value_buffer == ".INF")
        {
            return std::numeric_limits<double>::infinity();
        }

        if (m_value_buffer == "-.inf" || m_value_buffer == "-.Inf" || m_value_buffer == "-.INF")
        {
            static_assert(std::numeric_limits<double>::is_iec559, "IEEE 754 required.");
            return -1 * std::numeric_limits<double>::infinity();
        }

        if (m_value_buffer == ".nan" || m_value_buffer == ".NaN" || m_value_buffer == ".NAN")
        {
            return std::nan("");
        }

        char* endptr = nullptr;
        const double value = std::strtod(m_value_buffer.data(), &endptr);

        if (endptr != m_value_buffer.data() + m_value_buffer.size())
        {
            throw Exception("Failed to convert a string to a double.");
        }

        if ((value == HUGE_VAL || value == -HUGE_VAL) && errno == ERANGE)
        {
            throw Exception("Range error on converting from a string to a double.");
        }

        return value;
    }

    /**
     * @brief Get a scanned string value.
     *
     * @return const std::string& Constant reference to a scanned string.
     */
    const std::string& GetString() const
    {
        return m_value_buffer;
    }

private:
    /**
     * @brief A utility function to convert a hexadecimal character to an integer.
     *
     * @param source A hexadecimal character ('0'~'9', 'A'~'F', 'a'~'f')
     * @return char A integer converted from @a source.
     */
    static char ConvertHexCharToByte(char source)
    {
        if ('0' <= source && source <= '9')
        {
            // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
            return static_cast<char>(source - '0');
        }

        if ('A' <= source && source <= 'F')
        {
            // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
            return static_cast<char>(source - 'A' + 10);
        }

        if ('a' <= source && source <= 'f')
        {
            // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
            return static_cast<char>(source - 'a' + 10);
        }

        throw Exception("Non-hexadecimal character has been given.");
    }

    /**
     * @brief Skip until a newline code or a null character is found.
     *
     * @return LexicalTokenType The lexical token type for comments
     */
    LexicalTokenType ScanComment()
    {
        if (RefCurrentChar() != '#')
        {
            throw Exception("Not the beginning of a comment section.");
        }

        while (true)
        {
            switch (GetNextChar())
            {
            case '\r':
                if (RefNextChar() == '\r')
                {
                    GetNextChar();
                }
            case '\n':
            case '\0':
                return LexicalTokenType::COMMENT_PREFIX;
            default:
                break;
            }
        }
    }

    /**
     * @brief Scan and determine a number type(signed/unsigned/float). This method is the entrypoint for all number
     * tokens.
     *
     * @return LexicalTokenType A lexical token type for a determined number type.
     */
    LexicalTokenType ScanNumber()
    {
        m_value_buffer.clear();
        switch (RefCurrentChar())
        {
        case '-':
            m_value_buffer.push_back(RefCurrentChar());
            return ScanNegativeNumber();
        case '0':
            m_value_buffer.push_back(RefCurrentChar());
            return ScanNumberAfterZeroAtFirst();
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            m_value_buffer.push_back(RefCurrentChar());
            return ScanDecimalNumber();
        default:
            throw Exception("Invalid character found in a number token.");
        }
    }

    /**
     * @brief Scan a next character after the negative sign(-).
     *
     * @return LexicalTokenType The lexical token type for either signed or float numbers.
     */
    LexicalTokenType ScanNegativeNumber()
    {
        const char& next = GetNextChar();

        if (std::isdigit(next))
        {
            m_value_buffer.push_back(next);
            const LexicalTokenType ret = ScanDecimalNumber();
            return (ret == LexicalTokenType::FLOAT_NUMBER_VALUE) ? ret : LexicalTokenType::SIGNED_INT_VALUE;
        }

        if (next == '.')
        {
            const std::string tmp_str = m_input_buffer.substr(m_position_info.total_read_char_counts, 4);
            if (tmp_str == ".inf" || tmp_str == ".Inf" || tmp_str == ".INF")
            {
                m_value_buffer += tmp_str;
                for (int i = 0; i < 4; ++i)
                {
                    GetNextChar();
                }
                return LexicalTokenType::FLOAT_NUMBER_VALUE;
            }
        }
        throw Exception("Invalid character found in a negative number token.");
    }

    /**
     * @brief Scan a next character after '0' at the beginning of a token.
     *
     * @return LexicalTokenType The lexical token type for one of number types(signed/unsigned/float).
     */
    LexicalTokenType ScanNumberAfterZeroAtFirst()
    {
        const char& next = GetNextChar();
        switch (next)
        {
        case '.':
            m_value_buffer.push_back(next);
            return ScanDecimalNumberAfterDecimalPoint();
        case 'o':
            // Do not store 'o' since std::strtoull does not support "0o" but "0" as the prefix for octal numbers.
            // YAML specifies octal values start with the prefix "0o".
            // See "10.3.2. Tag Resolution" section in https://yaml.org/spec/1.2.2/
            return ScanOctalNumber();
        case 'x':
            m_value_buffer.push_back(next);
            return ScanHexadecimalNumber();
        default:
            return LexicalTokenType::UNSIGNED_INT_VALUE;
        }
    }

    /**
     * @brief Scan a next character after a decimal point.
     *
     * @return LexicalTokenType The lexical token type for float numbers.
     */
    LexicalTokenType ScanDecimalNumberAfterDecimalPoint()
    {
        const char& next = GetNextChar();

        if (std::isdigit(next))
        {
            m_value_buffer.push_back(next);
            ScanDecimalNumber();
            return LexicalTokenType::FLOAT_NUMBER_VALUE;
        }

        throw Exception("Invalid character found after a decimal point.");
    }

    /**
     * @brief Scan a next character after exponent(e/E).
     *
     * @return LexicalTokenType The lexical token type for float numbers.
     */
    LexicalTokenType ScanDecimalNumberAfterExponent()
    {
        const char& next = GetNextChar();
        if (next == '+' || next == '-')
        {
            m_value_buffer.push_back(next);
            ScanDecimalNumberAfterSign();
        }
        else if (std::isdigit(next))
        {
            m_value_buffer.push_back(next);
            ScanDecimalNumber();
        }
        return LexicalTokenType::FLOAT_NUMBER_VALUE;
    }

    /**
     * @brief Scan a next character after a sign(+/-) after exponent(e/E).
     *
     * @return LexicalTokenType The lexical token type for one of number types(signed/unsigned/float)
     */
    LexicalTokenType ScanDecimalNumberAfterSign()
    {
        const char& next = GetNextChar();
        if (std::isdigit(next))
        {
            m_value_buffer.push_back(next);
            return ScanDecimalNumber();
        }
        throw Exception("Non-numeric character found after a sign(+/-) after exponent(e/E).");
    }

    /**
     * @brief Scan a next character for decimal numbers.
     *
     * @return LexicalTokenType The lexical token type for one of number types(signed/unsigned/float)
     */
    LexicalTokenType ScanDecimalNumber()
    {
        const char& next = GetNextChar();

        if (std::isdigit(next))
        {
            m_value_buffer.push_back(next);
            return ScanDecimalNumber();
        }

        if (next == '.')
        {
            if (m_value_buffer.find(next) != std::string::npos) // NOLINT(abseil-string-find-str-contains)
            {
                throw Exception("Multiple decimal points found in a token.");
            }
            m_value_buffer.push_back(next);
            return ScanDecimalNumberAfterDecimalPoint();
        }

        if (next == 'e' || next == 'E')
        {
            m_value_buffer.push_back(next);
            return ScanDecimalNumberAfterExponent();
        }

        return LexicalTokenType::UNSIGNED_INT_VALUE;
    }

    /**
     * @brief Scan a next character for octal numbers.
     * @note All octal numbers are interpreted as unsigned integers.
     *
     * @return LexicalTokenType The lexical token type for unsigned numbers.
     */
    LexicalTokenType ScanOctalNumber()
    {
        const char& next = GetNextChar();
        if ('0' <= next && next <= '7')
        {
            m_value_buffer.push_back(next);
            ScanOctalNumber();
        }
        return LexicalTokenType::UNSIGNED_INT_VALUE;
    }

    /**
     * @brief Scan a next character for hexadecimal numbers.
     * @note All hexadecimal numbers are interpreted as unsigned integers.
     *
     * @return LexicalTokenType The lexical token type for unsigned numbers.
     */
    LexicalTokenType ScanHexadecimalNumber()
    {
        const char& next = GetNextChar();
        if (std::isxdigit(next))
        {
            m_value_buffer.push_back(next);
            ScanHexadecimalNumber();
        }
        return LexicalTokenType::UNSIGNED_INT_VALUE;
    }

    /**
     * @brief Scan a string token(unquoted/single-quoted/double-quoted).
     * @note Multibyte characters(including escaped ones) are currently unsupported.
     *
     * @return LexicalTokenType The lexical token type for strings.
     */
    LexicalTokenType ScanString()
    {
        m_value_buffer.clear();

        const bool needs_last_double_quote = (RefCurrentChar() == '\"');
        const bool needs_last_single_quote = (RefCurrentChar() == '\'');
        char current = (needs_last_double_quote || needs_last_single_quote) ? GetNextChar() : RefCurrentChar();

        for (;; current = GetNextChar())
        {
            if (current == '\"')
            {
                if (needs_last_double_quote)
                {
                    return LexicalTokenType::STRING_VALUE;
                }

                if (!needs_last_single_quote)
                {
                    throw Exception("Invalid double quotation mark found in a string token.");
                }

                // if the target is a single-quoted string token.
                m_value_buffer.push_back(current);
                continue;
            }

            // Handle single quotation marks.
            if (current == '\'')
            {
                if (needs_last_double_quote || !needs_last_single_quote)
                {
                    throw Exception("Invalid single quotation mark found in a string token.");
                }

                // If single quotation marks are repeated twice in a single-quoted string token. they are considered as
                // an escaped single quotation mark.
                if (RefNextChar() == '\'')
                {
                    m_value_buffer.push_back(GetNextChar());
                    continue;
                }

                return LexicalTokenType::STRING_VALUE;
            }

            // Handle colons.
            if (current == ':')
            {
                // Just regard a colon as a character if surrounded by quotation marks.
                if (needs_last_double_quote || needs_last_single_quote)
                {
                    m_value_buffer.push_back(current);
                    continue;
                }

                // A colon as a key separator must be followed by a space.
                if (RefNextChar() != ' ')
                {
                    m_value_buffer.push_back(current);
                    continue;
                }

                return LexicalTokenType::STRING_VALUE;
            }

            // Handle commas.
            if (current == ',')
            {
                // Just regard a comma as a character if surrounded by quotation marks.
                if (needs_last_double_quote || needs_last_single_quote)
                {
                    m_value_buffer.push_back(current);
                    continue;
                }

                return LexicalTokenType::STRING_VALUE;
            }

            // Handle right square brackets.
            if (current == ']')
            {
                // just regard a right square bracket as a character if surrounded by quotation marks.
                if (needs_last_double_quote || needs_last_single_quote)
                {
                    m_value_buffer.push_back(current);
                    continue;
                }

                // Trim trailing spaces already added to m_value_buffer.
                while (m_value_buffer.back() == ' ')
                {
                    m_value_buffer.pop_back();
                }

                return LexicalTokenType::STRING_VALUE;
            }

            // Handle right curly braces.
            if (current == '}')
            {
                // just regard a right curly brace as a character if surrounded by quotation marks.
                if (needs_last_double_quote || needs_last_single_quote)
                {
                    m_value_buffer.push_back(current);
                    continue;
                }

                // Trim trailing spaces already added to m_value_buffer.
                while (m_value_buffer.back() == ' ')
                {
                    m_value_buffer.pop_back();
                }

                return LexicalTokenType::STRING_VALUE;
            }

            // Handle newline codes.
            if (current == '\r' || current == '\n')
            {
                if (!needs_last_double_quote && !needs_last_single_quote)
                {
                    return LexicalTokenType::STRING_VALUE;
                }

                SkipWhiteSpaces();
                continue;
            }

            // Handle the end of input buffer.
            if (current == '\0' || current == s_EOF)
            {
                if (needs_last_double_quote)
                {
                    throw Exception("Invalid end of input buffer in a double-quoted string token.");
                }

                if (needs_last_single_quote)
                {
                    throw Exception("Invalid end of input buffer in a single-quoted string token.");
                }

                return LexicalTokenType::STRING_VALUE;
            }

            // Handle escaped characters.
            // See "5.7. Escaped Characters" section in https://yaml.org/spec/1.2.2/
            if (current == '\\')
            {
                if (!needs_last_double_quote)
                {
                    throw Exception("Escaped characters are only available in a double-quoted string token.");
                }

                current = GetNextChar();
                switch (current)
                {
                case '0':
                    m_value_buffer.push_back('\0');
                    break;
                case 'a':
                    m_value_buffer.push_back('\a');
                    break;
                case 'b':
                    m_value_buffer.push_back('\b');
                    break;
                case 't':
                    m_value_buffer.push_back('\t');
                    break;
                case 'n':
                    m_value_buffer.push_back('\n');
                    break;
                case 'v':
                    m_value_buffer.push_back('\v');
                    break;
                case 'f':
                    m_value_buffer.push_back('\f');
                    break;
                case 'r':
                    m_value_buffer.push_back('\r');
                    break;
                case 'e':
                    m_value_buffer.push_back('\u001B');
                    break;
                case ' ':
                    m_value_buffer.push_back('\u0020');
                    break;
                case '\"':
                    m_value_buffer.push_back('\"');
                    break;
                case '/':
                    m_value_buffer.push_back('/');
                    break;
                case '\\':
                    m_value_buffer.push_back('\\');
                    break;
                case 'x': {
                    char byte = 0;
                    for (int i = 1; i >= 0; --i)
                    {
                        char four_bits = ConvertHexCharToByte(GetNextChar());
                        byte |= static_cast<char>(four_bits << (4 * i));
                    }
                    m_value_buffer.push_back(byte);
                }
                // Multibyte characters are currently unsupported.
                // Thus \N, \_, \L, \P \uXX, \UXXXX are currently unavailable.
                default:
                    throw Exception("Unsupported escape sequence found in a string token.");
                }
            }

            // Handle ASCII characters except control characters.
            if (0x20 <= current && current <= 0x7E)
            {
                m_value_buffer.push_back(current);
                continue;
            }

            // Handle unescaped control characters.
            switch (current)
            {
            case 0x00:
                throw Exception("Control character U+0000 (NUL) must be escaped to \\0 or \\u0000.");
            case 0x01:
                throw Exception("Control character U+0001 (SOH) must be escaped to \\u0001.");
            case 0x02:
                throw Exception("Control character U+0002 (STX) must be escaped to \\u0002.");
            case 0x03:
                throw Exception("Control character U+0003 (ETX) must be escaped to \\u0003.");
            case 0x04:
                throw Exception("Control character U+0004 (EOT) must be escaped to \\u0004.");
            case 0x05:
                throw Exception("Control character U+0005 (ENQ) must be escaped to \\u0005.");
            case 0x06:
                throw Exception("Control character U+0006 (ACK) must be escaped to \\u0006.");
            case 0x07:
                throw Exception("Control character U+0007 (BEL) must be escaped to \\a or \\u0007.");
            case 0x08:
                throw Exception("Control character U+0008 (BS) must be escaped to \\b or \\u0008.");
            case 0x09: // HT
                m_value_buffer.push_back(current);
                break;
            // 0x0A(LF) has already been handled above.
            case 0x0B:
                throw Exception("Control character U+000B (VT) must be escaped to \\v or \\u000B.");
            case 0x0C:
                throw Exception("Control character U+000C (FF) must be escaped to \\f or \\u000C.");
            // 0x0D(CR) has already been handled above.
            case 0x0E:
                throw Exception("Control character U+000E (SO) must be escaped to \\u000E.");
            case 0x0F:
                throw Exception("Control character U+000F (SI) must be escaped to \\u000F.");
            case 0x10:
                throw Exception("Control character U+0010 (DLE) must be escaped to \\u0010.");
            case 0x11:
                throw Exception("Control character U+0011 (DC1) must be escaped to \\u0011.");
            case 0x12:
                throw Exception("Control character U+0012 (DC2) must be escaped to \\u0012.");
            case 0x13:
                throw Exception("Control character U+0013 (DC3) must be escaped to \\u0013.");
            case 0x14:
                throw Exception("Control character U+0014 (DC4) must be escaped to \\u0014.");
            case 0x15:
                throw Exception("Control character U+0015 (NAK) must be escaped to \\u0015.");
            case 0x16:
                throw Exception("Control character U+0016 (SYN) must be escaped to \\u0016.");
            case 0x17:
                throw Exception("Control character U+0017 (ETB) must be escaped to \\u0017.");
            case 0x18:
                throw Exception("Control character U+0018 (CAN) must be escaped to \\u0018.");
            case 0x19:
                throw Exception("Control character U+0019 (EM) must be escaped to \\u0019.");
            case 0x1A:
                throw Exception("Control character U+001A (SUB) must be escaped to \\u001A.");
            case 0x1B:
                throw Exception("Control character U+001B (ESC) must be escaped to \\e or \\u001B.");
            case 0x1C:
                throw Exception("Control character U+001C (FS) must be escaped to \\u001C.");
            case 0x1D:
                throw Exception("Control character U+001D (GS) must be escaped to \\u001D.");
            case 0x1E:
                throw Exception("Control character U+001E (RS) must be escaped to \\u001E.");
            case 0x1F:
                throw Exception("Control character U+001F (US) must be escaped to \\u001F.");
            // 0x20('0')~0x7E('~') have already been handled above.
            // 16bit, 32bit characters are currently not supported.
            default:
                throw Exception("Unsupported multibytes character found.");
            }
        }
    }

    /**
     * @brief Get reference to the current character from the input buffer without position updates.
     *
     * @return const char& Constant reference to the current character.
     */
    const char& RefCurrentChar() const noexcept
    {
        return m_input_buffer[m_position_info.total_read_char_counts];
    }

    /**
     * @brief Get reference to the next character from the input buffer without position updates.
     *
     * @return const char& Constant reference to the next character.
     */
    const char& RefNextChar() const noexcept
    {
        return m_input_buffer[m_position_info.total_read_char_counts + 1];
    }

    /**
     * @brief Get reference to the next character from the input buffer with position updates.
     *
     * @return const char& Constant reference to the next character.
     */
    const char& GetNextChar() noexcept
    {
        const char& current = m_input_buffer[++m_position_info.total_read_char_counts];
        ++m_position_info.read_char_counts_in_line;
        if (current == '\n')
        {
            ++m_position_info.total_read_line_counts;
            m_position_info.prev_char_counts_in_line = m_position_info.read_char_counts_in_line;
            m_position_info.read_char_counts_in_line = 0;
        }
        return current;
    }

    /**
     * @brief Skip white spaces, tabs and newline codes until any other kind of character is found.
     */
    void SkipWhiteSpaces()
    {
        while (true)
        {
            switch (RefCurrentChar())
            {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                break;
            default:
                return;
            }
            GetNextChar();
        }
    }

private:
    //!< The value of EOF.
    static constexpr char_int_type s_EOF = char_traits_type::eof();

    //!< An input buffer to be analyzed.
    std::string m_input_buffer {};
    //!< The information set for the input buffer.
    Position m_position_info {};
    //!< The current indent width.
    size_t m_current_indent_width = 0;
    //!< A temporal buffer to store a string to be parsed to an actual datum.
    std::string m_value_buffer {};
};

} // namespace fkyaml

#endif /* FK_YAML_LEXICAL_ANALIZER_HPP_ */