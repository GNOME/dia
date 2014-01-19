import os, sys

# a global object to make it changeable by main()
format_extensions = ["drs", "svg", "png"]

if sys.platform == "win32" :
	print "Adjusting PATH ..."
	sys.path.insert(0, r'd:\graph\dia2\python')
	sys.path.insert(0, r'd:\graph\dia2\bin')
	sys.path.insert(0, r'..\plug-ins\python')
	#format_extensions.insert (0, "wmf")
else : # sorry only Linux and win32 tested ;)
	sys.path.insert (0, os.getcwd() + "/.libs")
	sys.path.insert(0, r'../plug-ins/python')

import dia


def Dump () :
	print dir(dia)

	r = dia.Rectangle
	print r, dir(r)
	o = dia.Object
	print o, dir(o)
	rd = dia.Renderer
	print rd, dir(rd)

if sys.platform == "win32" :
	os.environ["DIA_LIB_PATH"] = r"d:\graph\dia2\dia"
else :
	print "FIXME: trouble with dynamic loading on '%s'?" % (sys.platform,)
	base_path = os.getcwd() + "/.."
	os.environ["DIA_LIB_PATH"] = base_path + "/objects//:" + base_path + "/plug-ins//"
	os.environ["DIA_SHAPE_PATH"] = base_path + "/shapes//"
	os.environ["DIA_XSLT_PATH"] = base_path + "/plug-ins/xslt"

try :
	dia.register_plugins()
except AttributeError :
	print "Wrong '%s' picked up?" % (dia.__file__, )

def Export (name, data) :
	# write data to file, the format is choosen here
	for ext in format_extensions :
		ef = dia.filter_guess_export_filter (name + "." + ext)
		if ef :
			ef.do_export (data, name + "." + ext)
			break
	
def Import () :
	data = dia.DiagramData()
	import diasvg_import
	diasvg_import.import_svg (r"D:\graph\dia2\samples\render-test.svg", data)

	filename = "render-test-swig.svg"
	ef = dia.filter_export_get_by_name ("svg")
	if not ef :
		print "Guessing ..."
		ef = dia.filter_guess_export_filter ("dummy.png")
		filename = "render-test-swig.png"

	print ef, "\n", ef.description, "\n", dir(ef)

	ef.do_export (data, filename)

def Gx () :
	data = dia.DiagramData()

	import diagx
	diagx.ImportXml (r'D:\data\work\camera\Dp70Camera.xml', data)
	Export ("gx-test", data)

def Doc () :
	data = dia.DiagramData()

	import pydiadoc
	res = pydiadoc.autodoc_fresh_cb (data, 1)
	if res :
		data = res
	Export ("pydiadoc", data)

def Dox () :
	data = dia.DiagramData()

	import doxrev
	#doxrev.import_file (r'D:\data\work\dev\include\obscam\xml\classOBS_1_1ICamera.xml', data)
	doxrev.import_files (r'D:\data\work\dev\include\obscam\xml\classOBS_1_1ICamera.xml', data)
	data.update_extents()
	Export ("doxrev-test", data)

def Types () :
	data = dia.DiagramData()

	import otypes
	res = otypes.otypes_cb (data, 0)
	if res :
		data = res
	Export ("otypes", data)

def AObj () :
	data = dia.DiagramData()

	import aobjects
	data = aobjects.aobjects_cb (data, 0)
	Export ("aobjs", data)

def Gen () :
	data = dia.DiagramData()

	import gendia
	data = gendia.dia_generate_dia_cb (data, 0)
	Export ("gendia", data)
def DRaw () :
	data = dia.DiagramData()
	# create something interesting to save
	if 0 :
		import aobjects
		data = aobjects.aobjects_cb (data, 0)
	else :
		import pydiadoc
		data = pydiadoc.autodoc_cb (data, 0)
	import diaraw
	r = diaraw.DiaOutRenderer()
	r.begin_render (data, "diaraw.dia")
	r.end_render ()

def Props () :
	data = dia.DiagramData()
	import allprops
	allprops.allprops_cb(data,0)

def Self () :
	data = dia.DiagramData()
	r = data.extents
	try :
		data.extents = r
	except AttributeError, s :
		print "Expected except", s, r.top, r.left

for arg in sys.argv[1:] :
	if '--dump' == arg : Dump ()
	elif '--import' == arg : Import ()
	elif '--gx' == arg : Gx ()
	elif '--doc' == arg : Doc ()
	elif '--dox' == arg : Dox ()
	elif '--types' == arg : Types ()
	elif '--aobj' == arg : AObj ()
	elif '--gen' == arg : Gen ()
	elif '--self' == arg : Self ()
	elif '--draw' == arg : DRaw ()
	elif '--props' == arg : Props ()
	elif '--all' == arg :
		Dump()
		Import()
		Gx()
		Doc()
		Dox()
		Types()
		AObj ()
		Gen()
		#DRaw()
		Props()
	else : format_extensions.insert(0, arg)
