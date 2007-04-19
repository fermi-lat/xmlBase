// $Header: /nfs/slac/g/glast/ground/cvs/xmlBase/src/test/test_write.cxx,v 1.2 2006/02/04 00:44:36 jrb Exp $
/// Test program for serialization of DOM, stripping of comments

#include "xmlBase/Dom.h"
#include "xmlBase/XmlParser.h"

#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMDocument.hpp>

#include <string>
#include <iostream>



int main() {
    
  // File is well-formed, no reference to dtd or schema
  std::string instanceDoc("$(XMLBASEROOT)/xml/aDocument.xml");
  std::string theSchema("$(XMLBASEROOT)/xml/theSchema.xsd");

    
  XERCES_CPP_NAMESPACE_USE
  using xmlBase::Dom;

  xmlBase::XmlParser parser;
  parser.doSchema(true);

  parser.setSchemaLocation(theSchema.c_str(), false);
  DOMDocument* doc = parser.parse(instanceDoc.c_str());
  if (!doc) {
    std::cerr << "Parse of file " << instanceDoc << " failed" << std::endl;
    return 1;
  }
  else {
    std::cout << "Parse of file " << instanceDoc << " succeeded!" 
              << std::endl;
  } 


  return 0;
}
  
  
