def generate(env, **kw):
    if not kw.get('depsOnly',0):
        env.Tool('addLibrary', library = ['xmlBase'], package = 'xmlBase')
    env.Tool('facilitiesLib')
    env.Tool('addLibrary', library = env['xercesLibs'])

def exists(env):
    return 1
