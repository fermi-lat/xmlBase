#ifndef XML_PRINTER_HPP
#define XML_PRINTER_HPP

#include "rapidxml.hpp"
#include "rapidxml_error_framework.hpp"
#include "xml_result.hpp"
#include "safe_xml_parser.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <algorithm>  // Added: for std::sort

namespace xml_framework {

// ==================== Print Options ====================

struct XmlPrintOptions {
    std::string indentString = "  ";      // String used for each indent level
    bool sortAttributes = true;            // Sort attributes alphabetically
    bool escapeText = true;               // Escape special characters in text
    bool omitDeclaration = false;         // Skip XML declaration when printing
    bool compactEmptyElements = true;     // Use <tag /> instead of <tag></tag>
    bool trimWhitespace = true;           // Trim whitespace-only text nodes
    char newlineChar = '\n';              // Character to use for newlines
};

// ==================== XML Printer Class ====================

class XmlPrinter {
public:
    using XmlDocument = rapidxml::xml_document<char>;
    using XmlNode = rapidxml::xml_node<char>;
    using XmlAttribute = rapidxml::xml_attribute<char>;

    // ==================== Print Element ====================
    
    [[nodiscard]] static XmlResult<void> tryPrintElement(
        const XmlNode* node,
        std::ostream& out,
        const XmlPrintOptions& options = XmlPrintOptions{}) {
        
        if (!node) {
            return XmlResult<void>::error(XmlError{
                XmlErrorCode::NodeNotFound,
                "Null node argument",
                "XmlPrinter::tryPrintElement"
            });
        }
        
        try {
            printNodeInternal(node, out, options, false, {});
            return XmlResult<void>::success();
        } catch (const std::exception& e) {
            return XmlResult<void>::error(XmlError{
                XmlErrorCode::Unknown,
                e.what(),
                "XmlPrinter::tryPrintElement"
            });
        }
    }
    
    static void printElement(const XmlNode* node,
                            std::ostream& out,
                            const XmlPrintOptions& options = XmlPrintOptions{}) {
        if (!node) {
            throw XmlException("Null node argument", "XmlPrinter::printElement");
        }
        printNodeInternal(node, out, options, false, {});
    }

    // ==================== Pretty Print Element ====================
    
    [[nodiscard]] static XmlResult<void> tryPrettyPrintElement(
        const XmlNode* node,
        std::ostream& out,
        std::string_view prefix = {},
        const XmlPrintOptions& options = XmlPrintOptions{}) {
        
        if (!node) {
            return XmlResult<void>::error(XmlError{
                XmlErrorCode::NodeNotFound,
                "Null node argument",
                "XmlPrinter::tryPrettyPrintElement"
            });
        }
        
        try {
            printNodeInternal(node, out, options, true, std::string(prefix));
            return XmlResult<void>::success();
        } catch (const std::exception& e) {
            return XmlResult<void>::error(XmlError{
                XmlErrorCode::Unknown,
                e.what(),
                "XmlPrinter::tryPrettyPrintElement"
            });
        }
    }
    
    static void prettyPrintElement(const XmlNode* node,
                                   std::ostream& out,
                                   std::string_view prefix = {},
                                   const XmlPrintOptions& options = XmlPrintOptions{}) {
        if (!node) {
            throw XmlException("Null node argument", "XmlPrinter::prettyPrintElement");
        }
        printNodeInternal(node, out, options, true, std::string(prefix));
    }

    // ==================== Print to String ====================
    
    [[nodiscard]] static XmlResult<std::string> tryToString(
        const XmlNode* node,
        bool pretty = false,
        std::string_view prefix = {},
        const XmlPrintOptions& options = XmlPrintOptions{}) {
        
        if (!node) {
            return XmlResult<std::string>::error(XmlError{
                XmlErrorCode::NodeNotFound,
                "Null node argument",
                "XmlPrinter::tryToString"
            });
        }
        
        try {
            std::ostringstream oss;
            if (pretty) {
                printNodeInternal(node, oss, options, true, std::string(prefix));
            } else {
                printNodeInternal(node, oss, options, false, {});
            }
            return XmlResult<std::string>::success(oss.str());
        } catch (const std::exception& e) {
            return XmlResult<std::string>::error(XmlError{
                XmlErrorCode::Unknown,
                e.what(),
                "XmlPrinter::tryToString"
            });
        }
    }
    
