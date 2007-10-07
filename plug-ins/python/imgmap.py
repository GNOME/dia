#    PyDia imgmap.py : produce an html image map 
#    Copyright (c) 2007  Hans Breuer  <hans@breuer.org>

#    This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

import dia, sys, os.path, string

class ObjRenderer :
	def __init__ (self) :
		self.f = None
		# the bintmap exporters calculate their ow margins from the bunding box
		self.xofs = 0
		self.yofs = 0
	def begin_render (self, data, filename) :
		self.f = open(filename, "w")
		bMapLayer = 0 # if there is a special layer containing the links
		for layer in data.layers :
			if layer.name == "map" :
				bMapLayer = 1
				break
		scale = 20.0 * data.paper.scaling
		r = data.extents
		width = int((r.right - r.left) * scale)
		height = int((r.bottom - r.top) * scale)
		name = os.path.split (filename)[1]
		fname = name[:string.find(name, ".")] + ".png" # guessing
		self.f.write ('<image src="%s" width="%d", height="%d" usemap="#%s">\n' % (fname, width, height, name))
		self.f.write ('<map name="%s">\n' % (name,))
		
		self.xofs = - (r.left * scale)
		self.yofs = - (r.top * scale)
		if bMapLayer :
			self.WriteAreas (layer, scale)
		else :
			for layer in data.layers :
				self.WriteAreas (layer, scale)
		self.f.write ('</map>\n')

	def WriteAreas (self, layer, scale) :
		for o in layer.objects :
			r = o.bounding_box
			if o.properties.has_key ("name") :
				url = o.properties["name"].value
			elif o.properties.has_key ("text") :
				url = o.properties["text"].value.text
			else :
				continue
			if len(url) == 0 or string.find (url, " ") >= 0 :
				continue

			alt = url
			if o.properties.has_key ("comment") :
				alt = o.properties["comment"].value
			# need to sale the original coords to the bitmap size
			x1 = int(r.left * scale) + self.xofs
			y1 = int(r.top * scale) + self.yofs
			x2 = int(r.right * scale) + self.xofs
			y2 = int(r.bottom * scale) + self.yofs
			self.f.write ('  <area shape="rect" href="%s" title="%s" alt="%s" coords="%d,%d,%d,%d">\n' % \
				 ("#" + url, url, alt, x1, y1, x2, y2))
	def end_render (self) :
		self.f.close()

# dia-python keeps a reference to the renderer class and uses it on demand
dia.register_export ("Imagemap", "cmap", ObjRenderer())
