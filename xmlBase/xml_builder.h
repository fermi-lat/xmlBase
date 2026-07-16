#ifndef XML_BUILDER_H
#define XML_BUILDER_H

#include "rapidxml.hpp"
#include "rapidxml_error_framework.hpp"
#include "xml_result.hpp"
#include "safe_xml_parser.hpp"
#include "xml_printer.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <functional>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace xml_framework {

// ==================== Forward Declarations ====================
class XmlElementBuilder;
class XmlDomBuilder;

// ==================== Value Converter Utility ====================

class ValueConverter {
public:
    template<typename T>
    static std::string toString(const T& value) {
        if constexpr (std::is_same_v<T, bool>) {
            return value ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::string>) {
            return value;
        } else if constexpr (std::is_same_v<T, std::string_view>) {
            return std::string(value);
        } else if constexpr (std::is_same_v<T, const char*>) {
            return value ? std::string(value) : "";
        } else if constexpr (std::is_arithmetic_v<T>) {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        } else {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }
    }
    
    template<typename T>
    static std::string join(const std::vector<T>& values, 
                            std::string_view delimiter = ",") {
        if (values.empty()) {
            return {};
        }
        
        std::ostringstream oss;
        bool first = true;
        
        for (const auto& value : values) {
            if (!first) {
                oss << delimiter;
            }
            first = false;
            oss << toString(value);
        }
        
        return oss.str();
    }
};

// ==================== XmlElementBuilder ====================

class XmlElementBuilder {
public:
    using XmlDocument = rapidxml::xml_document<>;
    using XmlNode = rapidxml::xml_node<>;
    using XmlAttribute = rapidxml::xml_attribute<>;

    XmlElementBuilder(XmlDocument& doc, XmlNode* node)
        : doc_(doc), node_(node) {}

    // ==================== Attribute Methods ====================
    
    /**
     * @brief Add a string_view attribute
     * @param name Attribute name
     * @param value Attribute value
     * @return Reference to this builder for chaining
     */
    XmlElementBuilder& addAttribute(std::string_view name, std::string_view value) {
        char* allocName = doc_.allocate_string(name.data(), name.size() + 1);
        char* allocValue = doc_.allocate_string(value.data(), value.size() + 1);
        
        auto* attr = doc_.allocate_attribute(allocName, allocValue);
        node_->append_attribute(attr);
        
        return *this;
    }
    
    /**
     * @brief Add an attribute with any convertible type
     * @tparam T Value type
     * @param name Attribute name
     * @param value Attribute value
     * @return Reference to this builder for chaining
     */
    template<typename T>
    XmlElementBuilder& addAttribute(std::string_view name, const T& value) {
        std::string strValue = ValueConverter::toString(value);
        return addAttribute(name, std::string_view(strValue));
    }
    
    /**
     * @brief Add attribute only if optional has a value
     * @tparam T Optional's value type
     * @param name Attribute name
     * @param value Optional value
     * @return Reference to this builder for chaining
     */
    template<typename T>
    XmlElementBuilder& addAttributeIf(std::string_view name, 
                                       const std::optional<T>& value) {
        if (value.has_value()) {
            addAttribute(name, *value);
        }
        return *this;
    }
    
    /**
     * @brief Add attribute only if condition is true
     * @tparam T Value type
     * @param condition Boolean condition
     * @param name Attribute name
     * @param value Attribute value
     * @return Reference to this builder for chaining
     */
    template<typename T>
    XmlElementBuilder& addAttributeIf(bool condition,
                                       std::string_view name, 
                                       const T& value) {
        if (condition) {
            addAttribute(name, value);
        }
        return *this;
    }
    
    /**
     * @brief Set (replace) an attribute value, removes existing if present
     * @tparam T Value type
     * @param name Attribute name
     * @param value New attribute value
     * @return Reference to this builder for chaining
     */
    template<typename T>
    XmlElementBuilder& setAttribute(std::string_view name, const T& value) {
        removeAttribute(name);
        return addAttribute(name, value);
    }
    
    /**
     * @brief Remove an attribute by name
     * @param name Attribute name to remove
     * @return Reference to this builder for chaining
     */
    XmlElementBuilder& removeAttribute(std::string_view name) {
        std::string nameStr(name);
        auto* attr = node_->first_attribute(nameStr.c_str());
        
        if (attr) {
            node_->remove_attribute(attr);
        }
        
        return *this;
    }
    
