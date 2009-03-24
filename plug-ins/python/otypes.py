#    PyDia Self Documentation Series - Part II : Object Types
#    Copyright (c) 2003, Hans Breuer <hans@breuer.org>
#
#        generates a new diagram which contains all the currently
#    registered object types sorted by their containing package
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

import sys, dia, string

def _log(s, append=1) :
	pass
	if append :
		mode = "a"
	else :
		mode = "w"
	f = open("c:\\temp\\otypes.log", mode)
	f.write(s)

def otypes_cb(data, flags) :

	if data :
		diagram = None # we may be running w/o GUI
	else :
		diagram = dia.new("Object Types.dia")
		data = diagram.data
	layer = data.active_layer

	otypes = dia.registered_types()
	keys = otypes.keys()
	keys.sort()

	# property keys w/o overlap
	object_props = ["obj_pos", "obj_bb", "meta"]
	element_props = ["elem_corner", "elem_width", "elem_height"]
	orthconn_props = ["orth_points", "orth_orient", "orth_autoroute"]
	shape_props = ["flip_horizontal", "flip_vertical"]
	# the following are not exclusuve to any objects type
	line_props = ["line_width", "line_style", "line_colour"]
	fill_props = ["fill_colour", "show_background"]
	text_props = ["text_colour", "text_font", "text_height", "text"] # "text_alignment", "text_pos"

	packages = {}
	for s in keys :
		kt = string.split(s, " - ")
		if len(kt) == 2 :
			if len(kt[0]) == 0 :
				sp = "<unnamed>"
			else :
				sp = kt[0]
			st = kt[1]
		else :
			sp = "<broken>"
			st = kt[0]
		if packages.has_key(sp) :
			packages[sp].append(st)
		else :
			packages[sp] = [st] 

	dtp = dia.get_object_type("UML - LargePackage")
	dtc = dia.get_object_type("UML - Class")
	cy = 0
	maxy = 0
	maxx = 0

	for sp in packages.keys() :
		pkg = packages[sp]
		op, h1, h2 = dtp.create(0.0, cy + 1.0)
		op.properties["name"] = sp
		layer.add_object(op)
		cx = 0
		for st in pkg :
			if st == "Group" :
				continue # too special to handle
			oc, h3, h4 = dtc.create(cx + 1.0, cy + 4.0)
			oc.properties["name"] = st
			attrs = []
			# we detect inheritance by common props
			n_object = 0
			n_element = 0
			n_orthconn = 0
			n_shape = 0
			n_line = 0
			n_fill = 0
			n_text = 0
			if otypes.has_key(st) :
				o_real, h5, h6 = dia.get_object_type(st).create(0,0)
			elif otypes.has_key(sp + " - " + st) :
				o_real, h5, h6 = dia.get_object_type(sp + " - " + st).create(0,0)
			else :
				o_real = None
				print "Failed to create object", sp, st
			formal_params = []
			if not o_real is None :
				for p in o_real.properties.keys() :
					if p in object_props : n_object = n_object + 1
					elif p in orthconn_props : n_orthconn = n_orthconn + 1
					elif p in element_props : n_element = n_element + 1
					elif p in shape_props : n_shape = n_shape + 1
					elif p in line_props : n_line = n_line + 1
					elif p in text_props : n_text = n_text + 1
					elif p in fill_props : n_fill = n_fill + 1
					else : # don't replicate common props
						attrs.append((p, o_real.properties[p].type, '', '', 0, 0, 0))
				if n_line == len(line_props) :
					formal_params.append(('Line', ''))
				else : # need to add the incomplete set
					for pp in line_props : 
						if o_real.properties.has_key(pp) :
							attrs.append((pp, o_real.properties[pp].type, '', '', 0, 0, 0))
				if n_fill == len(fill_props) :
					formal_params.append(('Fill', ''))
				else :
					for pp in fill_props : 
						if o_real.properties.has_key(pp) :
							attrs.append((pp, o_real.properties[pp].type, '', '', 0, 0, 0))
				if n_text == len(text_props) :
					formal_params.append(('Text', ''))
				else :
					for pp in text_props : 
						if o_real.properties.has_key(pp) :
							attrs.append((pp, o_real.properties[pp].type, '', '', 0, 0, 0))
			if n_orthconn == len(orthconn_props) :
				oc.properties["stereotype"] = "OrthConn"
				oc.properties["fill_colour"] = "light blue"
			elif n_shape == len(shape_props) :
				oc.properties["stereotype"] = "Shape"
				oc.properties["fill_colour"] = "light cyan"
			elif n_element == len(element_props) :
				oc.properties["stereotype"] = "Element"
				oc.properties["fill_colour"] = "light yellow"
			elif n_object == len(object_props) :
				oc.properties["stereotype"] = "Object"
			else :
				print "Huh?", st
				oc.properties["fill_colour"] = "red"
			oc.properties["attributes"] = attrs
			if len(formal_params) > 0 :
				oc.properties["template"] = 1
				oc.properties["templates"] = formal_params
			layer.add_object(oc)
			# XXX: there really should be a way to safely delete an object. This one will crash:
			# - when the object got added somewhere 
			# - any object method gets called afterwards
			if not o_real is None :
				o_real.destroy()
				del o_real
			cx = oc.bounding_box.right
			if maxy < oc.bounding_box.bottom :
				maxy = oc.bounding_box.bottom
			if maxx < cx :
				maxx = cx
			# wrapping too long lines
			if cx > 300 :
				cx = 0
				cy = maxy
		h = op.handles[7]
		# adjust the package size to fit the objects
		op.move_handle(h,(maxx + 1.0, maxy + 1.0), 0, 0)
		cy = maxy + 2.0
		maxx = 0 # every package a new size
	data.update_extents()
	if diagram :
		diagram.display()
		diagram.flush()
	# make it work standalone
	return data

dia.register_action ("HelpOtypes", "Dia Object Types",
                     "/ToolboxMenu/Help/HelpExtensionStart", 
                     otypes_cb)
