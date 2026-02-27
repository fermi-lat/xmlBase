#ifndef SAFE_XML_PARSER_AHPP
#define SAFE_XML_PARSER_HPP

#include "rapidxml.hpp"
#include "rapidxml_error_framework.hpp"
#include "xml_result.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <functional>
#include <charconv>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string_view>
#include <optional>
#include <type_traits>

namespace xml_framework {

// ==================== Forward Declarations ====================

template<typename T>
struct VectorParser;

// ==================== SafeXmlParser Class ====================

class SafeXmlParser {
public:
    using XmlDocument = rapidxml::xml_document<>;
    using XmlNode = rapidxml::xml_node<>;
    using XmlAttribute = rapidxml::xml_attribute<>;

    // ==================== Configuration ====================
    
    // Default delimiter for parsing vector values from strings
    inline static char vectorDelimiter = ',';
    
    // Set custom delimiter for vector parsing
    static void setVectorDelimiter(char delim) noexcept {
        vectorDelimiter = delim;
    }
    
    [[nodiscard]] static char getVectorDelimiter() noexcept {
        return vectorDelimiter;
    }

    // ==================== Position Calculation ====================
    
    [[nodiscard]] static std::pair<size_t, size_t> calculatePosition(
        const char* source, 
        const char* position) noexcept {
        size_t line = 1;
        size_t column = 1;
        for (const char* p = source; p < position && *p; ++p) {
            if (*p == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
        }
        return {line, column};
    }

    // ==================== XML Parsing ====================
    
    static void parseString(XmlDocument& doc, char* xmlString, 
                           [[maybe_unused]] int flags = 0) {
        try {
            doc.parse<0>(xmlString);
        } catch (const rapidxml::parse_error& e) {
            auto [line, col] = calculatePosition(xmlString, e.where<char>());
            throw XmlParseException(e.what(), line, col);
        }
    }

    [[nodiscard]] static XmlResult<void> tryParseString(
        XmlDocument& doc, 
        char* xmlString,
        [[maybe_unused]] int flags = 0) noexcept {
        try {
            doc.parse<0>(xmlString);
            return XmlResult<void>::success();
        } catch (const rapidxml::parse_error& e) {
            auto [line, col] = calculatePosition(xmlString, e.where<char>());
            return XmlResult<void>::error(
                XmlError{XmlErrorCode::ParseError, e.what(), "", line, col}
            );
        } catch (const std::exception& e) {
            return XmlResult<void>::error(
                XmlError{XmlErrorCode::ParseError, e.what()}
            );
        } catch (...) {
            return XmlResult<void>::error(
                XmlError{XmlErrorCode::ParseError, "Unknown parsing error"}
            );
        }
    }

    [[nodiscard]] static XmlResult<std::vector<char>> loadFile(
        const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return XmlResult<std::vector<char>>::error(
                XmlErrorCode::FileNotFound, 
                "Cannot open file: " + filename
            );
        }

        const auto size = file.tellg();
        if (size < 0) {
            return XmlResult<std::vector<char>>::error(
                XmlErrorCode::ParseError,
                "Failed to determine file size: " + filename
            );
        }
        
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(static_cast<size_t>(size) + 1);
        if (!file.read(buffer.data(), size)) {
            return XmlResult<std::vector<char>>::error(
                XmlErrorCode::ParseError,
                "Failed to read file: " + filename
            );
        }
        buffer[static_cast<size_t>(size)] = '\0';

        return XmlResult<std::vector<char>>::success(std::move(buffer));
    }

    // ==================== Node Access ====================
    
    [[nodiscard]] static XmlNode* getRequiredNode(
        XmlNode* parent, 
        const char* name,
        const std::string& context = "") {
        if (!parent) {
            throw XmlNodeNotFoundException(name, "null parent");
        }

        XmlNode* node = parent->first_node(name);
        if (!node) {
            throw XmlNodeNotFoundException(name, context);
        }
        return node;
    }

    [[nodiscard]] static XmlResult<XmlNode*> tryGetNode(
        XmlNode* parent, 
        const char* name) noexcept {
        if (!parent) {
            return XmlResult<XmlNode*>::error(
                XmlErrorCode::NodeNotFound, 
                "Parent node is null"
            );
        }

        XmlNode* node = parent->first_node(name);
        if (!node) {
            return XmlResult<XmlNode*>::error(
                XmlErrorCode::NodeNotFound,
                std::string("Node '") + name + "' not found"
            );
        }
        return XmlResult<XmlNode*>::success(node);
    }