    /**
     * @brief Check if attribute exists
     * @param name Attribute name
     * @return true if attribute exists
     */
    [[nodiscard]] bool hasAttribute(std::string_view name) const {
        std::string nameStr(name);
        return node_->first_attribute(nameStr.c_str()) != nullptr;
    }
    
    /**
     * @brief Get attribute value using SafeXmlParser
     * @tparam T Target type
     * @param name Attribute name
     * @return XmlResult containing the value or error
     */
    template<typename T>
    [[nodiscard]] XmlResult<T> getAttributeValue(const char* name) const {
        return SafeXmlParser::getAttributeValue<T>(node_, name);
    }
    
    /**
     * @brief Add multiple attributes from a map
     * @param attributes Map of name-value pairs
     * @return Reference to this builder for chaining
     */
    XmlElementBuilder& addAttributes(
        const std::map<std::string, std::string>& attributes) {
        for (const auto& [name, value] : attributes) {
            addAttribute(name, value);
        }
        return *this;
    }
    
    /**
     * @brief Add multiple attributes from initializer list
     * @param attributes List of name-value pairs
     * @return Reference to this builder for chaining
     */
    XmlElementBuilder& addAttributes(
        std::initializer_list<std::pair<std::string_view, std::string_view>> attributes) {
        for (const auto& [name, value] : attributes) {
            addAttribute(name, value);
        }
        return *this;
    }

    // ==================== Child Element Methods ====================
    
    /**
     * @brief Add a child element
     * @param name Element name
     * @return Builder for the new child element
     */
    XmlElementBuilder addElement(std::string_view name) {
        char* allocName = doc_.allocate_string(name.data(), name.size() + 1);
        auto* child = doc_.allocate_node(rapidxml::node_element, allocName);
        node_->append_node(child);
        
        return XmlElementBuilder(doc_, child);
    }
    
    /**
     * @brief Add a child element with text content
     * @tparam T Value type
     * @param name Element name
     * @param value Element text content
     * @return Builder for the new child element
     */
    template<typename T>
    XmlElementBuilder addElement(std::string_view name, const T& value) {
        std::string strValue = ValueConverter::toString(value);
        
        char* allocName = doc_.allocate_string(name.data(), name.size() + 1);
        char* allocValue = doc_.allocate_string(strValue.c_str(), strValue.size() + 1);
        
        auto* child = doc_.allocate_node(rapidxml::node_element, allocName, allocValue);
        node_->append_node(child);
        
        return XmlElementBuilder(doc_, child);
    }
    
    /**
     * @brief Add multiple child elements with same name from vector
     * @tparam T Value type
     * @param name Element name for each
     * @param values Vector of values
     * @return Reference to this builder for chaining
     */
    template<typename T>
    XmlElementBuilder& addElements(std::string_view name, 
                                   const std::vector<T>& values) {
        for (const auto& value : values) {
            addElement(name, value);
        }
        return *this;
    }
    
    /**
     * @brief Add multiple child elements with custom builder for each
     * @tparam T Vector element type
     * @param name Element name for each
     * @param items Vector of items
     * @param builder Callback to build each element
     * @return Reference to this builder for chaining
     */
    template<typename T>
    XmlElementBuilder& addElements(
        std::string_view name,
        const std::vector<T>& items,
        const std::function<void(XmlElementBuilder&, const T&)>& builder) {
        
        for (const auto& item : items) {
            auto child = addElement(name);
            builder(child, item);
        }
        return *this;
    }
    
    /**
     * @brief Prepend a child element (add as first child)
     * @param name Element name
     * @return Builder for the new child element
     */
    XmlElementBuilder prependElement(std::string_view name) {
        char* allocName = doc_.allocate_string(name.data(), name.size() + 1);
        auto* child = doc_.allocate_node(rapidxml::node_element, allocName);
        node_->prepend_node(child);
        
        return XmlElementBuilder(doc_, child);
    }
    
