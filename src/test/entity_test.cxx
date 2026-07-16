/**
 * @file entity_test.cpp
 * @brief Compare handling of elements in the physical file passed to parser
 * versus those coming from a separate file, included via entity reference.
 * 
 * Note: RapidXML does not support external entity references or entity
 * expansion in the same way as Xerces-C. This test demonstrates basic
 * XML parsing with RapidXML. For full entity reference support, a
 * preprocessing step or different parser would be needed.
 */

#include "xmlBase/safe_xml_parser.hpp"
#include "xmlBase/xml_printer.hpp"

#include "facilities/commonUtilities.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {
   using namespace xml_framework;

   // Helper function to get attribute value from a node
   [[nodiscard]] std::string getAttribute(rapidxml::xml_node<>* node, 
                                          const char* attrName) {
      if (!node) {
         return "";
      }
      auto* attr = node->first_attribute(attrName);
      if (attr && attr->value()) {
         return std::string(attr->value());
      }
      return "";
   }

   // Helper function to get text content from a node
   [[nodiscard]] std::string getTextContent(rapidxml::xml_node<>* node) {
      if (!node) {
         return "";
      }
      
      std::string content;
      for (auto* child = node->first_node(); child; child = child->next_sibling()) {
         if (child->type() == rapidxml::node_data || 
             child->type() == rapidxml::node_cdata) {
            if (child->value()) {
               content += child->value();
            }
         }
      }
      return content;
   }

   // Helper to read file contents
   [[nodiscard]] std::string readFileContents(const std::string& filename) {
      std::ifstream file(filename);
      if (!file) {
         throw std::runtime_error("Cannot open file: " + filename);
      }
      std::ostringstream ss;
      ss << file.rdbuf();
      return ss.str();
   }

   // Process elements with the given tag name
   void processElements(rapidxml::xml_node<>* doc, std::string_view eltName) {
      if (!doc) {
         std::cerr << "invalid document" << std::endl;
         return;
      }

      const std::string eltNameStr(eltName);
      
      // Find all elements with the given name
      std::vector<rapidxml::xml_node<>*> elements;
      
      // Recursive lambda to find all matching elements
      std::function<void(rapidxml::xml_node<>*)> findElements = 
         [&](rapidxml::xml_node<>* node) {
            if (!node) return;
            
            for (auto* child = node->first_node(); child; child = child->next_sibling()) {
               if (child->type() == rapidxml::node_element) {
                  if (child->name() && std::string(child->name()) == eltNameStr) {
                     elements.push_back(child);
                  }
                  // Recurse into children
                  findElements(child);
               }
            }
         };
      
      findElements(doc);

      std::cout << "Found " << elements.size() << " elements with tag '" 
                << eltName << "'" << std::endl;

      for (auto* elt : elements) {
         std::string name = getAttribute(elt, "name");
         std::string value = getAttribute(elt, "value");
         std::string modified = getAttribute(elt, "modified");

         std::cout << "  Element '" << eltName << "':" << std::endl;
         
         if (!name.empty()) {
            std::cout << "    name attribute: " << name << std::endl;
         }
         if (!value.empty()) {
            std::cout << "    value attribute: " << value << std::endl;
         }
         if (!modified.empty()) {
            std::cout << "    modified attribute: " << modified << std::endl;
         }

         // Check for text content
         std::string textContent = getTextContent(elt);
         if (!textContent.empty()) {
            // Trim whitespace
            auto start = textContent.find_first_not_of(" \t\n\r");
            auto end = textContent.find_last_not_of(" \t\n\r");
            if (start != std::string::npos && end != std::string::npos) {
               textContent = textContent.substr(start, end - start + 1);
               if (!textContent.empty()) {
                  std::cout << "    text content: " << textContent << std::endl;
               }
            }
         }

         // Check for child elements
         int childCount = 0;
         for (auto* child = elt->first_node(); child; child = child->next_sibling()) {
            if (child->type() == rapidxml::node_element) {
               ++childCount;
            }
         }
         if (childCount > 0) {
            std::cout << "    child elements: " << childCount << std::endl;
         }
      }
      std::cout << std::endl;
   }
}

int main(int argc, char* argv[]) {
   using namespace xml_framework;
   
   facilities::commonUtilities::setupEnvironment();
   
   // Determine input file
   std::string infile = [&]() {
      if (argc >= 2) {
         return std::string(argv[1]);
      }
      return facilities::commonUtilities::joinPath(
         facilities::commonUtilities::getXmlPath("xmlBase"), "simpleDoc.xml");
   }();

   std::cout << "Parsing file: " << infile << std::endl;
   std::cout << std::string(60, '=') << std::endl << std::endl;

   auto parser = std::make_unique<SafeXmlParser>();

   try {
      // Load and parse the file
      auto loadResult = parser->loadFile(infile);
      if (!loadResult.isSuccess()) {
         std::cerr << "Error loading file: " << infile << std::endl;
         return 1;
      }

      auto buffer = loadResult.value();
      rapidxml::xml_document<> doc;
      parser->parseString(doc, buffer.data());

      auto* root = doc.first_node();
      if (!root) {
         std::cerr << "No root element found in document" << std::endl;
         return 1;
      }

      std::cout << "Document successfully parsed" << std::endl;
      std::cout << "Root element: " << (root->name() ? root->name() : "(unnamed)") 
                << std::endl << std::endl;

      // Process local elements
      std::cout << "Processing local elements ('const')..." << std::endl;
      processElements(root, "const");

      // Process external elements (if any)
      std::cout << "Processing external elements ('extConst')..." << std::endl;
      std::cout << "Note: RapidXML does not automatically expand external entity references."
                << std::endl;
      std::cout << "External entities would need to be preprocessed before parsing."
                << std::endl << std::endl;
      processElements(root, "extConst");

      std::cout << std::string(60, '=') << std::endl;
      std::cout << std::endl;
      std::cout << "RapidXML Entity Reference Behavior:" << std::endl;
      std::cout << "-----------------------------------" << std::endl;
      std::cout << "Unlike Xerces-C, RapidXML does not support:" << std::endl;
      std::cout << "  - External entity reference expansion" << std::endl;
      std::cout << "  - DTD processing and validation" << std::endl;
      std::cout << "  - The setCreateEntityReferenceNodes option" << std::endl;
      std::cout << std::endl;
      std::cout << "For external entity support, preprocess the XML document" << std::endl;
      std::cout << "or use a different parser for the initial expansion." << std::endl;

   } catch (const rapidxml::parse_error& e) {
      std::cerr << "XML parse error: " << e.what() << std::endl;
      return 1;
   } catch (const XmlException& e) {
      std::cerr << "XML exception: " << e.what() << std::endl;
      return 1;
   } catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
   }

   return 0;
}