    [[nodiscard]] static std::string toString(
        const XmlNode* node,
        bool pretty = false,
        std::string_view prefix = {},
        const XmlPrintOptions& options = XmlPrintOptions{}) {
            
        auto result = tryToString(node, pretty, prefix, options);
        if (result.isError()) {
            throw XmlException(result.error().message, result.error().context);
        }
        return std::move(result).value();
    }

    // ==================== Print Document ====================
    
    [[nodiscard]] static XmlResult<void> tryPrintToFile(
        const XmlNode* node,
        const std::filesystem::path& filePath,
        bool pretty = false,
        std::string_view prefix = {},
        const XmlPrintOptions& options = XmlPrintOptions{}) {
        
        if (!node) {
            return XmlResult<void>::error(XmlError{
                XmlErrorCode::NodeNotFound,
                "Null node argument",
                "XmlPrinter::tryPrintToFile"
            });
        }
        
        try {
            if (filePath.has_parent_path()) {
                std::filesystem::create_directories(filePath.parent_path());
            }
            
            std::ofstream file(filePath, std::ios::out | std::ios::trunc);
            if (!file) {
                return XmlResult<void>::error(XmlError{
                    XmlErrorCode::FileNotFound,
                    "Failed to open file for writing",
                    filePath.string()
                });
            }
            
            if (pretty) {
                printNodeInternal(node, file, options, true, std::string(prefix));
            } else {
                printNodeInternal(node, file, options, false, {});
            }
            
            file.flush();
            if (file.fail()) {
                return XmlResult<void>::error(XmlError{
                    XmlErrorCode::Unknown,
                    "Failed to write to file",
                    filePath.string()
                });
            }
            
            return XmlResult<void>::success();
            
        } catch (const std::exception& e) {
            return XmlResult<void>::error(XmlError{
                XmlErrorCode::Unknown,
                e.what(),
                filePath.string()
            });
        }
    }
    
    static void printToFile(const XmlNode* node,
                            const std::filesystem::path& filePath,
                            bool pretty = false,
                            std::string_view prefix = {},
                            const XmlPrintOptions& options = XmlPrintOptions{}) {
        auto result = tryPrintToFile(node, filePath, pretty, prefix, options);
        if (result.isError()) {
            throw XmlException(result.error().message, result.error().context);
        }
    }

private:
    // ==================== Internal Helper Methods ====================
    
    static std::string escapeXml(std::string_view text) {
        std::string result;
        result.reserve(text.size());
        
        for (char c : text) {
            switch (c) {
                case '&':  result += "&amp;"; break;
                case '<':  result += "&lt;"; break;
                case '>':  result += "&gt;"; break;
                case '"':  result += "&quot;"; break;
                case '\'': result += "&apos;"; break;
                default:   result += c; break;
            }
        }
        return result;
    }
    
    static bool isWhitespaceOnly(std::string_view text) {
        return std::all_of(text.begin(), text.end(), 
            [](unsigned char c) { return std::isspace(c); });
    }
    
    static void printNodeInternal(const XmlNode* node, 
                                  std::ostream& out,
                                  const XmlPrintOptions& options,
                                  bool pretty,
                                  const std::string& indent) {
        if (!node) return;
        
        switch (node->type()) {
            case rapidxml::node_document:
                printDocumentNode(node, out, options, pretty, indent);
                break;
            case rapidxml::node_element:
                printElementNode(node, out, options, pretty, indent);
                break;
            case rapidxml::node_data:
                printDataNode(node, out, options, pretty);
                break;
            case rapidxml::node_cdata:
                printCDataNode(node, out, pretty, indent);
                break;
            case rapidxml::node_comment:
                printCommentNode(node, out, pretty, indent);
                break;
            case rapidxml::node_declaration:
                if (!options.omitDeclaration) {
                    printDeclarationNode(node, out, options, pretty, indent);
                }
                break;
            case rapidxml::node_doctype:
                printDoctypeNode(node, out, pretty, indent);
                break;
            case rapidxml::node_pi:
                printPINode(node, out, pretty, indent);
                break;
            default:
                break;
        }
    }
    
    static void printDocumentNode(const XmlNode* node,
                                  std::ostream& out,
                                  const XmlPrintOptions& options,
                                  bool pretty,
                                  const std::string& indent) {
        for (auto* child = node->first_node(); child; child = child->next_sibling()) {
            printNodeInternal(child, out, options, pretty, indent);
        }
    }
    
