---
package: xmlBase
summary: XML layer of the Fermitools; a vendored, header-only RapidXML 1.13 plus header-only helpers (SafeXmlParser, XmlDomBuilder, XmlPrinter, XmlResult) and a small compiled IFile reader.
layer: foundation
depends_on: [facilities]
used_by: [Likelihood, optimizers, flux, pyExposure]
last_reviewed_commit: acd03a2
---

# xmlBase Architecture

**xmlBase** gives the Fermitools their XML parsing and writing. Until the
`xerces-replacement` merge (PR #8, head commit `acd03a2`) it wrapped
Xerces-C. It now consists of:

1. **RapidXML 1.13** (`xmlBase/rapidxml.hpp`), vendored as a single header.
2. **Header-only helpers** in namespace `xml_framework`, written for the
   Xerces→RapidXML migration: `SafeXmlParser` (load, parse, typed attribute
   access), `XmlDomBuilder`/`XmlElementBuilder` (build documents), `XmlPrinter`
   (serialize), `XmlResult<T>` (value-or-error) and an exception hierarchy.
3. **`xmlBase::XmlParser`** (header-only): a small "Xerces-like" facade.
4. **`xmlBase::IFile`**: the only compiled code (`src/IFile.cxx`). It reads
   an INI-style `<section>/<item>` XML file. **No other package uses it.**

So almost everything consumers use is header-only. The shared library
`libxmlBase` contains just `IFile`.

The Doxygen mainpage (`src/mainpage.h`) and `doc/release.notes` still
describe the Xerces-based package (DOM, `Dom` class, schema validation,
`EResolver`, `DocMan`). **None of that exists any more.** Use this document
and the headers instead.

## Purpose and scope

**Does:**
- Parse XML files or strings in place into a RapidXML DOM, and report parse
  errors with a line and column (`SafeXmlParser`).
- Give typed access to attributes and text (`int`, `double`, `bool`,
  `std::string`) through `XmlResult<T>`, or through exceptions.
- Build new documents and write them to streams or files, pretty-printed,
  with escaping (`XmlDomBuilder`, `XmlPrinter`).
- Read `IFile` configuration files, expanding `$(VAR)` in the file name.

**Does not:**
- **Validate** against DTDs or XML Schemas. `DOCTYPE` declarations and
  `xsi:schemaLocation` attributes are ignored. Several test files and
  comments still expect validation errors that are no longer raised.
- Expand DTD entities or external entities (`<!ENTITY x SYSTEM "file.xml">`
  and `&x;`). RapidXML translates only the five predefined entities and
  numeric character references; other `&name;` references are left in the
  text as-is.
- Handle XML namespaces, XPath or XInclude.
- Know about Fermi file contents. Source-model and function XML formats
  belong to `Likelihood`, `optimizers` and `flux`.

## Position in the system

| Direction | Package | Why |
|-----------|---------|-----|
| Depends on | `facilities` | Linked `PUBLIC`. Used only by `src/IFile.cxx` (`Util::expandEnvVar`, `Util::stringToDouble`/`stringToInt`) and the tests (`commonUtilities::getXmlPath`, `setupEnvironment`). The headers consumers use do not need it. |
| Used by | `Likelihood` | `rapidxml.hpp` in public headers (`SourceModel.h`, `SourceFactory.h`, `FluxBuilder.h`, `SourceModelBuilder.h`, `SpatialFunction.h`, `XmlBuilder.h`); `safe_xml_parser.hpp` (`Likelihood/XmlParser.h` singleton, `SourceFactory.cxx`, `gtsrcprob`); `xml_printer.hpp` (`SourceModelBuilder.cxx`); `xmlBase/XmlParser.h` (`gtdiffrsp`, in `diffuseResponses.cxx`). |
| Used by | `optimizers` | `rapidxml.hpp` in public headers (`optimizers/Dom.h`, `Parameter.h`); `xml_builder.h` with `XmlDomBuilder` (`FunctionFactory.cxx`, `Dom.cxx`); `safe_xml_parser.hpp`, `xml_printer.hpp`. |
| Used by | `flux` | `flux/FluxMgr.h` (public) includes `rapidxml.hpp`, `rapidxml_error_framework.hpp`, `safe_xml_parser.hpp`, `xml_result.hpp`; `FluxSource.cxx`, `SimpleSpectrum.cxx` use the raw DOM. |
| Used by | `pyExposure` | `rapidxml.hpp` only (`gtexposure.cxx`, which has its own small helpers). |

`celestialSources`, `observationSim` and others get RapidXML types
indirectly through the public headers of `flux`, `optimizers` and
`Likelihood`. Because those headers expose `rapidxml::xml_node<>*`
in their APIs, RapidXML is effectively part of the public API of all three.

Nothing in `src/` includes `xmlBase/IFile.h` outside xmlBase itself, so the
compiled library is linked by four packages but used by none (verified by
grep over `src/*`).

## Key abstractions

| Type | Header | Status | Role |
|------|--------|--------|------|
| `rapidxml::xml_document<>`, `xml_node<>`, `xml_attribute<>` | `rapidxml.hpp` | used | Upstream RapidXML 1.13 DOM. Parsing is in place and destructive. Node and attribute strings point into the source buffer or into the document's memory pool. |
| `xml_framework::SafeXmlParser` | `safe_xml_parser.hpp` | used | All-static: `loadFile` (→ `XmlResult<std::vector<char>>`), `parseString`/`tryParseString`, `getRequiredNode`/`tryGetNode`/`getOptionalNode`, `getRequiredAttribute`/`tryGetAttribute`, `getAttributeValue<T>`, `getNodeValue<T>`, `forEachChild`, `getChildren`, and the `parseValue<T>` specializations. |
| `xml_framework::XmlResult<T>`, `XmlError`, `XmlErrorCode` | `xml_result.hpp` | used | A `std::variant<T, XmlError>`, with `isSuccess()`, `value()`, `error()`, `valueOr()` and `map()`. `XmlResult<void>` is specialized. |
| `xml_framework::XmlException` and subclasses (`XmlParseException`, `XmlNodeNotFoundException`, `XmlAttributeNotFoundException`, `XmlTypeConversionException`, `XmlValidationException`) | `rapidxml_error_framework.hpp` | used | Exception hierarchy derived from `std::runtime_error`. |
| `xml_framework::XmlDomBuilder`, `XmlElementBuilder`, `ValueConverter` | `xml_builder.h` | used (optimizers) | Owns a document plus its source buffer. Chaining API: `createDocument().addDeclaration()`, `createRoot(name).addAttribute(..).addElement(..)`, `loadFile`, `parseString`, `saveToFile`, `toString`. |
| `xml_framework::XmlPrinter`, `XmlPrintOptions` | `xml_printer.hpp` | used | Serializer: `printElement`, `prettyPrintElement`, `toString`, `printToFile`, and `try*` variants. Options cover indent, attribute sorting, escaping, the declaration, empty-element style and whitespace trimming. |
| `xmlBase::XmlParser` | `XmlParser.h` | used (Likelihood `gtdiffrsp`) | Owns a buffer and document. `parse(file)` returns a document pointer or `nullptr`. Static helpers include `collectChildren`, `getAttribute`, `getDoubleAttribute`, `getFirstChildElement` and `getSiblingElement`. |
| `xmlBase::IFile`, `IFile_Section`, `IFile_Item`, `IFileException` | `IFile.h` | compiled, **unused** | INI-style reader (see below). |

**Dead headers and sources.** These are installed but not compiled, and
they cannot compile:
- `EResolver.h` and `docMan/DocMan.h`, `docMan/DocClient.h`: these include
  Xerces headers or use Xerces `DomElement`.
- `RapidXmlWrapper.h`: an older draft of `SafeXmlParser`, in the same
  namespace `xml_framework` (so including it next to `safe_xml_parser.hpp`
  redefines the class). It contains a typo (`XmlResult<t>`), and it
  refers to exception types it never declares.
- `XmlErrorHandler.h`: a second copy of the exception hierarchy, in
  namespace `xmlBase`. It is used only by `RapidXmlWrapper.h`, and it
  clashes with the `using xml_framework::XmlException` declarations in
  `IFile.h`.
- `src/XmlParser.cxx`, `src/EResolver.cxx`, `src/XmlErrorHandler.cxx`,
  `src/docMan/DocMan.cxx`: these include the removed `xmlBase/Dom.h`.
  `XmlParser.cxx` is a partial, non-compiling port.
- `src/test/test_altSchema.cxx` (not in CMake; it needs files that are not
  in `xml/`), `expat/` (fragments of expat C sources), `xmlChangeLog`
  (2004–2006 history of the ancestor `xml` package).
- `test_xmlbase/`: a scratch macOS build of a prototype, **with committed
  binaries** (`*.o`, `*.a`, `xmlbase_test`, `*.dSYM`) and private copies of
  the headers and of facilities `Util`/`commonUtilities`. It is not part
  of the build.

## Data flow and control flow

### Reading a document (the idiomatic pattern)

```cpp
#include "xmlBase/safe_xml_parser.hpp"
using namespace xml_framework;

auto loaded = SafeXmlParser::loadFile(path);           // reads file + appends '\0'
if (loaded.isError()) throw std::runtime_error(loaded.error().toString());
std::vector<char> buffer = std::move(loaded).value();   // must outlive doc and every node pointer

rapidxml::xml_document<> doc;
SafeXmlParser::parseString(doc, buffer.data());         // in place; throws XmlParseException(line, col)
rapidxml::xml_node<> * root = doc.first_node();

for (auto * src : SafeXmlParser::getChildren(root, "source")) {
   std::string name = SafeXmlParser::getAttributeValue<std::string>(src, "name").valueOr("");
   double scale = SafeXmlParser::getAttributeValue<double>(src, "scale").valueOr(1.0);
}
```

`XmlDomBuilder::loadFile`/`tryLoadFile` wraps the same steps and keeps the
buffer and document together; optimizers uses it this way
(`FunctionFactory::readXml`). `xmlBase::XmlParser::parse` does the same and
returns `nullptr` on any error, without a message.

### Writing a document

```cpp
#include "xmlBase/xml_builder.h"
xml_framework::XmlDomBuilder b;
b.createDocument().addDeclaration();
auto lib = b.createRoot("source_library");
lib.addAttribute("title", "source library");
lib.addElement("source").addAttribute("name", "Crab").addAttribute("type", "PointSource");
b.saveToFile("model.xml");      // XmlPrinter with default XmlPrintOptions
```

Builder methods copy names and values into the document's pool
(`allocate_string`), so passing temporaries is safe. Code that uses raw
RapidXML (`doc.allocate_node(node_element, name)`) must allocate the
strings itself; see `src/test/test_write.cxx`.

### IFile

`IFile(filename)` → `Util::expandEnvVar` on the name → read the file →
`parse<parse_default>` → `domToIni`. Each `<section name=...>` becomes an
`IFile_Section` map, keyed by name, in a single flat map. Each
`<item name=... value=...>` becomes an `IFile_Item`. Text and comments are
ignored. Getters strip blanks from the section and item names, then look
them up. `getInt`/`getDouble` use `facilities::Util`, and `getBool` accepts
`yes/true/1/no/false/0`.

## Public interfaces

### C++ API

Include paths are `xmlBase/<header>`. The installed headers include each
other with plain quotes (`#include "rapidxml.hpp"`), which works because
they sit in one directory. All of them need **C++17** (`<variant>`,
`<optional>`, `<charconv>`, `<filesystem>`, structured bindings). The
top-level build forces `CMAKE_CXX_STANDARD 17`.

Recommended headers for new code: `safe_xml_parser.hpp` (read),
`xml_builder.h` (write), `xml_printer.hpp` (serialize an existing DOM).
Avoid adding new uses of `XmlParser.h`, which is a migration shim, and of
`IFile.h`.

### Applications

None. Six test programs are installed to `bin/` (see below).

### Python

None.

### File formats

- Any well-formed XML, read without validation.
- `IFile` format: root element (any name) → `<section name>` (nestable) →
  `<item name value>`. The DTD and schema, `xml/ifile.dtd` and
  `xml/ifile.xsd`, are installed but not enforced.
- `xml/` (installed to `share/fermitools/xml/fermi/xmlBase`, found at run
  time through `getXmlPath("xmlBase")` or `$(XMLBASEXMLPATH)`) contains
  only test inputs.

## Build and test

- **CMake target:** `xmlBase` (shared library, `src/IFile.cxx` only,
  `PUBLIC facilities`). Headers are installed to
  `include/fermitools/xmlBase`, **including the dead Xerces-era headers**.
  `xml/` is installed to `${FERMI_INSTALL_XMLDIR}/xmlBase`.
- **Test executables** (`add_executable`, installed to `bin/`, **not
  registered with CTest**; run them by hand):

  | Executable | Source | What it does | Input |
  |------------|--------|--------------|-------|
  | `test_xmlBase` | `src/main.cxx` | Parses and prints a file. An optional second argument writes it out (`-` for stdout). | `argv[1]`, else `getXmlPath("xmlBase")/test.xml` |
  | `entity_test` | `src/test/entity_test.cxx` | Walks `simpleDoc.xml`. The external entity is **not** expanded. | `argv[1]`, else `simpleDoc.xml` |
  | `test_parse` | `src/test/test_parse.cxx` | `loadFile` plus `parseString` | `argv[1]`, else `$(XMLBASEXMLPATH)/simpleDoc.xml` |
  | `test_mem` | `src/test/test_mem.cxx` | Parses an in-memory string | — |
  | `test_IFile` | `src/test/test_IFile.cxx` | IFile lookups on `myIFile.xml` and `mySchemaIFile.xml` | XML dir |
  | `test_write` | `src/test/test_write.cxx` | Builds a document with raw RapidXML and writes it | writes **`myDoc.xml` in the current directory** |

  All call `commonUtilities::setupEnvironment()`, so `FERMI_INST_DIR` must
  point at the install prefix (`$CONDA_PREFIX/share/fermitools` in a conda
  environment) and `xmlBase` must be installed. Most of them print results
  rather than check them. Treat a non-zero exit as a failure, and read the
  output.
- **Real coverage** of the helpers comes from the consumers' tests:
  `test_optimizers`, Likelihood's `test_Likelihood` (XML model round-trips
  via `XmlDiff`) and the flux tests.
