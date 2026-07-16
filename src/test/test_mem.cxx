/**
 * @file test_mem.cpp
 * @brief Test program for XML facility. Parse XML from a string and write it out to a stream.
 */

#include "xmlBase/safe_xml_parser.hpp"
#include "xmlBase/xml_printer.hpp"

#include "facilities/commonUtilities.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {
   // XML document as a string literal
   // Note: RapidXML does not validate against DTD, so the DTD portion
   // is preserved in the string but will be ignored during parsing.
   constexpr const char* doc_string = 
      "<?xml version=\"1.0\" ?>"
      "<!DOCTYPE TopElement ["
      "  <!ELEMENT TopElement (ChildElt*) >"
      "  <!ELEMENT ChildElt (ChildWithText | EmptyChild)* >"
      "  <!ATTLIST ChildElt  anAttribute CDATA #REQUIRED >"
      "  <!ELEMENT ChildWithText (#PCDATA) >"
      "  <!ATTLIST ChildWithText attr CDATA #IMPLIED>"
      "  <!ELEMENT EmptyChild EMPTY>  ]"
      ">"
      "<TopElement>"
      "  <ChildElt  anAttribute=\"I'm nested but empty\" />"
      "  <ChildElt  anAttribute=\"I'm nested with content\">"
      "     <ChildWithText attr=\"text content\" >"
      "         Text content here."
      "     </ChildWithText>"
      "     <EmptyChild />"
      "  </ChildElt>"
      "</TopElement>";
}

int main() {
   using namespace xml_framework;
   
   facilities::commonUtilities::setupEnvironment();
   
   auto parser = std::make_unique<SafeXmlParser>();
   
   // RapidXML requires a mutable buffer for in-place parsing
   std::string xmlContent(doc_string);
   std::vector<char> buffer(xmlContent.begin(), xmlContent.end());
   buffer.push_back('\0');
   
   rapidxml::xml_document<> doc;
   
   try {
      // Parse the XML string
      parser->parseString(doc, buffer.data());
      
      // Get the document element (root)
      auto* docElt = doc.first_node();
      
      // Skip any non-element nodes (like DTD declarations) to find the actual root element
      while (docElt && docElt->type() != rapidxml::node_element) {
         docElt = docElt->next_sibling();
      }
      
      if (docElt != nullptr) {
         std::cout << "Document successfully parsed" << std::endl;
         std::cout << std::endl;
         
         // Pretty print the document element
         XmlPrinter printer;
         auto result = printer.tryToString(docElt, true, "");
         
         if (result.isSuccess()) {
            std::cout << result.value() << std::endl;
         } else {
            std::cerr << "Failed to serialize document" << std::endl;
            return 1;
         }
      } else {
         std::cerr << "Parse succeeded but no root element found" << std::endl;
         return 1;
      }
      
   } catch (const rapidxml::parse_error& e) {
      std::cerr << "XML parse error: " << e.what() << std::endl;
      return 1;
   } catch (const XmlException& e) {
      std::cerr << "XML exception: " << e.what() << std::endl;
      return 1;
   } catch (const std::exception& e) {
      std::cerr << "Unexpected error: " << e.what() << std::endl;
      return 1;
   }
   
   return 0;
}