    [[nodiscard]] static std::optional<XmlNode*> getOptionalNode(
        XmlNode* parent, 
        const char* name) noexcept {
        if (!parent) {
            return std::nullopt;
        }
        XmlNode* node = parent->first_node(name);
        return node ? std::make_optional(node) : std::nullopt;
    }

    // ==================== Attribute Access ====================
    
    [[nodiscard]] static XmlAttribute* getRequiredAttribute(
        XmlNode* node, 
        const char* name) {
        if (!node) {
            throw XmlAttributeNotFoundException(name, "null node");
        }

        XmlAttribute* attr = node->first_attribute(name);
        if (!attr) {
            throw XmlAttributeNotFoundException(
                name, 
                node->name() ? node->name() : "unknown"
            );
        }
        return attr;
    }

    [[nodiscard]] static XmlResult<XmlAttribute*> tryGetAttribute(
        XmlNode* node, 
        const char* name) noexcept {
        if (!node) {
            return XmlResult<XmlAttribute*>::error(
                XmlErrorCode::AttributeNotFound,
                "Node is null"
            );
        }

        XmlAttribute* attr = node->first_attribute(name);
        if (!attr) {
            return XmlResult<XmlAttribute*>::error(
                XmlErrorCode::AttributeNotFound,
                std::string("Attribute '") + name + "' not found"
            );
        }
        return XmlResult<XmlAttribute*>::success(attr);
    }

    [[nodiscard]] static std::optional<XmlAttribute*> getOptionalAttribute(
        XmlNode* node,
        const char* name) noexcept {
        if (!node) {
            return std::nullopt;
        }
        XmlAttribute* attr = node->first_attribute(name);
        return attr ? std::make_optional(attr) : std::nullopt;
    }

    // ==================== Value Parsing (Primary Template Declaration) ====================
    
    template<typename T>
    [[nodiscard]] static XmlResult<T> parseValue(
        const char* str, 
        const std::string& ctx = "");

    // Internal helper for parsing single values (used by vector parser)
    template<typename T>
    [[nodiscard]] static XmlResult<T> parseSingleValue(
        std::string_view sv, 
        const std::string& ctx);

    // ==================== Node/Attribute Value Extraction ====================
    
    template<typename T>
    [[nodiscard]] static XmlResult<T> getNodeValue(XmlNode* node) {
        if (!node) {
            return XmlResult<T>::error(
                XmlErrorCode::NodeNotFound,
                "Node is null"
            );
        }
        
        const char* nodeValue = node->value();
        if (!nodeValue) {
            return XmlResult<T>::error(
                XmlErrorCode::NodeNotFound,
                "Node value is null"
            );
        }
        
        const char* nodeName = node->name();
        return parseValue<T>(nodeValue, nodeName ? nodeName : "");
    }

    template<typename T>
    [[nodiscard]] static XmlResult<T> getAttributeValue(
        XmlNode* node, 
        const char* attrName) {
        auto attrResult = tryGetAttribute(node, attrName);
        if (attrResult.isError()) {
            return XmlResult<T>::error(attrResult.error());
        }
        return parseValue<T>(attrResult.value()->value(), attrName);
    }

    template<typename T>
    [[nodiscard]] static T getNodeValueOr(XmlNode* node, T defaultValue) {
        return getNodeValue<T>(node).valueOr(std::move(defaultValue));
    }

    template<typename T>
    [[nodiscard]] static T getAttributeValueOr(
        XmlNode* node, 
        const char* attrName, 
        T defaultValue) {
        return getAttributeValue<T>(node, attrName).valueOr(std::move(defaultValue));
    }

    // ==================== Child Iteration ====================
    
    static void forEachChild(
        XmlNode* parent, 
        const char* childName,
        const std::function<void(XmlNode*)>& callback,
        bool throwOnEmpty = false) {
        if (!parent) {
            if (throwOnEmpty) {
                throw XmlNodeNotFoundException(childName ? childName : "*", "null parent");
            }
            return;
        }

        bool found = false;
        for (XmlNode* child = parent->first_node(childName);
             child != nullptr;
             child = child->next_sibling(childName)) {
            found = true;
            callback(child);
        }

        if (!found && throwOnEmpty) {
            const char* parentName = parent->name();
            throw XmlNodeNotFoundException(
                childName ? childName : "*", 
                parentName ? parentName : "unknown"
            );
        }
    }

