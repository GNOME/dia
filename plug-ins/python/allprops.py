#    PyDia Self Documentation Series - Part IV : All Objects Properties
#    Copyright (c) 2008, Hans Breuer <hans@breuer.org>
#
#        generates a new diagram which contains all the currently
#    known std-props, that is iterates all objects for their properties
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
	
def allprops_cb(data, flags) :

	# copied from otypes.py
	if data :
		diagram = None # we may be running w/o GUI
	else :
		diagram = dia.new("All Object Properties.dia")
		data = diagram.data
	layer = data.active_layer

	props_by_name = {}
	otypes = dia.registered_types()
	name_type_clashes = []

	for oname in otypes :
		try :
			obj, h1, h2 = dia.get_object_type(oname).create (0, 0)
		except :
			print "Huh?", oname
			continue
		prop_keys = obj.properties.keys()
		for k in prop_keys :
			p = obj.properties[k]
			if props_by_name.has_key(k) :
				# check if it is the same type
				p0, names = props_by_name[k]
				try :
					if p0.type != p.type :
						# construct a unique name
						uname = p.name + "<" + p.type + ">"
						if props_by_name.has_key(uname) :
							props_by_name[uname][1].append(oname)
						else :
							props_by_name[uname] = (p, [oname])
							name_type_clashes.append (oname + " as " + p0.type + " and " + p.type)
					else :
						# remember the origin of the propety
						props_by_name[k][1].append(oname)
				except KeyError :
					print oname, "::", k, p, "?"
			else :
				props_by_name[k] = (p, [oname])
		obj.destroy() # unsave delete, any method call will fail/crash afterweards
		obj = None

	grid = {} # one type per row
	dx = 1.0
	dy = 5.0
	ot = dia.get_object_type("UML - Class")

	props_keys = props_by_name.keys()
	# alpha-numeric sorting by type; after by number of users
	props_keys.sort (lambda a,b : len(props_by_name[b][1]) - len(props_by_name[a][1]))
	props_keys.sort (lambda a,b : cmp(props_by_name[a][0].type, props_by_name[b][0].type))
	
	almost_all = 98 * len(otypes) / 100 # 98 %

	for pname in props_keys :
	
		p, names = props_by_name[pname]

		x = 0.0
		y = 0.0
		if grid.has_key(p.type) :
			x, y = grid[p.type]
		else :
			x = 0.0
			y = len(grid.keys()) * dy
		o, h1, h2 = ot.create (x,y)
		o.properties["name"] = pname
		o.properties["template"] = 1
		o.properties["templates"] = [(p.type, '')]
		# coloring depending on use
		if len(names) > almost_all :
			o.properties["fill_colour"] = "lightgreen"
		elif len(names) > 100 :
			o.properties["fill_colour"] = "lightblue"
		elif len(names) > 2 :
			o.properties["fill_colour"] = "lightcyan"
		elif len(names) > 1 :
			o.properties["fill_colour"] = "lightyellow"
		# if there is only one user show it
		if len(names) == 1 :
			o.properties["comment"] = names[0]
			o.properties["visible_comments"] = 1
			o.properties["comment_line_length"] = 60
		else :
			o.properties["comment"] = string.join(names, "; ")
			o.properties["visible_comments"] = 0
			o.properties["comment_line_length"] = 60

		# store position for next in row
		x += (dx + o.properties["elem_width"].value)
		grid[p.type] = (x,y)

		layer.add_object(o)
	layer.update_extents()
	data.update_extents()
	if diagram :
		diagram.display()
		diagram.flush()
	if len(name_type_clashes) > 0 :
		dia.message(0, "One name, one type?!\n" + string.join(name_type_clashes, "\n"))
	return data

dia.register_action ("HelpAllPropts", "All Object Properties",
                     "/ToolboxMenu/Help/HelpExtensionStart", 
                     allprops_cb)
