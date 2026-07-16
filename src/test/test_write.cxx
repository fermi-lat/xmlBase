/**
 * @file test_write.cpp
 * @brief Test program for serialization of DOM, stripping of comments
 * 
 * This version uses RapidXML for XML creation and manipulation.
 */

#include "xmlBase/safe_xml_parser.hpp"
#include "xmlBase/xml_printer.hpp"

#include "facilities/commonUtilities.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {
   using namespace xml_framework;

   // Helper to allocate string in document
   [[nodiscard]] char* allocString(rapidxml::xml_document<>& doc, const std::string& str) {
      return doc.allocate_string(str.c_str(), str.size() + 1);
   }

   // Helper to create a child element
   rapidxml::xml_node<>* makeChildNode(rapidxml::xml_document<>& doc,
                                       rapidxml::xml_node<>* parent,
                                       const std::string& name) {
      auto* node = doc.allocate_node(rapidxml::node_element, allocString(doc, name));
      parent->append_node(node);
      return node;
   }

   // Helper to create a child element with text content
   rapidxml::xml_node<>* makeChildNodeWithContent(rapidxml::xml_document<>& doc,
                                                  rapidxml::xml_node<>* parent,
                                                  const std::string& name,
                                                  const std::string& content) {
      auto* node = doc.allocate_node(rapidxml::node_element, allocString(doc, name));
      auto* textNode = doc.allocate_node(rapidxml::node_data, nullptr, allocString(doc, content));
      node->append_node(textNode);
      parent->append_node(node);
      return node;
   }

   // Helper to create a child element with integer content
   rapidxml::xml_node<>* makeChildNodeWithContent(rapidxml::xml_document<>& doc,
                                                  rapidxml::xml_node<>* parent,
                                                  const std::string& name,
                                                  int value) {
      return makeChildNodeWithContent(doc, parent, name, std::to_string(value));
   }

   // Helper to create a child element with hex content
   rapidxml::xml_node<>* makeChildNodeWithHexContent(rapidxml::xml_document<>& doc,
                                                     rapidxml::xml_node<>* parent,
                                                     const std::string& name,
                                                     int value) {
      std::ostringstream ss;
      ss << "0x" << std::hex << value;
      return makeChildNodeWithContent(doc, parent, name, ss.str());
   }

   // Helper to strip comment nodes from XML tree
   void stripComments(rapidxml::xml_node<>* node) {
      if (!node) return;
      
      auto* child = node->first_node();
      while (child) {
         auto* next = child->next_sibling();
         if (child->type() == rapidxml::node_comment) {
            node->remove_node(child);
         } else {
            stripComments(child);
         }
         child = next;
      }
   }

   // Helper to write XML to file
   [[nodiscard]] bool writeXml(rapidxml::xml_node<>* node, 
                               const std::string& filename,
                               bool standalone = false) {
      std::ofstream file(filename);
      if (!file) {
         return false;
      }
      
      // Write XML declaration
      file << "<?xml version=\"1.0\"";
      if (standalone) {
         file << " standalone=\"yes\"";
      }
      file << "?>\n";
      
      // Use XmlPrinter to serialize
      XmlPrinter printer;
      auto result = printer.tryToString(node, true);
      if (!result.isSuccess()) {
         return false;
      }
      
      file << result.value();
      return file.good();
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
}

// Return of 0 is good status
unsigned stripAndWrite(const std::string& fname, bool standalone = false);

int main() {
   using namespace xml_framework;
   using facilities::commonUtilities;

   // Test out creating a new document
   rapidxml::xml_document<> doc;
   
   // Create root element
   auto* myDocRoot = doc.allocate_node(rapidxml::node_element, 
                                       doc.allocate_string("myDoc"));
   doc.append_node(myDocRoot);
   
   // Create child elements
   [[maybe_unused]] auto* emptyNode = makeChildNode(doc, myDocRoot, "emptyElement");
   auto* secondChild = makeChildNode(doc, myDocRoot, "secondChild");
   [[maybe_unused]] auto* grandChild = makeChildNodeWithContent(doc, secondChild, 
                                                                 "grandChild", "yoo-hoo");
   [[maybe_unused]] auto* intChild = makeChildNodeWithContent(doc, secondChild, 
                                                               "intChild", 2);
   [[maybe_unused]] auto* hexChild = makeChildNodeWithHexContent(doc, secondChild, 
                                                                  "hexChild", 1023);

   if (writeXml(myDocRoot, "myDoc.xml")) {
      std::cout << "Successfully wrote myDoc.xml" << std::endl;
   } else {
      std::cout << "Unable to write myDoc.xml" << std::endl;
   }

   commonUtilities::setupEnvironment();
   
   // File is well-formed, no reference to dtd or schema
   std::string WFfile = commonUtilities::joinPath(
      commonUtilities::getXmlPath("xmlBase"), "test.xml");
   unsigned ret = stripAndWrite(WFfile);
   std::string summary = ret ? "FAILURE" : "SUCCESS";
   std::cout << "strip and write " << WFfile << " was a " << summary << std::endl;

   // Here's a file with embedded dtd
   std::string embeddedDtd = commonUtilities::joinPath(
      commonUtilities::getXmlPath("xmlBase"), "test-dtd.xml");
   ret = stripAndWrite(embeddedDtd); 
   summary = ret ? "FAILURE" : "SUCCESS";
   std::cout << "strip and write " << embeddedDtd << " was a " << summary << std::endl;

   // Now do it again for test file referencing dtd
   std::string refDtd = commonUtilities::joinPath(
      commonUtilities::getXmlPath("xmlBase"), "myIFile.xml");
   ret = stripAndWrite(refDtd, true);
   summary = ret ? "FAILURE" : "SUCCESS";
   std::cout << "strip and write " << refDtd << " was a " << summary << std::endl;

   // One last time for test file referencing schema rather than dtd
   std::string refSchema = commonUtilities::joinPath(
      commonUtilities::getXmlPath("xmlBase"), "mySchemaIFile.xml");
   ret = stripAndWrite(refSchema);
   summary = ret ? "FAILURE" : "SUCCESS";
   std::cout << "strip and write " << refSchema << " was a " << summary << std::endl;
  
   return 0;
}

// Return of 0 is good status
unsigned stripAndWrite(const std::string& fname, bool standalone) {
   using namespace xml_framework;

   auto parser = std::make_unique<SafeXmlParser>();
   std::string outname = fname + "-stripped";

   try {
      auto parseResult = parser->loadFile(fname);
      if (!parseResult.isSuccess()) {
         std::cerr << "Parse of file " << fname << " failed" << std::endl;
         return 1;
      }
      
      auto buffer = parseResult.value();
      rapidxml::xml_document<> doc;
      parser->parseString(doc, buffer.data());
      
      auto* root = doc.first_node();
      if (!root) {
         std::cerr << "Parse of file " << fname << " failed - no root element" << std::endl;
         return 1;
      }

      // Strip comments from document
      stripComments(&doc);

      // Write the result
      bool status = writeXml(root, outname, standalone);
      return status ? 0 : 1;
      
   } catch (const XmlParseException& e) {
      std::cerr << "Parse of file " << fname << " failed: " << e.what() << std::endl;
      return 1;
   } catch (const std::exception& e) {
      std::cerr << "Error processing " << fname << ": " << e.what() << std::endl;
      return 1;
   }
}
