import glob,os,platform

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

xmlBaseLib = libEnv.SharedLibrary('xmlBase', [ 'src/Dom.cxx', 'src/IFile.cxx', 'src/XmlErrorHandler.cxx', 'src/XmlParser.cxx', 'src/EResolver.cxx', 'src/docMan/DocMan.cxx'])

progEnv.Tool('xmlBaseLib')
entity_test = progEnv.Program('entity_test', 'src/test/entity_test.cxx')
test_xmlBaseBin = progEnv.Program('test_xmlBase', 'src/main.cxx')
test_memBin = progEnv.Program('test_mem', 'src/test/test_mem.cxx')
test_IFileBin = progEnv.Program('test_IFile', 'src/test/test_IFile.cxx')
test_writeBin = progEnv.Program('test_write', 'src/test/test_write.cxx')

progEnv.Tool('registerObjects', package = 'xmlBase', libraries = [xmlBaseLib], testApps = [entity_test, test_xmlBaseBin, test_memBin, test_IFileBin, test_writeBin], includes = listFiles(['xmlBase/*.h']))