    /**
     * @brief Prepend a child element with text content
     * @tparam T Value type
     * @param name Element name
     * @param value Element text content
     * @return Builder for the new child element
     */
    template<typename T>
    XmlElementBuilder prependElement(std::string_view name, const T& value) {
        std::string strValue = ValueConverter::toString(value);
        
        char* allocName = doc_.allocate_string(name.data(), name.size() + 1);
        char* allocValue = doc_.allocate_string(strValue.c_str(), strValue.size() + 1);
        
        auto* child = doc_.allocate_node(rapidxml::node_element, allocName, allocValue);
        node_->prepend_node(child);
        
        return XmlElementBuilder(doc_, child);
    }
    
    /**
     * @brief Insert element before a reference element
     * @param name Element name
     * @param before Node to insert before
     * @return Builder for the new child element
     */
    XmlElementBuilder insertElementBefore(std::string_view name, XmlNode* before) {
        char* allocName = doc_.allocate_string(name.data(), name.size() + 1);
        auto* child = doc_.allocate_node(rapidxml::node_element, allocName);
        node_->insert_node(before, child);
        
        return XmlElementBuilder(doc_, child);
    }
    
    // ==================== Text Content Methods ====================
    
    /**
     * @brief Set text content of this element (replaces existing text)
     * @tparam T Value type
     * @param value Text content
     * @return Reference to this builder for chaining
     */
    template<typename T>
    XmlElementBuilder& setText(const T& value) {
        // Remove existing text nodes
        auto* child = node_->first_node();
        while (child) {
            auto* next = child->next_sibling();
            if (child->type() == rapidxml::node_data) {
                node_->remove_node(child);
            }
            child = next;
        }
        
        std::string strValue = ValueConverter::toString(value);
        char* allocValue = doc_.allocate_string(strValue.c_str(), strValue.size() + 1);
        auto* textNode = doc_.allocate_node(rapidxml::node_data, nullptr, allocValue);
        node_->prepend_node(textNode);
        
        return *this;
    }
    
    /**
     * @brief Add CDATA section
     * @param data CDATA content
     * @return Reference to this builder for chaining
     */
    XmlElementBuilder& addCData(std::string_view data) {
        char* allocValue = doc_.allocate_string(data.data(), data.size() + 1);
        auto* cdataNode = doc_.allocate_node(rapidxml::node_cdata, nullptr, allocValue);
        node_->append_node(cdataNode);
        
        return *this;
    }
    
    /**
     * @brief Get element text value using SafeXmlParser
     * @tparam T Target type
     * @return XmlResult containing the value or error
     */
    template<typename T>
    [[nodiscard]] XmlResult<T> getValue() const {
        return SafeXmlParser::getNodeValue<T>(node_);
    }
    
    // ==================== Comment Methods ====================
    
    /**
     * @brief Add a comment as child
     * @param comment Comment text
     * @return Reference to this builder for chaining
     */
    XmlElementBuilder& addComment(std::string_view comment) {
        char* allocValue = doc_.allocate_string(comment.data(), comment.size() + 1);
        auto* commentNode = doc_.allocate_node(rapidxml::node_comment, nullptr, allocValue);
        node_->append_node(commentNode);
        
        return *this;
    }
    
    // ==================== Navigation Methods ====================
    
    /**
     * @brief Get parent element builder
     * @return Optional builder for parent, empty if no parent element
     */
    [[nodiscard]] std::optional<XmlElementBuilder> parent() {
        auto* parentNode = node_->parent();
        if (parentNode && parentNode->type() == rapidxml::node_element) {
            return XmlElementBuilder(doc_, parentNode);
        }
        return std::nullopt;
    }
    
    /**
     * @brief Navigate to parent and continue building
     * @return Reference to parent builder
     * @throws XmlException if no parent element exists
     */
    XmlElementBuilder& up() {
        auto* parentNode = node_->parent();
        if (!parentNode || parentNode->type() != rapidxml::node_element) {
            throw XmlException("No parent element", "XmlElementBuilder::up");
        }
        node_ = parentNode;
        return *this;
    }
    
    /**
     * @brief Get first child element with given name
     * @param name Child element name
     * @return Optional builder for child, empty if not found
     */
    [[nodiscard]] std::optional<XmlElementBuilder> child(std::string_view name) {
        std::string nameStr(name);
        auto* childNode = node_->first_node(nameStr.c_str());
        if (childNode && childNode->type() == rapidxml::node_element) {
            return XmlElementBuilder(doc_, childNode);
        }
        return std::nullopt;
    }
    
