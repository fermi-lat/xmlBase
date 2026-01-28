// $Header: /nfs/slac/g/glast/ground/cvs/xmlBase/xmlBase/XmlParser.h,v 1.3 2007/04/19 21:51:23 jrb Exp $
// Author:  J. Bogart

#ifndef xmlBase_XmlParser_h
#define xmlBase_XmlParser_h

#include "xmlBase/XmlErrorHandler.h"
 // following indirectly includes DOMDocument, DOMElement...
//#include <xercesc/parsers/XercesDOMParser.hpp>
#include "xmlBase/rapidxml.hpp"
#include <string>
#include <iosfwd>


namespace xmlBase {
  /// This class provides an interface to the Xerces DOM parser
  /// with validation turned on if the file to be parsed has a dtd.
  class EResolver;
  //using XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument;
  //using XERCES_CPP_NAMESPACE_QUALIFIER XercesDOMParser;
  using namespace rapidxml;

  class XmlParser {
  public:
    XmlParser(bool throwErrors = false);

    /// Call this method to turn on schema processing (else it's off)
    /// NOTE: rapidXML DOES NOT natively validate or process schemas!
    /// Disabling for now but need to see if this is something we actually need.
    void doSchema(bool doit);
    ~XmlParser();

    /** In case we want to force use of a certain schema
     Set @a ns to 'false' to set NoNamespaceSchemaLocation
    */
    void setSchemaLocation(const std::string& loc, bool ns=true);

    /// Parse an xml file, returning document node if successful
    xml_document<> parse(const char* const filename, 
                       const std::string& docType=std::string(""));


    /// Parse an xml file as a string, returning document node if successful
    xml_document<> parse(const std::string& buffer,
                       const std::string& docType=std::string("") );

    /// Reset the parser so it may be used to parse another document (note
    /// this destroys old DOM)
    void reset() {m_parser->reset();}

    void set_ErrorHandler(
  private:
    /// Xerces-supplied parser which does the real work
    //XercesDOMParser* m_parser;
    XmlErrorHandler* m_errorHandler;  // TODO: Figure out error handling for rapidXML
    /// Entity resolver
    //EResolver*       m_resolver;    // rapidXML DOES NOT have a built-in entity resolver
    bool             m_throwErrors;
    bool             m_errorsOccurred;
    bool             m_doSchema;
    //    static int       s_didInit;
  };
}
#endif