    [[nodiscard]] static std::vector<XmlNode*> collectChildren(
        XmlNode* parent, 
        const char* childName = nullptr) {
        std::vector<XmlNode*> children;
        if (!parent) {
            return children;
        }

        for (XmlNode* child = parent->first_node(childName);
             child != nullptr;
             child = child->next_sibling(childName)) {
            children.push_back(child);
        }
        return children;
    }

    // ==================== Vector Value Collection from Child Nodes ====================
    
    template<typename T>
    [[nodiscard]] static XmlResult<std::vector<T>> collectChildValues(
        XmlNode* parent,
        const char* childName) {
        if (!parent) {
            return XmlResult<std::vector<T>>::error(
                XmlErrorCode::NodeNotFound, 
                "Parent node is null"
            );
        }

        std::vector<T> values;
        XmlError aggregateError{
            XmlErrorCode::PartialFailure, 
            "Some child values failed to parse"
        };
        bool hasErrors = false;

        for (XmlNode* child = parent->first_node(childName);
             child != nullptr;
             child = child->next_sibling(childName)) {
            auto result = getNodeValue<T>(child);
            if (result.isSuccess()) {
                values.push_back(std::move(result).value());
            } else {
                hasErrors = true;
                aggregateError.addNestedError(std::move(result).error());
            }
        }

        if (values.empty() && hasErrors) {
            return XmlResult<std::vector<T>>::error(std::move(aggregateError));
        }

        return XmlResult<std::vector<T>>::success(std::move(values));
    }

    template<typename T>
    [[nodiscard]] static XmlResult<std::vector<T>> collectChildValuesStrict(
        XmlNode* parent,
        const char* childName) {
        if (!parent) {
            return XmlResult<std::vector<T>>::error(
                XmlErrorCode::NodeNotFound, 
                "Parent node is null"
            );
        }

        std::vector<T> values;

        for (XmlNode* child = parent->first_node(childName);
             child != nullptr;
             child = child->next_sibling(childName)) {
            auto result = getNodeValue<T>(child);
            if (result.isError()) {
                return XmlResult<std::vector<T>>::error(std::move(result).error());
            }
            values.push_back(std::move(result).value());
        }

        return XmlResult<std::vector<T>>::success(std::move(values));
    }

    // ==================== String Utility Functions ====================
    
    [[nodiscard]] static std::string_view trim(std::string_view sv) noexcept {
        while (!sv.empty() && 
               std::isspace(static_cast<unsigned char>(sv.front()))) {
            sv.remove_prefix(1);
        }
        while (!sv.empty() && 
               std::isspace(static_cast<unsigned char>(sv.back()))) {
            sv.remove_suffix(1);
        }
        return sv;
    }

    [[nodiscard]] static std::vector<std::string_view> split(
        std::string_view sv, 
        char delimiter) {
        std::vector<std::string_view> parts;
        size_t start = 0;
        
        for (size_t i = 0; i <= sv.size(); ++i) {
            if (i == sv.size() || sv[i] == delimiter) {
                auto part = trim(sv.substr(start, i - start));
                if (!part.empty()) {
                    parts.push_back(part);
                }
                start = i + 1;
            }
        }
        
        return parts;
    }

