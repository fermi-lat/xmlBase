/**
 * @file test_altSchema.cpp
 * @brief Test program for XML parsing with schema validation (RapidXML version)
 * 
 * Note: RapidXML does not support schema validation. This test now simply
 * verifies that the XML document can be parsed successfully. For schema
 * validation, an external validator should be used before parsing.
 */

#include "xmlBase/safe_xml_parser.hpp"

#include "facilities/commonUtilities.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {
   // Helper function to read file contents
   [[nodiscard]] std::string readFileContents(const std::string& filename) {
      std::ifstream file(filename);
      if (!file) {
         throw std::runtime_error("Cannot open file: " + filename);
      }
      std::ostringstream ss;
      ss << file.rdbuf();
      return ss.str();
   }
}

int main() {
   using namespace xml_framework;
   
   facilities::commonUtilities::setupEnvironment();
   
   // File is well-formed, no reference to dtd or schema
   const std::string instanceDoc = facilities::commonUtilities::joinPath(
      facilities::commonUtilities::getXmlPath("xmlBase"), "aDocument.xml");
   const std::string theSchema = facilities::commonUtilities::joinPath(
      facilities::commonUtilities::getXmlPath("xmlBase"), "theSchema.xsd");

   // Note: RapidXML does not support schema validation.
   // The schema file path is preserved here for documentation purposes,
   // but validation must be done externally if needed.
   std::cout << "Schema file (not used for validation): " << theSchema << std::endl;
   std::cout << "Note: RapidXML does not support schema validation." << std::endl;
   std::cout << std::endl;

   auto parser = std::make_unique<SafeXmlParser>();
   
   try {
      auto parseResult = parser->loadFile(instanceDoc);
      if (!parseResult.isSuccess()) {
         std::cerr << "Failed to load file " << instanceDoc << std::endl;
         return 1;
      }
      
      auto buffer = parseResult.value();
      rapidxml::xml_document<> doc;
      parser->parseString(doc, buffer.data());
      
      // Verify we got a document root
      auto* root = doc.first_node();
      if (!root) {
         std::cerr << "Parse of file " << instanceDoc << " failed - no root element" 
                   << std::endl;
         return 1;
      }
      
      std::cout << "Parse of file " << instanceDoc << " succeeded!" << std::endl;
      std::cout << "Root element: " << root->name() << std::endl;
      
   } catch (const XmlParseException& e) {
      std::cerr << "Parse of file " << instanceDoc << " failed" << std::endl;
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
   } catch (const std::exception& e) {
      std::cerr << "Unexpected error: " << e.what() << std::endl;
      return 1;
   }

   return 0;
}