    /**
     * @brief Get the underlying node
     * @return Pointer to the XML node
     */
    [[nodiscard]] XmlNode* node() const noexcept { return node_; }
    
    /**
     * @brief Get reference to the document
     * @return Reference to the XML document
     */
    [[nodiscard]] XmlDocument& document() noexcept { return doc_; }

private:
    XmlDocument& doc_;
    XmlNode* node_;
};

// ==================== XmlDomBuilder ====================

class XmlDomBuilder {
public:
    using XmlDocument = rapidxml::xml_document<>;
    using XmlNode = rapidxml::xml_node<>;

    XmlDomBuilder() = default;
    
    // Non-copyable
    XmlDomBuilder(const XmlDomBuilder&) = delete;
    XmlDomBuilder& operator=(const XmlDomBuilder&) = delete;
    
    // Movable
    XmlDomBuilder(XmlDomBuilder&& other) noexcept 
        : doc_(), sourceBuffer_(std::move(other.sourceBuffer_)) {
        // Note: rapidxml::xml_document doesn't have a proper move constructor,
        // so we need to re-parse if there's content
        if (!sourceBuffer_.empty()) {
            try {
                doc_.parse<rapidxml::parse_default>(sourceBuffer_.data());
            } catch (...) {
                sourceBuffer_.clear();
            }
        }
    }
    
    XmlDomBuilder& operator=(XmlDomBuilder&& other) noexcept {
        if (this != &other) {
            doc_.clear();
            sourceBuffer_ = std::move(other.sourceBuffer_);
            if (!sourceBuffer_.empty()) {
                try {
                    doc_.parse<rapidxml::parse_default>(sourceBuffer_.data());
                } catch (...) {
                    sourceBuffer_.clear();
                }
            }
        }
        return *this;
    }
    
    // ==================== Document Creation ====================
    
    /**
     * @brief Create a new empty document
     * @return Reference to this builder for chaining
     */
    XmlDomBuilder& createDocument() {
        doc_.clear();
        sourceBuffer_.clear();
        return *this;
    }
    
    /**
     * @brief Add XML declaration (<?xml version="1.0" encoding="UTF-8"?>)
     * @param version XML version (default "1.0")
     * @param encoding Character encoding (default "UTF-8")
     * @param standalone Standalone attribute (default empty/omitted)
     * @return Reference to this builder for chaining
     */
    XmlDomBuilder& addDeclaration(
        std::string_view version = "1.0",
        std::string_view encoding = "UTF-8",
        std::string_view standalone = "") {
        
        auto* decl = doc_.allocate_node(rapidxml::node_declaration);
        
        char* allocVersion = doc_.allocate_string(version.data(), version.size() + 1);
        decl->append_attribute(doc_.allocate_attribute("version", allocVersion));
        
        if (!encoding.empty()) {
            char* allocEncoding = doc_.allocate_string(encoding.data(), encoding.size() + 1);
            decl->append_attribute(doc_.allocate_attribute("encoding", allocEncoding));
        }
        
        if (!standalone.empty()) {
            char* allocStandalone = doc_.allocate_string(standalone.data(), standalone.size() + 1);
            decl->append_attribute(doc_.allocate_attribute("standalone", allocStandalone));
        }
        
        doc_.prepend_node(decl);
        return *this;
    }
    
    /**
     * @brief Add a comment at document level
     * @param comment Comment text
     * @return Reference to this builder for chaining
     */
    XmlDomBuilder& addComment(std::string_view comment) {
        char* allocValue = doc_.allocate_string(comment.data(), comment.size() + 1);
        auto* commentNode = doc_.allocate_node(rapidxml::node_comment, nullptr, allocValue);
        doc_.append_node(commentNode);
        return *this;
    }
    
    /**
     * @brief Create and return root element builder
     * @param name Root element name
     * @return Builder for the root element
     */
    XmlElementBuilder createRoot(std::string_view name) {
        char* allocName = doc_.allocate_string(name.data(), name.size() + 1);
        auto* root = doc_.allocate_node(rapidxml::node_element, allocName);
        doc_.append_node(root);
        
        return XmlElementBuilder(doc_, root);
    }
    
