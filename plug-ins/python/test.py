import sys, dia

fp = open('/tmp/dia.python', 'w')

fp.write(`dir(sys)` + '\n\n')
fp.write(`sys.builtin_module_names` + '\n\n')
fp.write(`dir(dia)` + '\n\n')

fp.close