    // Grant VectorParser access to private/protected members
    template<typename T>
    friend struct VectorParser;
};

// ==================== Scalar Type Specializations ====================

template<>
[[nodiscard]] inline XmlResult<std::string> SafeXmlParser::parseValue<std::string>(
    const char* str, 
    [[maybe_unused]] const std::string& ctx) {
    return XmlResult<std::string>::success(str ? std::string(str) : std::string{});
}

template<>
[[nodiscard]] inline XmlResult<std::string_view> SafeXmlParser::parseValue<std::string_view>(
    const char* str, 
    [[maybe_unused]] const std::string& ctx) {
    return XmlResult<std::string_view>::success(
        str ? std::string_view(str) : std::string_view{}
    );
}

template<>
[[nodiscard]] inline XmlResult<int> SafeXmlParser::parseValue<int>(
    const char* str, 
    const std::string& ctx) {
    if (!str || *str == '\0') {
        return XmlResult<int>::error(
            XmlErrorCode::TypeConversionError,
            "Empty string cannot be converted to int",
            ctx
        );
    }

    // Skip leading whitespace
    while (*str && std::isspace(static_cast<unsigned char>(*str))) {
        ++str;
    }
    
    const char* end = str + std::strlen(str);
    
    // Skip trailing whitespace for end pointer
    while (end > str && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    
    int value{};
    auto [ptr, ec] = std::from_chars(str, end, value);
    
    if (ec != std::errc{} || ptr != end) {
        return XmlResult<int>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(str) + "' to int",
            ctx
        );
    }
    return XmlResult<int>::success(value);
}

template<>
[[nodiscard]] inline XmlResult<long> SafeXmlParser::parseValue<long>(
    const char* str, 
    const std::string& ctx) {
    if (!str || *str == '\0') {
        return XmlResult<long>::error(
            XmlErrorCode::TypeConversionError,
            "Empty string cannot be converted to long",
            ctx
        );
    }

    while (*str && std::isspace(static_cast<unsigned char>(*str))) {
        ++str;
    }
    
    const char* end = str + std::strlen(str);
    while (end > str && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    
    long value{};
    auto [ptr, ec] = std::from_chars(str, end, value);
    
    if (ec != std::errc{} || ptr != end) {
        return XmlResult<long>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(str) + "' to long",
            ctx
        );
    }
    return XmlResult<long>::success(value);
}

template<>
[[nodiscard]] inline XmlResult<long long> SafeXmlParser::parseValue<long long>(
    const char* str, 
    const std::string& ctx) {
    if (!str || *str == '\0') {
        return XmlResult<long long>::error(
            XmlErrorCode::TypeConversionError,
            "Empty string cannot be converted to long long",
            ctx
        );
    }

    while (*str && std::isspace(static_cast<unsigned char>(*str))) {
        ++str;
    }
    
    const char* end = str + std::strlen(str);
    while (end > str && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    
    long long value{};
    auto [ptr, ec] = std::from_chars(str, end, value);
    
    if (ec != std::errc{} || ptr != end) {
        return XmlResult<long long>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(str) + "' to long long",
            ctx
        );
    }
    return XmlResult<long long>::success(value);
}

template<>
[[nodiscard]] inline XmlResult<unsigned int> SafeXmlParser::parseValue<unsigned int>(
    const char* str, 
    const std::string& ctx) {
    if (!str || *str == '\0') {
        return XmlResult<unsigned int>::error(
            XmlErrorCode::TypeConversionError,
            "Empty string cannot be converted to unsigned int",
            ctx
        );
    }

    while (*str && std::isspace(static_cast<unsigned char>(*str))) {
        ++str;
    }
    
    const char* end = str + std::strlen(str);
    while (end > str && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    
    unsigned int value{};
    auto [ptr, ec] = std::from_chars(str, end, value);
    
    if (ec != std::errc{} || ptr != end) {
        return XmlResult<unsigned int>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(str) + "' to unsigned int",
            ctx
        );
    }
    return XmlResult<unsigned int>::success(value);
}

template<>
[[nodiscard]] inline XmlResult<unsigned long> SafeXmlParser::parseValue<unsigned long>(
    const char* str, 
    const std::string& ctx) {
    if (!str || *str == '\0') {
        return XmlResult<unsigned long>::error(
            XmlErrorCode::TypeConversionError,
            "Empty string cannot be converted to unsigned long",
            ctx
        );
    }

    while (*str && std::isspace(static_cast<unsigned char>(*str))) {
        ++str;
    }
    
    const char* end = str + std::strlen(str);
    while (end > str && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    
    unsigned long value{};
    auto [ptr, ec] = std::from_chars(str, end, value);
    
    if (ec != std::errc{} || ptr != end) {
        return XmlResult<unsigned long>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(str) + "' to unsigned long",
            ctx
        );
    }
    return XmlResult<unsigned long>::success(value);
}

template<>
[[nodiscard]] inline XmlResult<unsigned long long> SafeXmlParser::parseValue<unsigned long long>(
    const char* str, 
    const std::string& ctx) {
    if (!str || *str == '\0') {
        return XmlResult<unsigned long long>::error(
            XmlErrorCode::TypeConversionError,
            "Empty string cannot be converted to unsigned long long",
            ctx
        );
    }

    while (*str && std::isspace(static_cast<unsigned char>(*str))) {
        ++str;
    }
    
    const char* end = str + std::strlen(str);
    while (end > str && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    
    unsigned long long value{};
    auto [ptr, ec] = std::from_chars(str, end, value);
    
    if (ec != std::errc{} || ptr != end) {
        return XmlResult<unsigned long long>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(str) + "' to unsigned long long",
            ctx
        );
    }
    return XmlResult<unsigned long long>::success(value);
}

template<>
[[nodiscard]] inline XmlResult<float> SafeXmlParser::parseValue<float>(
    const char* str, 
    const std::string& ctx) {
    if (!str || *str == '\0') {
        return XmlResult<float>::error(
            XmlErrorCode::TypeConversionError,
            "Empty string cannot be converted to float",
            ctx
        );
    }

    try {
        std::size_t pos{};
        float value = std::stof(str, &pos);
        
        // Check that we consumed all non-whitespace characters
        const char* remaining = str + pos;
        while (*remaining && std::isspace(static_cast<unsigned char>(*remaining))) {
            ++remaining;
        }
        
        if (*remaining != '\0') {
            return XmlResult<float>::error(
                XmlErrorCode::TypeConversionError,
                "Cannot convert '" + std::string(str) + "' to float",
                ctx
            );
        }
        
        return XmlResult<float>::success(value);
    } catch (const std::invalid_argument&) {
        return XmlResult<float>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(str) + "' to float (invalid)",
            ctx
        );
    } catch (const std::out_of_range&) {
        return XmlResult<float>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(str) + "' to float (out of range)",
            ctx
        );
    }
}

