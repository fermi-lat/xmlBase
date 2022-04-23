####################################################
add_library(
  xmlBase SHARED
  src/Dom.cxx
  src/IFile.cxx
  src/XmlErrorHandler.cxx
  src/XmlParser.cxx
  src/EResolver.cxx
  src/docMan/DocMan.cxx
)
target_include_directories(
  xmlBase PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:>
)

target_link_libraries(xmlBase PUBLIC PkgConfig::xerces-c facilities)

add_executable(test_xmlBase src/main.cxx)
add_executable(entity_test src/test/entity_test.cxx)
add_executable(test_mem src/test/test_mem.cxx)
add_executable(test_IFile src/test/test_IFile.cxx)
add_executable(test_write src/test/test_write.cxx)
add_executable(test_parse src/test/test_parse.cxx)

target_link_libraries(test_xmlBase xmlBase)
target_link_libraries(entity_test xmlBase)
target_link_libraries(test_mem xmlBase)
target_link_libraries(test_IFile xmlBase)
target_link_libraries(test_write xmlBase)
target_link_libraries(test_parse xmlBase)

###############################################################
# Installation
###############################################################
install(DIRECTORY xmlBase DESTINATION ${FERMI_INSTALL_INCLUDEDIR})
install(DIRECTORY xml/ DESTINATION ${FERMI_INSTALL_XMLDIR}/fermi/xmlBase/)

install(
  TARGETS xmlBase entity_test test_mem test_IFile test_write test_parse test_xmlBase
  EXPORT fermiTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
