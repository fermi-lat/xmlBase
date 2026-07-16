/**
 * @file test_parse.cpp
 * @brief Test program for XML facility. Parse XML file; complain if it fails.
 * 
 * Input arg, if present, is path to file to be parsed. Otherwise use default.
 */

#include "xmlBase/safe_xml_parser.hpp"

#include "facilities/commonUtilities.h"
#include "facilities/Util.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
   using namespace xml_framework;
   
   facilities::commonUtilities::setupEnvironment();

   // Determine file path
   std::string filepath = [&]() {
      if (argc > 1) {
         return std::string(argv[1]);
      }
      return std::string("$(XMLBASEXMLPATH)/simpleDoc.xml");
   }();
   
   // Expand any environment variables in the path
   facilities::Util::expandEnvVar(&filepath);
   
   auto parser = std::make_unique<SafeXmlParser>();
   
   try {
      auto parseResult = parser->loadFile(filepath);
      if (!parseResult.isSuccess()) {
         std::cerr << "Failed to load file: " << filepath << std::endl;
         return 1;
      }
      
      auto buffer = parseResult.value();
      rapidxml::xml_document<> doc;
      parser->parseString(doc, buffer.data());
      
      // Verify we got a valid document
      auto* root = doc.first_node();
      if (root != nullptr) {
         std::cout << "Document successfully parsed" << std::endl;
         // Optionally print root element name
         if (root->name()) {
            std::cout << "Root element: " << root->name() << std::endl;
         }
      } else {
         std::cerr << "Document parsed but no root element found" << std::endl;
         return 1;
      }
      
   } catch (const XmlParseException& e) {
      std::cerr << "Parse failed: " << e.what() << std::endl;
      return 1;
   } catch (const std::exception& e) {
      std::cerr << "Unexpected error: " << e.what() << std::endl;
      return 1;
   }
   
   return 0;
}
