// $Header: /nfs/slac/g/glast/ground/cvs/xml/xml/IFile.h,v 1.6 2004/11/10 17:40:08 jrb Exp $
// adapted from Bonn, originally written by Ruediger Gross-Hardt
// hardy@servax.iskp.uni-bonn.de

// converted to use STL map

// User: Suson      Date: 6/29/98
// Added getIntVector, getDoubleVector

#ifndef xmlBase_IFile_h
#define xmlBase_IFile_h

#include <cstdio>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <memory>

// Include the corrected XML framework headers
#include "rapidxml.hpp"
#include "rapidxml_error_framework.hpp"
#include "xml_result.hpp"
#include "safe_xml_parser.hpp"

namespace xmlBase {

  // Use the xml_framework types
  using xml_framework::SafeXmlParser;
  using xml_framework::XmlResult;
  using xml_framework::XmlErrorCode;
  using xml_framework::XmlException;
  using xml_framework::XmlNodeNotFoundException;
  using xml_framework::XmlAttributeNotFoundException;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  class IFileException {
  public:
    IFileException(const std::string& err) : msg(err) {}
    std::string msg;
  };

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  class IFile_Item 
  {
    // basic item, should be private to IFile
    friend class IFile_Section;
    friend class IFile;
  public:
    IFile_Item() = default;
    IFile_Item(const std::string& name, const std::string& value)
      : itemname(name), itemstring(value) {}
    
    std::string& title()   { return itemname;    }
    std::string& comment() { return itemcomment; }
    std::string& mystring(){ return itemstring;  }
    
    const std::string& title()    const { return itemname;    }
    const std::string& comment()  const { return itemcomment; }
    const std::string& mystring() const { return itemstring;  }
    
  private:
    std::string itemname;
    std::string itemstring;
    std::string itemcomment;
  };


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  class IFile_Section : 
    public std::map<std::string, IFile_Item*, std::less<std::string> > 
  {
    // section, a map of items
    friend class IFile;
  public:
    IFile_Section() = default;
    explicit IFile_Section(const std::string& name) : sectionname(name) {}
    ~IFile_Section();
    
    std::string& title() { return sectionname; }
    const std::string& title() const { return sectionname; }
    
  private:
    bool contains(const std::string& name) const { 
      return find(name) != end(); 
    }
    
    IFile_Item* lookUp(const std::string& name) {
      auto it = find(name);
      return (it == end()) ? nullptr : it->second;
    }
    
    const IFile_Item* lookUp(const std::string& name) const {
      auto it = find(name);
      return (it == end()) ? nullptr : it->second;
    }
    
    std::string sectionname;
  };

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  class IFile : 
    protected std::map<std::string, IFile_Section*, std::less<std::string> > 
  {
    // IFile is a map of sections
    
  public:
    explicit IFile(const char* filename);
    explicit IFile(const rapidxml::xml_document<>* instrument); 
    explicit IFile(const rapidxml::xml_node<>* instrument);
    
    virtual ~IFile();
    
    // Disable copy
    IFile(const IFile&) = delete;
    IFile& operator=(const IFile&) = delete;
    
    // Enable move
    IFile(IFile&&) = default;
    IFile& operator=(IFile&&) = default;
    
    typedef std::vector<int> intVector;
    typedef std::vector<double> doubleVector;
    
    virtual bool contains(const char* section, const char* item);
    // check, whether section and item is contained in IFile
    
    virtual int getInt(const char* section, const char* item);
    virtual double getDouble(const char* section, const char* item);
    virtual int getBool(const char* section, const char* item);
    virtual const char* getString(const char* section, const char* item);
    virtual intVector getIntVector(const char* section, const char* item);
    virtual doubleVector getDoubleVector(const char* section, const char* item);
    // get data from [section]item
    
    virtual int getInt(const char* section, const char* item, int defValue);
    virtual double getDouble(const char* section, const char* item, double defValue);
    virtual int getBool(const char* section, const char* item, int defValue);
    virtual const char* getString(const char* section, const char* item, 
                                  const char* defValue);
    virtual intVector getIntVector(const char* section, const char* item, 
                                   intVector defValues);
    virtual doubleVector getDoubleVector(const char* section, const char* item,
                                         doubleVector defValues);
    // get data from [section]item, providing a default value, if not present
    
    void setString(const char* section, const char* item, const char* newString);
    
    void print(); // print contents to cout
    virtual void printOn(std::ostream& out = std::cout);
    // print IFile on stream
    
  protected:
    friend class IFile_Section;
    friend class IFile_Item;
    
    static void stripBlanks(char* str1, const char* str2, int flags);
    static int stricmp(const char* str1, const char* str2);

    // helper functions
    IFile() = default;
    
  private:
    void addSection(const rapidxml::xml_node<>* elt);
    void domToIni(const rapidxml::xml_document<>* doc);
    void domToIni(const rapidxml::xml_node<>* doc);
    virtual const char* _getstring(const char* section, const char* item,
                                   int failIfNotFoundFlag = 1);
    // internal function, that does the work
    
    // Storage for file content (needed because RapidXML modifies the buffer)
    std::vector<char> sourceBuffer_;
    
    // Current section being processed (used during parsing)
    IFile_Section* curSection_ = nullptr;
  };

}  // end namespace xmlBase

#endif
