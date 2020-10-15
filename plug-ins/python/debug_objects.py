import sys, dia

import gettext
_ = gettext.gettext

# sys.path.insert(0, 'd:/graph/dia/dia')

def dia_debug_cb (data, flags) :
	"gets passed in the active diagram, flags are unused at the moment"
	for layer in data.layers :
		print("Layer :", layer.name)
		for o in layer.objects :
			print(str(o), str(o.bounding_box))

def dia_debug_props_cb (data, flags) :
	for layer in data.layers :
		print("Layer :", layer.name)
		for o in layer.objects :
			print(str(o))
			props = o.properties
			for s in list(props.keys()) :
				print(props[s].type + " " + s + " (visible=%d)" % props[s].visible)
				try :
					p = props[s].value
				except :
					p = None
				print("\t" + str(p))

# dia-python keeps a reference to the renderer class and uses it on demand
dia.register_action ("DebugBoundingbox", _("Dia _BoundingBox Debugger"),
                     "/DisplayMenu/Debug/DebugExtensionStart",
                     dia_debug_cb)

dia.register_action ("DebugProperty", _("Dia _Property API Debugger"),
                     "/DisplayMenu/Debug/DebugExtensionStart",
                     dia_debug_props_cb)