    /**
     * @brief Create root element with initial attributes
     * @param name Root element name
     * @param attributes Initializer list of attribute name-value pairs
     * @return Builder for the root element
     */
    XmlElementBuilder createRoot(
        std::string_view name,
        std::initializer_list<std::pair<std::string_view, std::string_view>> attributes) {
        
        auto builder = createRoot(name);
        builder.addAttributes(attributes);
        return builder;
    }
    
    /**
     * @brief Get existing root element
     * @return Optional builder for root, empty if no root
     */
    [[nodiscard]] std::optional<XmlElementBuilder> root() {
        auto rootOpt = getRootNode();
        if (rootOpt) {
            return XmlElementBuilder(doc_, *rootOpt);
        }
        return std::nullopt;
    }
    
    /**
     * @brief Check if document has a root element
     * @return true if root element exists
     */
    [[nodiscard]] bool hasRoot() const noexcept {
        return getRootNodeConst() != nullptr;
    }
    
    // ==================== File Operations ====================
    
    /**
     * @brief Load and parse XML from file
     * @param filePath Path to XML file
     * @throws XmlException on file or parse error
     */
    void loadFile(const std::filesystem::path& filePath) {
        auto result = tryLoadFile(filePath);
        if (result.isError()) {
            throw XmlException(result.error().message, result.error().context);
        }
    }
    
    /**
     * @brief Try to load XML from file (returns XmlResult)
     * @param filePath Path to XML file
     * @return XmlResult indicating success or error
     */
    [[nodiscard]] XmlResult<void> tryLoadFile(const std::filesystem::path& filePath) {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return XmlResult<void>::error(
                XmlErrorCode::FileNotFound,
                "Cannot open file",
                filePath.string()
            );
        }
        
        auto size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        sourceBuffer_.resize(static_cast<size_t>(size) + 1);
        if (!file.read(sourceBuffer_.data(), size)) {
            return XmlResult<void>::error(
                XmlErrorCode::ParseError,
                "Failed to read file",
                filePath.string()
            );
        }
        sourceBuffer_[static_cast<size_t>(size)] = '\0';
        
        return SafeXmlParser::tryParseString(doc_, sourceBuffer_.data());
    }
    
    /**
     * @brief Parse XML from string
     * @param xml XML content
     * @throws XmlParseException on parse error
     */
    void parseString(std::string_view xml) {
        sourceBuffer_.assign(xml.begin(), xml.end());
        sourceBuffer_.push_back('\0');
        SafeXmlParser::parseString(doc_, sourceBuffer_.data());
    }
    
    /**
     * @brief Try to parse XML from string (returns XmlResult)
     * @param xml XML content
     * @return XmlResult indicating success or error
     */
    [[nodiscard]] XmlResult<void> tryParseString(std::string_view xml) {
        sourceBuffer_.assign(xml.begin(), xml.end());
        sourceBuffer_.push_back('\0');
        return SafeXmlParser::tryParseString(doc_, sourceBuffer_.data());
    }
    
    /**
     * @brief Save document to file
     * @param filePath Path to output file
     * @throws XmlException on write error
     */
    void saveToFile(const std::filesystem::path& filePath) const {
        auto result = trySaveToFile(filePath);
        if (result.isError()) {
            throw XmlException(result.error().message, result.error().context);
        }
    }
    
    /**
     * @brief Try to save document to file (returns XmlResult)
     * @param filePath Path to output file
     * @return XmlResult indicating success or error
     */
    [[nodiscard]] XmlResult<void> trySaveToFile(
        const std::filesystem::path& filePath) const {
        
        try {
            if (filePath.has_parent_path()) {
                std::filesystem::create_directories(filePath.parent_path());
            }
        } catch (const std::filesystem::filesystem_error& e) {
            return XmlResult<void>::error(
                XmlErrorCode::FileNotFound,
                std::string("Failed to create directory: ") + e.what(),
                filePath.string()
            );
        }
        
        std::ofstream file(filePath, std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            return XmlResult<void>::error(
                XmlErrorCode::FileNotFound,
                "Failed to create file",
                filePath.string()
            );
        }
        
        // Use XmlPrinter
        auto result = XmlPrinter::tryPrintElement(&doc_, file);
        if (result.isError()) {
            return result;
        }
        
        if (file.fail()) {
            return XmlResult<void>::error(
                XmlErrorCode::ParseError,
                "Failed to write file",
                filePath.string()
            );
        }
        
        file.close();
        return XmlResult<void>::success();
    }
    
    /**
     * @brief Convert document to string (formatted)
     * @return XML string with indentation
     */
    [[nodiscard]] std::string toString() const {
        return XmlPrinter::toString(&doc_, true);
    }
    
    /**
     * @brief Convert document to compact string (no formatting)
     * @return XML string without extra whitespace
     */
    [[nodiscard]] std::string toCompactString() const {
        return XmlPrinter::toString(&doc_, false);
    }
    
    /**
     * @brief Get reference to the underlying document
     * @return Reference to the XML document
     */
    [[nodiscard]] XmlDocument& document() noexcept { return doc_; }
    [[nodiscard]] const XmlDocument& document() const noexcept { return doc_; }

