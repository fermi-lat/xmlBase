#ifndef XML_PRINTER_HPP
#define XML_PRINTER_HPP

#include <iostream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <map>
#include "rapidxml.hpp"

namespace xml_framework {

// ==================== Exception Types ====================

class NullNode : public std::runtime_error {
public:
    explicit NullNode(const std::string& message)
        : std::runtime_error(message) {}
    
    explicit NullNode(const char* message)
        : std::runtime_error(message) {}
};

// ==================== XML Printer Class ====================

class XmlPrinter {
public:
    using XmlNode = rapidxml::xml_node<char>;
    using XmlAttribute = rapidxml::xml_attribute<char>;

    // ==================== Print Element (Compact) ====================
    
    /**
     * @brief Print an XML element without formatting (compact output)
     * @param node The XML node to print
     * @param out Output stream
     * @throws NullNode if node is nullptr
     * 
     * Replicates Xerces-C Dom::printElement behavior
     */
    static void printElement(const XmlNode* node, std::ostream& out) {
        if (node == nullptr) {
            throw NullNode("from xml_framework::XmlPrinter::printElement. Null argument");
        }

        switch (node->type()) {
            case rapidxml::node_element:
                printElementNode(node, out);
                break;
                
            case rapidxml::node_data:
                // Text node - output as is
                printTextContent(node, out);
                break;
                
            case rapidxml::node_cdata:
                // CDATA section
                out << "<![CDATA[" << getNodeValue(node) << "]]>";
                break;
                
            case rapidxml::node_comment:
                // Comment node
                out << "<!--" << getNodeValue(node) << "-->";
                break;
                
            case rapidxml::node_declaration:
                // XML declaration
                printDeclaration(node, out);
                break;
                
            case rapidxml::node_doctype:
                // DOCTYPE
                out << "<!DOCTYPE " << getNodeValue(node) << ">";
                break;
                
            case rapidxml::node_pi:
                // Processing instruction
                out << "<?" << getNodeName(node) << " " << getNodeValue(node) << "?>";
                break;
                
            default:
                // Ignore anything else (node_document, etc.)
                break;
        }
    }

    // ==================== Pretty Print Element (Formatted) ====================
    
    /**
     * @brief Print an XML element with formatting (indentation and line breaks)
     * @param node The XML node to print
     * @param out Output stream
     * @param prefix Current indentation prefix
     * @throws NullNode if node is nullptr
     * 
     * Replicates Xerces-C Dom::prettyPrintElement behavior
     */
    static void prettyPrintElement(const XmlNode* node, 
                                   std::ostream& out,
                                   std::string prefix = "") {
        if (node == nullptr) {
            throw NullNode("from xml_framework::XmlPrinter::prettyPrintElement. Null argument");
        }

        switch (node->type()) {
            case rapidxml::node_element:
                prettyPrintElementNode(node, out, prefix);
                break;
                
            case rapidxml::node_data:
                // Text node - check if it's just whitespace
                {
                    std::string_view text = getNodeValue(node);
                    if (!isWhitespaceOnly(text)) {
                        out << prefix << text << '\n';
                    }
                }
                break;
                
            case rapidxml::node_cdata:
                out << prefix << "<![CDATA[" << getNodeValue(node) << "]]>" << '\n';
                break;
                
            case rapidxml::node_comment:
                out << prefix << "<!--" << getNodeValue(node) << "-->" << '\n';
                break;
                
            case rapidxml::node_declaration:
                out << prefix;
                printDeclaration(node, out);
                out << '\n';
                break;
                
            case rapidxml::node_doctype:
                out << prefix << "<!DOCTYPE " << getNodeValue(node) << ">" << '\n';
                break;
                
            case rapidxml::node_pi:
                out << prefix << "<?" << getNodeName(node) << " " 
                    << getNodeValue(node) << "?>" << '\n';
                break;
                
            default:
                break;
        }
    }

    // ==================== Convenience Overloads ====================
    
    /**
     * @brief Print element to string (compact)
     */
    [[nodiscard]] static std::string printElementToString(const XmlNode* node) {
        std::ostringstream oss;
        printElement(node, oss);
        return oss.str();
    }
    