- **Legacy:** `SConscript`, `xmlBaseLib.py`, `.cvsignore`.
- **CI:** `azure-pipelines.yml` (shared `fermi-lat/ci` templates).

## Invariants and gotchas

**Buffer and document lifetime**
- RapidXML parses **in place**: it writes `'\0'` terminators and decoded
  entities into your buffer, and every `name()`/`value()` points into it.
  The buffer must be mutable and NUL-terminated, and it must outlive the
  document and every node pointer taken from it. A temporary
  `std::string`/`vector` is the classic bug.
- A buffer that has already been parsed cannot be parsed again: its
  contents have been modified.
- `XmlDomBuilder`'s move constructor and move assignment **re-parse the
  moved source buffer** instead of moving the DOM, because
  `xml_document` is not movable. That buffer has already been parsed in
  place, so the result is likely truncated or wrong. Any nodes added
  programmatically are lost, and a builder built from scratch moves as an
  empty document. Do not move or return an `XmlDomBuilder` by value (from
  reading the code; not checked at runtime).
- `xml_document` embeds a 64 KiB static memory pool
  (`RAPIDXML_STATIC_POOL_SIZE`), so each stack-allocated document uses about
  64 KiB of stack.
- `XmlElementBuilder::addAttribute(std::string_view, std::string_view)`
  copies `size() + 1` bytes from `data()`. It assumes the view is
  NUL-terminated, so a substring view copies one byte too many and the
  stored string has no terminator (from reading the code; not checked at
  runtime). Pass `std::string` or literals.

