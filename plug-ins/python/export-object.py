import sys, dia

import gettext
_ = gettext.gettext

# sys.path.insert(0, 'd:/graph/dia/dia')

class ObjRenderer :
	def __init__ (self) :
		pass
	def begin_render (self, data, filename) :
		self.f = open(filename, "w")
		for layer in data.layers :
			self.f.write ("<layer name=\"" + layer.name + "\">\n")
			for o in layer.objects :
				self.f.write ("\t"*1 + str(o) + "\n")
				r = o.bounding_box
				self.f.write ("\t"*2 + "<bbox>" + str(r.left) + "," + str(r.top) + ";" \
						+ str(r.right) + "," + str(r.bottom) + "</bbox>\n")

				self.f.write ("\t"*2 + "<connections>\n")
				for c in o.connections :
					if (len(c.connected) < 1) :
						continue
					self.f.write ("\t"*3 + str(c) + "\n")
					for co in c.connected :
						self.f.write ("\t"*4 + str(co) + "\n")
					self.f.write ("\t"*3 + "</DiaConnectionPoint>\n")
				self.f.write ("\t"*2 + "</connections>\n")

				self.f.write ("\t"*2 + str(o.properties) + "\n")
				keys = list(o.properties.keys())
				for s in keys :
					self.f.write ("\t"*3 + str(o.properties[s]) + "\n")
					if o.properties[s].type == "string" :
						self.f.write ("\t"*3 + str(o.properties[s].value) + "\n")
					self.f.write ("\t"*3 + "</DiaProperty>\n")
				self.f.write ("\t"*2 + "</DiaProperties>\n")
				self.f.write ("\t"*1 + "</DiaObject>\n\n")
			self.f.write ("\t"*0 + "</layer>\n")
	def end_render (self) :
		self.f.close()

# dia-python keeps a reference to the renderer class and uses it on demand
dia.register_export (_("PyDia Object Export"), "diapyo", ObjRenderer())