template<>
[[nodiscard]] inline XmlResult<double> SafeXmlParser::parseValue<double>(
    const char* str, 
    const std::string& ctx) {
    if (!str || *str == '\0') {
        return XmlResult<double>::error(
            XmlErrorCode::TypeConversionError,
            "Empty string cannot be converted to double",
            ctx
        );
    }

    try {
        std::size_t pos{};
        double value = std::stod(str, &pos);
        
        const char* remaining = str + pos;
        while (*remaining && std::isspace(static_cast<unsigned char>(*remaining))) {
            ++remaining;
        }
        
        if (*remaining != '\0') {
            return XmlResult<double>::error(
                XmlErrorCode::TypeConversionError,
                "Cannot convert '" + std::string(str) + "' to double",
                ctx
            );
        }
        
        return XmlResult<double>::success(value);
    } catch (const std::invalid_argument&) {
        return XmlResult<double>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(str) + "' to double (invalid)",
            ctx
        );
    } catch (const std::out_of_range&) {
        return XmlResult<double>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(str) + "' to double (out of range)",
            ctx
        );
    }
}

template<>
[[nodiscard]] inline XmlResult<long double> SafeXmlParser::parseValue<long double>(
    const char* str, 
    const std::string& ctx) {
    if (!str || *str == '\0') {
        return XmlResult<long double>::error(
            XmlErrorCode::TypeConversionError,
            "Empty string cannot be converted to long double",
            ctx
        );
    }

    try {
        std::size_t pos{};
        long double value = std::stold(str, &pos);
        
        const char* remaining = str + pos;
        while (*remaining && std::isspace(static_cast<unsigned char>(*remaining))) {
            ++remaining;
        }
        
        if (*remaining != '\0') {
            return XmlResult<long double>::error(
                XmlErrorCode::TypeConversionError,
                "Cannot convert '" + std::string(str) + "' to long double",
                ctx
            );
        }
        
        return XmlResult<long double>::success(value);
    } catch (const std::invalid_argument&) {
        return XmlResult<long double>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(str) + "' to long double (invalid)",
            ctx
        );
    } catch (const std::out_of_range&) {
        return XmlResult<long double>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(str) + "' to long double (out of range)",
            ctx
        );
    }
}

template<>
[[nodiscard]] inline XmlResult<bool> SafeXmlParser::parseValue<bool>(
    const char* str, 
    const std::string& ctx) {
    if (!str) {
        return XmlResult<bool>::error(
            XmlErrorCode::TypeConversionError,
            "Null string cannot be converted to bool",
            ctx
        );
    }

    // Skip leading whitespace
    while (*str && std::isspace(static_cast<unsigned char>(*str))) {
        ++str;
    }

    std::string s(str);
    
    // Trim trailing whitespace
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    
    // Convert to lowercase for comparison
    std::transform(s.begin(), s.end(), s.begin(), 
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (s == "true" || s == "1" || s == "yes" || s == "on") {
        return XmlResult<bool>::success(true);
    }
    if (s == "false" || s == "0" || s == "no" || s == "off") {
        return XmlResult<bool>::success(false);
    }

    return XmlResult<bool>::error(
        XmlErrorCode::TypeConversionError,
        "Cannot convert '" + std::string(str) + "' to bool",
        ctx
    );
}

template<>
[[nodiscard]] inline XmlResult<char> SafeXmlParser::parseValue<char>(
    const char* str, 
    const std::string& ctx) {
    if (!str || *str == '\0') {
        return XmlResult<char>::error(
            XmlErrorCode::TypeConversionError,
            "Empty string cannot be converted to char",
            ctx
        );
    }
    return XmlResult<char>::success(*str);
}

// ==================== parseSingleValue Specializations (for vector parsing) ====================

template<>
[[nodiscard]] inline XmlResult<int> SafeXmlParser::parseSingleValue<int>(
    std::string_view sv, 
    const std::string& ctx) {
    int value{};
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    
    if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
        return XmlResult<int>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(sv) + "' to int",
            ctx
        );
    }
    return XmlResult<int>::success(value);
}