**Parsing semantics**
- `SafeXmlParser::parseString`/`tryParseString` ignore their `flags`
  argument and always use `parse<0>` (= `parse_default`). With the default
  flags, no declaration, comment, DOCTYPE or PI nodes are created, so
  `doc.first_node()` is the root element. Whitespace is not trimmed:
  element text keeps its surrounding blanks and newlines.
- Element and attribute names are matched **case-sensitively**
  (`first_node(name)`, `first_attribute(name)`).
- `getChildren(parent, name)` / `first_node(name)` filter by name, but with
  `name == nullptr` they return *all* children, including data nodes. Check
  `type() == node_element`.

**Typed conversion is inconsistent**
- `parseValue<int>` uses `std::from_chars`. It **rejects leading spaces and a
  leading `+`**, and it **accepts trailing junk** (`"3abc"` → 3).
- `parseValue<double>` uses `std::stod`. It accepts leading whitespace and
  ignores trailing junk (`"1.5xyz"` → 1.5).
- `parseValue<bool>` accepts `true/false/1/0/yes/no` in any case.
- `xmlBase::XmlParser::getDoubleAttribute` returns `0.0` both for a missing
  attribute and for an unparsable one, with no way to tell them apart.
- `XmlResult<T>::value()` on an error result throws
  `std::bad_variant_access`, not an `XmlException`. Check `isSuccess()` or
  use `valueOr`.

