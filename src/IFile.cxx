// $Header: /nfs/slac/g/glast/ground/cvs/xmlBase/src/IFile.cxx,v 1.1.1.1 2004/12/29 22:36:26 jrb Exp $

#include "xmlBase/IFile.h"
#include "facilities/Util.h"  // for expandEnvVar

#include <sstream>
#include <vector>
#include <cstdio>
#include <cstring>
#include <string>
#include <cctype>
#include <cstdlib>
#include <fstream>

#define FATAL_MACRO(output) do { std::cerr << output << std::endl; throw(IFileException(output)); } while(0)

namespace xmlBase {

#define LEADING  1
#define ALL      2
#define TRAILING 4

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  void IFile::printOn(std::ostream& out) {
    for (auto section_map = begin(); section_map != end(); ++section_map) {
      IFile_Section& section = *section_map->second;
      out << "\n[" << section_map->first << "]\n";
      
      for (auto item_map = section.begin(); item_map != section.end(); ++item_map) {
        IFile_Item& item = *item_map->second;
        out << item_map->first << " = " << item.mystring() << "\n";
      }
    }
  }
  
  void IFile::print() {
    printOn(std::cout);
    std::cout.flush();
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  IFile_Section::~IFile_Section() {
    for (auto it = begin(); it != end(); ++it) {
      delete it->second;
    }
    clear();
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  IFile::~IFile() {
    for (auto it = begin(); it != end(); ++it) {
      delete it->second;
    }
    clear();
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  IFile::IFile(const char* filename) : curSection_(nullptr) {
    // Expand environment variables in filename
    std::string fname(filename);
    facilities::Util::expandEnvVar(&fname);
    
    // Load file into buffer
    std::ifstream file(fname, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      std::string errorString = "IFile cannot open file: " + fname;
      FATAL_MACRO(errorString);
    }
    
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    sourceBuffer_.resize(static_cast<size_t>(size) + 1);
    if (!file.read(sourceBuffer_.data(), size)) {
      std::string errorString = "IFile cannot read file: " + fname;
      FATAL_MACRO(errorString);
    }
    sourceBuffer_[static_cast<size_t>(size)] = '\0';
    file.close();
    
    // Parse the XML
    rapidxml::xml_document<> doc;
    try {
      doc.parse<rapidxml::parse_default>(sourceBuffer_.data());
    } catch (const rapidxml::parse_error& e) {
      std::string errorString = std::string("XML Parse Error in file ") + fname + ": " + e.what();
      FATAL_MACRO(errorString);
    }
    
    domToIni(&doc);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  IFile::IFile(const rapidxml::xml_document<>* instrument) : curSection_(nullptr) {
    domToIni(instrument);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  IFile::IFile(const rapidxml::xml_node<>* instrument) : curSection_(nullptr) {
    domToIni(instrument);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  void IFile::domToIni(const rapidxml::xml_document<>* doc) {
    // Find the root element
    rapidxml::xml_node<>* root = doc->first_node();
    
    // Skip declaration and other non-element nodes
    while (root && root->type() != rapidxml::node_element) {
      root = root->next_sibling();
    }
    
    if (!root) {
      FATAL_MACRO("IFile: Document has no root element");
    }
    
    domToIni(root);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  void IFile::domToIni(const rapidxml::xml_node<>* doc) {
    // Get all child nodes using SafeXmlParser
    auto children = SafeXmlParser::getChildren(
      const_cast<rapidxml::xml_node<>*>(doc), nullptr);
    
    for (auto* child : children) {
      if (child->type() != rapidxml::node_element) continue;
      
      std::string tagName = child->name() ? child->name() : "";
      
      if (tagName == "section") {
        addSection(child);
      }
      else {
        std::string errorString = "unexpected tag in initialization: " + tagName;
        FATAL_MACRO(errorString);
      }
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  void IFile::addSection(const rapidxml::xml_node<>* elt) {
    // Get section name attribute
    auto nameResult = SafeXmlParser::getAttributeValue<std::string>(elt, "name");
    if (nameResult.isError()) {
      FATAL_MACRO("IFile: section element missing 'name' attribute");
    }
    std::string sectionName = nameResult.value();
    
    // Create new section
    curSection_ = new IFile_Section(sectionName);
    (*this)[sectionName] = curSection_;
    
    // Process children (items and nested sections)
    auto children = SafeXmlParser::getChildren(
      const_cast<rapidxml::xml_node<>*>(elt), nullptr);
    
    for (auto* child : children) {
      if (child->type() != rapidxml::node_element) continue;
      
      std::string tagName = child->name() ? child->name() : "";
      
      if (tagName == "section") {
        addSection(child);
      }
      else if (tagName == "item") {
        auto itemNameResult = SafeXmlParser::getAttributeValue<std::string>(child, "name");
        auto itemValueResult = SafeXmlParser::getAttributeValue<std::string>(child, "value");
        
        if (itemNameResult.isError()) {
          FATAL_MACRO("IFile: item element missing 'name' attribute");
        }
        if (itemValueResult.isError()) {
          FATAL_MACRO("IFile: item element missing 'value' attribute");
        }
        
        std::string itemName = itemNameResult.value();
        std::string itemValue = itemValueResult.value();
        
        // Make the new item
        IFile_Item* newItem = new IFile_Item(itemName, itemValue);
        
        // Add it to the section map
        (*curSection_)[newItem->title()] = newItem;
      }
      else {
        std::string errorString = "unexpected tag in initialization: " + tagName;
        FATAL_MACRO(errorString);
      }
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  bool IFile::contains(const char* section, const char* item) {
    return (_getstring(section, item, 0) != nullptr);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char* IFile::_getstring(const char* sectionname, const char* itemname, 
                                int failFlag) {
    char hitem[1000], hsection[1000];
    IFile_Item* item = nullptr;
    IFile_Section* section = nullptr;
    
    stripBlanks(hitem, itemname, ALL);
    stripBlanks(hsection, sectionname, ALL);
    
    auto entry = find(std::string(hsection));
    
    if (entry != end()) {
      section = entry->second;
      
      auto it = section->find(std::string(hitem));
      item = (it != section->end()) ? it->second : nullptr;
    }
    
    if (item != nullptr) {
#ifdef DEBUG
      std::cout << "getstring: [" << hsection << "]" << hitem << ": ->" 
                << item->mystring() << "<-" << std::endl;
#endif
      return item->mystring().c_str();
    }
    else if (failFlag) {
      if (section == nullptr) {
        std::string errorString =
          std::string("cannot find section [") + sectionname + "]";
        FATAL_MACRO(errorString);
      }
      else {
        std::string errorString =
          std::string("cannot find item '") + itemname + "' in section [" 
          + sectionname + "]";
        FATAL_MACRO(errorString);
      }
      return nullptr;
    }
    else {
      return nullptr;
    }
  }

  // getting data from [section]item, exiting when not found
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char* IFile::getString(const char* section, const char* item) {
    return _getstring(section, item);
  }

  // setting data in [section]
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  void IFile::setString(const char* sectionname, const char* itemname, 
                        const char* newString) {
    char hitem[1000], hsection[1000];
    IFile_Item* item = nullptr;
    IFile_Section* section = nullptr;
    
    stripBlanks(hitem, itemname, ALL);
    stripBlanks(hsection, sectionname, ALL);
    
    auto it = find(std::string(hsection));
    
    if (it != end()) {
      section = it->second;
      
      if (section->contains(hitem)) {
        item = section->lookUp(hitem);
      }
    }
    
    if (item) {
      item->mystring() = newString;
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  double IFile::getDouble(const char* section, const char* item) {
    std::string hilf(_getstring(section, item));
    
    try {
      return facilities::Util::stringToDouble(hilf);
    }
    catch (facilities::WrongType& ex) {
      std::cerr << "from xmlBase::IFile::getDouble  " << std::endl;
      std::cerr << ex.getMsg() << std::endl;
      throw(IFileException(" "));
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int IFile::getInt(const char* section, const char* item) {
    std::string hilf(_getstring(section, item));
    
    try {
      return facilities::Util::stringToInt(hilf);
    }
    catch (facilities::WrongType& ex) {
      std::cerr << "from xmlBase::IFile::getInt  " << std::endl;
      std::cerr << ex.getMsg() << std::endl;
      throw(IFileException(" "));
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int IFile::getBool(const char* section, const char* item) {
    std::string hilf(_getstring(section, item));
    
    if (hilf == "yes" || hilf == "true" || hilf == "1") {
      return 1;
    }
    else if (hilf == "no" || hilf == "false" || hilf == "0") {
      return 0;
    }
    else {
      std::string errorString = "[" + std::string(section) + "]" + item 
        + " = '" + hilf + "' is not boolean";
      FATAL_MACRO(errorString);
      return 0;
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  IFile::intVector IFile::getIntVector(const char* section, const char* item) {
    intVector iv;
    char buffer[1024];
    
    std::strncpy(buffer, _getstring(section, item), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    if (std::strlen(buffer) >= sizeof(buffer) - 1) {
      FATAL_MACRO("string returned from _getstring is too long");
      return iv;
    }
    
    char* vString = std::strtok(buffer, "}");
    vString = std::strtok(buffer, "{");
    
    char* test = std::strtok(vString, ",");
    while (test != nullptr) {
      iv.push_back(std::atoi(test));
      test = std::strtok(nullptr, ",");
    }
    
    if (iv.empty()) {
      std::string hilf(buffer);
      std::string errorString = "[" + std::string(section) + "]" + item 
        + " = '" + hilf + "' is not an integer vector";
      FATAL_MACRO(errorString);
    }
    
    return iv;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  IFile::doubleVector IFile::getDoubleVector(const char* section, 
                                              const char* item) {
    doubleVector dv;
    char buffer[1024];
    
    std::strncpy(buffer, _getstring(section, item), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    if (std::strlen(buffer) >= sizeof(buffer) - 1) {
      FATAL_MACRO("string from _getstring() too long");
      return dv;
    }
    
    char* vString = std::strtok(buffer, "}");
    vString = std::strtok(buffer, "{");
    
    char* test = std::strtok(vString, ",");
    while (test != nullptr) {
      dv.push_back(std::atof(test));
      test = std::strtok(nullptr, ",");
    }
    
    if (dv.empty()) {
      std::string hilf(buffer);
      std::string errorString = "[" + std::string(section) + "]" + item 
        + " = '" + hilf + "' is not a double vector";
      FATAL_MACRO(errorString);
    }
    
    return dv;
  }

  // getting data from [section]item, with default values provided
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int IFile::getInt(const char* section, const char* item, int defValue) {
    return contains(section, item) ? getInt(section, item) : defValue;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int IFile::getBool(const char* section, const char* item, int defValue) {
    return contains(section, item) ? getBool(section, item) : defValue;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  double IFile::getDouble(const char* section, const char* item, 
                          double defValue) {
    return contains(section, item) ? getDouble(section, item) : defValue;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char* IFile::getString(const char* section, const char* item, 
                                const char* defValue) {
    return contains(section, item) ? getString(section, item) : defValue;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  IFile::intVector IFile::getIntVector(const char* section, const char* item, 
                                        intVector defValues) {
    return contains(section, item) ? getIntVector(section, item) : defValues;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  IFile::doubleVector IFile::getDoubleVector(const char* section, 
                                              const char* item, 
                                              doubleVector defValues) {
    return contains(section, item) ? getDoubleVector(section, item) : defValues;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Strip blanks from string
  void IFile::stripBlanks(char* str1, const char* str2, int flags) {
    const char* p = str2;
    char* q = str1;
    
    // Skip leading blanks if requested
    if (flags & LEADING) {
      while (*p && std::isspace(static_cast<unsigned char>(*p))) {
        ++p;
      }
    }
    
    // Copy the string
    while (*p) {
      *q++ = *p++;
    }
    *q = '\0';
    
    // Strip trailing blanks if requested
    if (flags & TRAILING) {
      --q;
      while (q >= str1 && std::isspace(static_cast<unsigned char>(*q))) {
        *q-- = '\0';
      }
    }
    
    // Strip all blanks if requested
    if (flags & ALL) {
      p = str1;
      q = str1;
      while (*p) {
        if (!std::isspace(static_cast<unsigned char>(*p))) {
          *q++ = *p;
        }
        ++p;
      }
      *q = '\0';
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Case-insensitive string comparison
  int IFile::stricmp(const char* str1, const char* str2) {
    while (*str1 && *str2) {
      int diff = std::tolower(static_cast<unsigned char>(*str1)) 
               - std::tolower(static_cast<unsigned char>(*str2));
      if (diff != 0) return diff;
      ++str1;
      ++str2;
    }
    return std::tolower(static_cast<unsigned char>(*str1)) 
         - std::tolower(static_cast<unsigned char>(*str2));
  }

} // end namespace xmlBase