template<>
[[nodiscard]] inline XmlResult<long> SafeXmlParser::parseSingleValue<long>(
    std::string_view sv, 
    const std::string& ctx) {
    long value{};
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    
    if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
        return XmlResult<long>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(sv) + "' to long",
            ctx
        );
    }
    return XmlResult<long>::success(value);
}

template<>
[[nodiscard]] inline XmlResult<long long> SafeXmlParser::parseSingleValue<long long>(
    std::string_view sv, 
    const std::string& ctx) {
    long long value{};
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    
    if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
        return XmlResult<long long>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(sv) + "' to long long",
            ctx
        );
    }
    return XmlResult<long long>::success(value);
}

template<>
[[nodiscard]] inline XmlResult<unsigned int> SafeXmlParser::parseSingleValue<unsigned int>(
    std::string_view sv, 
    const std::string& ctx) {
    unsigned int value{};
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    
    if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
        return XmlResult<unsigned int>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(sv) + "' to unsigned int",
            ctx
        );
    }
    return XmlResult<unsigned int>::success(value);
}

template<>
[[nodiscard]] inline XmlResult<unsigned long> SafeXmlParser::parseSingleValue<unsigned long>(
    std::string_view sv, 
    const std::string& ctx) {
    unsigned long value{};
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    
    if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
        return XmlResult<unsigned long>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(sv) + "' to unsigned long",
            ctx
        );
    }
    return XmlResult<unsigned long>::success(value);
}

template<>
[[nodiscard]] inline XmlResult<unsigned long long> SafeXmlParser::parseSingleValue<unsigned long long>(
    std::string_view sv, 
    const std::string& ctx) {
    unsigned long long value{};
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    
    if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
        return XmlResult<unsigned long long>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(sv) + "' to unsigned long long",
            ctx
        );
    }
    return XmlResult<unsigned long long>::success(value);
}

template<>
[[nodiscard]] inline XmlResult<float> SafeXmlParser::parseSingleValue<float>(
    std::string_view sv, 
    const std::string& ctx) {
    // std::from_chars for float is not guaranteed in all C++17 implementations
    // Use std::stof with a temporary string
    try {
        std::string temp(sv);
        std::size_t pos{};
        float value = std::stof(temp, &pos);
        
        if (pos != temp.size()) {
            return XmlResult<float>::error(
                XmlErrorCode::TypeConversionError,
                "Cannot convert '" + temp + "' to float",
                ctx
            );
        }
        
        return XmlResult<float>::success(value);
    } catch (const std::exception&) {
        return XmlResult<float>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(sv) + "' to float",
            ctx
        );
    }
}

template<>
[[nodiscard]] inline XmlResult<double> SafeXmlParser::parseSingleValue<double>(
    std::string_view sv, 
    const std::string& ctx) {
    try {
        std::string temp(sv);
        std::size_t pos{};
        double value = std::stod(temp, &pos);
        
        if (pos != temp.size()) {
            return XmlResult<double>::error(
                XmlErrorCode::TypeConversionError,
                "Cannot convert '" + temp + "' to double",
                ctx
            );
        }
        
        return XmlResult<double>::success(value);
    } catch (const std::exception&) {
        return XmlResult<double>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(sv) + "' to double",
            ctx
        );
    }
}