private:
    [[nodiscard]] std::optional<XmlNode*> getRootNode() noexcept {
        auto* root = doc_.first_node();
        
        while (root && (root->type() == rapidxml::node_declaration ||
                        root->type() == rapidxml::node_doctype ||
                        root->type() == rapidxml::node_comment)) {
            root = root->next_sibling();
        }
        
        if (root && root->type() == rapidxml::node_element) {
            return root;
        }
        return std::nullopt;
    }
    
    [[nodiscard]] XmlNode* getRootNodeConst() const noexcept {
        auto* root = doc_.first_node();
        
        while (root && (root->type() == rapidxml::node_declaration ||
                        root->type() == rapidxml::node_doctype ||
                        root->type() == rapidxml::node_comment)) {
            root = root->next_sibling();
        }
        
        if (root && root->type() == rapidxml::node_element) {
            return root;
        }
        return nullptr;
    }

    mutable XmlDocument doc_;
    std::vector<char> sourceBuffer_;
};

// ==================== Convenience Factory Functions ====================

/**
 * @brief Create a new XML document with declaration and root element
 * @param rootName Root element name
 * @param version XML version (default "1.0")
 * @param encoding Character encoding (default "UTF-8")
 * @return XmlDomBuilder instance
 */
[[nodiscard]] inline XmlDomBuilder createXmlDocument(
    std::string_view rootName,
    std::string_view version = "1.0",
    std::string_view encoding = "UTF-8") {
    
    XmlDomBuilder builder;
    builder.createDocument()
           .addDeclaration(version, encoding)
           .createRoot(rootName);
    return builder;
}

/**
 * @brief Load XML from file
 * @param filePath Path to XML file
 * @return XmlDomBuilder instance with loaded document
 * @throws XmlException on error
 */
[[nodiscard]] inline XmlDomBuilder loadXmlFile(const std::filesystem::path& filePath) {
    XmlDomBuilder builder;
    builder.loadFile(filePath);
    return builder;
}

/**
 * @brief Try to load XML from file (returns XmlResult)
 * @param filePath Path to XML file
 * @param outBuilder Output builder (will be populated on success)
 * @return XmlResult indicating success or error
 */
[[nodiscard]] inline XmlResult<void> tryLoadXmlFile(
    const std::filesystem::path& filePath,
    XmlDomBuilder& outBuilder) {
    
    return outBuilder.tryLoadFile(filePath);
}

/**
 * @brief Parse XML from string
 * @param xml XML content
 * @return XmlDomBuilder instance with parsed document
 * @throws XmlParseException on error
 */
[[nodiscard]] inline XmlDomBuilder parseXml(std::string_view xml) {
    XmlDomBuilder builder;
    builder.parseString(xml);
    return builder;
}

/**
 * @brief Try to parse XML from string (returns XmlResult)
 * @param xml XML content
 * @param outBuilder Output builder (will be populated on success)
 * @return XmlResult indicating success or error
 */
[[nodiscard]] inline XmlResult<void> tryParseXml(
    std::string_view xml,
    XmlDomBuilder& outBuilder) {
    
    return outBuilder.tryParseString(xml);
}

} // namespace xml_framework

#endif // XML_BUILDER_H