**Output**
- `XmlPrintOptions::sortAttributes` defaults to **true**, so files written
  through `XmlPrinter`/`XmlDomBuilder::saveToFile` list attributes
  alphabetically, not in insertion order. This matters when diffing model
  XML against reference files.
- With the default `trimWhitespace`, whitespace-only text nodes are dropped
  on output.

**IFile** (from reading `src/IFile.cxx`; not checked at runtime)
- Section names form **one flat namespace**. A nested `<section>` is
  stored at the top level under its own name. A duplicate name silently
  replaces the earlier section, and the earlier section is leaked.
- `addSection` sets `curSection_` for a nested section and does not restore
  it. Items that come *after* a nested section in the parent are filed
  under the nested section.
- `IFileException` does not derive from `std::exception`. Errors are also
  printed to `std::cerr` before the throw (`FATAL_MACRO`).
- `getIntVector`/`getDoubleVector` copy into fixed 1024-byte buffers, and
  `stripBlanks` into 1000-byte buffers.

**Dead code**
- `EResolver.h`, `docMan/`, `RapidXmlWrapper.h` and `XmlErrorHandler.h` are
  installed, but including any of them fails to compile or conflicts with
  the live headers. Do not include them.

## Extension points

- **New typed conversion:** add a `SafeXmlParser::parseValue<T>`
  specialization at the end of `safe_xml_parser.hpp` (and if needed a
  `ValueConverter::toString` case in `xml_builder.h` for writing).
  `getAttributeValue<T>`/`getNodeValue<T>` pick it up automatically.
- **New output option:** add a field to `XmlPrintOptions` and handle it in
  `XmlPrinter::printNodeInternal` and the functions it calls.
- **Cleanup candidates:** remove the dead headers and sources listed
  above, and `test_xmlbase/`; fix `XmlDomBuilder`'s move operations; decide
  whether `IFile` (unused) should stay.

All changes here are header changes. They rebuild `optimizers`, `flux`,
`Likelihood` and everything that includes their headers. Run
`test_optimizers` and the Likelihood tests, which round-trip XML models.

## Further reading

- `xmlBase/rapidxml.hpp`: the embedded RapidXML 1.13 manual comments. The
  [RapidXML manual](https://rapidxml.sourceforge.net/manual.html) covers
  parse flags and in-situ parsing.
- `optimizers/src/FunctionFactory.cxx` (ScienceTools `src/optimizers/`): a
  full read/write example using `XmlDomBuilder`.
- `Likelihood/src/SourceFactory.cxx` (ScienceTools `src/Likelihood/`):
  reading source models with `SafeXmlParser`.
- `git log` in this repo, PR #8 (`xerces-replacement`): the history of the
  migration. `doc/release.notes` and `src/mainpage.h` describe the
  Xerces-era package.
