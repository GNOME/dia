import sys, dia

sys.path.insert(0, '/home/james/gnomecvs/dia/plug-ins/python')
sys.argv = [ 'dia-python' ]

import gtkcons

fp = open('/tmp/dia.python', 'w')

fp.write(`dir(sys)` + '\n\n')
fp.write(`sys.builtin_module_names` + '\n\n')
fp.write(`dir(dia)` + '\n\n')

fp.close

gtkcons.gtk_console({'__builtins__': __builtins__, '__name__': '__main__',
		     '__doc__': None, 'dia': dia}, 'Python Dia Console')
