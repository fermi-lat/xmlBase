#ifndef RAPIDXML_ERROR_FRAMEWORK_HPP
#define RAPIDXML_ERROR_FRAMEWORK_HPP

#include <stdexcept>
#include <string>
#include <sstream>
#include "rapidxml.hpp"

namespace xml_framework {

// Base exception for all XML-related errors
class XmlException : public std::runtime_error {
public:
    explicit XmlException(const std::string& message, 
                          const std::string& context = "")
        : std::runtime_error(buildMessage(message, context)),
          message_(message),
          context_(context) {}

    const std::string& getContext() const { return context_; }
    const std::string& getBaseMessage() const { return message_; }

private:
    static std::string buildMessage(const std::string& msg, 
                                    const std::string& ctx) {
        if (ctx.empty()) return msg;
        return msg + " [Context: " + ctx + "]";
    }

    std::string message_;
    std::string context_;
};

// Parsing-specific errors
class XmlParseException : public XmlException {
public:
    XmlParseException(const std::string& message,
                      size_t line = 0,
                      size_t column = 0,
                      const std::string& context = "")
        : XmlException(buildParseMessage(message, line, column), context),
          line_(line),
          column_(column) {}

    size_t getLine() const { return line_; }
    size_t getColumn() const { return column_; }

private:
    static std::string buildParseMessage(const std::string& msg,
                                         size_t line, size_t col) {
        std::ostringstream oss;
        oss << "XML Parse Error: " << msg;
        if (line > 0) oss << " (Line: " << line << ", Column: " << col << ")";
        return oss.str();
    }

    size_t line_;
    size_t column_;
};

// Node/Element not found errors
class XmlNodeNotFoundException : public XmlException {
public:
    explicit XmlNodeNotFoundException(const std::string& nodeName,
                                      const std::string& parentPath = "")
        : XmlException("Node not found: '" + nodeName + "'", parentPath),
          nodeName_(nodeName) {}

    const std::string& getNodeName() const { return nodeName_; }

private:
    std::string nodeName_;
};

// Attribute not found errors
class XmlAttributeNotFoundException : public XmlException {
public:
    XmlAttributeNotFoundException(const std::string& attrName,
                                  const std::string& nodeName)
        : XmlException("Attribute '" + attrName + "' not found in node '" + nodeName + "'"),
          attrName_(attrName),
          nodeName_(nodeName) {}

    const std::string& getAttributeName() const { return attrName_; }
    const std::string& getNodeName() const { return nodeName_; }

private:
    std::string attrName_;
    std::string nodeName_;
};

// Type conversion errors
class XmlTypeConversionException : public XmlException {
public:
    XmlTypeConversionException(const std::string& value,
                               const std::string& targetType,
                               const std::string& context = "")
        : XmlException("Cannot convert '" + value + "' to " + targetType, context),
          value_(value),
          targetType_(targetType) {}

    const std::string& getValue() const { return value_; }
    const std::string& getTargetType() const { return targetType_; }

private:
    std::string value_;
    std::string targetType_;
};

// Validation errors
class XmlValidationException : public XmlException {
public:
    explicit XmlValidationException(const std::string& message,
                                    const std::string& context = "")
        : XmlException("Validation Error: " + message, context) {}
};

} // namespace xml_framework

#endif // RAPIDXML_ERROR_FRAMEWORK_HPP