template<>
[[nodiscard]] inline XmlResult<long double> SafeXmlParser::parseSingleValue<long double>(
    std::string_view sv, 
    const std::string& ctx) {
    try {
        std::string temp(sv);
        std::size_t pos{};
        long double value = std::stold(temp, &pos);
        
        if (pos != temp.size()) {
            return XmlResult<long double>::error(
                XmlErrorCode::TypeConversionError,
                "Cannot convert '" + temp + "' to long double",
                ctx
            );
        }
        
        return XmlResult<long double>::success(value);
    } catch (const std::exception&) {
        return XmlResult<long double>::error(
            XmlErrorCode::TypeConversionError,
            "Cannot convert '" + std::string(sv) + "' to long double",
            ctx
        );
    }
}

template<>
[[nodiscard]] inline XmlResult<std::string> SafeXmlParser::parseSingleValue<std::string>(
    std::string_view sv, 
    [[maybe_unused]] const std::string& ctx) {
    return XmlResult<std::string>::success(std::string(sv));
}

template<>
[[nodiscard]] inline XmlResult<bool> SafeXmlParser::parseSingleValue<bool>(
    std::string_view sv, 
    const std::string& ctx) {
    std::string s(sv);
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (s == "true" || s == "1" || s == "yes" || s == "on") {
        return XmlResult<bool>::success(true);
    }
    if (s == "false" || s == "0" || s == "no" || s == "off") {
        return XmlResult<bool>::success(false);
    }

    return XmlResult<bool>::error(
        XmlErrorCode::TypeConversionError,
        "Cannot convert '" + std::string(sv) + "' to bool",
        ctx
    );
}

// ==================== Generic Vector Parser ====================

template<typename T>
struct VectorParser {
    [[nodiscard]] static XmlResult<std::vector<T>> parse(
        const char* str, 
        const std::string& ctx) {
        if (!str) {
            return XmlResult<std::vector<T>>::error(
                XmlErrorCode::TypeConversionError,
                "Null string cannot be converted to vector",
                ctx
            );
        }

        // Empty string = empty vector (valid)
        if (*str == '\0') {
            return XmlResult<std::vector<T>>::success(std::vector<T>{});
        }

        std::string_view sv(str);
        auto parts = SafeXmlParser::split(sv, SafeXmlParser::vectorDelimiter);
        
        std::vector<T> values;
        values.reserve(parts.size());
        
        XmlError aggregateError{
            XmlErrorCode::PartialFailure, 
            "Some vector elements failed to parse"
        };
        bool hasErrors = false;

        for (std::size_t i = 0; i < parts.size(); ++i) {
            auto result = SafeXmlParser::parseSingleValue<T>(
                parts[i], 
                ctx + "[" + std::to_string(i) + "]"
            );
            
            if (result.isSuccess()) {
                values.push_back(std::move(result).value());
            } else {
                hasErrors = true;
                aggregateError.addNestedError(std::move(result).error());
            }
        }

        if (values.empty() && hasErrors) {
            return XmlResult<std::vector<T>>::error(std::move(aggregateError));
        }

        return XmlResult<std::vector<T>>::success(std::move(values));
    }
    
    [[nodiscard]] static XmlResult<std::vector<T>> parseStrict(
        const char* str, 
        const std::string& ctx) {
        if (!str) {
            return XmlResult<std::vector<T>>::error(
                XmlErrorCode::TypeConversionError,
                "Null string cannot be converted to vector",
                ctx
            );
        }

        if (*str == '\0') {
            return XmlResult<std::vector<T>>::success(std::vector<T>{});
        }

        std::string_view sv(str);
        auto parts = SafeXmlParser::split(sv, SafeXmlParser::vectorDelimiter);
        
        std::vector<T> values;
        values.reserve(parts.size());

        for (std::size_t i = 0; i < parts.size(); ++i) {
            auto result = SafeXmlParser::parseSingleValue<T>(
                parts[i], 
                ctx + "[" + std::to_string(i) + "]"
            );
            
            if (result.isError()) {
                return XmlResult<std::vector<T>>::error(std::move(result).error());
            }
            values.push_back(std::move(result).value());
        }

        return XmlResult<std::vector<T>>::success(std::move(values));
    }
};

// ==================== Vector Type Specializations ====================

template<>
[[nodiscard]] inline XmlResult<std::vector<int>> 
SafeXmlParser::parseValue<std::vector<int>>(
    const char* str, 
    const std::string& ctx) {
    return VectorParser<int>::parse(str, ctx);
}

template<>
[[nodiscard]] inline XmlResult<std::vector<long>> 
SafeXmlParser::parseValue<std::vector<long>>(
    const char* str, 
    const std::string& ctx) {
    return VectorParser<long>::parse(str, ctx);
}

