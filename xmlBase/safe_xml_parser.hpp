#ifndef SAFE_XML_PARSER_HPP
#define SAFE_XML_PARSER_HPP

#include "rapidxml.hpp"
#include "rapidxml_error_framework.hpp"
#include "xml_result.hpp"

#include <fstream>
#include <sstream>
#include <vector>
#include <functional>
#include <charconv>
#include <cstring>    // Added: Required for std::strlen
#include <optional>   // Added: Required for std::optional
#include <cctype>     // Added: Required for std::tolower
#include <string>

namespace xml_framework {

class SafeXmlParser {
public:
    using XmlDocument = rapidxml::xml_document<>;
    using XmlNode = rapidxml::xml_node<>;
    using XmlAttribute = rapidxml::xml_attribute<>;

    // Calculate line and column from position in source
    static std::pair<size_t, size_t> calculatePosition(const char* source, 
                                                        const char* position) {
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

    // Parse XML from string (throws on error)
    static void parseString(XmlDocument& doc, char* xmlString, 
                           int flags = 0) {
        try {
            doc.parse<0>(xmlString);
        } catch (const rapidxml::parse_error& e) {
            auto [line, col] = calculatePosition(xmlString, e.where<char>());
            throw XmlParseException(e.what(), line, col);
        }
    }

    // Parse XML from string (returns Result)
    static XmlResult<void> tryParseString(XmlDocument& doc, char* xmlString,
                                          int flags = 0) {
        try {
            doc.parse<0>(xmlString);
            return XmlResult<void>::success();
        } catch (const rapidxml::parse_error& e) {
            auto [line, col] = calculatePosition(xmlString, e.where<char>());
            return XmlResult<void>::error(
                XmlError{XmlErrorCode::ParseError, e.what(), "", line, col}
            );
        }
    }

    // Load and parse XML from file
    static XmlResult<std::vector<char>> loadFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return XmlResult<std::vector<char>>::error(
                XmlErrorCode::FileNotFound, 
                "Cannot open file: " + filename
            );
        }

        auto size = file.tellg();
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

    // Safe node access (throws on not found)
    static XmlNode* getRequiredNode(XmlNode* parent, const char* name,
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

    // Safe node access (returns Result)
    static XmlResult<XmlNode*> tryGetNode(XmlNode* parent, const char* name) {
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

    // Optional node access
    static std::optional<XmlNode*> getOptionalNode(XmlNode* parent, 
                                                    const char* name) {
        if (!parent) return std::nullopt;
        XmlNode* node = parent->first_node(name);
        return node ? std::optional{node} : std::nullopt;
    }

    // Safe attribute access (throws)
    static XmlAttribute* getRequiredAttribute(const XmlNode* node, const char* name) {
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

    // Safe attribute access (returns Result)
    static XmlResult<XmlAttribute*> tryGetAttribute(const XmlNode* node, 
                                                     const char* name) {
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

    // Type-safe value extraction
    template<typename T>
    static XmlResult<T> parseValue(const char* str, const std::string& ctx = "");

    // Get node value with type conversion
    template<typename T>
    static XmlResult<T> getNodeValue(XmlNode* node) {
        if (!node || !node->value()) {
            return XmlResult<T>::error(
                XmlErrorCode::NodeNotFound,
                "Node or value is null"
            );
        }
        return parseValue<T>(node->value(), node->name() ? node->name() : "");
    }

    // Get attribute value with type conversion
    template<typename T>
    static XmlResult<T> getAttributeValue(const XmlNode* node, const char* attrName) {
        auto attrResult = tryGetAttribute(node, attrName);
        if (attrResult.isError()) {
            return XmlResult<T>::error(attrResult.error());
        }
        return parseValue<T>(attrResult.value()->value(), attrName);
    }

    // Iterate children with error handling
    static void forEachChild(XmlNode* parent, const char* childName,
                            std::function<void(XmlNode*)> callback,
                            bool throwOnEmpty = false) {
        if (!parent) {
            if (throwOnEmpty) {
                throw XmlNodeNotFoundException(childName, "null parent");
            }
            return;
        }

        bool found = false;
        for (XmlNode* child = parent->first_node(childName);
             child;
             child = child->next_sibling(childName)) {
            found = true;
            callback(child);
        }

        if (!found && throwOnEmpty) {
            throw XmlNodeNotFoundException(childName, 
                parent->name() ? parent->name() : "unknown parent");
        }
    }

    // Collect all children into a vector
    static std::vector<XmlNode*> getChildren(XmlNode* parent, 
                                              const char* childName = nullptr) {
        std::vector<XmlNode*> children;
        if (!parent) return children;

        for (XmlNode* child = parent->first_node(childName);
             child;
             child = child->next_sibling(childName)) {
            children.push_back(child);
        }
        return children;
    }
};

// Template specializations for common types
template<>
inline XmlResult<std::string> SafeXmlParser::parseValue<std::string>(
    const char* str, const std::string& /*ctx*/) {
    if (!str) {
        return XmlResult<std::string>::error(
            XmlErrorCode::TypeConversionError,
            "Null string"
        );
    }
    return XmlResult<std::string>::success(std::string(str));
}

template<>
inline XmlResult<int> SafeXmlParser::parseValue<int>(
    const char* str, const std::string& ctx) {
    if (!str || *str == '\0') {
        return XmlResult<int>::error(
            XmlErrorCode::TypeConversionError,
            "Empty string cannot be converted to int"
        );
    }

    int value;
    auto [ptr, ec] = std::from_chars(str, str + std::strlen(str), value);
    if (ec != std::errc()) {
        return XmlResult<int>::error(
            XmlError{XmlErrorCode::TypeConversionError,
                    "Cannot convert '" + std::string(str) + "' to int", ctx}
        );
    }
    return XmlResult<int>::success(value);
}

template<>
inline XmlResult<double> SafeXmlParser::parseValue<double>(
    const char* str, const std::string& ctx) {
    if (!str || *str == '\0') {
        return XmlResult<double>::error(
            XmlErrorCode::TypeConversionError,
            "Empty string cannot be converted to double"
        );
    }

    try {
        size_t pos;
        double value = std::stod(str, &pos);
        return XmlResult<double>::success(value);
    } catch (const std::exception&) {
        return XmlResult<double>::error(
            XmlError{XmlErrorCode::TypeConversionError,
                    "Cannot convert '" + std::string(str) + "' to double", ctx}
        );
    }
}

template<>
inline XmlResult<bool> SafeXmlParser::parseValue<bool>(
    const char* str, const std::string& ctx) {
    if (!str) {
        return XmlResult<bool>::error(
            XmlErrorCode::TypeConversionError,
            "Null string cannot be converted to bool"
        );
    }

    std::string s(str);
    // Convert to lowercase for comparison
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (s == "true" || s == "1" || s == "yes") {
        return XmlResult<bool>::success(true);
    }
    if (s == "false" || s == "0" || s == "no") {
        return XmlResult<bool>::success(false);
    }

    return XmlResult<bool>::error(
        XmlError{XmlErrorCode::TypeConversionError,
                "Cannot convert '" + std::string(str) + "' to bool", ctx}
    );
}

} // namespace xml_framework

#endif // SAFE_XML_PARSER_HPP
