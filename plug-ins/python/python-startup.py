import sys, os
import dia

# Please don't include pygtk or gtk here. There
# are quite some PyDia plug-ins which don't require
# i.e. dont have any (Py)Gtk code

# find system python plugin dir
curdir = os.path.dirname(__file__)
plugindir = os.path.join(curdir, 'python')

if os.path.isdir(plugindir):
    # import all plugins found in system plugin dir
    sys.path.insert(0, plugindir)
    for file in os.listdir(plugindir):
	if file[-3:] == '.py':
	    try:
		__import__(file[:-3])
	    except:
		sys.stderr.write('could not import %s\n' % file)

# import any python plugins from the user ...
if not os.environ.has_key('HOME'):
    os.environ['HOME'] = os.pathsep + 'tmp'
plugindir = os.path.join(os.environ['HOME'], '.dia', 'python')

if os.path.isdir(plugindir):
    # import all plugins found in user plugin dir
    sys.path.insert(0, plugindir)
    for file in os.listdir(plugindir):
	if file[-3:] == '.py':
	    try:
		__import__(file[:-3])
	    except:
		sys.stderr.write('could not import %s\n' % file)
