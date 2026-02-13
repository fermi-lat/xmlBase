/// Test program for xmlBase facility.  Parse Axml file and optionally
/// write it out to a stream.

#include "xmlbase/safe_xml_parser.hpp"
#include "xml_printer.hpp"
#include "facilities/Util.h"
#include "facilities/commonUtilities.h"

#include <cstring>
#include <iostream>
#include <sstream>
//#include <fstream>

/**
   Two arguments may be supplied for input and output file.
     * First argument defaults to $XMLBASEROOT/xml/test.xml
     * Second argument defaults to no output file at all.  If supplied,
       the value of  "-" is taken to mean standard output.  Anything
       else is assumed to be a filename.
*/

using namespace xml_framework;

int main(int argc, char* argv[]) {
  facilities::commonUtilities::setupEnvironment();
  std::string infile;
  if (argc < 2) { 
    infile=facilities::commonUtilities::joinPath(facilities::commonUtilities::getXmlPath("xmlbase"), "test.xml");
  }
  else {
    infile = std::string(argv[1]);
  }
    
  facilities::Util::expandEnvVar(&infile);
 
  xml_framework::SafeXmlParser* parser = new xml_framework::SafeXmlParser();

  std::vector<char> buffer;
  rapidxml::xml_document<> doc;

  int load_flag = 0;
  
  try {
    XmlResult<std::vector<char>> parse_result = parser->loadFile(infile);
    if (parse_result.isSuccess() !=0) {
      load_flag = 1;
      std::cout << "Document successfully parsed" << std::endl;
      buffer = parse_result.value();
    }
  }
  catch (const XmlParseException& e) {
    std::cout << "caught exception with message " << std::endl;
    std::cout << e.what() << std::endl;
    delete parser;
    return 0;
  }

  //TODO: Figure out best way to encapsulate in try/catch for testing
  parser->parseString(doc, buffer.data()); // sets xml_doc<> doc to parsed data
  
  if (load_flag != 0) {  // successful
    rapidxml::xml_node<>* docRoot = doc.first_node();
    rapidxml::xml_node<>* nameChild = parser->tryGetNode(docRoot, "ChildWithAttributes").value();

    double doubleVal;
    int    intVal;

    try {
      XmlResult<int> intResult = parser->getAttributeValue<int>(nameChild, "goodInt"); // Get Specific attribute
      intVal = intResult.value();
      std::cout << "goodInt value was " << intVal << std::endl << std::endl;
    }
    catch (const XmlException& e) {
      std::cout << std::endl << "XmlException:  " << e.what() 
                << std::endl << std::endl;
    }
      
    // try {
    //   std::vector<int> ints;
    //   XmlResult<std::vector<int>> intsResult = parser->getAttributeValue<std::vector<int>>(nameChild, "goodInts");
    //   ints = intsResult.value();
    //   unsigned nInts = ints.size();
    //   std::cout << "Found " << nInts << " goodInts:  " <<  std::endl;
    //   for (unsigned iInt=0; iInt < nInts; iInt++) {
    //     std::cout << ints[iInt] << "  ";
    //   }
    //   std::cout << std::endl << std::endl;
    // }
    // catch (const XmlException& e) {
    //   std::cout << std::endl << "XmlException:  " << e.what()
    //             << std::endl << std::endl;
    // }

    // try {
    //   std::vector<double> doubles;
    //   XmlResult<std::vector<double>> doublesResult = parser->getAttributeValue<std::vector<double>>(nameChild, "goodDoubles");
    //   doubles = doublesResult.value();
    //   unsigned nD = doubles.size();
    //   std::cout << "Found " << nD << " goodDoubles:  " <<  std::endl;
    //   for (unsigned iD=0; iD < nD; iD++) {
    //     std::cout << doubles[iD] << "  ";
    //   }
    //   std::cout << std::endl << std::endl;
    // }
    // catch (const XmlException& e) {
    //   std::cout << std::endl << "XmlException:  " << e.what()
    //             << std::endl << std::endl;
    // }

    try {
      XmlResult<int> intResult = parser->getAttributeValue<int>(nameChild, "badInt");
      intVal = intResult.value();
      std::cout << "badInt value was " << intVal << std::endl << std::endl;
    }
    catch (const XmlException& e) {
      std::cout << std::endl << "XmlException:  " << e.what()
                << std::endl << std::endl;
    }
      
    try {
      XmlResult<double> doubleResult = parser->getAttributeValue<double>(nameChild, "goodDouble");
      doubleVal = doubleResult.value();
      std::cout << "goodDouble value was " << doubleVal 
                << std::endl << std::endl;
    }
    catch (const XmlException& e) {
      std::cout << std::endl << "XmlException:  " << e.what()
                << std::endl << std::endl;
    }

    try {
      XmlResult<double> doubleResult = parser->getAttributeValue<double>(nameChild, "badDouble");
      if (doubleResult.isError() != 1) {
	doubleVal = doubleResult.value();
	std::cout << std::endl << "badDouble value was " << doubleVal 
		  << std::endl << std::endl;
      }
      else {
	std::cout << std::endl << "Error parsing badDouble: " << doubleResult.error().toString()
		  << std::endl << std::endl;
      }
    }
    catch (const XmlException& e) {
      std::cout << std::endl << "XmlException:  " << e.what()
                << std::endl << std::endl;
    }

    // try {
    //   std::vector<double> doubles;
    //   XmlResult<std::vector<double>> doublesResult = parser->getAttributeValue<std::vector<double>>(nameChild, "badDoubles");
    //   doubles = doublesResult.value();
    //   unsigned nD = doubles.size();
    //   std::cout << "Found " << nD << " badDoubles:  " <<  std::endl;
    //   for (unsigned iD=0; iD < nD; iD++) {
    //     std::cout << doubles[iD] << "  ";
    //   }
    //   std::cout << std::endl << std::endl;
    // }
    // catch (const XmlException& e) {
    //   std::cout << std::endl << "XmlException:  " << e.what()
    //             << std::endl << std::endl;
    // }

    if (argc > 2) { // attempt to output
      const char  *hyphen = "-";

      std::ostream* out;
      std::ofstream* fileStream = nullptr; // Allows tracking if file stream opened

      if (*(argv[2]) == *hyphen) {
        out = &std::cout;
      }
      else {   // try to open file as ostream
        char *filename = argv[2];
	fileStream = new std::ofstream(filename);
	if (!fileStream->is_open()) {
	  std::cerr << "Failed to open file: " << filename << std::endl;
	  delete fileStream;
	  return 1;
	}
	out = fileStream;
      }
      *out << "Document source: " << std::string(argv[1]) << std::endl;
      *out << std::endl << "Straight print of document:" << std::endl;
      XmlPrinter::printElement(nameChild, *out);
      *out << std::endl << std::endl << "Add indentation and line breaks:" 
           << std::endl;
      XmlPrinter::prettyPrintElement(nameChild, *out, "");

      // If writing to file, clean up and flush the buffer
      if (fileStream) {
	fileStream->flush();
	fileStream->close();
	delete fileStream;
      }
    }
  }
  delete parser;
  return(0);
}
