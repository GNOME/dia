import sys, os
import dia

# Please don't include pygtk or gtk here. There
# are quite some PyDia plug-ins which don't require
# i.e. dont have any (Py)Gtk code

loaded = []

def load(plugindir):
    global loaded
    if os.path.isdir(plugindir):
        sys.path.insert(0, plugindir)
        for file in os.listdir(plugindir):
            if file[-3:] == '.py' and file[:-3] not in loaded:
                try:
                    __import__(file[:-3])
                    loaded.append(file[:-3])
                except Exception as e:
                    sys.stderr.write('could not import %s [%s]\n' % (file, e))

# import any python plugins from the user ...
if not os.environ.has_key('HOME'):
    os.environ['HOME'] = os.pathsep + 'tmp'
    os.environ['XDG_DATA_HOME'] = os.pathsep + 'tmp'
else:
    default = os.path.join(os.environ['HOME'], '.local', 'share')
    oldlocation = os.path.join(os.environ['HOME'], '.dia')
    newlocation = os.path.join (
        os.getenv ('XDG_DATA_HOME', default),
        'dia'
    )
    if os.path.isdir(oldlocation):
        try:
            os.rename(oldlocation, newlocation)
        except OSError:
            sys.stderr.write('Could not migrate python scripts from \
                "%s" to "%s". Check if %s is empty.' %
            (oldlocation, newlocation, newlocation))

# import all plugins found in user plugin dir
load(os.path.join(os.environ['XDG_DATA_HOME'], 'dia', 'python'))

# find system python plugin dir
curdir = os.path.dirname(__file__)
plugindir = os.path.join(curdir, 'python')
#If we're running from source there is no plugin dir
if not os.path.exists(plugindir):
    plugindir = curdir
load(plugindir)
