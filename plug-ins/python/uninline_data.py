#   Un-inline (i.e. save) embedded images
#
#   Copyright (c) 2011, Hans Breuer <hans@breuer.org>
#
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

import os, string, sys, dia

import gettext
_ = gettext.gettext

class UninlineRenderer :
	def __init__ (self) :
		self.count = 0
	def begin_render (self, data, filename) :
		imgmap = {}
		dirname = os.path.dirname (filename)
		basename = os.path.basename(filename)
		ext = filename[filename.rfind(".")+1:]
		for layer in data.layers :
			for o in layer.objects :
				if "inline_data" in list(o.properties.keys()) :
					if o.properties["inline_data"].value :
						# remember by position
						pos = o.properties["obj_pos"].value
						xk = "%03g" % pos.x
						yk = "%03g" % pos.y
						key = basename + "-" + layer.name + "x" + xk + "y" + yk
						imgmap[key] = o
			for k, o in imgmap.items() :
				fname = dirname + "/" + k + "." + ext
				print(fname)
				o.properties["image_file"] = fname
				o.properties["inline_data"] = 0

	def end_render (self) :
		pass

# dia-python keeps a reference to the renderer class and uses it on demand
dia.register_export (_("Uninline Images"), "png", UninlineRenderer())
