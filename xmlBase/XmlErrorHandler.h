// $Header: /nfs/slac/g/glast/ground/cvs/xml/xml/XmlErrorHandler.h,v 1.8 2004/11/10 17:40:08 jrb Exp $
//  Author:  J. Bogart

#ifndef xmlBase_XmlErrorHandler_h
#define xmlBase_XmlErrorHandler_h

//#include <xercesc/sax/ErrorHandler.hpp>
//#include <xercesc/sax/SAXParseException.hpp>
//#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include "rapidxml.hpp"

namespace xmlBase {
  // XERCES_CPP_NAMESPACE_USE

  // //! Exception class for XmlParser, XmlErrorHandler
  // class ParseException : std::exception {
  // public:
  //   ParseException(const std::string& extraInfo = "") : std::exception(),
  //     m_name("ParseException"), m_extra(extraInfo) {}
  //   virtual ~ParseException() throw() {}
  //   virtual std::string getMsg() {
  //     std::string msg = m_name + ": " + m_extra;
  //     return msg;}
  //   virtual const char* what() {
  //     return m_extra.c_str();
  //   }
  // protected: 
  //   std::string m_name;
  // private:
  //   std::string m_extra;
  // };


  /// This class handles errors during parsing of an xml file.
  /// By default output will go to cerr, but if @a throwErrors is
  /// set to true an exception of type xml::ParseException will be
  /// thrown instead
  // class XmlErrorHandler : public XERCES_CPP_NAMESPACE_QUALIFIER ErrorHandler  {
  // public:
  //   // Constructor, destructor have little to do.
  //   XmlErrorHandler(bool throwErrors = false) : m_throwErrors(throwErrors)
  //     {resetErrors();}
  //   ~XmlErrorHandler() {}
  //   // The following methods all override pure virtual methods
  //   // from ErrorHandler base class.
  //   /// Keep count of warnings seen
  //   void warning(const SAXParseException& exception);
  //   /// Output row, column of parse error and increment counter
  //   void error(const SAXParseException& exception);
  //   /// Output row, column of fatal parse error and increment counter
  //   void fatalError(const SAXParseException& exception);
  //   /// Clear counters
  //   void resetErrors();

  //   // "get" methods
  //   int getWarningCount() const {return m_nWarning;}
  //   int getErrorCount() const {return m_nError;}
  //   int getFatalCount() const {return m_nFatal;}

  // private: 
  //   int m_nWarning;
  //   int m_nError;
  //   int m_nFatal;
  //   bool m_throwErrors;
  // };                  // end class definition

  // Base exception for all XML-related errors
  class XmlException : public std::runtime_error {
  public:
    explicit XmlException(const std::string& message, 
                          const std::string& context = "")
      : std::runtime_error(buildMessage(message, context)),
	message_(message),
	context_(context) {}

    const std::string& getContext() const { return context_; }
    const std::string& getBaseMessage() const { return message_; }

  private:
    static std::string buildMessage(const std::string& msg, 
                                    const std::string& ctx) {
      if (ctx.empty()) return msg;
      return msg + " [Context: " + ctx + "]";
    }

    std::string message_;
    std::string context_;
  };

  // Parsing-specific errors
  class XmlParseException : public XmlException {
  public:
    XmlParseException(const std::string& message,
                      size_t line = 0,
                      size_t column = 0,
                      const std::string& context = "")
      : XmlException(buildParseMessage(message, line, column), context),
	line_(line),
	column_(column) {}

    size_t getLine() const { return line_; }
    size_t getColumn() const { return column_; }

  private:
    static std::string buildParseMessage(const std::string& msg,
                                         size_t line, size_t col) {
      std::ostringstream oss;
      oss << "XML Parse Error: " << msg;
      if (line > 0) oss << " (Line: " << line << ", Column: " << col << ")";
      return oss.str();
    }

    size_t line_;
    size_t column_;
  };

  // Node/Element not found errors
  class XmlNodeNotFoundException : public XmlException {
  public:
    explicit XmlNodeNotFoundException(const std::string& nodeName,
                                      const std::string& parentPath = "")
      : XmlException("Node not found: '" + nodeName + "'", parentPath),
	nodeName_(nodeName) {}

    const std::string& getNodeName() const { return nodeName_; }

  private:
    std::string nodeName_;
  };

  // Attribute not found errors
  class XmlAttributeNotFoundException : public XmlException {
  public:
    XmlAttributeNotFoundException(const std::string& attrName,
                                  const std::string& nodeName)
      : XmlException("Attribute '" + attrName + "' not found in node '" + nodeName + "'"),
	attrName_(attrName),
	nodeName_(nodeName) {}

    const std::string& getAttributeName() const { return attrName_; }
    const std::string& getNodeName() const { return nodeName_; }

  private:
    std::string attrName_;
    std::string nodeName_;
  };

  // Type conversion errors
  class XmlTypeConversionException : public XmlException {
  public:
    XmlTypeConversionException(const std::string& value,
                               const std::string& targetType,
                               const std::string& context = "")
      : XmlException("Cannot convert '" + value + "' to " + targetType, context),
	value_(value),
	targetType_(targetType) {}

    const std::string& getValue() const { return value_; }
    const std::string& getTargetType() const { return targetType_; }

  private:
    std::string value_;
    std::string targetType_;
  };

  // Validation errors
  class XmlValidationException : public XmlException {
  public:
    explicit XmlValidationException(const std::string& message,
                                    const std::string& context = "")
      : XmlException("Validation Error: " + message, context) {}
  };
}                     // end namespace xml

#endif
