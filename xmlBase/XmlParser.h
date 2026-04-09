/** @file XmlParser.h
    @brief declaration of XmlParser wrapper for RapidXML

    Provides compatibility layer for code migrating from Xerces-C to RapidXML.
*/
#ifndef xmlBase_XmlParser_h
#define xmlBase_XmlParser_h

#include "rapidxml.hpp"
#include "rapidxml_error_framework.hpp"
#include "safe_xml_parser.hpp"
#include "xml_result.hpp"

#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <cstring>

namespace xmlBase {

// Type aliases for RapidXML types
using xml_document = rapidxml::xml_document<char>;
using xml_node = rapidxml::xml_node<char>;
using xml_attribute = rapidxml::xml_attribute<char>;

/**
 * @class XmlParser
 * @brief Wrapper class providing Xerces-like API over RapidXML
 * 
 * This class provides a compatibility layer for code that was written
 * against the Xerces-C API but needs to use RapidXML instead.
 */
class XmlParser {
public:
    XmlParser() = default;
    ~XmlParser() = default;
    
    // Non-copyable
    XmlParser(const XmlParser&) = delete;
    XmlParser& operator=(const XmlParser&) = delete;
    
    // Movable
    XmlParser(XmlParser&&) = default;
    XmlParser& operator=(XmlParser&&) = default;

    /**
     * @brief Parse an XML file
     * @param filename Path to the XML file
     * @return Pointer to the parsed document, or nullptr on error
     * 
     * Note: The returned document is owned by this XmlParser instance.
     * The document remains valid until the next call to parse() or
     * until the XmlParser is destroyed.
     */
    xml_document* parse(const char* filename) {
        return parse(std::string(filename));
    }
    
    xml_document* parse(const std::string& filename) {
        // Read file into buffer
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return nullptr;
        }
        
        auto size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        m_buffer.resize(static_cast<size_t>(size) + 1);
        if (!file.read(m_buffer.data(), size)) {
            return nullptr;
        }
        m_buffer[static_cast<size_t>(size)] = '\0';
        
        // Parse the document
        m_document = std::make_unique<xml_document>();
        try {
            m_document->parse<rapidxml::parse_default>(m_buffer.data());
            return m_document.get();
        } catch (const rapidxml::parse_error& e) {
            m_document.reset();
            return nullptr;
        }
    }
    
    /**
     * @brief Get the document element (root element)
     * @param doc Document to get root from (if nullptr, uses internal document)
     * @return Pointer to root element, or nullptr if none
     */
    static xml_node* getDocumentElement(xml_document* doc) {
        if (!doc) return nullptr;
        return doc->first_node();
    }
    
    /**
     * @brief Collect all child elements with a given tag name
     * @param parent Parent node
     * @param tagName Tag name to search for
     * @param children Output vector to receive child nodes
     */
    static void collectChildren(xml_node* parent, const char* tagName, 
                                std::vector<xml_node*>& children) {
        if (!parent) return;
        children.clear();
        
        for (xml_node* child = parent->first_node(tagName); 
             child; 
             child = child->next_sibling(tagName)) {
            if (child->type() == rapidxml::node_element) {
                children.push_back(child);
            }
        }
    }
    
    /**
     * @brief Get attribute value with type conversion
     * @tparam T Target type
     * @param node Node to get attribute from
     * @param attrName Attribute name
     * @return XmlResult containing the value or error
     */
    template<typename T>
    static xml_framework::XmlResult<T> getAttributeValue(xml_node* node, const char* attrName) {
        return xml_framework::SafeXmlParser::getAttributeValue<T>(node, attrName);
    }
    
    /**
     * @brief Get string attribute value (convenience method)
     * @param node Node to get attribute from  
     * @param attrName Attribute name
     * @return Attribute value as string, or empty string if not found
     */
    static std::string getAttribute(xml_node* node, const char* attrName) {
        if (!node) return "";
        xml_attribute* attr = node->first_attribute(attrName);
        return attr ? std::string(attr->value()) : "";
    }
    
    /**
     * @brief Get double attribute value (convenience method)
     * @param node Node to get attribute from
     * @param attrName Attribute name
     * @return Attribute value as double, or 0.0 if not found/invalid
     */
    static double getDoubleAttribute(xml_node* node, const char* attrName) {
        std::string val = getAttribute(node, attrName);
        if (val.empty()) return 0.0;
        try {
            return std::stod(val);
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief Get first child element
     * @param parent Parent node
     * @return First child element, or nullptr if none
     */
    static xml_node* getFirstChildElement(xml_node* parent) {
        if (!parent) return nullptr;
        for (xml_node* child = parent->first_node(); child; child = child->next_sibling()) {
            if (child->type() == rapidxml::node_element) {
                return child;
            }
        }
        return nullptr;
    }
    
    /**
     * @brief Get first child element with specific name
     * @param parent Parent node
     * @param name Element name (use "*" for any element)
     * @return First matching child element, or nullptr if none
     */
    static xml_node* findFirstChildByName(xml_node* parent, const char* name) {
        if (!parent) return nullptr;
        if (name && std::strcmp(name, "*") == 0) {
            return getFirstChildElement(parent);
        }
        return parent->first_node(name);
    }
    
    /**
     * @brief Get next sibling element
     * @param node Current node
     * @return Next sibling element, or nullptr if none
     */
    static xml_node* getSiblingElement(xml_node* node) {
        if (!node) return nullptr;
        for (xml_node* sibling = node->next_sibling(); sibling; sibling = sibling->next_sibling()) {
            if (sibling->type() == rapidxml::node_element) {
                return sibling;
            }
        }
        return nullptr;
    }
    
    /**
     * @brief Get tag name of a node
     * @param node Node to get name from
     * @return Tag name, or empty string if node is null
     */
    static std::string getTagName(xml_node* node) {
        if (!node || !node->name()) return "";
        return std::string(node->name(), node->name_size());
    }
    
    /**
     * @brief Check if node has a specific attribute
     * @param node Node to check
     * @param attrName Attribute name
     * @return true if attribute exists
     */
    static bool hasAttribute(xml_node* node, const char* attrName) {
        if (!node) return false;
        return node->first_attribute(attrName) != nullptr;
    }
    
    /**
     * @brief Check if node has a specific tag name
     * @param node Node to check
     * @param tagName Tag name to compare
     * @return true if tag names match
     */
    static bool checkTagName(xml_node* node, const char* tagName) {
        if (!node || !tagName) return false;
        return std::strcmp(node->name(), tagName) == 0;
    }
    
    /**
     * @brief Reset the parser (clear document and buffer)
     */
    void reset() {
        m_document.reset();
        m_buffer.clear();
    }

private:
    std::unique_ptr<xml_document> m_document;
    std::vector<char> m_buffer;  // Buffer must persist during document lifetime
};

} // namespace xmlBase

#endif // xmlBase_XmlParser_h
