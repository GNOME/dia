#    PyDia Self Documentation Series - Part I : PyDia itself
#    Copyright (c) 2003, Hans Breuer <hans@breuer.org>
#
#        generates a new diagram which contains all objects
#    of dir(dia). If it finally is possible to add attributes and operations
#    to UML classes it may even become useful ...

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

import sys, math, dia

def distribute_objects (objs) :
	width = 0.0
	height = 0.0
	for o in objs :
		if width < o.properties["elem_width"].value :
			width = o.properties["elem_width"].value
		if height < o.properties["elem_height"].value : 
			height = o.properties["elem_height"].value
	# add 20 % 'distance'
	width *= 1.2
	height *= 1.2
	area = len (objs) * width * height
	max_width = math.sqrt (area)
	x = 0.0
	y = 0.0
	for o in objs :
		o.move (x, y)
		x += width
		if x > max_width :
			x = 0.0
			y += height

def autodoc_cb (data, flags) :
	diagram = dia.new("PyDiaObjects.dia")
	layer = diagram.active_layer

	# add UML classes for every object in dir
	oType = dia.get_object_type ("UML - Class")		
	theDir = dir(dia)
	for s in theDir :
		o, h1, h2 = oType.create (0,0) # p.x, p.y
		layer.add_object (o)
		#layer.add_object_at (o, pos)

		# set the objects name
		o.properties["name"] = s
		
		# now populate the object with ...
		# ... attributes and
		# ... methods
	# all objects got there bounding box, distribute them
	distribute_objects (layer.objects)

	diagram.display()
	diagram.update_extents()
	diagram.flush()

dia.register_callback ("PyDia Auto Documentation", 
                       "<Toolbox>/Help/Self Doc/PyDia Docs", 
                       autodoc_cb)