    /**
     * @brief Pretty print element to string
     */
    [[nodiscard]] static std::string prettyPrintElementToString(
        const XmlNode* node,
        const std::string& prefix = "") {
        std::ostringstream oss;
        prettyPrintElement(node, oss, prefix);
        return oss.str();
    }

    // ==================== Print Document ====================
    
    /**
     * @brief Print entire document (compact)
     */
    static void printDocument(const rapidxml::xml_document<char>& doc, 
                             std::ostream& out) {
        for (const XmlNode* node = doc.first_node(); 
             node != nullptr; 
             node = node->next_sibling()) {
            printElement(node, out);
        }
    }
    
    /**
     * @brief Pretty print entire document
     */
    static void prettyPrintDocument(const rapidxml::xml_document<char>& doc,
                                    std::ostream& out,
                                    const std::string& prefix = "") {
        for (const XmlNode* node = doc.first_node(); 
             node != nullptr; 
             node = node->next_sibling()) {
            prettyPrintElement(node, out, prefix);
        }
    }

private:
    // ==================== Helper: Get Node Name ====================
    
    [[nodiscard]] static std::string_view getNodeName(const XmlNode* node) noexcept {
        if (node && node->name()) {
            return std::string_view(node->name(), node->name_size());
        }
        return std::string_view{};
    }
    
    // ==================== Helper: Get Node Value ====================
    
    [[nodiscard]] static std::string_view getNodeValue(const XmlNode* node) noexcept {
        if (node && node->value()) {
            return std::string_view(node->value(), node->value_size());
        }
        return std::string_view{};
    }
    
    // ==================== Helper: Get Attribute Value ====================
    
    [[nodiscard]] static std::string_view getAttributeValue(
        const XmlAttribute* attr) noexcept {
        if (attr && attr->value()) {
            return std::string_view(attr->value(), attr->value_size());
        }
        return std::string_view{};
    }
    
    [[nodiscard]] static std::string_view getAttributeName(
        const XmlAttribute* attr) noexcept {
        if (attr && attr->name()) {
            return std::string_view(attr->name(), attr->name_size());
        }
        return std::string_view{};
    }

    // ==================== Helper: Check Whitespace ====================
    