template<>
[[nodiscard]] inline XmlResult<std::vector<long long>> 
SafeXmlParser::parseValue<std::vector<long long>>(
    const char* str, 
    const std::string& ctx) {
    return VectorParser<long long>::parse(str, ctx);
}

template<>
[[nodiscard]] inline XmlResult<std::vector<unsigned int>> 
SafeXmlParser::parseValue<std::vector<unsigned int>>(
    const char* str, 
    const std::string& ctx) {
    return VectorParser<unsigned int>::parse(str, ctx);
}

template<>
[[nodiscard]] inline XmlResult<std::vector<unsigned long>> 
SafeXmlParser::parseValue<std::vector<unsigned long>>(
    const char* str, 
    const std::string& ctx) {
    return VectorParser<unsigned long>::parse(str, ctx);
}

template<>
[[nodiscard]] inline XmlResult<std::vector<unsigned long long>> 
SafeXmlParser::parseValue<std::vector<unsigned long long>>(
    const char* str, 
    const std::string& ctx) {
    return VectorParser<unsigned long long>::parse(str, ctx);
}

template<>
[[nodiscard]] inline XmlResult<std::vector<float>> 
SafeXmlParser::parseValue<std::vector<float>>(
    const char* str, 
    const std::string& ctx) {
    return VectorParser<float>::parse(str, ctx);
}

template<>
[[nodiscard]] inline XmlResult<std::vector<double>> 
SafeXmlParser::parseValue<std::vector<double>>(
    const char* str, 
    const std::string& ctx) {
    return VectorParser<double>::parse(str, ctx);
}

template<>
[[nodiscard]] inline XmlResult<std::vector<long double>> 
SafeXmlParser::parseValue<std::vector<long double>>(
    const char* str, 
    const std::string& ctx) {
    return VectorParser<long double>::parse(str, ctx);
}

template<>
[[nodiscard]] inline XmlResult<std::vector<std::string>> 
SafeXmlParser::parseValue<std::vector<std::string>>(
    const char* str, 
    const std::string& ctx) {
    return VectorParser<std::string>::parse(str, ctx);
}

template<>
[[nodiscard]] inline XmlResult<std::vector<bool>> 
SafeXmlParser::parseValue<std::vector<bool>>(
    const char* str, 
    const std::string& ctx) {
    return VectorParser<bool>::parse(str, ctx);
}

// ==================== Convenience Functions ====================

template<typename T>
[[nodiscard]] inline XmlResult<std::vector<T>> parseVectorWithDelimiter(
    const char* str, 
    char delimiter,
    const std::string& ctx = "") {
    
    // Save and restore delimiter (RAII-style would be better for thread safety)
    const char oldDelimiter = SafeXmlParser::getVectorDelimiter();
    SafeXmlParser::setVectorDelimiter(delimiter);
    
    auto result = SafeXmlParser::parseValue<std::vector<T>>(str, ctx);
    
    SafeXmlParser::setVectorDelimiter(oldDelimiter);
    return result;
}

template<typename T>
[[nodiscard]] inline XmlResult<std::vector<T>> parseVectorStrict(
    const char* str,
    const std::string& ctx = "") {
    return VectorParser<T>::parseStrict(str, ctx);
}

// Thread-safe version with explicit delimiter
template<typename T>
[[nodiscard]] inline XmlResult<std::vector<T>> parseVector(
    const char* str,
    char delimiter,
    const std::string& ctx = "") {
    
    if (!str) {
        return XmlResult<std::vector<T>>::error(
            XmlErrorCode::TypeConversionError,
            "Null string cannot be converted to vector",
            ctx
        );
    }

    if (*str == '\0') {
        return XmlResult<std::vector<T>>::success(std::vector<T>{});
    }

    std::string_view sv(str);
    auto parts = SafeXmlParser::split(sv, delimiter);
    
    std::vector<T> values;
    values.reserve(parts.size());

    for (std::size_t i = 0; i < parts.size(); ++i) {
        auto result = SafeXmlParser::parseSingleValue<T>(
            parts[i], 
            ctx + "[" + std::to_string(i) + "]"
        );
        
        if (result.isError()) {
            return XmlResult<std::vector<T>>::error(std::move(result).error());
        }
        values.push_back(std::move(result).value());
    }

    return XmlResult<std::vector<T>>::success(std::move(values));
}

} // namespace xml_framework

#endif // SAFE_XML_PARSER_HPP