    static void printElementNode(const XmlNode* node,
                                 std::ostream& out,
                                 const XmlPrintOptions& options,
                                 bool pretty,
                                 const std::string& indent) {
        if (pretty) out << indent;
        
        out << '<' << node->name();
        
        // Print attributes
        printAttributes(node, out, options);
        
        // Check if element has children
        auto* firstChild = node->first_node();
        bool hasChildren = (firstChild != nullptr);
        bool hasOnlyText = hasChildren && 
                          firstChild->type() == rapidxml::node_data &&
                          firstChild->next_sibling() == nullptr;
        
        if (!hasChildren && options.compactEmptyElements) {
            out << " />";
            if (pretty) out << options.newlineChar;
        } else if (hasOnlyText && !pretty) {
            out << '>';
            if (options.escapeText) {
                out << escapeXml(firstChild->value());
            } else {
                out << firstChild->value();
            }
            out << "</" << node->name() << '>';
        } else {
            out << '>';
            if (pretty && !hasOnlyText) out << options.newlineChar;
            
            std::string childIndent = pretty ? indent + options.indentString : "";
            
            for (auto* child = firstChild; child; child = child->next_sibling()) {
                if (hasOnlyText && child->type() == rapidxml::node_data) {
                    if (options.escapeText) {
                        out << escapeXml(child->value());
                    } else {
                        out << child->value();
                    }
                } else {
                    printNodeInternal(child, out, options, pretty, childIndent);
                }
            }
            
            if (pretty && !hasOnlyText) out << indent;
            out << "</" << node->name() << '>';
            if (pretty) out << options.newlineChar;
        }
    }
    
    static void printAttributes(const XmlNode* node,
                               std::ostream& out,
                               const XmlPrintOptions& options) {
        if (options.sortAttributes) {
            // Collect and sort attributes
            std::vector<std::pair<std::string, std::string>> attrs;
            for (auto* attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
                attrs.emplace_back(attr->name(), attr->value());
            }
            std::sort(attrs.begin(), attrs.end());
            
            for (const auto& [name, value] : attrs) {
                out << ' ' << name << "=\"";
                if (options.escapeText) {
                    out << escapeXml(value);
                } else {
                    out << value;
                }
                out << '"';
            }
        } else {
            for (auto* attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
                out << ' ' << attr->name() << "=\"";
                if (options.escapeText) {
                    out << escapeXml(attr->value());
                } else {
                    out << attr->value();
                }
                out << '"';
            }
        }
    }
    
    static void printDataNode(const XmlNode* node,
                             std::ostream& out,
                             const XmlPrintOptions& options,
                             bool pretty) {
        std::string_view text(node->value(), node->value_size());
        
        if (options.trimWhitespace && isWhitespaceOnly(text)) {
            return;
        }
        
        if (options.escapeText) {
            out << escapeXml(text);
        } else {
            out << text;
        }
    }
    
    static void printCDataNode(const XmlNode* node,
                              std::ostream& out,
                              bool pretty,
                              const std::string& indent) {
        if (pretty) out << indent;
        out << "<![CDATA[" << node->value() << "]]>";
        if (pretty) out << '\n';
    }
    
    static void printCommentNode(const XmlNode* node,
                                std::ostream& out,
                                bool pretty,
                                const std::string& indent) {
        if (pretty) out << indent;
        out << "<!--" << node->value() << "-->";
        if (pretty) out << '\n';
    }
    
    static void printDeclarationNode(const XmlNode* node,
                                     std::ostream& out,
                                     const XmlPrintOptions& options,
                                     bool pretty,
                                     const std::string& indent) {
        if (pretty) out << indent;
        out << "<?xml";
        printAttributes(node, out, options);
        out << "?>";
        if (pretty) out << options.newlineChar;
    }
    
    static void printDoctypeNode(const XmlNode* node,
                                std::ostream& out,
                                bool pretty,
                                const std::string& indent) {
        if (pretty) out << indent;
        out << "<!DOCTYPE " << node->value() << '>';
        if (pretty) out << '\n';
    }
    
    static void printPINode(const XmlNode* node,
                           std::ostream& out,
                           bool pretty,
                           const std::string& indent) {
        if (pretty) out << indent;
        out << "<?" << node->name();
        if (node->value_size() > 0) {
            out << ' ' << node->value();
        }
        out << "?>";
        if (pretty) out << '\n';
    }
};

} // namespace xml_framework

#endif // XML_PRINTER_HPP
