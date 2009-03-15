#    PyDia Self Documentation Series - Part III : All Objects
#    Copyright (c) 2007, Hans Breuer <hans@breuer.org>
#
#        generates a new diagram which contains all the currently
#    registered objects sorted to layers by their containing package
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

def set_object_string (o) :
	keys = o.properties.keys()
	for s in keys :
		p = o.properties[s]
		if p.type in ["string", "text"] :
			if s in ["name", "text"] :
				o.properties[s] = o.type.name
			else :
				o.properties[s] = s

def aobjects_cb(data, flags) :

	# copied from otypes.py
	if data :
		diagram = None # we may be running w/o GUI
	else :
		diagram = dia.new("All Objects.dia")
		data = diagram.data
	layer = data.active_layer

	otypes = dia.registered_types()
	keys = otypes.keys()
	keys.sort()

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
			packages[sp].append(s)
		else :
			packages[sp] = [s] 

	for sp in packages.keys() :
		# add a layer per package
		layer = data.add_layer (sp)

		cx = 0.0
		cy = 0.0
		n = 0 # counting objects
		my = 0.0
		pkg = packages[sp]
		for st in pkg :
			if st == "Group" :
				continue # can't create empty group
			#print st
			o, h1, h2 = dia.get_object_type(st).create (cx, cy)
			# to make the resulting diagram more interesting we set every sting property with it's name
			set_object_string (o)
			w = o.bounding_box.right - o.bounding_box.left
			h = o.bounding_box.bottom - o.bounding_box.top
			o.move (cx, cy)
			cx += w * 1.5
			if h > my : my = h
			n += 1
			if n % 10 == 0 :
				cx = 0.0
				cy += my * 1.5
				my = 0
			layer.add_object (o)
		layer.update_extents()
	data.update_extents()
	if diagram :
		diagram.display()
		diagram.flush()
	return data

dia.register_action ("HelpAObjects", "All Objects",
                     "/ToolboxMenu/Help/HelpExtensionStart", 
                     aobjects_cb)
