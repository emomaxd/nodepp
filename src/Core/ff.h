#pragma once

#include <iostream>
#include <sstream>
#include <bitset>
#include <type_traits>
#include <iomanip>
#include <cassert>
#include <string>
#include <vector>
#include <cctype>
#include <cstdint>
#include <algorithm>

namespace ff {

    /* Converts a value to its binary string representation without leading zeros */
    template <typename T>
    std::string format_binary(const T& value) {
        std::string binary_str = std::bitset<sizeof(T) * 8>(value).to_string();
        size_t first_one_pos = binary_str.find('1');
        if (first_one_pos == std::string::npos) return "0";
        return binary_str.substr(first_one_pos);
    }

    // Enum to specify integer format types
    enum class IntegerFormatSpec {
        Decimal,
        HexadecimalLower,
        HexadecimalUpper,
        Octal,
        Binary
    };

    /* Specifies float format type and optional precision */
    struct FloatFormat {
        enum class Type { Decimal, FixedPrecision, Scientific, Binary };
        Type type;
        int precision;

        FloatFormat(Type type = Type::Decimal, int precision = 0) 
            : type(type), precision(precision) {}
    };

    /* Parses string to determine integer format specification */
    static IntegerFormatSpec parse_integer_format_spec(const std::string& format_spec) {
        if (format_spec == "x") return IntegerFormatSpec::HexadecimalLower;
        if (format_spec == "X") return IntegerFormatSpec::HexadecimalUpper;
        if (format_spec == "o") return IntegerFormatSpec::Octal;
        if (format_spec == "b") return IntegerFormatSpec::Binary;
        return IntegerFormatSpec::Decimal;
    }

    /* Parses string to determine float format specification */
    static FloatFormat parse_float_format_spec(const std::string& format_spec) {
        if (format_spec == "s") return {FloatFormat::Type::Scientific};
        if (format_spec == "b") return {FloatFormat::Type::Binary};
        if (format_spec[0] == '.') {
            int precision = std::stoi(format_spec.substr(1));
            return {FloatFormat::Type::FixedPrecision, precision};
        }
        return {FloatFormat::Type::Decimal};
    }

    /* Counts the number of placeholders in the format string */
    static size_t count_placeholders(const std::string& formatStr) {
        size_t count = 0;
        for (size_t i = 0; i < formatStr.size(); ++i) {
            if (formatStr[i] == '{') {
                auto close_pos = formatStr.find('}', i);
                assert(close_pos != std::string::npos && "ASSERTION FAILED: Unmatched '{' in format string");
                ++count;
                i = close_pos;
            }
        }
        return count;
    }

    /* Formats integer values based on the specified integer format */
    template <typename T>
    std::string format_integer(const T& value, IntegerFormatSpec format_spec) {
        std::ostringstream oss;
        switch (format_spec) {
            case IntegerFormatSpec::HexadecimalLower: oss << "0x" << std::hex << value; break;
            case IntegerFormatSpec::HexadecimalUpper: oss << "0x" << std::hex << std::uppercase << value; break;
            case IntegerFormatSpec::Octal: oss << "0o" << std::oct << value; break;
            case IntegerFormatSpec::Binary: oss << "0b" << format_binary(value); break;
            default: oss << value;
        }
        return oss.str();
    }

    /* Formats float values based on the specified float format */
    template <typename T>
    std::string format_float(const T& value, FloatFormat float_format) {
        std::ostringstream oss;
        switch (float_format.type) {
            case FloatFormat::Type::FixedPrecision:
                oss << std::fixed << std::setprecision(float_format.precision) << value; break;
            case FloatFormat::Type::Scientific:
                oss << std::scientific << value; break;
            case FloatFormat::Type::Binary: {
                auto raw_bits = *reinterpret_cast<const uint32_t*>(&value);
                std::string binary_str = format_binary(raw_bits);
                oss << "0b" << binary_str;
                break;
            }
            default: oss << value;
        }
        return oss.str();
    }

    /* Applies integer or float formatting based on the type of the input */
    template <typename T>
    typename std::enable_if<std::is_integral<T>::value, std::string>::type
    apply_format(const T& value, const std::string& format_spec) {
        return format_integer(value, parse_integer_format_spec(format_spec));
    }

    template <typename T>
    typename std::enable_if<std::is_floating_point<T>::value, std::string>::type
    apply_format(const T& value, const std::string& format_spec) {
        return format_float(value, parse_float_format_spec(format_spec));
    }

    /* Retrieves argument by index in a parameter pack */
    template <typename T>
    T get_arg_by_index(size_t, const T& arg) {
        return arg;
    }

    template <typename T, typename... Rest>
    T get_arg_by_index(size_t index, const T& first, const Rest&... rest) {
        return index == 0 ? first : get_arg_by_index(index - 1, rest...);
    }

    /* Formats a string with placeholders using specified format specifiers for each argument */
    template <typename... Args>
    std::string format(const std::string& formatStr, const Args&... args) {
        size_t num_placeholders = count_placeholders(formatStr);
        std::ostringstream oss;
        std::string::size_type pos = 0, last_pos = 0;
        size_t placeholder_index = 0;

        for (size_t i = 0; i < num_placeholders; ++i) {
            pos = formatStr.find('{', last_pos);
            if (pos == std::string::npos) break;

            oss << formatStr.substr(last_pos, pos - last_pos);

            size_t close_pos = formatStr.find('}', pos);
            std::string format_spec = formatStr.substr(pos + 1, close_pos - pos - 1);

            size_t index = placeholder_index;
            size_t colon_pos = format_spec.find(':');
            if (colon_pos != std::string::npos) {
                std::string argument = format_spec.substr(0, colon_pos);
                std::string specifier = format_spec.substr(colon_pos + 1);

                if (!argument.empty() && std::all_of(argument.begin(), argument.end(), ::isdigit)) {
                    index = std::stoi(argument);
                }
                oss << apply_format(get_arg_by_index(index, args...), specifier);
            } else {
                if (!format_spec.empty() && std::all_of(format_spec.begin(), format_spec.end(), ::isdigit)) {
                    index = std::stoi(format_spec);
                }
                oss << apply_format(get_arg_by_index(index, args...), format_spec);
            }

            last_pos = close_pos + 1;
            ++placeholder_index;
        }

        oss << formatStr.substr(last_pos);
        return oss.str();
    }

}