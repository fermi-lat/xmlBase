/**
 * @file test_IFile.cpp
 * @brief Test program for IFile facility within xmlBase package.
 * 
 * Note: The IFile class is assumed to be updated to use RapidXML internally.
 * This test file uses modern C++17 features.
 */

#include "xmlBase/IFile.h"

#include "facilities/commonUtilities.h"

#include <iostream>
#include <memory>
#include <string>
#include <string_view>

// Forward declaration
void lookFor(const xmlBase::IFile& ifile, std::string_view section, std::string_view item);

int main() {
   facilities::commonUtilities::setupEnvironment();
   
   const std::string filename = facilities::commonUtilities::joinPath(
      facilities::commonUtilities::getXmlPath("xmlBase"), "myIFile.xml");
   
   auto ifile = std::make_unique<xmlBase::IFile>(filename.c_str());

   if (ifile) {
      lookFor(*ifile, "section1", "section1-val1");
      lookFor(*ifile, "section1", "section1-val2");
      lookFor(*ifile, "subsection", "subsectionItem");
      lookFor(*ifile, "section2", "subsectionItem");
      lookFor(*ifile, "section2", "bad-int");
   } else {
      std::cout << "Unable to read file " << filename << std::endl;
   }

   // Now do it again for IFile referencing schema rather than dtd
   const std::string filename2 = facilities::commonUtilities::joinPath(
      facilities::commonUtilities::getXmlPath("xmlBase"), "mySchemaIFile.xml");

   std::cout << std::endl << std::endl;
   std::cout << "And for my next trick:  process an IFile using XML Schema." << std::endl;
   std::cout << "This file should be flagged bad; it doesn't follow schema content model." 
             << std::endl << std::endl;

   auto ifile2 = std::make_unique<xmlBase::IFile>(filename2.c_str());

   if (ifile2) {
      lookFor(*ifile2, "section1", "section1-val1");
      lookFor(*ifile2, "section1", "section1-val2");
      lookFor(*ifile2, "subsection", "subsectionItem");
      lookFor(*ifile2, "section2", "subsectionItem");
      lookFor(*ifile2, "section2", "bad-int");
   } else {
      std::cout << "Unable to read file " << filename2 << std::endl;
   }
  
   return 0;
}

void lookFor(const xmlBase::IFile& ifile, std::string_view section, std::string_view item) {
   // Convert string_view to const char* for IFile interface
   const std::string sectionStr(section);
   const std::string itemStr(item);
   
   bool haveItem = ifile.contains(sectionStr.c_str(), itemStr.c_str());
   std::cout << "Item   " << item << "   in section   " << section;

   if (haveItem) {
      std::cout << " was found." << std::endl;
      
      // Try writing in various formats
      const char* charRep = ifile.getString(sectionStr.c_str(), itemStr.c_str());
      std::cout << "..as a string = " << charRep << std::endl;

      try {
         int intRep = ifile.getInt(sectionStr.c_str(), itemStr.c_str());
         std::cout << "..as an int = " << intRep << std::endl;
      } catch (const xmlBase::IFileException& /*ex*/) {
         std::cout << "**ERROR** int conversion failed " << std::endl << std::endl;
      }

      try {
         double doubleRep = ifile.getDouble(sectionStr.c_str(), itemStr.c_str());
         std::cout << "..as a double = " << doubleRep << std::endl;
      } catch (const xmlBase::IFileException& /*ex*/) {
         std::cout << " **ERROR** double conversion failed " << std::endl << std::endl;
      }
   } else {
      std::cout << " was NOT found." << std::endl;
   }
}
