#$Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/xmlBase/xmlBaseLib.py,v 1.4 2009/01/23 00:21:04 ecephas Exp $
def generate(env, **kw):
    if not kw.get('depsOnly',0):
        env.Tool('addLibrary', library = ['xmlBase'], package = 'xmlBase')
        if env['PLATFORM']=='win32' and env.get('CONTAINERNAME','')=='GlastRelease':
            env.Tool('findPkgPath', package = 'xmlBase') 

    env.Tool('facilitiesLib')
    env.Tool('addLibrary', library = env['xercesLibs'])

def exists(env):
    return 1
