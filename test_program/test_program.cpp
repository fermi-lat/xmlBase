#include "safe_xml_parser.hpp"
#include <iostream>

using namespace xml_framework;

void exampleWithExceptions() {
    const char* xmlData = R"(
        <?xml version="1.0"?>
        <config>
            <server port="8080" enabled="true">
                <name>MainServer</name>
                <timeout>30</timeout>
            </server>
            <database>
                <connection>localhost:5432</connection>
            </database>
        </config>
    )";

    // Copy for parsing (RapidXML modifies input)
    std::vector<char> buffer(xmlData, xmlData + strlen(xmlData) + 1);

    try {
        rapidxml::xml_document<> doc;
        SafeXmlParser::parseString(doc, buffer.data());

        // Get required nodes (throws if not found)
        auto* root = SafeXmlParser::getRequiredNode(&doc, "config");
        auto* server = SafeXmlParser::getRequiredNode(root, "server");

        // Get attribute with type conversion
        auto* portAttr = SafeXmlParser::getRequiredAttribute(server, "port");
        auto portResult = SafeXmlParser::parseValue<int>(portAttr->value());
        if (portResult.isSuccess()) {
            std::cout << "Port: " << portResult.value() << std::endl;
        }

        // Iterate children
        SafeXmlParser::forEachChild(root, "server", [](auto* node) {
            std::cout << "Found server: " << node->first_node("name")->value() << std::endl;
        });

    } catch (const XmlParseException& e) {
        std::cerr << "Parse error at line " << e.getLine() 
                  << ": " << e.what() << std::endl;
    } catch (const XmlNodeNotFoundException& e) {
        std::cerr << "Missing node: " << e.getNodeName() << std::endl;
    } catch (const XmlException& e) {
        std::cerr << "XML error: " << e.what() << std::endl;
    }
}

void exampleWithResults() {
    const char* xmlData = R"(
        <config>
            <value>42</value>
            <enabled>true</enabled>
        </config>
    )";

    std::vector<char> buffer(xmlData, xmlData + strlen(xmlData) + 1);
    rapidxml::xml_document<> doc;

    // Non-throwing parse
    auto parseResult = SafeXmlParser::tryParseString(doc, buffer.data());
    if (parseResult.isError()) {
        std::cerr << parseResult.error().toString() << std::endl;
        return;
    }

    // Chain operations with Result type
    auto valueResult = SafeXmlParser::tryGetNode(&doc, "config")
        .map([](auto* node) { return node->first_node("value"); });

    if (valueResult.isSuccess() && valueResult.value()) {
        auto intResult = SafeXmlParser::getNodeValue<int>(valueResult.value());
        std::cout << "Value: " << intResult.valueOr(-1) << std::endl;
    }

    // Using optional for truly optional nodes
    auto optionalNode = SafeXmlParser::getOptionalNode(
        doc.first_node("config"), "optional_setting"
    );
    
    if (optionalNode.has_value()) {
        std::cout << "Optional setting found!" << std::endl;
    } else {
        std::cout << "Optional setting not present (this is OK)" << std::endl;
    }
}

int main() {
    std::cout << "=== Exception-based Example ===" << std::endl;
    exampleWithExceptions();

    std::cout << "\n=== Result-based Example ===" << std::endl;
    exampleWithResults();

    return 0;
}