    [[nodiscard]] static bool isWhitespaceOnly(std::string_view sv) noexcept {
        for (char c : sv) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                return false;
            }
        }
        return true;
    }
    
    // ==================== Helper: Escape XML Special Characters ====================
    
    static void writeEscaped(std::ostream& out, std::string_view text) {
        for (char c : text) {
            switch (c) {
                case '&':
                    out << "&amp;";
                    break;
                case '<':
                    out << "&lt;";
                    break;
                case '>':
                    out << "&gt;";
                    break;
                case '"':
                    out << "&quot;";
                    break;
                case '\'':
                    out << "&apos;";
                    break;
                default:
                    out << c;
                    break;
            }
        }
    }
    
    static void writeAttributeEscaped(std::ostream& out, std::string_view text) {
        for (char c : text) {
            switch (c) {
                case '&':
                    out << "&amp;";
                    break;
                case '<':
                    out << "&lt;";
                    break;
                case '"':
                    out << "&quot;";
                    break;
                default:
                    out << c;
                    break;
            }
        }
    }

    // ==================== Helper: Print Text Content ====================
    
    static void printTextContent(const XmlNode* node, std::ostream& out) {
        std::string_view text = getNodeValue(node);
        writeEscaped(out, text);
    }

    // ==================== Helper: Print Attributes ====================
    
    static void printAttributes(const XmlNode* node, std::ostream& out) {
        // Collect attributes into a map for consistent ordering (like Xerces version)
        std::map<std::string, std::string_view> atts;
        
        for (const XmlAttribute* attr = node->first_attribute();
             attr != nullptr;
             attr = attr->next_attribute()) {
            std::string_view name = getAttributeName(attr);
            std::string_view value = getAttributeValue(attr);
            atts.emplace(std::string(name), value);
        }
        
        for (const auto& [attName, attValue] : atts) {
            out << ' ' << attName << "=\"";
            writeAttributeEscaped(out, attValue);
            out << '"';
        }
    }
    
    // Alternative: Print attributes in document order (faster, no map)
    static void printAttributesInOrder(const XmlNode* node, std::ostream& out) {
        for (const XmlAttribute* attr = node->first_attribute();
             attr != nullptr;
             attr = attr->next_attribute()) {
            out << ' ' << getAttributeName(attr) << "=\"";
            writeAttributeEscaped(out, getAttributeValue(attr));
            out << '"';
        }
    }

    // ==================== Helper: Print Declaration ====================
    
    static void printDeclaration(const XmlNode* node, std::ostream& out) {
        out << "<?xml";
        printAttributesInOrder(node, out);
        out << "?>";
    }

    // ==================== Core: Print Element Node (Compact) ====================
    
    static void printElementNode(const XmlNode* node, std::ostream& out) {
        // Output start tag
        std::string_view tagName = getNodeName(node);
        out << '<' << tagName;
        
        // Output attributes
        printAttributes(node, out);
        
        // Iterate through children
        const XmlNode* child = node->first_node();
        
        if (child != nullptr) {
            // There are children
            out << '>';
            
            while (child != nullptr) {
                printElement(child, out);
                child = child->next_sibling();
            }
            
            // Output end tag, long form
            out << "</" << tagName << ">";
        }
        else {
            // No children; use short form for empty tag
            out << " />";
        }
    }

    // ==================== Core: Pretty Print Element Node ====================
    
    static void prettyPrintElementNode(const XmlNode* node, 
                                       std::ostream& out,
                                       const std::string& prefix) {
        // Output prefix and start tag
        std::string_view tagName = getNodeName(node);
        out << prefix << '<' << tagName;
        
        // Output attributes
        printAttributes(node, out);
        
        // Check for children
        const XmlNode* child = node->first_node();
        
        if (child != nullptr) {
            // Determine if children are all text/simple content
            bool hasElementChildren = false;
            bool hasOnlyWhitespace = true;
            
            for (const XmlNode* c = child; c != nullptr; c = c->next_sibling()) {
                if (c->type() == rapidxml::node_element) {
                    hasElementChildren = true;
                    hasOnlyWhitespace = false;
                    break;
                }
                if (c->type() == rapidxml::node_data || 
                    c->type() == rapidxml::node_cdata) {
                    if (!isWhitespaceOnly(getNodeValue(c))) {
                        hasOnlyWhitespace = false;
                    }
                }
            }
            
            if (hasElementChildren) {
                // Has element children - use multi-line format
                out << '>' << '\n';
                
                while (child != nullptr) {
                    prettyPrintElement(child, out, prefix + "  ");
                    child = child->next_sibling();
                }
                
                // Output end tag with prefix
                out << prefix << "</" << tagName << ">" << '\n';
            }
            else if (!hasOnlyWhitespace) {
                // Has only text content - keep on same line
                out << '>';
                
                while (child != nullptr) {
                    if (child->type() == rapidxml::node_data) {
                        std::string_view text = getNodeValue(child);
                        if (!isWhitespaceOnly(text)) {
                            writeEscaped(out, text);
                        }
                    }
                    else if (child->type() == rapidxml::node_cdata) {
                        out << "<![CDATA[" << getNodeValue(child) << "]]>";
                    }
                    child = child->next_sibling();
                }
                
                out << "</" << tagName << ">" << '\n';
            }
            else {
                // Only whitespace children - treat as empty
                out << " />" << '\n';
            }
        }
        else {
            // No children; use short form for empty tag
            out << " />" << '\n';
        }
    }
};

// ==================== Free Functions (for convenience) ====================

inline void printElement(const rapidxml::xml_node<char>* node, std::ostream& out) {
    XmlPrinter::printElement(node, out);
}

inline void prettyPrintElement(const rapidxml::xml_node<char>* node, 
                               std::ostream& out,
                               const std::string& prefix = "") {
    XmlPrinter::prettyPrintElement(node, out, prefix);
}

[[nodiscard]] inline std::string printElementToString(
    const rapidxml::xml_node<char>* node) {
    return XmlPrinter::printElementToString(node);
}

[[nodiscard]] inline std::string prettyPrintElementToString(
    const rapidxml::xml_node<char>* node,
    const std::string& prefix = "") {
    return XmlPrinter::prettyPrintElementToString(node, prefix);
}

} // namespace xml_framework

#endif // XML_PRINTER_HPP
